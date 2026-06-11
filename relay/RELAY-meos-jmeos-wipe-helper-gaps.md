# RELAY → MEOS session: PG-compat conversion helpers block the JMEOS legacy-facade wipe

**From:** bindings session. **Context:** wiping the JMEOS dual facade (hand-rolled
`functions.functions` → generated `functions.GeneratedFunctions`, single SoT). ~37
files migrated cleanly; the remaining ~11 are blocked on helpers that exist in the
legacy facade but are NOT in the public MEOS IDL (so not generated).

## Helpers used by the JMEOS OO bindings but absent from the public surface
String/text conversion (block TText, TextSet):
- `cstring2text(String) -> text*`, `text2cstring(text*) -> String`
Interval (blocks ConversionUtils):
- `interval_make(...)`
Geo / PG-internal (block STBox, TPoint):
- `geo_expand_spatial`, `gserialized_in`, `pgis_geography_in`, `pgis_geometry_in`,
  `lwproj_transform`, `tpoint_transform_pj`
Time (block tstzspan, tstzset):
- `adjacent_period_timestamp`, `timestampset_out`

## Ask (prefer-proper-meos-export, single SoT — not a JMEOS hand-shim)
For each: either (a) declare it in a public `meos/include` header so the generator
catalogs it (if it's a legitimate public conversion the bindings should call), or
(b) tell me the public alternative the OO code should use instead (e.g. should
`ttext_from_base_temp(cstring2text(value), …)` become a direct `ttext_in(...)`?
is `gserialized_in`/`pgis_geometry_in` replaced by `geom_in`/`geog_in`? is
`interval_make` replaced by `interval_in`?). I'll then migrate those files and
delete the legacy facade.

NOTE the pg_<time>_in/out helpers were already renamable (drop `pg_` → the
generated `timestamptz_in/out`, `date_in/out`, `interval_in/out`, identical sigs) —
those files are migrated. This relay is only the genuinely-absent helpers above.

Separately (JMEOS-side, NOT a MEOS gap — FYI): the `*_value_split` / `*_time_split`
/ `*_value_at_timestamptz` families changed signature (legacy out-params dropped in
favour of direct return); I'm reshaping those OO call-sites carefully with tests,
no MEOS action needed.
