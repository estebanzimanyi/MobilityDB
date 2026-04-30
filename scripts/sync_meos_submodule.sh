#!/usr/bin/env bash
# sync_meos_submodule.sh — replay master's meos/ + postgis/ + postgres/ deltas
# into the MEOS submodule and bump the submodule pointer on this branch.
#
# Workflow:
#   1. Fetch upstream master.
#   2. Compute new master commits since the last sync (stored in
#      .meos-sync-tag, a regular commit-marker file).
#   3. For each new commit, format a patch scoped to the moved trees and
#      apply it inside the meos/ submodule.
#   4. Push the submodule, bump the parent's submodule SHA, push the parent.
#
# Designed for a weekly cadence. Per-PR triggers are too noisy — most
# in-flight PRs iterate after their first merge.
#
# Required environment:
#   - upstream remote pointing at MobilityDB/MobilityDB
#   - meos/ submodule's origin pointing at MobilityDB/MEOS, write access
#   - clean working tree on this branch
#
# Usage:
#   ./scripts/sync_meos_submodule.sh [--dry-run]

set -euo pipefail

DRY_RUN=0
[ "${1:-}" = "--dry-run" ] && DRY_RUN=1

ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT"

if [ -n "$(git status --porcelain)" ]; then
  echo "ERROR: working tree not clean. Commit or stash first." >&2
  exit 1
fi

if [ ! -f "$ROOT/.meos-sync-tag" ]; then
  echo "ERROR: .meos-sync-tag missing. Initialise it with the merge-base SHA:" >&2
  echo "  git merge-base HEAD upstream/master > .meos-sync-tag" >&2
  echo "  git add .meos-sync-tag && git commit -m 'Initialise MEOS sync tag'" >&2
  exit 1
fi

LAST_SYNC="$(cat "$ROOT/.meos-sync-tag")"
echo "Last sync at: $LAST_SYNC"

git fetch upstream master --quiet

NEW_HEAD="$(git rev-parse upstream/master)"
echo "Master tip:   $NEW_HEAD"

if [ "$LAST_SYNC" = "$NEW_HEAD" ]; then
  echo "Already in sync. Nothing to do."
  exit 0
fi

# Trees that move to the MEOS submodule. Keep in sync with #785's deletions.
MOVED_TREES=(meos postgis postgres)

# Collect the list of master commits in chronological order that touch any
# moved tree.
COMMITS=$(git log --reverse --format=%H "$LAST_SYNC..$NEW_HEAD" -- "${MOVED_TREES[@]}")

if [ -z "$COMMITS" ]; then
  echo "No moved-tree changes since $LAST_SYNC. Bumping tag only."
  if [ "$DRY_RUN" -eq 0 ]; then
    echo "$NEW_HEAD" > "$ROOT/.meos-sync-tag"
    git add .meos-sync-tag
    git commit -m "MEOS sync: bump tag (no submodule changes)"
  fi
  exit 0
fi

N=$(echo "$COMMITS" | wc -l)
echo "Replaying $N master commits into the MEOS submodule…"

PATCH_DIR="$(mktemp -d /tmp/meos-sync.XXXXXX)"
trap 'rm -rf "$PATCH_DIR"' EXIT

# format-patch for each commit, restricted to the moved trees. The MEOS
# submodule uses a FLAT layout: files that live at meos/X in MobilityDB
# live at X (root) in MEOS, while postgis/X and postgres/X are preserved.
# We post-process each patch to strip the leading meos/ prefix from the
# diff headers so `git am` applies them at MEOS root.
i=0
while IFS= read -r SHA; do
  i=$((i + 1))
  git format-patch -1 "$SHA" \
    --output-directory "$PATCH_DIR" \
    --start-number "$i" \
    -- "${MOVED_TREES[@]}" \
    >/dev/null
done <<<"$COMMITS"

# Strip leading meos/ from patch paths so they apply at the submodule's
# flat root. Anchored on the `diff --git`, `---`, and `+++` header lines
# so we don't accidentally rewrite content inside the diff body.
for f in "$PATCH_DIR"/*.patch; do
  sed -i \
    -e 's|^\(diff --git \)a/meos/\(.*\) b/meos/|\1a/\2 b/|' \
    -e 's|^--- a/meos/|--- a/|' \
    -e 's|^+++ b/meos/|+++ b/|' \
    "$f"
done

PATCH_COUNT=$(find "$PATCH_DIR" -maxdepth 1 -name '*.patch' | wc -l)
if [ "$PATCH_COUNT" -eq 0 ]; then
  echo "format-patch produced 0 files (all commits filtered out)."
  if [ "$DRY_RUN" -eq 0 ]; then
    echo "$NEW_HEAD" > "$ROOT/.meos-sync-tag"
    git add .meos-sync-tag
    git commit -m "MEOS sync: bump tag (filtered to zero)"
  fi
  exit 0
fi

if [ "$DRY_RUN" -eq 1 ]; then
  echo "DRY RUN — patches generated at $PATCH_DIR but not applied."
  ls "$PATCH_DIR"/*.patch
  exit 0
fi

# Apply inside the submodule.
cd "$ROOT/meos"
git fetch origin master --quiet
git checkout master
git pull --ff-only

if ! git am --keep-cr -3 "$PATCH_DIR"/*.patch; then
  echo "ERROR: git am failed inside meos/. Resolve manually:" >&2
  echo "  cd $ROOT/meos" >&2
  echo "  # fix the failing patch, then:" >&2
  echo "  git am --continue   # or --abort to roll back" >&2
  echo "Patches at: $PATCH_DIR" >&2
  exit 1
fi

git push origin master

# Bump submodule pointer on the parent branch.
cd "$ROOT"
git add meos
git add .meos-sync-tag <<<"$NEW_HEAD" 2>/dev/null || true
echo "$NEW_HEAD" > "$ROOT/.meos-sync-tag"
git add .meos-sync-tag

git commit -m "MEOS sync: replay $N master commits ($LAST_SYNC..$NEW_HEAD)"

echo
echo "Sync complete. Push when ready:"
echo "  git push origin $(git branch --show-current)"
