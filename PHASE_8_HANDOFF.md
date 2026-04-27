# Phase 8 handoff — pgPointCloud integration

This doc is the in-tree counterpart of the auto-memory entry
`project_pointcloud_post_upstream.md`. It captures the cumulative state
of the phase-8 work so the next session can resume after pgPointCloud
upstream answers the static-keyword removal request, without losing
context.

---

## tl;dr

- **14 logical PRs of work** are stacked on the single branch
  `phase-8i-pointcloud-quickstart`. Branch-per-PR splits are documented
  in `PHASE_8B_PR_DRAFTS.md` (titles, bodies, reviewer notes) and have
  not yet been carved out as separate commits.
- **Blocking dependency**: pgPointCloud's
  `pc_(point|patch)_(de)serialize` helpers live in `pgsql/pc_pgsql.c`
  with local visibility (lowercase `t` in `nm pointcloud-1.2.so`).
  MEOS, which links `libpc.a` and not the PG extension, cannot reach
  them. The upstream ask drafted at
  `PGPOINTCLOUD_UPSTREAM_PR_DRAFT.md` requests a move into `lib/` with
  declarations in `pc_api.h`.
- Once unblocked, the next PR is **~600 LOC of C-side patch
  decomposition** that powers per-point `atTpcbox` / `eIntersects` and
  replaces the slow SQL `points(tpcpatch)` wrapper with a native
  SRF. Branch name: `phase-9-pointcloud-perpoint-c`.

---

## What's shipped (phase-8c..i)

All on `phase-8i-pointcloud-quickstart` as a single working-tree
diff against `phase-8b-pointcloud-wkb`. The full per-PR titles, bodies,
size estimates, and dependency graph live in
`PHASE_8B_PR_DRAFTS.md` (12 PRs there) — phases 8h and 8i extend that
list to 14.

### Production code

| File | Phase | What |
|---|---|---|
| `meos/src/temporal/meos_catalog.c` | 8c | Add T_PCPOINT / T_PCPATCH to `temporal_basetype()` debug-assert helper. Release builds compile the assert out; Debug builds segfaulted on every `asBinary(tpcpoint)` without this. |
| `meos/CMakeLists.txt` | 8h | Add `if(POINTCLOUD)` block wiring the `pointcloud` OBJECT lib into `libmeos.so`. Was missing — `libmeos.so` previously did not export `pcpoint_hex_in` / etc. even when `MEOS=ON POINTCLOUD=ON`. |
| `meos/src/pointcloud/{pcpoint,pcpatch}.c` | 8h | Replace PG `hex_encode`/`hex_decode` with PostGIS `parse_hex`/`deparse_hex` (from `liblwgeom`). Output now uppercase, matches MEOS's existing `HEXCHR` convention. Allows `MEOS=ON` builds (PG headers unavailable in standalone). |
| `mobilitydb/src/pointcloud/tpcpatch.c` | 8c | `Tpcpatch_npoints` accessor — sums per-instant `pcpatch_npoints` without decompression. |
| `mobilitydb/src/pointcloud/tpc_typmod.c` (NEW) | 8e | `Tpc_typmod_in` / `_out` / `_enforce_typmod` — column-level pcid pinning. |
| `mobilitydb/src/pointcloud/CMakeLists.txt` | 8e | Wire `tpc_typmod.c`. |
| `mobilitydb/sql/pointcloud/420_tpcpoint.in.sql` | 8d, 8e | Ergonomic `pcpoint(int, x, y, z)` constructors + typmod plumbing on `CREATE TYPE` + cast. |
| `mobilitydb/sql/pointcloud/430_tpcpatch.in.sql` | 8c, 8d, 8e | `numPoints` + `points` SRF + ergonomic `pcpatch(int, VARIADIC)` + typmod cast. |

### Tests + expected outputs

| File | Phase | What |
|---|---|---|
| `mobilitydb/test/pointcloud/queries/415_tpc_typmod.test.sql` (NEW) | 8e | Typmod_in error paths, `format_type` round-trip, INSERT match/mismatch on tpcpoint and tpcpatch, ALTER TABLE re-validation. |
| `mobilitydb/test/pointcloud/queries/420_tpcpoint.test.sql` | 8d | Ergonomic constructor equivalence assertions. |
| `mobilitydb/test/pointcloud/queries/430_tpcpatch.test.sql` | 8c, 8d | numPoints / points SRF literal tests + ergonomic pcpatch tests. |
| `mobilitydb/test/pointcloud/queries/430_tpcpatch_tbl.test.sql` | 8c | numPoints / points table-driven assertions over `tbl_tpcpatch{,_inst,_seq,_discseq,_seqset}`. |
| `mobilitydb/test/pointcloud/queries/431_tpcpatch_cmp.test.sql` (NEW) | 8g | B-tree comparator audit: reflexivity, anti-symmetry, total order, pcid-as-primary-discriminator, transitivity chain, `array_agg(k ORDER BY pa)` round-trip. |
| `mobilitydb/test/pointcloud/queries/432_tpcpatch_mfjson.test.sql` (NEW) | 8h | MF-JSON shape assertions on tpcpatch — type tag, per-instant value object (pcid/npoints/4-element bounds), bbox embedding (options=1), datetimes-vs-values length parity. |

