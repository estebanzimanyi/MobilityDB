# Phase 8 — pgpointcloud integration — Session handoff

> **Purpose of this document.** A new Claude Code session reading this file
> should have ZERO ramp-up cost: all context from previous sessions is
> distilled here. Complementary to the architectural sketch at
> `~/.claude/projects/-home-esteban-src-MobilityDB/memory/project_pointcloud_integration.md`
> which has the deeper design material (struct layouts, SQL surface,
> PCL/PDAL stack). Both files should be loaded before any code action.

> **Parallel active task: MobilityDuck.** The user also has an
> interrupted MobilityDuck (DuckDB extension consuming MEOS) effort.
> See `~/.claude/projects/-home-esteban-src-MobilityDB/memory/project_mobilityduck.md`
> — that memory file intentionally does NOT capture MobilityDuck's own
> repo state; it tells future Claude to ask the user to paste the
> prior Claude.ai conversation when resuming. Phase 7N (th3index
> MEOS-layer split) was explicitly motivated by MobilityDuck
> consumption, so any pgpointcloud-phase work that adds MEOS-layer
> functions should follow the same "business logic in MEOS, Datum
> plumbing in MobilityDB" split to keep MobilityDuck consumption
> viable without a second rewrite.

## Current state as of handoff

### What's done

| Commit | Phase | Summary |
|---|---|---|
| `2a867853e` | 8A | Squashed pgpointcloud v1.2.5 content into `pointcloud-pg/` |
| `17609dc34` | 8A | Merged subtree into `pointcloud-pg/` at repo root |
| `f9847b682` | 8A | Scaffold: `scripts/pointcloud_import/` (extractor + ruleset + opt-out), `meos/src/pointcloud/CMakeLists.txt`, `mobilitydb/src/pointcloud/CMakeLists.txt`, `-DPOINTCLOUD=ON` option at top-level, synthesised `pc_config.h` via `file(WRITE)` so upstream headers compile |
| `83d9b790b` | 8B+C | `meosType` entries T_PCPOINT=63, T_PCPOINTSET=64, T_TPCPOINT=65, T_PCPATCH=66, T_PCPATCHSET=67, T_TPCPATCH=68. Catalog name table + SETTYPE and TEMPTYPE linkages. `libpc.a` static-link wiring with a fatal-error guard if the library isn't built. |

All four commits live on branch **`phase-8a-pointcloud-import`** — not on master yet. Build verified in both `-DPOINTCLOUD=OFF` (default, zero impact) and `-DPOINTCLOUD=ON` (pointcloud module links libpc.a cleanly).

### User's install state

* PG 17 at `/usr/local/pgsql/17/` — source-pinned, NOT apt-installed.
* pgpointcloud v1.2.5 installed against PG 17 via in-tree `./configure && make && sudo make install`.
* `libpc.a` exists at `/home/esteban/src/MobilityDB/pointcloud-pg/lib/libpc.a` (needed for Phase 8B+ link).
* Extension `CREATE EXTENSION pointcloud` + `CREATE EXTENSION pointcloud_postgis` enabled in database `test`.
* Smoke test `PC_Get(PC_MakePoint(1, ARRAY[10.0,20.0,30.0]), 'X')` returns `10` — the full stack works.
* PG 18 not yet done. Same install procedure applies, targeting `/usr/local/pgsql/18/bin/pg_config`.

### Architectural pivot recorded

Early in the session the plan was "MobilityDB defines its own pcpoint SQL type". After reading upstream pgpointcloud, the correct pattern is **MobilityDB reuses pgpointcloud's pcpoint/pcpatch types the way it reuses PostGIS geometry** — it does not redefine them.

Consequence: Phase 8B (pcpoint base type) and 8C (pcpatch base type) collapse to catalog registration only. The real work starts at Phase 8D (set types), 8E (TPCBox), 8F (tpcpoint lifted).

## What's pending — sub-phases

See `~/.claude/projects/-home-esteban-src-MobilityDB/memory/project_pointcloud_integration.md` for the full 11-phase plan. Short version:

| Phase | Scope | Estimated effort |
|---|---|---|
| 8D | pcpointset, pcpatchset SQL types + MEOS implementations | ~400 LOC, 2–3 hours |
| 8E | TPCBox bbox type + operators | ~500 LOC, 3–4 hours |
| 8F | tpcpoint lifted temporal type | ~700 LOC, 5–6 hours |
| 8G | tpcpatch lifted temporal type | ~600 LOC, 4–5 hours |
| 8H | SQL registration + GiST/SPGiST operator classes | ~500 LOC, 3–4 hours |
| 8I | Docs chapter + test suite | 6–8 hours |
| 8J | Compliance pass (mirrors th3index Phase 6) | 3–4 hours |
| 8K | PDAL custom reader/writer for tpcpoint+tpcpatch | 8+ hours (optional) |

