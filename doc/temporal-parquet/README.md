<!--
Copyright(c) MobilityDB Contributors

This documentation is licensed under a
Creative Commons Attribution-Share Alike 3.0 License
https://creativecommons.org/licenses/by-sa/3.0/
-->

# TemporalParquet — a Parquet footer convention for MobilityDB temporal types

TemporalParquet is a Parquet footer-metadata convention for files containing MobilityDB temporal columns, modelled directly on GeoParquet. Each temporal column is a `BYTE_ARRAY` carrying the MEOS-WKB encoding of the value; a JSON object in the Parquet file footer describes each column's base type, encoding, SRID, and interpolation. It lets `tgeompoint`, `tint`, `tfloat`, and the other temporal types survive an export and import round-trip with structure intact, where the prior options were MF-JSON text (3 to 10 times larger, no spatial-tooling hooks) or opaque hex EWKB (compact but unparseable outside MobilityDB and MEOS).

## File structure

For every column carrying a MobilityDB temporal type, the column is `BYTE_ARRAY` with logical-type `NONE`; each row value is the MEOS-WKB encoding of the temporal value; nulls are encoded as Parquet nulls.

The Parquet file's `key_value_metadata` carries an entry with key `temporal` whose value is a JSON document:

```jsonc
{
  "version": "1.0.0",
  "primary_temporal_column": "traj",
  "columns": {
    "traj": {
      "encoding": "MEOS-WKB",
      "encoding_version": "1.0",
      "base_type": "tgeompoint",
      "subtype": "Sequence",
      "interpolation": "linear",
      "srid": 4326,
      "geodetic": false,
      "has_z": false
    }
    /* one entry per temporal column */
  }
}
```

The `temporal` object coexists with GeoParquet's `geo` object: a single file can have both.

## Type coverage

| MobilityDB type | `base_type` value | Notes |
|---|---|---|
| `tbool`, `tint`, `tfloat`, `ttext` | `tbool` / `tint` / `tfloat` / `ttext` | scalar temporals |
| `tgeompoint`, `tgeogpoint` | `tgeompoint` / `tgeogpoint` | spatial-temporal; `srid` + `geodetic` + `has_z` populated |
| `tgeometry`, `tgeography` | `tgeometry` / `tgeography` | general spatial-temporal |
| `tcbuffer`, `tnpoint`, `tpose`, `trgeo`, `tpcpoint`, `tpcpatch`, `th3index` | each as its own `base_type` | extended temporal types |
| `stbox`, `tbox` | `stbox` / `tbox` | bounding boxes |
| `intspan`, `floatspan`, `tstzspan`, spansets, sets | each as its own `base_type` | spans, spansets, sets |

The `subtype` field applies only to lifted temporal types (Instant / Sequence / SequenceSet); span, set, and box columns omit it.

### Type-specific optional fields

Some `base_type` values carry optional metadata that lets consumers decide whether the column is usable for a given workload without decoding any row:

