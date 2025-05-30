-------------------------------------------------------------------------------
--
-- This MobilityDB code is provided under The PostgreSQL License.
-- Copyright (c) 2016-2025, Université libre de Bruxelles and MobilityDB
-- contributors
--
-- MobilityDB includes portions of PostGIS version 3 source code released
-- under the GNU General Public License (GPLv2 or later).
-- Copyright (c) 2001-2025, PostGIS contributors
--
-- Permission to use, copy, modify, and distribute this software and its
-- documentation for any purpose, without fee, and without a written
-- agreement is hereby granted, provided that the above copyright notice and
-- this paragraph and the following two paragraphs appear in all copies.
--
-- IN NO EVENT SHALL UNIVERSITE LIBRE DE BRUXELLES BE LIABLE TO ANY PARTY FOR
-- DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING
-- LOST PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION,
-- EVEN IF UNIVERSITE LIBRE DE BRUXELLES HAS BEEN ADVISED OF THE POSSIBILITY
-- OF SUCH DAMAGE.
--
-- UNIVERSITE LIBRE DE BRUXELLES SPECIFICALLY DISCLAIMS ANY WARRANTIES,
-- INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
-- AND FITNESS FOR A PARTICULAR PURPOSE. THE SOFTWARE PROVIDED HEREUNDER IS ON
-- AN "AS IS" BASIS, AND UNIVERSITE LIBRE DE BRUXELLES HAS NO OBLIGATIONS TO
-- PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
--
-------------------------------------------------------------------------------

-------------------------------------------------------------------------------
-- Test all operators without having collected statistics
-------------------------------------------------------------------------------

SELECT COUNT(*) FROM tbl_tstzset WHERE t = tstzset '{2001-06-01, 2001-07-07}';
SELECT COUNT(*) FROM tbl_tstzset WHERE t <> tstzset '{2001-06-01, 2001-07-07}';
SELECT COUNT(*) FROM tbl_tstzset WHERE t < tstzset '{2001-06-01, 2001-07-07}';
SELECT COUNT(*) FROM tbl_tstzset WHERE t <= tstzset '{2001-06-01, 2001-07-07}';
SELECT COUNT(*) FROM tbl_tstzset WHERE t > tstzset '{2001-06-01, 2001-07-07}';
SELECT COUNT(*) FROM tbl_tstzset WHERE t >= tstzset '{2001-06-01, 2001-07-07}';

SELECT COUNT(*) FROM tbl_tstzspan WHERE t = tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t <> tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t < tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t <= tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t > tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t >= tstzspan '[2001-06-01, 2001-07-01]';

SELECT COUNT(*) FROM tbl_tstzspanset WHERE t = tstzspanset '{[2001-06-01, 2001-07-01]}';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t <> tstzspanset '{[2001-06-01, 2001-07-01]}';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t < tstzspanset '{[2001-06-01, 2001-07-01]}';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t <= tstzspanset '{[2001-06-01, 2001-07-01]}';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t > tstzspanset '{[2001-06-01, 2001-07-01]}';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t >= tstzspanset '{[2001-06-01, 2001-07-01]}';

SELECT COUNT(*) FROM tbl_tstzset WHERE t @> timestamptz '2001-06-01';
SELECT COUNT(*) FROM tbl_tstzset WHERE t @> tstzset '{2001-06-01, 2001-07-07}';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t @> timestamptz '2001-06-01';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t @> tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t @> tstzspanset '{[2001-06-01, 2001-07-01]}';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t @> timestamptz '2001-06-01';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t @> tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t @> tstzspanset '{[2001-06-01, 2001-07-01]}';

SELECT COUNT(*) FROM tbl_timestamptz WHERE t <@ tstzset '{2001-06-01, 2001-07-07}';
SELECT COUNT(*) FROM tbl_timestamptz WHERE t <@ tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_timestamptz WHERE t <@ tstzspanset '{[2001-06-01, 2001-07-01]}';
SELECT COUNT(*) FROM tbl_tstzset WHERE t <@ tstzset '{2001-06-01, 2001-07-07}';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t <@ tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t <@ tstzspanset '{[2001-06-01, 2001-07-01]}';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t <@ tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t <@ tstzspanset '{[2001-06-01, 2001-07-01]}';

