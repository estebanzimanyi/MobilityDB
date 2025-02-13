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
-- Input
-------------------------------------------------------------------------------

SELECT asText(tgeometry 'Point(1 1)@2000-01-01');
SELECT asText(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}');
SELECT asText(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]');
SELECT asText(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}');

SELECT asText(tgeometry '  Point(1 1)  @  2000-01-01  ');
SELECT asText(tgeometry '  {   Point(1 1)  @ 2000-01-01  , Linestring(1 1,2 2)@2000-01-02, Point(1 1) @  2000-01-03   }   ');
SELECT asText(tgeometry '  [  Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]');
SELECT asText(tgeometry '  {  [  Point(1 1)@2000-01-01 ,    Point(1 1)  @2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}');

-- Normalization
SELECT asText(tgeometry '[Linestring(1 1,2 2)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Linestring(1 1,2 2)@2000-01-03]');
SELECT asText(tgeometry'{[Linestring(1 1,2 2)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Linestring(1 1,2 2)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05, Point(2 2)@2000-01-06]}');

-------------------------------------------------------------------------------

SELECT asText(tgeography 'Point(1 1)@2000-01-01');
SELECT asText(tgeography '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}');
SELECT asText(tgeography '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]');
SELECT asText(tgeography '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}');

SELECT asText(tgeography '  Point(1 1)  @  2000-01-01  ');
SELECT asText(tgeography '  {   Point(1 1)  @ 2000-01-01  , Linestring(1 1,2 2)@2000-01-02, Point(1 1) @  2000-01-03   }   ');
SELECT asText(tgeography '  [  Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]');
SELECT asText(tgeography '  {  [  Point(1 1)@2000-01-01 ,    Point(1 1)  @2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}');

-------------------------------------------------------------------------------
-- Input/output in WKT, WKB and HexWKB formats
-------------------------------------------------------------------------------

SELECT asText(tgeometry 'Point(1 1.123456789)@2000-01-01', 6);
SELECT asText(tgeometry '{Point(1 1.123456789)@2000-01-01, Linestring(1 1.523456789,2 2)@2000-01-02, Point(1 1.123456789)@2000-01-03}', 6);
SELECT asText(tgeometry '[Point(1 1.123456789)@2000-01-01, Linestring(1 1.523456789,2 2)@2000-01-02, Point(1 1.123456789)@2000-01-03]', 6);
SELECT asText(tgeometry '{[Point(1 1.123456789)@2000-01-01, Linestring(1 1.523456789,2 2)@2000-01-02, Point(1 1.123456789)@2000-01-03],[Point(2 2.723456789)@2000-01-04, Point(2 2.723456789)@2000-01-05]}', 6);

-------------------------------------------------------------------------------

SELECT asText(tgeography 'Point(1 1.123456789)@2000-01-01', 6);
SELECT asText(tgeography '{Point(1 1.123456789)@2000-01-01, Linestring(1 1.523456789,2 2)@2000-01-02, Point(1 1.123456789)@2000-01-03}', 6);
SELECT asText(tgeography '[Point(1 1.123456789)@2000-01-01, Linestring(1 1.523456789,2 2)@2000-01-02, Point(1 1.123456789)@2000-01-03]', 6);
SELECT asText(tgeography '{[Point(1 1.123456789)@2000-01-01, Linestring(1 1.523456789,2 2)@2000-01-02, Point(1 1.123456789)@2000-01-03],[Point(2 2.723456789)@2000-01-04, Point(2 2.723456789)@2000-01-05]}', 6);

-------------------------------------------------------------------------------
-- Maximum decimal digits

