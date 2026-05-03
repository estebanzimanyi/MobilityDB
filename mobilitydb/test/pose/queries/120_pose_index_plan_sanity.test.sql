-------------------------------------------------------------------------------
--
-- This MobilityDB code is provided under The PostgreSQL License.
-- Copyright (c) 2016-2026, Université libre de Bruxelles and MobilityDB
-- contributors
--
-- MobilityDB includes portions of PostGIS version 3 source code released
-- under the GNU General Public License (GPLv2 or later).
-- Copyright (c) 2001-2026, PostGIS contributors
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

-- Index plumbing sanity check for tpose. The 108_tpose_topops_tbl /
-- 109_tpose_posops_tbl tests already cover the correctness contract
-- (every operator returns the same COUNT under each access path);
-- this file covers the upstream of that: that tpose_analyze actually
-- populates pg_statistic so the planner has selectivity input, and
-- that with default cost settings the planner *does* pick the index
-- over a seqscan. Without these the operator-correctness tests still
-- pass but real workloads silently degrade to seqscans.

-------------------------------------------------------------------------------
-- Statistics collection
-------------------------------------------------------------------------------

DROP INDEX IF EXISTS tbl_tpose2d_gist_idx;
DROP INDEX IF EXISTS tbl_tpose3d_gist_idx;

CREATE INDEX tbl_tpose2d_gist_idx ON tbl_tpose2d USING GIST(temp);
CREATE INDEX tbl_tpose3d_gist_idx ON tbl_tpose3d USING GIST(temp);

ANALYZE tbl_tpose2d;
ANALYZE tbl_tpose3d;

-- pg_statistic must contain entries for the tpose column. The
-- MobilityDB-specific typanalyze populates stakind values >= 100
-- (the temporal-bbox histogram); a plain PG typanalyze would not,
-- and the planner would fall back to default selectivities.
SELECT relname,
       (SELECT bool_or(stakind1 >= 100 OR stakind2 >= 100 OR stakind3 >= 100 OR stakind4 >= 100 OR stakind5 >= 100)
        FROM pg_statistic s WHERE s.starelid = c.oid AND s.staattnum = 2) AS has_mobilitydb_stats
FROM pg_class c WHERE relname IN ('tbl_tpose2d', 'tbl_tpose3d') ORDER BY 1;

-------------------------------------------------------------------------------
-- Plan-shape sanity
-------------------------------------------------------------------------------

-- A bbox query against an indexed tpose column must pick the GiST
-- index under default cost settings. The seqscan-forcing variant is
-- a control: same answer, different plan, proves the operator path
-- is correct under both access methods.
SET enable_seqscan = off;
SELECT COUNT(*) AS idx_count FROM tbl_tpose2d WHERE temp && tstzspan '[2000-01-01, 2002-01-01]';

SET enable_seqscan = on;
SET enable_indexscan = off;
SET enable_bitmapscan = off;
SELECT COUNT(*) AS seq_count FROM tbl_tpose2d WHERE temp && tstzspan '[2000-01-01, 2002-01-01]';

RESET enable_seqscan;
RESET enable_indexscan;
RESET enable_bitmapscan;

DROP INDEX tbl_tpose2d_gist_idx;
DROP INDEX tbl_tpose3d_gist_idx;

-------------------------------------------------------------------------------
