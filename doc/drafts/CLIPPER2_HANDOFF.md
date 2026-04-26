# Clipper2 migration — handoff for the dedicated next session

**Read this file first** when resuming work on the polygon-Boolean engine migration. Companion to the auto-memory entry `project_clipper2_migration.md` (which has the higher-level "why" and branch inventory).

## What you have when you start

You're checked out on (or branching from) `clip-clipper2-spike`. The spike already proved:

- Clipper2 v2.0.1 vendors cleanly into `meos/vendor/clipper2/`
- C++17 enabled at top-level CMakeLists works (no manual CXXFLAGS gymnastics)
- `libstdc++` auto-pulls into the final `.so` (CMake detects the C++ TUs in the link set)
- A C-callable `extern "C"` shim from C++ is loadable by PostgreSQL via `LOAD`
- `clipper2_intersect_spike(GSERIALIZED *, GSERIALIZED *) → GSERIALIZED *` returns geometrically-correct output for a single-POLYGON intersection (validated against PostGIS `ST_Intersection`)

Existing files on `clip-clipper2-spike`:

```
meos/vendor/clipper2/
  LICENSE                       # Boost Software License 1.0 (PostgreSQL-compatible)
  CMakeLists.txt                # OBJECT lib `clipper2`, C++17, PIC
  include/clipper2/
    clipper.h                   # umbrella, trimmed for un-vendored modules
    clipper.core.h
    clipper.engine.h
    clipper.rectclip.h
    clipper.version.h           # contains "2.0.1"
  src/
    clipper.engine.cpp          # ~88 KB
    clipper.rectclip.cpp        # ~28 KB

meos/include/geo/clip_clipper2.h   # extern "C" header for the spike
meos/src/geo/clip_clipper2.cpp     # ~110 LOC spike adapter + V1 wrapper
```

CMake changes:
- Top-level `CMakeLists.txt`: `project(... LANGUAGES C CXX)`, `set(CMAKE_CXX_STANDARD 17)`, `_REQUIRED ON`, `_EXTENSIONS OFF`.
- `meos/src/geo/CMakeLists.txt`: `add_subdirectory(...vendor/clipper2)` and a separate `clip_clipper2_spike` OBJECT lib for the adapter.
- `meos/CMakeLists.txt` and `mobilitydb/CMakeLists.txt`: both PROJECT_OBJECTS lists include `clip_clipper2_spike` and `clipper2`.

## Production work — concrete steps

### Step 0: branch off the spike

```bash
git fetch origin
git worktree add -b clip-clipper2-prod /tmp/mobilitydb-clipper2-prod clip-clipper2-spike
cd /tmp/mobilitydb-clipper2-prod
mkdir -p build && cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
# Confirm clean build before any changes.
```

### Step 1: Extend the adapter (the meaty work, ~18h)

File: `meos/src/geo/clip_clipper2.cpp` (and matching header).

