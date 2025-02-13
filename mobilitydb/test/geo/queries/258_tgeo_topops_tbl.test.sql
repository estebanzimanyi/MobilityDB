-------------------------------------------------------------------------------
--
-- This MobilityDB code is provided under The PostgreSQL License.
-- Copyright (c) 2016-2024, Université libre de Bruxelles and MobilityDB
-- contributors
--
-- MobilityDB includes portions of PostGIS version 3 source code released
-- under the GNU General Public License (GPLv2 or later).
-- Copyright (c) 2001-2024, PostGIS contributors
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

DROP INDEX IF EXISTS tbl_tgeometry_rtree_idx;
DROP INDEX IF EXISTS tbl_tgeometry_quadtree_idx;
DROP INDEX IF EXISTS tbl_tgeometry_kdtree_idx;

-------------------------------------------------------------------------------

DROP TABLE if exists test_tgeometry_topops;
CREATE TABLE test_tgeometry_topops(
  op char(3),
  leftarg text,
  rightarg text,
  no_idx BIGINT,
  rtree_idx BIGINT,
  quadtree_idx BIGINT,
  kdtree_idx BIGINT
);

-------------------------------------------------------------------------------
-- <type> op tgeometry

INSERT INTO test_tgeometry_topops(op, leftarg, rightarg, no_idx)
SELECT '&&', 'tstzspan', 'tgeometry', COUNT(*) FROM tbl_tstzspan, tbl_tgeometry WHERE t && temp;
INSERT INTO test_tgeometry_topops(op, leftarg, rightarg, no_idx)
SELECT '@>', 'tstzspan', 'tgeometry', COUNT(*) FROM tbl_tstzspan, tbl_tgeometry WHERE t @> temp;
INSERT INTO test_tgeometry_topops(op, leftarg, rightarg, no_idx)
SELECT '<@', 'tstzspan', 'tgeometry', COUNT(*) FROM tbl_tstzspan, tbl_tgeometry WHERE t <@ temp;
INSERT INTO test_tgeometry_topops(op, leftarg, rightarg, no_idx)
SELECT '~=', 'tstzspan', 'tgeometry', COUNT(*) FROM tbl_tstzspan, tbl_tgeometry WHERE t ~= temp;
INSERT INTO test_tgeometry_topops(op, leftarg, rightarg, no_idx)
SELECT '-|-', 'tstzspan', 'tgeometry', COUNT(*) FROM tbl_tstzspan, tbl_tgeometry WHERE t -|- temp;

INSERT INTO test_tgeometry_topops(op, leftarg, rightarg, no_idx)
SELECT '&&', 'stbox', 'tgeometry', COUNT(*) FROM tbl_stbox, tbl_tgeometry WHERE SetSRID(b, 5676) && temp;
INSERT INTO test_tgeometry_topops(op, leftarg, rightarg, no_idx)
SELECT '@>', 'stbox', 'tgeometry', COUNT(*) FROM tbl_stbox, tbl_tgeometry WHERE SetSRID(b, 5676) @> temp;
INSERT INTO test_tgeometry_topops(op, leftarg, rightarg, no_idx)
SELECT '<@', 'stbox', 'tgeometry', COUNT(*) FROM tbl_stbox, tbl_tgeometry WHERE SetSRID(b, 5676) <@ temp;
INSERT INTO test_tgeometry_topops(op, leftarg, rightarg, no_idx)
SELECT '~=', 'stbox', 'tgeometry', COUNT(*) FROM tbl_stbox, tbl_tgeometry WHERE SetSRID(b, 5676) ~= temp;
INSERT INTO test_tgeometry_topops(op, leftarg, rightarg, no_idx)
SELECT '-|-', 'stbox', 'tgeometry', COUNT(*) FROM tbl_stbox, tbl_tgeometry WHERE SetSRID(b, 5676) -|- temp;

-------------------------------------------------------------------------------
--  tgeometry op <type>

