# `tgeo-fast-clip-rebased` correctness audit — DO NOT MERGE AS-IS

**Status (2026-04-26):** the fast trajectory-vs-polygon clipper on
`origin/tgeo-fast-clip-rebased` (`f56a06776 feat(geo): port fast trajectory-vs-polygon clipper from tcbuffer_fix`)
contains two independent bugs. **Bug A** is a one-line fix; **Bug B** is a
structural defect in the parity sweep that cannot be patched surgically. Until
Bug B is addressed, merging this branch would silently produce wrong answers
for any tpoint trajectory that enters *and* exits a polygon — the common case.

This document records the audit so the next session can pick up the
trajectory-clip work without re-deriving any of it.

---

## Why the bugs were not caught in the original development

The branch was developed and tested against a build where the trajectory-vs-polygon
dispatch in `tpointseq_linear_restrict_geom` (added by this branch) was *wired*,
but where the loaded `libMobilityDB-1.4.so` at `$libdir` was the master build
without the dispatch. PostgreSQL extension `probin` strings carry an explicit
`$libdir/` prefix, which **bypasses `dynamic_library_path` entirely** — the
loader always reads from the build-time `pkglibdir`. So `CREATE EXTENSION
mobilitydb` resolved every function to the master `.so`, and every
`atGeometry(tgeompoint, polygon)` query fell through to PostGIS GEOS via
`tpointseq_linear_at_geom`. Outputs looked correct because GEOS was correct;
`tpointseq_linear_at_poly` was never actually invoked.

This was confirmed during the audit by inserting `elog(ERROR, "FAST_CLIP_TRACE
…")` at the top of `tpointseq_linear_at_poly` — the query succeeded with no
error and no NOTICE, proving the function was unreached. After installing the
audit build over `$libdir/libMobilityDB-1.4.so` (so `CREATE EXTENSION` actually
loads the audited code), both bugs manifest immediately on the simplest inputs.

**Methodology lesson:** before claiming any validation against a real PG cluster,
run `nm $libdir/libMobilityDB-1.4.so | grep <new-symbol>` to confirm the loaded
binary actually contains the change under test. `dynamic_library_path` is a
red herring for extension functions.

---

## Bug A — inverted horizontal-edge filter

**File:** `meos/src/geo/tgeo_fast_clip.c`, lines 282–285:

```c
diff = e.y1 - e.y2;
if (diff > POSTGIS_FP_TOLERANCE || diff < -POSTGIS_FP_TOLERANCE ||
    xmax < e.xmin || xmin > e.xmax || ymax < e.ymin || ymin > e.ymax)
  continue;
```

The comment above this block reads *"Skip horizontal edges and perform bounding
box test"*. The boolean is the opposite: `(|diff| > eps || …) → continue` skips
**every non-horizontal edge** (and bbox-disjoint horizontal edges). For a
typical convex polygon with mostly diagonal edges, virtually every edge is
silently dropped before reaching the segment-segment intersection test.

**Repro (after installing the audit build):**

```sql
SELECT asText(atGeometry(
  tgeompoint '[POINT(0 5)@2026-01-01 00:00, POINT(10 5)@2026-01-01 02:00]',
  'POLYGON((0 0,10 0,5 10,0 0))'::geometry));
-- Pre-fix output (Bug A active): empty result
-- Expected (PostGIS reference): (2.5,5)→(7.5,5)
```

With instrumentation, the per-edge skip log for the triangle is:

```
edge #0 (0,0)->(10,0)  diff=0    SKIPPED  ← horizontal, but bbox-disjoint with traj at y=5
edge #1 (10,0)->(5,10) diff=-10  SKIPPED  ← Bug A: non-horizontal, falsely skipped
edge #2 (5,10)->(0,0)  diff=10   SKIPPED  ← Bug A: non-horizontal, falsely skipped
```

**Fix (one line):** invert the diff test from non-horizontal to horizontal:

```c
if ((diff < POSTGIS_FP_TOLERANCE && diff > -POSTGIS_FP_TOLERANCE) ||
    xmax < e.xmin || xmin > e.xmax || ymax < e.ymin || ymin > e.ymax)
  continue;
```

After this change, the same triangle repro reaches `linesegm_intersection`
for both diagonal edges — they correctly report crossings at `t=0.25`
(left, x=2.5) and `t=0.75` (right, x=7.5). But the answer is *still* wrong
because of Bug B.

