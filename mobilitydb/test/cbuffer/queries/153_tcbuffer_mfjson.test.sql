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
-- asMFJSON / tcbufferFromMFJSON round-trip across all temporal subtypes
-------------------------------------------------------------------------------

SELECT asEWKT(tcbufferFromMFJSON(asMFJSON(tcbuffer 'Cbuffer(Point(1 1), 0.5)@2000-01-01')));
SELECT asEWKT(tcbufferFromMFJSON(asMFJSON(tcbuffer '{Cbuffer(Point(1 1), 0.3)@2000-01-01, Cbuffer(Point(1 1), 0.5)@2000-01-02, Cbuffer(Point(1 1), 0.5)@2000-01-03}')));
SELECT asEWKT(tcbufferFromMFJSON(asMFJSON(tcbuffer '[Cbuffer(Point(1 1), 0.2)@2000-01-01, Cbuffer(Point(1 1), 0.4)@2000-01-02, Cbuffer(Point(1 1), 0.5)@2000-01-03]')));
SELECT asEWKT(tcbufferFromMFJSON(asMFJSON(tcbuffer '{[Cbuffer(Point(1 1), 0.2)@2000-01-01, Cbuffer(Point(1 1), 0.4)@2000-01-02, Cbuffer(Point(1 1), 0.5)@2000-01-03], [Cbuffer(Point(2 2), 0.6)@2000-01-04, Cbuffer(Point(2 2), 0.6)@2000-01-05]}')));
SELECT asEWKT(tcbufferFromMFJSON(asMFJSON(tcbuffer 'Interp=Step;[Cbuffer(Point(1 1), 0.2)@2000-01-01, Cbuffer(Point(1 1), 0.4)@2000-01-02, Cbuffer(Point(1 1), 0.5)@2000-01-03]')));

-- SRID preservation
SELECT asEWKT(tcbufferFromMFJSON(asMFJSON(tcbuffer 'SRID=4326;Cbuffer(Point(1.123456 2.654321), 0.5)@2000-01-01', 1, 0, 2)));
SELECT asEWKT(tcbufferFromMFJSON(asMFJSON(tcbuffer 'SRID=3812;[Cbuffer(Point(1 1), 0.2)@2000-01-01, Cbuffer(Point(1 1), 0.4)@2000-01-02]', 1, 1, 6)));

-------------------------------------------------------------------------------
/* Errors */
-------------------------------------------------------------------------------

-- Malformed JSON
SELECT tcbufferFromMFJSON('ABC');
-- Wrong moving-feature type tag
SELECT tcbufferFromMFJSON('{"type":"MovingPoint","coordinates":[1,1],"datetimes":["2000-01-01T00:00:00+01"],"interpolation":"None"}');
-- Unknown moving-feature type tag
SELECT tcbufferFromMFJSON('{"type":"XXX","values":[{"point":[1,1],"radius":0.5}],"datetimes":["2000-01-01T00:00:00+01"],"interpolation":"None"}');
-- 3D point in payload (cbuffer is 2D-only)
SELECT tcbufferFromMFJSON('{"type":"MovingCircularBuffer","values":[{"point":[1,1,1],"radius":0.5}],"datetimes":["2000-01-01T00:00:00+01"],"interpolation":"None"}');
-- Negative radius
SELECT tcbufferFromMFJSON('{"type":"MovingCircularBuffer","values":[{"point":[1,1],"radius":-0.5}],"datetimes":["2000-01-01T00:00:00+01"],"interpolation":"None"}');
-- Missing radius
SELECT tcbufferFromMFJSON('{"type":"MovingCircularBuffer","values":[{"point":[1,1]}],"datetimes":["2000-01-01T00:00:00+01"],"interpolation":"None"}');
-- Missing point
SELECT tcbufferFromMFJSON('{"type":"MovingCircularBuffer","values":[{"radius":0.5}],"datetimes":["2000-01-01T00:00:00+01"],"interpolation":"None"}');
-- Missing interpolation
SELECT tcbufferFromMFJSON('{"type":"MovingCircularBuffer","values":[{"point":[1,1],"radius":0.5}],"datetimes":["2000-01-01T00:00:00+01"]}');
-- Bad interpolation
SELECT tcbufferFromMFJSON('{"type":"MovingCircularBuffer","values":[{"point":[1,1],"radius":0.5}],"datetimes":["2000-01-01T00:00:00+01"],"interpolation":"XXX"}');

-------------------------------------------------------------------------------