INSERT INTO test_tgeometry_topops(op, leftarg, rightarg, no_idx)
SELECT '&&', 'tgeometry', 'tstzspan', COUNT(*) FROM tbl_tgeometry, tbl_tstzspan WHERE temp && t;
INSERT INTO test_tgeometry_topops(op, leftarg, rightarg, no_idx)
SELECT '@>', 'tgeometry', 'tstzspan', COUNT(*) FROM tbl_tgeometry, tbl_tstzspan WHERE temp @> t;
INSERT INTO test_tgeometry_topops(op, leftarg, rightarg, no_idx)
SELECT '<@', 'tgeometry', 'tstzspan', COUNT(*) FROM tbl_tgeometry, tbl_tstzspan WHERE temp <@ t;
INSERT INTO test_tgeometry_topops(op, leftarg, rightarg, no_idx)
SELECT '~=', 'tgeometry', 'tstzspan', COUNT(*) FROM tbl_tgeometry, tbl_tstzspan WHERE temp ~= t;
INSERT INTO test_tgeometry_topops(op, leftarg, rightarg, no_idx)
SELECT '-|-', 'tgeometry', 'tstzspan', COUNT(*) FROM tbl_tgeometry, tbl_tstzspan WHERE temp -|- t;

INSERT INTO test_tgeometry_topops(op, leftarg, rightarg, no_idx)
SELECT '&&', 'tgeometry', 'stbox', COUNT(*) FROM tbl_tgeometry, tbl_stbox WHERE temp && SetSRID(b, 5676);
INSERT INTO test_tgeometry_topops(op, leftarg, rightarg, no_idx)
SELECT '@>', 'tgeometry', 'stbox', COUNT(*) FROM tbl_tgeometry, tbl_stbox WHERE temp @> SetSRID(b, 5676);
INSERT INTO test_tgeometry_topops(op, leftarg, rightarg, no_idx)
SELECT '<@', 'tgeometry', 'stbox', COUNT(*) FROM tbl_tgeometry, tbl_stbox WHERE temp <@ SetSRID(b, 5676);
INSERT INTO test_tgeometry_topops(op, leftarg, rightarg, no_idx)
SELECT '~=', 'tgeometry', 'stbox', COUNT(*) FROM tbl_tgeometry, tbl_stbox WHERE temp ~= SetSRID(b, 5676);
INSERT INTO test_tgeometry_topops(op, leftarg, rightarg, no_idx)
SELECT '-|-', 'tgeometry', 'stbox', COUNT(*) FROM tbl_tgeometry, tbl_stbox WHERE temp -|- SetSRID(b, 5676);

INSERT INTO test_tgeometry_topops(op, leftarg, rightarg, no_idx)
SELECT '&&', 'tgeometry', 'tgeometry', COUNT(*) FROM tbl_tgeometry t1, tbl_tgeometry t2 WHERE t1.temp && t2.temp;
INSERT INTO test_tgeometry_topops(op, leftarg, rightarg, no_idx)
SELECT '@>', 'tgeometry', 'tgeometry', COUNT(*) FROM tbl_tgeometry t1, tbl_tgeometry t2 WHERE t1.temp @> t2.temp;
INSERT INTO test_tgeometry_topops(op, leftarg, rightarg, no_idx)
SELECT '<@', 'tgeometry', 'tgeometry', COUNT(*) FROM tbl_tgeometry t1, tbl_tgeometry t2 WHERE t1.temp <@ t2.temp;
INSERT INTO test_tgeometry_topops(op, leftarg, rightarg, no_idx)
SELECT '~=', 'tgeometry', 'tgeometry', COUNT(*) FROM tbl_tgeometry t1, tbl_tgeometry t2 WHERE t1.temp ~= t2.temp;
INSERT INTO test_tgeometry_topops(op, leftarg, rightarg, no_idx)
SELECT '-|-', 'tgeometry', 'tgeometry', COUNT(*) FROM tbl_tgeometry t1, tbl_tgeometry t2 WHERE t1.temp -|- t2.temp;

-------------------------------------------------------------------------------

CREATE INDEX tbl_tgeometry_rtree_idx ON tbl_tgeometry USING GIST(temp);

-------------------------------------------------------------------------------
-- <type> op tgeometry