---

## Bug B — monotonic parity accumulation (structural)

**File:** `meos/src/geo/tgeo_fast_clip.c`. Three pieces of code interact:

1. Edge tagging (`poly_extract_edges`, line 98): every edge of a ring carries
   the same parity:
   ```c
   int parity = (r == 0) ? 1 : -1;     // outer ring: +1, holes: -1
   ```
2. Crossing event emission (`linesegm_clip`, line 300): each crossing pushes
   the edge's parity unchanged into the event list.
3. Parity sweep (`linesegm_clip`, lines 318–346):
   ```c
   int inside = 0;
   for (int i = 0; i < n - 1; i++) {
     inside += events[i].parity;
     if (inside > 0) /* emit segment (events[i].t, events[i+1].t) */
   }
   ```

For a trajectory crossing a convex outer polygon at `t=0.25` (entry) and
`t=0.75` (exit), both events carry `parity=+1`. The sweep produces:

| step | event | `inside` after | emit? |
|---|---|---|---|
| 1 | `t=0.0`, parity 0 | 0 | no |
| 2 | `t=0.25`, parity +1 | **1** | yes — `(0.25, 0.75)` |
| 3 | `t=0.75`, parity +1 | **2** | yes — `(0.75, 1.0)` ← **wrong** |

Once `inside > 0`, every subsequent interval gets emitted. The exit at `t=0.75`
is never recognised as an exit — it is treated as a *second entry*. So the
trajectory is reported as inside-the-polygon all the way through to its end.

**Confirmed empirically** with the audit build (Bug A fix applied so edges are
processed):

```
TRIANGLE: (0 5)→(10 5) clipped against POLYGON((0 0,10 0,5 10,0 0))
  expected: (2.5,5)→(7.5,5)
  got:      (2.5,5)→(10,5)            ← exit lost; trajectory exits at (7.5,5)

DIAMOND:  (-1 0)→(11 0) clipped against POLYGON((5 -5,10 0,5 5,0 0,5 -5))
  expected: (0,0)→(10,0)
  got:      (0,0)→(11,0)               ← exit lost; trajectory exits at (10,0)
```

This is a **structural** defect of the algorithm as written. A parity sweep
that produces correct entry/exit pairs from edge crossings needs one of:

- **Sign-by-direction** (Hormann-Agathos-style): the per-event sign is
  `+1` if the polygon edge crosses the trajectory ray from below to above,
  `-1` for the opposite, computed from the edge's `(y2 - y1)` vs the
  trajectory's perpendicular direction. Edge crossings come in pairs and
  the sweep returns to `inside = 0` after each exit. This requires
  rewriting the per-edge parity computation in `linesegm_clip` and
  changing what `e.parity` means.

- **XOR alternation** with `inside ^= 1` instead of `inside += parity`.
  Simpler but loses the +1/-1 distinction needed for nested holes
  (which the current code uses parity ±1 for).

- **Replace the parity sweep entirely** — see "Recommended path" below.

There is no surgical patch to lines 318–346 that preserves the rest of
the algorithm and produces correct output. The `(r == 0) ? 1 : -1`
parity assignment in `poly_extract_edges` and the `inside > 0` test in
the sweep are co-designed in a way that doesn't model entry/exit
semantics.

---

## Recommended path forward

**Replace `linesegm_clip` and `linesegm_intersection` with Clipper2 open-path
clipping.** Clipper2 supports clipping `Paths64` with `is_open=true` against a
closed polygon (`AddOpenSubject(...) + AddClip(...) + Execute(ClipType,
FillRule, closed_unused, open_solution)`), returning open paths that are
exactly the trajectory-inside-polygon segments we want. Clipper2 is already
vendored on `clip-clipper2-prod` (`clipper2/`), already int64-robust
by construction, and the build pipeline already pulls in C++17.

**Rough sketch:**

