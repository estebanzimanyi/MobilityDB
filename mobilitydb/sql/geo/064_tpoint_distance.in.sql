/*****************************************************************************
 *
 * This MobilityDB code is provided under The PostgreSQL License.
 * Copyright (c) 2016-2025, Université libre de Bruxelles and MobilityDB
 * contributors
 *
 * MobilityDB includes portions of PostGIS version 3 source code released
 * under the GNU General Public License (GPLv2 or later).
 * Copyright (c) 2001-2025, PostGIS contributors
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
 * @file
 * @brief Distance functions for temporal points
 */

/*****************************************************************************
 * Distance functions
 *****************************************************************************/

CREATE FUNCTION tdistance(geometry(Point), tgeompoint)
  RETURNS tfloat
  AS 'MODULE_PATHNAME', 'Tdistance_geo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tdistance(tgeompoint, geometry(Point))
  RETURNS tfloat
  AS 'MODULE_PATHNAME', 'Tdistance_tgeo_geo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tdistance(tgeompoint, tgeompoint)
  RETURNS tfloat
  AS 'MODULE_PATHNAME', 'Tdistance_tgeo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR <-> (
  PROCEDURE = tdistance,
  LEFTARG = geometry, RIGHTARG = tgeompoint,
  COMMUTATOR = <->
);
CREATE OPERATOR <-> (
  PROCEDURE = tdistance,
  LEFTARG = tgeompoint, RIGHTARG = geometry,
  COMMUTATOR = <->
);
CREATE OPERATOR <-> (
  PROCEDURE = tdistance,
  LEFTARG = tgeompoint, RIGHTARG = tgeompoint,
  COMMUTATOR = <->
);

/*****************************************************************************/

CREATE FUNCTION tdistance(geography(Point), tgeogpoint)
  RETURNS tfloat
  AS 'MODULE_PATHNAME', 'Tdistance_geo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tdistance(tgeogpoint, geography(Point))
  RETURNS tfloat
  AS 'MODULE_PATHNAME', 'Tdistance_tgeo_geo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tdistance(tgeogpoint, tgeogpoint)
  RETURNS tfloat
  AS 'MODULE_PATHNAME', 'Tdistance_tgeo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR <-> (
  PROCEDURE = tdistance,
  LEFTARG = geography, RIGHTARG = tgeogpoint,
  COMMUTATOR = <->
);
CREATE OPERATOR <-> (
  PROCEDURE = tdistance,
  LEFTARG = tgeogpoint, RIGHTARG = geography,
  COMMUTATOR = <->
);
CREATE OPERATOR <-> (
  PROCEDURE = tdistance,
  LEFTARG = tgeogpoint, RIGHTARG = tgeogpoint,
  COMMUTATOR = <->
);

/*****************************************************************************
 * Nearest approach instant/distance and shortest line functions
 *****************************************************************************/

