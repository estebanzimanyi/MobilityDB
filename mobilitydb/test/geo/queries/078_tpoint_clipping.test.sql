-------------------------------------------------------------------------------
--
-- This MobilityDB code is provided under The PostgreSQL License.
-- Copyright (c) 2016-2023, Université libre de Bruxelles and MobilityDB
-- contributors
--
-- MobilityDB includes portions of PostGIS version 3 source code released
-- under the GNU General Public License (GPLv2 or later).
-- Copyright (c) 2001-2023, PostGIS contributors
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
-- Polygon-Polygon
-------------------------------------------------------------------------------

-- Point intersections
SELECT st_astext(cl_intersection(geometry 'Polygon((1 1,1 5,5 5,5 1,1 1))',
  geometry 'Polygon((0 3,3 0,6 3,3 6,0 3))'));

-- Overlapping Segments
SELECT st_astext(cl_intersection(geometry 'Polygon((1 1,1 5,5 5,5 1,1 1))',
  geometry 'Polygon((3 1,3 5,7 5,7 1,3 1))'));
SELECT st_astext(cl_intersection(geometry 'Polygon((1 1,1 5,5 5,5 1,1 1))',
  geometry 'Polygon((1 3,1 7,5 7,5 3,1 3))'));
SELECT st_astext(cl_intersection(geometry 'Polygon((0 3,3 0,6 3,3 6,0 3))',
  geometry 'Polygon((1 4,4 1,7 4,4 7,1 4))'));

-- Point and segment intersections
SELECT st_astext(cl_intersection(geometry 'Polygon((1 1,1 5,5 5,5 1,1 1))',
  geometry 'Polygon((3 1,3 6,7 6,7 1,3 1))'));
SELECT st_astext(cl_intersection(geometry 'Polygon((1 1,1 5,5 5,5 1,1 1))',
  geometry 'Polygon((1 3,1 7,6 8,6 3,1 3))'));
SELECT st_astext(cl_intersection(geometry 'Polygon((0 3,3 0,6 3,3 6,0 3))',
  geometry 'Polygon((0 5,4 1,7 4,3 8,0 5))'));

-- Hole that does not interact with the other polygon
SELECT st_astext(cl_intersection(geometry 'Polygon((1 1,1 5,5 5,5 1,1 1),(2 2,2 4,3 4,3 2,2 2))',
  geometry 'Polygon((4 1,4 5,8 5,8 1,4 1))'));
-- Hole whose countour intersects with the contour of the other polygon
SELECT st_astext(cl_intersection(geometry 'Polygon((1 1,1 5,5 5,5 1,1 1),(2 2,2 4,3 4,3 2,2 2))',
  geometry 'Polygon((3 1,3 5,6 5,6 1,3 1))'));
-- Hole that removes part of the contour of the other polygon
SELECT st_astext(cl_intersection(geometry 'Polygon((1 1,1 5,5 5,5 1,1 1),(2 2,2 4,4 4,4 2,2 2))',
  geometry 'Polygon((3 1,3 5,6 5,6 1,3 1))'));

-- Point intersections with a hole that does not interact with the other polygon
-- SELECT st_astext(cl_intersection(geometry 'Polygon((1 1,1 5,5 5,5 1,1 1),(2 2,4 2,4 4,2 4,2 2))',
  -- geometry 'Polygon((0 3,3 0,6 3,3 6,0 3))'));

--------------------------------------------------------

