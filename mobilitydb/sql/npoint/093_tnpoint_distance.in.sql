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
 * @brief Temporal distance for temporal network points
 */

CREATE FUNCTION tDistance(geometry(Point), tnpoint)
  RETURNS tfloat
  AS 'MODULE_PATHNAME', 'Distance_point_tnpoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tDistance(npoint, tnpoint)
  RETURNS tfloat
  AS 'MODULE_PATHNAME', 'Distance_npoint_tnpoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tDistance(tnpoint, geometry(Point))
  RETURNS tfloat
  AS 'MODULE_PATHNAME', 'Distance_tnpoint_point'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tDistance(tnpoint, npoint)
  RETURNS tfloat
  AS 'MODULE_PATHNAME', 'Distance_tnpoint_npoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tDistance(tnpoint, tnpoint)
  RETURNS tfloat
  AS 'MODULE_PATHNAME', 'Distance_tnpoint_tnpoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR <-> (
  PROCEDURE = tDistance,
  LEFTARG = geometry,
  RIGHTARG = tnpoint,
  COMMUTATOR = <->
);
CREATE OPERATOR <-> (
  PROCEDURE = tDistance,
  LEFTARG = npoint,
  RIGHTARG = tnpoint,
  COMMUTATOR = <->
);
CREATE OPERATOR <-> (
  PROCEDURE = tDistance,
  LEFTARG = tnpoint,
  RIGHTARG = geometry,
  COMMUTATOR = <->
);
CREATE OPERATOR <-> (
  PROCEDURE = tDistance,
  LEFTARG = tnpoint,
  RIGHTARG = npoint,
  COMMUTATOR = <->
);
CREATE OPERATOR <-> (
  PROCEDURE = tDistance,
  LEFTARG = tnpoint,
  RIGHTARG = tnpoint,
  COMMUTATOR = <->
);

/*****************************************************************************/
