-- Copyright(c) MobilityDB Contributors
-- This file is part of MobilityDB documentation.
-- Licensed under Creative Commons Attribution 4.0 International (CC BY 4.0).
--
-- BerlinMOD Q9: What is the longest distance travelled by a vehicle during
-- each of the periods from QueryPeriods?
--
-- Portable: works unchanged on MobilityDB/PostgreSQL, MobilityDuck/DuckDB,
-- and MobilitySpark/Spark SQL.
--
-- Temporal operations used:
--   atTime(tgeompoint, tstzspan) → tgeompoint   (restrict to period)
--   length(tgeompoint) → float8                  (Euclidean path length)

WITH Distances AS (
  SELECT p.periodId, p.period, t.vehId,
         SUM(length(atTime(t.trip, p.period))) AS dist
  FROM   Trips t, QueryPeriods p
  WHERE  overlaps(timeSpan(t.trip), p.period)
  GROUP  BY p.periodId, p.period, t.vehId
)
SELECT periodId, period, ROUND(MAX(dist)::numeric, 3) AS maxDist
FROM   Distances
GROUP  BY periodId, period
ORDER  BY periodId;