UPDATE test_tgeometry_topops
SET rtree_idx = ( SELECT COUNT(*) FROM tbl_tstzspan, tbl_tgeometry WHERE t && temp )
WHERE op = '&&' and leftarg = 'tstzspan' and rightarg = 'tgeometry';
UPDATE test_tgeometry_topops
SET rtree_idx = ( SELECT COUNT(*) FROM tbl_tstzspan, tbl_tgeometry WHERE t @> temp )
WHERE op = '@>' and leftarg = 'tstzspan' and rightarg = 'tgeometry';
UPDATE test_tgeometry_topops
SET rtree_idx = ( SELECT COUNT(*) FROM tbl_tstzspan, tbl_tgeometry WHERE t <@ temp )
WHERE op = '<@' and leftarg = 'tstzspan' and rightarg = 'tgeometry';
UPDATE test_tgeometry_topops
SET rtree_idx = ( SELECT COUNT(*) FROM tbl_tstzspan, tbl_tgeometry WHERE t ~= temp )
WHERE op = '~=' and leftarg = 'tstzspan' and rightarg = 'tgeometry';
UPDATE test_tgeometry_topops
SET rtree_idx = ( SELECT COUNT(*) FROM tbl_tstzspan, tbl_tgeometry WHERE t -|- temp )
WHERE op = '-|-' and leftarg = 'tstzspan' and rightarg = 'tgeometry';

UPDATE test_tgeometry_topops
SET rtree_idx = ( SELECT COUNT(*) FROM tbl_stbox, tbl_tgeometry WHERE SetSRID(b, 5676) && temp )
WHERE op = '&&' and leftarg = 'stbox' and rightarg = 'tgeometry';
UPDATE test_tgeometry_topops
SET rtree_idx = ( SELECT COUNT(*) FROM tbl_stbox, tbl_tgeometry WHERE SetSRID(b, 5676) @> temp )
WHERE op = '@>' and leftarg = 'stbox' and rightarg = 'tgeometry';
UPDATE test_tgeometry_topops
SET rtree_idx = ( SELECT COUNT(*) FROM tbl_stbox, tbl_tgeometry WHERE SetSRID(b, 5676) <@ temp )
WHERE op = '<@' and leftarg = 'stbox' and rightarg = 'tgeometry';
UPDATE test_tgeometry_topops
SET rtree_idx = ( SELECT COUNT(*) FROM tbl_stbox, tbl_tgeometry WHERE SetSRID(b, 5676) ~= temp )
WHERE op = '~=' and leftarg = 'stbox' and rightarg = 'tgeometry';
UPDATE test_tgeometry_topops
SET rtree_idx = ( SELECT COUNT(*) FROM tbl_stbox, tbl_tgeometry WHERE SetSRID(b, 5676) -|- temp )
WHERE op = '-|-' and leftarg = 'stbox' and rightarg = 'tgeometry';

-------------------------------------------------------------------------------
-- tgeometry op <type>

UPDATE test_tgeometry_topops
SET rtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry, tbl_tstzspan WHERE temp && t )
WHERE op = '&&' and leftarg = 'tgeometry' and rightarg = 'tstzspan';
UPDATE test_tgeometry_topops
SET rtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry, tbl_tstzspan WHERE temp @> t )
WHERE op = '@>' and leftarg = 'tgeometry' and rightarg = 'tstzspan';
UPDATE test_tgeometry_topops
SET rtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry, tbl_tstzspan WHERE temp <@ t )
WHERE op = '<@' and leftarg = 'tgeometry' and rightarg = 'tstzspan';
UPDATE test_tgeometry_topops
SET rtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry, tbl_tstzspan WHERE temp ~= t )
WHERE op = '~=' and leftarg = 'tgeometry' and rightarg = 'tstzspan';
UPDATE test_tgeometry_topops
SET rtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry, tbl_tstzspan WHERE temp -|- t )
WHERE op = '-|-' and leftarg = 'tgeometry' and rightarg = 'tstzspan';