| Field | Applies to | Semantics |
|---|---|---|
| `srid` | `tgeompoint`, `tgeogpoint`, `tgeometry`, `tgeography` | EPSG code of the column's CRS; required for spatial-temporal types |
| `geodetic` | `tgeogpoint`, `tgeography` | `true` means spheroidal-metre math (Haversine / Vincenty); see [Geodetic distances](#geodetic-distances) |
| `has_z` | spatial-temporal types | column carries a Z dimension throughout |
| `h3_resolution` | `th3index` | optional integer in `[0, 15]` declaring that every cell in the column was produced at this resolution; consumers may rely on it for cell-membership prefilters, which only make sense when both sides share a resolution |

## Encoding versioning

`encoding_version` is `MAJOR.MINOR` of the WKB schema. New WKB tags bump MINOR; breaking layout changes bump MAJOR. Readers MUST refuse files with a higher MAJOR than they support.

## Geodetic distances

`tgeompoint` stores coordinates in the input CRS (for example lon/lat WGS-84) and computes Euclidean distances in that coordinate space, so `length(tgeompoint)` over a WGS-84 trajectory returns degrees, not kilometres.

`tgeogpoint` is the geodetically-correct variant: it carries the same MEOS-WKB bytes with the geodetic flag set in the type tag, causing MEOS to route all spatial math through the spheroidal Haversine / Vincenty engine, so lengths and speeds are in metres. The geodetic flag is self-describing in MEOS-WKB: a file written with `asBinary(tgeogpointSeq(...))` reconstructs as a geodetic sequence on any platform that calls `tgeogpointFromBinary(blob)`. When writing files that consumers will use for distance or speed analytics, prefer `tgeogpoint` and set `"geodetic": true` in the column metadata.

DuckDB has no native GEOGRAPHY type in its core (the community `duckdb-geography` extension uses Google S2's spherical model, which is incompatible with MEOS's spheroidal Vincenty engine). MobilityDuck's `TGEOGPOINT` therefore accepts the same `GEOMETRY` lon/lat input as `TGEOMPOINT` and sets the geodetic flag internally; all geodetic math lives exclusively in MEOS. This is the uniform pattern across MobilityDB, MobilityDuck, and PyMEOS.

## Worked example: th3index

A `th3index` column uses the same `BYTE_ARRAY` carrier as every other temporal type. The MEOS-WKB encoder handles the th3index payload identically to `tbigint` (both lift over an `int64`); the metadata `base_type` is the discriminator. Example footer for a BerlinMOD trips table carrying both a trajectory and a companion H3 column:

```jsonc
{
  "version": "1.0.0",
  "primary_temporal_column": "trip",
  "columns": {
    "trip": {
      "encoding": "MEOS-WKB",
      "encoding_version": "1.0",
      "base_type": "tgeompoint",
      "subtype": "Sequence",
      "interpolation": "linear",
      "srid": 4326,
      "geodetic": false,
      "has_z": false
    },
    "trip_h3": {
      "encoding": "MEOS-WKB",
      "encoding_version": "1.0",
      "base_type": "th3index",
      "subtype": "Sequence",
      "interpolation": "step",
      "h3_resolution": 7
    }
  }
}
```

The companion `trip_h3` column is produced once at load time via `tgeompoint_to_th3index(trip, 7)` and acts as a cheap cell-membership prefilter on subsequent cross-join queries:

```sql
SELECT t1.licence AS licence1, t2.licence AS licence2,
       nearestApproachDistance(t1.trip, t2.trip) AS d
FROM   Trips t1 JOIN Trips t2 ON t1.vehId < t2.vehId
WHERE  everEqTh3IndexTh3Index(t1.trip_h3, t2.trip_h3)
  AND  nearestApproachDistance(t1.trip, t2.trip) IS NOT NULL;
```

`interpolation` is always `step` for th3index, since a vehicle is in exactly one cell at a time; `subtype` follows the source `tgeompoint` subtype after the lifted conversion. The `h3_resolution` field lets a reader gate the prefilter without decoding any row.

Limitation: `tgeompoint_to_th3index` samples one cell per source instant; cells traversed by the straight-line segment between consecutive instants are not visited, so `trip_h3` under-samples the true cell set and a prefilter built on it may miss true hits. The metadata schema is unaffected: `base_type` and `h3_resolution` describe the bytes regardless of how the column was produced. A consumer needing strict semantic correctness evaluates the underlying `tgeompoint` predicate directly rather than relying on the prefilter as a `WHERE` clause.

## Related

- [Issue #830](https://github.com/MobilityDB/MobilityDB/issues/830) — TemporalParquet community discussion thread
- [`doc/specs/meos-wkb-0.9.md`](../specs/meos-wkb-0.9.md) — MEOS-WKB byte-format specification, the encoding TemporalParquet columns carry
- [`doc/specs/meos-api-0.1-draft.md`](../specs/meos-api-0.1-draft.md) — MEOS-API catalog, the machine-readable function registry that bindings consuming TemporalParquet files also use
- [`doc/rfc/temporal-data-lake/README.md`](../rfc/temporal-data-lake/README.md) — Temporal Data Lake architecture, of which TemporalParquet is the file-format substrate
- [GeoParquet specification](https://geoparquet.org/) — the spatial-Parquet standard this convention is modelled on
- [MobilityDuck](https://github.com/MobilityDB/MobilityDuck) — DuckDB extension; primary consumer of TemporalParquet on the read and write path
