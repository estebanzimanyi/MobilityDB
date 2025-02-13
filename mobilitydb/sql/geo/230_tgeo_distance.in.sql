/*****************************************************************************
 *
 * This MobilityDB code is provided under The PostgreSQL License.
 * Copyright (c) 2016-2024, Université libre de Bruxelles and MobilityDB
 * contributors
 *
 * MobilityDB includes portions of PostGIS version 3 source code released
 * under the GNU General Public License (GPLv2 or later).
 * Copyright (c) 2001-2024, PostGIS contributors
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without a written
 * agreement is hereby granted, provided that the above copyright notice and
 * this paragraph and the following two paragraphs appear in all copies.
 *
 * IN NO EVENT SHALL UNIVERSITE LIBRE DE BRUXELLES BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING
 * LOST PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION,
 * EVEN IF UNIVERSITE LIBRE DE BRUXELLES HAS BEEN ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * UNIVERSITE LIBRE DE BRUXELLES SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE. THE SOFTWARE PROVIDED HEREUNDER IS ON
 * AN "AS IS" BASIS, AND UNIVERSITE LIBRE DE BRUXELLES HAS NO OBLIGATIONS TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 *****************************************************************************/

/**
 * Distance functions for temporal circular buffers.
 */

/*****************************************************************************
 * Temporal distance functions
 *****************************************************************************/

CREATE FUNCTION tDistance(geometry, tgeometry)
  RETURNS tfloat
  AS 'MODULE_PATHNAME', 'Distance_geo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tDistance(tgeometry, geometry)
  RETURNS tfloat
  AS 'MODULE_PATHNAME', 'Distance_tgeo_geo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tDistance(tgeometry, tgeometry)
  RETURNS tfloat
  AS 'MODULE_PATHNAME', 'Distance_tgeo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR <-> (
  PROCEDURE = tDistance,
  LEFTARG = geometry,
  RIGHTARG = tgeometry,
  COMMUTATOR = <->
);
CREATE OPERATOR <-> (
  PROCEDURE = tDistance,
  LEFTARG = tgeometry,
  RIGHTARG = geometry,
  COMMUTATOR = <->
);
CREATE OPERATOR <-> (
  PROCEDURE = tDistance,
  LEFTARG = tgeometry,
  RIGHTARG = tgeometry,
  COMMUTATOR = <->
);

/*****************************************************************************
 * Nearest approach instant/distance and shortest line functions
 *****************************************************************************/

CREATE FUNCTION nearestApproachInstant(geometry, tgeometry)
  RETURNS tgeometry
  AS 'MODULE_PATHNAME', 'NAI_geo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION nearestApproachInstant(stbox, tgeometry)
  RETURNS tgeometry
  AS 'SELECT @extschema@.nearestApproachInstant(geometry($1), $2)'
  LANGUAGE SQL IMMUTABLE PARALLEL SAFE;
CREATE FUNCTION nearestApproachInstant(tgeometry, geometry)
  RETURNS tgeometry
  AS 'MODULE_PATHNAME', 'NAI_tgeo_geo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION nearestApproachInstant(tgeometry, stbox)
  RETURNS tgeometry
  AS 'SELECT @extschema@.nearestApproachInstant($1, geometry($2))'
  LANGUAGE SQL IMMUTABLE PARALLEL SAFE;
CREATE FUNCTION nearestApproachInstant(tgeometry, tgeometry)
  RETURNS tgeometry
  AS 'MODULE_PATHNAME', 'NAI_tgeo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

/*****************************************************************************/

CREATE FUNCTION nearestApproachDistance(geometry, tgeometry)
  RETURNS float
  AS 'MODULE_PATHNAME', 'NAD_geo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION nearestApproachDistance(stbox, tgeometry)
  RETURNS float
  AS 'SELECT @extschema@.nearestApproachDistance(geometry($1), $2)'
  LANGUAGE SQL IMMUTABLE PARALLEL SAFE;
CREATE FUNCTION nearestApproachDistance(tgeometry, geometry)
  RETURNS float
  AS 'MODULE_PATHNAME', 'NAD_tgeo_geo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION nearestApproachDistance(tgeometry, stbox)
  RETURNS float
  AS 'SELECT @extschema@.nearestApproachDistance($1, geometry($2))'
  LANGUAGE SQL IMMUTABLE PARALLEL SAFE;
CREATE FUNCTION nearestApproachDistance(tgeometry, tgeometry)
  RETURNS float
  AS 'MODULE_PATHNAME', 'NAD_tgeo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR |=| (
  LEFTARG = geometry, RIGHTARG = tgeometry,
  PROCEDURE = nearestApproachDistance,
  COMMUTATOR = '|=|'
);
CREATE OPERATOR |=| (
  LEFTARG = tgeometry, RIGHTARG = geometry,
  PROCEDURE = nearestApproachDistance,
  COMMUTATOR = '|=|'
);
CREATE OPERATOR |=| (
  LEFTARG = stbox, RIGHTARG = tgeometry,
  PROCEDURE = nearestApproachDistance,
  COMMUTATOR = '|=|'
);
CREATE OPERATOR |=| (
  LEFTARG = tgeometry, RIGHTARG = stbox,
  PROCEDURE = nearestApproachDistance,
  COMMUTATOR = '|=|'
);
CREATE OPERATOR |=| (
  LEFTARG = tgeometry, RIGHTARG = tgeometry,
  PROCEDURE = nearestApproachDistance,
  COMMUTATOR = '|=|'
);

/*****************************************************************************/

CREATE FUNCTION shortestLine(geometry, tgeometry)
  RETURNS geometry
  AS 'MODULE_PATHNAME', 'Shortestline_geo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION shortestLine(stbox, tgeometry)
  RETURNS geometry
  AS 'SELECT @extschema@.shortestLine(geometry($1), $2)'
  LANGUAGE SQL IMMUTABLE PARALLEL SAFE;
CREATE FUNCTION shortestLine(tgeometry, geometry)
  RETURNS geometry
  AS 'MODULE_PATHNAME', 'Shortestline_tgeo_geo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION shortestLine(tgeometry, stbox)
  RETURNS geometry
  AS 'SELECT @extschema@.shortestLine($1, geometry($2))'
  LANGUAGE SQL IMMUTABLE PARALLEL SAFE;
CREATE FUNCTION shortestLine(tgeometry, tgeometry)
  RETURNS geometry
  AS 'MODULE_PATHNAME', 'Shortestline_tgeo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

/*****************************************************************************/
