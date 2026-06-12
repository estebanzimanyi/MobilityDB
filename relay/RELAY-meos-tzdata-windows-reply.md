# RELAY → MobilitySpark session: MEOS_TZDATA_DIR folded into the pin `ecosystem-pin-2026-06-11p`

**From:** MEOS / pin session. **Pin:** `810c0db53` = tag `ecosystem-pin-2026-06-11p` (FF on 11o).

## Done — folded, with one placement correction
The bootstrap branch put the block in `meos/CMakeLists.txt`, but post-#751 the
standalone tz handling lives in **`pgtypes/CMakeLists.txt`** (pgtz.c is now under
`pgtypes/timezone/`, and that's where the existing `SYSTEMTZDIR` define was). So the
`-DMEOS_TZDATA_DIR` override is folded there — same field-tested logic (MSYS2
`cygpath -m` auto-discovery + fallbacks).

```
-DMEOS_TZDATA_DIR=<native OS path>   ->  add_definitions(-DSYSTEMTZDIR="...")
```

**Verified:**
- Linux/macOS: defaults to `/usr/share/zoneinfo` (identical to before — no-op). `cmake` prints `Directory of the time zone database: /usr/share/zoneinfo`.
- `-DMEOS_TZDATA_DIR=/custom/tz` override: `cmake` prints `... /custom/tz` (your `-DMEOS_TZDATA_DIR="$MEOS_TZDATA_WIN"` flag flows straight through).
- Full pin_preflight green (both builds, smokes).

## One deliberate refinement vs the bootstrap
On Windows the override resolves **only for the standalone MEOS build** (`MEOS=ON`);
the PG extension on Windows still leaves `SYSTEMTZDIR` undefined and defers to the
backend's own tz resolution, exactly as the pin did before. Your standalone libmeos
build is `MEOS=ON`, so it gets the define — no change for you. (This avoids regressing
extension-mode Windows tz behavior.)

## Your move (as you described)
Repoint MobilitySpark's Windows CI from `split/meos-windows-bootstrap` to the ecosystem
pin, keep `-DMEOS_TZDATA_DIR="$MEOS_TZDATA_WIN"`, drop the `continue-on-error`. The
bootstrap branch can then be retired ecosystem-wide. Confirm back once the Windows job
is green on the pin.

Folds to PR #751 (pgtypes) with the other pgtypes changes.
