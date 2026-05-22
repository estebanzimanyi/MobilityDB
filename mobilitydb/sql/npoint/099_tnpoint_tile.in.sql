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
 * @brief Tile functions for temporal network points
 *
 * All functions delegate to the tgeometry equivalents by casting
 * tnpoint → tgeompoint → tgeometry for the spatial computation.
 * Split functions reconstruct the tnpoint fragment by restricting the
 * original tnpoint to the time extent of each returned tgeometry tile,
 * preserving the radius channel at each surviving instant.
 */

/******************************************************************************
 * Box functions
 ******************************************************************************/

CREATE FUNCTION spaceBoxes(tnpoint, xsize float, ysize float, zsize float,
    sorigin geometry DEFAULT 'Point(0 0 0)', bitmatrix boolean DEFAULT TRUE,
    borderInc boolean DEFAULT TRUE)
  RETURNS stbox[]
  LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE AS $$
    SELECT @extschema@.spaceBoxes(
      $1::@extschema@.tgeompoint, $2, $3, $4, $5, $6, $7)
  $$;
CREATE FUNCTION spaceBoxes(tnpoint, xsize float,
    sorigin geometry DEFAULT 'Point(0 0 0)', bitmatrix boolean DEFAULT TRUE,
    borderInc boolean DEFAULT TRUE)
  RETURNS stbox[]
  LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE AS $$
    SELECT @extschema@.spaceBoxes($1, $2, $2, $2, $3, $4, $5)
  $$;
CREATE FUNCTION spaceBoxes(tnpoint, xsize float, ysize float,
    sorigin geometry DEFAULT 'Point(0 0 0)', bitmatrix boolean DEFAULT TRUE,
    borderInc boolean DEFAULT TRUE)
  RETURNS stbox[]
  LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE AS $$
    SELECT @extschema@.spaceBoxes($1, $2, $3, $2, $4, $5, $6)
  $$;

CREATE FUNCTION timeBoxes(tnpoint, interval,
    torigin timestamptz DEFAULT '2000-01-03', bitmatrix boolean DEFAULT TRUE,
    borderInc boolean DEFAULT TRUE)
  RETURNS stbox[]
  LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE AS $$
    SELECT @extschema@.timeBoxes(
      $1::@extschema@.tgeompoint, $2, $3, $4, $5)
  $$;

CREATE FUNCTION spaceTimeBoxes(tnpoint, xsize float, ysize float,
    zsize float, interval, sorigin geometry DEFAULT 'Point(0 0 0)',
    torigin timestamptz DEFAULT '2000-01-03', bitmatrix boolean DEFAULT TRUE,
    borderInc boolean DEFAULT TRUE)
  RETURNS stbox[]
  LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE AS $$
    SELECT @extschema@.spaceTimeBoxes(
      $1::@extschema@.tgeompoint, $2, $3, $4, $5, $6, $7, $8, $9)
  $$;
CREATE FUNCTION spaceTimeBoxes(tnpoint, xsize float, interval,
    sorigin geometry DEFAULT 'Point(0 0 0)',
    torigin timestamptz DEFAULT '2000-01-03', bitmatrix boolean DEFAULT TRUE,
    borderInc boolean DEFAULT TRUE)
  RETURNS stbox[]
  LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE AS $$
    SELECT @extschema@.spaceTimeBoxes($1, $2, $2, $2, $3, $4, $5, $6, $7)
  $$;
CREATE FUNCTION spaceTimeBoxes(tnpoint, xsize float, ysize float, interval,
    sorigin geometry DEFAULT 'Point(0 0 0)',
    torigin timestamptz DEFAULT '2000-01-03', bitmatrix boolean DEFAULT TRUE,
    borderInc boolean DEFAULT TRUE)
  RETURNS stbox[]
  LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE AS $$
    SELECT @extschema@.spaceTimeBoxes($1, $2, $3, $2, $4, $5, $6, $7, $8)
  $$;

/******************************************************************************
 * Split functions
 ******************************************************************************/

CREATE TYPE point_tnpoint AS (
  point geometry,
  tnpoint tnpoint
);

CREATE FUNCTION spaceSplit(tnpoint, xsize float, ysize float, zsize float,
    sorigin geometry DEFAULT 'Point(0 0 0)', bitmatrix boolean DEFAULT TRUE,
    borderInc boolean DEFAULT TRUE)
  RETURNS SETOF point_tnpoint
  LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE AS $$
    SELECT r.point, @extschema@.atTime($1, @extschema@.getTime(r.tgeo))
    FROM @extschema@.spaceSplit(
      $1::@extschema@.tgeompoint, $2, $3, $4, $5, $6, $7) AS r
  $$;
CREATE FUNCTION spaceSplit(tnpoint, size float,
    sorigin geometry DEFAULT 'Point(0 0 0)', bitmatrix boolean DEFAULT TRUE,
    borderInc boolean DEFAULT TRUE)
  RETURNS SETOF point_tnpoint
  LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE AS $$
    SELECT @extschema@.spaceSplit($1, $2, $2, $2, $3, $4, $5)
  $$;
