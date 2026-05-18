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
 * temporal geometries/geographies
 */

/*****************************************************************************
 * Index Support Function
 *****************************************************************************/

CREATE FUNCTION tspatial_supportfn(internal)
  RETURNS internal
  AS 'MODULE_PATHNAME', 'Tspatial_supportfn'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

/*****************************************************************************
 * Ever/Always Comparison Functions
 *****************************************************************************/

CREATE FUNCTION everEq(geometry, tgeometry)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_eq_geo_tgeo'
  SUPPORT tspatial_supportfn
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everEq(geography, tgeography)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_eq_geo_tgeo'
  SUPPORT tspatial_supportfn
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ?= (
  LEFTARG = geometry, RIGHTARG = tgeometry,
  PROCEDURE = everEq,
  NEGATOR = %<>,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);
CREATE OPERATOR ?= (
  LEFTARG = geography, RIGHTARG = tgeography,
  PROCEDURE = everEq,
  NEGATOR = %<>,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);

CREATE FUNCTION alwaysEq(geometry, tgeometry)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_eq_geo_tgeo'
  SUPPORT tspatial_supportfn
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysEq(geography, tgeography)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_eq_geo_tgeo'
  SUPPORT tspatial_supportfn
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR %= (
  LEFTARG = geometry, RIGHTARG = tgeometry,
  PROCEDURE = alwaysEq,
  NEGATOR = ?<>,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);
CREATE OPERATOR %= (
  LEFTARG = geography, RIGHTARG = tgeography,
  PROCEDURE = alwaysEq,
  NEGATOR = ?<>,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);

CREATE FUNCTION everNe(geometry, tgeometry)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_ne_geo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everNe(geography, tgeography)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_ne_geo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ?<> (
  LEFTARG = geometry, RIGHTARG = tgeometry,
  PROCEDURE = everNe,
  NEGATOR = %=,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);
CREATE OPERATOR ?<> (
  LEFTARG = geography, RIGHTARG = tgeography,
  PROCEDURE = everNe,
  NEGATOR = %=,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);

CREATE FUNCTION alwaysNe(geometry, tgeometry)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_ne_geo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysNe(geography, tgeography)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_ne_geo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR %<> (
  LEFTARG = geometry, RIGHTARG = tgeometry,
  PROCEDURE = alwaysNe,
  NEGATOR = ?=,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);
CREATE OPERATOR %<> (
  LEFTARG = geography, RIGHTARG = tgeography,
  PROCEDURE = alwaysNe,
  NEGATOR = ?=,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);

/*****************************************************************************/

CREATE FUNCTION everEq(tgeometry, geometry)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_eq_tgeo_geo'
  SUPPORT tspatial_supportfn
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everEq(tgeography, geography)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_eq_tgeo_geo'
  SUPPORT tspatial_supportfn
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ?= (
  LEFTARG = tgeometry, RIGHTARG = geometry,
  PROCEDURE = everEq,
  NEGATOR = %<>,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);
CREATE OPERATOR ?= (
  LEFTARG = tgeography, RIGHTARG = geography,
  PROCEDURE = everEq,
  NEGATOR = %<>,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);

CREATE FUNCTION alwaysEq(tgeometry, geometry)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_eq_tgeo_geo'
  SUPPORT tspatial_supportfn
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysEq(tgeography, geography)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_eq_tgeo_geo'
  SUPPORT tspatial_supportfn
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR %= (
  LEFTARG = tgeometry, RIGHTARG = geometry,
  PROCEDURE = alwaysEq,
  NEGATOR = ?<>,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);
CREATE OPERATOR %= (
  LEFTARG = tgeography, RIGHTARG = geography,
  PROCEDURE = alwaysEq,
  NEGATOR = ?<>,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);

CREATE FUNCTION everNe(tgeometry, geometry)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_ne_tgeo_geo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everNe(tgeography, geography)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_ne_tgeo_geo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ?<> (
  LEFTARG = tgeometry, RIGHTARG = geometry,
  PROCEDURE = everNe,
  NEGATOR = %=,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);
CREATE OPERATOR ?<> (
  LEFTARG = tgeography, RIGHTARG = geography,
  PROCEDURE = everNe,
  NEGATOR = %=,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);

CREATE FUNCTION alwaysNe(tgeometry, geometry)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_ne_tgeo_geo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysNe(tgeography, geography)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_ne_tgeo_geo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR %<> (
  LEFTARG = tgeometry, RIGHTARG = geometry,
  PROCEDURE = alwaysNe,
  NEGATOR = ?=,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);
CREATE OPERATOR %<> (
  LEFTARG = tgeography, RIGHTARG = geography,
  PROCEDURE = alwaysNe,
  NEGATOR = ?=,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);

/*****************************************************************************/

CREATE FUNCTION everEq(tgeometry, tgeometry)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_eq_tgeo_tgeo'
  SUPPORT tspatial_supportfn
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everEq(tgeography, tgeography)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_eq_tgeo_tgeo'
  SUPPORT tspatial_supportfn
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ?= (
  LEFTARG = tgeometry, RIGHTARG = tgeometry,
  PROCEDURE = everEq,
  NEGATOR = %<>,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);
