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
 * Geometric functions for temporal geometries
 */

/*****************************************************************************
 * SRID
 *****************************************************************************/

CREATE FUNCTION SRID(tgeometry)
  RETURNS integer
  AS 'MODULE_PATHNAME', 'Tspatial_srid'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION setSRID(tgeometry, integer)
  RETURNS tgeometry
  AS 'MODULE_PATHNAME', 'Tspatial_set_srid'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION transform(tgeometry, integer)
  RETURNS tgeometry
  AS 'MODULE_PATHNAME', 'Tspatial_transform'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION transformPipeline(tgeometry, text, srid integer DEFAULT 0,
    is_forward boolean DEFAULT true)
  RETURNS tgeometry
  AS 'MODULE_PATHNAME', 'Tspatial_transform_pipeline'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

/*****************************************************************************
 * Traversed area
 *****************************************************************************/

CREATE FUNCTION traversedArea(tgeometry)
  RETURNS geoset
  AS 'MODULE_PATHNAME', 'Tgeo_traversed_area'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

/*****************************************************************************
 * AtGeometry and MinusGeometry
 *****************************************************************************/

-- CREATE FUNCTION atGeometry(tgeometry, geometry)
  -- RETURNS tgeometry
  -- AS 'MODULE_PATHNAME', 'Tgeo_at_geom'
  -- LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

-- CREATE FUNCTION minusGeometry(tgeometry, geometry)
  -- RETURNS tgeometry
  -- AS 'MODULE_PATHNAME', 'Tgeo_minus_geom'
  -- LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

-- CREATE FUNCTION atStbox(tgeometry, stbox, bool DEFAULT TRUE)
  -- RETURNS tgeometry
  -- AS 'MODULE_PATHNAME', 'Tgeo_at_stbox'
  -- LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

-- CREATE FUNCTION minusStbox(tgeometry, stbox, bool DEFAULT TRUE)
  -- RETURNS tgeometry
  -- AS 'MODULE_PATHNAME', 'Tgeo_minus_stbox'
  -- LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

/*****************************************************************************
 * Length
 *****************************************************************************/

-- CREATE FUNCTION length(tgeometry)
  -- RETURNS double precision
  -- AS 'MODULE_PATHNAME', 'Tgeo_length'
  -- LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

/*****************************************************************************
 * Cumulative length
 *****************************************************************************/

-- CREATE FUNCTION cumulativeLength(tgeometry)
  -- RETURNS tfloat
  -- AS 'MODULE_PATHNAME', 'Tgeo_cumulative_length'
  -- LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

/*****************************************************************************
 * Speed
 *****************************************************************************/

-- CREATE FUNCTION speed(tgeometry)
  -- RETURNS tfloat
  -- AS 'MODULE_PATHNAME', 'Tgeo_speed'
  -- LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

/*****************************************************************************
 * Time-weighted centroid
 *****************************************************************************/

-- CREATE FUNCTION twCentroid(tgeometry)
  -- RETURNS geometry
  -- AS 'MODULE_PATHNAME', 'Tgeo_twcentroid'
  -- LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

/*****************************************************************************
 * Temporal azimuth
 *****************************************************************************/

-- CREATE FUNCTION azimuth(tgeometry)
  -- RETURNS tfloat
  -- AS 'MODULE_PATHNAME', 'Tgeo_azimuth'
  -- LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

/*****************************************************************************/