UPDATE test_tgeometry_topops
SET rtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry, tbl_stbox WHERE temp && SetSRID(b, 5676) )
WHERE op = '&&' and leftarg = 'tgeometry' and rightarg = 'stbox';
UPDATE test_tgeometry_topops
SET rtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry, tbl_stbox WHERE temp @> SetSRID(b, 5676) )
WHERE op = '@>' and leftarg = 'tgeometry' and rightarg = 'stbox';
UPDATE test_tgeometry_topops
SET rtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry, tbl_stbox WHERE temp <@ SetSRID(b, 5676) )
WHERE op = '<@' and leftarg = 'tgeometry' and rightarg = 'stbox';
UPDATE test_tgeometry_topops
SET rtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry, tbl_stbox WHERE temp ~= SetSRID(b, 5676) )
WHERE op = '~=' and leftarg = 'tgeometry' and rightarg = 'stbox';
UPDATE test_tgeometry_topops
SET rtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry, tbl_stbox WHERE temp -|- SetSRID(b, 5676) )
WHERE op = '-|-' and leftarg = 'tgeometry' and rightarg = 'stbox';

UPDATE test_tgeometry_topops
SET rtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry t1, tbl_tgeometry t2 WHERE t1.temp && t2.temp )
WHERE op = '&&' and leftarg = 'tgeometry' and rightarg = 'tgeometry';
UPDATE test_tgeometry_topops
SET rtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry t1, tbl_tgeometry t2 WHERE t1.temp @> t2.temp )
WHERE op = '@>' and leftarg = 'tgeometry' and rightarg = 'tgeometry';
UPDATE test_tgeometry_topops
SET rtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry t1, tbl_tgeometry t2 WHERE t1.temp <@ t2.temp )
WHERE op = '<@' and leftarg = 'tgeometry' and rightarg = 'tgeometry';
UPDATE test_tgeometry_topops
SET rtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry t1, tbl_tgeometry t2 WHERE t1.temp ~= t2.temp )
WHERE op = '~=' and leftarg = 'tgeometry' and rightarg = 'tgeometry';
UPDATE test_tgeometry_topops
SET rtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry t1, tbl_tgeometry t2 WHERE t1.temp -|- t2.temp )
WHERE op = '-|-' and leftarg = 'tgeometry' and rightarg = 'tgeometry';

-------------------------------------------------------------------------------

DROP INDEX tbl_tgeometry_rtree_idx;
CREATE INDEX tbl_tgeometry_quadtree_idx ON tbl_tgeometry USING SPGIST(temp);

-------------------------------------------------------------------------------
-- <type> op tgeometry

UPDATE test_tgeometry_topops
SET quadtree_idx = ( SELECT COUNT(*) FROM tbl_tstzspan, tbl_tgeometry WHERE t && temp )
WHERE op = '&&' and leftarg = 'tstzspan' and rightarg = 'tgeometry';
UPDATE test_tgeometry_topops
SET quadtree_idx = ( SELECT COUNT(*) FROM tbl_tstzspan, tbl_tgeometry WHERE t @> temp )
WHERE op = '@>' and leftarg = 'tstzspan' and rightarg = 'tgeometry';
UPDATE test_tgeometry_topops
SET quadtree_idx = ( SELECT COUNT(*) FROM tbl_tstzspan, tbl_tgeometry WHERE t <@ temp )
WHERE op = '<@' and leftarg = 'tstzspan' and rightarg = 'tgeometry';
UPDATE test_tgeometry_topops
SET quadtree_idx = ( SELECT COUNT(*) FROM tbl_tstzspan, tbl_tgeometry WHERE t ~= temp )
WHERE op = '~=' and leftarg = 'tstzspan' and rightarg = 'tgeometry';
UPDATE test_tgeometry_topops
SET quadtree_idx = ( SELECT COUNT(*) FROM tbl_tstzspan, tbl_tgeometry WHERE t -|- temp )
WHERE op = '-|-' and leftarg = 'tstzspan' and rightarg = 'tgeometry';