CREATE FUNCTION nearestApproachInstant(geometry, tgeompoint)
  RETURNS tgeompoint
  AS 'MODULE_PATHNAME', 'NAI_geo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION nearestApproachInstant(tgeompoint, geometry)
  RETURNS tgeompoint
  AS 'MODULE_PATHNAME', 'NAI_tgeo_geo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION nearestApproachInstant(tgeompoint, tgeompoint)
  RETURNS tgeompoint
  AS 'MODULE_PATHNAME', 'NAI_tgeo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION nearestApproachInstant(geography, tgeogpoint)
  RETURNS tgeogpoint
  AS 'MODULE_PATHNAME', 'NAI_geo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION nearestApproachInstant(tgeogpoint, geography)
  RETURNS tgeogpoint
  AS 'MODULE_PATHNAME', 'NAI_tgeo_geo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION nearestApproachInstant(tgeogpoint, tgeogpoint)
  RETURNS tgeogpoint
  AS 'MODULE_PATHNAME', 'NAI_tgeo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION nearestApproachDistance(geometry, tgeompoint)
  RETURNS float
  AS 'MODULE_PATHNAME', 'NAD_geo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION nearestApproachDistance(tgeompoint, geometry)
  RETURNS float
  AS 'MODULE_PATHNAME', 'NAD_tgeo_geo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION nearestApproachDistance(stbox, geometry)
  RETURNS float
  AS 'MODULE_PATHNAME', 'NAD_stbox_geo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION nearestApproachDistance(geometry, stbox)
  RETURNS float
  AS 'MODULE_PATHNAME', 'NAD_geo_stbox'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION nearestApproachDistance(stbox, tgeompoint)
  RETURNS float
  AS 'MODULE_PATHNAME', 'NAD_stbox_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION nearestApproachDistance(tgeompoint, stbox)
  RETURNS float
  AS 'MODULE_PATHNAME', 'NAD_tgeo_stbox'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION nearestApproachDistance(stbox, stbox)
  RETURNS float
  AS 'MODULE_PATHNAME', 'NAD_stbox_stbox'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION nearestApproachDistance(tgeompoint, tgeompoint)
  RETURNS float
  AS 'MODULE_PATHNAME', 'NAD_tgeo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION nearestApproachDistance(geography, tgeogpoint)
  RETURNS float
  AS 'MODULE_PATHNAME', 'NAD_geo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION nearestApproachDistance(tgeogpoint, geography)
  RETURNS float
  AS 'MODULE_PATHNAME', 'NAD_tgeo_geo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION nearestApproachDistance(stbox, geography)
  RETURNS float
  AS 'MODULE_PATHNAME', 'NAD_stbox_geo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION nearestApproachDistance(geography, stbox)
  RETURNS float
  AS 'MODULE_PATHNAME', 'NAD_geo_stbox'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION nearestApproachDistance(stbox, tgeogpoint)
  RETURNS float
  AS 'MODULE_PATHNAME', 'NAD_stbox_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION nearestApproachDistance(tgeogpoint, stbox)
  RETURNS float
  AS 'MODULE_PATHNAME', 'NAD_tgeo_stbox'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION nearestApproachDistance(tgeogpoint, tgeogpoint)
  RETURNS float
  AS 'MODULE_PATHNAME', 'NAD_tgeo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR |=| (
  LEFTARG = geometry, RIGHTARG = tgeompoint,
  PROCEDURE = nearestApproachDistance,
  COMMUTATOR = '|=|'
);
CREATE OPERATOR |=| (
  LEFTARG = tgeompoint, RIGHTARG = geometry,
  PROCEDURE = nearestApproachDistance,
  COMMUTATOR = '|=|'
);
CREATE OPERATOR |=| (
  LEFTARG = stbox, RIGHTARG = geometry,
  PROCEDURE = nearestApproachDistance,
  COMMUTATOR = '|=|'
);
CREATE OPERATOR |=| (
  LEFTARG = geometry, RIGHTARG = stbox,
  PROCEDURE = nearestApproachDistance,
  COMMUTATOR = '|=|'
);
CREATE OPERATOR |=| (
  LEFTARG = stbox, RIGHTARG = tgeompoint,
  PROCEDURE = nearestApproachDistance,
  COMMUTATOR = '|=|'
);
CREATE OPERATOR |=| (
  LEFTARG = tgeompoint, RIGHTARG = stbox,
  PROCEDURE = nearestApproachDistance,
  COMMUTATOR = '|=|'
);
CREATE OPERATOR |=| (
  LEFTARG = stbox, RIGHTARG = stbox,
  PROCEDURE = nearestApproachDistance,
  COMMUTATOR = '|=|'
);
CREATE OPERATOR |=| (
  LEFTARG = tgeompoint, RIGHTARG = tgeompoint,
  PROCEDURE = nearestApproachDistance,
  COMMUTATOR = '|=|'
);

CREATE OPERATOR |=| (
  LEFTARG = geography, RIGHTARG = tgeogpoint,
  PROCEDURE = nearestApproachDistance,
  COMMUTATOR = '|=|'
);
CREATE OPERATOR |=| (
  LEFTARG = tgeogpoint, RIGHTARG = geography,
  PROCEDURE = nearestApproachDistance,
  COMMUTATOR = '|=|'
);
CREATE OPERATOR |=| (
  LEFTARG = stbox, RIGHTARG = geography,
  PROCEDURE = nearestApproachDistance,
  COMMUTATOR = '|=|'
);
CREATE OPERATOR |=| (
  LEFTARG = geography, RIGHTARG = stbox,
  PROCEDURE = nearestApproachDistance,
  COMMUTATOR = '|=|'
);
CREATE OPERATOR |=| (
  LEFTARG = stbox, RIGHTARG = tgeogpoint,
  PROCEDURE = nearestApproachDistance,
  COMMUTATOR = '|=|'
);
CREATE OPERATOR |=| (
  LEFTARG = tgeogpoint, RIGHTARG = stbox,
  PROCEDURE = nearestApproachDistance,
  COMMUTATOR = '|=|'
);
CREATE OPERATOR |=| (
  LEFTARG = tgeogpoint, RIGHTARG = tgeogpoint,
  PROCEDURE = nearestApproachDistance,
  COMMUTATOR = '|=|'
);

CREATE FUNCTION shortestLine(geometry, tgeompoint)
  RETURNS geometry
  AS 'MODULE_PATHNAME', 'Shortestline_geo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION shortestLine(tgeompoint, geometry)
  RETURNS geometry
  AS 'MODULE_PATHNAME', 'Shortestline_tgeo_geo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION shortestLine(tgeompoint, tgeompoint)
  RETURNS geometry
  AS 'MODULE_PATHNAME', 'Shortestline_tgeo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION shortestLine(geography, tgeogpoint)
  RETURNS geography
  AS 'MODULE_PATHNAME', 'Shortestline_geo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION shortestLine(tgeogpoint, geography)
  RETURNS geography
  AS 'MODULE_PATHNAME', 'Shortestline_tgeo_geo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION shortestLine(tgeogpoint, tgeogpoint)
  RETURNS geography
  AS 'MODULE_PATHNAME', 'Shortestline_tgeo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

/*****************************************************************************/
