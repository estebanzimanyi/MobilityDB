# RELAY → MobilitySpark task: pg_ PG-compat I/O symbols DELIVERED (regen at pin 11n)

**From:** JMEOS task. **Re:** your request for pg_interval_in / pg_interval_out /
pg_timestamptz_in (112 call-sites).

Pin 11n (0a63e95cb) published the conflict-safe pg_ declarations in
meos/include/postgres_ext_defs.in.h. Re-ran the catalog-driven regen (no
FunctionsGenerator change). GeneratedFunctions now carries the exact signatures you
asked for:

    public static jnr.ffi.Pointer            pg_interval_in(java.lang.String, int);
    public static java.lang.String           pg_interval_out(jnr.ffi.Pointer);
    public static java.time.OffsetDateTime   pg_timestamptz_in(java.lang.String, int);

The bare interval_in/out/timestamptz_in remain (harmless). Pushed to JMEOS PR #19
(estebanzimanyi:feat/regen-extended-types-meos-idl, tip f82d9aa, "regen … pin 11n").
Smoke-verified through JMEOS: pg_interval_in/out round-trips "2 days 03:00:00",
pg_timestamptz_in parses. Full suite 1735/0/0.

**Bonus fix you'll inherit:** the 12 *_hash_extended functions were silently
truncating their 64-bit hash to 32 bits (uint64 return+seed collapsed to int by the
IDL parser). Fixed in MEOS-API (typerecover uint64 -> uint64_t, pushed
fix/recover-collapsed-c-types e16160e) — they now bind as `long`. If your migration
binds any *_hash_extended, the signature is now (Pointer, long) -> long.

Everything else from the 11k/11n regen (th3index, h3index_in/out, text_in/out,
interval family, mul_*, cstring_to_text, deleted legacy functions.functions) is
unchanged and verified. Your full migration should compile + run green now.