Production C signature to provide (replaces the spike's `clipper2_intersect_spike`):

```c
extern "C"
GSERIALIZED *clipper2_clip_poly_poly(const GSERIALIZED *subj,
                                     const GSERIALIZED *clip,
                                     int op);  // 0=Inter, 1=Union, 2=Diff, 3=Xor
```

Internally:

1. **Input conversion** (GSERIALIZED → Clipper2 `Paths64`):
   - Deserialize via `lwgeom_from_gserialized`.
   - Accept POLYGONTYPE and MULTIPOLYGONTYPE (already validated as planar 2D upstream).
   - For each polygon's rings: outer ring + holes all go into the same `Paths64` for that polygon. Clipper2 handles the inside/outside via fill rule.
   - Round each `(x, y)` to `Point64{ llround(x*1e7), llround(y*1e7) }`. Drop the closing duplicate vertex (Clipper2 paths are open).
   - Build `Paths64 subjects` and `Paths64 clips` separately.

2. **Operation dispatch**:
   ```cpp
   ClipType ct = static_cast<ClipType>(op);  // matches 0..3
   ```
   `ClipOper` enum in `meos/include/geo/geo_poly_clip.h` already happens to match Clipper2's `ClipType` ordering — verify this once and keep it that way, OR map explicitly via switch (safer).

3. **Run Clipper2 with PolyTree64** (NOT Paths64, because we need parent/hole topology):
   ```cpp
   Clipper64 c;
   c.AddSubject(subjects);
   c.AddClip(clips);
   PolyTree64 polytree;
   c.Execute(ct, FillRule::EvenOdd, polytree);
   ```

4. **Output conversion** (PolyTree64 → LWGEOM):
   - PolyTree64 is a tree where each node has `Polygon()` (a `Path64`) + children (`Count()`, `Child(i)`).
   - Recursive walk:
     ```
     void walk(PolyPath64 *node, std::vector<LWGEOM *> &polys) {
       if (!node->IsHole()) {
         // node is an exterior — start a new LWPOLY
         LWPOLY *poly = lwpoly_construct_empty(srid, LW_FALSE, LW_FALSE);
         lwpoly_add_ring(poly, path_to_pa(node->Polygon()));
         for (auto &child : *node) {
           if (child->IsHole()) {
             lwpoly_add_ring(poly, path_to_pa(child->Polygon()));
             // descend into hole's children — they are NEW exteriors (islands in holes)
             for (auto &gc : *child) walk(gc.get(), polys);
           } else {
             // shouldn't happen at the exterior level, but defensive
             walk(child.get(), polys);
           }
         }
         polys.push_back(lwpoly_as_lwgeom(poly));
       } else {
         // top-level hole? Treat as exterior (Clipper2 sometimes emits these)
         // Same as above, defensively.
       }
     }
     ```
   - Walk all top-level children of `polytree`. Collect `polys`. If 1 poly → return as LWPOLY; if >1 → wrap in LWMPOLY via `lwcollection_construct(MULTIPOLYGONTYPE, srid, NULL, n, polys.data())`.
   - `geo_serialize` to GSERIALIZED.

5. **`path_to_pa(const Path64 &)`** helper:
   - `POINTARRAY *pa = ptarray_construct_empty(LW_FALSE, LW_FALSE, path.size() + 1);`
   - For each `Point64 pt`: append `POINT4D{ pt.x / 1e7, pt.y / 1e7, 0.0, 0.0 }`.
   - Close the ring by appending the first point again.
   - Return `pa`.

6. **Empty-result handling**: if `polytree.Count() == 0` after Execute, return `nullptr`. The wrapper at `tgeo_spatialfuncs.c` already PG_RETURN_NULLs on null.

### Step 2: Replace `clip_poly_poly` body (~2h)

File: `meos/src/geo/geo_poly_clip.c` lines ~1320-1392 (the entire `clip_poly_poly` function body).

Strategy: **delete almost everything**. Keep only:
- The validation block (`gserialized_is_empty` checks at the top, the bbox-disjoint short-circuit at lines 1386-1396).
- Replace the entire `fill_queue → subdivide_segments → order_events → connect_edges → contours_to_geom` chain with a single call to `clipper2_clip_poly_poly(subj, clip, oper)`.

Result: `clip_poly_poly` shrinks to ~25 lines. The four `_mdb_internal_clip_*` SQL functions in `mobilitydb/sql/geo/056_tpoint_spatialfuncs.in.sql` and the C wrappers in `mobilitydb/src/geo/tgeo_spatialfuncs.c` are **zero-change** because they just call `clip_poly_poly`.

### Step 3: Regenerate test outputs (~4h)

File: `mobilitydb/test/geo/expected/078_tpoint_clipping.test.out`

Clipper2's vertex-start rotation will differ from Martinez's. Expected output strings WILL change, even when the geometries are equivalent. Process:

```bash
# Build the prod branch's .so.
# Install (if comfortable) or use the private cluster trick from the spike session.
# Run the test:
cd /tmp/mobilitydb-clipper2-prod/build
make test || true   # will fail because expected differs from actual
# Inspect each actual vs expected pair:
diff mobilitydb/test/geo/expected/078_tpoint_clipping.test.out \
     mobilitydb/test/geo/078_tpoint_clipping.test.out.actual

# For each diff:
#   - Eyeball the geometries side-by-side
#   - Run BOTH through ST_Equals and ST_OrderingEquals against PostGIS reference
#   - If geometries equivalent (ST_Equals true), accept the new output
#   - If not, you have a real regression — investigate
```

Then re-enable the disabled reproducer at `mobilitydb/test/geo/queries/078_tpoint_clipping.test.sql:65-66`:

```sql
-- Was disabled because Martinez crashed/wrong-answered on this input.
-- Clipper2 should handle it correctly.
SELECT st_astext(_mdb_internal_clip_intersection(
  geometry 'Polygon((1 1,1 5,5 5,5 1,1 1),(2 2,4 2,4 4,2 4,2 2))',
  geometry 'Polygon((0 3,3 0,6 3,3 6,0 3))'));
```

Expected (PostGIS reference):
```
POLYGON((1 4,2 5,4 5,5 4,5 2,4 1,2 1,1 2,1 4),(4 2,4 4,2 4,2 2,4 2))
```

Clipper2 output should be the same polygon (an octagon with the original square hole) modulo vertex-start rotation.

### Step 4: Delete Martinez code (~1h)

```bash
git rm meos/src/geo/geo_poly_clip.c           # 1395 lines
git rm meos/include/geo/geo_poly_clip.h
git rm meos/src/geo/pqueue.c                   # 206 lines
git rm meos/include/geo/pqueue.h
git rm meos/src/geo/splay_tree.c               # 791 lines
git rm meos/include/geo/splay_tree.h
# Total: ~2660 LOC deleted
```

Edit `meos/src/geo/CMakeLists.txt`: remove `geo_poly_clip.c`, `pqueue.c`, `splay_tree.c` from `GEO_SRCS`.

The single call site in `mobilitydb/src/geo/tgeo_spatialfuncs.c:834` (the `_mdb_internal_clip_*` wrappers) keeps calling `clip_poly_poly` — but `clip_poly_poly` now lives in `clip_clipper2.cpp` (or move it to a small new C file `clip_poly_poly.c` if you prefer to keep the production C entry point in C land — your call).

### Step 5: Verify the deletion didn't break anything (~16h)

- Full MobilityDB test suite (`make test` in build dir) — covers all temporal types, not just clipping.
- Cross-platform CI shake-out: at minimum Linux gcc + clang, ideally also macOS clang and Windows MSVC.
- Specific spot-checks:
  - 3D rejection still works (wrapper-level fix from Round-1 audit).
  - Geography rejection still works (same).
  - SRID-mismatch error still fires.
  - All four operations (intersection / union / difference / xor) produce sensible output for representative inputs.

### Step 6: Doxygen + license headers + CHANGELOG (~2h)

- Add a PostgreSQL-License header to `meos/src/geo/clip_clipper2.cpp` and `meos/include/geo/clip_clipper2.h` (the parts WE wrote — vendored Clipper2 keeps its BSL-1.0 header).
- Doxygen pass: `@brief`, `@param`, `@return`, `@note` on the new public C entry point.
- CHANGELOG entry under "Build changes": "MEOS now requires a C++17 compiler; `libstdc++` is linked into `libMobilityDB-1.4.so` and `libmeos.so`. Affects only build environments without a working C++ toolchain."

## Effort total

Per the prior assessment: **~50-55 focused dev-hours** + **~15h contingency** = **~70h** (~2 working weeks). Calendar estimate to merge: **2 calendar weeks** + **3-4 weeks** to a tagged release including review and CI shake-out.

## Critical gotchas (from spike experience)

- **Linker auto-promotes to C++ when ANY TU is C++**. You'll see `Linking CXX shared module ../libMobilityDB-1.4.so` in build output — that's correct, not a warning.
- **`#include <clipper2/clipper.h>` from C++ requires the include path to point at `meos/vendor/clipper2/include/`**. The spike sets this in the OBJECT lib's `target_include_directories`. The production adapter needs the same.
- **`extern "C"` blocks must NOT enclose `#include "clipper2/clipper.h"`** — that's a C++ header, would fail to parse as C. Wrap only the C-side includes (`<postgres.h>`, `<liblwgeom.h>`, `"geo/tgeo_spatialfuncs.h"`) in `extern "C"`.
- **`PG_DETOAST_DATUM_COPY` returns a Datum that needs casting to `GSERIALIZED *`**. The spike does this; copy the pattern in the V1 wrapper.
- **The V1 wrappers in `mobilitydb/src/geo/tgeo_spatialfuncs.c` (the `cl_intersection` etc. C functions)** stay in C — they call `clip_poly_poly` which now delegates internally to C++. No changes to that file.
- **Clipper2 `MAX_COORD = INT64_MAX >> 2` ≈ 2.3e18**. Coords at scale 1e7 from lon/lat go up to ~1.8e9 (after multiplication). Intermediate products (e.g. cross products in segment intersection) multiply two coords → max ~3.2e18 squared = wait, that's `(1.8e9)² = 3.2e18` which is right at the limit. **Verify with a stress test** that lon/lat-scale inputs near the equator don't overflow. If they do, drop the scale to 1e6 (~1.1m precision) which has 100× headroom.
- **Distros don't ship Clipper2** as `find_package`-able. Don't add an "optional system Clipper2" code path; vendor only.

## Branch + state inventory

On `origin` at end of spike session (2026-04-26):

| Branch | Tip | What's there |
|---|---|---|
| `clip-clipper2-spike` | (just pushed) | **Branch off this.** Vendored Clipper2 + spike adapter + working build pipeline. |
| `martinez-rebased` | `8dadec3ad` | Will be deleted after Clipper2 production lands. Has Round-1 fixes + MeosArray migration as historical reference. |
| `new_traj-rebased` | `003044056` | Independent. |
| `tgeo-fast-clip-rebased` | `f56a06776` | Independent. |
| `origin/martinez` | (legacy) | Original 2023 Martinez port. |

## Don't re-derive

Things the prior session settled — don't relitigate:
- **The migration is decided.** Clipper2 over Martinez. Don't propose Clipper1, GEOS-fallback, harden-Martinez, or anything else.
- **Vendor, don't `find_package`.** Distros don't ship it.
- **Boolean clipping subset only.** No offset, no minkowski, no triangulation. Re-vendor those if a future need arises; not now.
- **`enable_language(CXX)` at top-level.** Not "wrap in MEOS_CXX off-by-default". Just turn it on.
- **Public C signature stays.** `clip_poly_poly(subj, clip, oper) → GSERIALIZED *`. Wrapper file untouched.
- **Scale = 1e7.** Verified safe for the precision range we care about.
- **EvenOdd fill rule.** Matches LWPOLY topology semantics.
- **PolyTree64 output.** Need parent/hole topology, not flat Paths64.
