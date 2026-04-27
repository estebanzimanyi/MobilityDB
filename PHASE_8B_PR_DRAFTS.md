# Phase 8 PR drafts (8B + 8C + 8D + 8E)

Pre-baked titles and bodies for the 12-branch stack. Each is sized for a
single review session. Use `gh pr create --base <parent> --title "..."
--body-file -` or paste into the GitHub web UI.

The intended target for upstream is `MobilityDB/MobilityDB`. The `--base` for
PRs #0 / #0.5 / A is `master` (after PR #799 lands); thereafter each PR's base
is the previous PR's branch on the upstream side.

**Stack at a glance** — smallest reviews first, then the WKB foundation, then
the operator surface, then per-point access, then ergonomics + typmod. Sizes
are approximate diff counts excluding test fixtures.

| #   | Title                                   | LOC  | Reviewer time | Depends on |
|-----|-----------------------------------------|------|---------------|------------|
| 0   | PCBOUNDS field order fix                |  ~10 | 5 min         | #799       |
| 0.5 | Debug assert helper (PCPOINT/PCPATCH)   |   ~5 | 2 min         | #799       |
| A   | WKB I/O + schema embedding              | ~600 | 45 min        | 0.5        |
| B   | Aggregates                              | ~250 | 20 min        | A          |
| C   | Bbox ops + GiST/SP-GiST                 | ~900 | 60 min        | B          |
| D   | NAD `\|=\|` + KNN                       | ~150 | 15 min        | C          |
| E   | Spatial relationships (tpcpoint)        | ~200 | 20 min        | D          |
| F   | MF-JSON                                 | ~150 | 15 min        | E          |
| G   | Literal-value tests                     | test | 15 min        | F          |
| H   | Per-point access (numPoints + points)   |  ~100 | 15 min       | G          |
| I   | Ergonomic pcpoint/pcpatch constructors  |   ~30 | 5 min        | H          |
| J   | pcid typmod (column-level pinning)      |  ~200 | 20 min       | I          |

---

## PR 0 — `phase-8b-fix-pcbounds` → `master`

**Title** (≤70 chars):
```
fix(pointcloud): correct PCBOUNDS field order — bounds[1] is xmax not ymin
```

