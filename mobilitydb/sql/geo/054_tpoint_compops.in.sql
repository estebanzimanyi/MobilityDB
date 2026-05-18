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
 * @brief Ever/always and temporal comparison functions and operators for
 * temporal geometry/geography points
 */

/*****************************************************************************
 * Ever/Always Comparison Functions
 *****************************************************************************/

CREATE FUNCTION everEq(geometry, tgeompoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_eq_geo_tgeo'
  SUPPORT tspatial_supportfn
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everEq(geography, tgeogpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_eq_geo_tgeo'
  SUPPORT tspatial_supportfn
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ?= (
  LEFTARG = geometry, RIGHTARG = tgeompoint,
  PROCEDURE = everEq,
  NEGATOR = %<>,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);
CREATE OPERATOR ?= (
  LEFTARG = geography, RIGHTARG = tgeogpoint,
  PROCEDURE = everEq,
  NEGATOR = %<>,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);

CREATE FUNCTION alwaysEq(geometry, tgeompoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_eq_geo_tgeo'
  SUPPORT tspatial_supportfn
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysEq(geography, tgeogpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_eq_geo_tgeo'
  SUPPORT tspatial_supportfn
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR %= (
  LEFTARG = geography, RIGHTARG = tgeogpoint,
  PROCEDURE = alwaysEq,
  NEGATOR = ?<>,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);
CREATE OPERATOR %= (
  LEFTARG = geometry, RIGHTARG = tgeompoint,
  PROCEDURE = alwaysEq,
  NEGATOR = ?<>,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);

CREATE FUNCTION everNe(geometry, tgeompoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_ne_geo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everNe(geography, tgeogpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_ne_geo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ?<> (
  LEFTARG = geometry, RIGHTARG = tgeompoint,
  PROCEDURE = everNe,
  NEGATOR = %=,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);
CREATE OPERATOR ?<> (
  LEFTARG = geography, RIGHTARG = tgeogpoint,
  PROCEDURE = everNe,
  NEGATOR = %=,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);

CREATE FUNCTION alwaysNe(geometry, tgeompoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_ne_geo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysNe(geography, tgeogpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_ne_geo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR %<> (
  LEFTARG = geometry, RIGHTARG = tgeompoint,
  PROCEDURE = alwaysNe,
  NEGATOR = ?=,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);
CREATE OPERATOR %<> (
  LEFTARG = geography, RIGHTARG = tgeogpoint,
  PROCEDURE = alwaysNe,
  NEGATOR = ?=,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);

/*****************************************************************************/

CREATE FUNCTION everEq(tgeompoint, geometry)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_eq_tgeo_geo'
  SUPPORT tspatial_supportfn
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everEq(tgeogpoint, geography)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_eq_tgeo_geo'
  SUPPORT tspatial_supportfn
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ?= (
  LEFTARG = tgeompoint, RIGHTARG = geometry,
  PROCEDURE = everEq,
  NEGATOR = %<>,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);
CREATE OPERATOR ?= (
  LEFTARG = tgeogpoint, RIGHTARG = geography,
  PROCEDURE = everEq,
  NEGATOR = %<>,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);

CREATE FUNCTION alwaysEq(tgeompoint, geometry)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_eq_tgeo_geo'
  SUPPORT tspatial_supportfn
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysEq(tgeogpoint, geography)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_eq_tgeo_geo'
  SUPPORT tspatial_supportfn
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR %= (
  LEFTARG = tgeogpoint, RIGHTARG = geography,
  PROCEDURE = alwaysEq,
  NEGATOR = ?<>,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);
CREATE OPERATOR %= (
  LEFTARG = tgeompoint, RIGHTARG = geometry,
  PROCEDURE = alwaysEq,
  NEGATOR = ?<>,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);

CREATE FUNCTION everNe(tgeompoint, geometry)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_ne_tgeo_geo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everNe(tgeogpoint, geography)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_ne_tgeo_geo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ?<> (
  LEFTARG = tgeompoint, RIGHTARG = geometry,
  PROCEDURE = everNe,
  NEGATOR = %=,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);
CREATE OPERATOR ?<> (
  LEFTARG = tgeogpoint, RIGHTARG = geography,
  PROCEDURE = everNe,
  NEGATOR = %=,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);

CREATE FUNCTION alwaysNe(tgeompoint, geometry)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_ne_tgeo_geo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysNe(tgeogpoint, geography)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_ne_tgeo_geo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR %<> (
  LEFTARG = tgeompoint, RIGHTARG = geometry,
  PROCEDURE = alwaysNe,
  NEGATOR = ?=,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);
CREATE OPERATOR %<> (
  LEFTARG = tgeogpoint, RIGHTARG = geography,
  PROCEDURE = alwaysNe,
  NEGATOR = ?=,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);

/*****************************************************************************/

CREATE FUNCTION everEq(tgeompoint, tgeompoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_eq_tgeo_tgeo'
  SUPPORT tspatial_supportfn
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everEq(tgeogpoint, tgeogpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_eq_tgeo_tgeo'
  SUPPORT tspatial_supportfn
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ?= (
  LEFTARG = tgeompoint, RIGHTARG = tgeompoint,
  PROCEDURE = everEq,
  NEGATOR = %<>,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);
CREATE OPERATOR ?= (
  LEFTARG = tgeogpoint, RIGHTARG = tgeogpoint,
  PROCEDURE = everEq,
  NEGATOR = %<>,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);

CREATE FUNCTION alwaysEq(tgeompoint, tgeompoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_eq_tgeo_tgeo'
  SUPPORT tspatial_supportfn
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysEq(tgeogpoint, tgeogpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_eq_tgeo_tgeo'
  SUPPORT tspatial_supportfn
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR %= (
  LEFTARG = tgeogpoint, RIGHTARG = tgeogpoint,
  PROCEDURE = alwaysEq,
  NEGATOR = ?<>,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);
CREATE OPERATOR %= (
  LEFTARG = tgeompoint, RIGHTARG = tgeompoint,
  PROCEDURE = alwaysEq,
  NEGATOR = ?<>,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);

CREATE FUNCTION everNe(tgeompoint, tgeompoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_ne_tgeo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everNe(tgeogpoint, tgeogpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_ne_tgeo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ?<> (
  LEFTARG = tgeompoint, RIGHTARG = tgeompoint,
  PROCEDURE = everNe,
  NEGATOR = %=,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);
CREATE OPERATOR ?<> (
  LEFTARG = tgeogpoint, RIGHTARG = tgeogpoint,
  PROCEDURE = everNe,
  NEGATOR = %=,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);

CREATE FUNCTION alwaysNe(tgeompoint, tgeompoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_ne_tgeo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysNe(tgeogpoint, tgeogpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_ne_tgeo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR %<> (
  LEFTARG = tgeompoint, RIGHTARG = tgeompoint,
  PROCEDURE = alwaysNe,
  NEGATOR = ?=,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);
CREATE OPERATOR %<> (
  LEFTARG = tgeogpoint, RIGHTARG = tgeogpoint,
  PROCEDURE = alwaysNe,
  NEGATOR = ?=,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);

/*****************************************************************************
 * Temporal equal
 *****************************************************************************/

CREATE FUNCTION tempEq(geometry, tgeompoint)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Teq_geo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempEq(tgeompoint, geometry)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Teq_tgeo_geo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempEq(tgeompoint, tgeompoint)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Teq_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR #= (
  PROCEDURE = tempEq,
  LEFTARG = geometry, RIGHTARG = tgeompoint,
  COMMUTATOR = #=
);
CREATE OPERATOR #= (
  PROCEDURE = tempEq,
  LEFTARG = tgeompoint, RIGHTARG = geometry,
  COMMUTATOR = #=
);
CREATE OPERATOR #= (
  PROCEDURE = tempEq,
  LEFTARG = tgeompoint, RIGHTARG = tgeompoint,
  COMMUTATOR = #=
);

/*****************************************************************************/

CREATE FUNCTION tempEq(geography, tgeogpoint)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Teq_geo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempEq(tgeogpoint, geography)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Teq_tgeo_geo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempEq(tgeogpoint, tgeogpoint)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Teq_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR #= (
  PROCEDURE = tempEq,
  LEFTARG = geography, RIGHTARG = tgeogpoint,
  COMMUTATOR = #=
);
CREATE OPERATOR #= (
  PROCEDURE = tempEq,
  LEFTARG = tgeogpoint, RIGHTARG = geography,
  COMMUTATOR = #=
);
CREATE OPERATOR #= (
  PROCEDURE = tempEq,
  LEFTARG = tgeogpoint, RIGHTARG = tgeogpoint,
  COMMUTATOR = #=
);

/*****************************************************************************
 * Temporal not equal
 *****************************************************************************/

CREATE FUNCTION tempNe(geometry, tgeompoint)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tne_geo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempNe(tgeompoint, geometry)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tne_tgeo_geo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempNe(tgeompoint, tgeompoint)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tne_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR #<> (
  PROCEDURE = tempNe,
  LEFTARG = geometry, RIGHTARG = tgeompoint,
  COMMUTATOR = #<>
);
CREATE OPERATOR #<> (
  PROCEDURE = tempNe,
  LEFTARG = tgeompoint, RIGHTARG = geometry,
  COMMUTATOR = #<>
);
CREATE OPERATOR #<> (
  PROCEDURE = tempNe,
  LEFTARG = tgeompoint, RIGHTARG = tgeompoint,
  COMMUTATOR = #<>
);

/*****************************************************************************/

CREATE FUNCTION tempNe(geography, tgeogpoint)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tne_geo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempNe(tgeogpoint, geography)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tne_tgeo_geo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempNe(tgeogpoint, tgeogpoint)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tne_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR #<> (
  PROCEDURE = tempNe,
  LEFTARG = geography, RIGHTARG = tgeogpoint,
  COMMUTATOR = #<>
);
CREATE OPERATOR #<> (
  PROCEDURE = tempNe,
  LEFTARG = tgeogpoint, RIGHTARG = geography,
  COMMUTATOR = #<>
);
CREATE OPERATOR #<> (
  PROCEDURE = tempNe,
  LEFTARG = tgeogpoint, RIGHTARG = tgeogpoint,
  COMMUTATOR = #<>
);

/******************************************************************************/