SELECT asText(tgeometryFromBinary(asBinary(tgeometry 'Point(1 1)@2000-01-01')));
SELECT asText(tgeometryFromBinary(asBinary(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}')));
SELECT asText(tgeometryFromBinary(asBinary(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]')));
SELECT asText(tgeometryFromBinary(asBinary(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}')));

SELECT asText(tgeometryFromBinary(asBinary(tgeometry 'Point(1 1)@2000-01-01', 'NDR')));
SELECT asText(tgeometryFromBinary(asBinary(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}', 'NDR')));
SELECT asText(tgeometryFromBinary(asBinary(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]', 'NDR')));
SELECT asText(tgeometryFromBinary(asBinary(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}', 'NDR')));

SELECT asText(tgeometryFromBinary(asBinary(tgeometry 'Point(1 1)@2000-01-01', 'XDR')));
SELECT asText(tgeometryFromBinary(asBinary(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}', 'XDR')));
SELECT asText(tgeometryFromBinary(asBinary(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]', 'XDR')));
SELECT asText(tgeometryFromBinary(asBinary(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}', 'XDR')));

-------------------------------------------------------------------------------

SELECT asText(tgeographyFromBinary(asBinary(tgeography 'Point(1 1)@2000-01-01')));
SELECT asText(tgeographyFromBinary(asBinary(tgeography '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}')));
SELECT asText(tgeographyFromBinary(asBinary(tgeography '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]')));
SELECT asText(tgeographyFromBinary(asBinary(tgeography '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}')));

SELECT asText(tgeographyFromBinary(asBinary(tgeography 'Point(1 1)@2000-01-01', 'NDR')));
SELECT asText(tgeographyFromBinary(asBinary(tgeography '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}', 'NDR')));
SELECT asText(tgeographyFromBinary(asBinary(tgeography '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]', 'NDR')));
SELECT asText(tgeographyFromBinary(asBinary(tgeography '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}', 'NDR')));

SELECT asText(tgeographyFromBinary(asBinary(tgeography 'Point(1 1)@2000-01-01', 'XDR')));
SELECT asText(tgeographyFromBinary(asBinary(tgeography '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}', 'XDR')));
SELECT asText(tgeographyFromBinary(asBinary(tgeography '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]', 'XDR')));
SELECT asText(tgeographyFromBinary(asBinary(tgeography '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}', 'XDR')));

-------------------------------------------------------------------------------

SELECT asText(tgeometryFromHexEWKB(asHexEWKB(tgeometry 'Point(1 1)@2000-01-01')));
SELECT asText(tgeometryFromHexEWKB(asHexEWKB(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}')));
SELECT asText(tgeometryFromHexEWKB(asHexEWKB(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]')));
SELECT asText(tgeometryFromHexEWKB(asHexEWKB(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}')));

SELECT asText(tgeometryFromHexEWKB(asHexEWKB(tgeometry 'Point(1 1)@2000-01-01', 'NDR')));
SELECT asText(tgeometryFromHexEWKB(asHexEWKB(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}', 'NDR')));
SELECT asText(tgeometryFromHexEWKB(asHexEWKB(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]', 'NDR')));
SELECT asText(tgeometryFromHexEWKB(asHexEWKB(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}', 'NDR')));

SELECT asText(tgeometryFromHexEWKB(asHexEWKB(tgeometry 'Point(1 1)@2000-01-01', 'XDR')));
SELECT asText(tgeometryFromHexEWKB(asHexEWKB(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}', 'XDR')));
SELECT asText(tgeometryFromHexEWKB(asHexEWKB(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]', 'XDR')));
SELECT asText(tgeometryFromHexEWKB(asHexEWKB(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}', 'XDR')));

-------------------------------------------------------------------------------

SELECT asText(tgeographyFromHexEWKB(asHexEWKB(tgeography 'Point(1 1)@2000-01-01')));
SELECT asText(tgeographyFromHexEWKB(asHexEWKB(tgeography '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}')));
SELECT asText(tgeographyFromHexEWKB(asHexEWKB(tgeography '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]')));
SELECT asText(tgeographyFromHexEWKB(asHexEWKB(tgeography '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}')));

SELECT asText(tgeographyFromHexEWKB(asHexEWKB(tgeography 'Point(1 1)@2000-01-01', 'NDR')));
SELECT asText(tgeographyFromHexEWKB(asHexEWKB(tgeography '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}', 'NDR')));
SELECT asText(tgeographyFromHexEWKB(asHexEWKB(tgeography '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]', 'NDR')));
SELECT asText(tgeographyFromHexEWKB(asHexEWKB(tgeography '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}', 'NDR')));

SELECT asText(tgeographyFromHexEWKB(asHexEWKB(tgeography 'Point(1 1)@2000-01-01', 'XDR')));
SELECT asText(tgeographyFromHexEWKB(asHexEWKB(tgeography '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}', 'XDR')));
SELECT asText(tgeographyFromHexEWKB(asHexEWKB(tgeography '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]', 'XDR')));
SELECT asText(tgeographyFromHexEWKB(asHexEWKB(tgeography '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}', 'XDR')));

-------------------------------------------------------------------------------
-- Constructors
-------------------------------------------------------------------------------

SELECT asText(tgeometry('Point(1 1)'::geometry, '2000-01-01'::timestamp));

SELECT asText(tgeometrySeq(ARRAY['Point(1 1)@2000-01-01'::tgeometry, 'Linestring(1 1,2 2)@2000-01-02'::tgeometry], 'discrete'));

SELECT asText(tgeometrySeqSet(ARRAY[tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02]', '[Point(2 2)@2000-03-01, Point(2 2)@2000-04-01]']));

/* Errors */
SELECT asText(tgeometrySeq(ARRAY['Point(1 1)@2000-01-01'::tgeometry, 'Linestring(1 1,2 2)@2000-01-02'::tgeometry], 'linear', true, false));

-------------------------------------------------------------------------------

SELECT asText(tgeography('Point(1 1)'::geography, '2000-01-01'::timestamp));

SELECT asText(tgeographySeq(ARRAY['Point(1 1)@2000-01-01'::tgeography, 'Linestring(1 1,2 2)@2000-01-02'::tgeography], 'discrete'));

SELECT asText(tgeographySeqSet(ARRAY[tgeography '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02]', '[Point(2 2)@2000-03-01, Point(2 2)@2000-04-01]']));

/* Errors */
SELECT asText(tgeography('SRID=5676;Point(1 1)'::geography, '2000-01-01'::timestamp));

-------------------------------------------------------------------------------
-- Transformation functions
-------------------------------------------------------------------------------

SELECT asText(tgeometryInst(tgeometry 'Point(1 1)@2000-01-01'));
SELECT asText(setInterp(tgeometry 'Point(1 1)@2000-01-01', 'discrete'));
SELECT asText(tgeometrySeq(tgeometry 'Point(1 1)@2000-01-01'));
SELECT asText(tgeometrySeqSet(tgeometry 'Point(1 1)@2000-01-01'));

SELECT asText(tgeometryInst(tgeometry '{Point(1 1)@2000-01-01}'));
SELECT asText(setInterp(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}', 'discrete'));
SELECT asText(tgeometrySeq(tgeometry '{Point(1 1)@2000-01-01}'));
SELECT asText(tgeometrySeqSet(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}'));

SELECT asText(tgeometryInst(tgeometry '[Point(1 1)@2000-01-01]'));
SELECT asText(setInterp(tgeometry '[Point(1 1)@2000-01-01]', 'discrete'));
SELECT asText(tgeometrySeq(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]'));
SELECT asText(tgeometrySeqSet(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]'));

SELECT asText(tgeometryInst(tgeometry '{[Point(1 1)@2000-01-01]}'));
SELECT asText(setInterp(tgeometry '{[Point(1 1)@2000-01-01], [Point(2 2)@2000-01-04]}', 'discrete'));
SELECT asText(tgeometrySeq(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]}'));
SELECT asText(tgeometrySeqSet(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}'));

SELECT asText(round(tgeometry '{[Point(1 1.123456789)@2000-01-01, Linestring(1 1,2 2)@2000-01-02]}', 6));

-------------------------------------------------------------------------------

SELECT asText(tgeographyInst(tgeography 'Point(1 1)@2000-01-01'));
SELECT asText(setInterp(tgeography 'Point(1 1)@2000-01-01', 'discrete'));
SELECT asText(tgeographySeq(tgeography 'Point(1 1)@2000-01-01'));
SELECT asText(tgeographySeqSet(tgeography 'Point(1 1)@2000-01-01'));

SELECT asText(tgeographyInst(tgeography '{Point(1 1)@2000-01-01}'));
SELECT asText(setInterp(tgeography '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}', 'discrete'));
SELECT asText(tgeographySeq(tgeography '{Point(1 1)@2000-01-01}'));
SELECT asText(tgeographySeqSet(tgeography '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}'));

SELECT asText(tgeographyInst(tgeography '[Point(1 1)@2000-01-01]'));
SELECT asText(setInterp(tgeography '[Point(1 1)@2000-01-01]', 'discrete'));
SELECT asText(tgeographySeq(tgeography '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]'));
SELECT asText(tgeographySeqSet(tgeography '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]'));

SELECT asText(tgeographyInst(tgeography '{[Point(1 1)@2000-01-01]}'));
SELECT asText(setInterp(tgeography '{[Point(1 1)@2000-01-01], [Point(2 2)@2000-01-04]}', 'discrete'));
SELECT asText(tgeographySeq(tgeography '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]}'));
SELECT asText(tgeographySeqSet(tgeography '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}'));

SELECT asText(round(tgeography '{[Point(1 1.123456789)@2000-01-01, Linestring(1 1,2 2)@2000-01-02]}', 6));

-------------------------------------------------------------------------------
-- Append functions
-------------------------------------------------------------------------------

SELECT asText(appendInstant(tgeometry 'Point(1 1)@2000-01-01', tgeometry 'Linestring(1 1,2 2)@2000-01-02'));
SELECT asText(appendInstant(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}', tgeometry 'Point(1 1)@2000-01-04'));
SELECT asText(appendInstant(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]', tgeometry 'Point(1 1)@2000-01-04'));
SELECT asText(appendInstant(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05] }', tgeometry 'Point(2 2)@2000-01-06'));

-------------------------------------------------------------------------------

SELECT asText(appendSequence(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02}', tgeometry '{Point(3 3)@2000-01-03}'));
SELECT asText(appendSequence(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02]', tgeometry '[Point(1 1)@2000-01-03]'));

-------------------------------------------------------------------------------
-- Cast functions
-------------------------------------------------------------------------------

-- SELECT asText(round(tgeometry 'Point(1 1)@2000-01-01'::tgeompoint, 6));
-- SELECT asText(round(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}'::tgeompoint, 6));
-- SELECT asText(round(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]'::tgeompoint, 6));
-- SELECT asText(round(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05] }'::tgeompoint, 6));

-- SELECT round(tgeometry 'Point(1 1)@2000-01-01'::tfloat, 6);
-- SELECT round(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}'::tfloat, 6);
-- SELECT round(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]'::tfloat, 6);
-- SELECT round(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05] }'::tfloat, 6);

-- SELECT asText(round((tgeometry 'Point(1 1)@2000-01-01'::tgeompoint)::tgeometry, 6));
-- SELECT asText(round((tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}'::tgeompoint)::tgeometry, 6));
-- SELECT asText(round((tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]'::tgeompoint)::tgeometry, 6));
-- SELECT asText(round((tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05] }'::tgeompoint)::tgeometry, 6));
-- NULL
-- SELECT asText(tgeompoint 'SRID=5676;Point(-1 -1)@2000-01-01'::tgeometry;
-- SELECT asText(tgeompoint 'SRID=5676;{POINT(48.7186629128278 77.7640705101509)@2000-01-01, POINT(48.71 77.76)@2000-01-02}'::tgeometry;
-- SELECT asText(tgeompoint 'SRID=5676;[POINT(48.7186629128278 77.7640705101509)@2000-01-01, POINT(48.71 77.76)@2000-01-02]'::tgeometry;
-- SELECT asText(tgeompoint 'SRID=5676;{[POINT(62.7866330839742 80.1435561997142)@2000-01-01, POINT(62.7866330839742 80.1435561997142)@2000-01-02],[POINT(48.7186629128278 77.7640705101509)@2000-01-03, POINT(48.71 77.76)@2000-01-04]}'::tgeometry;

-------------------------------------------------------------------------------
-- Accessor Functions
-------------------------------------------------------------------------------

SELECT tempSubtype(tgeometry 'Point(1 1)@2000-01-01');
SELECT tempSubtype(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}');
SELECT tempSubtype(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]');
SELECT tempSubtype(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}');

SELECT memSize(tgeometry 'Point(1 1)@2000-01-01');
SELECT memSize(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}');
SELECT memSize(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]');
SELECT memSize(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}');

SELECT ST_AsText(getValue(tgeometry 'Point(1 1)@2000-01-01'));

SELECT asText(getValues(tgeometry 'Point(1 1)@2000-01-01'));
SELECT asText(getValues(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}'));
SELECT asText(getValues(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]'));
SELECT asText(getValues(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}'));

SELECT getTime(tgeometry 'Point(1 1)@2000-01-01');
SELECT getTime(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}');
SELECT getTime(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]');
SELECT getTime(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}');

SELECT timeSpan(tgeometry 'Point(1 1)@2000-01-01');
SELECT timeSpan(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}');
SELECT timeSpan(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]');
SELECT timeSpan(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}');

SELECT duration(tgeometry 'Point(1 1)@2000-01-01', true);
SELECT duration(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}', true);
SELECT duration(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]', true);
SELECT duration(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}', true);

SELECT duration(tgeometry 'Point(1 1)@2000-01-01');
SELECT duration(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}');
SELECT duration(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]');
SELECT duration(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}');

SELECT getTimestamp(tgeometry 'Point(1 1)@2000-01-01');

-------------------------------------------------------------------------------
-- Shift and scale functions
-------------------------------------------------------------------------------

SELECT asText(shiftTime(tgeometry 'Point(1 1)@2000-01-01', '5 min'));
SELECT asText(shiftTime(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}', '5 min'));
SELECT asText(shiftTime(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]', '5 min'));
SELECT asText(shiftTime(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}', '5 min'));

SELECT asText(scaleTime(tgeometry 'Point(1 1)@2000-01-01', '1 day'));
SELECT asText(scaleTime(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}', '1 day'));
SELECT asText(scaleTime(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]', '1 day'));
SELECT asText(scaleTime(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}', '1 day'));

-------------------------------------------------------------------------------
-- Ever/always comparison functions
-------------------------------------------------------------------------------

SELECT tgeometry 'Point(1 1)@2000-01-01' ?= 'Point(1 1)';
SELECT tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}' ?= 'Point(1 1)';
SELECT tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]' ?= 'Point(1 1)';
SELECT tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}' ?= 'Point(1 1)';

SELECT tgeometry 'Point(1 1)@2000-01-01' %= 'Point(1 1)';
SELECT tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}' %= 'Point(1 1)';
SELECT tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]' %= 'Point(1 1)';
SELECT tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}' %= 'Point(1 1)';

SELECT asText(shiftTime(tgeometry 'Point(1 1)@2000-01-01', '1 year'::interval));
SELECT asText(shiftTime(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}', '1 year'::interval));
SELECT asText(shiftTime(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]', '1 year'::interval));
SELECT asText(shiftTime(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}', '1 year'::interval));

SELECT ST_AsText(startValue(tgeometry 'Point(1 1)@2000-01-01'));
SELECT ST_AsText(startValue(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}'));
SELECT ST_AsText(startValue(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]'));
SELECT ST_AsText(startValue(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}'));

SELECT ST_AsText(endValue(tgeometry 'Point(1 1)@2000-01-01'));
SELECT ST_AsText(endValue(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}'));
SELECT ST_AsText(endValue(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]'));
SELECT ST_AsText(endValue(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}'));

SELECT ST_AsText(valueN(tgeometry 'Point(1 1)@2000-01-01', 1));
SELECT ST_AsText(valueN(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}', 1));
SELECT ST_AsText(valueN(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]', 1));
SELECT ST_AsText(valueN(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}', 1));

SELECT numInstants(tgeometry 'Point(1 1)@2000-01-01');
SELECT numInstants(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}');
SELECT numInstants(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]');
SELECT numInstants(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}');

SELECT asText(startInstant(tgeometry 'Point(1 1)@2000-01-01'));
SELECT asText(startInstant(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}'));
SELECT asText(startInstant(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]'));
SELECT asText(startInstant(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}'));

SELECT asText(endInstant(tgeometry 'Point(1 1)@2000-01-01'));
SELECT asText(endInstant(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}'));
SELECT asText(endInstant(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]'));
SELECT asText(endInstant(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}'));

SELECT asText(instantN(tgeometry 'Point(1 1)@2000-01-01', 1));
SELECT asText(instantN(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}', 1));
SELECT asText(instantN(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]', 1));
SELECT asText(instantN(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}', 1));

SELECT asText(instants(tgeometry 'Point(1 1)@2000-01-01'));
SELECT asText(instants(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}'));
SELECT asText(instants(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]'));
SELECT asText(instants(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}'));

SELECT numTimestamps(tgeometry 'Point(1 1)@2000-01-01');
SELECT numTimestamps(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}');
SELECT numTimestamps(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]');
SELECT numTimestamps(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}');

SELECT startTimestamp(tgeometry 'Point(1 1)@2000-01-01');
SELECT startTimestamp(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}');
SELECT startTimestamp(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]');
SELECT startTimestamp(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}');

SELECT endTimestamp(tgeometry 'Point(1 1)@2000-01-01');
SELECT endTimestamp(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}');
SELECT endTimestamp(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]');
SELECT endTimestamp(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}');

SELECT timestampN(tgeometry 'Point(1 1)@2000-01-01', 1);
SELECT timestampN(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}', 1);
SELECT timestampN(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]', 1);
SELECT timestampN(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}', 1);

SELECT timestamps(tgeometry 'Point(1 1)@2000-01-01');
SELECT timestamps(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}');
SELECT timestamps(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]');
SELECT timestamps(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}');

SELECT numSequences(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}');
SELECT asText(startSequence(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}'));
SELECT asText(endSequence(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}'));
SELECT asText(sequenceN(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}', 1));
SELECT asText(sequences(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}'));

SELECT startTimestamp(tgeometry 'Point(1 1)@2000-01-01');
SELECT startTimestamp(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}');
SELECT startTimestamp(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]');
SELECT startTimestamp(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}');

SELECT endTimestamp(tgeometry 'Point(1 1)@2000-01-01');
SELECT endTimestamp(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}');
SELECT endTimestamp(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]');
SELECT endTimestamp(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}');

SELECT timestampN(tgeometry 'Point(1 1)@2000-01-01', 1);
SELECT timestampN(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}', 1);
SELECT timestampN(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]', 1);
SELECT timestampN(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}', 1);

SELECT timestamps(tgeometry 'Point(1 1)@2000-01-01');
SELECT timestamps(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}');
SELECT timestamps(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]');
SELECT timestamps(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}');

-------------------------------------------------------------------------------
-- Restriction Functions
-------------------------------------------------------------------------------

SELECT asText(atValues(tgeometry 'Point(1 1)@2000-01-01', geometry 'Point(1 1)'));
SELECT asText(atValues(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}', geometry 'Point(1 1)'));
SELECT asText(atValues(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]', geometry 'Point(1 1)'));
SELECT asText(atValues(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}', geometry 'Point(1 1)'));

SELECT asText(minusValues(tgeometry 'Point(1 1)@2000-01-01', geometry 'Point(1 1)'));
SELECT asText(minusValues(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}', geometry 'Point(1 1)'));
SELECT asText(minusValues(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]', geometry 'Point(1 1)'));
SELECT asText(minusValues(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}', geometry 'Point(1 1)'));

SELECT asText(atValues(tgeometry 'Point(1 1)@2000-01-01', geomset '{"Point(1 1)"}'));
SELECT asText(atValues(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}', geomset '{"Point(1 1)"}'));
SELECT asText(atValues(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]', geomset '{"Point(1 1)"}'));
SELECT asText(atValues(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}', geomset '{"Point(1 1)"}'));

SELECT asText(minusValues(tgeometry 'Point(1 1)@2000-01-01', geomset '{"Point(1 1)"}'));
SELECT asText(minusValues(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}', geomset '{"Point(1 1)"}'));
SELECT asText(minusValues(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]', geomset '{"Point(1 1)"}'));
SELECT asText(minusValues(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}', geomset '{"Point(1 1)"}'));

SELECT asText(atTime(tgeometry 'Point(1 1)@2000-01-01', timestamptz '2000-01-01'));
SELECT asText(atTime(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}', timestamptz '2000-01-01'));
SELECT asText(atTime(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]', timestamptz '2000-01-01'));
SELECT asText(atTime(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}', timestamptz '2000-01-01'));

SELECT ST_AsText(valueAtTimestamp(tgeometry 'Point(1 1)@2000-01-01', '2000-01-01'));
SELECT ST_AsText(valueAtTimestamp(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}', '2000-01-01'));
SELECT ST_AsText(valueAtTimestamp(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]', '2000-01-01'));
SELECT ST_AsText(valueAtTimestamp(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}', '2000-01-01'));

SELECT asText(minusTime(tgeometry 'Point(1 1)@2000-01-01', timestamptz '2000-01-01'));
SELECT asText(minusTime(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}', timestamptz '2000-01-01'));
SELECT asText(minusTime(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]', timestamptz '2000-01-01'));
SELECT asText(minusTime(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}', timestamptz '2000-01-01'));

SELECT asText(atTime(tgeometry 'Point(1 1)@2000-01-01', tstzset '{2000-01-01}'));
SELECT asText(atTime(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}', tstzset '{2000-01-01}'));
SELECT asText(atTime(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]', tstzset '{2000-01-01}'));
SELECT asText(atTime(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}', tstzset '{2000-01-01}'));

SELECT asText(minusTime(tgeometry 'Point(1 1)@2000-01-01', tstzset '{2000-01-01}'));
SELECT asText(minusTime(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}', tstzset '{2000-01-01}'));
SELECT asText(minusTime(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]', tstzset '{2000-01-01}'));
SELECT asText(minusTime(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}', tstzset '{2000-01-01}'));

SELECT asText(atTime(tgeometry 'Point(1 1)@2000-01-01', tstzspan '[2000-01-01, 2000-01-02]'));
SELECT asText(atTime(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}', tstzspan '[2000-01-01, 2000-01-02]'));
SELECT asText(atTime(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]', tstzspan '[2000-01-01, 2000-01-02]'));
SELECT asText(atTime(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}', tstzspan '[2000-01-01, 2000-01-02]'));

SELECT asText(minusTime(tgeometry 'Point(1 1)@2000-01-01', tstzspan '[2000-01-01, 2000-01-02]'));
SELECT asText(minusTime(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}', tstzspan '[2000-01-01, 2000-01-02]'));
SELECT asText(minusTime(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]', tstzspan '[2000-01-01, 2000-01-02]'));
SELECT asText(minusTime(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}', tstzspan '[2000-01-01, 2000-01-02]'));

SELECT asText(atTime(tgeometry 'Point(1 1)@2000-01-01', tstzspanset '{[2000-01-01, 2000-01-02]}'));
SELECT asText(atTime(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}', tstzspanset '{[2000-01-01, 2000-01-02]}'));
SELECT asText(atTime(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]', tstzspanset '{[2000-01-01, 2000-01-02]}'));
SELECT asText(atTime(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}', tstzspanset '{[2000-01-01, 2000-01-02]}'));

SELECT asText(minusTime(tgeometry 'Point(1 1)@2000-01-01', tstzspanset '{[2000-01-01, 2000-01-02]}'));
SELECT asText(minusTime(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}', tstzspanset '{[2000-01-01, 2000-01-02]}'));
SELECT asText(minusTime(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]', tstzspanset '{[2000-01-01, 2000-01-02]}'));
SELECT asText(minusTime(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}', tstzspanset '{[2000-01-01, 2000-01-02]}'));

-------------------------------------------------------------------------------
-- Modification functions
-------------------------------------------------------------------------------

SELECT asText(deleteTime(tgeometry 'Point(1 1)@2000-01-01', timestamptz '2000-01-01'));
SELECT asText(deleteTime(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}', timestamptz '2000-01-01'));
SELECT asText(deleteTime(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]', timestamptz '2000-01-01'));
SELECT asText(deleteTime(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}', timestamptz '2000-01-01'));

SELECT asText(deleteTime(tgeometry 'Point(1 1)@2000-01-01', tstzset '{2000-01-01}'));
SELECT asText(deleteTime(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}', tstzset '{2000-01-01}'));
SELECT asText(deleteTime(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]', tstzset '{2000-01-01}'));
SELECT asText(deleteTime(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}', tstzset '{2000-01-01}'));

SELECT asText(deleteTime(tgeometry 'Point(1 1)@2000-01-01', tstzspan '[2000-01-01, 2000-01-02]'));
SELECT asText(deleteTime(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}', tstzspan '[2000-01-01, 2000-01-02]'));
SELECT asText(deleteTime(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]', tstzspan '[2000-01-01, 2000-01-02]'));
SELECT asText(deleteTime(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}', tstzspan '[2000-01-01, 2000-01-02]'));

SELECT asText(deleteTime(tgeometry 'Point(1 1)@2000-01-01', tstzspanset '{[2000-01-01, 2000-01-02]}'));
SELECT asText(deleteTime(tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}', tstzspanset '{[2000-01-01, 2000-01-02]}'));
SELECT asText(deleteTime(tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]', tstzspanset '{[2000-01-01, 2000-01-02]}'));
SELECT asText(deleteTime(tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}', tstzspanset '{[2000-01-01, 2000-01-02]}'));

-------------------------------------------------------------------------------
-- Comparison functions and B-tree indexing
-------------------------------------------------------------------------------

SELECT tgeometry 'Point(1 1)@2000-01-01' = tgeometry 'Point(1 1)@2000-01-01';
SELECT tgeometry 'Point(1 1)@2000-01-01' = tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}';
SELECT tgeometry 'Point(1 1)@2000-01-01' = tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]';
SELECT tgeometry 'Point(1 1)@2000-01-01' = tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}';

SELECT tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}' = tgeometry 'Point(1 1)@2000-01-01';
SELECT tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}' = tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}';
SELECT tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}' = tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]';
SELECT tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}' = tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}';

SELECT tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]' = tgeometry 'Point(1 1)@2000-01-01';
SELECT tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]' = tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}';
SELECT tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]' = tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]';
SELECT tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]' = tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}';

SELECT tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}' = tgeometry 'Point(1 1)@2000-01-01';
SELECT tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}' = tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}';
SELECT tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}' = tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]';
SELECT tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}' = tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}';

SELECT tgeometry 'Point(1 1)@2000-01-01' != tgeometry 'Point(1 1)@2000-01-01';
SELECT tgeometry 'Point(1 1)@2000-01-01' != tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}';
SELECT tgeometry 'Point(1 1)@2000-01-01' != tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]';
SELECT tgeometry 'Point(1 1)@2000-01-01' != tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}';

SELECT tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}' != tgeometry 'Point(1 1)@2000-01-01';
SELECT tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}' != tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}';
SELECT tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}' != tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]';
SELECT tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}' != tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}';

SELECT tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]' != tgeometry 'Point(1 1)@2000-01-01';
SELECT tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]' != tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}';
SELECT tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]' != tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]';
SELECT tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]' != tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}';

SELECT tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}' != tgeometry 'Point(1 1)@2000-01-01';
SELECT tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}' != tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}';
SELECT tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}' != tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]';
SELECT tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}' != tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}';

SELECT tgeometry 'Point(1 1)@2000-01-01' < tgeometry 'Point(1 1)@2000-01-01';
SELECT tgeometry 'Point(1 1)@2000-01-01' < tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}';
SELECT tgeometry 'Point(1 1)@2000-01-01' < tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]';
SELECT tgeometry 'Point(1 1)@2000-01-01' < tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}';

SELECT tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}' < tgeometry 'Point(1 1)@2000-01-01';
SELECT tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}' < tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}';
SELECT tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}' < tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]';
SELECT tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}' < tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}';

SELECT tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]' < tgeometry 'Point(1 1)@2000-01-01';
SELECT tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]' < tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}';
SELECT tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]' < tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]';
SELECT tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]' < tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}';

SELECT tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}' < tgeometry 'Point(1 1)@2000-01-01';
SELECT tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}' < tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}';
SELECT tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}' < tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]';
SELECT tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}' < tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}';

SELECT tgeometry 'Point(1 1)@2000-01-01' <= tgeometry 'Point(1 1)@2000-01-01';
SELECT tgeometry 'Point(1 1)@2000-01-01' <= tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}';
SELECT tgeometry 'Point(1 1)@2000-01-01' <= tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]';
SELECT tgeometry 'Point(1 1)@2000-01-01' <= tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}';

SELECT tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}' <= tgeometry 'Point(1 1)@2000-01-01';
SELECT tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}' <= tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}';
SELECT tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}' <= tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]';
SELECT tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}' <= tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}';

SELECT tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]' <= tgeometry 'Point(1 1)@2000-01-01';
SELECT tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]' <= tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}';
SELECT tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]' <= tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]';
SELECT tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]' <= tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}';

SELECT tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}' <= tgeometry 'Point(1 1)@2000-01-01';
SELECT tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}' <= tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}';
SELECT tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}' <= tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]';
SELECT tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}' <= tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}';

SELECT tgeometry 'Point(1 1)@2000-01-01' > tgeometry 'Point(1 1)@2000-01-01';
SELECT tgeometry 'Point(1 1)@2000-01-01' > tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}';
SELECT tgeometry 'Point(1 1)@2000-01-01' > tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]';
SELECT tgeometry 'Point(1 1)@2000-01-01' > tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}';

SELECT tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}' > tgeometry 'Point(1 1)@2000-01-01';
SELECT tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}' > tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}';
SELECT tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}' > tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]';
SELECT tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}' > tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}';

SELECT tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]' > tgeometry 'Point(1 1)@2000-01-01';
SELECT tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]' > tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}';
SELECT tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]' > tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]';
SELECT tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]' > tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}';

SELECT tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}' > tgeometry 'Point(1 1)@2000-01-01';
SELECT tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}' > tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}';
SELECT tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}' > tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]';
SELECT tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}' > tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}';

SELECT tgeometry 'Point(1 1)@2000-01-01' >= tgeometry 'Point(1 1)@2000-01-01';
SELECT tgeometry 'Point(1 1)@2000-01-01' >= tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}';
SELECT tgeometry 'Point(1 1)@2000-01-01' >= tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]';
SELECT tgeometry 'Point(1 1)@2000-01-01' >= tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}';

SELECT tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}' >= tgeometry 'Point(1 1)@2000-01-01';
SELECT tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}' >= tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}';
SELECT tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}' >= tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]';
SELECT tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}' >= tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}';

SELECT tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]' >= tgeometry 'Point(1 1)@2000-01-01';
SELECT tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]' >= tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}';
SELECT tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]' >= tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]';
SELECT tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]' >= tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}';

SELECT tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}' >= tgeometry 'Point(1 1)@2000-01-01';
SELECT tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}' >= tgeometry '{Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03}';
SELECT tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}' >= tgeometry '[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03]';
SELECT tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}' >= tgeometry '{[Point(1 1)@2000-01-01, Linestring(1 1,2 2)@2000-01-02, Point(1 1)@2000-01-03], [Point(2 2)@2000-01-04, Point(2 2)@2000-01-05]}';

-------------------------------------------------------------------------------/