**Body**:
```markdown
## Summary

pgPointCloud's `PCBOUNDS` struct (`pointcloud-pg/lib/pc_api.h`) is laid out as
`{double xmin, xmax, ymin, ymax}` — `xmax` comes second, not third. Two
helpers in this repo were reading `bounds[1..2]` as if they were `ymin` and
`xmax`, which silently produced inverted-axis tpcpatch bboxes (`xmax < xmin`)
on roughly 20% of the datagen sample.

The bug was latent because:
- `tpcbox_eq` compares fields by index, so `b ~= b` held trivially.
- `atTpcbox` / `minusTpcbox` used the swapped indices on both sides, so the
  test passed for the small fixed-seed datagen sample.

Detected when wiring up `temp && temp` (self-overlap) on tpcpatch and finding
22/100 rows returning false.

Updates the `Pcpatch.bounds[]` doc comment to match the upstream PCBOUNDS
layout so future reviewers can't fall into the same trap.

## Test plan

- [x] All existing `pointcloud/*` ctest pass.
- [x] `tpcpatch &&` self-overlap reflexivity holds for every row in `tbl_tpcpatch`.

## Reviewer notes

- Three-line, surgical change. Good candidate to land independently of the
  rest of the Phase 8B stack.
- Affects every existing tpcpatch user since the bbox xmax/ymin values were
  wrong — the fix changes externally visible behaviour, but in the direction
  of correctness.
```

---

## PR 0.5 — `phase-8-fix-temporal-basetype-debug` → `master`

**Title** (≤70 chars):
```
fix(pointcloud): include T_PCPOINT/T_PCPATCH in temporal_basetype debug helper
```

**Body**:
```markdown
## Summary

`temporal_basetype()` in `meos/src/temporal/meos_catalog.c:1152` is the
debug-build assertion helper used at every entry to the WKB / TInstant
codecs (`assert(temporal_basetype(basetype))`). It enumerates every
known base type but was missing the two pgPointCloud entries — so a
Debug build of MobilityDB segfaults on the first `asBinary(tpcpoint)`
or `COPY tbl_tpcpoint TO ... (FORMAT BINARY)` call.

Release builds (NDEBUG) compile the assertion out, which is why the
issue went undetected — every CI run uses release optimisation.

Three lines under `#if POINTCLOUD` to add `T_PCPOINT` and `T_PCPATCH`.

## Test plan

- [x] Debug build: `SELECT length(asBinary(tpcpoint(...)));` returns
      35 bytes instead of crashing the backend.
- [x] `420_tpcpoint_tbl` and `430_tpcpatch_tbl` (which exercise
      `COPY BINARY` round-trip) match expected outputs.
- [x] Release build behavior unchanged.

## Reviewer notes

- The function is already gated behind `#ifndef NDEBUG` and lists every
  other temporal base type (T_BOOL, T_INT4, T_GEOMETRY, T_NPOINT, …).
  The missing PCPOINT/PCPATCH entries are a follow-up correction to
  PR #799 / phase-8a where these types were first registered.
- Two-line semantic change. Independent of every other Phase 8B PR;
  can land before or after PR 0 without conflict.
```

---

## PR A — `phase-8b-wkb-base` → `phase-8-fix-temporal-basetype-debug`

**Title**:
```
feat(pointcloud): WKB I/O for tpcpoint and tpcpatch with schema embedding
```

**Body**:
```markdown
## Summary

Adds binary I/O (`asBinary`, `tpcpointFromBinary`, `COPY ... FORMAT BINARY`)
for tpcpoint and tpcpatch.

The encoding is option-(b) self-contained:
- Per-Temporal: `int32 pcid + int32 xml_len + utf-8 xml_bytes` after the
  optional SRID slot, gated by a new `MEOS_WKB_PCSCHEMAFLAG` (0x80) bit.
- Per-instant: `int32 body_length + body_bytes` (the varlena payload trimmed
  of pgPointCloud tail padding).

A receiver that doesn't already have the `pcid` registered automatically parses
and registers the embedded schema XML, so `pg_dump --format=binary` works
across clusters with no out-of-band schema preload step.

## What changed
- `meos/src/temporal/{type_in.c,type_out.c}` — base value codec for
  `T_PCPOINT` / `T_PCPATCH`, schema-prefix header for `T_TPCPOINT` /
  `T_TPCPATCH`.
- `meos/src/temporal/meos_catalog.c` — `T_TPCPOINT` / `T_TPCPATCH` added to
  the `temporal_type()` predicate (was missing).
- `meos/{include,src}/pointcloud/meos_schema_hook.{h,c}` — schema cache
  extended to optionally hold XML; new `meos_pc_schema_register_xml`,
  `meos_pc_schema_xml`, `meos_pc_parse_xml_fn` hook.
- `mobilitydb/src/pointcloud/schema_cache.c` + `mobilitydb/src/geo/tgeo.c` —
  PG-side hooks register both XML and parsed schema at `mobilitydb_init`.
- Doc subsection `Binary representation and portability` in
  `temporal_pointcloud.xml`.

## Test plan
- [x] `420_tpcpoint_tbl`, `430_tpcpatch_tbl`: `COPY BINARY` round-trip + 
      `asBinary(temp)::tpcpoint` round-trip both yield `count = 0` differing.
- [x] All ctest green.

## Reviewer notes

- The XML-parse hook is necessary because the MEOS layer must not depend on
  `libpc.a` directly (standalone-MEOS builds don't link it). The PG layer
  installs the parser at init time.
- Cross-cluster portability has no cluster-internal optimisation yet — every
  WKB blob carries the schema. A future optimisation could skip the schema
  bit when the receiver is the same backend, behind a flag.
```

---

## PR B — `phase-8b-aggregates` → `phase-8b-wkb-base`

**Title**:
```
feat(pointcloud): aggregates extent / tcount / wcount / merge / append
```

**Body**:
```markdown
## Summary

Adds the standard MobilityDB aggregate surface to the pgPointCloud temporal
types. Mirrors the cbuffer / tnpoint aggregate set:

- `extent(tpcpoint | tpcpatch | tpcbox) → tpcbox` — bbox aggregate. Rejects
  mixed pcids (would yield uninterpretable dimensions).
- `tcount(tpcpoint | tpcpatch) → tint` and
  `wcount(tpcpoint | tpcpatch, interval) → tint`.
- `merge(tpcpoint) → tpcpoint`, `merge(tpcpatch) → tpcpatch`.
- `appendInstant`, `appendSequence` for streaming construction.

`extent` is a fresh transition function (`tpcbox_extent_transfn` in MEOS,
`Tpc_extent_transfn` PG); the others are pure SQL plumbing on top of the
generic `Temporal_*` aggfuncs machinery.

## Test plan
- [x] `440_tpc_aggfuncs_tbl`: pcid propagates from rows to aggregate result;
      every row's start/end timestamp is contained in the extent's time span.

## Reviewer notes

- `tcount` / `merge` require uniform subtype across input rows (table-level
  test reflects this by aggregating from `tbl_tpcpoint_inst` etc., not the
  merged `tbl_tpcpoint`).
- `extent(tpcbox)` reuses its own transition function as the parallel
  combine; same shape as the stbox extent precedent.
```

---

## PR C — `phase-8b-bbox-ops` → `phase-8b-aggregates`

**Title**:
```
feat(pointcloud): bbox topological + position operators with GiST and SP-GiST
```

**Body**:
```markdown
## Summary

Wires up the full MobilityDB bbox operator surface on tpcpoint, tpcpatch, and
tpcbox, and registers the matching GiST (R-tree) and SP-GiST (quadtree +
kd-tree) operator classes.

**Operators** between (tpcpoint | tpcpatch) and (tpcbox | tstzspan | self):
- Topological: `&&`, `@>`, `<@`, `~=`, `-|-`
- Directional X/Y/Z/time: `<<`, `&<`, `>>`, `&>`, `<<|`, `&<|`, `|>>`,
  `|&>`, `<</`, `&</`, `/>>`, `/&>`, `<<#`, `&<#`, `#>>`, `#&>`

**Opclasses**:
- GiST: `tpcbox_rtree_ops` (existing in phase-8a, extended), new
  `tpcpoint_rtree_ops` and `tpcpatch_rtree_ops` with TPCBox storage.
- SP-GiST: new `tpcbox_quadtree_ops`, `tpcbox_kdtree_ops`,
  `tpcpoint_quadtree_ops`, `tpcpoint_kdtree_ops`,
  `tpcpatch_quadtree_ops`, `tpcpatch_kdtree_ops` with **STBox storage**
  (lossy on pcid; recovered by recheck on the actual operator).

**Three small bridges** to keep cross-module dependencies minimal:
- `tpcbox_set_stbox` (MEOS) — lossy TPCBox→STBox conversion.
- `Tpc_gist_compress` / `Tpc_spgist_compress` / `Tpcbox_spgist_compress` (PG)
  — index compress methods.
- `tspatial_spgist_get_stbox` (in `tspatial_spgist.c`) gains 2 branches for
  T_TPCBOX / T_TPCPOINT / T_TPCPATCH so the existing
  `Stbox_spgist_leaf_consistent` dispatches on tpointcloud RHS query types.

## Test plan
- [x] `435_tpc_topops_tbl`: reflexivity of topological ops; all-encompassing
      box contains every row.
- [x] `436_tpc_posops_tbl`: irreflexivity of strict directional ops on every
      row.
- [x] `437_tpc_gist_tbl`, `438_tpc_spgist_tbl`: index-scan and seq-scan
      counts agree on bbox-overlap queries.

## Reviewer notes

- Why STBox storage for SP-GiST instead of TPCBox? Reuses the existing
  `stbox_spgist_*` 1500-line implementation. Lossy on pcid is fine because
  the operator's recheck filters the rare false positive on the leaf entry.
  Same trade-off as cbuffer.
- The single shared `Tpc_gist_compress` works for both tpcpoint and tpcpatch
  because `temporal_set_bbox` dispatches per temptype to the right
  per-instant tpcbox setter.
```

---

## PR D — `phase-8b-distance-knn` → `phase-8b-bbox-ops`

**Title**:
```
feat(pointcloud): nearest-approach distance |=| with GiST KNN ordering
```

**Body**:
```markdown
## Summary

Adds the `|=|` (nearest-approach distance) operator between any pair of
(tpcbox, tpcpoint, tpcpatch), and wires it into the existing GiST opclasses
as KNN strategy 25 with a distance support function. Mirrors cbuffer / npoint.

`SELECT … FROM tbl ORDER BY temp |=| query LIMIT N` is index-accelerated.

## What changed

- MEOS-side: `nad_tpcbox_tpcbox`, `nad_tpointcloud_tpcbox`,
  `nad_tpointcloud_tpointcloud` in `tpc_boxops.c` — thin wrappers over the
  existing `nad_stbox_stbox` (using our `tpcbox_set_stbox` lossy conversion).
- PG: `NAD_*` wrappers + `Tpcbox_gist_distance` support function that
  handles tpcbox / tpcpoint / tpcpatch query types.
- SQL: `nearestApproachDistance(...)` + `|=|` operator family on all 7 type
  pairings; `ALTER OPERATOR FAMILY` adds strategy 25 + function 8 to
  tpcbox_rtree_ops, tpcpoint_rtree_ops, tpcpatch_rtree_ops.

## Test plan

- [x] `439_tpc_distance_tbl`: self-distance is zero; GiST KNN top-5 ordering
      matches sequential scan.

## Reviewer notes

- pcid mismatch returns `DBL_MAX` (infinity), not 0 — matches the
  contract that pcid mismatches mean "incomparable schemas".
- Distance is computed on the bounding boxes (the stbox-derived euclidean
  distance), with the operator recheck on leaf entries.
```

---

## PR E — `phase-8b-spatial-rels` → `phase-8b-distance-knn`

**Title**:
```
feat(pointcloud): spatial relationships eIntersects / eDisjoint / eDwithin for tpcpoint
```

**Body**:
```markdown
## Summary

Adds the standard MobilityDB ever / always spatial relationship functions on
tpcpoint, against geometry and another tpcpoint:

- `eIntersects(...)` / `aIntersects(...)`
- `eDisjoint(...)` / `aDisjoint(...)`
- `eDwithin(..., dist float)` / `aDwithin(..., dist float)`

Implemented by projecting the tpcpoint to a tgeompoint via the schema cache
(reusing the static helpers behind `Tpcpoint_to_tgeompoint`) and delegating
to the existing `ea_intersects_tgeo_*` / `ea_disjoint_tgeo_*` /
`ea_dwithin_tgeo_*` MEOS primitives. **tpcpatch is intentionally out of
scope** — patch-level intersection would require per-point decompression
and a different design (deferred to a future PR).

## Test plan

- [x] `442_tpcpoint_spatialrels_tbl`: self-relations (every row eIntersects
      itself, none eDisjoint itself); eDwithin↔eIntersects-at-zero-distance
      equivalence; all-encompassing radius round-trip.

## Reviewer notes

- The new `tpcpoint_project_tgeompoint` static helper is extracted from the
  existing `Tpcpoint_to_tgeompoint` cast — same projection, exposed for
  reuse by the spatialrels wrappers.
```

---

## PR F — `phase-8b-mfjson` → `phase-8b-spatial-rels`

**Title**:
```
feat(pointcloud): MF-JSON output for tpcpoint and tpcpatch
```

**Body**:
```markdown
## Summary

Adds `asMFJSON()` for tpcpoint and tpcpatch, closing the JSON I/O gap
relative to tcbuffer / tnpoint / tgeompoint. Today the call errors out at
`temptype_as_mfjson_sb`'s "Unknown temporal type" branch.

- For tpcpoint: emits `{"type":"MovingPCPoint", … "coordinates":[[X,Y,Z], …]}`.
  X/Y/Z are extracted from each instant's pcpoint via the schema cache; if
  the schema lacks X/Y the coordinate array is empty (still well-formed JSON).
- For tpcpatch: emits `{"type":"MovingPCPatch", …
  "values":[{"pcid":N,"npoints":N,"bounds":[xmin,xmax,ymin,ymax]}, …]}`.
  The compressed point payload is intentionally not in the JSON — use
  `asBinary` for round-trip.
- Bbox embedding (options=1): emits a TPCBox JSON with an extra `pcid`
  field next to the standard stbox-shaped bbox array.

## Test plan
- [x] `421_tpcpoint_mfjson_tbl`: type tag check; bbox embedding includes
      pcid; coordinate-array regex shape on per-instant tpcpoint output.

## Reviewer notes

- Extends the existing `tinstant_as_mfjson_sb` / `tsequence_as_mfjson_sb` /
  `tsequenceset_as_mfjson_sb` dispatch — three small `else if` branches.
- Schema-aware coordinate extraction adds an O(1) cache lookup per instant.
```

---

## PR G — `phase-8b-tests` → `phase-8b-mfjson`

**Title**:
```
test(pointcloud): literal-value tests for tpcpoint / tpcpatch operators
```

**Body**:
```markdown
## Summary

Adds the standard MobilityDB literal-value test suite to match the cbuffer /
tnpoint / tgeompoint coverage pattern. Each existing `*_tbl` table-level test
gains a companion non-`_tbl` literal test that exercises the operator surface
on hand-built sample values rather than reducing the datagen tables to
scalars.

Files added:
- `420_tpcpoint.test.sql` — constructors, accessors, casts, comparisons,
  restrictions
- `421_tpcpoint_mfjson.test.sql` — asMFJSON output on instant / sequence /
  patch values
- `430_tpcpatch.test.sql` — same surface for tpcpatch
- `435_tpc_topops.test.sql` — &&, @>, <@, ~=, -|- between tpcpoint /
  tpcpatch and tpcbox / tstzspan / self
- `436_tpc_posops.test.sql` — directional position ops on X / Y / Z / time
  axes, including irreflexivity checks
- `439_tpc_distance.test.sql` — |=| nearest-approach distance, self-distance
  zero, 3-4-5 right triangle, infinity on time disjoint and pcid mismatch
- `442_tpcpoint_spatialrels.test.sql` — eIntersects / aIntersects /
  eDisjoint / aDisjoint / eDwithin / aDwithin against geometry and self

Sample values are built inline via PC_MakePoint / PC_Patch constructor calls
and the tpcpoint / tpcpatch / tpcpointSeq / tpcpatchSeq builders.

## Test plan
- [x] All 7 files run and produce stable expected outputs.
- [x] Full ctest green: 168 tests pass.

## Reviewer notes

- pgPointCloud's `pcpoint_in` only accepts hex-WKB ("text form not yet
  implemented" upstream), so cbuffer-style human-readable string literals
  aren't available. Constructor-function expressions are the readable
  equivalent — same operator coverage, more verbose construction.
- Tests are pure additions; no production code changes.
```

---

## PR H — `phase-8c-pointcloud-tpcpatch-perpoint` → `phase-8b-tests`

**Title**:
```
feat(pointcloud): per-point access on tpcpatch — numPoints and points SRF
```

**Body**:
```markdown
## Summary

Adds two per-point accessors on tpcpatch, both without per-instant
patch decompression in C:

- `numPoints(tpcpatch) → bigint` — total points across every instant.
  Reads each instant's patch header (`pcpatch_npoints` is in the
  varlena header — no decompression needed); sums to int64 because a
  long sequence of dense LiDAR patches can exceed int32.
- `points(tpcpatch) → setof (timestamptz, pcpoint)` — set-returning
  function: one row per (instant timestamp, point) by walking each
  instant and decomposing the patch via pgPointCloud's
  `PC_Explode(pcpatch) → setof pcpoint`. Pure SQL wrapper, no new C
  code on the explosion path.

## What changed

- `mobilitydb/src/pointcloud/tpcpatch.c` — `Tpcpatch_npoints` accessor
  (~50 lines). Walks all subtypes (TInstant / TSequence / TSequenceSet)
  and sums per-instant `pcpatch_npoints`.
- `mobilitydb/sql/pointcloud/430_tpcpatch.in.sql` — `numPoints(tpcpatch)`
  C binding + `points(tpcpatch)` SQL SRF (`generate_series` ×
  `LATERAL PC_Explode(...)`).
- `doc/temporal_pointcloud.xml` + `doc/reference.xml` — listitems for
  both accessors plus a deferral note for per-point restrictions
  (see "Limitations" below).
- `mobilitydb/test/pointcloud/queries/430_tpcpatch{,_tbl}.test.sql` —
  literal + table-driven assertions: per-instant equivalence with
  `startNumPoints` for TInstant, `numPoints == COUNT(*) FROM points()`,
  `numInstants == COUNT(DISTINCT t) FROM points()`.

## Test plan

- [x] Literal: numPoints = 2/4/8 on TInstant/TSequence/TSequenceSet
      values; points emits 2/4 rows correctly grouped per timestamp.
- [x] Table-driven over `tbl_tpcpatch{,_inst,_seq,_discseq,_seqset}`:
      all four phase-8c assertions return `t`.

## Limitations (deferred)

Per-point restriction operators on tpcpatch — e.g. an `atTpcbox` that
filters individual points inside each patch, or per-point
`eIntersects(tpcpatch, geometry)` — are intentionally out of scope.
They require C-side patch decomposition, which depends on a small
upstream pgPointCloud surface change to expose
`pc_patch_deserialize` / `pc_point_serialize` from `lib/` instead of
`pgsql/` (today they're compiled only into `pointcloud-1.2.so` with
local visibility, unlinkable from `libpc.a`-only consumers).

The upstream ask is drafted in `PGPOINTCLOUD_UPSTREAM_PR_DRAFT.md`
in this repository and has been communicated to the pgPointCloud
maintainers. Once that lands, a follow-up MobilityDB PR can replace
the SQL `points(...)` wrapper with a native C-side decomposition path
and add the per-point restriction operators.

## Reviewer notes

- `points(tpcpatch)` is roughly the cost of N `PC_Explode` calls on
  dense patches (one per instant). Correct but slow on multi-thousand
  point patches; a fast path is gated on the upstream change above.
- The accessor and SRF are non-blocking on each other, but kept in one
  PR because the test assertions cross-check them
  (`numPoints == COUNT(*) FROM points()`).
- No bbox-op, index, or aggregate surface change.
```

---

## PR I — `phase-8d-pointcloud-ergonomics` → `phase-8c-pointcloud-tpcpatch-perpoint`

**Title**:
```
feat(pointcloud): ergonomic constructors pcpoint(int, x, y, z) / pcpatch(int, …)
```

**Body**:
```markdown
## Summary

Three thin SQL constructors on top of pgPointCloud's `PC_MakePoint`
and `PC_Patch` so test literals, `INSERT … VALUES`, and ad-hoc
queries don't have to spell out `ARRAY[…]::float[]` every time:

- `pcpoint(pcid integer, x float, y float) → pcpoint`
- `pcpoint(pcid integer, x float, y float, z float) → pcpoint`
- `pcpatch(pcid integer, VARIADIC points pcpoint[]) → pcpatch`

```sql
-- before
PC_Patch(ARRAY[
  PC_MakePoint(1, ARRAY[1.0, 1.0, 1.0]::float[]),
  PC_MakePoint(1, ARRAY[2.0, 2.0, 2.0]::float[])])
-- after
pcpatch(1, pcpoint(1, 1, 1, 1), pcpoint(1, 2, 2, 2))
```

Pure SQL — no new C code. Schema-dimension validation still happens
inside `PC_MakePoint`/`PC_Patch`; these wrappers inherit it.

## What changed

- `mobilitydb/sql/pointcloud/420_tpcpoint.in.sql` — two `pcpoint(…)`
  overloads.
- `mobilitydb/sql/pointcloud/430_tpcpatch.in.sql` — `pcpatch(…)`
  variadic wrapper.
- `mobilitydb/test/pointcloud/queries/420_tpcpoint.test.sql` and
  `430_tpcpatch.test.sql` — equivalence assertions vs. the verbose
  `PC_MakePoint`/`PC_Patch` form.
- `doc/temporal_pointcloud.xml` and `doc/reference.xml` — listitems
  for the new constructors next to the existing pcpoint/pcpatch
  accessors.

## Test plan

- [x] `pcpoint(1, 1, 1, 1)::text = PC_MakePoint(1, ARRAY[…])::text`.
- [x] `pcpatch(1, …)` produces the same value as the `PC_Patch(ARRAY[…])` form.
- [x] `numPoints(tpcpatch(pcpatch(1, …), …)) = 2`.

## Reviewer notes

- Function-name overloading on a type-named function is unambiguous in
  PG when arity > 1 (cast syntax `expr::pcpoint` is single-arg only).
- The `pcid` parameter on `pcpatch(...)` is reader-facing; the body
  forwards the variadic array to `PC_Patch`, which validates same-pcid.
- No production-code change in MEOS or the C wrapper layer.
```

---

## PR J — `phase-8e-pointcloud-typmod` → `phase-8d-pointcloud-ergonomics`

**Title**:
```
feat(pointcloud): pcid typmod for tpcpoint/tpcpatch (column-level pinning)
```

**Body**:
```markdown
## Summary

Adds the standard PG typmod surface to tpcpoint and tpcpatch so a
column or domain can pin a single pcid:

```sql
CREATE TABLE scans (id int, traj tpcpoint(1), full_scan tpcpatch(1));
INSERT INTO scans VALUES (1,
  tpcpoint(pcpoint(1, 1.0, 2.0, 3.0), '2024-01-01'),
  tpcpatch(pcpatch(1, pcpoint(1,1,1,1), pcpoint(1,2,2,2)), '2024-01-01'));
-- ERROR if any pcid disagrees with the column typmod
```

Mirrors PostGIS' `geometry(Point, 4326)` and MobilityDB's existing
`tgeompoint(Point, SRID)` patterns. Catches at INSERT/cast time a
class of mistake that today surfaces only inside aggregates (or
silently never).

## What changed

- `mobilitydb/src/pointcloud/tpc_typmod.c` — new file with
  `Tpc_typmod_in` (parses single-int pcid; rejects negative / zero /
  non-numeric / multi-arg), `Tpc_typmod_out` (renders `(pcid)` or
  empty when -1), `Tpc_enforce_typmod` (cast hook that raises on
  pcid mismatch).
- `mobilitydb/src/pointcloud/CMakeLists.txt` — wire the new file.
- `mobilitydb/sql/pointcloud/420_tpcpoint.in.sql` and
  `430_tpcpatch.in.sql` — register `tpc_typmod_in`/`_out` C functions,
  add them to `CREATE TYPE`, register the
  `tpcpoint(tpcpoint, integer)` / `tpcpatch(tpcpatch, integer)` cast
  via `Tpc_enforce_typmod` and a single `CREATE CAST … AS IMPLICIT`.
- `mobilitydb/test/pointcloud/queries/415_tpc_typmod.test.sql` (new) —
  full coverage: typmod_in error paths, `format_type` round-trip on
  pinned columns, INSERT match/mismatch on tpcpoint and tpcpatch,
  unconstrained column accepts mixed pcids, `ALTER TABLE … TYPE
  tpcpoint(2)` re-validates existing rows.
- `doc/temporal_pointcloud.xml` — typmod paragraph at the top of the
  tpcpoint section with a CREATE TABLE example showing both columns.

## Test plan

- [x] All four typmod_in error cases raise at parse time
      (`tpcpoint(-1)`, `tpcpoint(0)`, `tpcpoint(abc)`, `tpcpoint(1, 2)`).
- [x] Matching pcid on INSERT succeeds; mismatch raises with
      `Pcid of tpcpoint value (N) does not match column typmod pcid (M)`.
- [x] `format_type(atttypid, atttypmod)` prints `tpcpoint(1)` /
      `tpcpatch(1)` / `tpcpoint` (unconstrained) correctly.
- [x] `ALTER TABLE … ALTER COLUMN … TYPE tpcpoint(2)` re-validates
      existing rows and raises if any disagree.
- [x] No regressions in 420/430 tpcpoint/tpcpatch literal or table tests.

## Reviewer notes

- The typmod packs only a positive int32 pcid; -1 is unconstrained.
  No bit-packing tricks (only one field), so the layout is trivially
  comparable against future extensions.
- `Tpc_typmod_in` is hand-rolled `strtol` rather than `pg_strtoint32`
  because `<utils/builtins.h>` collides with `<json-c/json.h>`
  pulled in transitively by `meos_internal.h` (existing constraint
  in this repo — see `mobilitydb/src/pointcloud/tpcpoint.c`'s lead
  comment).
- The `pcid` mismatch error mirrors the SRID-mismatch wording from
  `Tspatial_enforce_typmod`.
- `extent` aggregate's runtime pcid check is unchanged — it remains
  the safety net for unconstrained columns.

## Out of scope

- Mixed-pcid columns inside SP-GiST/GiST opclasses with TPCBox
  storage — the existing recheck-on-leaf design is unchanged.
- Multi-column typmod for SRID alongside pcid — pgPointCloud schemas
  pin SRID via the schema definition, so a separate typmod would be
  redundant today.
```

---

## Suggested PR-opening sequence

1. **PR #0 + PR #0.5** (independent fixes) — both can open in parallel
   the moment **PR #799** lands. Tiny, surgical; aim to merge both
   within the same review cycle so the rest of the stack rebases on a
   correct foundation.
2. **PR A (WKB)** — open with `--base phase-8-fix-temporal-basetype-debug`
   once #0.5 merges (the WKB tbl tests rely on the assertion fix to
   pass under Debug-build CI). If CI is release-only, A can also rebase
   onto `master` directly with no behavioural change.
3. **PRs B → C → D → E → F → G** — open each with `--base` set to the
   previous PR's branch on upstream. Each rebases automatically as its
   parent merges.
4. **PR H (per-point access)** — open with `--base phase-8b-tests`.
   Pure additive, no risk of regressing earlier PRs.
5. **PR I (ergonomic constructors)** — open with `--base
   phase-8c-pointcloud-tpcpatch-perpoint`. Tiny SQL-only diff; should
   review in 5 minutes.
6. **PR J (pcid typmod)** — open with `--base
   phase-8d-pointcloud-ergonomics`. Self-contained; the typmod test
   exercises both tpcpoint and tpcpatch so it lands the entire
   column-level pinning surface in one review.

If reviewers prefer one large PR over a stack, the integrated branch
`phase-8e-pointcloud-typmod` at HEAD is the squashed single-PR
equivalent (covers everything from PR 0 through J).

## External / non-MobilityDB

- `PGPOINTCLOUD_UPSTREAM_PR_DRAFT.md` — separate ask to
  `pgpointcloud/pointcloud`, NOT a MobilityDB PR. Tracks the
  unblocking work for native per-point restriction operators on
  tpcpatch (see PR H "Limitations"). When it lands, a follow-up
  PR K can replace the SQL `points(...)` wrapper with a native
  C-side decomposition path and add the per-point `atTpcbox` /
  `eIntersects` variants on tpcpatch.
