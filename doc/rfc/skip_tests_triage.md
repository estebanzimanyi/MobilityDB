# Triage: SKIP_TESTS lists across the MobilityDB regression suite

## Status

Coveralls reports a coverage drop of **−11.0%** to **83.93%** on master
(commit `ee27da1a6`, NAD failure sentinel mismatch fix). The drop is
not from the change itself — its diff covers 2 lines — but from the
cumulative effect of **8 SQL regression tests left in `SKIP_TESTS`**
across the cbuffer and rgeo suites. Each skip silences a real upstream
behavioural drift; the underlying functions are no longer exercised by
the regression suite, so their `.c` files report 0–26 % line coverage.

This document catalogues each skipped test, the symptom that caused it
to be skipped, and what's needed to re-enable it.

## File-level coverage hotspots (`coveralls/79234435`)

| File                                          | rel  | cov  | miss | %    |
|----------------------------------------------:|:----:|:----:|:----:|:----:|
| `meos/src/rgeo/trgeo_distance.c`              |  830 |    0 |  830 |   0  |
| `meos/src/cbuffer/tcbuffer_tempspatialrels.c` |  293 |    0 |  293 |   0  |
| `meos/src/rgeo/trgeo_vclip.c`                 |  263 |    0 |  263 |   0  |
| `meos/src/cbuffer/tcbuffer_spatialrels.c`     |  250 |    0 |  250 |   0  |
| `meos/src/cbuffer/tcbuffer_spatialfuncs.c`    |  231 |   15 |  216 | 6.5  |
| `mobilitydb/src/rgeo/trgeo_distance.c`        |  238 |   22 |  216 | 9.2  |
| `mobilitydb/src/cbuffer/tcbuffer_distance.c`  |  244 |   58 |  186 | 23.8 |
| `mobilitydb/src/pose/tpose_distance.c`        |  193 |   15 |  178 | 7.8  |
| `meos/src/cbuffer/tcbuffer_distance.c`        |  233 |   61 |  172 | 26.2 |
| `meos/src/pose/tpose_distance.c`              |   94 |    0 |   94 |   0  |
| `mobilitydb/src/rgeo/trgeo_vclip.c`           |  122 |    6 |  116 | 4.9  |

## Skipped tests

### `mobilitydb/test/cbuffer/CMakeLists.txt`

```
160_tcbuffer_distance_tbl         # NearestApproachDistance / |=| count drift
162_tcbuffer_spatialrels          # spatialrels output drift
162_tcbuffer_spatialrels_tbl      # spatialrels output drift (table form)
164_tcbuffer_tempspatialrels      # tempspatialrels output drift
164_tcbuffer_tempspatialrels_tbl  # tempspatialrels output drift (table form)
```

#### 160 — NearestApproachDistance / `|=|` count drift

Symptom: a count-of-non-NULL like `SELECT count(*) FROM ... WHERE a |=| b
IS NOT NULL` drifted by N rows because the MEOS-side `nad_tcbuffer_*`
returned −1.0 on failure while the PG-side wrapper checked for `DBL_MAX`.

Status: master commit `ee27da1a6` aligned the sentinels. The expected
output of `160_tcbuffer_distance_tbl` was never regenerated and the
test stayed skipped. **Re-enable + regenerate** is mechanical now.

#### 162 / 164 — tcbuffer (temp-)spatialrels output drift

Symptom: `tdisjoint_tcbuffer_*` and `tintersects_tcbuffer_*` paths
crash inside the spanset machinery — surfaced reliably by the MEOS
smoke suite. The errors visible in stderr are

```
ERROR: type %s is not a span type
ERROR: Unknown compare function for type
```

Root cause: the spanset-construction code for tcbuffer's spatial-rels
path passes an unmapped `MeosType` to `spantype_basetype` /
`spantype_spansettype`. The `meostype_name(...) → NULL` defensive fix
on `fix/meos-bug-tpose-to-tpoint` lets the error message survive
without segfaulting; the underlying type-mapping gap remains.