CREATE OPERATOR ?= (
  LEFTARG = tgeography, RIGHTARG = tgeography,
  PROCEDURE = everEq,
  NEGATOR = %<>,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);

CREATE FUNCTION alwaysEq(tgeometry, tgeometry)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_eq_tgeo_tgeo'
  SUPPORT tspatial_supportfn
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysEq(tgeography, tgeography)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_eq_tgeo_tgeo'
  SUPPORT tspatial_supportfn
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR %= (
  LEFTARG = tgeography, RIGHTARG = tgeography,
  PROCEDURE = alwaysEq,
  NEGATOR = ?<>,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);
CREATE OPERATOR %= (
  LEFTARG = tgeometry, RIGHTARG = tgeometry,
  PROCEDURE = alwaysEq,
  NEGATOR = ?<>,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);

CREATE FUNCTION everNe(tgeometry, tgeometry)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_ne_tgeo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everNe(tgeography, tgeography)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_ne_tgeo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ?<> (
  LEFTARG = tgeometry, RIGHTARG = tgeometry,
  PROCEDURE = everNe,
  NEGATOR = %=,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);
CREATE OPERATOR ?<> (
  LEFTARG = tgeography, RIGHTARG = tgeography,
  PROCEDURE = everNe,
  NEGATOR = %=,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);

CREATE FUNCTION alwaysNe(tgeometry, tgeometry)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_ne_tgeo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysNe(tgeography, tgeography)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_ne_tgeo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR %<> (
  LEFTARG = tgeometry, RIGHTARG = tgeometry,
  PROCEDURE = alwaysNe,
  NEGATOR = ?=,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);
CREATE OPERATOR %<> (
  LEFTARG = tgeography, RIGHTARG = tgeography,
  PROCEDURE = alwaysNe,
  NEGATOR = ?=,
  RESTRICT = tspatial_sel, JOIN = tspatial_joinsel
);

/*****************************************************************************
 * Temporal equal
 *****************************************************************************/

CREATE FUNCTION tempEq(geometry, tgeometry)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Teq_geo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempEq(tgeometry, geometry)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Teq_tgeo_geo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempEq(tgeometry, tgeometry)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Teq_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR #= (
  PROCEDURE = tempEq,
  LEFTARG = geometry, RIGHTARG = tgeometry,
  COMMUTATOR = #=
);
CREATE OPERATOR #= (
  PROCEDURE = tempEq,
  LEFTARG = tgeometry, RIGHTARG = geometry,
  COMMUTATOR = #=
);
CREATE OPERATOR #= (
  PROCEDURE = tempEq,
  LEFTARG = tgeometry, RIGHTARG = tgeometry,
  COMMUTATOR = #=
);

/*****************************************************************************/

CREATE FUNCTION tempEq(geography, tgeography)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Teq_geo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempEq(tgeography, geography)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Teq_tgeo_geo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempEq(tgeography, tgeography)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Teq_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR #= (
  PROCEDURE = tempEq,
  LEFTARG = geography, RIGHTARG = tgeography,
  COMMUTATOR = #=
);
CREATE OPERATOR #= (
  PROCEDURE = tempEq,
  LEFTARG = tgeography, RIGHTARG = geography,
  COMMUTATOR = #=
);
CREATE OPERATOR #= (
  PROCEDURE = tempEq,
  LEFTARG = tgeography, RIGHTARG = tgeography,
  COMMUTATOR = #=
);

/*****************************************************************************
 * Temporal not equal
 *****************************************************************************/

CREATE FUNCTION tempNe(geometry, tgeometry)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tne_geo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempNe(tgeometry, geometry)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tne_tgeo_geo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempNe(tgeometry, tgeometry)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tne_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR #<> (
  PROCEDURE = tempNe,
  LEFTARG = geometry, RIGHTARG = tgeometry,
  COMMUTATOR = #<>
);
CREATE OPERATOR #<> (
  PROCEDURE = tempNe,
  LEFTARG = tgeometry, RIGHTARG = geometry,
  COMMUTATOR = #<>
);
CREATE OPERATOR #<> (
  PROCEDURE = tempNe,
  LEFTARG = tgeometry, RIGHTARG = tgeometry,
  COMMUTATOR = #<>
);

/*****************************************************************************/

CREATE FUNCTION tempNe(geography, tgeography)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tne_geo_tgeo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempNe(tgeography, geography)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tne_tgeo_geo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempNe(tgeography, tgeography)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tne_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR #<> (
  PROCEDURE = tempNe,
  LEFTARG = geography, RIGHTARG = tgeography,
  COMMUTATOR = #<>
);
CREATE OPERATOR #<> (
  PROCEDURE = tempNe,
  LEFTARG = tgeography, RIGHTARG = geography,
  COMMUTATOR = #<>
);
CREATE OPERATOR #<> (
  PROCEDURE = tempNe,
  LEFTARG = tgeography, RIGHTARG = tgeography,
  COMMUTATOR = #<>
);

/******************************************************************************/
