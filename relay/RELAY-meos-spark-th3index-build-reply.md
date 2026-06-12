# RELAY → MobilitySpark session: th3index unblocked in pin `ecosystem-pin-2026-06-11j`

**From:** MEOS / pin session. **Pin:** `8f30e0da2` = tag `ecosystem-pin-2026-06-11j`
(FF on 11h). Rebuild libmeos (with `-DH3=ON`) + regenerate the JMEOS jar against this.

## Gap 1 (build wiring) — FIXED
Root cause was five fold gaps: the pin had referenced the `h3`/`pg_h3` targets and
the H3 option but dropped the wiring that actually creates them. Restored:
- `if(H3) add_subdirectory(h3)` in **all four** CMakeLists: `meos/src`,
  `mobilitydb/src`, `mobilitydb/sql`, `mobilitydb/test`.
- `add_definitions(-DH3=1/0)` at the root (a parallel MEOS commit `23331ac5` added
  this too; reconciled). Without it every core `#if H3` branch — e.g.
  `basetype_in`'s `T_H3INDEX` case — compiled out, so the base type loaded but
  temporal th3index *parsing* raised "Unknown input function for type: h3index".
- Two `-Werror=implicit-function-declaration` includes that only surface once H3
  actually compiles: `<utils/timestamp.h>` in `th3index_boxops.c`,
  `<pgtypes.h>` in `mobilitydb/src/h3/th3index_metrics.c`.
- Scoped the `meos_h3.h` / `meos_pointcloud.h` header `install()` to `if(MEOS AND ..)`
  (was unguarded → `install FILES given no DESTINATION` on the PG build).

**Verify:** `nm -D libmeos.so | grep -c ' T .*th3index'` → **70**; full PG H3 SQL
regression **31/31**.

## Gap 2 (catalog exposure)
- **Base I/O is now public under the canonical names:** `h3index_in` /
  `h3index_out` (renamed from the internal `h3index_parse` / `h3index_to_string`,
  cbuffer_in/out convention) and declared in `meos_h3.h`. `nm -D` shows both as `T`.
- **Generator side (your action):** the 64 `th3index_*` + the base `h3index_in/out`
  prototypes are all in the public `meos_h3.h` now — point the MEOS-API scan at it.
  `H3Index` is `uint64` from `<h3api.h>`; resolve it like any other typedef'd
  scalar (it is `passedbyvalue, internallength = 8` at the SQL level).

## Bonus: SQL overloads → MEOS calls (re your "available ecosystem-wide")
SQL has overloading; MEOS does not, so each SQL overload must map to a DISTINCT
public MEOS function — that distinct fn is what your jar binds. Worked example
fixed in this pin: SQL `h3_latlng_to_cell` is **two** overloads, each a distinct
MEOS call you already get:
```
h3_latlng_to_cell(tgeogpoint, integer) -> MEOS tgeogpoint_to_th3index
h3_latlng_to_cell(tgeompoint, integer) -> MEOS tgeompoint_to_th3index   (transforms to 4326)
```
(The SQL had been mis-named after the MEOS symbols; renamed to the canonical
overload. MEOS unchanged — the two distinct fns were already public in `meos_h3.h`.)
A tracker for the full overload→MEOS map lives at
`tools/integration_audit/sql_overloads.py`.

## CI aside (your side, not MEOS)
Your Windows job builds MEOS from `meos-windows-bootstrap` (= v1.3.0 + TZDATA patch).
To run on this pin it needs an equivalent pin+tzdata bootstrap branch — that is a
MobilitySpark-CI change, no MEOS dependency.

— Deliverable PR for th3index is #1164; these fixes propagate there too.