UPDATE test_tgeometry_topops
SET quadtree_idx = ( SELECT COUNT(*) FROM tbl_stbox, tbl_tgeometry WHERE SetSRID(b, 5676) && temp )
WHERE op = '&&' and leftarg = 'stbox' and rightarg = 'tgeometry';
UPDATE test_tgeometry_topops
SET quadtree_idx = ( SELECT COUNT(*) FROM tbl_stbox, tbl_tgeometry WHERE SetSRID(b, 5676) @> temp )
WHERE op = '@>' and leftarg = 'stbox' and rightarg = 'tgeometry';
UPDATE test_tgeometry_topops
SET quadtree_idx = ( SELECT COUNT(*) FROM tbl_stbox, tbl_tgeometry WHERE SetSRID(b, 5676) <@ temp )
WHERE op = '<@' and leftarg = 'stbox' and rightarg = 'tgeometry';
UPDATE test_tgeometry_topops
SET quadtree_idx = ( SELECT COUNT(*) FROM tbl_stbox, tbl_tgeometry WHERE SetSRID(b, 5676) ~= temp )
WHERE op = '~=' and leftarg = 'stbox' and rightarg = 'tgeometry';
UPDATE test_tgeometry_topops
SET quadtree_idx = ( SELECT COUNT(*) FROM tbl_stbox, tbl_tgeometry WHERE SetSRID(b, 5676) -|- temp )
WHERE op = '-|-' and leftarg = 'stbox' and rightarg = 'tgeometry';

-------------------------------------------------------------------------------
-- tgeometry op <type>

UPDATE test_tgeometry_topops
SET quadtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry, tbl_tstzspan WHERE temp && t )
WHERE op = '&&' and leftarg = 'tgeometry' and rightarg = 'tstzspan';
UPDATE test_tgeometry_topops
SET quadtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry, tbl_tstzspan WHERE temp @> t )
WHERE op = '@>' and leftarg = 'tgeometry' and rightarg = 'tstzspan';
UPDATE test_tgeometry_topops
SET quadtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry, tbl_tstzspan WHERE temp <@ t )
WHERE op = '<@' and leftarg = 'tgeometry' and rightarg = 'tstzspan';
UPDATE test_tgeometry_topops
SET quadtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry, tbl_tstzspan WHERE temp ~= t )
WHERE op = '~=' and leftarg = 'tgeometry' and rightarg = 'tstzspan';
UPDATE test_tgeometry_topops
SET quadtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry, tbl_tstzspan WHERE temp -|- t )
WHERE op = '-|-' and leftarg = 'tgeometry' and rightarg = 'tstzspan';

UPDATE test_tgeometry_topops
SET quadtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry, tbl_stbox WHERE temp && SetSRID(b, 5676) )
WHERE op = '&&' and leftarg = 'tgeometry' and rightarg = 'stbox';
UPDATE test_tgeometry_topops
SET quadtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry, tbl_stbox WHERE temp @> SetSRID(b, 5676) )
WHERE op = '@>' and leftarg = 'tgeometry' and rightarg = 'stbox';
UPDATE test_tgeometry_topops
SET quadtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry, tbl_stbox WHERE temp <@ SetSRID(b, 5676) )
WHERE op = '<@' and leftarg = 'tgeometry' and rightarg = 'stbox';
UPDATE test_tgeometry_topops
SET quadtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry, tbl_stbox WHERE temp ~= SetSRID(b, 5676) )
WHERE op = '~=' and leftarg = 'tgeometry' and rightarg = 'stbox';
UPDATE test_tgeometry_topops
SET quadtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry, tbl_stbox WHERE temp -|- SetSRID(b, 5676) )
WHERE op = '-|-' and leftarg = 'tgeometry' and rightarg = 'stbox';