SELECT COUNT(*) FROM tbl_tstzset WHERE t && tstzset '{2001-06-01, 2001-07-07}';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t && tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t && tstzspanset '{[2001-06-01, 2001-07-01]}';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t && tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t && tstzspanset '{[2001-06-01, 2001-07-01]}';

SELECT COUNT(*) FROM tbl_timestamptz WHERE t <<# tstzset '{2001-06-01, 2001-07-07}';
SELECT COUNT(*) FROM tbl_timestamptz WHERE t <<# tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_timestamptz WHERE t <<# tstzspanset '{[2001-06-01, 2001-07-01]}';
SELECT COUNT(*) FROM tbl_tstzset WHERE t <<# timestamptz '2001-06-01';
SELECT COUNT(*) FROM tbl_tstzset WHERE t <<# tstzset '{2001-06-01, 2001-07-07}';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t <<# timestamptz '2001-06-01';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t <<# tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t <<# tstzspanset '{[2001-06-01, 2001-07-01]}';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t <<# timestamptz '2001-06-01';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t <<# tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t <<# tstzspanset '{[2001-06-01, 2001-07-01]}';

SELECT COUNT(*) FROM tbl_timestamptz WHERE t #>> tstzset '{2001-06-01, 2001-07-07}';
SELECT COUNT(*) FROM tbl_timestamptz WHERE t #>> tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_timestamptz WHERE t #>> tstzspanset '{[2001-06-01, 2001-07-01]}';
SELECT COUNT(*) FROM tbl_tstzset WHERE t #>> timestamptz '2001-06-01';
SELECT COUNT(*) FROM tbl_tstzset WHERE t #>> tstzset '{2001-06-01, 2001-07-07}';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t #>> timestamptz '2001-06-01';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t #>> tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t #>> tstzspanset '{[2001-06-01, 2001-07-01]}';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t #>> timestamptz '2001-06-01';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t #>> tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t #>> tstzspanset '{[2001-06-01, 2001-07-01]}';

SELECT COUNT(*) FROM tbl_timestamptz WHERE t &<# tstzset '{2001-06-01, 2001-07-07}';
SELECT COUNT(*) FROM tbl_timestamptz WHERE t &<# tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_timestamptz WHERE t &<# tstzspanset '{[2001-06-01, 2001-07-01]}';
SELECT COUNT(*) FROM tbl_tstzset WHERE t &<# timestamptz '2001-06-01';
SELECT COUNT(*) FROM tbl_tstzset WHERE t &<# tstzset '{2001-06-01, 2001-07-07}';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t &<# timestamptz '2001-06-01';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t &<# tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t &<# tstzspanset '{[2001-06-01, 2001-07-01]}';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t &<# timestamptz '2001-06-01';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t &<# tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t &<# tstzspanset '{[2001-06-01, 2001-07-01]}';

SELECT COUNT(*) FROM tbl_timestamptz WHERE t #&> tstzset '{2001-06-01, 2001-07-07}';
SELECT COUNT(*) FROM tbl_timestamptz WHERE t #&> tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_timestamptz WHERE t #&> tstzspanset '{[2001-06-01, 2001-07-01]}';
SELECT COUNT(*) FROM tbl_tstzset WHERE t #&> timestamptz '2001-06-01';
SELECT COUNT(*) FROM tbl_tstzset WHERE t #&> tstzset '{2001-06-01, 2001-07-07}';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t #&> timestamptz '2001-06-01';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t #&> tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t #&> tstzspanset '{[2001-06-01, 2001-07-01]}';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t #&> timestamptz '2001-06-01';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t #&> tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t #&> tstzspanset '{[2001-06-01, 2001-07-01]}';

SELECT COUNT(*) FROM tbl_timestamptz WHERE t -|- tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_timestamptz WHERE t -|- tstzspanset '{[2001-06-01, 2001-07-01]}';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t -|- timestamptz '2001-06-01';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t -|- tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t -|- tstzspanset '{[2001-06-01, 2001-07-01]}';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t -|- timestamptz '2001-06-01';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t -|- tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t -|- tstzspanset '{[2001-06-01, 2001-07-01]}';

-- Test the commutator
SELECT COUNT(*) FROM tbl_tstzspan WHERE tstzspan '[2001-01-01, 2001-06-01]' <<# t;
SELECT COUNT(*) FROM tbl_tstzspan WHERE tstzspan '[2001-01-01, 2001-06-01]' &<# t;

