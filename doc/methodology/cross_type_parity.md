<!--
Copyright(c) MobilityDB Contributors

This documentation is licensed under a
Creative Commons Attribution-Share Alike 3.0 License
https://creativecommons.org/licenses/by-sa/3.0/
-->

# Cross-Type Parity in MobilityDB

## Two Kinds of Parity in the Ecosystem

The MobilityDB ecosystem tracks two orthogonal parity dimensions:

| Parity dimension | What it means |
|---|---|
| **Cross-platform** | Same SQL query runs unchanged on MobilityDB, MobilityDuck, and MobilitySpark |
| **Cross-type** | A function defined for one temporal spatial type is also defined for the others wherever the operation makes semantic sense |

Both dimensions are measured against the same underlying MEOS C library. Cross-platform parity ensures portability. Cross-type parity ensures consistency: if `eIntersects(tgeompoint, geometry)` exists, then `eIntersects(tnpoint, geometry)`, `eIntersects(tcbuffer, geometry)`, and `eIntersects(trgeometry, geometry)` should also exist with the same name, signature shape, and analogous semantics, so the same query pattern works regardless of which temporal type is being queried.

## Type Families

Each temporal spatial type's parity surface is determined by what it carries at each instant:

| Type | Per-instant payload | Family |
|---|---|---|
| `tgeompoint` | A 0-D Euclidean point | Point |
| `tgeogpoint` | A 0-D geodetic point on WGS84 | Point |
| `tnpoint` | A network point (route id + fraction along the edge) | Point |
| `tgeometry` | An arbitrary geometry (point, line, polygon, multi-geom) | Extended-shape |
| `trgeometry` | A pose plus a reference geometry held rigid | Extended-shape |
| `tcbuffer` | A circle parameterised by center and radius | Extended-shape |
| `tpose` | A frame (position plus orientation), no shape | Extended-shape |

The **Point** family uses `tgeompoint` as its reference: any function defined on `tgeompoint` is expected on every other point-payload type unless its semantics break under the constrained representation.

The **Extended-shape** family uses `tgeometry` as its reference: any function defined on `tgeometry` is expected on every other extended-payload type unless the operation would bypass that type's internal invariants (the rigid-body pose for trgeometry, the center+radius parameterisation for tcbuffer, the shape-less frame for tpose).

## Function Categories

Each temporal spatial type provides the following categories of operations wherever the category applies to its payload:

1. Constructors, casts, accessors
2. Input / output
3. Comparison operators
4. Spatial functions
5. Tile functions (spaceSplit, spaceTimeSplit)
6. Box operations (expandSpace, spans, stboxes, splitN)
7. Position operators (`<<`, `>>`, `&<`)
8. Distance (tdistance, nearestApproachDistance, shortestLine)
9. Trajectory similarity (Frechet, DTW)
10. Aggregate functions (tcount, wcount, extent, merge, appendInstant)
11. Ever and always spatial relationships (eIntersects, eContains, eDwithin)
12. Temporal spatial relationships (tIntersects, tContains, tDwithin)
13. Index support (GiST, SP-GiST operator classes)
14. Analytics (simplify variants)

Category 4 (spatial functions) carries different content per family. On the extended-shape family it includes `traversedArea`, `centroid`, `convexHull`, `atGeometry`, and `minusGeometry`. On the point family it reduces to `trajectory`, `length`, `cumulativeLength`, and `azimuth`. Category 9 (trajectory similarity) applies only to point trajectories.

`tpose` carries no shape, so the category 4 functions that depend on a swept footprint (`traversedArea`, `convexHull`, `centroid`) reduce to position-only computations or are absent. `atGeometry` and `minusGeometry` against an external geometry test the position alone.

## Intentional Exclusions

The following functions are deliberately absent from the listed types and are **not gaps to fix** — they are *correct absences*. Some are formally meaningless for the type; others would be meaningful but cannot be supported because of a representation or underlying-library (PostGIS) limitation. Either way each one must stay explicitly marked, with its reason, so no future contributor "implements" a non-operation to chase a parity number.

**No affine transforms for structured types.** `affine`, `rotate`, `rotateX/Y/Z`, `translate`, `transscale`, `scale` are PostGIS compatibility wrappers that modify the geometry at each instant. For types with structured internal geometry, trgeometry (pose + reference geometry), tcbuffer (center + radius), tnpoint (route + fraction), raw affine transforms would bypass the type invariants. The correct way to move a rigid body is to modify the pose sequence; the correct way to resize a buffer is to modify the radius; network points have no affine action.

**No trajectory similarity outside the point family.** `frechetDistance` and `dynamicTimeWarp` are defined on point trajectories. Extended-shape types have no canonical pointwise representation against which a similarity metric is well-defined.

**No `atGeometry` / `minusGeometry` for tnpoint.** A network point is constrained to a 1-D network edge; use route filtering instead.