CREATE FUNCTION spaceSplit(tnpoint, sizeX float, sizeY float,
    sorigin geometry DEFAULT 'Point(0 0 0)', bitmatrix boolean DEFAULT TRUE,
    borderInc boolean DEFAULT TRUE)
  RETURNS SETOF point_tnpoint
  LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE AS $$
    SELECT @extschema@.spaceSplit($1, $2, $3, $2, $4, $5, $6)
  $$;

CREATE TYPE point_time_tnpoint AS (
  point geometry,
  time timestamptz,
  tnpoint tnpoint
);

CREATE FUNCTION spaceTimeSplit(tnpoint, xsize float, ysize float,
    zsize float, interval, sorigin geometry DEFAULT 'Point(0 0 0)',
    torigin timestamptz DEFAULT '2000-01-03', bitmatrix boolean DEFAULT TRUE,
    borderInc boolean DEFAULT TRUE)
  RETURNS SETOF point_time_tnpoint
  LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE AS $$
    SELECT r.point, r.time, @extschema@.atTime($1, @extschema@.getTime(r.tgeo))
    FROM @extschema@.spaceTimeSplit(
      $1::@extschema@.tgeompoint, $2, $3, $4, $5, $6, $7, $8, $9) AS r
  $$;
CREATE FUNCTION spaceTimeSplit(tnpoint, size float, interval,
    sorigin geometry DEFAULT 'Point(0 0 0)',
    torigin timestamptz DEFAULT '2000-01-03', bitmatrix boolean DEFAULT TRUE,
    borderInc boolean DEFAULT TRUE)
  RETURNS SETOF point_time_tnpoint
  LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE AS $$
    SELECT @extschema@.spaceTimeSplit($1, $2, $2, $2, $3, $4, $5, $6, $7)
  $$;
CREATE FUNCTION spaceTimeSplit(tnpoint, xsize float, ysize float, interval,
    sorigin geometry DEFAULT 'Point(0 0 0)',
    torigin timestamptz DEFAULT '2000-01-03', bitmatrix boolean DEFAULT TRUE,
    borderInc boolean DEFAULT TRUE)
  RETURNS SETOF point_time_tnpoint
  LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE AS $$
    SELECT @extschema@.spaceTimeSplit($1, $2, $3, $2, $4, $5, $6, $7, $8)
  $$;

/*****************************************************************************/
CREATE FUNCTION spaceTiles(tnpoint, xsize float, ysize float, zsize float,
    sorigin geometry DEFAULT 'Point(0 0 0)', borderInc boolean DEFAULT TRUE)
  RETURNS SETOF index_stbox
  AS 'SELECT @extschema@.spaceTiles(@extschema@.stbox($1), $2, $3, $4, $5, $6)'
  LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION spaceTiles(tnpoint, xsize float,
    sorigin geometry DEFAULT 'Point(0 0 0)', borderInc boolean DEFAULT TRUE)
  RETURNS SETOF index_stbox
  AS 'SELECT @extschema@.spaceTiles(@extschema@.stbox($1), $2, $2, $2, $3, $4)'
  LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION spaceTiles(tnpoint, xsize float, ysize float,
    sorigin geometry DEFAULT 'Point(0 0 0)', borderInc boolean DEFAULT TRUE)
  RETURNS SETOF index_stbox
  AS 'SELECT @extschema@.spaceTiles(@extschema@.stbox($1), $2, $3, $2, $4, $5)'
  LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION timeTiles(tnpoint, duration interval,
  torigin timestamptz DEFAULT '2000-01-03', borderInc boolean DEFAULT TRUE)
  RETURNS SETOF index_stbox
  AS 'SELECT @extschema@.timeTiles(@extschema@.stbox($1), $2, $3, $4)'
  LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION spaceTimeTiles(tnpoint, xsize float, ysize float,
  zsize float, duration interval, sorigin geometry DEFAULT 'Point(0 0 0)',
  torigin timestamptz DEFAULT '2000-01-03', borderInc boolean DEFAULT TRUE)
  RETURNS SETOF index_stbox
  AS 'SELECT @extschema@.spaceTimeTiles(@extschema@.stbox($1), $2, $3, $4, $5, $6, $7, $8)'
  LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION spaceTimeTiles(tnpoint, xsize float,
  duration interval, sorigin geometry DEFAULT 'Point(0 0 0)',
  torigin timestamptz DEFAULT '2000-01-03', borderInc boolean DEFAULT TRUE)
  RETURNS SETOF index_stbox
  AS 'SELECT @extschema@.spaceTimeTiles(@extschema@.stbox($1), $2, $2, $2, $3, $4, $5, $6)'
  LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION spaceTimeTiles(tnpoint, xsize float, ysize float,
  duration interval, sorigin geometry DEFAULT 'Point(0 0 0)',
  torigin timestamptz DEFAULT '2000-01-03', borderInc boolean DEFAULT TRUE)
  RETURNS SETOF index_stbox
  AS 'SELECT @extschema@.spaceTimeTiles(@extschema@.stbox($1), $2, $3, $2, $4, $5, $6, $7)'
  LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE;

/*****************************************************************************/