**Fix needed:** add the missing entry in
`meos/src/temporal/meos_catalog.c::MEOS_SPANTYPE_CATALOG` (or its
spansettype counterpart) for the type tcbuffer's spatial-rels code is
trying to span.

### `mobilitydb/test/rgeo/CMakeLists.txt`

```
122_trgeo               # Operation on mixed SRID drift
124_trgeo_compops       # Operation on mixed SRID drift
124_trgeo_compops_tbl   # Operation on mixed SRID drift (table form)
```

#### 122 / 124 — Operation on mixed SRID

Symptom: `trgeometry '...';[Pose(...)]@... ?= geometry '...'` — TINSTANT
inputs return the expected `f`, **TSEQUENCE / TSEQUENCESET inputs error
with "Operation on mixed SRID"** even when both operands have
`srid = 0`.

Reproduction (both SRIDs print as 0):

```sql
SELECT srid(trgeometry 'Polygon((1 1,2 2,3 1,1 1));[Pose(Point(1 1), 0.2)@2000-01-01, Pose(Point(1 1), 0.4)@2000-01-02, Pose(Point(1 1), 0.5)@2000-01-03]');  -- 0
SELECT st_srid(geometry 'Polygon((1 1,2 2,3 1,1 1))');                                                                                                       -- 0
SELECT trgeometry '...;[Pose...]' ?= geometry 'Polygon...';
-- ERROR: Operation on mixed SRID
```

The `ensure_valid_trgeo_geo` SRID gate passes (both 0). The error fires
deeper in the lifted-function path for sequence inputs only — likely a
per-instant SRID comparison between the trgeo's pose and the materialised
hull, where one of the two has been re-tagged with a non-zero SRID by an
intermediate constructor.

**Fix needed:** trace `eacomp_temporal_base → eafunc_temporal_base`
for TSEQUENCE inputs of trgeometry; identify which intermediate retags
the SRID to non-zero. Likely in `geom_apply_pose` or
`trgeoinst_geom_p`'s lookup path.

## Status of each reactivation (this triage)

1. **160_tcbuffer_distance_tbl** — ✅ shipped on
   `ci/reactivate-160-tcbuffer-distance-tbl`. Master commit
   `ee27da1a6` already aligned the NAD failure sentinel; the test
   passes immediately on dropping the SKIP entry.

2. **162_tcbuffer_spatialrels [+_tbl]** — ✅ shipped on
   `ci/reactivate-162-tcbuffer-spatialrels`. The expected outputs
   were missing (cmake auto-disable rule). Generated against current
   master, both forms pass cleanly. The 11 ERROR lines in the
   standalone form are intentional `/* Errors */` mixed-SRID checks.

3. **122_trgeo / 124_trgeo_compops [+_tbl]** — ⏳ blocked on PR
   `MobilityDB/MobilityDB#849`
   (`fix-trgeo-cross-type-lifting`, currently OPEN/MERGEABLE).
   The sequence-only "Operation on mixed SRID" surfaces because the
   lifting infrastructure crashes on trgeo↔geometry cross-type
   comparisons; #849 is the targeted fix.

4. **164_tcbuffer_tempspatialrels [+_tbl]** — ⏳ blocked on PR
   `MobilityDB/MobilityDB#847`
   (`feat/cbuffer-tempspatialrels-2d-only`, currently OPEN). #847
   enables the two missing `tDwithin(geometry, tcbuffer, ...)`
   overloads, drops the dead 3D tests that referenced the
   never-existing `tbl_tcbuffer3d` table, and ships the expected
   outputs for both forms.

After #847 and #849 merge, the remaining four SKIP entries can drop
together. Coverage should climb back close to its pre-merge baseline
once all six branches / PRs land.

Review-and-merge order:

  - `ci/reactivate-160-tcbuffer-distance-tbl` (independent)
  - `ci/reactivate-162-tcbuffer-spatialrels`  (independent)
  - `MobilityDB/MobilityDB#847` → drop 164 SKIPs after merge
  - `MobilityDB/MobilityDB#849` → drop 122/124 SKIPs after merge
