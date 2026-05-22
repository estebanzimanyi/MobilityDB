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
 * @brief Cross-type parity functions for temporal geographic points, backed
 * by the geodetic-aware temporal kernels shared with tgeompoint.
 */

/*****************************************************************************
 * Simplification and simplicity
 *****************************************************************************/

CREATE FUNCTION douglasPeuckerSimplify(tgeogpoint, float, boolean DEFAULT TRUE)
  RETURNS tgeogpoint
  AS 'MODULE_PATHNAME', 'Temporal_simplify_dp'
  LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE FUNCTION maxDistSimplify(tgeogpoint, float, boolean DEFAULT TRUE)
  RETURNS tgeogpoint
  AS 'MODULE_PATHNAME', 'Temporal_simplify_max_dist'
  LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE FUNCTION isSimple(tgeogpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Tpoint_is_simple'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION makeSimple(tgeogpoint)
  RETURNS tgeogpoint[]
  AS 'MODULE_PATHNAME', 'Tpoint_make_simple'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

/*****************************************************************************
 * Elevation restriction
 *****************************************************************************/

CREATE FUNCTION atElevation(tgeogpoint, floatspan)
  RETURNS tgeogpoint
  AS 'MODULE_PATHNAME', 'Tgeo_at_elevation'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION minusElevation(tgeogpoint, floatspan)
  RETURNS tgeogpoint
  AS 'MODULE_PATHNAME', 'Tgeo_minus_elevation'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

/*****************************************************************************
 * tDwithin
 *****************************************************************************/

CREATE FUNCTION tDwithin(tgeogpoint, geography, dist float)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tdwithin_tgeo_geo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION tDwithin(tgeogpoint, tgeogpoint, dist float)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tdwithin_tgeo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

/* NOTE: timeBoxes/spaceBoxes and the planar tiling family are intentionally
 * NOT exposed for tgeogpoint: the box-tiling kernels build a planar STBox and
 * raise "Operation on mixed planar and geodetic coordinates". Planar spatial
 * tiling, Gauss-Krüger projection (transform_gk), and AsMVTGeom are geodetic
 * exceptions (planar-only), reason-marked in the parity methodology. */

/*****************************************************************************/