```cpp
// New file or extension to meos/src/geo/clip_clipper2.cpp
extern "C" TSequenceSet *
clipper2_clip_traj_poly(const TSequence *seq, const GSERIALIZED *gs)
{
  // 1. Convert seq's (x,y) sequence to a single open Path64 (scale 1e7 like
  //    the closed-poly path; preserve a parallel TimestampTz array indexed
  //    by trajectory parametric t).
  // 2. Convert gs (POLYGON/MULTIPOLYGON, with holes) to closed Paths64.
  // 3. Clipper64::AddOpenSubject(open_path); AddClip(closed_paths);
  //    Paths64 closed_unused; Paths64 open_solution;
  //    c.Execute(ClipType::Intersection, FillRule::EvenOdd, closed_unused, open_solution);
  // 4. For each open output path, walk its vertices and look up the t-fraction
  //    on the original trajectory (binary search on the input sequence's
  //    arc-length-vs-time table). Reconstruct TInstant pairs.
  // 5. Return TSequenceSet.
}
```

**Z-span filter** stays as a *pre-pass* on the trajectory before the open-path
clip (drop instants outside `zspan`, split the trajectory at the dropped
boundaries) or a *post-pass* on the output sequence set (`tnumber_restrict_span`
on the Z coordinate, then time-restrict). This is what
`tpointseq_linear_restrict_geom` already does for the GEOS path; the same
post-pass works here.

**Effort:** ~6h of focused work. Net change vs the broken parity sweep:
likely `−400 LOC` because Clipper2 absorbs `linesegm_clip`,
`linesegm_intersection`, the event-buffer machinery, and the dedup/sort
logic. `poly_extract_edges` and `tpointseq_clip` stay (they handle the
trajectory-side bookkeeping that Clipper2 doesn't know about).

**Speed expectation:** likely 2–3× slower than a *correctly-implemented* parity
sweep would be (Clipper2 has int64 scaling overhead), but still substantially
faster than the GEOS round-trip (no GSERIALIZED → LWGEOM → GEOSGeom →
GEOSIntersection → reverse). Specifically, the GEOS path must convert the
entire trajectory to a GEOSGeom *every call*; Clipper2's `Paths64` is a flat
`vector<Point64>` we can build in one pass.

**Don't bother fixing fast_clip's parity logic in place.** The Clipper2 vendor
is already in the build and proven correct on the polygon-Boolean side; it's
also what the rest of `clip-clipper2-prod` standardises on. A correctly-fixed
parity sweep would be roughly the same effort as the Clipper2 rewrite and
would leave us with a second engine to maintain.

---

## Audit ground state

To reproduce the bugs from a fresh checkout:

```bash
# 1. Branch off clip-clipper2-prod, merge in fast-clip
git worktree add -b fast-clip-audit /tmp/fast-clip-audit clip-clipper2-prod
cd /tmp/fast-clip-audit
git merge --no-ff origin/tgeo-fast-clip-rebased

# 2. Build, install over $libdir (CRITICAL — see "Methodology lesson" above)
mkdir -p build && cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
sudo cp build/libMobilityDB-1.4.so /usr/local/pgsql/17/lib/libMobilityDB-1.4.so

# 3. Drop and recreate the test database (any prior CREATE EXTENSION
#    captured the old probin pointing at $libdir/libMobilityDB-1.4 which
#    is fine — but a stale catalog from manual function registrations
#    blocks CREATE EXTENSION).
psql -c "DROP DATABASE IF EXISTS mart_test; CREATE DATABASE mart_test;"
psql -d mart_test -c "CREATE EXTENSION postgis; CREATE EXTENSION mobilitydb;"

# 4. Run Bug A / Bug B repros (psql input above).
```

To inject diagnostics without rebuilding repeatedly, add `elog(NOTICE, ...)`
inside `tpointseq_linear_at_poly` and the per-edge filter in `linesegm_clip`,
then `SET client_min_messages TO NOTICE` in psql.

## Pinned references

- Branch tip at audit time: `origin/tgeo-fast-clip-rebased` =
  `f56a06776 feat(geo): port fast trajectory-vs-polygon clipper from tcbuffer_fix`
- `clip-clipper2-prod` tip at audit time:
  `eadbab710 feat(clipper2): wire clip_poly_poly to Clipper2; route atGeometry through it`
- Clipper2 vendor: `clipper2/` (v2.0.1)
- Clipper2 open-path API: `Clipper2Lib::Clipper64::AddOpenSubject`,
  `Execute(ClipType, FillRule, Paths64& closed, Paths64& open)`
- Companion memory entry: `project_fast_clip_audit.md` (pointer summary
  for the next session)
