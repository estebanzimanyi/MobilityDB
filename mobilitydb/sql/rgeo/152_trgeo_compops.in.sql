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
 * @brief Ever, always, and temporal comparisons for temporal rigid geometries
 */

/*****************************************************************************
 * Ever/Always Comparison Functions
 *****************************************************************************/

CREATE FUNCTION everEq(geometry, trgeometry)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_eq_geo_trgeometry'
  SUPPORT tspatial_supportfn
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysEq(geometry, trgeometry)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_eq_geo_trgeometry'
  SUPPORT tspatial_supportfn
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ?= (
  LEFTARG = geometry, RIGHTARG = trgeometry,
  PROCEDURE = everEq,
  NEGATOR = %<>,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);
CREATE OPERATOR %= (
  LEFTARG = geometry, RIGHTARG = trgeometry,
  PROCEDURE = alwaysEq,
  NEGATOR = ?<>,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);

CREATE FUNCTION everNe(geometry, trgeometry)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_ne_geo_trgeometry'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysNe(geometry, trgeometry)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_ne_geo_trgeometry'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ?<> (
  LEFTARG = geometry, RIGHTARG = trgeometry,
  PROCEDURE = everNe,
  NEGATOR = %=,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);
CREATE OPERATOR %<> (
  LEFTARG = geometry, RIGHTARG = trgeometry,
  PROCEDURE = alwaysNe,
  NEGATOR = ?=,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);

/*****************************************************************************/

CREATE FUNCTION everEq(trgeometry, geometry)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_eq_trgeometry_geo'
  SUPPORT tspatial_supportfn
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysEq(trgeometry, geometry)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_eq_trgeometry_geo'
  SUPPORT tspatial_supportfn
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ?= (
  LEFTARG = trgeometry, RIGHTARG = geometry,
  PROCEDURE = everEq,
  NEGATOR = %<>,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);
CREATE OPERATOR %= (
  LEFTARG = trgeometry, RIGHTARG = geometry,
  PROCEDURE = alwaysEq,
  NEGATOR = ?<>,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);

CREATE FUNCTION everNe(trgeometry, geometry)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_ne_trgeometry_geo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysNe(trgeometry, geometry)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_ne_trgeometry_geo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ?<> (
  LEFTARG = trgeometry, RIGHTARG = geometry,
  PROCEDURE = everNe,
  NEGATOR = %=,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);
CREATE OPERATOR %<> (
  LEFTARG = trgeometry, RIGHTARG = geometry,
  PROCEDURE = alwaysNe,
  NEGATOR = ?=,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);

/*****************************************************************************/

CREATE FUNCTION everEq(trgeometry, trgeometry)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_eq_trgeometry_trgeometry'
  SUPPORT tspatial_supportfn
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysEq(trgeometry, trgeometry)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_eq_trgeometry_trgeometry'
  SUPPORT tspatial_supportfn
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ?= (
  LEFTARG = trgeometry, RIGHTARG = trgeometry,
  PROCEDURE = everEq,
  NEGATOR = %<>,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);
CREATE OPERATOR %= (
  LEFTARG = trgeometry, RIGHTARG = trgeometry,
  PROCEDURE = alwaysEq,
  NEGATOR = ?<>,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);

CREATE FUNCTION everNe(trgeometry, trgeometry)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_ne_trgeometry_trgeometry'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysNe(trgeometry, trgeometry)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_ne_trgeometry_trgeometry'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ?<> (
  LEFTARG = trgeometry, RIGHTARG = trgeometry,
  PROCEDURE = everNe,
  NEGATOR = %=,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);
CREATE OPERATOR %<> (
  LEFTARG = trgeometry, RIGHTARG = trgeometry,
  PROCEDURE = alwaysNe,
  NEGATOR = ?=,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);

/*****************************************************************************
 * Temporal equal
 *****************************************************************************/

CREATE FUNCTION tempEq(geometry, trgeometry)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Teq_geo_trgeometry'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempEq(trgeometry, geometry)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Teq_trgeometry_geo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempEq(trgeometry, trgeometry)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Teq_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR #= (
  PROCEDURE = tempEq,
  LEFTARG = geometry, RIGHTARG = trgeometry,
  COMMUTATOR = #=
);
CREATE OPERATOR #= (
  PROCEDURE = tempEq,
  LEFTARG = trgeometry, RIGHTARG = geometry,
  COMMUTATOR = #=
);
CREATE OPERATOR #= (
  PROCEDURE = tempEq,
  LEFTARG = trgeometry, RIGHTARG = trgeometry,
  COMMUTATOR = #=
);

/*****************************************************************************
 * Temporal not equal
 *****************************************************************************/

CREATE FUNCTION tempNe(geometry, trgeometry)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tne_geo_trgeometry'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempNe(trgeometry, geometry)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tne_trgeometry_geo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempNe(trgeometry, trgeometry)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tne_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR #<> (
  PROCEDURE = tempNe,
  LEFTARG = geometry, RIGHTARG = trgeometry,
  COMMUTATOR = #<>
);
CREATE OPERATOR #<> (
  PROCEDURE = tempNe,
  LEFTARG = trgeometry, RIGHTARG = geometry,
  COMMUTATOR = #<>
);
CREATE OPERATOR #<> (
  PROCEDURE = tempNe,
  LEFTARG = trgeometry, RIGHTARG = trgeometry,
  COMMUTATOR = #<>
);

/*****************************************************************************/
