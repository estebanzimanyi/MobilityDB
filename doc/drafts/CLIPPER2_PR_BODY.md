# PR body — Clipper2 polygon Boolean engine migration

> **Title** (≤70 chars): `Migrate polygon Boolean engine from Martinez-Rueda to Clipper2`
>
> **Branch:** `clip-clipper2-prod` → `master`
>
> **One-liner to open the PR (after `gh auth login`):**
>
> ```bash
> gh pr create --base master --head clip-clipper2-prod \
>   --title "Migrate polygon Boolean engine from Martinez-Rueda to Clipper2" \
>   --body-file doc/drafts/CLIPPER2_PR_BODY.md
> ```
>
> Edit the body below before firing the command if anything's stale.

---

## Summary

Replaces the bespoke Martinez-Rueda polygon-clipping port (~2660 LOC, including
the priority queue and splay-tree support code) with vendored Clipper2 v2.0.1
(Boost 1.0 / PostgreSQL-License-compatible). The migration:

- Fixes a confirmed correctness bug in the Martinez port on `hole + clip
  extending outside the subject polygon` inputs (the previously-disabled
  reproducer at `mobilitydb/test/geo/queries/078_tpoint_clipping.test.sql:65-66`
  is **re-enabled** in this PR and now produces the topologically-correct
  octagon-with-square-hole that PostGIS / GEOS also produce).
- Lights up `atGeometry(tgeometry, geometry)`, `atStbox(tgeometry, stbox)`, and
  `minusGeometry(tgeometry, geometry)` to use the Clipper2 path transparently
  for 2D polygonal inputs (other types fall through to GEOS unchanged).
- Net diff: **−2605 / +384 LOC** of MobilityDB code, plus the vendored
  Clipper2 sources under `meos/vendor/clipper2/` (5 headers + 2 `.cpp`,
  Boolean-clipping subset only).

## What changed

- New file `meos/src/geo/clip_clipper2.cpp` — production C++ adapter exposing
  `clipper2_clip_poly_poly(subj, clip, op) → GSERIALIZED *` to the C side.
  Handles POLYGON + MULTIPOLYGON with holes, all four ops (intersection /
  union / difference / xor), and recursive `PolyTree64` walking for
  islands-in-holes.
- `meos/src/geo/geo_poly_clip.c` shrunk from 1395 LOC to ~110: just empty-input
  and bbox-disjoint short-circuits, then delegation to `clipper2_clip_poly_poly`.
  `_Static_assert`s pin the SQL ABI of `ClipOper` to the values consumed by the
  adapter so future drift fails to compile rather than silently corrupting.
- `meos/src/geo/postgis_funcs.c` — `geom_intersection2d` and `geom_difference2d`
  fast-path through `clip_poly_poly` when both inputs are 2D polygonal. This
  is the single change that lights up `atGeometry`/`atStbox`/`minusGeometry`
  on tgeometry. Geography, 3D, and non-polygonal inputs unchanged.
- Deleted `meos/src/geo/{pqueue,splay_tree}.{c,h}` (~1248 LOC, Martinez-only
  support structures with no other callers). The legacy implementation
  remains accessible on `origin/martinez-rebased` for historical reference.
- Vendored `meos/vendor/clipper2/` (Clipper2 v2.0.1, BSL-1.0). Only the
  Boolean-clipping subset (`clipper.{h,core.h,engine.h,rectclip.h}` +
  `clipper.{engine,rectclip}.cpp`); offset, Minkowski, and triangulation are
  not vendored.

## Build impact

- MEOS now requires a C++17 compiler. `enable_language(CXX)` is set at the
  top-level `CMakeLists.txt`, and `CMAKE_CXX_STANDARD 17` / `_REQUIRED ON` /
  `_EXTENSIONS OFF` apply project-wide. The README's `Requirements` section
  is updated to list `A C++17 compiler (e.g. GCC >= 7, Clang >= 5,
  MSVC >= 19.14)` so downstream packagers don't get caught out.
- The final shared module is now linked as `Linking CXX shared module
  libMobilityDB-1.4.so` — `libstdc++` is auto-pulled in. Trivial on glibc
  systems; flagged here because it changes the library closure for downstream
  packagers.

## Test plan

- [x] `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build`
      builds clean on Linux gcc.
- [x] `sudo cmake --install build --prefix /usr/local/pgsql/17` installs the
      `.so` + extension SQL + control files.
- [x] `ctest --output-on-failure -j$(nproc)` in `build/`: **137/137 passed
      (400 sec)**, including:
    - `078_tpoint_clipping` (regenerated expected output; previously-disabled
      reproducer re-enabled).
    - All `tgeo_*` and `tpoint_*` spatial / restrict / box suites that touch
      the polygon Boolean path indirectly.
- [x] Honest end-to-end validation against a private test cluster after
      installing this PR's `.so` over `$libdir/libMobilityDB-1.4.so`:
    - `atGeometry(tgeometry, polygon)` returns the 4-vertex Clipper2-style
      output on the collinear-hole-edge case (vs PostGIS's 6-vertex form),
      proving the call routes through Clipper2 rather than GEOS.
    - Area conservation `subj = inter + diff` holds.
    - `atStbox(tgeometry, stbox)`, `minusGeometry(tgeometry, polygon)` —
      both correct.
- [x] Cross-platform CI green on this branch: Linux gcc + clang, macOS 14
      and 15 × PG 17 + 18, Windows MSYS2 (UCRT64 / GCC), and the
      `WITH_COVERAGE=1` matrix variant. Two portability fixes were needed
      and are included on this branch:
    - `clip_clipper2.cpp` includes are reordered so the C++ stdlib (via
      `<clipper2/clipper.h>`) is parsed before `<postgres.h>`, avoiding
      MSYS2's `win32_port.h:#define bind pgwin32_bind` mangling
      `std::bind`.
    - `WITH_COVERAGE` now passes `--coverage` on the module / shared / exe
      linker flag families and mirrors `-fprofile-arcs -ftest-coverage`
      into `CMAKE_CXX_FLAGS`, since the `.so` is now linked with the g++
      driver and that does not auto-pull libgcov from compile flags.
- [x] Coveralls measurement is meaningful: vendored Clipper2 sources are
      excluded from the lcov capture (mirroring the existing PostGIS
      exclusion), so the reported figure reflects MobilityDB's own
      coverage rather than diluted by ~5000 LOC of upstream-tested
      library code.

## Out of scope (intentionally)

- The trajectory-vs-polygon clip on `origin/tgeo-fast-clip-rebased` is **not**
  merged here. An audit (committed as `doc/drafts/FAST_CLIP_ANALYSIS.md` in
  this branch) found two bugs in that code, one structural. Recommended
  follow-up is to rewrite it on top of Clipper2's open-path API; estimated
  ~6h of work, tracked separately. Until then, `atGeometry(tgeompoint, ...)`
  continues to use the existing PostGIS / GEOS path.

## Companion docs

- `doc/drafts/CLIPPER2_HANDOFF.md` — the full operational handoff document
  written during the spike, retained for historical context.
- `doc/drafts/FAST_CLIP_ANALYSIS.md` — the trajectory-clip audit
  (Bugs A + B), reproduction recipe, and Clipper2 open-path rewrite sketch.

🤖 Generated with [Claude Code](https://claude.com/claude-code)
