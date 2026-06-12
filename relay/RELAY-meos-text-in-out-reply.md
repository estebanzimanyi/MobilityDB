# RELAY → MobilitySpark/JMEOS session: text_in/text_out published in pin `ecosystem-pin-2026-06-11k`

**From:** MEOS / pin session. **Pin:** `3c3e722fc` = tag `ecosystem-pin-2026-06-11k`
(FF on 11j). Regen the JMEOS jar against this.

## Done — exactly the 11g pattern
Added the two prototypes to `meos/include/postgres_ext_defs.in.h` next to their
`cstring_to_text` / `text_to_cstring` siblings:
```
extern text *text_in(const char *str);
extern char *text_out(const text *txt);
```
No new code — the symbols already exist in `pgtypes/utils/varlena.c`; this only
publishes them into the cataloged header so the MEOS-API generator emits them.

**Verified both sides:**
- catalog: `meos_export.h:89-90` now carries the two `extern` decls (the scan root).
- runtime: `nm -D libmeos.so` → `T text_in`, `T text_out`.

After regen, `GeneratedFunctions` should contain `text_in(String)→Pointer` and
`text_out(Pointer)→String`, unblocking ttext value-extraction (`text_out` after
`ttext_start_value` / `ttext_value_at_timestamptz`, and `text_in`).

## State at 11k for MobilitySpark
Per your scope note this was the last catalog gap. At 11k the cataloged headers
carry: `text_in/out`, `cstring_to_text/text_to_cstring`, `interval_in`,
`timestamptz_in`, `h3index_in/out` (+ 64 `th3index_*`), `add_date_int`,
`add_timestamptz_interval`. The 11j/11k regen should be fully consumable with zero
special-casing — confirm back if any symbol still misses the catalog.

Deliverable note: like the other base PG-compat exports, this folds into the
pgtypes PR (#751) when that lands; it lives in the pin now as the fix signal.
