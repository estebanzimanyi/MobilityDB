<!--
Copyright(c) MobilityDB Contributors

This documentation is licensed under a
Creative Commons Attribution-Share Alike 3.0 License
https://creativecommons.org/licenses/by-sa/3.0/
-->

# Temporal Data Lake — an edge-to-cloud architecture for MobilityDB temporal data

Temporal and spatiotemporal data is generated continuously at the edge: AIS ship transponders, IoT sensors, GPS fleet trackers, mobile network probes. Without a shared substrate the lifecycle fragments three ways: ingest arrives as CSV, MQTT, or database rows with no typed temporal structure and trajectory reconstruction happens ad hoc in application code; there is no portable binary format for temporal sequences readable by DuckDB, Spark, and PostgreSQL without a MobilityDB installation; and SQL written for MobilityDB uses PostgreSQL-specific operator syntax that does not run on DuckDB or Spark. The Temporal Data Lake removes all three: MEOS is the single semantic substrate, TemporalParquet is the portable storage format, and a named-function SQL profile runs unchanged on every platform.

## Architecture

A Temporal Data Lake is a set of TemporalParquet-annotated Parquet files on object storage (S3, GCS, Azure Blob, or local filesystem), combined with a portable query dialect that makes those files queryable on any MobilityDB-ecosystem platform without conversion.

```
┌──────────────────────────────────────────────────────────────────────────────┐
│  EDGE / INGEST                                                               │
│                                                                              │
│  raw events (CSV, MQTT, NMEA, …)                                             │
│       │                                                                      │
│       ▼                                                                      │
│  MobilityDuck (DuckDB)  ──or──  MEOS C library                               │
│  • deduplicate + validate                                                    │
│  • build typed sequences: tgeogpointSeq / tintSeq / …                       │
│  • COPY … TO 'shard_NNN.parquet' (FORMAT PARQUET)                           │
│  • temporal_parquet.py annotate (inject temporal footer metadata)            │
└────────────────────────────┬─────────────────────────────────────────────────┘
                             │  TemporalParquet shards
                             ▼
┌──────────────────────────────────────────────────────────────────────────────┐
│  STORAGE LAYER  (object storage or filesystem)                               │
│                                                                              │
│  lake/                                                                       │
│    year=2026/month=02/day=26/                                                │
│      shard_000.parquet   ←──  BYTE_ARRAY traj column                        │
│      shard_001.parquet       + "temporal" footer key                        │
│      …                                                                       │
│                                                                              │
│  Each file is self-describing: the footer names the base_type, encoding,    │
│  srid, geodetic flag — no MobilityDB installation needed to interpret it.   │
└────────────────────────────┬─────────────────────────────────────────────────┘
                             │  read_parquet('lake/**/*.parquet')
                             ▼
┌──────────────────────────────────────────────────────────────────────────────┐
│  QUERY LAYER                                                                 │
│                                                                              │
│  ┌────────────────┐  ┌────────────────┐  ┌────────────────────────────────┐ │
│  │  MobilityDuck  │  │  MobilityDB    │  │  MobilitySpark / MobilityPySpark│ │
│  │  (DuckDB)      │  │  (PostgreSQL)  │  │  (Apache Spark + JMEOS)        │ │
│  │  laptop / edge │  │  OLTP + GiST   │  │  cloud-scale batch             │ │
│  └────────────────┘  └────────────────┘  └────────────────────────────────┘ │
│                                                                              │
│  All three platforms run the same portable SQL — named functions only,      │
│  no operator symbols.                                                        │
└──────────────────────────────────────────────────────────────────────────────┘
```

## Specification

### Shard format

Each shard is a Parquet file conforming to the TemporalParquet convention ([`doc/temporal-parquet/README.md`](../../temporal-parquet/README.md)):

- Temporal columns are `BYTE_ARRAY` carrying MEOS-WKB values.
- The file's `key_value_metadata` contains a `temporal` key whose value is a JSON object describing each temporal column (`base_type`, `encoding`, `srid`, `geodetic`, `subtype`, `interpolation`).
- Non-temporal columns (entity IDs, ping counts, partition keys) are plain Parquet types.

### Directory layout

Shards are organised in a Hive-style partition tree:

```
lake/
  year=YYYY/
    month=MM/
      day=DD/
        shard_NNN.parquet
```

Alternative partition dimensions:

| Use case | Partition key | Example |
|---|---|---|
| Time-series (default) | `year` / `month` / `day` | AIS trajectories, sensor streams |
| Spatial coverage | H3 resolution-3 cell | Regional analytics with `th3index` |
| Entity range | entity ID prefix / hash bucket | Fleet or user partitioning |

Partitions may be nested: `year=2026/month=02/h3cell=832830fffffffff/`.

### Relationship to MEOS multidimensional tiling

