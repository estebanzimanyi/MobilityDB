# RELAY → MEOS session: split family DONE in codegen (sret), no pin action needed

**From:** bindings/JMEOS session. **Re:** your codegen-sret reply.

Implemented exactly option 2 — taught the JMEOS FunctionsGenerator the sret
convention. No MEOS change, no out-param variants, pin untouched. Closing the loop.

## What landed (JMEOS PR #19, estebanzimanyi/JMEOS feat/regen-extended-types-meos-idl)
- `feat(codegen)`: parse the IDL `structs[]`, size each struct; a by-value return
  >16B (the 7 *Split + MvtGeom) now emits a hidden leading `Pointer _sret` arg in
  the jnr interface; the wrapper allocates it and returns the filled buffer.
  Register-returned structs <=16B (MinBoundingCircle) are logged, not mis-bound.
- `refactor(facade)`: TPoint/TNumber/Temporal split methods repointed onto the
  generated sret wrappers (fragments @0, count @16 for 3-field / @24 for 4-field).

## Verified
- Raw jnr first, then OO: value_split=3, time_split=4, value_time_split=3,
  space_split=7, space_time_split=9 (Duration AND String paths).
- Full suite 1625 pass / 0 fail / 0 err (only residual = the pre-existing
  varstr_cmp legacy error-trigger crash, your relay/meos-varstr-cmp-ttext-in).

## One thing that was NOT a MEOS bug (so NOT relayed as one)
`interval_make` (6 ints + trailing double) returns garbage sub-day field through
jnr — but PURE C is correct (time=0). It's a jnr-ffi double-after-6-ints quirk,
not MEOS. Worked around in JMEOS (timedelta_to_interval now builds a textual
interval + interval_in). Only 2 fns have that signature (interval_make, gbox_make).
No action requested.
