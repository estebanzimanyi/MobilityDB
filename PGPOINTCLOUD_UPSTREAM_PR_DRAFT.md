# Upstream PR draft — expose `pc_patch_(de)serialize` / `pc_point_(de)serialize` from `libpc`

> Target repo: `pgpointcloud/pointcloud`
> Status: **draft** — not yet filed. Ask: move 4 serialization functions from `pgsql/pc_pgsql.c` into `lib/`, so they live in `libpc.a` and are linkable by external consumers.

## Problem

External consumers of pgPointCloud's library (MobilityDB's lifted
`tpcpatch` / `tpcpoint` types being the immediate motivator) need to be
able to **decompose a serialized patch into its constituent points in C**
without going back through the SQL layer.

Today's surface in `lib/pc_api.h` exposes the *in-memory* APIs:

- `PCPATCH *pc_patch_uncompress(const PCPATCH *patch)`
- `PCPOINTLIST *pc_pointlist_from_patch(const PCPATCH *patch)`
- `PCPOINT *pc_pointlist_get_point(PCPOINTLIST *list, int i)`

But the **bridge between the on-disk varlena form and the in-memory
PCPATCH / PCPOINT structs** is hidden inside `pgsql/pc_pgsql.c`:

| Function                  | Header                | Library      |
|---------------------------|-----------------------|--------------|
| `pc_patch_deserialize`    | `pgsql/pc_pgsql.h`    | only `pointcloud-1.2.so` |
| `pc_patch_serialize`      | `pgsql/pc_pgsql.h`    | only `pointcloud-1.2.so` |
| `pc_point_deserialize`    | `pgsql/pc_pgsql.h`    | only `pointcloud-1.2.so` |
| `pc_point_serialize`      | `pgsql/pc_pgsql.h`    | only `pointcloud-1.2.so` |

Three problems for non-PG consumers:

1. **Header location** — `pgsql/pc_pgsql.h` pulls in `<postgres.h>` and
   the PG headers, so external libraries that don't link PG can't
   include it.
2. **Library location** — the symbols are compiled only into the
   PG extension shared object. They don't exist in `libpc.a`, so
   anyone linking against the static library has to vendor the bodies.
3. **Symbol visibility** — even at runtime, `nm pointcloud-1.2.so`
   shows them as local (`t`, not `T`), so `dlsym`-based access from
   another extension fails:

   ```
   $ nm pointcloud-1.2.so | grep -E " (pc_patch_deserialize|pc_point_serialize)$"
   0000000000010d87 t pc_patch_deserialize
   000000000001004b t pc_point_serialize
   ```

The bodies themselves are pure manipulation of the byte layout +
`PCSCHEMA` — they don't reference PG types or fmgr machinery, so the
move is mechanical.

## Proposal

Move the four functions out of `pgsql/pc_pgsql.c` into a new
`lib/pc_serialize.c` (or `lib/pc_pgsql_compat.c` if the SERIALIZED_*
naming is to be preserved as a hint about origin), and declare them in
`lib/pc_api.h`.

Concretely:

1. **New file `lib/pc_serialize.c`** containing the bodies of:
   - `pc_point_serialize`
   - `pc_point_deserialize`
   - `pc_patch_serialize` (and its helper `pc_patch_serialized_size`)
   - `pc_patch_serialize_to_uncompressed`
   - `pc_patch_deserialize`

2. **In `lib/pc_api.h`** add forward declarations:
   ```c
   /* Serialized layouts — currently defined in pgsql/pc_pgsql.h,
      promote them here so non-PG callers can construct/parse them. */
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

3. **In `pgsql/pc_pgsql.h`**, replace the redefined typedefs with a
   single `#include "pc_api.h"` forwarder, and drop the duplicate
   declarations.

4. **In `pgsql/pc_pgsql.c`**, drop the now-moved bodies. The file
   shrinks; PG-specific helpers (`pc_schema_from_pcid`,
   `pc_point_from_hexwkb`, etc.) stay where they are.

5. Optional: add a small `lib/cunit/cu_pc_serialize.c` test that
   round-trips a synthetic `PCPATCH` through serialize/deserialize
   without `<postgres.h>` in scope, proving the move is clean.

## Why these four (and not just the deserialize pair)

For symmetry: a consumer that can read patches from disk almost always
wants to be able to write them back too, especially once they start
constructing patches in C (e.g. an SQL-callable `tpcpatch_filter` that
keeps only points satisfying a predicate per instant). Exposing only
deserialize forces vendoring of the serialize helpers anyway.

## Backwards compatibility

- The PG extension's behavior is unchanged — same call sites, same
  on-disk layout, same SQL surface.
- `pgsql/pc_pgsql.h` continues to declare the symbols (now via the
  `pc_api.h` forwarder), so out-of-tree PG extensions that already
  include it keep building.
- Library SOVERSION bump is **not** required: this is purely additive
  to `libpc`'s public surface (the `lib/` directory has no prior export
  of these symbols) and a no-op for `pointcloud-1.2.so` callers.

## Use case (the immediate motivator)

MobilityDB's pgPointCloud integration (commit-stack
`phase-8a..phase-8c` on the
[Norwegian fork](https://github.com/MobilityDB/MobilityDB)) lifts
`pcpoint` and `pcpatch` to temporal types `tpcpoint` / `tpcpatch`. The
patch-level operations (`atTpcbox`, `eIntersects`, etc.) currently
restrict instants by their 2D `PCBOUNDS` only — they can't filter on
per-point coordinates because MEOS, which is the cross-platform C
library underneath MobilityDB, links `libpc.a` and not the PG extension.

With the four functions promoted to `libpc`, MEOS can:

1. Deserialize the patch in C (no SQL roundtrip, no `PC_Explode` per
   instant, no temp arrays).
2. Walk it via the existing `pc_pointlist_from_patch` / `_get_point` /
   `pc_point_get_x` API.
3. Build a filtered `PCPATCH` and serialize it back via
   `pc_patch_serialize` for storage in the new instant.

The current workaround is a SQL wrapper around `PC_Explode`
([here](https://github.com/MobilityDB/MobilityDB/blob/phase-8c-pointcloud-tpcpatch-perpoint/mobilitydb/sql/pointcloud/430_tpcpatch.in.sql)),
which is correct but ~10× slower on dense patches because every call
re-decompresses through the SPI machinery.

## Out of scope

- Refactoring the in-memory `PCPATCH` / `PCPOINT` structs.
- Changing the serialized byte layout.
- Anything user-visible at the SQL layer.

## Acknowledgements

Driven by MobilityDB pgPointCloud integration work
(`@gaspard@norse.be`); code refs and benchmarks against `PC_Explode`
available on request.
