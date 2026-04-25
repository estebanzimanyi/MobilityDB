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
-------------------------------------------------------------------------------

-- Value-level tests for tpcpatch — same constructor-function pattern
-- as 420_tpcpoint.test.sql.

\set patch1 'PC_Patch(ARRAY[PC_MakePoint(1, ARRAY[1.0, 1.0, 1.0]::float[]), PC_MakePoint(1, ARRAY[2.0, 2.0, 2.0]::float[])])'
\set patch2 'PC_Patch(ARRAY[PC_MakePoint(1, ARRAY[5.0, 5.0, 5.0]::float[]), PC_MakePoint(1, ARRAY[6.0, 6.0, 6.0]::float[])])'

\set inst1 'tpcpatch(:patch1, ''2024-01-01''::timestamptz)'
\set inst2 'tpcpatch(:patch2, ''2024-01-02''::timestamptz)'

-------------------------------------------------------------------------------
-- pcid + per-instant point counts
-------------------------------------------------------------------------------

SELECT pcid(:inst1);
SELECT startNumPoints(:inst1);
SELECT endNumPoints(:inst1);
SELECT numInstants(:inst1);
SELECT numInstants(tpcpatchSeq(ARRAY[:inst1, :inst2]));

-------------------------------------------------------------------------------
-- Time accessors
-------------------------------------------------------------------------------

SELECT startTimestamp(tpcpatchSeq(ARRAY[:inst1, :inst2]));
SELECT endTimestamp(tpcpatchSeq(ARRAY[:inst1, :inst2]));
SELECT pcid(tpcpatchSeq(ARRAY[:inst1, :inst2]));

-------------------------------------------------------------------------------
-- Comparisons
-------------------------------------------------------------------------------

SELECT (:inst1) = (:inst1);
SELECT (:inst1) = (:inst2);
SELECT (:inst1) <> (:inst2);
SELECT (:inst1) < (:inst2);

-------------------------------------------------------------------------------
-- Restrictions — at/minusTime, at/minusTpcbox.
-------------------------------------------------------------------------------

SELECT atTime(tpcpatchSeq(ARRAY[:inst1, :inst2]),
  tstzspan '[2024-01-02, 2024-01-03]') IS NOT NULL;
SELECT atTpcbox(:inst1, tpcbox_zt(0, 0, 0, 10, 10, 10,
  tstzspan '[2024-01-01, 2024-01-31]', 1, 0)) IS NOT NULL;
SELECT minusTpcbox(:inst1, tpcbox_zt(0, 0, 0, 10, 10, 10,
  tstzspan '[2024-01-01, 2024-01-31]', 1, 0)) IS NULL;
SELECT atTpcbox(:inst1, tpcbox_zt(0, 0, 0, 10, 10, 10,
  tstzspan '[2024-01-01, 2024-01-31]', 999, 0)) IS NULL;

-------------------------------------------------------------------------------