UPDATE test_tgeometry_topops
SET quadtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry t1, tbl_tgeometry t2 WHERE t1.temp && t2.temp )
WHERE op = '&&' and leftarg = 'tgeometry' and rightarg = 'tgeometry';
UPDATE test_tgeometry_topops
SET quadtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry t1, tbl_tgeometry t2 WHERE t1.temp @> t2.temp )
WHERE op = '@>' and leftarg = 'tgeometry' and rightarg = 'tgeometry';
UPDATE test_tgeometry_topops
SET quadtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry t1, tbl_tgeometry t2 WHERE t1.temp <@ t2.temp )
WHERE op = '<@' and leftarg = 'tgeometry' and rightarg = 'tgeometry';
UPDATE test_tgeometry_topops
SET quadtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry t1, tbl_tgeometry t2 WHERE t1.temp ~= t2.temp )
WHERE op = '~=' and leftarg = 'tgeometry' and rightarg = 'tgeometry';
UPDATE test_tgeometry_topops
SET quadtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry t1, tbl_tgeometry t2 WHERE t1.temp -|- t2.temp )
WHERE op = '-|-' and leftarg = 'tgeometry' and rightarg = 'tgeometry';

-------------------------------------------------------------------------------

DROP INDEX tbl_tgeometry_quadtree_idx;
CREATE INDEX tbl_tgeometry_kdtree_idx ON tbl_tgeometry USING SPGIST(temp tgeometry_kdtree_ops);

-------------------------------------------------------------------------------
-- <type> op tgeometry

UPDATE test_tgeometry_topops
SET kdtree_idx = ( SELECT COUNT(*) FROM tbl_tstzspan, tbl_tgeometry WHERE t && temp )
WHERE op = '&&' and leftarg = 'tstzspan' and rightarg = 'tgeometry';
UPDATE test_tgeometry_topops
SET kdtree_idx = ( SELECT COUNT(*) FROM tbl_tstzspan, tbl_tgeometry WHERE t @> temp )
WHERE op = '@>' and leftarg = 'tstzspan' and rightarg = 'tgeometry';
UPDATE test_tgeometry_topops
SET kdtree_idx = ( SELECT COUNT(*) FROM tbl_tstzspan, tbl_tgeometry WHERE t <@ temp )
WHERE op = '<@' and leftarg = 'tstzspan' and rightarg = 'tgeometry';
UPDATE test_tgeometry_topops
SET kdtree_idx = ( SELECT COUNT(*) FROM tbl_tstzspan, tbl_tgeometry WHERE t ~= temp )
WHERE op = '~=' and leftarg = 'tstzspan' and rightarg = 'tgeometry';
UPDATE test_tgeometry_topops
SET kdtree_idx = ( SELECT COUNT(*) FROM tbl_tstzspan, tbl_tgeometry WHERE t -|- temp )
WHERE op = '-|-' and leftarg = 'tstzspan' and rightarg = 'tgeometry';

UPDATE test_tgeometry_topops
SET kdtree_idx = ( SELECT COUNT(*) FROM tbl_stbox, tbl_tgeometry WHERE SetSRID(b, 5676) && temp )
WHERE op = '&&' and leftarg = 'stbox' and rightarg = 'tgeometry';
UPDATE test_tgeometry_topops
SET kdtree_idx = ( SELECT COUNT(*) FROM tbl_stbox, tbl_tgeometry WHERE SetSRID(b, 5676) @> temp )
WHERE op = '@>' and leftarg = 'stbox' and rightarg = 'tgeometry';
UPDATE test_tgeometry_topops
SET kdtree_idx = ( SELECT COUNT(*) FROM tbl_stbox, tbl_tgeometry WHERE SetSRID(b, 5676) <@ temp )
WHERE op = '<@' and leftarg = 'stbox' and rightarg = 'tgeometry';
UPDATE test_tgeometry_topops
SET kdtree_idx = ( SELECT COUNT(*) FROM tbl_stbox, tbl_tgeometry WHERE SetSRID(b, 5676) ~= temp )
WHERE op = '~=' and leftarg = 'stbox' and rightarg = 'tgeometry';
UPDATE test_tgeometry_topops
SET kdtree_idx = ( SELECT COUNT(*) FROM tbl_stbox, tbl_tgeometry WHERE SetSRID(b, 5676) -|- temp )
WHERE op = '-|-' and leftarg = 'stbox' and rightarg = 'tgeometry';

-------------------------------------------------------------------------------
-- tgeometry op <type>

