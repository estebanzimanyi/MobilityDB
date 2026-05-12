# RFC: pcpoint libpc exposure — serialize/deserialize without PostgreSQL

> Discussion: [#869](https://github.com/MobilityDB/MobilityDB/discussions/869)
> Upstream PR target: [`pgpointcloud/pointcloud`](https://github.com/pgpointcloud/pointcloud)

## The Problem

MobilityDB's `tpcpoint` / `tpcpatch` types lift pgPointCloud's `pcpoint` and `pcpatch`
into the temporal framework. Temporal operators such as `atTpcbox`, `eIntersects`, and
per-point predicates need to decompose a serialized patch into its constituent points
in C, without going through the SQL layer.

The in-memory API in `lib/pc_api.h` already exists:

```c
PCPATCH     *pc_patch_uncompress(const PCPATCH *patch);
PCPOINTLIST *pc_pointlist_from_patch(const PCPATCH *patch);
PCPOINT     *pc_pointlist_get_point(PCPOINTLIST *list, int i);
```

But the bridge between the on-disk varlena form and these in-memory structs is trapped
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
`PCSCHEMA` only.

## Proposal

Move the four serialize/deserialize functions from `pgsql/pc_pgsql.c` into a new
`lib/pc_serialize.c`, declare them in `lib/pc_api.h`, and make `pgsql/pc_pgsql.h`
forward to `pc_api.h`. No SQL surface changes, no on-disk format changes, no SOVERSION bump.

### 1. New file `lib/pc_serialize.c`

Contains the bodies of (moved verbatim from `pgsql/pc_pgsql.c`):

- `pc_point_serialize`
- `pc_point_deserialize`
- `pc_patch_serialize` (and its helper `pc_patch_serialized_size`)
- `pc_patch_serialize_to_uncompressed`
- `pc_patch_deserialize`

### 2. Additions to `lib/pc_api.h`

```c
/* Serialized on-disk layouts */
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

### 3. `pgsql/pc_pgsql.h` forwards to `pc_api.h`

```c
#include "pc_api.h"   /* SERIALIZED_POINT, SERIALIZED_PATCH, and the six functions */
```

Out-of-tree PG extensions that already include `pgsql/pc_pgsql.h` keep building without
changes — they get the declarations from `pc_api.h` via the forwarder.

### 4. `pgsql/pc_pgsql.c` drops moved bodies

PG-specific helpers (`pc_schema_from_pcid`, `pc_point_from_hexwkb`, etc.) remain in place.

### 5. CUnit round-trip test

`lib/cunit/cu_pc_serialize.c` round-trips a synthetic `PCPATCH` through serialize/deserialize
without `<postgres.h>` in scope, proving the move is clean.

### Backwards compatibility

- PG extension behavior: unchanged. Same call sites, same on-disk layout, same SQL surface.
- `pgsql/pc_pgsql.h`: continues to declare all symbols, now via the `pc_api.h` forwarder.
- `libpc.a` SOVERSION: no bump — purely additive to the public surface.
- `pointcloud-1.2.so` callers: unaffected.

## Related

- [Discussion #869](https://github.com/MobilityDB/MobilityDB/discussions/869)
- [PR #818](https://github.com/MobilityDB/MobilityDB/pull/818) — pgPointCloud temporal types (`tpcpoint`/`tpcpatch`)
- [Discussion #863](https://github.com/MobilityDB/MobilityDB/discussions/863) — npoint portability
- [RFC #830](https://github.com/MobilityDB/MobilityDB/issues/830) — TemporalParquet
- [`pgpointcloud/pointcloud`](https://github.com/pgpointcloud/pointcloud) — upstream repository