-------------------------------------------------------------------------------

analyze tbl_tstzspan;
analyze tbl_tstzspanset;
analyze tbl_tstzset;

-------------------------------------------------------------------------------
-- Test all operators after having collected statistics
-------------------------------------------------------------------------------

SELECT COUNT(*) FROM tbl_tstzset WHERE t = tstzset '{2001-06-01, 2001-07-07}';
SELECT COUNT(*) FROM tbl_tstzset WHERE t <> tstzset '{2001-06-01, 2001-07-07}';
SELECT COUNT(*) FROM tbl_tstzset WHERE t < tstzset '{2001-06-01, 2001-07-07}';
SELECT COUNT(*) FROM tbl_tstzset WHERE t <= tstzset '{2001-06-01, 2001-07-07}';
SELECT COUNT(*) FROM tbl_tstzset WHERE t > tstzset '{2001-06-01, 2001-07-07}';
SELECT COUNT(*) FROM tbl_tstzset WHERE t >= tstzset '{2001-06-01, 2001-07-07}';

SELECT COUNT(*) FROM tbl_tstzspan WHERE t = tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t <> tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t < tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t <= tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t > tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t >= tstzspan '[2001-06-01, 2001-07-01]';

SELECT COUNT(*) FROM tbl_tstzspanset WHERE t = tstzspanset '{[2001-06-01, 2001-07-01]}';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t <> tstzspanset '{[2001-06-01, 2001-07-01]}';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t < tstzspanset '{[2001-06-01, 2001-07-01]}';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t <= tstzspanset '{[2001-06-01, 2001-07-01]}';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t > tstzspanset '{[2001-06-01, 2001-07-01]}';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t >= tstzspanset '{[2001-06-01, 2001-07-01]}';

SELECT COUNT(*) FROM tbl_tstzset WHERE t @> timestamptz '2001-06-01';
SELECT COUNT(*) FROM tbl_tstzset WHERE t @> tstzset '{2001-06-01, 2001-07-07}';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t @> timestamptz '2001-06-01';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t @> tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t @> tstzspanset '{[2001-06-01, 2001-07-01]}';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t @> timestamptz '2001-06-01';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t @> tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t @> tstzspanset '{[2001-06-01, 2001-07-01]}';

SELECT COUNT(*) FROM tbl_timestamptz WHERE t <@ tstzset '{2001-06-01, 2001-07-07}';
SELECT COUNT(*) FROM tbl_timestamptz WHERE t <@ tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_timestamptz WHERE t <@ tstzspanset '{[2001-06-01, 2001-07-01]}';
SELECT COUNT(*) FROM tbl_tstzset WHERE t <@ tstzset '{2001-06-01, 2001-07-07}';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t <@ tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t <@ tstzspanset '{[2001-06-01, 2001-07-01]}';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t <@ tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t <@ tstzspanset '{[2001-06-01, 2001-07-01]}';

SELECT COUNT(*) FROM tbl_tstzset WHERE t && tstzset '{2001-06-01, 2001-07-07}';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t && tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t && tstzspanset '{[2001-06-01, 2001-07-01]}';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t && tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t && tstzspanset '{[2001-06-01, 2001-07-01]}';

SELECT COUNT(*) FROM tbl_timestamptz WHERE t <<# tstzset '{2001-06-01, 2001-07-07}';
SELECT COUNT(*) FROM tbl_timestamptz WHERE t <<# tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_timestamptz WHERE t <<# tstzspanset '{[2001-06-01, 2001-07-01]}';
SELECT COUNT(*) FROM tbl_tstzset WHERE t <<# timestamptz '2001-06-01';
SELECT COUNT(*) FROM tbl_tstzset WHERE t <<# tstzset '{2001-06-01, 2001-07-07}';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t <<# timestamptz '2001-06-01';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t <<# tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t <<# tstzspanset '{[2001-06-01, 2001-07-01]}';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t <<# timestamptz '2001-06-01';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t <<# tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t <<# tstzspanset '{[2001-06-01, 2001-07-01]}';

