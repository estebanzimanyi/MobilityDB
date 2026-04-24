# MobilityDB — Claude Code project notes

## Starting a new work session

- **Phase 8 / pgPointCloud integration** — before touching any code, read the memory file `~/.claude/projects/-home-esteban-src-MobilityDB/memory/project_pointcloud_integration.md`. It contains the complete 9-sub-phase plan (8A–8I), the PCPOINT / TPCBox struct layouts, the SQL surface sketch, the CMake opt-in, the PCL/PDAL four-layer stack, and every known pitfall carried over from the th3index (Phase 7) work. The task is self-contained once that file is loaded.
- **th3index (Phase 7) cleanup** — the `th3index` branch is 57 commits ahead of master and ready to PR. The h3 implementation is complete; remaining open items only concern cross-type cleanup (see `doxygen-sql-stubs` and `xml-sql-linewrap-fix` branches).

## Build commands

- Configure + build the MobilityDB PG extension: `cmake -S . -B build && cmake --build build`
- Install (requires sudo): `sudo cmake --build build --target install`
- Enable developer Doxygen output: reconfigure once with `cmake -B build -DDOC_DEV=1`, then `cmake --build build --target doc_dev`
- Enable pgPointCloud support when it lands: `cmake -B build -DPOINTCLOUD=ON`

## Directory map

- `meos/` — MEOS standalone library (C code, no PG dependency at runtime)
- `mobilitydb/` — PostgreSQL extension layer (PG V1 wrappers on MEOS)
- `h3-pg/` — git subtree of upstream h3-pg (source for auto-generated bindings)
- `scripts/h3pg_import/` — the h3 extraction generator + ruleset + opt-out lists. Template for the upcoming `scripts/pointcloud_import/`.
- `doc/` — DocBook XML manual (English + `doc/es/` Spanish)
- `doxygen/` — Doxyfile + dev-doc CMakeLists; generated Doxygen lands under `build/doxygen/docs/html/`

## Conventions to follow

See `~/.claude/projects/-home-esteban-src-MobilityDB/memory/` for the full set:
- `_meos.c` suffix means "MEOS-only compile" — don't rename unless compiled in both builds
- PostGIS peek macros (`GSERIALIZED_POINT2D_P` etc.) are the fast path for POINT GSERIALIZED
- `*_tbl.test.sql` queries should collapse to a single scalar (COUNT / bool_and / =) for diff-readability
- h3index has no span/spanset by design (cells have no meaningful total order — same precedent as geometry and text). pcpoint likewise on the same grounds.
