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

-- Table-level tests for the extent() aggregate over the pgPointCloud
-- temporal types.  Each test reduces a whole table to a single scalar
-- so the expected output stays human-readable.

-------------------------------------------------------------------------------
-- extent(tpcbox) — folding the per-row tpcbox bboxes is idempotent
-- against re-running the same aggregation on the result row.
-------------------------------------------------------------------------------

SELECT pcid(extent(b)) FROM tbl_tpcbox;

-- Aggregating the aggregate result yields the same value back.
SELECT extent(b) ~= (SELECT extent(b) FROM tbl_tpcbox) FROM tbl_tpcbox;

-------------------------------------------------------------------------------
-- extent(tpcpoint) / extent(tpcpatch) — pcid must propagate from the
-- rows to the aggregate result, and the box must contain every row's
-- start time.
-------------------------------------------------------------------------------

SELECT pcid(extent(temp)) FROM tbl_tpcpoint;
SELECT pcid(extent(temp)) FROM tbl_tpcpatch;

SELECT bool_and(startTimestamp(temp) >= tmin(ext) AND endTimestamp(temp) <= tmax(ext))
FROM tbl_tpcpoint, (SELECT extent(temp) AS ext FROM tbl_tpcpoint) e;
SELECT bool_and(startTimestamp(temp) >= tmin(ext) AND endTimestamp(temp) <= tmax(ext))
FROM tbl_tpcpatch, (SELECT extent(temp) AS ext FROM tbl_tpcpatch) e;

-------------------------------------------------------------------------------
