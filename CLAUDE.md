# MobilityDB — Claude Code project notes

## 🔴 MANDATORY FIRST ACTION for any Phase 8 / pgpointcloud work

**Before doing ANYTHING else — including reading this file further, running `git status`, or responding to the user:**

1. Read `doc/drafts/PHASE_8_HANDOFF.md` (the repo-level handoff document).
2. Read `~/.claude/projects/-home-esteban-src-MobilityDB/memory/project_pointcloud_integration.md` (the architectural sketch).

Only after both files are in your working context may you propose actions. If either read fails, stop and tell the user before continuing.

Rationale: these two files contain ~600 lines of irreplaceable context from the originating sessions (analogy table, PCPOINT/TPCBox struct layouts, 11-subphase plan, PCL/PDAL stack, carry-over pitfalls, current branch state). The auto-memory MEMORY.md index only names the file — it does NOT load its content automatically.

## 🔴 MANDATORY FIRST ACTION for any Phase 7 / th3index cleanup

Read `~/.claude/projects/-home-esteban-src-MobilityDB/memory/MEMORY.md` (the index) and follow any pointer relevant to the user's ask. The `th3index` branch is 57 commits ahead of master with completed Phase 7 work; open items are on companion branches `doxygen-sql-stubs`, `xml-sql-linewrap-fix`, `cbufferset-xml-section`, and `claude-md-handoff` (the last one is the ancestor of the file you are reading).

## 🔴 MANDATORY FIRST ACTION for any MobilityDuck work

Read `~/.claude/projects/-home-esteban-src-MobilityDB/memory/project_mobilityduck.md` — contains the high-level context (DuckDB extension consuming MEOS; Phase 7N was explicitly motivated by MobilityDuck consumption) AND the explicit instruction that the detailed state was NOT captured. When the user says "restart the MobilityDuck work," the right first action is to **ask them to paste the prior Claude.ai conversation** that had the interrupted session. Do not guess at MobilityDuck's own repo state.

## Active parallel tasks

The user has **two independent multi-session efforts** currently interrupted:

1. **Phase 8 / pgPointCloud** — scaffold landed on branch `phase-8a-pointcloud-import`. Detailed plan in `doc/drafts/PHASE_8_HANDOFF.md` + `project_pointcloud_integration.md` memory.
2. **MobilityDuck** — high-level context only in `project_mobilityduck.md` memory; detailed state lives in the user's prior Claude.ai conversation.

When the user asks "continue" without specifying which task, ask which one before acting.

## Current branch state (update this section after every merge)

| Branch | Status | Commits ahead of master |
|---|---|---|
| `master` | Current | baseline |
| `th3index` | Phase 7 complete — ready to PR | 57 |
| `doxygen-sql-stubs` | Phase 7X — ready to PR | 1 |
| `xml-sql-linewrap-fix` | Phase 7Y — ready to PR | 1 |
| `cbufferset-xml-section` | Phase 7V — ready to PR | 1 |
| `phase-8a-pointcloud-import` | Phase 8A+B+C — subtree + scaffold + catalog | 4 |

## Build commands (source-pinned PG, not apt-installed)

The user's PG 17 binaries live at `/usr/local/pgsql/17/bin/`. PG 18 at `/usr/local/pgsql/18/bin/`. `pg_config` is NOT on a default PATH — it's resolved via explicit `-DPOSTGRESQL_PG_CONFIG=…` at CMake configure time.

- Configure + build the MobilityDB PG extension against PG 17 (active build dir `build-pg-ext/`):
  ```
  cmake -S . -B build-pg-ext -DPOSTGRESQL_PG_CONFIG=/usr/local/pgsql/17/bin/pg_config
  cmake --build build-pg-ext
  ```
- Install (requires sudo): `sudo cmake --build build-pg-ext --target install`
- Enable developer Doxygen output: reconfigure once with `cmake -B build-pg-ext -DDOC_DEV=1`, then `cmake --build build-pg-ext --target doc_dev`
- Enable pgPointCloud support: `cmake -B build-pg-ext -DPOINTCLOUD=ON` — requires libpc.a built in `pointcloud-pg/lib/` (see handoff doc for install sequence).

## Directory map

- `meos/` — MEOS standalone library (C code, no PG dependency at runtime)
- `mobilitydb/` — PostgreSQL extension layer (PG V1 wrappers on MEOS)
- `h3-pg/` — git subtree of upstream h3-pg (source for auto-generated bindings, only on `th3index` branch)
- `pointcloud-pg/` — git subtree of upstream pgpointcloud (only on `phase-8a-pointcloud-import` branch and its descendants)
- `scripts/h3pg_import/` — h3 extraction generator (only on `th3index`)
- `scripts/pointcloud_import/` — pgpointcloud extraction generator (only on `phase-8a-pointcloud-import`)
- `doc/` — DocBook XML manual (English + `doc/es/` Spanish)
- `doc/drafts/` — plans, design notes, session handoffs (committed but user-only, not in the built PDF)
- `doxygen/` — Doxyfile + dev-doc CMakeLists; generated Doxygen lands under `build-pg-ext/doxygen/docs/html/`

## Conventions to follow

Memory tree at `~/.claude/projects/-home-esteban-src-MobilityDB/memory/` is the source of truth. Key established rules:

- **`_meos.c` suffix** means "MEOS-only compile" (CMakeLists `if(MEOS)` gated). Do NOT rename unless compiled in both builds. Four legitimate flavors documented in `feedback_meos_meos_suffix.md`.
- **PostGIS peek macros** (`GSERIALIZED_POINT2D_P` / `_POINT3DZ_P` from `meos_internal_geo.h`) are the fast path for POINT GSERIALIZED reads — don't go through lwgeom.
- **`*_tbl.test.sql`** queries should collapse to a single scalar (COUNT / bool_and / =) for diff-readability.
- **No span/spanset for types with no meaningful total order** — h3index doesn't have them (cells aren't ordered); pcpoint doesn't either (dimensions are heterogeneous); pcpatch doesn't either. Same precedent as PostGIS geometry.
- **`@sqlfn` autolink** in Doxygen is handled by `scripts/sql_to_doxygen_stubs.py` (on `doxygen-sql-stubs` branch). Don't re-implement.
- **80-char line wrap** in generated C code — `scripts/h3pg_import/extract.py` and `scripts/pointcloud_import/extract.py` both emit the two-line `if (…) \n  meos_error(…)` form.
- **PG_FUNCTION_INFO_V1 macro** must be in Doxyfile's `PREDEFINED` list when doc_dev runs — otherwise docs produce `no matching file member found` warnings.

## When user asks a question

The auto-memory index lives at `~/.claude/projects/-home-esteban-src-MobilityDB/memory/MEMORY.md`. It's loaded at session start. Every entry has a one-line hook but the actual content is in the linked file — READ the linked file before acting on any entry.