Matching `expected/*.test.out` files (regenerated under `PGTZ='America/Los_Angeles' PGDATESTYLE='Postgres, MDY'` to match the CI fixture-loaded data).

### Standalone-MEOS

| File | Phase | What |
|---|---|---|
| `meos/examples/tpc_wkb_roundtrip.c` (NEW) | 8h | Hand-coded SERIALIZED_POINT seed → `pcpoint_hex_in` → `tinstant_make` → `temporal_as_wkb` → `temporal_from_wkb` → `temporal_eq`. Two timestamps. Pin the MEOS-only WKB code path independent of any PG roundtrip. |

### Documentation

| File | Phase | What |
|---|---|---|
| `doc/temporal_pointcloud.xml` | 8c..i | Quickstart sect1 (8i); ergonomic constructors subsection (8d); pcid typmod paragraph (8e); per-point ops roadmap subsection (8f); tpcpoint and tpcpatch B-tree ordering subsections (8g, 8h); numPoints / points listitems (8c). |
| `doc/reference.xml` | 8c..i | Index entries for every new accessor / subsection. |
| `doxygen/Doxyfile` | session-wide | EXCLUDE_PATTERNS for `tbox_master.c` and `load_test.sql` scratch files. |
| `meos/src/geo/tspatial_parser.c`, `meos/src/temporal/type_parser.c`, `meos/src/rgeo/trgeo_parser.c` | session-wide | `@param[out] result` → `@return` for parsers that no longer take an out-param. |
| `meos/src/rgeo/trgeo.c` | session-wide | `@param strict` → `@param atfunc` matching the actual signature on `trgeo_before_timestamptz` / `trgeo_after_timestamptz`. |
| `mobilitydb/src/temporal/tbox.c` | session-wide | `@ingroup mobilitydb_temporal_box_comp` → `mobilitydb_box_comp` on Tbox_hash / Tbox_hash_extended. |

### Process artefacts (root)

| File | Why |
|---|---|
| `PHASE_8B_PR_DRAFTS.md` | Per-PR titles + bodies + reviewer notes for the 12-PR phase-8b-d-e stack. Two more (phase-8h, 8i) need appending if you split into PRs. |
| `PGPOINTCLOUD_UPSTREAM_PR_DRAFT.md` | The upstream ask. References `nm` evidence and proposes the move from `pgsql/` to `lib/`. |
| `PHASE_8_HANDOFF.md` | This file. |

---

## What's blocked

**Per-point filtering / construction** on tpcpatch — the operators
that build a new patch from a subset of the original's points
(e.g. an `atTpcbox` overload that drops individual points failing the
box's xy/z extent, or `eIntersects(tpcpatch, geometry)` over actual
point coordinates rather than just the patch bounding box).

The blocker is `pc_patch_deserialize` and `pc_point_serialize` not
being exposed from `libpc.a`. See `PGPOINTCLOUD_UPSTREAM_PR_DRAFT.md`
for the technical detail and the proposed surface.

The current SQL `points(tpcpatch)` wrapper (in
`mobilitydb/sql/pointcloud/430_tpcpatch.in.sql`) provides correct but
slow per-point access via `PC_Explode`. It will become a fallback once
the C path lands.

---

## Resuming after pgPointCloud answers

### If they merged the change as-is

1. **Bump the subtree**:
   ```sh
   git subtree pull --prefix=pointcloud-pg \
     https://github.com/pgpointcloud/pointcloud master --squash
   ```
2. **Rebuild libpc.a**:
   ```sh
   cd pointcloud-pg && ./autogen.sh && ./configure && make
   ```
3. **Verify exposure**: `nm pointcloud-pg/lib/libpc.a | grep -E " T (pc_patch_deserialize|pc_point_serialize)"` — must be uppercase T.
4. **Cut `phase-9-pointcloud-perpoint-c`** from latest phase-8 tip.
5. **Implement**: see auto-memory `project_pointcloud_post_upstream.md` "What to build first" section for the 7-step plan (~600 LOC).
6. **Drop the deferral note** in the per-point roadmap subsection of `doc/temporal_pointcloud.xml#tpcpatch_perpoint_roadmap`.

### If they want a different shape

Discuss with the user before adapting. Options:
- Vendor the four functions into `meos/src/pointcloud/pgsql_compat.c` under a guard flag. License audit lives in `pointcloud-pg/COPYING`.
- Provide our own (de)serializer wrappers that don't depend on the upstream symbols. Probably means re-implementing parts of `pc_patch_compress` / decompress logic — significant scope.

### If they decline

Document the rejection rationale in this file. Options:
- Vendor (as above) — most pragmatic.
- Drop the per-point operators entirely from MobilityDB's tpcpatch surface. The current SQL `points()` wrapper remains; users can apply per-point predicates in SQL after explosion.

