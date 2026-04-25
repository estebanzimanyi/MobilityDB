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

-- Value-level tests for asMFJSON() on tpcpoint and tpcpatch.

\set p1 'tpcpoint(PC_MakePoint(1, ARRAY[1.0, 2.0, 3.0]::float[]), ''2024-01-01''::timestamptz)'
\set p2 'tpcpoint(PC_MakePoint(1, ARRAY[4.0, 5.0, 6.0]::float[]), ''2024-01-02''::timestamptz)'
\set patch 'PC_Patch(ARRAY[PC_MakePoint(1, ARRAY[1.0, 1.0, 1.0]::float[]), PC_MakePoint(1, ARRAY[2.0, 2.0, 2.0]::float[])])'
\set q1 'tpcpatch(:patch, ''2024-01-01''::timestamptz)'

-------------------------------------------------------------------------------
-- tpcpoint MF-JSON: instant + sequence with bbox = options 1.
-------------------------------------------------------------------------------

SELECT asMFJSON(:p1);
SELECT asMFJSON(tpcpointSeq(ARRAY[:p1, :p2]));
SELECT asMFJSON(:p1, 1);

-------------------------------------------------------------------------------
-- tpcpatch MF-JSON: pcid + npoints + bounds in the values array.
-------------------------------------------------------------------------------

SELECT asMFJSON(:q1);
SELECT asMFJSON(:q1, 1);

-------------------------------------------------------------------------------
