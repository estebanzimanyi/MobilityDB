-- Copyright(c) MobilityDB Contributors
-- This file is part of MobilityDB documentation.
-- Licensed under Creative Commons Attribution 4.0 International (CC BY 4.0).
--
-- Portable named-function aliases for MobilityDB.
--
-- MobilityDuck and MobilitySpark expose the bare name `overlaps` for the
-- `&&` bounding-box operator. MobilityDB backs `&&` with the prefixed
-- functions `span_overlaps` / `temporal_overlaps` and does not register the
-- bare `overlaps` name. These wrappers add exactly the `overlaps` overloads
-- the BerlinMOD portable suite uses, so the same `.sql` runs unchanged on all
-- three engines.
--
-- Scope: benchmark-closing minimal set. The full operator -> bare-name alias
-- layer (overlaps/contains/before/left/... across every type overload) is the
-- tracked follow-up for MobilityDB core. DuckDB and Spark already expose these
-- names natively and do not load this file.
--
-- Apply on the MobilityDB/PostgreSQL reference engine after schema.sql:
--   psql -d <db> -f schema.sql -f portable_aliases.sql -f data/load.sql

CREATE OR REPLACE FUNCTION overlaps(tstzspan, tstzspan)
  RETURNS boolean
  AS $$ SELECT public.span_overlaps($1, $2) $$
  LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION overlaps(tgeompoint, stbox)
  RETURNS boolean
  AS $$ SELECT public.temporal_overlaps($1, $2) $$
  LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION overlaps(stbox, tgeompoint)
  RETURNS boolean
  AS $$ SELECT public.temporal_overlaps($1, $2) $$
  LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE;