MEOS provides a 4-dimensional (X, Y, Z, T) grid tiling engine (`STboxGridState`, `tgeo_space_time_tile_init`, `tpoint_at_tile`, `tpoint_set_tiles`) that is the natural primitive for space-time shard partitioning. Its interaction with the data lake spans five points.

Tiling as a spatial-query-optimal partition key. Hive-style `year/month/day` partitioning enables time pruning only; a spatial query must still read all files for the time range and post-filter by geometry. MEOS space-time tiles act as a partition key in all four dimensions: Hive partition pruning skips any shard file whose tile does not intersect the query's `STBox`. This is predicate push-down at the file level, the strongest form of spatial index available to a flat file store.

All spatiotemporal types covered, not just `tgeompoint`. `spaceTimeSplit`, `spaceSplit`, `timeSplit`, and `valueSplit` are SQL TableFunctions for all spatiotemporal types (`tgeompoint`, `tgeogpoint`, `tgeometry`, `tgeography`, `tint`, `tfloat`, and all sequence and sequence-set subtypes), so tile-based shard writing is available for any column type, not just point trajectories.

```sql
-- Write one shard per space-time tile — works for tgeompoint, tgeogpoint, tgeometry, …
INSERT INTO shard_table
SELECT tile_x, tile_y, tile_t,
       asBinary(traj_fragment)             AS traj,
       numInstants(traj_fragment)          AS ping_count
FROM (
    SELECT * FROM spaceTimeSplit(
        (SELECT traj FROM trajectories WHERE entity_id = :id),
        1.0,        -- xsize  (degrees for tgeogpoint, metres for projected)
        1.0,        -- ysize
        '1 hour'    -- duration
    )
) AS t(traj_fragment, tile_x, tile_y, tile_t);
```

BitMatrix as a Parquet spatial bloom filter. `tpoint_set_tiles(traj, grid_state, bitmatrix)` fills a `BitMatrix` of which tiles a trajectory passes through without clipping it. This BitMatrix can be stored in the Parquet shard footer alongside the `temporal` metadata key. A reader can skip an entire file by checking whether any cell in the BitMatrix intersects the query region, at the cost of a single footer read and zero row-group scans.

Fragmentation breaks the one-row-per-trajectory assumption. `spaceTimeSplit` clips a trajectory to a tile's spatial-temporal extent. If shards are written one row per tile per entity (fragmented layout), `*FromBinary` returns a fragment, not the full trajectory. Queries that need the full sequence (total `length`, `speed` profile, `eIntersects` over the whole path) must reassemble fragments with a `tgeogpointSeqSet(list(...))` aggregate. The unfragmented layout (one row per entity, full trajectory stored once) is the default because it is simpler and sufficient for most analytical workloads. Fragmented layout is an advanced option for workloads where spatial pruning dominates.

H3 versus MEOS tiles, chosen by CRS. H3 uses a discrete global hexagonal grid that distributes area uniformly on the sphere, the natural choice for WGS-84 lon/lat `tgeogpoint` data. MEOS tiles are axis-aligned rectangles, the natural choice for projected CRS (UTM zones, local coordinate systems).

| Data type | CRS | Recommended partition grid |
|---|---|---|
| `tgeogpoint` | WGS-84 lon/lat | H3 cells (`th3index`) |
| `tgeompoint` | Projected (UTM, etc.) | MEOS space-time tiles (`spaceTimeSplit`) |

A two-level partition (`h3cell=.../year=.../month=...`) gives spatial and temporal pruning without fragmentation.

### Ingest recipe (MobilityDuck)

```sql
-- Step 1: load raw events (deduplication + validation)
CREATE OR REPLACE TABLE raw AS
SELECT
    CAST(ts_str AS TIMESTAMPTZ) AS ts,
    CAST(entity_id AS BIGINT)   AS entity_id,
    CAST(lat AS DOUBLE)         AS lat,
    CAST(lon AS DOUBLE)         AS lon
FROM read_csv_auto('events_*.csv', header = true, nullstr = '')
WHERE TRY_CAST(lat AS DOUBLE) BETWEEN  -90 AND  90
  AND TRY_CAST(lon AS DOUBLE) BETWEEN -180 AND 180
QUALIFY ROW_NUMBER() OVER (PARTITION BY CAST(entity_id AS BIGINT), ts_str
                           ORDER BY     ts_str) = 1;

-- Step 2: build typed temporal sequences
CREATE OR REPLACE TABLE trajectories AS
SELECT
    entity_id,
    tgeogpointSeq(
        list(TGEOGPOINT(ST_Point(lon, lat), ts) ORDER BY ts)
    ) AS traj
FROM raw
GROUP BY entity_id
HAVING count(*) >= 3;

-- Step 3: write shard
COPY (
    SELECT
        entity_id,
        asBinary(traj) AS traj,
        numInstants(traj) AS ping_count
    FROM trajectories
)
TO 'lake/year=2026/month=02/day=26/shard_000.parquet'
(FORMAT PARQUET, ROW_GROUP_SIZE 1000);

-- Step 4: annotate with temporal footer metadata
-- python3 tools/temporal_parquet.py annotate shard_000.parquet \
--   --column "name=traj,base_type=tgeogpoint,subtype=Sequence,interp=linear,srid=4326,geodetic=true"
```

