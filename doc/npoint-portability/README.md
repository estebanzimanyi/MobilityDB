# RFC: npoint Portability — Stable Network Location Identifiers Across Map Versions

> Discussion: [#863](https://github.com/MobilityDB/MobilityDB/discussions/863)

## The Problem

[npoint](https://docs.mobilitydb.com/MobilityDB/master/tnpoint.html) represents a location on a road network as `(rid, pos)` where:

- `rid` is a pgRouting edge ID, a local sequential integer assigned during graph construction from an OSM dump
- `pos` is a fraction along that edge (0.0 to 1.0)

npoint data is locked to a specific pgRouting graph instance. Any of the following breaks it:

| Event | Effect |
|---|---|
| Different OSM dump date | `rid` values reassigned |
| Different pgRouting build parameters | Edge splitting changes, `rid` reassigned |
| Different database or organisation | No shared edge-ID namespace |
| Map update after import | Existing `tnpoint` trajectories become unresolvable |

A `.csv` of `tnpoint` trajectories cannot be shipped to a collaborator, stored in a data lake, or queried after a map update without a lossy coordinate fallback.

## Proposal

Replace `rid` (pgRouting edge ID) with `osm_way_id` (the [OSM persistent identifier](https://wiki.openstreetmap.org/wiki/Way)) and store the position as a fraction along the full OSM way, not just along a pgRouting edge fragment.

```
npoint = (osm_way_id BIGINT, pos FLOAT8)
```

OSM way IDs are:

| Property | Detail |
|---|---|
| Globally unique | Part of the OSM data model |
| Publicly resolvable | `https://api.openstreetmap.org/api/0.6/way/<id>` or any OSM mirror |
| Reasonably stable | Renumbered only on rare major splits / merges in OSM |
| Geometry-recoverable | Any recipient with an OSM dump or Overpass access reconstructs coordinates |
| Historically pinnable | OSM history API gives the exact geometry at any past timestamp |

For full reproducibility, an optional `osm_timestamp TIMESTAMPTZ` pins the exact OSM version of the way geometry via the [OSM history API](https://wiki.openstreetmap.org/wiki/API_v0.6#History:_GET_/api/0.6/[node|way|relation]/#id/history).

### Mapping pgRouting-based npoint to the new scheme

osm2pgrouting stores the parent OSM way ID (`osm_id`) and the endpoint nodes for each pgRouting edge. Given the raw OSM way geometry, `ST_LineLocatePoint` recovers the fraction range `[f_start, f_end]` that the pgRouting edge covers within the full way:

```sql
-- Step 1: build a fraction index for each pgRouting edge (one-time, per network import)
CREATE TABLE edge_way_fractions AS
SELECT
  e.id                                                        AS edge_id,
  e.osm_id                                                    AS osm_way_id,
  ST_LineLocatePoint(w.the_geom, ST_StartPoint(e.the_geom))   AS frac_start,
  ST_LineLocatePoint(w.the_geom, ST_EndPoint(e.the_geom))     AS frac_end
FROM ways e                           -- pgRouting edge table (osm2pgrouting output)
JOIN osm_ways w ON w.id = e.osm_id;  -- raw OSM way geometry before splitting

-- Step 2: migrate npoint(rid, pos) -> npoint(osm_way_id, pos_along_way)
SELECT
  f.osm_way_id,
  f.frac_start + n.pos * (f.frac_end - f.frac_start)  AS pos_along_way
FROM tnpoints n
JOIN edge_way_fractions f ON f.edge_id = n.rid;
```

### Resolving npoints to coordinates

The inverse requires only the OSM way geometry; no pgRouting graph is needed:

```sql
-- Resolve (osm_way_id=234567890, pos=0.37) to point coordinates
SELECT ST_LineInterpolatePoint(w.the_geom, 0.37)
FROM osm_ways w
WHERE w.id = 234567890;
```

Any recipient with a local OSM mirror, an Overpass instance, or access to the public OSM API can evaluate this query.

### Structural equivalence to pcid

| pgPointCloud | npoint |
|---|---|
| `pcid` references `pointcloud_formats` (schema) | `osm_way_id` references OSM dataset (geometry) |
| Schema is KB-scale, ships alongside data | OSM geometry is globally public, needs no shipping |
| `pcid` travels in the WKB binary | `osm_way_id` travels in the npoint binary |
| Receiver needs the schema rows | Receiver needs OSM access (universally available) |

The "catalog" is the OSM planet, already available everywhere.

### OpenLR interchange surface

[OpenLR](https://www.openlr-association.com/) is an open standard for cross-provider network location references that encodes road locations as geometry plus attributes (start/end node coordinates, bearing, functional road class, form of way, positive/negative offset). MEOS provides `npoint_to_openlr()` and `npoint_from_openlr()` for interchange with non-OSM providers. Storage uses `(osm_way_id, pos)`; OpenLR is the export format when shipping to systems that target other map providers.

## Related

- [Discussion #863](https://github.com/MobilityDB/MobilityDB/discussions/863)
- [RFC #830](https://github.com/MobilityDB/MobilityDB/issues/830) — TemporalParquet (interchange format that carries npoint data)
- [MobilityDB npoint documentation](https://docs.mobilitydb.com/MobilityDB/master/tnpoint.html)
- [pgRouting](https://pgrouting.org/)
- [osm2pgrouting](https://github.com/pgRouting/osm2pgrouting)
- [OpenLR Association](https://www.openlr-association.com/)
