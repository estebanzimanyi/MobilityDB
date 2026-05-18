<!--
Copyright(c) MobilityDB Contributors

This documentation is provided under Creative Commons Attribution 4.0
International License (CC BY 4.0): https://creativecommons.org/licenses/by/4.0/
-->

# RFC: Edge-to-Cloud SQL Portability for the MobilityDB Ecosystem

> Discussion: [#861](https://github.com/MobilityDB/MobilityDB/discussions/861)

This RFC is the **portable-computation** half of the
[Mobility data platform interoperability index](https://github.com/MobilityDB/MobilityDB/pull/1074):
the same mobility analytics run, comparably, across the ecosystem's engines. Its
**portable-data** counterpart is the Temporal Data Lake RFC
([#913](https://github.com/MobilityDB/MobilityDB/discussions/913)) — mobility values produced
by one engine read losslessly by any other. Each is necessary; neither is sufficient.

## The Problem

The MobilityDB ecosystem spans three execution platforms:

| Platform | Engine | Implementation |
|---|---|---|
| [MobilityDB](https://github.com/MobilityDB/MobilityDB) | PostgreSQL | Native C + PostGIS |
| [MobilityDuck](https://github.com/MobilityDB/MobilityDuck) | DuckDB | C++ extension (MEOS-backed) |
| [MobilitySpark](https://github.com/MobilityDB/MobilitySpark) | Apache Spark | JVM UDFs via [JMEOS](https://github.com/MobilityDB/JMEOS) |

Each platform understands the same temporal and spatial types — `tgeompoint`, `tgeometry`,
`tstzspan`, `tbox`, `stbox` — because all three are backed by the same
[MEOS](https://github.com/MobilityDB/MEOS-API) C library (either directly or through JMEOS).

The same SQL query cannot yet run unchanged on all three platforms because MobilityDB's
richest interface uses PostgreSQL custom operators (`&&`, `@>`, `<<#`, `?=`, `<->`, …) that
neither DuckDB nor Spark SQL can register.

## Proposal

A portable query uses only named functions, no operator symbols. The table below is the
canonical bare-name dialect. MobilityDB registers it natively
([PR #1075](https://github.com/MobilityDB/MobilityDB/pull/1075)): every operator overload is
aliased to its own backing C symbol, so the bare name and the operator are the same
implementation. MobilityDuck and MobilitySpark today expose type-qualified names
(e.g. `spanOverlaps`, `temporalContains`); adopting the same canonical bare names there, and
in the [`meos-api.json`](https://github.com/MobilityDB/MEOS-API) registry, is tracked
cross-platform follow-up. Until PR #1075 merges, the BerlinMOD suite keeps
[`berlinmod/portable_aliases.sql`](berlinmod/portable_aliases.sql) as a MobilityDB-side
fallback.

### Operator → Function mapping

| Category | MobilityDB operator | Portable function |
|---|---|---|
| **Time position** | `<<#` | `before(a, b)` |
| | `#>>` | `after(a, b)` |
| | `&<#` | `overbefore(a, b)` |
| | `#&>` | `overafter(a, b)` |
| **Space — X axis** | `<<` | `left(a, b)` |
| | `>>` | `right(a, b)` |
| | `&<` | `overleft(a, b)` |
| | `&>` | `overright(a, b)` |
| **Space — Y axis** | `<<`&#124; | `below(a, b)` |
| | &#124;`>>` | `above(a, b)` |
| | `&<`&#124; | `overbelow(a, b)` |
| | &#124;`&>` | `overabove(a, b)` |
| **Space — Z axis** _(3D types only)_ | `<</` | `front(a, b)` |
| | `/>>` | `back(a, b)` |
| | `&</` | `overfront(a, b)` |
| | `/&>` | `overback(a, b)` |
| **Ever** | `?=` | `ever_eq(v, t)` |
| | `?<>` | `ever_ne(v, t)` |
| | `?<` | `ever_lt(v, t)` |
| | `?<=` | `ever_le(v, t)` |
| | `?>` | `ever_gt(v, t)` |
| | `?>=` | `ever_ge(v, t)` |
| **Always** | `%=` | `always_eq(v, t)` |
| | `%<>` | `always_ne(v, t)` |
| | `%<` | `always_lt(v, t)` |
| | `%<=` | `always_le(v, t)` |
| | `%>` | `always_gt(v, t)` |
| | `%>=` | `always_ge(v, t)` |
| **Topology** | `&&` | `overlaps(a, b)` |
| | `@>` | `contains(a, b)` |
| | `<@` | `contained(a, b)` |
| | `-`&#124;`-` | `adjacent(a, b)` |
| **Distance** | `<->` | `tdistance(a, b)` |
| | &#124;`=`&#124; | `nearestApproachDistance(a, b)` |
| **Temporal comparison** | `#=` | `teq(a, b)` |
| | `#<>` | `tne(a, b)` |
| | `#<` | `tlt(a, b)` |
| | `#<=` | `tle(a, b)` |
| | `#>` | `tgt(a, b)` |
| | `#>=` | `tge(a, b)` |
| **Same** | `~=` | `same(a, b)` |

Spatial relationship functions (`eIntersects`, `aIntersects`, `tIntersects`, `eTouches`,
`eContains`, `eDwithin`, …) and restriction functions (`atTime`, `atGeometry`, `atValue`, …)
already use named-function syntax in MobilityDB and require no mapping — write them the same
way on all three platforms.

### Serialization interchange

Objects round-trip between platforms unchanged. The canonical interchange format is MF-JSON
(OGC Moving Features JSON), supported by `asMFJSON()` / `fromMFJSON()` in MobilityDB and MEOS.
For bulk transfer, MEOS binary (hex-encoded WKB extension) is the efficient alternative; the
Arrow C Data Interface export is the columnar, zero-runtime variant verified by the Temporal
Data Lake portable-data lane.

The `nearestApproachDistance` failure sentinel is `DBL_MAX` at the MEOS C level
([#846](https://github.com/MobilityDB/MobilityDB/pull/846)) and is exposed as `NULL` at the
SQL level on every platform — MobilityDB, MobilityDuck, and the Spark/PySpark UDF surface.
There is no platform-specific sentinel in the portable dialect.

## BerlinMOD Reference Queries

The [`berlinmod/`](berlinmod/) subdirectory is the canonical source of truth for the
benchmark suite (Q1–Q17 + binary roundtrip) written in the portable dialect. The same `.sql`
file produces the same result on MobilityDB, MobilityDuck, and MobilitySpark without
modification.

| Query | Temporal operations used | File |
|---|---|---|
| Q1  | None (relational join) | [berlinmod/q01.sql](berlinmod/q01.sql) |
| Q2  | `eIntersects()` | [berlinmod/q02.sql](berlinmod/q02.sql) |
| Q3  | `atTime()` + `asHexWKB()` | [berlinmod/q03.sql](berlinmod/q03.sql) |
| Q4  | `eIntersects()` | [berlinmod/q04.sql](berlinmod/q04.sql) |
| Q5  | `nearestApproachDistance()` + `MIN()` | [berlinmod/q05.sql](berlinmod/q05.sql) |
| Q6  | `eDwithin()` | [berlinmod/q06.sql](berlinmod/q06.sql) |
| Q7  | `atTime()` (period) + `asHexWKB()` | [berlinmod/q07.sql](berlinmod/q07.sql) |
| Q8  | `trajectory()` | [berlinmod/q08.sql](berlinmod/q08.sql) |
| Q9  | `atTime()` + `length()` + `overlaps()` pre-filter | [berlinmod/q09.sql](berlinmod/q09.sql) |
| Q10 | `tDwithin()` + `whenTrue()` + `expandSpace()` | [berlinmod/q10.sql](berlinmod/q10.sql) |
| Q11 | `valueAtTimestamp()` + `stbox()` pre-filter | [berlinmod/q11.sql](berlinmod/q11.sql) |
| Q12 | `valueAtTimestamp()` + `stbox()` pre-filter (pairs) | [berlinmod/q12.sql](berlinmod/q12.sql) |
| Q13 | `atTime()` + `eIntersects()` + `stbox()` pre-filter | [berlinmod/q13.sql](berlinmod/q13.sql) |
| Q14 | `valueAtTimestamp()` + `ST_Contains()` + `stbox()` pre-filter | [berlinmod/q14.sql](berlinmod/q14.sql) |
| Q15 | `atTime()` + `eIntersects()` + `stbox()` pre-filter | [berlinmod/q15.sql](berlinmod/q15.sql) |
| Q16 | `atTime()` + `eIntersects()` + `aDisjoint()` + `stbox()` pre-filter | [berlinmod/q16.sql](berlinmod/q16.sql) |
| Q17 | `eIntersects()` + `COUNT(DISTINCT)` + subquery MAX | [berlinmod/q17.sql](berlinmod/q17.sql) |
| QRT | `asHexWKB()` (binary roundtrip verification) | [berlinmod/qrt.sql](berlinmod/qrt.sql) |

Cross-platform timing and readiness measurements for this suite are published, with their
methodology, in the
[MobilityDB-BerlinMOD benchmark restructure](https://github.com/estebanzimanyi/MobilityDB-BerlinMOD/tree/doc/benchmark-restructure/BerlinMOD/benchmarks).

## Status

Phases refer to the six-phase plan in
[discussion #861](https://github.com/MobilityDB/MobilityDB/discussions/861).

| Phase | What | State |
|---|---|---|
| 1 | JMEOS → MobilitySpark wiring | In review — [JMEOS #9](https://github.com/MobilityDB/JMEOS/pull/9), regenerated against MEOS 1.4 in [JMEOS #15](https://github.com/MobilityDB/JMEOS/pull/15) |
| 2 | Canonical function registry (`meos-api.json`) | In review — [MEOS-API #1–#4](https://github.com/MobilityDB/MEOS-API/pulls) |
| 3 | MobilitySpark generated UDF surface | In review — [MobilitySpark #5](https://github.com/MobilityDB/MobilitySpark/pull/5): temporal+geo at 100% parity (858/858); `cbuffer`/`npoint`/`pose`/`rgeo` tracked remaining toward full parity |
| 4 | Serialization alignment (MF-JSON / MEOS-WKB / Arrow) | In review — Arrow C Data Interface PR stack on MobilityDB; MobilityDuck `temporalFooter` |
| 5 | BerlinMOD Q1–Q17 + QRT three-platform run | Works on MobilityDB, MobilityDuck, and MobilitySpark |
| 6 | Spark Catalyst predicate push-down | Partial — th3index prefilter in [MobilitySpark #9](https://github.com/MobilityDB/MobilitySpark/pull/9) |

MobilityDB registers the canonical bare-name surface natively
([PR #1075](https://github.com/MobilityDB/MobilityDB/pull/1075)) — 1303 operator-overload
aliases across **all** temporal types (temporal, geo, cbuffer, npoint, pose, rgeo), 100%
operator coverage by the committed audit. MobilityDuck and MobilitySpark expose the
MobilityDB function surface type-qualified; their temporal+geo surface is at 100% by the
shared audit, with `cbuffer`/`npoint`/`pose`/`rgeo` and the canonical bare names (and
`meos-api.json`) tracked as remaining cross-platform work toward full parity — a tracked
gap, not an accepted exclusion. The `nearestApproachDistance` sentinel difference that previously
blocked Q5 on PySpark is resolved — every platform returns SQL `NULL` on failure. The
BerlinMOD Q1–Q17 + QRT suite runs on all three platforms (MobilityDB natively once PR #1075
merges, via the bundled fallback until then); cross-platform result equality is checked by
the documented reference run in [`berlinmod/README.md`](berlinmod/README.md).

## Future work

- Open this branch as a PR to `master`, or fold it into the
  [interoperability index PR #1074](https://github.com/MobilityDB/MobilityDB/pull/1074) as
  the portable-computation companion.
- Merge [PR #1075](https://github.com/MobilityDB/MobilityDB/pull/1075) (native bare-name
  alias layer in MobilityDB core); then drop the `berlinmod/portable_aliases.sql` fallback.
- Expose the same canonical bare-name table in MobilityDuck, MobilitySpark, and the
  `meos-api.json` registry so all three engines share one named-function surface.
- Land the stacked ecosystem waves in merge order: Arrow C Data Interface substrate
  (MobilityDB), MobilityDuck type-port chain, MEOS-API registry, JMEOS 1.4, MobilitySpark #5.
- Add Catalyst push-down rules for `eIntersects` / `before` / `after` (Phase 6) leveraging
  the SP-GiST primitives.

## Related

- [MobilityDB](https://github.com/MobilityDB/MobilityDB) — PostgreSQL extension
- [MobilityDuck](https://github.com/MobilityDB/MobilityDuck) — DuckDB extension
- [MobilitySpark](https://github.com/MobilityDB/MobilitySpark) — Apache Spark UDFs (Java/JMEOS)
- [JMEOS](https://github.com/MobilityDB/JMEOS) — Java bindings for MEOS
- [MEOS-API](https://github.com/MobilityDB/MEOS-API) — canonical function registry (`meos-api.json`)
- [Mobility data platform interoperability index (#1074)](https://github.com/MobilityDB/MobilityDB/pull/1074) — portable data + portable computation umbrella
- [Temporal Data Lake RFC (#913)](https://github.com/MobilityDB/MobilityDB/discussions/913) — portable-data counterpart