SELECT COUNT(*) FROM tbl_timestamptz WHERE t #>> tstzset '{2001-06-01, 2001-07-07}';
SELECT COUNT(*) FROM tbl_timestamptz WHERE t #>> tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_timestamptz WHERE t #>> tstzspanset '{[2001-06-01, 2001-07-01]}';
SELECT COUNT(*) FROM tbl_tstzset WHERE t #>> timestamptz '2001-06-01';
SELECT COUNT(*) FROM tbl_tstzset WHERE t #>> tstzset '{2001-06-01, 2001-07-07}';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t #>> timestamptz '2001-06-01';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t #>> tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t #>> tstzspanset '{[2001-06-01, 2001-07-01]}';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t #>> timestamptz '2001-06-01';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t #>> tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t #>> tstzspanset '{[2001-06-01, 2001-07-01]}';

SELECT COUNT(*) FROM tbl_timestamptz WHERE t &<# tstzset '{2001-06-01, 2001-07-07}';
SELECT COUNT(*) FROM tbl_timestamptz WHERE t &<# tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_timestamptz WHERE t &<# tstzspanset '{[2001-06-01, 2001-07-01]}';
SELECT COUNT(*) FROM tbl_tstzset WHERE t &<# timestamptz '2001-06-01';
SELECT COUNT(*) FROM tbl_tstzset WHERE t &<# tstzset '{2001-06-01, 2001-07-07}';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t &<# timestamptz '2001-06-01';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t &<# tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t &<# tstzspanset '{[2001-06-01, 2001-07-01]}';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t &<# timestamptz '2001-06-01';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t &<# tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t &<# tstzspanset '{[2001-06-01, 2001-07-01]}';

SELECT COUNT(*) FROM tbl_timestamptz WHERE t #&> tstzset '{2001-06-01, 2001-07-07}';
SELECT COUNT(*) FROM tbl_timestamptz WHERE t #&> tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_timestamptz WHERE t #&> tstzspanset '{[2001-06-01, 2001-07-01]}';
SELECT COUNT(*) FROM tbl_tstzset WHERE t #&> timestamptz '2001-06-01';
SELECT COUNT(*) FROM tbl_tstzset WHERE t #&> tstzset '{2001-06-01, 2001-07-07}';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t #&> timestamptz '2001-06-01';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t #&> tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t #&> tstzspanset '{[2001-06-01, 2001-07-01]}';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t #&> timestamptz '2001-06-01';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t #&> tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t #&> tstzspanset '{[2001-06-01, 2001-07-01]}';

SELECT COUNT(*) FROM tbl_timestamptz WHERE t -|- tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_timestamptz WHERE t -|- tstzspanset '{[2001-06-01, 2001-07-01]}';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t -|- timestamptz '2001-06-01';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t -|- tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_tstzspan WHERE t -|- tstzspanset '{[2001-06-01, 2001-07-01]}';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t -|- timestamptz '2001-06-01';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t -|- tstzspan '[2001-06-01, 2001-07-01]';
SELECT COUNT(*) FROM tbl_tstzspanset WHERE t -|- tstzspanset '{[2001-06-01, 2001-07-01]}';

-- Test the commutator
SELECT COUNT(*) FROM tbl_tstzspan WHERE tstzspan '[2001-01-01, 2001-06-01]' <<# t;
SELECT COUNT(*) FROM tbl_tstzspan WHERE tstzspan '[2001-01-01, 2001-06-01]' &<# t;

-------------------------------------------------------------------------------

--SELECT tstzspan_statistics_validate();
--VACUUM ANALYSE tbl_tstzspan;
--VACUUM ANALYSE tbl_tstzspanset;
--VACUUM ANALYSE tbl_tstzset;
--SELECT COUNT(*) FROM execution_stats WHERE abs(PlanRows-ActualRows) > 10
-- STATISTICS COLLECTION FUNCTIONS

CREATE OR REPLACE FUNCTION tstzspan_statistics_validate()
RETURNS CHAR(10) AS $$
DECLARE
  Query CHAR(5);
  PlanRows BIGINT;
  ActualRows BIGINT;
  QFilter VARCHAR;
  RowsRemovedbyFilter BIGINT;
  J JSON;
  StartTime TIMESTAMP;
  RandTimestamp TIMESTAMPTZ;
  RandPeriod tstzspan;
  RandTstzSet tstzset;
  RandPeriodset tstzspanset;
  k INT;