Totals to ~25–35 hours of careful work. Not a single session; a multi-session effort like Phases 7A–7P of th3index were.

## How to start the next session

### The user runs

```bash
cd /home/esteban/src/MobilityDB
git switch phase-8a-pointcloud-import   # start where Phase 8A left off
claude
```

### The user pastes, verbatim, as the first message

```
Continue Phase 8 — pgPointCloud integration.

STEP 1 (do before anything else): read the two handoff files —
  /home/esteban/src/MobilityDB/doc/drafts/PHASE_8_HANDOFF.md
  ~/.claude/projects/-home-esteban-src-MobilityDB/memory/project_pointcloud_integration.md

STEP 2 (verification): confirm you loaded them by telling me three
things only someone who read them would know:
  (a) which pgpointcloud version is pinned in the subtree
  (b) the name of the new bbox type this phase introduces
  (c) which two sub-phases were collapsed during the session's architectural pivot

STEP 3: only after I confirm your answers, propose the next sub-phase.

If STEP 1 fails for any reason, stop and tell me immediately — do NOT
guess at content from the MEMORY.md index alone. The index entries
are pointers, not content.
```

### Verification cheat sheet (for the user, not Claude)

When Claude responds to the verification step, the right answers are:

* (a) **v1.2.5** (pinned in `pointcloud-pg/POINTCLOUD_REVISION`)
* (b) **TPCBox** — introduced in Phase 8E
* (c) **Phase 8B (pcpoint base type) and Phase 8C (pcpatch base type)** collapsed into a single catalog-registration commit, because pgpointcloud already provides both as SQL types.

If Claude gets any of these wrong, it did NOT actually read the handoff files — ask it to re-read them before proceeding.

## Safety procedure before starting the real work

```bash
# Bookmark to reset to if a long agent run goes sideways
git branch phase-8-safety-point

# Make autonomous work tolerable. This is a dangerous setting —
# the agent can run any shell command without asking. The safety
# bookmark above lets you reset clean if anything goes wrong.
mkdir -p .claude
echo '{"permissions":{"defaultMode":"bypassPermissions"}}' > .claude/settings.local.json
```

If a session goes wrong: `git reset --hard phase-8-safety-point && git checkout -- .` gets you back to the known-clean state.

## Known gotchas the next session should remember

1. **`_meos.c` suffix** means "MEOS-only compile" (CMakeLists `if(MEOS)` gated). Do not rename unless the file is compiled for both builds. Four flavors documented in `feedback_meos_meos_suffix.md`.
2. **PostGIS peek macros** (`GSERIALIZED_POINT2D_P` / `_POINT3DZ_P` from `meos_internal_geo.h`) are the fast path for POINT GSERIALIZED. pgpointcloud patches are more complex — peek may not apply; read serialized bytes through libpc helpers instead.
3. **`@ingroup` typo risk** — defgroup registry in `doxygen_*.h` must match source-side `@ingroup` tags exactly. Run a reconciliation sweep after authoring.
4. **PG reserved-word collisions** on cast signatures — `CREATE CAST ... WITHOUT FUNCTION` is the workaround (see th3index's handling of `bigint`).
5. **Schema (pcid) consistency** — unique to pgpointcloud. tpcpoint sequences MUST validate that every instant's pcpoint shares the same pcid (the schema id in pgpointcloud's `pointcloud_formats` table). Implement `ensure_same_pcid` in the static layer and call it from every constructor / append / merge path. This constraint doesn't exist for tgeompoint.
6. **pgpointcloud ships no `libpointcloud.so`** — only a static `libpc.a` produced as a side-effect of the upstream PG-extension build. MobilityDB static-links against that. `-fvisibility=hidden` in both builds prevents symbol collision at `LOAD` time.
7. **No span/spanset for pcpoint/pcpatch** — dimensions are heterogeneous (intensity, return number, classification), so no meaningful total order. Same precedent as h3index. Drop these from the plan whenever they come up.
8. **The extractor's `PG_FN_RE` regex** was widened in Phase 8A to accept pgpointcloud's single-line `Datum fn(PG_FUNCTION_ARGS)` form (h3-pg uses `Datum\n fn(...)`). Don't tighten it back.
9. **Opt-out parking lot in Phase 8A** — `scripts/pointcloud_import/opt-out.yaml` has a `file_glob: pointcloud-pg/pgsql/*.c` that skips all ~50 PG wrappers. As Phase 8B+ adds rules, that glob narrows to per-function skips. Don't touch the glob until the ruleset covers the macros needed (PG_GETARG_PCPOINT_P, PG_GETARG_PCPATCH_P, PG_GETHEADER_SERPATCH_P, PG_GETARG_ARRAYTYPE_P, …).

## How to update this document

Add a one-line summary under "What's done" after every phase commit. Update the "Branch state" table after every merge. Keep this file in the repo so any future session sees it regardless of auto-memory state.
