# RFC: pcpoint libpc exposure — serialize/deserialize without PostgreSQL

> **Discussion [#869](https://github.com/MobilityDB/MobilityDB/discussions/869)** — MobilityDB community context  
> **Upstream PR** — to be filed against [`pgpointcloud/pointcloud`](https://github.com/pgpointcloud/pointcloud)

## The Problem

MobilityDB's `tpcpoint` / `tpcpatch` types lift pgPointCloud's `pcpoint` and `pcpatch`
into the temporal framework. Temporal operators such as `atTpcbox`, `eIntersects`, and
per-point predicates need to **decompose a serialized patch into its constituent points
in C** — without going through the SQL layer.

The in-memory API in `lib/pc_api.h` already exists:

```c
PCPATCH     *pc_patch_uncompress(const PCPATCH *patch);
PCPOINTLIST *pc_pointlist_from_patch(const PCPATCH *patch);
PCPOINT     *pc_pointlist_get_point(PCPOINTLIST *list, int i);
```

But the bridge between the **on-disk varlena form** and these in-memory structs is trapped
inside `pgsql/pc_pgsql.c`, inaccessible to any caller that does not link the full PG extension:

| Function | Header | Library |
|---|---|---|
| `pc_patch_deserialize` | `pgsql/pc_pgsql.h` | `pointcloud-1.2.so` only |
| `pc_patch_serialize` | `pgsql/pc_pgsql.h` | `pointcloud-1.2.so` only |
| `pc_point_deserialize` | `pgsql/pc_pgsql.h` | `pointcloud-1.2.so` only |
| `pc_point_serialize` | `pgsql/pc_pgsql.h` | `pointcloud-1.2.so` only |

Three consequences for external consumers:

1. **Header dependency** — `pgsql/pc_pgsql.h` includes `<postgres.h>`, making it unlinkable
   from any library that does not include the PostgreSQL headers (MEOS, standalone C programs,
   language bindings).
2. **Library gap** — the symbols live only in the PG extension `.so`; they are absent from
   `libpc.a`, so static-link consumers must vendor the function bodies.
3. **Symbol visibility** — even at runtime the symbols are local (`t`, not `T`), so
   `dlsym`-based access from another PG extension fails:
   ```
   $ nm pointcloud-1.2.so | grep pc_patch_deserialize
   0000000000010d87 t pc_patch_deserialize
   ```

The bodies contain no PostgreSQL-specific code — they manipulate the byte layout and
`PCSCHEMA` only. Moving them is mechanical.

---

## Why Now

MobilityDB's pgPointCloud integration (PR [#818](https://github.com/MobilityDB/MobilityDB/pull/818))
adds the `tpcpoint` and `tpcpatch` temporal types. Per-point operations (`atTpcbox`,
`eIntersects`, per-dimension predicates) currently filter instants by their 2D `PCBOUNDS`
only — they cannot inspect per-point coordinates because MEOS links `libpc.a`, not the PG
extension shared object.

The current workaround is a SQL wrapper around `PC_Explode`:

```sql
-- current: correct but ~10× slower on dense patches
SELECT tpcpatch_from_pcpatch(
  PC_Patch(PC_FilterEquals(PC_Explode(patch), 'Intensity', threshold))
)
FROM tpcpatch_instants;
```

Every call re-decompresses through the SPI machinery. With the four functions promoted to
`libpc`, MEOS can instead:

1. Deserialize the patch in C (no SQL round-trip, no `PC_Explode` per instant).
2. Walk it via `pc_pointlist_from_patch` / `pc_pointlist_get_point` / `pc_point_get_x`.
3. Build a filtered `PCPATCH` and serialize it back via `pc_patch_serialize` in-place.

This unblocks the full per-point operator surface for all `tpcpatch` temporal functions,
and enables MEOS-standalone use of pgPointCloud data without a running PostgreSQL instance.

The analogy to the npoint portability problem (RFC [#863](https://github.com/MobilityDB/MobilityDB/discussions/863))
is exact: both are cases where a temporal type's core operations are blocked by a missing
library exposure in an upstream dependency.

---

## Proposal

Move the four serialize/deserialize functions from `pgsql/pc_pgsql.c` into a new
`lib/pc_serialize.c`, declare them in `lib/pc_api.h`, and make `pgsql/pc_pgsql.h`
forward to `pc_api.h`. No SQL surface changes; no on-disk format changes; no SOVERSION bump.

### 1. New file `lib/pc_serialize.c`

Contains the bodies of (moved verbatim from `pgsql/pc_pgsql.c`):

- `pc_point_serialize`
- `pc_point_deserialize`
- `pc_patch_serialize` (and its helper `pc_patch_serialized_size`)
- `pc_patch_serialize_to_uncompressed`
- `pc_patch_deserialize`

### 2. Additions to `lib/pc_api.h`

```c
/* Serialized on-disk layouts — promoted from pgsql/pc_pgsql.h */
typedef struct {
  uint32_t size;
  uint32_t pcid;
  uint8_t  data[1];
} SERIALIZED_POINT;

typedef struct {
  uint32_t size;
  uint32_t pcid;
  uint32_t compression;
  uint32_t npoints;
  PCBOUNDS bounds;
  uint8_t  data[1];
} SERIALIZED_PATCH;

extern SERIALIZED_POINT *pc_point_serialize(const PCPOINT *pcpt);
extern PCPOINT          *pc_point_deserialize(const SERIALIZED_POINT *serpt,
                                              const PCSCHEMA *schema);
extern size_t            pc_patch_serialized_size(const PCPATCH *patch);
extern SERIALIZED_PATCH *pc_patch_serialize(const PCPATCH *patch, void *userdata);
extern SERIALIZED_PATCH *pc_patch_serialize_to_uncompressed(const PCPATCH *patch);
extern PCPATCH          *pc_patch_deserialize(const SERIALIZED_PATCH *serpatch,
                                              const PCSCHEMA *schema);
```

### 3. `pgsql/pc_pgsql.h` — forward to `pc_api.h`

Replace the redefined typedefs and duplicate declarations with:

```c
#include "pc_api.h"   /* SERIALIZED_POINT, SERIALIZED_PATCH, and the six functions */
```

Out-of-tree PG extensions that already include `pgsql/pc_pgsql.h` keep building
without changes — they now get the declarations from `pc_api.h` via the forwarder.

### 4. `pgsql/pc_pgsql.c` — drop moved bodies

PG-specific helpers (`pc_schema_from_pcid`, `pc_point_from_hexwkb`, etc.) remain in place.

### 5. Optional: CUnit round-trip test

A small `lib/cunit/cu_pc_serialize.c` that round-trips a synthetic `PCPATCH` through
serialize/deserialize **without `<postgres.h>` in scope**, proving the move is clean.

### Why serialize and deserialize (not just deserialize)

A consumer that reads patches almost always needs to write filtered patches back.
Exposing only deserialize forces vendoring of the serialize helpers anyway.
Exposing both also enables a future standalone `meos_pcpatch_filter` that avoids the
SPI round-trip entirely.

### Backwards compatibility

- PG extension behavior: unchanged — same call sites, same on-disk layout, same SQL surface.
- `pgsql/pc_pgsql.h`: continues to declare all symbols (now via the `pc_api.h` forwarder).
- `libpc.a` SOVERSION: no bump — purely additive to the public surface.
- `pointcloud-1.2.so` callers: unaffected.

---

## Alternatives Considered

| Approach | Why rejected |
|---|---|
| Vendor the four bodies inside MobilityDB | Duplicates pgPointCloud code; maintenance burden whenever pgPointCloud patches the functions |
| Route all filtering through `PC_Explode` SQL | Correct but ~10× slower on dense patches; requires SPI machinery even inside MEOS standalone |
| Copy `pgsql/pc_pgsql.c` into MEOS build | Pulls in PG headers, breaking MEOS's PostgreSQL-free build contract |
| Wait for pgPointCloud to expose a higher-level filter API | Unblocks per-point ops for MobilityDB specifically, but the lower-level serialize/deserialize exposure is useful to any external consumer |

---

## Open Questions

1. **Naming** — should the new file be `lib/pc_serialize.c` or `lib/pc_pgsql_compat.c`?
   The `SERIALIZED_*` prefix carries a hint of PG origin; a `pc_pgsql_compat.c` name
   would preserve that context. `pc_serialize.c` is more discoverable for new readers.

2. **Test coverage** — is a CUnit test (item 5 above) required for this PR to merge,
   or acceptable as a follow-up?

3. **PCSCHEMA ownership** — `pc_patch_deserialize` and `pc_point_deserialize` take a
   `PCSCHEMA *` that today is retrieved via `pc_schema_from_pcid` (which calls into PG).
   For MEOS standalone use, callers must supply their own schema. Should `pc_api.h`
   document this contract explicitly?

4. **Other hidden symbols** — are there additional functions in `pgsql/pc_pgsql.c` that
   external consumers need? A broader audit could be done in parallel or as a follow-up.

---

## Related

- MobilityDB Discussion [#869](https://github.com/MobilityDB/MobilityDB/discussions/869) — MobilityDB community context for this upstream ask
- MobilityDB PR [#818](https://github.com/MobilityDB/MobilityDB/pull/818) — pgPointCloud temporal types (`tpcpoint`/`tpcpatch`)
- RFC [#863](https://github.com/MobilityDB/MobilityDB/discussions/863) — npoint portability (analogous upstream-dependency problem)
- RFC [#830](https://github.com/MobilityDB/MobilityDB/issues/830) — TemporalParquet (uses `tpcpatch` in type coverage table)
- [`pgpointcloud/pointcloud`](https://github.com/pgpointcloud/pointcloud) — the upstream repository where the PR will be filed
