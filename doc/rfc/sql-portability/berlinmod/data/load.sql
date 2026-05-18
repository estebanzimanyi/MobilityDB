-- Copyright(c) MobilityDB Contributors
-- This file is part of MobilityDB documentation.
-- Licensed under Creative Commons Attribution 4.0 International (CC BY 4.0).
--
-- Self-contained synthetic BerlinMOD reference dataset (5 trips, planar SRID 0).
-- Run after schema.sql on the MobilityDB/PostgreSQL reference engine:
--   psql -d <db> -f schema.sql -f data/load.sql
-- The values below are the canonical dataset definition; MobilityDuck and
-- MobilitySpark load the same logical dataset through their own loaders.
--
-- All trips are active 2020-01-01 00:00..00:10 UTC, SRID 0 (planar):
--   trip1  B-AA 100  (0,0)->(100,0)         passenger
--   trip2  B-BB 200  (0,5)->(100,5)         passenger
--   trip3  B-CC 300  (0,3)->(100,3)         truck
--   trip4  B-DD 400  (0,4)->(100,4)         truck      (1 unit from trip3)
--   trip5  B-EE 500  (0,1000)->(1000,1000)  passenger  (remote, 10x faster)

TRUNCATE Trips, Vehicles, QueryLicences, QueryInstants,
         QueryPoints, QueryRegions, QueryPeriods RESTART IDENTITY CASCADE;

INSERT INTO Vehicles (vehId, licence, type, model) VALUES
  (1, 'B-AA 100', 'passenger', 'Sedan'),
  (2, 'B-BB 200', 'passenger', 'Hatchback'),
  (3, 'B-CC 300', 'truck',     'Lorry'),
  (4, 'B-DD 400', 'truck',     'Lorry'),
  (5, 'B-EE 500', 'passenger', 'Sedan');

INSERT INTO Trips (tripId, vehId, trip) VALUES
  (1, 1, '[POINT(0 0)@2020-01-01 00:00:00+00, POINT(100 0)@2020-01-01 00:10:00+00]'),
  (2, 2, '[POINT(0 5)@2020-01-01 00:00:00+00, POINT(100 5)@2020-01-01 00:10:00+00]'),
  (3, 3, '[POINT(0 3)@2020-01-01 00:00:00+00, POINT(100 3)@2020-01-01 00:10:00+00]'),
  (4, 4, '[POINT(0 4)@2020-01-01 00:00:00+00, POINT(100 4)@2020-01-01 00:10:00+00]'),
  (5, 5, '[POINT(0 1000)@2020-01-01 00:00:00+00, POINT(1000 1000)@2020-01-01 00:10:00+00]');

INSERT INTO QueryLicences (licenceId, licence, vehId) VALUES
  (1, 'B-AA 100', 1),
  (2, 'B-CC 300', 3);

INSERT INTO QueryInstants (instantId, instant) VALUES
  (1, '2020-01-01 00:05:00+00');

INSERT INTO QueryPoints (pointId, geom, geomWKT) VALUES
  (1, ST_GeomFromText('POINT(50 0)', 0), 'POINT(50 0)'),
  (2, ST_GeomFromText('POINT(50 5)', 0), 'POINT(50 5)');

INSERT INTO QueryRegions (regionId, geom) VALUES
  (1, ST_GeomFromText('POLYGON((40 -1, 60 -1, 60 6, 40 6, 40 -1))', 0));

INSERT INTO QueryPeriods (periodId, period) VALUES
  (1, '[2020-01-01 00:02:00+00, 2020-01-01 00:08:00+00]');