### Portable query recipe

The following SQL runs unchanged on MobilityDuck, MobilityDB, and MobilitySpark (portable dialect: named functions, no operator symbols):

```sql
-- Top 10 entities by total trajectory length (metres, geodetic)
SELECT
    entity_id,
    ping_count,
    round(length(traj))             AS length_m,
    round(maxValue(speed(traj)), 2) AS max_speed_ms
FROM (
    SELECT
        entity_id,
        ping_count,
        tgeogpointFromBinary(traj)  AS traj   -- MobilityDuck / MobilitySpark
        -- tgeogpoint(traj)                   -- MobilityDB (after COPY … BINARY)
    FROM read_parquet('lake/**/*.parquet')
)
ORDER BY length_m DESC
LIMIT 10;

-- Entities that passed through a region of interest
SELECT entity_id, ping_count
FROM (
    SELECT entity_id, ping_count, tgeogpointFromBinary(traj) AS traj
    FROM read_parquet('lake/**/*.parquet')
)
WHERE eIntersects(
    ST_GeomFromText('POLYGON((11.5 55.0, 13.5 55.0, 13.5 56.5, 11.5 56.5, 11.5 55.0))'),
    traj
);
```

### Geodetic note

Use `tgeogpoint`, not `tgeompoint`, for any column where distance or speed will be computed. `tgeogpoint` stores the geodetic flag in the MEOS-WKB type tag; the flag is preserved through the Parquet round-trip and is self-describing on every platform. `length(tgeogpoint)` returns metres uniformly across MobilityDuck, MobilityDB, and PyMEOS/MobilitySpark. See the [TGEOGPOINT design note](https://github.com/MobilityDB/MobilityDuck/blob/main/docs/tgeogpoint-design.md).

## The data-interchange spec stack

The architecture rests on three normative specifications plus a portable-SQL profile. Each spec is a stand-alone document; this document defines how they compose into the edge-to-cloud workflow.

Layer 1, wire format: [`doc/specs/meos-wkb-0.9.md`](../../specs/meos-wkb-0.9.md), MEOS-WKB. The byte-level encoding for every MobilityDB temporal value (TInstant / TSequence / TSequenceSet across all base types). Every MEOS binding, every TemporalParquet payload column, and every WKB-flavoured I/O path inside MobilityDB serialises to and deserialises from this format.

Layer 2, file format: [`doc/temporal-parquet/README.md`](../../temporal-parquet/README.md) and the matching DocBook chapter [`doc/temporal_parquet.xml`](../../temporal_parquet.xml), TemporalParquet. Standardises the Parquet footer convention, the payload column (MEOS-WKB `BYTE_ARRAY`), and the spatial and temporal min-max statistics needed for predicate pushdown.

Layer 3, function registry: [`doc/specs/meos-api-0.1-draft.md`](../../specs/meos-api-0.1-draft.md), MEOS-API. The cross-binding contract: every MobilityDB function name, signature, and JSON-schema-validated argument set that PyMEOS, JMEOS, MobilityDuck, and MobilitySpark are expected to expose. The portable-SQL named-function dialect sources its function set from this registry.

Layer 4, query dialect: [Discussion #861](https://github.com/MobilityDB/MobilityDB/discussions/861), Edge-to-Cloud portable SQL. The named-function SQL profile (used in the Portable query recipe above) that runs unchanged on DuckDB, PostgreSQL, and Spark by binding to MEOS-API function names rather than dialect-specific operators.

The four layers compose: a MobilityDuck `COPY trips TO '...' (FORMAT PARQUET)` writes MEOS-WKB-encoded rows under the TemporalParquet footer convention; a portable-SQL `SELECT eIntersects(region, trip_h3) FROM ...` binds via MEOS-API to the same kernel on whichever platform reads the shard.

## Related

- [Discussion #913](https://github.com/MobilityDB/MobilityDB/discussions/913) — Temporal Data Lake community discussion thread
- [Discussion #861](https://github.com/MobilityDB/MobilityDB/discussions/861) — Edge-to-Cloud portable SQL profile
- [MobilityDuck](https://github.com/MobilityDB/MobilityDuck) — DuckDB ingest and query engine, primary TemporalParquet consumer; carries the quickstart, generic-ingest, and AIS data-lake examples and the `temporal_parquet.py` annotation tool
- [MobilitySpark](https://github.com/MobilityDB/MobilitySpark) — Apache Spark analytics engine; carries the BerlinMOD portable-SQL benchmark
- [MEOS-API repo](https://github.com/MobilityDB/MEOS-API) — function-registry implementation, parsed from MEOS headers and consumed by all bindings
- AIS reference dataset: Danish Maritime Authority open data
