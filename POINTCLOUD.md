# pgPointCloud Integration in MobilityDB

This document is the review-and-test companion for the work that
adds full MobilityDB temporal-type support on top of pgPointCloud
(`tpcpoint`, `tpcpatch`, `tpcbox`, the bbox/index/aggregate/I/O
surface around them, and the column-level pcid typmod).

It is structured in two parts:

- **Part I — Practical** (sections [A](#a-build-install-test) and
  [B](#b-architecture-at-a-glance)): how to build and test the work
  on a fresh checkout, and just enough architectural context to
  read the rest. A new colleague should be able to skim Part I and
  start running queries.

- **Part II — PR-by-PR review** (sections [1](#1-pcbounds-field-order-fix)
  through [14](#14-pointcloud-quickstart-in-the-user-manual)): one
  description per logically-independent PR, in dependency order.
  Each section has Summary, What changed, Test plan, and Reviewer
  notes — readable as a standalone review request against the
  corresponding subset of the diff.

Followed by [Deferred work](#deferred-work-summary),
[Pitfalls](#pitfalls-worth-knowing-before-testing), and
[Where to read more](#where-to-read-more).

The 14 PRs build on one another in the order listed below. They
have all landed on a single working branch; a reviewer can either
walk Part II top-to-bottom (recommended for first read) or focus
on the section corresponding to the change set being tested.

---

## Index

### Part I — Practical

A. [Build, install, test](#a-build-install-test)
B. [Architecture at a glance](#b-architecture-at-a-glance)

### Part II — PR-by-PR review

1. [PCBOUNDS field-order fix](#1-pcbounds-field-order-fix)
2. [Debug-build basetype assertion fix](#2-debug-build-basetype-assertion-fix)
3. [WKB I/O with embedded schema](#3-wkb-io-with-embedded-schema)
4. [Aggregates: extent / tcount / wcount / merge / append](#4-aggregates-extent--tcount--wcount--merge--append)
5. [Bbox topological + position operators with GiST and SP-GiST](#5-bbox-topological--position-operators-with-gist-and-sp-gist)
6. [Nearest-approach distance `|=|` with GiST KNN ordering](#6-nearest-approach-distance--with-gist-knn-ordering)
7. [Spatial relationships eIntersects / eDisjoint / eDwithin for tpcpoint](#7-spatial-relationships-eintersects--edisjoint--edwithin-for-tpcpoint)
8. [MF-JSON output for tpcpoint and tpcpatch](#8-mf-json-output-for-tpcpoint-and-tpcpatch)
9. [Literal-value test suite](#9-literal-value-test-suite)
10. [Per-point access on tpcpatch — numPoints and points SRF](#10-per-point-access-on-tpcpatch--numpoints-and-points-srf)
11. [Ergonomic constructors pcpoint(int, x, y, z) / pcpatch(int, …)](#11-ergonomic-constructors-pcpointint-x-y-z--pcpatchint-)
12. [pcid typmod for tpcpoint / tpcpatch (column-level pinning)](#12-pcid-typmod-for-tpcpoint--tpcpatch-column-level-pinning)
13. [Standalone-MEOS plumbing for the pointcloud surface](#13-standalone-meos-plumbing-for-the-pointcloud-surface)
14. [Pointcloud quickstart in the user manual](#14-pointcloud-quickstart-in-the-user-manual)

### Closing

* [Deferred work summary](#deferred-work-summary)
* [Pitfalls worth knowing before testing](#pitfalls-worth-knowing-before-testing)
* [Where to read more](#where-to-read-more)

---

# Part I — Practical

## A. Build, install, test

### Prerequisites

- PostgreSQL ≥ 13 with development headers (`pg_config` on PATH).
- PostGIS ≥ 3 (linked transitively via `liblwgeom`).
- pgPointCloud is built from the in-tree subtree at
  `pointcloud-pg/` — no separate install needed. The `libpc.a` it
  produces is statically linked into `libmeos.so`.
- Standard MobilityDB toolchain: GCC, CMake ≥ 3.7, libxml2,
  json-c, GEOS, PROJ.

### PG extension build

```sh
cd MobilityDB
mkdir -p build-pg-ext && cd build-pg-ext
cmake -DPOINTCLOUD=ON -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
sudo make install
```

After `make install`, recreate the extension to pick up the new
SQL catalog entries:

```sh
psql <db> -c "DROP EXTENSION IF EXISTS mobilitydb CASCADE;"
psql <db> -c "CREATE EXTENSION mobilitydb CASCADE;"
```

`CASCADE` is required because the extension depends on PostGIS
and on the bundled `pointcloud` extension (also created by
`CASCADE`).

### Standalone-MEOS build

For embedding MobilityDB without PostgreSQL — `libmeos.so`
exposes the pgPointCloud surface in the same build:

```sh
mkdir -p build-meos-only && cd build-meos-only
cmake -DMEOS=ON -DPOINTCLOUD=ON -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
```

A small worked example is at `meos/examples/tpc_wkb_roundtrip.c`,
which seeds a `pcpoint` from raw bytes, lifts to a `tpcpoint`,
encodes to WKB, decodes back, and asserts equality — useful as a
smoke test for standalone consumers.

### Regression test suite

The full ctest suite (≈ 170 tests) runs against the
just-installed PG extension:

```sh
cd build-pg-ext
ctest --output-on-failure
```

Pointcloud-only run (≈ 30 s on a developer laptop):

```sh
ctest -R "pcpoint|tpcbox|tpcpoint|tpcpatch|tpc_" --output-on-failure
```

The expected-output files were generated under
`PGTZ='America/Los_Angeles'` and `PGDATESTYLE='Postgres, MDY'`.
The test driver sets these for you; if you reproduce queries by
hand and see timestamp-format diffs, set the same env first.

### Reload after rebuild

If you `make install` over an existing install, drop and recreate
the extension before re-running tests; otherwise the old SQL
catalog remains attached to the old shared object.

---

## B. Architecture at a glance

The integration is deliberately **additive on top of unmodified
pgPointCloud**. Static `pcpoint` / `pcpatch` semantics, hex-WKB
encoding, the schema catalog (`pointcloud_formats`), and existing
SQL functions like `PC_MakePoint` / `PC_Patch` / `PC_Explode` are
unchanged. MobilityDB sits on top and lifts these into temporal
types.

The upstream pgPointCloud subtree is vendored at `pointcloud-pg/`
(v1.2.5) and built into `libpc.a`. MEOS links `libpc.a` for the
byte-level helpers; the PG extension links both.

### Type stack

```
   tpcpoint / tpcpatch     (lifted temporal types)
       │
       ├── tpcbox          (spatiotemporal bbox: stbox + pcid)
       │
       └── pcpoint / pcpatch  (static, from pgPointCloud)
            │
            └── pcpointset / pcpatchset  (set types)
```

- **`pcpoint`, `pcpatch`** — pgPointCloud's static base types,
  surfaced through MEOS with byte-level helpers and three
  ergonomic SQL constructors. Each value carries a `pcid` that
  resolves through `pointcloud_formats` to an XML schema declaring
  the dimensions (X, Y, Z, Intensity, …).
- **`pcpointset`, `pcpatchset`** — same shape as MobilityDB's
  other set types. Same-pcid uniformity enforced inside `set_make`.
- **`tpcbox`** — spatiotemporal bounding box, the pcid-aware mirror
  of `stbox`. Constructors for 2D, 3D, time-only, XY+T, and XYZ+T.
- **`tpcpoint`, `tpcpatch`** — lifted temporal types built using
  MobilityDB's standard `temporal_make` machinery, so the generic
  temporal API works automatically (`numInstants`, `atTime`,
  `valueAtTimestamp`, `appendInstant`, …) plus pgPointCloud-specific
  accessors (`pcid`, per-dimension projections, `tgeompoint` cast).

The default interpolation on `tpcpoint` / `tpcpatch` is **step**,
not linear — coordinate-only linear interpolation makes sense for
moving points, but not for the non-coordinate dimensions
(Intensity, ReturnNumber, …) that pgPointCloud schemas typically
include. Step is the safe default.

### Operator and index surface

- **Bbox-level operators** between any pair of `(tpcbox, tpcpoint,
  tpcpatch)`, with `tstzspan` accepted as a substitute for the
  temporal axis: topological (`&&`, `@>`, `<@`, `~=`, `-|-`),
  directional X/Y/Z/time (16 ops), and nearest-approach distance
  (`|=|`).
- **Spatial relationships** (`eIntersects` / `eDisjoint` /
  `eDwithin`, plus `aIntersects` / `aDisjoint` / `aDwithin`) on
  `tpcpoint` — against geometry and against another `tpcpoint`.
  `tpcpatch` is intentionally not in this surface (see
  [Deferred work](#deferred-work-summary)).
- **Restrictions**: `atTpcbox` / `minusTpcbox` on both `tpcpoint`
  and `tpcpatch`. The `tpcpatch` variants operate at instant
  granularity — kept-or-dropped as a whole, never partially.
  Per-point granularity is deferred.
- **Indexing**: GiST (R-tree) and SP-GiST (quadtree, kd-tree) on
  all three types. KNN ordering is wired through the GiST
  distance support function.
- **Aggregates**: `extent`, `tcount`, `wcount`, `merge`,
  `appendInstant`, `appendSequence`. `extent` rejects mixed pcids.

### Schema enforcement

- **Same-pcid checks** are built into every binary predicate that
  doesn't already pass through bbox storage: a pcid mismatch
  yields `false` for predicates and `DBL_MAX` for distance.
- **Column-level pcid pinning** via PG typmod — `tpcpoint(N)` /
  `tpcpatch(N)` columns enforce a single schema at INSERT time,
  mirroring PostGIS's `geometry(Point, SRID)` precedent.

### I/O surfaces

- **Hex-WKB** at the MEOS layer — the hex encoding of
  pgPointCloud's on-wire `SERIALIZED_POINT` / `SERIALIZED_PATCH`
  byte image.
- **Binary WKB with self-contained schema embedding** — every WKB
  blob carries its own pgPointCloud schema XML, so `pg_dump
  --format=binary` works across clusters with no out-of-band
  schema preload. Gated by the new `MEOS_WKB_PCSCHEMAFLAG` (0x80)
  bit.
- **MF-JSON** — `MovingPCPoint` (with X/Y/Z coordinates) and
  `MovingPCPatch` (with per-instant `{pcid, npoints, bounds}`
  summaries). Bbox embedding (options=1) emits a TPCBox JSON with
  an extra `pcid` field.

### Schema cache

Every byte-level operation that needs the pgPointCloud schema
(dimension extraction, MF-JSON output, WKB roundtrip) goes through
a process-global per-pcid cache.

- **Backend**: cache is built lazily from `pointcloud_formats` via
  the SPI client; entries are stored in `TopMemoryContext` so they
  outlive query boundaries.
- **MEOS layer**: declares the cache surface
  (`meos_pc_schema(pcid)`, `meos_pc_schema_xml(pcid)`,
  `meos_pc_schema_register*`, `meos_pc_schema_clear`) but does
  not implement the lookup. The implementation is plugged in via
  a hook (`meos_pc_schema_fn`), set by the PG layer at
  `mobilitydb_init` and by standalone-MEOS callers in their own
  bootstrap.

This architecture is what allows MobilityDB to be compiled as a
`MEOS=ON POINTCLOUD=ON` standalone library without dragging the
PG SPI / catalog into MEOS.

---

# Part II — PR-by-PR review

## 1. PCBOUNDS field-order fix

**Title**: fix(pointcloud): correct PCBOUNDS field order — `bounds[1]` is `xmax`, not `ymin`

### Summary

pgPointCloud's `PCBOUNDS` struct in `pointcloud-pg/lib/pc_api.h`
is laid out as `{double xmin, xmax, ymin, ymax}` — `xmax` comes
second, not third. Two helpers in this repo were reading
`bounds[1..2]` as if they were `ymin` and `xmax`, which silently
produced inverted-axis tpcpatch bboxes (`xmax < xmin`) on
roughly 20% of the datagen sample.

The bug was latent because:

- `tpcbox_eq` compares fields by index, so `b ~= b` held
  trivially on every row.
- `atTpcbox` / `minusTpcbox` used the swapped indices on both
  sides, so the test passed for the small fixed-seed datagen
  sample.

Detected when wiring up `temp && temp` (self-overlap) on tpcpatch
and finding 22/100 rows returning false.

The `Pcpatch.bounds[]` doc comment is updated to match the
upstream PCBOUNDS layout so future reviewers can't fall into the
same trap.

### Test plan

- [x] All existing `pointcloud/*` ctest pass.
- [x] `tpcpatch &&` self-overlap reflexivity holds for every row in `tbl_tpcpatch`.

### Reviewer notes

- Three-line, surgical change. Good candidate to land
  independently of the rest of the stack.
- The fix changes externally visible behaviour of every existing
  tpcpatch user — but in the direction of correctness.

---

## 2. Debug-build basetype assertion fix

**Title**: fix(pointcloud): include T_PCPOINT/T_PCPATCH in `temporal_basetype` debug helper

### Summary

`temporal_basetype()` in `meos/src/temporal/meos_catalog.c` is the
debug-build assertion helper used at every entry to the WKB /
TInstant codecs (`assert(temporal_basetype(basetype))`). It
enumerates every known base type but was missing the two
pgPointCloud entries — so a Debug build of MobilityDB segfaults
on the first `asBinary(tpcpoint)` or
`COPY tbl_tpcpoint TO ... (FORMAT BINARY)` call.

Release builds (NDEBUG) compile the assertion out, which is why
the issue went undetected — every CI run uses release optimisation.

Three lines under `#if POINTCLOUD` to add `T_PCPOINT` and
`T_PCPATCH`.

### Test plan

- [x] Debug build: `SELECT length(asBinary(tpcpoint(...)));` returns
      35 bytes instead of crashing the backend.
- [x] `420_tpcpoint_tbl` and `430_tpcpatch_tbl` (which exercise
      `COPY BINARY` round-trip) match expected outputs.
- [x] Release build behaviour unchanged.

### Reviewer notes

- The function is gated behind `#ifndef NDEBUG` and lists every
  other temporal base type (T_BOOL, T_INT4, T_GEOMETRY, T_NPOINT, …);
  the missing PCPOINT/PCPATCH entries are a follow-up correction
  to the type-registration commit.
- Independent of every other PR in this stack; can land before or
  after PR 1 without conflict.

---

## 3. WKB I/O with embedded schema

**Title**: feat(pointcloud): WKB I/O for tpcpoint and tpcpatch with schema embedding

### Summary

Adds binary I/O — `asBinary`, `tpcpointFromBinary`,
`tpcpatchFromBinary`, and `COPY ... FORMAT BINARY` — for tpcpoint
and tpcpatch.

The encoding is self-contained:

- **Per-Temporal**: `int32 pcid + int32 xml_len + utf-8 xml_bytes`
  after the optional SRID slot, gated by a new
  `MEOS_WKB_PCSCHEMAFLAG` (0x80) bit.
- **Per-instant**: `int32 body_length + body_bytes` (the varlena
  payload trimmed of pgPointCloud tail padding).

A receiver that doesn't already have the `pcid` registered
automatically parses and registers the embedded schema XML, so
`pg_dump --format=binary` works across clusters with no
out-of-band schema preload step.

### What changed

- `meos/src/temporal/{type_in.c,type_out.c}` — base value codec
  for `T_PCPOINT` / `T_PCPATCH`, schema-prefix header for
  `T_TPCPOINT` / `T_TPCPATCH`.
- `meos/src/temporal/meos_catalog.c` — `T_TPCPOINT` /
  `T_TPCPATCH` added to the `temporal_type()` predicate (was
  missing).
- `meos/{include,src}/pointcloud/meos_schema_hook.{h,c}` — schema
  cache extended to optionally hold XML; new
  `meos_pc_schema_register_xml`, `meos_pc_schema_xml`,
  `meos_pc_parse_xml_fn` hook.
- `mobilitydb/src/pointcloud/schema_cache.c` +
  `mobilitydb/src/geo/tgeo.c` — PG-side hooks register both XML
  and parsed schema at `mobilitydb_init`.
- New doc subsection "Binary representation and portability" in
  `temporal_pointcloud.xml`.

### Test plan

- [x] `420_tpcpoint_tbl`, `430_tpcpatch_tbl`: `COPY BINARY`
      round-trip + `asBinary(temp)::tpcpoint` round-trip both
      yield zero differing rows.
- [x] All ctest green.

### Reviewer notes

- The XML-parse hook is necessary because the MEOS layer must
  not depend on `libpc.a` directly (standalone-MEOS builds don't
  link it). The PG layer installs the parser at init time.
- Cross-cluster portability has no cluster-internal optimisation
  yet — every WKB blob carries the schema. A future optimisation
  could skip the schema bit when the receiver is the same backend,
  behind a flag.

---

## 4. Aggregates: extent / tcount / wcount / merge / append

**Title**: feat(pointcloud): aggregates extent / tcount / wcount / merge / append

### Summary

Adds the standard MobilityDB aggregate surface to the
pgPointCloud temporal types. Mirrors the cbuffer / tnpoint
aggregate set:

- `extent(tpcpoint | tpcpatch | tpcbox) → tpcbox` — bbox
  aggregate. Rejects mixed pcids (would yield uninterpretable
  dimensions).
- `tcount(tpcpoint | tpcpatch) → tint` and
  `wcount(tpcpoint | tpcpatch, interval) → tint`.
- `merge(tpcpoint) → tpcpoint`, `merge(tpcpatch) → tpcpatch`.
- `appendInstant`, `appendSequence` for streaming construction.

`extent` is a fresh transition function (`tpcbox_extent_transfn`
in MEOS, `Tpc_extent_transfn` PG); the others are pure SQL
plumbing on top of the generic `Temporal_*` aggfuncs machinery.

### Test plan

- [x] `440_tpc_aggfuncs_tbl`: pcid propagates from rows to
      aggregate result; every row's start/end timestamp is
      contained in the extent's time span.

### Reviewer notes

- `tcount` / `merge` require uniform subtype across input rows
  (the table-level test reflects this by aggregating from
  `tbl_tpcpoint_inst` etc., not the merged `tbl_tpcpoint`).
- `extent(tpcbox)` reuses its own transition function as the
  parallel combine; same shape as the stbox extent precedent.

---

## 5. Bbox topological + position operators with GiST and SP-GiST

**Title**: feat(pointcloud): bbox topological + position operators with GiST and SP-GiST

### Summary

Wires up the full MobilityDB bbox operator surface on tpcpoint,
tpcpatch, and tpcbox, and registers the matching GiST (R-tree)
and SP-GiST (quadtree + kd-tree) operator classes.

**Operators** between `(tpcpoint | tpcpatch)` and
`(tpcbox | tstzspan | self)`:

- Topological: `&&`, `@>`, `<@`, `~=`, `-|-`
- Directional X / Y / Z / time: `<<`, `&<`, `>>`, `&>`, `<<|`,
  `&<|`, `|>>`, `|&>`, `<</`, `&</`, `/>>`, `/&>`, `<<#`, `&<#`,
  `#>>`, `#&>`

**Opclasses**:

- GiST: `tpcbox_rtree_ops`, `tpcpoint_rtree_ops`,
  `tpcpatch_rtree_ops` (storage: `tpcbox`).
- SP-GiST: `tpcbox_quadtree_ops`, `tpcbox_kdtree_ops`,
  `tpcpoint_quadtree_ops`, `tpcpoint_kdtree_ops`,
  `tpcpatch_quadtree_ops`, `tpcpatch_kdtree_ops` (storage:
  `stbox`, lossy on `pcid`; recovered by recheck on the actual
  operator).

**Three small bridges** to keep cross-module dependencies
minimal:

- `tpcbox_set_stbox` (MEOS) — lossy `TPCBox → STBox` conversion.
- `Tpc_gist_compress` / `Tpc_spgist_compress` /
  `Tpcbox_spgist_compress` (PG) — index compress methods.
- `tspatial_spgist_get_stbox` (in `tspatial_spgist.c`) gains two
  branches for `T_TPCBOX` / `T_TPCPOINT` / `T_TPCPATCH` so the
  existing `Stbox_spgist_leaf_consistent` dispatches on
  tpointcloud RHS query types.

### Test plan

- [x] `435_tpc_topops_tbl`: reflexivity of topological ops;
      all-encompassing box contains every row.
- [x] `436_tpc_posops_tbl`: irreflexivity of strict directional
      ops on every row.
- [x] `437_tpc_gist_tbl`, `438_tpc_spgist_tbl`: index-scan and
      seq-scan counts agree on bbox-overlap queries.

### Reviewer notes

- Why STBox storage for SP-GiST instead of TPCBox? Reuses the
  existing `stbox_spgist_*` 1500-line implementation. Lossy on
  pcid is fine because the operator's recheck filters the rare
  false positive on the leaf entry. Same trade-off as cbuffer.
- The single shared `Tpc_gist_compress` works for both tpcpoint
  and tpcpatch because `temporal_set_bbox` dispatches per
  temptype to the right per-instant tpcbox setter.

---

## 6. Nearest-approach distance `|=|` with GiST KNN ordering

**Title**: feat(pointcloud): nearest-approach distance `|=|` with GiST KNN ordering

### Summary

Adds the `|=|` (nearest-approach distance) operator between any
pair of `(tpcbox, tpcpoint, tpcpatch)`, and wires it into the
existing GiST opclasses as KNN strategy 25 with a distance
support function. Mirrors cbuffer / npoint.

`SELECT … FROM tbl ORDER BY temp |=| query LIMIT N` is
index-accelerated.

### What changed

- MEOS-side: `nad_tpcbox_tpcbox`, `nad_tpointcloud_tpcbox`,
  `nad_tpointcloud_tpointcloud` in `tpc_boxops.c` — thin
  wrappers over the existing `nad_stbox_stbox` (using the
  `tpcbox_set_stbox` lossy conversion from PR 5).
- PG: `NAD_*` wrappers + `Tpcbox_gist_distance` support function
  that handles tpcbox / tpcpoint / tpcpatch query types.
- SQL: `nearestApproachDistance(...)` + `|=|` operator family on
  all 7 type pairings; `ALTER OPERATOR FAMILY` adds strategy 25 +
  function 8 to `tpcbox_rtree_ops`, `tpcpoint_rtree_ops`,
  `tpcpatch_rtree_ops`.

### Test plan

- [x] `439_tpc_distance_tbl`: self-distance is zero; GiST KNN
      top-5 ordering matches sequential scan.

### Reviewer notes

- pcid mismatch returns `DBL_MAX` (infinity), not zero — matches
  the contract that pcid mismatches mean "incomparable schemas".
- Distance is computed on the bounding boxes (the stbox-derived
  euclidean distance), with the operator recheck on leaf entries.

---

## 7. Spatial relationships eIntersects / eDisjoint / eDwithin for tpcpoint

**Title**: feat(pointcloud): spatial relationships eIntersects / eDisjoint / eDwithin for tpcpoint

### Summary

Adds the standard MobilityDB ever / always spatial relationship
functions on tpcpoint, against geometry and against another
tpcpoint:

- `eIntersects(...)` / `aIntersects(...)`
- `eDisjoint(...)` / `aDisjoint(...)`
- `eDwithin(..., dist float)` / `aDwithin(..., dist float)`

Implemented by projecting the tpcpoint to a tgeompoint via the
schema cache (reusing the static helpers behind
`Tpcpoint_to_tgeompoint`) and delegating to the existing
`ea_intersects_tgeo_*` / `ea_disjoint_tgeo_*` /
`ea_dwithin_tgeo_*` MEOS primitives.

**tpcpatch is intentionally out of scope** — patch-level
intersection requires per-point decompression and a different
design (deferred — see [Deferred work](#deferred-work-summary)).

### Test plan

- [x] `442_tpcpoint_spatialrels_tbl`: self-relations (every row
      eIntersects itself, none eDisjoint itself);
      eDwithin↔eIntersects-at-zero-distance equivalence;
      all-encompassing radius round-trip.

### Reviewer notes

- The new `tpcpoint_project_tgeompoint` static helper is
  extracted from the existing `Tpcpoint_to_tgeompoint` cast —
  same projection, exposed for reuse by the spatialrels wrappers.

---

## 8. MF-JSON output for tpcpoint and tpcpatch

**Title**: feat(pointcloud): MF-JSON output for tpcpoint and tpcpatch

### Summary

Adds `asMFJSON()` for tpcpoint and tpcpatch, closing the JSON I/O
gap relative to tcbuffer / tnpoint / tgeompoint. Today the call
errors out at `temptype_as_mfjson_sb`'s "Unknown temporal type"
branch.

- For **tpcpoint**: emits
  `{"type":"MovingPCPoint", … "coordinates":[[X,Y,Z], …]}`. X / Y / Z
  are extracted from each instant's pcpoint via the schema cache;
  if the schema lacks X / Y, the coordinate array is empty (still
  well-formed JSON).
- For **tpcpatch**: emits
  `{"type":"MovingPCPatch", … "values":[{"pcid":N,"npoints":N,"bounds":[xmin,xmax,ymin,ymax]}, …]}`.
  The compressed point payload is intentionally not in the JSON —
  use `asBinary` for round-trip.
- **Bbox embedding** (options=1): emits a TPCBox JSON with an
  extra `pcid` field next to the standard stbox-shaped bbox array.

### Test plan

- [x] `421_tpcpoint_mfjson_tbl`: type tag check; bbox embedding
      includes pcid; coordinate-array regex shape on per-instant
      tpcpoint output.
- [x] `432_tpcpatch_mfjson`: jsonb-cast structural assertions on
      type tag, per-instant value object (pcid, npoints, 4-element
      bounds), bbox embedding, sequence shape, datetimes-vs-values
      length parity.

### Reviewer notes

- Extends the existing `tinstant_as_mfjson_sb` /
  `tsequence_as_mfjson_sb` / `tsequenceset_as_mfjson_sb` dispatch
  — three small `else if` branches.
- Schema-aware coordinate extraction adds an O(1) cache lookup
  per instant.

---

## 9. Literal-value test suite

**Title**: test(pointcloud): literal-value tests for tpcpoint / tpcpatch operators

### Summary

Adds the standard MobilityDB literal-value test suite to match
the cbuffer / tnpoint / tgeompoint coverage pattern. Each
existing `*_tbl` table-level test gains a companion non-`_tbl`
literal test that exercises the operator surface on hand-built
sample values rather than reducing the datagen tables to scalars.

Files added:

- `420_tpcpoint.test.sql` — constructors, accessors, casts,
  comparisons, restrictions
- `421_tpcpoint_mfjson.test.sql` — `asMFJSON` output on
  instant / sequence / patch values
- `430_tpcpatch.test.sql` — same surface for tpcpatch
- `435_tpc_topops.test.sql` — `&&`, `@>`, `<@`, `~=`, `-|-`
  between tpcpoint / tpcpatch and tpcbox / tstzspan / self
- `436_tpc_posops.test.sql` — directional position ops on
  X / Y / Z / time axes, including irreflexivity checks
- `439_tpc_distance.test.sql` — `|=|` nearest-approach distance,
  self-distance zero, 3-4-5 right triangle, infinity on
  time-disjoint and pcid-mismatch
- `442_tpcpoint_spatialrels.test.sql` —
  eIntersects / aIntersects / eDisjoint / aDisjoint / eDwithin / aDwithin
  against geometry and self

Sample values are built inline via `PC_MakePoint` / `PC_Patch`
constructor calls and the `tpcpoint` / `tpcpatch` / `tpcpointSeq`
/ `tpcpatchSeq` builders.

### Test plan

- [x] All 7 files run and produce stable expected outputs.
- [x] Full ctest green: 168 tests pass.

### Reviewer notes

- pgPointCloud's `pcpoint_in` only accepts hex-WKB ("text form
  not yet implemented" upstream), so cbuffer-style human-readable
  string literals aren't available. Constructor-function
  expressions are the readable equivalent — same operator
  coverage, more verbose construction.
- Tests are pure additions; no production-code changes.

---

## 10. Per-point access on tpcpatch — numPoints and points SRF

**Title**: feat(pointcloud): per-point access on tpcpatch — `numPoints` and `points` SRF

### Summary

Adds two per-point accessors on tpcpatch, both without
per-instant patch decompression in C:

- `numPoints(tpcpatch) → bigint` — total points across every
  instant. Reads each instant's patch header (`pcpatch_npoints`
  is in the varlena header — no decompression needed); sums to
  int64 because a long sequence of dense LiDAR patches can exceed
  int32.
- `points(tpcpatch) → setof (timestamptz, pcpoint)` —
  set-returning function: one row per (instant timestamp, point)
  by walking each instant and decomposing the patch via
  pgPointCloud's `PC_Explode(pcpatch) → setof pcpoint`. Pure SQL
  wrapper, no new C code on the explosion path.

### What changed

- `mobilitydb/src/pointcloud/tpcpatch.c` — `Tpcpatch_npoints`
  accessor (~50 lines). Walks all subtypes (TInstant / TSequence
  / TSequenceSet) and sums per-instant `pcpatch_npoints`.
- `mobilitydb/sql/pointcloud/430_tpcpatch.in.sql` —
  `numPoints(tpcpatch)` C binding + `points(tpcpatch)` SQL SRF
  (`generate_series` × `LATERAL PC_Explode(...)`).
- `doc/temporal_pointcloud.xml` + `doc/reference.xml` — listitems
  for both accessors plus a deferral note for per-point
  restrictions (see "Limitations" below).
- `mobilitydb/test/pointcloud/queries/430_tpcpatch{,_tbl}.test.sql`
  — literal + table-driven assertions: per-instant equivalence
  with `startNumPoints` for TInstant,
  `numPoints == COUNT(*) FROM points()`,
  `numInstants == COUNT(DISTINCT t) FROM points()`.

### Test plan

- [x] Literal: `numPoints` = 2/4/8 on TInstant / TSequence /
      TSequenceSet values; `points` emits 2/4 rows correctly
      grouped per timestamp.
- [x] Table-driven over
      `tbl_tpcpatch{,_inst,_seq,_discseq,_seqset}`: all four
      assertions return `t`.

### Limitations (deferred)

Per-point restriction operators on tpcpatch — e.g. an
`atTpcbox` that filters individual points inside each patch, or
per-point `eIntersects(tpcpatch, geometry)` — are intentionally
out of scope. They require C-side patch decomposition, which
depends on a small upstream pgPointCloud surface change to expose
`pc_patch_deserialize` / `pc_point_serialize` from `lib/`
instead of `pgsql/`. The upstream ask has been communicated to
the pgPointCloud maintainers. See
[Deferred work](#deferred-work-summary).

### Reviewer notes

- `points(tpcpatch)` is roughly the cost of N `PC_Explode` calls
  on dense patches (one per instant). Correct but slow on
  multi-thousand-point patches; a fast path is gated on the
  upstream change above.
- The accessor and SRF are non-blocking on each other but kept
  in one PR because the test assertions cross-check them
  (`numPoints == COUNT(*) FROM points()`).
- No bbox-op, index, or aggregate surface change.

---

## 11. Ergonomic constructors `pcpoint(int, x, y, z)` / `pcpatch(int, …)`

**Title**: feat(pointcloud): ergonomic constructors `pcpoint(int, x, y, z)` / `pcpatch(int, …)`

### Summary

Three thin SQL constructors on top of pgPointCloud's
`PC_MakePoint` and `PC_Patch` so test literals,
`INSERT … VALUES`, and ad-hoc queries don't have to spell out
`ARRAY[…]::float[]` every time:

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

Pure SQL — no new C code. Schema-dimension validation still
happens inside `PC_MakePoint` / `PC_Patch`; these wrappers
inherit it.

### What changed

- `mobilitydb/sql/pointcloud/420_tpcpoint.in.sql` — two
  `pcpoint(…)` overloads.
- `mobilitydb/sql/pointcloud/430_tpcpatch.in.sql` — `pcpatch(…)`
  variadic wrapper.
- `mobilitydb/test/pointcloud/queries/420_tpcpoint.test.sql` and
  `430_tpcpatch.test.sql` — equivalence assertions vs. the
  verbose `PC_MakePoint` / `PC_Patch` form.
- `doc/temporal_pointcloud.xml` and `doc/reference.xml` —
  listitems for the new constructors next to the existing
  pcpoint / pcpatch accessors.

### Test plan

- [x] `pcpoint(1, 1, 1, 1)::text = PC_MakePoint(1, ARRAY[…])::text`.
- [x] `pcpatch(1, …)` produces the same value as the
      `PC_Patch(ARRAY[…])` form.
- [x] `numPoints(tpcpatch(pcpatch(1, …), …)) = 2`.

### Reviewer notes

- Function-name overloading on a type-named function is
  unambiguous in PG when arity > 1 (cast syntax `expr::pcpoint`
  is single-arg only).
- The `pcid` parameter on `pcpatch(...)` is reader-facing; the
  body forwards the variadic array to `PC_Patch`, which validates
  same-pcid.
- No production-code change in MEOS or the C wrapper layer.

---

## 12. pcid typmod for tpcpoint / tpcpatch (column-level pinning)

**Title**: feat(pointcloud): pcid typmod for tpcpoint / tpcpatch (column-level pinning)

### Summary

Adds the standard PG typmod surface to tpcpoint and tpcpatch so
a column or domain can pin a single pcid:

```sql
CREATE TABLE scans (id int, traj tpcpoint(1), full_scan tpcpatch(1));
INSERT INTO scans VALUES (1,
  tpcpoint(pcpoint(1, 1.0, 2.0, 3.0), '2024-01-01'),
  tpcpatch(pcpatch(1, pcpoint(1,1,1,1), pcpoint(1,2,2,2)), '2024-01-01'));
-- ERROR if any pcid disagrees with the column typmod
```

Mirrors PostGIS's `geometry(Point, 4326)` and MobilityDB's
existing `tgeompoint(Point, SRID)` patterns. Catches at
INSERT/cast time a class of mistake that today surfaces only
inside aggregates (or silently never).

### What changed

- `mobilitydb/src/pointcloud/tpc_typmod.c` — new file with
  `Tpc_typmod_in` (parses single-int pcid; rejects negative /
  zero / non-numeric / multi-arg), `Tpc_typmod_out` (renders
  `(pcid)` or empty when -1), `Tpc_enforce_typmod` (cast hook
  that raises on pcid mismatch).
- `mobilitydb/src/pointcloud/CMakeLists.txt` — wire the new file.
- `mobilitydb/sql/pointcloud/420_tpcpoint.in.sql` and
  `430_tpcpatch.in.sql` — register `tpc_typmod_in` / `_out` C
  functions, add them to `CREATE TYPE`, register the
  `tpcpoint(tpcpoint, integer)` / `tpcpatch(tpcpatch, integer)`
  cast via `Tpc_enforce_typmod` and a single
  `CREATE CAST … AS IMPLICIT`.
- `mobilitydb/test/pointcloud/queries/415_tpc_typmod.test.sql`
  (new) — full coverage: typmod_in error paths, `format_type`
  round-trip on pinned columns, INSERT match/mismatch on tpcpoint
  and tpcpatch, unconstrained column accepts mixed pcids,
  `ALTER TABLE … TYPE tpcpoint(2)` re-validates existing rows.
- `doc/temporal_pointcloud.xml` — typmod paragraph at the top of
  the tpcpoint section with a `CREATE TABLE` example showing
  both columns.

### Test plan

- [x] All four typmod_in error cases raise at parse time
      (`tpcpoint(-1)`, `tpcpoint(0)`, `tpcpoint(abc)`,
      `tpcpoint(1, 2)`).
- [x] Matching pcid on INSERT succeeds; mismatch raises with
      `Pcid of tpcpoint value (N) does not match column typmod pcid (M)`.
- [x] `format_type(atttypid, atttypmod)` prints `tpcpoint(1)` /
      `tpcpatch(1)` / `tpcpoint` (unconstrained) correctly.
- [x] `ALTER TABLE … ALTER COLUMN … TYPE tpcpoint(2)`
      re-validates existing rows and raises if any disagree.
- [x] No regressions in 420 / 430 tpcpoint / tpcpatch literal or
      table tests.

### Reviewer notes

- The typmod packs only a positive int32 pcid; -1 is
  unconstrained. No bit-packing tricks (only one field), so the
  layout is trivially comparable against future extensions.
- `Tpc_typmod_in` is hand-rolled `strtol` rather than
  `pg_strtoint32` because `<utils/builtins.h>` collides with
  `<json-c/json.h>` pulled in transitively by `meos_internal.h`
  (existing constraint in this repo — see
  `mobilitydb/src/pointcloud/tpcpoint.c`'s lead comment).
- The pcid-mismatch error mirrors the SRID-mismatch wording from
  `Tspatial_enforce_typmod`.
- `extent` aggregate's runtime pcid check is unchanged — it
  remains the safety net for unconstrained columns.

### Out of scope

- Mixed-pcid columns inside SP-GiST / GiST opclasses with TPCBox
  storage — the existing recheck-on-leaf design is unchanged.
- Multi-column typmod for SRID alongside pcid — pgPointCloud
  schemas pin SRID via the schema definition, so a separate
  typmod would be redundant today.

---

## 13. Standalone-MEOS plumbing for the pointcloud surface

**Title**: feat(pointcloud): wire pgPointCloud helpers into `libmeos.so`

### Summary

The pointcloud OBJECT lib was previously not wired into
`libmeos.so` — even with `MEOS=ON POINTCLOUD=ON`, the symbols
(`pcpoint_hex_in` / `pcpatch_hex_in` / etc.) weren't exported.
This PR adds the missing CMake glue and switches two PG-only
helpers to PostGIS equivalents so the same source files build
cleanly under `MEOS=ON`.

### What changed

- `meos/CMakeLists.txt` — `if(POINTCLOUD)` block adds the
  `pointcloud` OBJECT lib to the `meos` target sources.
- `meos/src/pointcloud/{pcpoint,pcpatch}.c` — replace PG's
  `hex_encode` / `hex_decode` with PostGIS's `parse_hex` /
  `deparse_hex` (from `liblwgeom`). Output is now uppercase, in
  line with MEOS's existing `HEXCHR` convention. Allows `MEOS=ON`
  builds, where PG headers are not on the include path.
- `meos/examples/tpc_wkb_roundtrip.c` (new) — a small worked
  example: hand-coded `SERIALIZED_POINT` seed → `pcpoint_hex_in`
  → `tinstant_make` → `temporal_as_wkb` → `temporal_from_wkb` →
  `temporal_eq`. Pins the MEOS-only WKB code path independently
  of any PG roundtrip.
- `mobilitydb/src/pointcloud/tpcpatch.c` — companion B-tree
  comparator audit: 17 literal-value assertions in
  `431_tpcpatch_cmp.test.sql` pinning the byte-wise
  pcid-then-payload semantics.

### Test plan

- [x] `MEOS=ON POINTCLOUD=ON` build: `nm libmeos.so | grep
      pcpoint_hex_in` shows uppercase `T` (exported).
- [x] `tpc_wkb_roundtrip` example builds and runs cleanly,
      asserting `temporal_eq` on the round-tripped value.
- [x] PG extension build unchanged (the same source files compile
      under both build configurations).
- [x] `431_tpcpatch_cmp` literal assertions pass.

### Reviewer notes

- The hex-encoding switch is observable: any external consumer
  that compared MEOS hex output character-by-character to PG's
  `hex_encode` output would now see uppercase letters. The byte
  semantics are unchanged.
- The example file is in `meos/examples/` alongside other MEOS
  smoke tests; build it manually as documented in the file's
  header comment.

---

## 14. Pointcloud quickstart in the user manual

**Title**: docs(pointcloud): add quickstart sect1 to the temporal pointcloud chapter

### Summary

Adds a "Quickstart" section at the top of the
`temporal_pointcloud` chapter walking a reader from "I have a
fresh database" to "I'm running indexed bbox queries against
moving sensor tracks" in six short subsections:

1. **Register a schema** — minimal 3D `int32` schema in
   `pointcloud_formats`.
2. **Build base and temporal values** — ergonomic constructors,
   typmod-pinned columns.
3. **Query with the bbox surface** — topological / directional /
   KNN operators between any pair of `(tpcbox, tpcpoint, tpcpatch)`.
4. **Index for bbox-driven scans** — GiST and SP-GiST opclass
   recipes.
5. **Aggregate** — `extent` / `tcount` / `merge`.
6. **Per-point access** — `points(tpcpatch)` SRF + `numPoints`.

The goal is to make every later section in this chapter pin to a
concrete usage. The quickstart is the first sect1 in the chapter
so it appears immediately after the chapter introduction.

### What changed

- `doc/temporal_pointcloud.xml` — new `tpointcloud_quickstart`
  sect1 with six sect2 subsections plus a `What's next` pointer
  back into the rest of the chapter.

### Test plan

- [x] `xmllint --xinclude --noout doc/mobilitydb-manual.xml`
      passes.
- [x] PDF and HTML renderers produce expected output (no broken
      xrefs, correct sect-numbering hierarchy).

### Reviewer notes

- All listed SQL examples are runnable against a fresh database
  with the registered schema in step 1.
- The quickstart deliberately does not cover MF-JSON, WKB
  portability, or the spatial-relationships surface — those are
  forward-pointed to from the `What's next` paragraph.

---

# Closing

## Deferred work summary

A single follow-up surface is intentionally deferred:

**Per-point operations on tpcpatch** — overloads of `atTpcbox` /
`minusTpcbox` that filter individual points inside each patch
(rather than keeping/dropping whole instants by bbox overlap),
plus per-point spatial relationships
(`eIntersects(tpcpatch, geometry)` over actual point coordinates
rather than the patch bounding box).

The blocker is that pgPointCloud's `pc_patch_deserialize` and
`pc_point_serialize` helpers live in pgPointCloud's PG-extension
side rather than in `libpc.a`, so MEOS — which links `libpc.a`
and not the PG extension — cannot reach them. A surface change
has been proposed upstream. Once it lands, the SQL
`points(tpcpatch)` SRF (PR 10) becomes a fallback alongside a
fast native path, and the per-point operators can be added in a
follow-up PR.

The bbox-driven `atTpcbox` / `minusTpcbox` operators on
`tpcpatch` that are already shipped operate at instant
granularity — an instant is kept or dropped as a whole based on
bbox overlap with its patch. Per-point bounding-box semantics are
the natural follow-up alongside the per-point spatial relationships.

---

## Pitfalls worth knowing before testing

- **Drop and recreate the extension after `make install`.** The
  catalog SQL is replaced; the existing extension instance still
  holds the old function table.

- **Debug builds** segfaulted in earlier states on every
  `asBinary(tpcpoint)` because `temporal_basetype()` (an
  assertion helper, compiled out under NDEBUG) was missing the
  pgPointCloud entries. Fixed by PR 2; mention here in case a
  colleague is bisecting an older state.

- **TZ-sensitive expected outputs.** The
  `mobilitydb/test/pointcloud/expected/*.test.out` files were
  generated under `PGTZ='America/Los_Angeles'` /
  `PGDATESTYLE='Postgres, MDY'`. The ctest driver sets these for
  you. Reproducing queries by hand requires the same env,
  otherwise you'll see harmless timestamp-format diffs.

- **`pcpoint` / `pcpatch` byte-level cmp/hash strip tail
  padding.** Two semantically-identical values produced by
  separate `palloc` calls always compare equal, even though
  their raw bytes differ in the trailing padding region. Any new
  code that builds a `Pcpatch` directly must respect this — the
  shipped helpers (`pcpoint_meaningful_size`,
  `pcpatch_meaningful_size`) wrap the truncation logic.

- **`PCBOUNDS` field order is `{xmin, xmax, ymin, ymax}`** —
  `xmax` comes second, not third. Earlier integration code mixed
  this up and produced inverted-axis tpcpatch bboxes (PR 1
  fixes this); the layout is now documented inline in
  `Pcpatch.bounds[]` and read by index, never by guess.

- **`<utils/builtins.h>` collides with `<json-c/json.h>`** when
  `meos_internal.h` is also in scope (`struct json_object` vs
  PG's `Datum json_object`). Use `parse_hex` / `deparse_hex` from
  `liblwgeom`, or hand-roll the equivalent — see the existing
  pattern in `mobilitydb/src/pointcloud/tpcpoint.c` lead comment.

- **Schema cache lifetime.** Registered schemas are palloc'd in
  `TopMemoryContext` and outlive the query that registered them.
  Don't `pfree` the returned `PCSCHEMA *` pointer — the cache
  owns it.

---

## Where to read more

- **User manual chapter**: `doc/temporal_pointcloud.xml` —
  human-readable reference covering every type, function, and
  operator with inline SQL examples. The Quickstart sect1 (PR 14)
  is the recommended entry point.
- **Doxygen**: `cmake -DDOC_DEV=ON . && make doc_dev` from the
  build directory; HTML output at `doxygen/docs/html/index.html`.
  Pointcloud-specific groups: `meos_pointcloud_*` (MEOS) and
  `mobilitydb_pointcloud_*` (PG wrappers).
- **Worked examples**: `meos/examples/tpcbox_rtree.c` (R-tree
  GiST walk) and `meos/examples/tpc_wkb_roundtrip.c` (MEOS-only
  WKB roundtrip).