UPDATE test_tgeometry_topops
SET kdtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry, tbl_tstzspan WHERE temp && t )
WHERE op = '&&' and leftarg = 'tgeometry' and rightarg = 'tstzspan';
UPDATE test_tgeometry_topops
SET kdtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry, tbl_tstzspan WHERE temp @> t )
WHERE op = '@>' and leftarg = 'tgeometry' and rightarg = 'tstzspan';
UPDATE test_tgeometry_topops
SET kdtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry, tbl_tstzspan WHERE temp <@ t )
WHERE op = '<@' and leftarg = 'tgeometry' and rightarg = 'tstzspan';
UPDATE test_tgeometry_topops
SET kdtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry, tbl_tstzspan WHERE temp ~= t )
WHERE op = '~=' and leftarg = 'tgeometry' and rightarg = 'tstzspan';
UPDATE test_tgeometry_topops
SET kdtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry, tbl_tstzspan WHERE temp -|- t )
WHERE op = '-|-' and leftarg = 'tgeometry' and rightarg = 'tstzspan';

UPDATE test_tgeometry_topops
SET kdtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry, tbl_stbox WHERE temp && SetSRID(b, 5676) )
WHERE op = '&&' and leftarg = 'tgeometry' and rightarg = 'stbox';
UPDATE test_tgeometry_topops
SET kdtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry, tbl_stbox WHERE temp @> SetSRID(b, 5676) )
WHERE op = '@>' and leftarg = 'tgeometry' and rightarg = 'stbox';
UPDATE test_tgeometry_topops
SET kdtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry, tbl_stbox WHERE temp <@ SetSRID(b, 5676) )
WHERE op = '<@' and leftarg = 'tgeometry' and rightarg = 'stbox';
UPDATE test_tgeometry_topops
SET kdtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry, tbl_stbox WHERE temp ~= SetSRID(b, 5676) )
WHERE op = '~=' and leftarg = 'tgeometry' and rightarg = 'stbox';
UPDATE test_tgeometry_topops
SET kdtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry, tbl_stbox WHERE temp -|- SetSRID(b, 5676) )
WHERE op = '-|-' and leftarg = 'tgeometry' and rightarg = 'stbox';

UPDATE test_tgeometry_topops
SET kdtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry t1, tbl_tgeometry t2 WHERE t1.temp && t2.temp )
WHERE op = '&&' and leftarg = 'tgeometry' and rightarg = 'tgeometry';
UPDATE test_tgeometry_topops
SET kdtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry t1, tbl_tgeometry t2 WHERE t1.temp @> t2.temp )
WHERE op = '@>' and leftarg = 'tgeometry' and rightarg = 'tgeometry';
UPDATE test_tgeometry_topops
SET kdtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry t1, tbl_tgeometry t2 WHERE t1.temp <@ t2.temp )
WHERE op = '<@' and leftarg = 'tgeometry' and rightarg = 'tgeometry';
UPDATE test_tgeometry_topops
SET kdtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry t1, tbl_tgeometry t2 WHERE t1.temp ~= t2.temp )
WHERE op = '~=' and leftarg = 'tgeometry' and rightarg = 'tgeometry';
UPDATE test_tgeometry_topops
SET kdtree_idx = ( SELECT COUNT(*) FROM tbl_tgeometry t1, tbl_tgeometry t2 WHERE t1.temp -|- t2.temp )
WHERE op = '-|-' and leftarg = 'tgeometry' and rightarg = 'tgeometry';

-------------------------------------------------------------------------------

DROP INDEX tbl_tgeometry_kdtree_idx;

-------------------------------------------------------------------------------

SELECT * FROM test_tgeometry_topops
WHERE no_idx <> rtree_idx OR no_idx <> quadtree_idx OR
  no_idx IS NULL OR rtree_idx IS NULL OR quadtree_idx IS NULL OR kdtree_idx IS NULL
ORDER BY op, leftarg, rightarg;

-------------------------------------------------------------------------------

DROP TABLE test_tgeometry_topops;

-------------------------------------------------------------------------------