BEGIN

CREATE TABLE IF NOT EXISTS execution_stats
(Query CHAR(5),
StartTime TIMESTAMP,
QFilter VARCHAR,
PlanRows BIGINT,
ActualRows BIGINT,
RowsRemovedByFilter BIGINT,
J JSON);

TRUNCATE TABLE execution_stats;

SET log_error_verbosity TO terse;
k:= 0;

-----------------------------------------------
---- OPERATOR @>-------------------------------
-----------------------------------------------

k:= k+1;
FOR i IN 1..100 LOOP
  RandTimeStamp:= random_timestamptz('2000-10-01', '2002-1-31');
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspan
  WHERE t @> RandTimeStamp
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandTstzSet:= random_tstzset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspan
  WHERE t @> RandTstzSet
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriod:= random_tstzspan('2000-10-01', '2002-1-31', 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspan
  WHERE t @> RandPeriod
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriod:= random_tstzspan('2000-10-01', '2002-1-31', 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspan
  WHERE RandPeriod @> t
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriodset:= random_tstzspanset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspan
  WHERE t @> RandPeriodset
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriodset:= random_tstzspanset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspan
  WHERE RandPeriodset @> t
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

----------------------

k:= k+1;
FOR i IN 1..100 LOOP
  RandTimeStamp:= random_timestamptz('2000-10-01', '2002-1-31');
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzset
  WHERE t @> RandTimeStamp
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandTstzSet:= random_tstzset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzset
  WHERE t @> RandTstzSet
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

-----------------------------

k:= k+1;
FOR i IN 1..100 LOOP
  RandTimeStamp:= random_timestamptz('2000-10-01', '2002-1-31');
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspanset
  WHERE t @> RandTimeStamp
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandTstzSet:= random_tstzset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspanset
  WHERE t @> RandTstzSet
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriod:= random_tstzspan('2000-10-01', '2002-1-31', 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspanset
  WHERE t @> RandPeriod
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriod:= random_tstzspan('2000-10-01', '2002-1-31', 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspanset
  WHERE RandPeriod @> t
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriodset:= random_tstzspanset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspanset
  WHERE t @> RandPeriodset
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;


k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriodset:= random_tstzspanset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspanset
  WHERE RandPeriodset @> t
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

-----------------------------------------------
---- OPERATOR <@-------------------------------
-----------------------------------------------

k:= k+1;
FOR i IN 1..100 LOOP
  RandTimeStamp:= random_timestamptz('2000-10-01', '2002-1-31');
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzset
  WHERE RandTimeStamp <@ t
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandTimeStamp:= random_timestamptz('2000-10-01', '2002-1-31');
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspan
  WHERE RandTimeStamp <@ t
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandTstzSet:= random_tstzset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzset
  WHERE t <@ RandTstzSet
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriod:= random_tstzspan('2000-10-01', '2002-1-31', 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzset
  WHERE t <@ RandPeriod
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriodSet:= random_tstzspanset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzset
  WHERE t <@ RandPeriodSet
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriod:= random_tstzspan('2000-10-01', '2002-1-31', 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspan
  WHERE t <@ RandPeriod
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriodset:= random_tstzspanset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspan
  WHERE t <@ RandPeriodset
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriod:= random_tstzspan('2000-10-01', '2002-1-31', 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspanset
  WHERE t <@ RandPeriod
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriodset:= random_tstzspanset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspanset
  WHERE t <@ RandPeriodset
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

-----------------------------------------------
---- OPERATOR &&-------------------------------
-----------------------------------------------

k:= k+1;
FOR i IN 1..100 LOOP
  RandTstzSet:= random_tstzset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzset
  WHERE t && RandTstzSet
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriod:= random_tstzspan('2000-10-01', '2002-1-31', 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzset
  WHERE t && RandPeriod
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriodset:= random_tstzspanset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzset
  WHERE t && RandPeriodset
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriod:= random_tstzspan('2000-10-01', '2002-1-31', 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspan
  WHERE t && RandPeriod
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandTstzSet:= random_tstzset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspan
  WHERE t && RandTstzset
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriodset:= random_tstzspanset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspan
  WHERE t && RandPeriodset
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandTstzSet:= random_tstzset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspanset
  WHERE t && RandTstzSet
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriod:= random_tstzspan('2000-10-01', '2002-1-31', 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspanset
  WHERE t && RandPeriod
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriodset:= random_tstzspanset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspanset
  WHERE t && RandPeriodset
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

-----------------------------------------------
---- OPERATOR <<#------------------------------
-----------------------------------------------

k:= k+1;
FOR i IN 1..100 LOOP
  RandTimestamp:= random_timestamptz('2000-10-01', '2002-1-31');
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspan
  WHERE RandTimestamp <<# t
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandTimestamp:= random_timestamptz('2000-10-01', '2002-1-31');
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzset
  WHERE RandTimestamp <<# t
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandTimestamp:= random_timestamptz('2000-10-01', '2002-1-31');
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspanset
  WHERE RandTimestamp <<# t
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandTimestamp:= random_timestamptz('2000-10-01', '2002-1-31');
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzset
  WHERE t <<# RandTimestamp
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandTstzset:= random_tstzset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzset
  WHERE t <<# RandTstzset
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriod:= random_tstzspan('2000-10-01', '2002-1-31', 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzset
  WHERE t <<# RandPeriod
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriodset:= random_tstzspanset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzset
  WHERE t <<# RandPeriodset
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandTimestamp:= random_timestamptz('2000-10-01', '2002-1-31');
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspan
  WHERE t <<# RandTimestamp
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandTstzset:= random_tstzset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspan
  WHERE t <<# RandTstzset
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriod:= random_tstzspan('2000-10-01', '2002-1-31', 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspan
  WHERE t <<# RandPeriod
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriodSet:= random_tstzspanset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspan
  WHERE t <<# RandPeriodset
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandTimestamp:= random_timestamptz('2000-10-01', '2002-1-31');
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspanset
  WHERE t <<# RandTimestamp
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandTstzSet:= random_tstzset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspanset
  WHERE t <<# RandTstzSet
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriod:= random_tstzspan('2000-10-01', '2002-1-31', 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspanset
  WHERE t <<# RandPeriod
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriodset:= random_tstzspanset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspanset
  WHERE t <<# RandPeriodset
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

-----------------------------------------------
---- OPERATOR #>>------------------------------
-----------------------------------------------

k:= k+1;
FOR i IN 1..100 LOOP
  RandTimestamp:= random_timestamptz('2000-10-01', '2002-1-31');
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzset
  WHERE t #>> RandTimestamp
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandTstzset:= random_tstzset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzset
  WHERE t #>> RandTstzset
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriod:= random_tstzspan('2000-10-01', '2002-1-31', 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzset
  WHERE t #>> RandPeriod
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriodset:= random_tstzspanset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzset
  WHERE t #>> RandPeriodset
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandTimestamp:= random_timestamptz('2000-10-01', '2002-1-31');
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspan
  WHERE t #>> RandTimestamp
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandTstzset:= random_tstzset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspan
  WHERE t #>> RandTstzset
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriod:= random_tstzspan('2000-10-01', '2002-1-31', 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspan
  WHERE t #>> RandPeriod
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriodSet:= random_tstzspanset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspan
  WHERE t #>> RandPeriodset
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandTimestamp:= random_timestamptz('2000-10-01', '2002-1-31');
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspanset
  WHERE t #>> RandTimestamp
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandTstzSet:= random_tstzset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspanset
  WHERE t #>> RandTstzSet
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriod:= random_tstzspan('2000-10-01', '2002-1-31', 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspanset
  WHERE t #>> RandPeriod
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriodset:= random_tstzspanset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspanset
  WHERE t #>> RandPeriodset
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

-----------------------------------------------
---- OPERATOR &<#------------------------------
-----------------------------------------------

k:= k+1;
FOR i IN 1..100 LOOP
  RandTimestamp:= random_timestamptz('2000-10-01', '2002-1-31');
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzset
  WHERE t &<# RandTimestamp
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandTstzset:= random_tstzset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzset
  WHERE t &<# RandTstzset
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriod:= random_tstzspan('2000-10-01', '2002-1-31', 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzset
  WHERE t &<# RandPeriod
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriodset:= random_tstzspanset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzset
  WHERE t &<# RandPeriodset
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandTimestamp:= random_timestamptz('2000-10-01', '2002-1-31');
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspan
  WHERE t &<# RandTimestamp
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandTstzset:= random_tstzset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspan
  WHERE t &<# RandTstzset
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriod:= random_tstzspan('2000-10-01', '2002-1-31', 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspan
  WHERE t &<# RandPeriod
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriodSet:= random_tstzspanset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspan
  WHERE t &<# RandPeriodset
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandTimestamp:= random_timestamptz('2000-10-01', '2002-1-31');
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspanset
  WHERE t &<# RandTimestamp
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandTstzSet:= random_tstzset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspanset
  WHERE t &<# RandTstzSet
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriod:= random_tstzspan('2000-10-01', '2002-1-31', 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspanset
  WHERE t &<# RandPeriod
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriodset:= random_tstzspanset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspanset
  WHERE t &<# RandPeriodset
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

-----------------------------------------------
---- OPERATOR #&>------------------------------
-----------------------------------------------

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriod:= random_tstzspan('2000-10-01', '2002-1-31', 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspan
  WHERE t #&> RandPeriod
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandTimestamp:= random_timestamptz('2000-10-01', '2002-1-31');
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspan
  WHERE t #&> RandTimestamp
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandTimestamp:= random_timestamptz('2000-10-01', '2002-1-31');
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzset
  WHERE t #&> RandTimestamp
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriod:= random_tstzspan('2000-10-01', '2002-1-31', 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzset
  WHERE t #&> RandPeriod
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandTstzset:= random_tstzset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspan
  WHERE t #&> RandTstzset
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriodset:= random_tstzspanset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzset
  WHERE t #&> RandPeriodset
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  Randtstzset:= random_tstzset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspanset
  WHERE t #&> RandTstzset
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandTstzset:= random_tstzset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzset
  WHERE t #&> RandTstzset
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandTimestamp:= random_timestamptz('2000-10-01', '2002-1-31');
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspanset
  WHERE t #&> RandTimestamp
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriod:= random_tstzspan('2000-10-01', '2002-1-31', 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspanset
  WHERE t #&> RandPeriod
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriodset:= random_tstzspanset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspan
  WHERE t #&> RandPeriodset
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriodset:= random_tstzspanset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspanset
  WHERE t #&> RandPeriodset
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

-----------------------------------------------
---- OPERATOR -|-  ----------------------------
-----------------------------------------------

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriod:= random_tstzspan('2000-10-01', '2002-1-31', 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzset
  WHERE t -|- RandPeriod
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriodset:= random_tstzspanset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzset
  WHERE t -|- RandPeriodset
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandTimestamp:= random_timestamptz('2000-10-01', '2002-1-31');
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspan
  WHERE t -|- RandTimestamp
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  Randtstzset:= random_tstzset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspan
  WHERE t -|- RandTstzset
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriod:= random_tstzspan('2000-10-01', '2002-1-31', 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspan
  WHERE t -|- RandPeriod
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriodset:= random_tstzspanset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspan
  WHERE t -|- RandPeriodset
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  Randtimestamp:= random_timestamptz('2000-10-01', '2002-1-31');
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspanset
  WHERE t -|- RandTimestamp
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandTstzset:= random_tstzset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspanset
  WHERE t -|- RandTstzset
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriod:= random_tstzspan('2000-10-01', '2002-1-31', 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspanset
  WHERE t -|- RandPeriod
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

k:= k+1;
FOR i IN 1..100 LOOP
  RandPeriodset:= random_tstzspanset('2000-10-01', '2002-1-31', 10, 10);
  EXPLAIN (ANALYZE, FORMAT JSON)
  SELECT *
  FROM tbl_tstzspanset
  WHERE t -|- RandPeriodset
  INTO J;

  StartTime := clock_timestamp();
  PlanRows:= (J->0->'Plan'->>'Plan Rows')::BIGINT;
  ActualRows:=  (J->0->'Plan'->>'Actual Rows')::BIGINT;
  QFilter:=  substring((J->0->'Plan'->>'Filter') for 100);
  RowsRemovedbyFilter:= (J->0->'Plan'->>'Rows Removed by Filter'):: BIGINT;

  Query:= 'Q' || k;
  INSERT INTO execution_stats VALUES (Query, StartTime, QFilter, PlanRows, ActualRows, RowsRemovedByFilter, J);
END LOOP;

RETURN 'THE END';
END;
$$ LANGUAGE 'plpgsql';

-------------------------------------------------------------------------------
