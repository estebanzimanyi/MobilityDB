# RELAY → MEOS session: `nad_trgeometry_tpoint` validates the wrong operand

**From:** MobilityNebula streaming-parity session (this box). **Date:** 2026-06-05.
**Severity:** correctness — the function is broken for every valid input.
**Type:** one-line copy-paste bug, surfaced by integrating the op into Nebula.

## The bug

`meos/src/rgeo/trgeo_distance.c`, `nad_trgeometry_tpoint(temp1, temp2)` (line ~2344)
validates the wrong arguments:

```c
double
nad_trgeometry_tpoint(const Temporal *temp1, const Temporal *temp2)
{
  if (! ensure_valid_trgeo_tpoint(temp2, temp2))   /* <-- passes temp2 twice */
    return DBL_MAX;
  Temporal *dist = tdistance_trgeometry_tpoint(temp1, temp2);
  ...
```

`ensure_valid_trgeo_tpoint(t1, t2)` runs `VALIDATE_TRGEOMETRY(t1)` + `VALIDATE_TPOINT(t2)`.
Called as `(temp2, temp2)` it runs `VALIDATE_TRGEOMETRY(temp2)` on the tpoint operand,
which always fails → the call raises "The temporal value must be of type trgeometry"
and the function returns `DBL_MAX` for every input.

## Fix

```c
  if (! ensure_valid_trgeo_tpoint(temp1, temp2))
```

The three sibling functions are already correct and are the reference:
- `tdistance_trgeometry_tpoint` (line ~2023): `ensure_valid_trgeo_tpoint(temp1, temp2)`
- `nai_trgeometry_tpoint` (line ~2217): `ensure_valid_trgeo_tpoint(temp1, temp2)`
- `shortestline_trgeometry_tpoint` (line ~2418): `ensure_valid_trgeo_tpoint(temp1, temp2)`

## Acceptance

A smoke test: `nad_trgeometry_tpoint(trgeometry, tpoint)` returns a finite distance
(not `DBL_MAX`) for a trgeometry and a tpoint that share an SRID. Build both targets.

## Consumer status

MobilityNebula wires `nai_trgeometry_tpoint` and `shortestline_trgeometry_tpoint`
now (both correct); `nad_trgeometry_tpoint` waits on this fix, then folds into the
next wave. No re-pin needed beyond folding this one-liner into the next assembly-s5
tip.
