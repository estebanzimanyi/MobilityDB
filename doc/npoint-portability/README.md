# RFC: npoint Portability — Stable Network Location Identifiers Across Map Versions

> **Discussion:** [MobilityDB/MobilityDB#863](https://github.com/MobilityDB/MobilityDB/discussions/863)

## The Problem

[npoint](https://docs.mobilitydb.com/MobilityDB/master/tnpoint.html) represents a location on
a road network as `(rid, pos)` where:

- `rid` is a **pgRouting edge ID** — a local sequential integer assigned during graph construction
  from an OSM dump
- `pos` is a fraction along that edge (0.0–1.0)

This means npoint data is **locked to a specific pgRouting graph instance**. Any of the
following breaks it:

| Event | Effect |
|---|---|
| Different OSM dump date | `rid` values reassigned |
| Different pgRouting build parameters | Edge splitting changes, `rid` reassigned |
| Different database / organisation | No shared edge-ID namespace |
| Map update after import | Existing `tnpoint` trajectories become unresolvable |

You cannot ship a `.csv` of `tnpoint` trajectories to a collaborator, store them in a data lake,
or query them after a map update — without a lossy coordinate fallback. This has always limited
the type's usability in production.

## Analogies with Existing Solutions

### The pgPointCloud analogy — partial fit

pgPointCloud solves a similar problem with `pcid`: every patch carries a foreign key into
`pointcloud_formats`, which defines what each byte dimension means.  Shipping data = data rows +
the relevant `pointcloud_formats` entries (a few hundred bytes per schema).

For npoint the analogous entity is not a schema definition but the **entire road network
geometry** (gigabytes of edge geometries).  The pcid pattern gives the right shape of solution
— an `nid` column pointing to a catalog table — but the catalog entry itself cannot be a
schema snippet.  It would have to carry the full edge geometry for every possible `rid`, making
the "ship catalog alongside data" approach GB-scale rather than KB-scale.

### The SRID analogy — closer, but the registry is missing

PostGIS geometries carry an SRID pointing into `spatial_ref_sys`.  CRS definitions are
internationally standardised (EPSG registry), so the same SRID means the same thing everywhere.

For npoint the equivalent would be a **network version ID** (`nwid`) pointing into a network
catalog.  The problem: unlike geographic CRS, road network versions are not globally standardised.
There is no EPSG-equivalent registry for "OSM Europe 2024-Q1 with pgRouting import scheme X".
But OSM itself provides exactly such a globally stable, publicly accessible reference — which
leads directly to the proposed solution.

---

## Three Approaches

### Option A — `nwid` catalog (pcid-style, local portability)

Add a `network_versions` table analogous to `pointcloud_formats`:

```sql
CREATE TABLE network_versions (
  nwid          INTEGER PRIMARY KEY,
  description   TEXT,
  osm_source    TEXT,           -- e.g. 'planet-250101.osm.pbf' or Overpass URL
  osm_timestamp TIMESTAMPTZ,
  build_params  JSONB           -- pgRouting import parameters
);
```

npoint becomes `(nwid, rid, pos)`.  The npoint WKB serialization includes `nwid` (analogous to
`pcid` in pgPointCloud WKB).  Shipping = data + `network_versions` rows + the relevant subset of
the pgRouting edge table.

**Pro:** zero schema change for existing users who stay within one organisation.  
**Con:** `rid` is still local.  The receiver needs the full edge table (potentially GB-scale) to
resolve coordinates.  Cross-organisation portability requires agreeing on a shared network catalog
— not realistic at scale.

---

### Option B — OSM way ID + fraction ← **proposed POC**

Replace `rid` (pgRouting edge ID) with `osm_way_id` (the
[OSM persistent identifier](https://wiki.openstreetmap.org/wiki/Way)) and store the position
as a fraction along the **full OSM way**, not just along a pgRouting edge fragment.

```
npoint_v2 = (osm_way_id BIGINT, pos FLOAT8)
```

OSM way IDs are:

| Property | Detail |
|---|---|
| Globally unique | Part of the OSM data model |
| Publicly resolvable | `https://api.openstreetmap.org/api/0.6/way/<id>` or any OSM mirror |
| Reasonably stable | Only renumbered on rare major splits / merges in OSM itself |
| Geometry-recoverable | Any recipient with an OSM dump or Overpass access can reconstruct coordinates |
| Historically pinnable | OSM history API gives the exact geometry at any past timestamp |

For full reproducibility, an optional `osm_timestamp TIMESTAMPTZ` pins the exact OSM version of
the way geometry via the
[OSM history API](https://wiki.openstreetmap.org/wiki/API_v0.6#History:_GET_/api/0.6/[node|way|relation]/#id/history).

#### Mapping current pgRouting-based npoint → Option B

osm2pgrouting stores the parent OSM way ID (`osm_id`) and the endpoint nodes for each pgRouting
edge.  Given the raw OSM way geometry, `ST_LineLocatePoint` recovers the fraction range
`[f_start, f_end]` that the pgRouting edge covers within the full way:

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

-- Step 2: migrate existing npoint(rid, pos) → npoint_v2(osm_way_id, pos_along_way)
SELECT
  f.osm_way_id,
  f.frac_start + n.pos * (f.frac_end - f.frac_start)  AS pos_along_way
FROM tnpoints n
JOIN edge_way_fractions f ON f.edge_id = n.rid;
```

#### Resolving Option B npoints → coordinates

The inverse requires **only the OSM way geometry** — no pgRouting graph needed:

```sql
-- Resolve (osm_way_id=234567890, pos=0.37) → point coordinates
SELECT ST_LineInterpolatePoint(w.the_geom, 0.37)
FROM osm_ways w
WHERE w.id = 234567890;
```

Any recipient with a local OSM mirror, an Overpass instance, or access to the public OSM API
can evaluate this query.  The pgRouting graph is not required.

#### Why this is the true structural equivalent of pcid

| pgPointCloud | npoint Option B |
|---|---|
| `pcid` references `pointcloud_formats` (schema) | `osm_way_id` references OSM dataset (geometry) |
| Schema is KB-scale, ships alongside data | OSM geometry is globally public, needs no shipping |
| `pcid` travels in the WKB binary | `osm_way_id` travels in the npoint binary |
| Receiver needs the schema rows | Receiver needs OSM access (universally available) |

The "catalog" for Option B is the OSM planet, already available everywhere.

---

### Option C — OpenLR encoding (cross-provider portability)

[OpenLR](https://www.openlr-association.com/) is an open standard (originally TomTom, now HERE,
TomTom, TfL, Waze, RWS) that encodes road locations as **geometry + attributes** — start/end
node coordinates, bearing, functional road class, form of way, positive/negative offset.  No
shared identifier is needed; any compatible map can decode it by finding the matching path.

OpenLR is self-contained in the way WKT is self-contained: the bytes carry all the information
needed to locate the point on any sufficiently-compatible road network.

**Pro:** true cross-provider portability; production-proven at scale by major navigation vendors.  
**Con:** lossy (tolerance ~5 m); requires an encoder/decoder library (open-source
[Java reference implementation](https://github.com/tomtom-international/openlr),
[Python port](https://github.com/openlr/openlr-python)); the encoded form is larger than a simple
`(osm_way_id, pos)` pair; better suited to interchange than primary storage.

A MEOS-side `npoint_to_openlr()` / `npoint_from_openlr()` pair could provide the interchange
surface while Option B (or the current scheme) is used for storage.

---

## Comparison

| | Option A | Option B | Option C |
|---|---|---|---|
| Analogous to | pcid | SRID (EPSG registry) | WKT (self-describing) |
| Portable within org | ✓ | ✓ | ✓ |
| Portable across orgs | ✗ | ✓ (requires OSM access) | ✓ |
| Portable across map providers | ✗ | ✗ (OSM-only) | ✓ |
| Schema change | Minimal (`nwid` column) | `rid` → `osm_way_id` | Serialization only |
| Catalog size to ship | Full edge table (GB) | Zero (OSM is public) | Zero (self-contained) |
| Lossless | ✓ | ✓ | ✗ (~5 m tolerance) |
| Requires OSM | ✓ (already assumed) | ✓ | ✗ |

Option B and Option A are not mutually exclusive: Option B's `osm_way_id` can coexist with an
Option A `nwid` column for organisations that need to pin an exact OSM snapshot timestamp while
still using globally resolvable identifiers.

---

## Open Questions

1. **npoint users**: what serialization format do you use today to exchange `tnpoint` data?
   Do you fall back to WGS84 coordinates at export time, or keep the `(rid, pos)` encoding?

2. **pgRouting community**: is there an existing convention for stable route identifiers across
   graph rebuilds?  osm2pgrouting and OSRM take different approaches — is there a de-facto
   standard?

3. **Maintainers**: should `osm_way_id` be the primary internal storage with pgRouting `rid` as
   a derived/cached field?  Or should both coexist behind an explicit `nwid` discriminator?

4. **OpenLR**: has anyone integrated OpenLR with MobilityDB or pgRouting?  A MEOS-side
   encoder/decoder seems feasible and would give Option B a cross-provider interchange path.

The goal is a `tnpoint` that you can serialize to a Parquet file today and still decode
correctly after the next OSM import — the same guarantee that `tgeompoint` already has via
WGS84 coordinates.

## Related

- [Discussion #863](https://github.com/MobilityDB/MobilityDB/discussions/863) — community discussion
- [RFC #830](https://github.com/MobilityDB/MobilityDB/issues/830) — TemporalParquet (interchange format that would carry npoint data)
- [MobilityDB npoint documentation](https://docs.mobilitydb.com/MobilityDB/master/tnpoint.html)
- [pgRouting](https://pgrouting.org/) — the routing library providing `rid` values today
- [osm2pgrouting](https://github.com/pgRouting/osm2pgrouting) — OSM → pgRouting import tool that stores `osm_id` alongside `rid`
- [OpenLR Association](https://www.openlr-association.com/) — open standard for network-portable location references