**No `convexHull` / `centroid` on a point trajectory's continuous form.** They collapse to the trajectory or its bounding box; use `trajectory()` and `stbox()` instead.

**No swept-shape functions on tpose.** `traversedArea`, `convexHull`, and `centroid` are absent on tpose because a pose carries no shape to sweep.

**No `routeops` outside tnpoint.** Route-membership operators (`@>`, `<@` over `nset`) are specific to the network-point representation.

**No geodetic `tcbuffer`, and no geography twin for structured types.** No `trgeography`, `tcbuffergeography`, or `tposegeography` exists. In particular, `tcbuffer` is **planar-2D only**: all PostGIS operations on circular segments are defined for planar 2D only, so a geodetic circular buffer is structurally impossible until PostGIS gains geodetic circular-segment support. The network-point representation is network-Euclidean by construction.

**No linear interpolation for `tgeometry` / `tgeography`.** These types support only the **discrete** and **step** interpolations, never **linear** — otherwise there would need to be a "morphing" function that interpolates between two arbitrary geometries at two timestamps, which does not exist. Types whose per-instant payload is parameterised do interpolate linearly: `tcbuffer` via center + radius, `trgeometry` / `tpose` via the pose (position lerp + orientation slerp), and the point types via the coordinate.

**No touches / contains / covers predicates on geodetic types.** `eTouches`, `eContains`, `eCovers` (and their always-`a*` and temporal-output `t*` forms) are computed through the GEOS DE-9IM relate matrix, which is planar-only, and PostGIS geography exposes no `ST_Touches`, `ST_Contains`, or `ST_Relate`. They are therefore absent on `tgeogpoint` (and on geodetic geometry generally). `eIntersects`, `eDwithin`, and `eDisjoint` *do* work geodetically (via `ST_Intersects` / `ST_DWithin` / `ST_Distance`) — only the DE-9IM relate-based predicates are excluded.

**tnpoint supports SRID; only per-value CRS change is excluded.** A network point fully supports SRID for reading and I/O — `srid(tnpoint)`, `asEWKT`, `asEWKB`, and `asHexEWKB` retain and output its SRID (e.g. `npoint 'SRID=5676;Npoint(1,0.5)'` round-trips as `SRID=5676;NPoint(1,0.5)`). What is absent is the per-value CRS *mutation*: `transform`, `transform_gk`, `transformPipeline` (reproject) and `setSRID` (relabel). A network point inherits its CRS from the underlying road network (the `ways` table, read via `get_srid_ways()`), so the SRID is a property of the network, not of an individual value; reprojection is performed on the network, not per tnpoint.

**No `tprecision` on tnpoint.** `tprecision` replaces each time bin with the time-weighted average of its values; the average of network points is not network-valid (it would leave the edge). `tsample`, which only resamples positions that remain on the network, is provided instead.

**No planar position or planar space tiling on tgeogpoint.** The relative-position operators (`<<`, `>>`, `&<`, `<<|`, … and their bare-name aliases `left`/`right`/`above`/`below`/`front`/`back` and the `over*` variants), the fixed planar-grid space tiling (`spaceTiles`/`spaceBoxes`/`spaceSplit` and the space-time combinations), `asMVTGeom`, and the Gauss-Krüger projection (`transform_gk`) are undefined on the sphere. Geodetic space-binning uses the H3 family (`geoToH3Cell`, `h3_latlng_to_cell`); generic reprojection uses `transform()`/`transformPipeline()`.

**No spherical centroid aggregate on tgeogpoint.** `tcentroid` and `twcentroid` compute a planar coordinate average; a geodetic centroid is not that average and no spherical equivalent is defined.

**No Z-axis position on the strictly-2D types.** tnpoint and tcbuffer carry no Z dimension, so the front/back position operators (`<</`, `/>>`, `&</`, `/&>`, the `temporal_front`/`temporal_back`/`temporal_overfront`/`temporal_overback` operators and their bare aliases) are absent, as are `atElevation` and `minusElevation` on tnpoint.

**No `makeSimple` or H3 lat/lng mapping on tnpoint.** `makeSimple` removes self-intersections of a free trajectory and `h3_latlng_to_cell` maps a free geographic point to an H3 cell; a network point follows network edges and is addressed by route plus fraction.

## Relationship to Cross-Platform Parity

Once a function exists for a given type in MobilityDB (cross-type parity achieved), it becomes a candidate for the cross-platform parity registry. The MEOS C library is the source of truth; bindings (MobilityDuck, MobilitySpark via JMEOS) are generated from `meos-api.json`. In practice:

1. A new function is added to MEOS (`meos/src/`) and exposed via `meos.h`.
2. The MobilityDB PostgreSQL extension registers a SQL wrapper.
3. `meos-api.json` is updated to reflect the new symbol.
4. MobilityDuck and MobilitySpark pick it up in their next sync against `meos-api.json`.

This pipeline means cross-type parity work in MobilityDB propagates automatically to all three platforms: it is the upstream that drives cross-platform parity downstream.
