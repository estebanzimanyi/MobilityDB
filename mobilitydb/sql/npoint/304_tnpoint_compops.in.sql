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
 * @brief Ever/always and temporal comparisons for temporal network points
 */

/*****************************************************************************
 * Ever/Always comparisons
 *****************************************************************************/

CREATE FUNCTION everEq(npoint, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_eq_npoint_tnpoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everEq(tnpoint, npoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_eq_tnpoint_npoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everEq(tnpoint, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_eq_tnpoint_tnpoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ?= (
  LEFTARG = npoint, RIGHTARG = tnpoint,
  PROCEDURE = everEq,
  NEGATOR = %<>,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);
CREATE OPERATOR ?= (
  LEFTARG = tnpoint, RIGHTARG = npoint,
  PROCEDURE = everEq,
  NEGATOR = %<>,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);
CREATE OPERATOR ?= (
  LEFTARG = tnpoint, RIGHTARG = tnpoint,
  PROCEDURE = everEq,
  NEGATOR = %<>,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);

CREATE FUNCTION everNe(npoint, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_ne_npoint_tnpoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everNe(tnpoint, npoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_ne_tnpoint_npoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everNe(tnpoint, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_ne_tnpoint_tnpoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ?<> (
  LEFTARG = npoint, RIGHTARG = tnpoint,
  PROCEDURE = everNe,
  NEGATOR = %=,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);
CREATE OPERATOR ?<> (
  LEFTARG = tnpoint, RIGHTARG = npoint,
  PROCEDURE = everNe,
  NEGATOR = %=,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);
CREATE OPERATOR ?<> (
  LEFTARG = tnpoint, RIGHTARG = tnpoint,
  PROCEDURE = everNe,
  NEGATOR = %=,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);

/*****************************************************************************/

CREATE FUNCTION alwaysEq(npoint, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_eq_npoint_tnpoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysEq(tnpoint, npoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_eq_tnpoint_npoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysEq(tnpoint, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_eq_tnpoint_tnpoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR %= (
  LEFTARG = npoint, RIGHTARG = tnpoint,
  PROCEDURE = alwaysEq,
  NEGATOR = ?<>,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);
CREATE OPERATOR %= (
  LEFTARG = tnpoint, RIGHTARG = npoint,
  PROCEDURE = alwaysEq,
  NEGATOR = ?<>,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);
CREATE OPERATOR %= (
  LEFTARG = tnpoint, RIGHTARG = tnpoint,
  PROCEDURE = alwaysEq,
  NEGATOR = ?<>,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);

CREATE FUNCTION alwaysNe(npoint, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_ne_npoint_tnpoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysNe(tnpoint, npoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_ne_tnpoint_npoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysNe(tnpoint, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_ne_tnpoint_tnpoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR %<> (
  LEFTARG = npoint, RIGHTARG = tnpoint,
  PROCEDURE = alwaysNe,
  NEGATOR = ?=,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);
CREATE OPERATOR %<> (
  LEFTARG = tnpoint, RIGHTARG = npoint,
  PROCEDURE = alwaysNe,
  NEGATOR = ?=,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);
CREATE OPERATOR %<> (
  LEFTARG = tnpoint, RIGHTARG = tnpoint,
  PROCEDURE = alwaysNe,
  NEGATOR = ?=,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);

/*****************************************************************************
 * Temporal comparisons
 *****************************************************************************/

CREATE FUNCTION tempEq(npoint, tnpoint)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Teq_npoint_tnpoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempEq(tnpoint, npoint)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Teq_tnpoint_npoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempEq(tnpoint, tnpoint)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Teq_tnpoint_tnpoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR #= (
  PROCEDURE = tempEq,
  LEFTARG = npoint, RIGHTARG = tnpoint,
  COMMUTATOR = #=
);
CREATE OPERATOR #= (
  PROCEDURE = tempEq,
  LEFTARG = tnpoint, RIGHTARG = npoint,
  COMMUTATOR = #=
);
CREATE OPERATOR #= (
  PROCEDURE = tempEq,
  LEFTARG = tnpoint, RIGHTARG = tnpoint,
  COMMUTATOR = #=
);

/*****************************************************************************/

CREATE FUNCTION tempNe(npoint, tnpoint)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tne_npoint_tnpoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempNe(tnpoint, npoint)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tne_tnpoint_npoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempNe(tnpoint, tnpoint)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tne_tnpoint_tnpoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR #<> (
  PROCEDURE = tempNe,
  LEFTARG = npoint, RIGHTARG = tnpoint,
  COMMUTATOR = #<>
);
CREATE OPERATOR #<> (
  PROCEDURE = tempNe,
  LEFTARG = tnpoint, RIGHTARG = npoint,
  COMMUTATOR = #<>
);
CREATE OPERATOR #<> (
  PROCEDURE = tempNe,
  LEFTARG = tnpoint, RIGHTARG = tnpoint,
  COMMUTATOR = #<>
);

/******************************************************************************/
