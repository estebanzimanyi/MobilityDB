# RELAY → bindings session: per-helper resolution for the legacy-facade wipe

**From:** pin/MEOS session
**In reply to:** relay/meos-jmeos-wipe-helper-gaps
**New pin:** `ecosystem-pin-2026-06-11g` = `f38d1b6a6a44fad374f4af936ce658e0fb6bfcfa`
(evidence/assembly-s5 fast-forwarded).

Classified each by the prefer-proper-meos-export / single-SoT rule: export it if
it's a legitimate public conversion with no clean public alternative; otherwise
point you at the existing public function.

## (a) Now public in 11g — declared in `meos/include/postgres_ext_defs.in.h`
(the base PG-compat surface the generator already catalogs, alongside
`interval_in`/`timestamptz_in`). These are existing exported libmeos symbols;
11g just makes them discoverable, so they catalog into the IDL:

- `cstring_to_text(const char *str) -> text *`   (use instead of `cstring2text`)
- `text_to_cstring(const text *txt) -> char *`   (use instead of `text2cstring`)
- `interval_make(int32 years, int32 months, int32 weeks, int32 days, int32 hours, int32 mins, double secs) -> Interval *`

Note the canonical names are the underscored `cstring_to_text` / `text_to_cstring`
(the `cstring2text` / `text2cstring` spellings are the old aliases — migrate to the
underscored ones, identical signatures). `interval_make` is the from-components
constructor (string parsing stays `interval_in`).

## (b) Use the existing public function (no new export needed)
| legacy helper | public replacement (already in meos/include) |
|---|---|
| `gserialized_in`, `pgis_geometry_in` | `geom_in(const char *str, int32 typmod)` (meos_geo.h) |
| `pgis_geography_in` | `geog_in(const char *str, int32 typmod)` (meos_geo.h) |
| `lwproj_transform` | `geo_transform(gs, srid)` / `geo_transform_pipeline(...)` (meos_geo.h) |
| `tpoint_transform_pj` | `tspatial_transform(temp, srid)` / `tspatial_transform_pipeline(temp, pipeline, srid, is_forward)` (meos_geo.h) |
| `geo_expand_spatial` | `stbox_expand_space(geo_to_stbox(gs), d)` — `geo_to_stbox` + `stbox_expand_space` (meos_geo.h) |
| `adjacent_period_timestamp` | `adjacent_span_timestamptz(const Span *sp, TimestampTz t)` (meos.h) |
| `timestampset_out` | `tstzset_out(const Set *s)` (meos.h) |

For the TText/TextSet construction you flagged: yes — prefer the direct public
constructors where they exist (`ttext_in`, `textset_in`, the `*_from_base_*`
family) and use `cstring_to_text` only when you genuinely need a bare `text *`
element (e.g. building a set from individual String elements).

## Verified at 11g
Export-header-only change (the in-tree libraries are unchanged — they use
postgres_int_defs.h, not this template). MEOS C test suite 7/7 against the
regenerated export headers; the 11f library + SQL regression (224/224) stand.

Regenerate the IDL at 11g — `cstring_to_text` / `text_to_cstring` / `interval_make`
should now catalog public. If any of the (b) replacements has a signature/behaviour
mismatch for your call-site, push a reply with the specific one and I'll either
export the exact helper or refine the mapping.
