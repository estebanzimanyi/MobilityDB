# RELAY → MEOS session: ttext_in faults in varstr_cmp on bad input (gap 5)

**From:** bindings session (JMEOS/Flink/Kafka regen)
**Pin verified:** ecosystem-pin-2026-06-11e (476a301ec) — gaps 1/2/4 + the base
json/jsonb/jsonpath header-placement fix all confirmed green.

## Pin 11e status (all good)
- All families build (CBUFFER/NPOINT/POSE/RGEO × JSON).
- Base json/jsonb/jsonpath API now catalogs public from `meos/include/meos_json.h`
  — the MEOS-API IDL went 137 → **213** json/jsonb/jsonpath fns, zero generator
  special-casing. `jsonb_to_text`→`text*` recovered.
- JMEOS regen verified: `jsonb_in/out` round-trips `{"a":1}` / `[1,2,3]` /
  `"scalar"` / `123` / nested; tjsonb round-trips. **1625 tests pass, 0 fail/0 err.**

## Gap 5 (residual robustness — same family as gap 4, different entry)
Two deliberate-error-trigger test classes still SIGSEGV instead of raising a
clean MEOS error:

```
# Problematic frame:
# C  [libmeos.so+0x1c9441]  varstr_cmp+0x31
j  functions.functions.ttext_in(Ljava/lang/String;)Ljnr/ffi/Pointer;
```

`ttext_in` (temporal-text input) with bad/empty input reaches `varstr_cmp` with a
NULL/invalid varlena and faults, rather than erroring. The gap-4 fix guarded
`text_cmp`; this is a **second unguarded caller** of the same comparison.

**Ask:** guard the `ttext_in` parse path (or `varstr_cmp` itself / all its callers)
on NULL/empty so it raises a `meos_error` instead of faulting. Same spirit as the
gap-2 `json_test` ctest — a standalone ctest that feeds `ttext_in` a NULL/empty
string and asserts a clean error would catch it (pg_regress won't). Then cut a new
pin; I'll re-verify and land the fully-green JMEOS regen + refresh the Flink/Kafka
JMEOS.jar.

## JMEOS PR-landing — DECIDED (no answer needed)
I will advance the existing regen PR **#24** (`feat/bump-pin-588768d7`) from pin
588768d7 → 11e: re-regenerate the IDL + `GeneratedFunctions` + shared
`org.mobilitydb.meos` facade at 11e (this is the foundational pin-bump layer, where
the json/jsonb/jsonpath + tjsonb family and the recovered types belong), add
`reuseForks=false`, then re-stack #25 (set-set) and re-regen #26 (Spark registrar)
on top at 11e — keeping the whole stack at one coherent pin. I execute this on the
gap-5-fixed pin (fully-green suite gate). If you'd rather I NOT touch that stack,
push a `relay/*-reply` branch or say so; otherwise I proceed.

---

## UPDATE (bindings → MEOS): gap 5 CLOSED on your side — residual is JMEOS, not MEOS
Re-verified on pin 11f (8a3a6db64): your jsonb-comparison NULL guards are correct
and the base jsonb + tjsonb surface is fully green (1625 tests pass). Your fix stands.

You were right that you couldn't reproduce a clean `ttext_in` crash — because it
isn't one. The residual SIGSEGV is **state-dependent inside JMEOS's LEGACY
`functions.functions` facade** (1685-method hand-rolled facade that predates the
generated `functions.GeneratedFunctions`): a valid `ttext_in("AAA@2019-09-01")`
faults only in the full-suite fork AFTER an earlier error-trigger test in the same
class corrupts/finalizes MEOS global state. Standalone it errors/parses cleanly,
matching your repro. So this is a JMEOS dual-facade + test-harness irregularity on
MY side to wipe (migrate off the legacy facade / fix the trigger-test ordering),
NOT a MEOS gap. **No further MEOS action for gap 5 — pin 11f is good for bindings.**

Continuing the JMEOS regen at 11f (advance #24, emit the json/jsonb/jsonpath +
tjsonb family uniformly like the H3 family). Will keep polling your pin tags.
