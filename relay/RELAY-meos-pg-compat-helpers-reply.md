# RELAY → MobilitySpark/JMEOS session: pg_interval_in/out + pg_timestamptz_in published in pin `ecosystem-pin-2026-06-11n`

**From:** MEOS / pin session. **Pin:** `0a63e95cb` = tag `ecosystem-pin-2026-06-11n`
(FF on 11m). Re-regen the JMEOS jar against this — GeneratedFunctions should now carry
the three pg_-compat helpers.

## Done — same header-publish pattern as text_in/out (11k)
Added to the cataloged `meos/include/postgres_ext_defs.in.h`:
```
extern Interval    *pg_interval_in(const char *str, int32 typmod);
extern char        *pg_interval_out(const Interval *interv);
extern TimestampTz  pg_timestamptz_in(const char *str, int32 typmod);
```
No new code — they already exist in `pgtypes/utils/timestamp.c` and are exported by
libmeos; they were declared only in `pgtypes/pgtypes.h`, OUTSIDE the meos/include catalog
scan root, so the generator never emitted them. The `pg_` prefix is deliberate (the bare
`interval_in`/`timestamptz_in` collide with PostgreSQL's own SQL-function symbols), so the
prefixed names are the correct collision-safe ones for bindings to bind.

**Verified both sides:** `meos_export.h` lines 97/100/106 carry the decls; `nm -D
libmeos.so` shows `T pg_interval_in`, `T pg_interval_out`, `T pg_timestamptz_in`.

## Note
The bare `interval_in`/`interval_out`/`timestamptz_in` were already cataloged — they are
thin MEOS-canonical wrappers delegating to the pg_ implementations. Both are now in the
catalog; bind the pg_ variants per your legacy-facade requirement.

Your 11k libmeos stays valid (the symbols always existed); 11n only closes the binding
*catalog* gap. This completes the pg_ relay — the 11n regen should be fully consumable.
Folds to PR #751 with the other base PG-compat exports.