---

## Build / install / test recipe

```sh
# Build the PG extension (default flow — produces libMobilityDB-1.4.so)
cd /home/esteban/src/MobilityDB
mkdir -p build-pg-ext && cd build-pg-ext
cmake -DPOINTCLOUD=ON -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
sudo make install
# Reload extension after install:
psql phase8 -c "DROP EXTENSION mobilitydb CASCADE; CREATE EXTENSION mobilitydb;"

# Build standalone MEOS (for tpc_wkb_roundtrip.c)
mkdir -p build-meos-only && cd build-meos-only
cmake -DMEOS=ON -DPOINTCLOUD=ON -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
PG_INC=$(pg_config --includedir-server)
gcc -Wall -g -I../meos/include -I$PG_INC \
  -o /tmp/tpc_wkb_roundtrip ../meos/examples/tpc_wkb_roundtrip.c \
  libmeos.so ../pointcloud-pg/lib/libpc.a -lm -lxml2
LD_LIBRARY_PATH=. /tmp/tpc_wkb_roundtrip

# Run pointcloud test suite (manual — ctest target also available)
PGTZ='America/Los_Angeles' PGDATESTYLE='Postgres, MDY' \
  psql -X phase8 -f build-pg-ext/mobilitydb/test/pointcloud/data/load_pointcloud.sql

cd /home/esteban/src/MobilityDB
for t in 415_tpc_typmod 420_tpcpoint{,_tbl} 430_tpcpatch{,_tbl} 431_tpcpatch_cmp \
         432_tpcpatch_mfjson; do
  out=/tmp/$t.out
  PGTZ='America/Los_Angeles' PGDATESTYLE='Postgres, MDY' \
    psql -X -e --set ON_ERROR_STOP=0 phase8 \
    < mobilitydb/test/pointcloud/queries/$t.test.sql > $out
  diff -q mobilitydb/test/pointcloud/expected/$t.test.out $out && echo "$t OK"
done

# Doxygen
cd build-pg-ext && cmake -DDOC_DEV=ON . && make doc_dev
# Output: build-pg-ext/doxygen/docs/html/index.html
```

---

## Pitfalls remembered the hard way

- **`temporal_basetype()` debug-assert helper** must list every base type. T_PCPOINT/T_PCPATCH were missing once; Debug builds segfaulted on `asBinary(tpcpoint)` until added under `#if POINTCLOUD`.
- **`<utils/builtins.h>` collides with json-c** when `meos_internal.h` (which includes `<json-c/json.h>`) is also in scope — `struct json_object` vs `Datum json_object(PG_FUNCTION_ARGS)`. Use `parse_hex` / `deparse_hex` from `liblwgeom`, or hand-rolled inline.
- **`pcpatch_meaningful_size`** strips trailing zero-padding. Any new code that builds a `Pcpatch` must respect this — `pcpatch_cmp` does memcmp over the meaningful window, hashing uses the same. Trailing padding is normal and expected.
- **PCBOUNDS layout is `{xmin, xmax, ymin, ymax}`** (NOT `{xmin, ymin, xmax, ymax}`). Earlier bug fixed in commit `db6903061` (in `phase-8b-fix-pcbounds`). Always read by index, not by guess.
- **Schema cache lifetime**: `meos_pc_schema_register_xml` palloc's the XML in `TopMemoryContext` so the pointer outlives a query. The `meos_pc_parse_xml_fn` and `meos_pc_schema_fn` hooks are installed at `mobilitydb_init` time on the PG side and at `meos_initialize` time in standalone.
- **TZ-sensitive tests**: the `expected/*.test.out` files for tpcpoint/tpcpatch were generated under `PGTZ='America/Los_Angeles'` and `PGDATESTYLE='Postgres, MDY'` to match the ctest cluster config. Reproducing locally requires the same env.
- **build-pg-ext root-owned files**: `sudo make install` runs CMake's install pass which can leave intermediate `.o.d` files owned by root, breaking subsequent non-sudo `make`. Recover with `sudo chown -R esteban: build-pg-ext`.

---

## Open questions / next decisions

1. **Should the per-point `atTpcbox` overload reuse the existing SQL function name or pick a new one?** Reusing means an INSERT-time disambiguation by argument shape; renaming means breaking source compatibility for existing callers. Discuss before deciding.
2. **Vendor-or-wait policy** if pgPointCloud is slow to respond. The `phase-8b..i` work has been deliberately additive and non-blocking; the per-point operators are the first thing that needs the upstream change. A 4-week response window seems reasonable before considering a temporary vendoring path.
3. **Squashed single-PR alternative** vs. opening the 14-PR stack. `phase-8i-pointcloud-quickstart` HEAD is the squashed equivalent; the per-PR draft in `PHASE_8B_PR_DRAFTS.md` is reviewer-friendly but has higher overhead. Ask the user which they prefer before opening anything on origin.
