<!--
Copyright(c) MobilityDB Contributors

This documentation is provided under Creative Commons Attribution 4.0
International License (CC BY 4.0): https://creativecommons.org/licenses/by/4.0/
-->

# BerlinMOD portable benchmark

The BerlinMOD Q1–Q17 + QRT (binary roundtrip) suite in the portable
named-function dialect. The same `.sql` file produces the same answer on
MobilityDB (PostgreSQL), MobilityDuck (DuckDB), and MobilitySpark (Spark SQL)
without modification. This directory is self-contained: schema, queries, and a
reference dataset, with no dependency on any other repository.

## Files

| File | Purpose |
|---|---|
| `schema.sql` | The seven tables, identical on all three platforms |
| `portable_aliases.sql` | MobilityDB-only: bare-name `overlaps` aliases DuckDB/Spark already expose |
| `data/load.sql` | Self-contained synthetic reference dataset (5 trips, planar SRID 0) |
| `q01.sql` … `q17.sql` | The 17 BerlinMOD queries |
| `qrt.sql` | Binary roundtrip — every trip serialised as MEOS hex-WKB |

## Portable dialect rule

Queries use **named functions only — no operator symbols**. Operator
equivalents appear only in `--` header comments for reference. The executable
SQL contains no `&&`, `|=|`, `<->`, `<<#`, `@>`, … . Verify:

```bash
grep -nE '(&&|\|=\||<->|<<#|#>>|@>|<@|&<|&>)' q*.sql qrt.sql | grep -vE ':[0-9]+: *--'
# (no output == compliant)
```

## Run on the reference engine (MobilityDB / PostgreSQL)

```bash
createdb berlinmod_portable
psql -d berlinmod_portable -c 'CREATE EXTENSION mobilitydb CASCADE;'
psql -d berlinmod_portable -f schema.sql -f portable_aliases.sql -f data/load.sql
for q in q??.sql qrt.sql; do
  psql -d berlinmod_portable -At -F',' -f "$q" > "expected/${q%.sql}.csv"
done
```

`portable_aliases.sql` is loaded on MobilityDB only — it adds the bare-name
`overlaps` overloads the suite needs (MobilityDB backs `&&` with
`span_overlaps`/`temporal_overlaps`). DuckDB and MobilitySpark already expose
`overlaps` natively and run the suite without it.

MobilityDB is the reference engine: the `expected/` CSVs it produces are the
oracle the other two platforms are checked against. MobilityDuck and
MobilitySpark run the identical `q*.sql` over the same logical dataset and
must produce byte-identical output (every query has a total `ORDER BY`; the
QRT hex-WKB is byte-for-byte identical because all three call the same MEOS
serializer).

## Scaling

The pairwise and multi-dimensional queries are the cost drivers:

| Query | Shape | Built-in portable pre-filter |
|---|---|---|
| Q5 | all query-licence pairs × `nearestApproachDistance` | `overlaps(timeSpan(t1.trip), timeSpan(t2.trip))` — temporal overlap is a necessary condition for a non-NULL NAD |
| Q6 | all truck pairs × `eDwithin` | `overlaps(expandSpace(t1.trip, 10.0), t2.trip)` — within-10 m implies expanded-bbox overlap |
| Q10 | query-licence × all other trips × `tDwithin` | `overlaps(t2.trip, expandSpace(t1.trip, 3))` |
| Q12 | trips × points × instants, self-joined on pairs | `overlaps(t.trip, stbox(p.geom, i.instant))` |
| Q13–Q16 | trips × regions/points × periods | `overlaps(t.trip, stbox(geom, period))` + the original BerlinMOD 10-item subset convention (`regionId <= 10`, …) |

Each pre-filter is a **necessary condition** for the query's result — it
cannot drop a true row (the proof is in each query header). It is what makes
the comparison apples-to-apples: PostgreSQL and DuckDB prune the pair space
with an STBox/span index, and Spark's Catalyst turns the cross product into a
broadcast/range-partitioned join with cheap bounding-box pruning, instead of
evaluating the expensive temporal predicate over the full O(n²) product.

For timing runs, scale the dataset by the number of trips and keep the
QueryLicences / QueryPoints / QueryRegions / QueryPeriods subsets at the
BerlinMOD 10-item convention so the per-query cost stays comparable across
engines.
