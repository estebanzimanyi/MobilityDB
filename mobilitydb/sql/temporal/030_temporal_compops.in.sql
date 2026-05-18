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
 * @brief Comparison functions and operators for temporal types
 * @note In this file we need both definitions of the functions with 2 and 3
 * parameters to be able to define the operators. This is not the case for
 * the temporal relationships while a single definition of the functions with
 * 3 parameters is enough
 */

/*****************************************************************************
 * Index Support Functions
 *****************************************************************************/

CREATE FUNCTION tnumber_supportfn(internal)
  RETURNS internal
  AS 'MODULE_PATHNAME', 'Tnumber_supportfn'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

/*****************************************************************************
 * Ever/Always Comparison Functions
 *****************************************************************************/

CREATE FUNCTION everEq(boolean, tbool)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_eq_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everEq(integer, tint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_eq_base_temporal'
  SUPPORT tnumber_supportfn
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everEq(float, tfloat)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_eq_base_temporal'
  SUPPORT tnumber_supportfn
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everEq(text, ttext)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_eq_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ?= (
  LEFTARG = boolean, RIGHTARG = tbool,
  PROCEDURE = everEq,
  NEGATOR = %<>,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);
CREATE OPERATOR ?= (
  LEFTARG = integer, RIGHTARG = tint,
  PROCEDURE = everEq,
  NEGATOR = %<>,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR ?= (
  LEFTARG = float, RIGHTARG = tfloat,
  PROCEDURE = everEq,
  NEGATOR = %<>,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR ?= (
  LEFTARG = text, RIGHTARG = ttext,
  PROCEDURE = everEq,
  NEGATOR = %<>,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);

CREATE FUNCTION everEq(tbool, boolean)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_eq_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everEq(tint, integer)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_eq_temporal_base'
  SUPPORT tnumber_supportfn
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everEq(tfloat, float)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_eq_temporal_base'
  SUPPORT tnumber_supportfn
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everEq(ttext, text)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_eq_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ?= (
  LEFTARG = tbool, RIGHTARG = boolean,
  PROCEDURE = everEq,
  NEGATOR = %<>,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);
CREATE OPERATOR ?= (
  LEFTARG = tint, RIGHTARG = integer,
  PROCEDURE = everEq,
  NEGATOR = %<>,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR ?= (
  LEFTARG = tfloat, RIGHTARG = float,
  PROCEDURE = everEq,
  NEGATOR = %<>,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR ?= (
  LEFTARG = ttext, RIGHTARG = text,
  PROCEDURE = everEq,
  NEGATOR = %<>,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);

CREATE FUNCTION everNe(boolean, tbool)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_ne_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everNe(integer, tint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_ne_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everNe(float, tfloat)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_ne_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everNe(text, ttext)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_ne_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ?<> (
  LEFTARG = boolean, RIGHTARG = tbool,
  PROCEDURE = everNe,
  NEGATOR = %=,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);
CREATE OPERATOR ?<> (
  LEFTARG = integer, RIGHTARG = tint,
  PROCEDURE = everNe,
  NEGATOR = %=,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR ?<> (
  LEFTARG = float, RIGHTARG = tfloat,
  PROCEDURE = everNe,
  NEGATOR = %=,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR ?<> (
  LEFTARG = text, RIGHTARG = ttext,
  PROCEDURE = everNe,
  NEGATOR = %=,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);

CREATE FUNCTION everNe(tbool, boolean)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_ne_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everNe(tint, integer)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_ne_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everNe(tfloat, float)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_ne_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everNe(ttext, text)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_ne_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ?<> (
  LEFTARG = tbool, RIGHTARG = boolean,
  PROCEDURE = everNe,
  NEGATOR = %=,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);
CREATE OPERATOR ?<> (
  LEFTARG = tint, RIGHTARG = integer,
  PROCEDURE = everNe,
  NEGATOR = %=,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR ?<> (
  LEFTARG = tfloat, RIGHTARG = float,
  PROCEDURE = everNe,
  NEGATOR = %=,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR ?<> (
  LEFTARG = ttext, RIGHTARG = text,
  PROCEDURE = everNe,
  NEGATOR = %=,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);

/*****************************************************************************/

CREATE FUNCTION alwaysEq(boolean, tbool)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_eq_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysEq(integer, tint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_eq_base_temporal'
  SUPPORT tnumber_supportfn
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysEq(float, tfloat)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_eq_base_temporal'
  SUPPORT tnumber_supportfn
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysEq(text, ttext)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_eq_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR %= (
  LEFTARG = boolean, RIGHTARG = tbool,
  PROCEDURE = alwaysEq,
  NEGATOR = ?<>,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);
CREATE OPERATOR %= (
  LEFTARG = integer, RIGHTARG = tint,
  PROCEDURE = alwaysEq,
  NEGATOR = ?<>,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR %= (
  LEFTARG = float, RIGHTARG = tfloat,
  PROCEDURE = alwaysEq,
  NEGATOR = ?<>,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR %= (
  LEFTARG = text, RIGHTARG = ttext,
  PROCEDURE = alwaysEq,
  NEGATOR = ?<>,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);

CREATE FUNCTION alwaysEq(tbool, boolean)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_eq_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysEq(tint, integer)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_eq_temporal_base'
  SUPPORT tnumber_supportfn
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysEq(tfloat, float)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_eq_temporal_base'
  SUPPORT tnumber_supportfn
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysEq(ttext, text)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_eq_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR %= (
  LEFTARG = tbool, RIGHTARG = boolean,
  PROCEDURE = alwaysEq,
  NEGATOR = ?<>,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);
CREATE OPERATOR %= (
  LEFTARG = tint, RIGHTARG = integer,
  PROCEDURE = alwaysEq,
  NEGATOR = ?<>,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR %= (
  LEFTARG = tfloat, RIGHTARG = float,
  PROCEDURE = alwaysEq,
  NEGATOR = ?<>,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR %= (
  LEFTARG = ttext, RIGHTARG = text,
  PROCEDURE = alwaysEq,
  NEGATOR = ?<>,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);

CREATE FUNCTION alwaysNe(boolean, tbool)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_ne_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysNe(integer, tint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_ne_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysNe(float, tfloat)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_ne_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysNe(text, ttext)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_ne_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR %<> (
  LEFTARG = boolean, RIGHTARG = tbool,
  PROCEDURE = alwaysNe,
  NEGATOR = ?=,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);
CREATE OPERATOR %<> (
  LEFTARG = integer, RIGHTARG = tint,
  PROCEDURE = alwaysNe,
  NEGATOR = ?=,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR %<> (
  LEFTARG = float, RIGHTARG = tfloat,
  PROCEDURE = alwaysNe,
  NEGATOR = ?=,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR %<> (
  LEFTARG = text, RIGHTARG = ttext,
  PROCEDURE = alwaysNe,
  NEGATOR = ?=,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);

CREATE FUNCTION alwaysNe(tbool, boolean)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_ne_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysNe(tint, integer)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_ne_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysNe(tfloat, float)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_ne_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysNe(ttext, text)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_ne_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR %<> (
  LEFTARG = tbool, RIGHTARG = boolean,
  PROCEDURE = alwaysNe,
  NEGATOR = ?=,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);
CREATE OPERATOR %<> (
  LEFTARG = tint, RIGHTARG = integer,
  PROCEDURE = alwaysNe,
  NEGATOR = ?=,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR %<> (
  LEFTARG = tfloat, RIGHTARG = float,
  PROCEDURE = alwaysNe,
  NEGATOR = ?=,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR %<> (
  LEFTARG = ttext, RIGHTARG = text,
  PROCEDURE = alwaysNe,
  NEGATOR = ?=,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);

/*****************************************************************************/

CREATE FUNCTION everLt(integer, tint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_lt_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everLt(float, tfloat)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_lt_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everLt(text, ttext)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_lt_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ?< (
  LEFTARG = integer, RIGHTARG = tint,
  PROCEDURE = everLt,
  NEGATOR = %>=,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR ?< (
  LEFTARG = float, RIGHTARG = tfloat,
  PROCEDURE = everLt,
  NEGATOR = %>=,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR ?< (
  LEFTARG = text, RIGHTARG = ttext,
  PROCEDURE = everLt,
  NEGATOR = %>=,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);

CREATE FUNCTION everLt(tint, integer)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_lt_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everLt(tfloat, float)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_lt_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everLt(ttext, text)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_lt_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ?< (
  LEFTARG = tint, RIGHTARG = integer,
  PROCEDURE = everLt,
  NEGATOR = %>=,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR ?< (
  LEFTARG = tfloat, RIGHTARG = float,
  PROCEDURE = everLt,
  NEGATOR = %>=,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR ?< (
  LEFTARG = ttext, RIGHTARG = text,
  PROCEDURE = everLt,
  NEGATOR = %>=,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);

CREATE FUNCTION everLe(integer, tint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_le_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everLe(float, tfloat)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_le_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everLe(text, ttext)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_le_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ?<= (
  LEFTARG = integer, RIGHTARG = tint,
  PROCEDURE = everLe,
  NEGATOR = %>,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR ?<= (
  LEFTARG = float, RIGHTARG = tfloat,
  PROCEDURE = everLe,
  NEGATOR = %>,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR ?<= (
  LEFTARG = text, RIGHTARG = ttext,
  PROCEDURE = everLe,
  NEGATOR = %>,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);

CREATE FUNCTION everLe(tint, integer)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_le_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everLe(tfloat, float)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_le_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everLe(ttext, text)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_le_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ?<= (
  LEFTARG = tint, RIGHTARG = integer,
  PROCEDURE = everLe,
  NEGATOR = %>,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR ?<= (
  LEFTARG = tfloat, RIGHTARG = float,
  PROCEDURE = everLe,
  NEGATOR = %>,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR ?<= (
  LEFTARG = ttext, RIGHTARG = text,
  PROCEDURE = everLe,
  NEGATOR = %>,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);

CREATE FUNCTION everGt(integer, tint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_gt_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everGt(float, tfloat)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_gt_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everGt(text, ttext)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_gt_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ?> (
  LEFTARG = integer, RIGHTARG = tint,
  PROCEDURE = everGt,
  NEGATOR = %<=,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR ?> (
  LEFTARG = float, RIGHTARG = tfloat,
  PROCEDURE = everGt,
  NEGATOR = %<=,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR ?> (
  LEFTARG = text, RIGHTARG = ttext,
  PROCEDURE = everGt,
  NEGATOR = %<=,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);

CREATE FUNCTION everGt(tint, integer)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_gt_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everGt(tfloat, float)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_gt_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everGt(ttext, text)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_gt_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ?> (
  LEFTARG = tint, RIGHTARG = integer,
  PROCEDURE = everGt,
  NEGATOR = %<=,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR ?> (
  LEFTARG = tfloat, RIGHTARG = float,
  PROCEDURE = everGt,
  NEGATOR = %<=,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR ?> (
  LEFTARG = ttext, RIGHTARG = text,
  PROCEDURE = everGt,
  NEGATOR = %<=,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);

CREATE FUNCTION everGe(integer, tint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_ge_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everGe(float, tfloat)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_ge_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everGe(text, ttext)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_ge_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ?>= (
  LEFTARG = integer, RIGHTARG = tint,
  PROCEDURE = everGe,
  NEGATOR = %<,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR ?>= (
  LEFTARG = float, RIGHTARG = tfloat,
  PROCEDURE = everGe,
  NEGATOR = %<,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR ?>= (
  LEFTARG = text, RIGHTARG = ttext,
  PROCEDURE = everGe,
  NEGATOR = %<,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);

CREATE FUNCTION everGe(tint, integer)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_ge_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everGe(tfloat, float)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_ge_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everGe(ttext, text)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_ge_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ?>= (
  LEFTARG = tint, RIGHTARG = integer,
  PROCEDURE = everGe,
  NEGATOR = %<,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR ?>= (
  LEFTARG = tfloat, RIGHTARG = float,
  PROCEDURE = everGe,
  NEGATOR = %<,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR ?>= (
  LEFTARG = ttext, RIGHTARG = text,
  PROCEDURE = everGe,
  NEGATOR = %<,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);

/*****************************************************************************/

CREATE FUNCTION alwaysLt(integer, tint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_lt_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysLt(float, tfloat)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_lt_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysLt(text, ttext)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_lt_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR %< (
  LEFTARG = integer, RIGHTARG = tint,
  PROCEDURE = alwaysLt,
  NEGATOR = ?>=,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR %< (
  LEFTARG = float, RIGHTARG = tfloat,
  PROCEDURE = alwaysLt,
  NEGATOR = ?>=,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR %< (
  LEFTARG = text, RIGHTARG = ttext,
  PROCEDURE = alwaysLt,
  NEGATOR = ?>=,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);

CREATE FUNCTION alwaysLt(tint, integer)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_lt_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysLt(tfloat, float)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_lt_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysLt(ttext, text)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_lt_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR %< (
  LEFTARG = tint, RIGHTARG = integer,
  PROCEDURE = alwaysLt,
  NEGATOR = ?>=,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR %< (
  LEFTARG = tfloat, RIGHTARG = float,
  PROCEDURE = alwaysLt,
  NEGATOR = ?>=,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR %< (
  LEFTARG = ttext, RIGHTARG = text,
  PROCEDURE = alwaysLt,
  NEGATOR = ?>=,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);

CREATE FUNCTION alwaysLe(integer, tint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_le_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysLe(float, tfloat)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_le_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysLe(text, ttext)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_le_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR %<= (
  LEFTARG = integer, RIGHTARG = tint,
  PROCEDURE = alwaysLe,
  NEGATOR = ?>,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR %<= (
  LEFTARG = float, RIGHTARG = tfloat,
  PROCEDURE = alwaysLe,
  NEGATOR = ?>,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR %<= (
  LEFTARG = text, RIGHTARG = ttext,
  PROCEDURE = alwaysLe,
  NEGATOR = ?>,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);

CREATE FUNCTION alwaysLe(tint, integer)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_le_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysLe(tfloat, float)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_le_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysLe(ttext, text)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_le_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR %<= (
  LEFTARG = tint, RIGHTARG = integer,
  PROCEDURE = alwaysLe,
  NEGATOR = ?>,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR %<= (
  LEFTARG = tfloat, RIGHTARG = float,
  PROCEDURE = alwaysLe,
  NEGATOR = ?>,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR %<= (
  LEFTARG = ttext, RIGHTARG = text,
  PROCEDURE = alwaysLe,
  NEGATOR = ?>,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);

CREATE FUNCTION alwaysGt(integer, tint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_gt_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysGt(float, tfloat)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_gt_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysGt(text, ttext)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_gt_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR %> (
  LEFTARG = integer, RIGHTARG = tint,
  PROCEDURE = alwaysGt,
  NEGATOR = ?<=,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR %> (
  LEFTARG = float, RIGHTARG = tfloat,
  PROCEDURE = alwaysGt,
  NEGATOR = ?<=,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR %> (
  LEFTARG = text, RIGHTARG = ttext,
  PROCEDURE = alwaysGt,
  NEGATOR = ?<=,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);

CREATE FUNCTION alwaysGt(tint, integer)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_gt_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysGt(tfloat, float)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_gt_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysGt(ttext, text)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_gt_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR %> (
  LEFTARG = tint, RIGHTARG = integer,
  PROCEDURE = alwaysGt,
  NEGATOR = ?<=,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR %> (
  LEFTARG = tfloat, RIGHTARG = float,
  PROCEDURE = alwaysGt,
  NEGATOR = ?<=,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR %> (
  LEFTARG = ttext, RIGHTARG = text,
  PROCEDURE = alwaysGt,
  NEGATOR = ?<=,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);

CREATE FUNCTION alwaysGe(integer, tint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_ge_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysGe(float, tfloat)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_ge_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysGe(text, ttext)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_ge_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR %>= (
  LEFTARG = integer, RIGHTARG = tint,
  PROCEDURE = alwaysGe,
  NEGATOR = ?<,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR %>= (
  LEFTARG = float, RIGHTARG = tfloat,
  PROCEDURE = alwaysGe,
  NEGATOR = ?<,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR %>= (
  LEFTARG = text, RIGHTARG = ttext,
  PROCEDURE = alwaysGe,
  NEGATOR = ?<,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);

CREATE FUNCTION alwaysGe(tint, integer)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_ge_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysGe(tfloat, float)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_ge_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysGe(ttext, text)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_ge_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR %>= (
  LEFTARG = tint, RIGHTARG = integer,
  PROCEDURE = alwaysGe,
  NEGATOR = ?<,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR %>= (
  LEFTARG = tfloat, RIGHTARG = float,
  PROCEDURE = alwaysGe,
  NEGATOR = ?<,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR %>= (
  LEFTARG = ttext, RIGHTARG = text,
  PROCEDURE = alwaysGe,
  NEGATOR = ?<,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);

/*****************************************************************************/

CREATE FUNCTION everEq(tbool, tbool)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_eq_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everEq(tint, tint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_eq_temporal_temporal'
  SUPPORT tnumber_supportfn
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everEq(tfloat, tfloat)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_eq_temporal_temporal'
  SUPPORT tnumber_supportfn
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everEq(ttext, ttext)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_eq_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ?= (
  LEFTARG = tbool, RIGHTARG = tbool,
  PROCEDURE = everEq,
  NEGATOR = %<>,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);
CREATE OPERATOR ?= (
  LEFTARG = tint, RIGHTARG = tint,
  PROCEDURE = everEq,
  NEGATOR = %<>,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR ?= (
  LEFTARG = tfloat, RIGHTARG = tfloat,
  PROCEDURE = everEq,
  NEGATOR = %<>,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR ?= (
  LEFTARG = ttext, RIGHTARG = ttext,
  PROCEDURE = everEq,
  NEGATOR = %<>,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);

CREATE FUNCTION alwaysEq(tbool, tbool)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_eq_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysEq(tint, tint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_eq_temporal_temporal'
  SUPPORT tnumber_supportfn
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysEq(tfloat, tfloat)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_eq_temporal_temporal'
  SUPPORT tnumber_supportfn
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysEq(ttext, ttext)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_eq_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR %= (
  LEFTARG = tbool, RIGHTARG = tbool,
  PROCEDURE = alwaysEq,
  NEGATOR = ?<>,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);
CREATE OPERATOR %= (
  LEFTARG = tint, RIGHTARG = tint,
  PROCEDURE = alwaysEq,
  NEGATOR = ?<>,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR %= (
  LEFTARG = tfloat, RIGHTARG = tfloat,
  PROCEDURE = alwaysEq,
  NEGATOR = ?<>,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR %= (
  LEFTARG = ttext, RIGHTARG = ttext,
  PROCEDURE = alwaysEq,
  NEGATOR = ?<>,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);

CREATE FUNCTION everNe(tbool, tbool)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_ne_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everNe(tint, tint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_ne_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everNe(tfloat, tfloat)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_ne_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everNe(ttext, ttext)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_ne_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ?<> (
  LEFTARG = tbool, RIGHTARG = tbool,
  PROCEDURE = everNe,
  NEGATOR = %=,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);
CREATE OPERATOR ?<> (
  LEFTARG = tint, RIGHTARG = tint,
  PROCEDURE = everNe,
  NEGATOR = %=,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR ?<> (
  LEFTARG = tfloat, RIGHTARG = tfloat,
  PROCEDURE = everNe,
  NEGATOR = %=,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR ?<> (
  LEFTARG = ttext, RIGHTARG = ttext,
  PROCEDURE = everNe,
  NEGATOR = %=,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);

CREATE FUNCTION alwaysNe(tbool, tbool)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_ne_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysNe(tint, tint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_ne_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysNe(tfloat, tfloat)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_ne_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysNe(ttext, ttext)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_ne_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR %<> (
  LEFTARG = tbool, RIGHTARG = tbool,
  PROCEDURE = alwaysNe,
  NEGATOR = ?=,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);
CREATE OPERATOR %<> (
  LEFTARG = tint, RIGHTARG = tint,
  PROCEDURE = alwaysNe,
  NEGATOR = ?=,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR %<> (
  LEFTARG = tfloat, RIGHTARG = tfloat,
  PROCEDURE = alwaysNe,
  NEGATOR = ?=,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR %<> (
  LEFTARG = ttext, RIGHTARG = ttext,
  PROCEDURE = alwaysNe,
  NEGATOR = ?=,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);

/*****************************************************************************/

CREATE FUNCTION everLt(tint, tint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_lt_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everLt(tfloat, tfloat)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_lt_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everLt(ttext, ttext)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_lt_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ?< (
  LEFTARG = tint, RIGHTARG = tint,
  PROCEDURE = everLt,
  NEGATOR = %>=,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR ?< (
  LEFTARG = tfloat, RIGHTARG = tfloat,
  PROCEDURE = everLt,
  NEGATOR = %>=,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR ?< (
  LEFTARG = ttext, RIGHTARG = ttext,
  PROCEDURE = everLt,
  NEGATOR = %>=,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);

CREATE FUNCTION everLe(tint, tint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_le_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everLe(tfloat, tfloat)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_le_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everLe(ttext, ttext)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_le_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ?<= (
  LEFTARG = tint, RIGHTARG = tint,
  PROCEDURE = everLe,
  NEGATOR = %>,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR ?<= (
  LEFTARG = tfloat, RIGHTARG = tfloat,
  PROCEDURE = everLe,
  NEGATOR = %>,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR ?<= (
  LEFTARG = ttext, RIGHTARG = ttext,
  PROCEDURE = everLe,
  NEGATOR = %>,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);

CREATE FUNCTION alwaysLt(tint, tint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_lt_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysLt(tfloat, tfloat)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_lt_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysLt(ttext, ttext)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_lt_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR %< (
  LEFTARG = tint, RIGHTARG = tint,
  PROCEDURE = alwaysLt,
  NEGATOR = ?>=,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR %< (
  LEFTARG = tfloat, RIGHTARG = tfloat,
  PROCEDURE = alwaysLt,
  NEGATOR = ?>=,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR %< (
  LEFTARG = ttext, RIGHTARG = ttext,
  PROCEDURE = alwaysLt,
  NEGATOR = ?>=,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);

CREATE FUNCTION alwaysLe(tint, tint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_le_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysLe(tfloat, tfloat)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_le_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysLe(ttext, ttext)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_le_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR %<= (
  LEFTARG = tint, RIGHTARG = tint,
  PROCEDURE = alwaysLe,
  NEGATOR = ?>,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR %<= (
  LEFTARG = tfloat, RIGHTARG = tfloat,
  PROCEDURE = alwaysLe,
  NEGATOR = ?>,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR %<= (
  LEFTARG = ttext, RIGHTARG = ttext,
  PROCEDURE = alwaysLe,
  NEGATOR = ?>,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);

CREATE FUNCTION everGt(tint, tint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_gt_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everGt(tfloat, tfloat)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_gt_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everGt(ttext, ttext)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_gt_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ?> (
  LEFTARG = tint, RIGHTARG = tint,
  PROCEDURE = everGt,
  NEGATOR = %<=,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR ?> (
  LEFTARG = tfloat, RIGHTARG = tfloat,
  PROCEDURE = everGt,
  NEGATOR = %<=,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR ?> (
  LEFTARG = ttext, RIGHTARG = ttext,
  PROCEDURE = everGt,
  NEGATOR = %<=,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);

CREATE FUNCTION everGe(tint, tint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_ge_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everGe(tfloat, tfloat)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_ge_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION everGe(ttext, ttext)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Ever_ge_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ?>= (
  LEFTARG = tint, RIGHTARG = tint,
  PROCEDURE = everGe,
  NEGATOR = %<,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR ?>= (
  LEFTARG = tfloat, RIGHTARG = tfloat,
  PROCEDURE = everGe,
  NEGATOR = %<,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR ?>= (
  LEFTARG = ttext, RIGHTARG = ttext,
  PROCEDURE = everGe,
  NEGATOR = %<,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);

CREATE FUNCTION alwaysGt(tint, tint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_gt_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysGt(tfloat, tfloat)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_gt_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysGt(ttext, ttext)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_gt_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR %> (
  LEFTARG = tint, RIGHTARG = tint,
  PROCEDURE = alwaysGt,
  NEGATOR = ?<=,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR %> (
  LEFTARG = tfloat, RIGHTARG = tfloat,
  PROCEDURE = alwaysGt,
  NEGATOR = ?<=,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR %> (
  LEFTARG = ttext, RIGHTARG = ttext,
  PROCEDURE = alwaysGt,
  NEGATOR = ?<=,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);

CREATE FUNCTION alwaysGe(tint, tint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_ge_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysGe(tfloat, tfloat)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_ge_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION alwaysGe(ttext, ttext)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Always_ge_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR %>= (
  LEFTARG = tint, RIGHTARG = tint,
  PROCEDURE = alwaysGe,
  NEGATOR = ?<,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR %>= (
  LEFTARG = tfloat, RIGHTARG = tfloat,
  PROCEDURE = alwaysGe,
  NEGATOR = ?<,
  RESTRICT = tnumber_sel, JOIN = tnumber_joinsel
);
CREATE OPERATOR %>= (
  LEFTARG = ttext, RIGHTARG = ttext,
  PROCEDURE = alwaysGe,
  NEGATOR = ?<,
  RESTRICT = scalarltsel, JOIN = scalarltjoinsel
);

/*****************************************************************************
 * Temporal eq
 *****************************************************************************/

-- Temporal boolean

CREATE FUNCTION tempEq(boolean, tbool)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Teq_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempEq(tbool, boolean)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Teq_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempEq(tbool, tbool)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Teq_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR #= (
  PROCEDURE = tempEq,
  LEFTARG = boolean, RIGHTARG = tbool,
  COMMUTATOR = #=
);
CREATE OPERATOR #= (
  PROCEDURE = tempEq,
  LEFTARG = tbool, RIGHTARG = boolean,
  COMMUTATOR = #=
);
CREATE OPERATOR #= (
  PROCEDURE = tempEq,
  LEFTARG = tbool, RIGHTARG = tbool,
  COMMUTATOR = #=
);

/*****************************************************************************/

-- Temporal integer

CREATE FUNCTION tempEq(integer, tint)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Teq_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempEq(tint, integer)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Teq_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempEq(tint, tint)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Teq_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR #= (
  PROCEDURE = tempEq,
  LEFTARG = integer, RIGHTARG = tint,
  COMMUTATOR = #=
);
CREATE OPERATOR #= (
  PROCEDURE = tempEq,
  LEFTARG = tint, RIGHTARG = integer,
  COMMUTATOR = #=
);
CREATE OPERATOR #= (
  PROCEDURE = tempEq,
  LEFTARG = tint, RIGHTARG = tint,
  COMMUTATOR = #=
);

/*****************************************************************************/

-- float #= <Type>

CREATE FUNCTION tempEq(float, tfloat)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Teq_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempEq(tfloat, float)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Teq_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempEq(tfloat, tfloat)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Teq_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR #= (
  PROCEDURE = tempEq,
  LEFTARG = float, RIGHTARG = tfloat,
  COMMUTATOR = #=
);
CREATE OPERATOR #= (
  PROCEDURE = tempEq,
  LEFTARG = tfloat, RIGHTARG = float,
  COMMUTATOR = #=
);
CREATE OPERATOR #= (
  PROCEDURE = tempEq,
  LEFTARG = tfloat, RIGHTARG = tfloat,
  COMMUTATOR = #=
);

/*****************************************************************************/

-- Temporal text

CREATE FUNCTION tempEq(text, ttext)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Teq_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempEq(ttext, text)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Teq_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempEq(ttext, ttext)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Teq_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR #= (
  PROCEDURE = tempEq,
  LEFTARG = text, RIGHTARG = ttext,
  COMMUTATOR = #=
);
CREATE OPERATOR #= (
  PROCEDURE = tempEq,
  LEFTARG = ttext, RIGHTARG = text,
  COMMUTATOR = #=
);
CREATE OPERATOR #= (
  PROCEDURE = tempEq,
  LEFTARG = ttext, RIGHTARG = ttext,
  COMMUTATOR = #=
);

/*****************************************************************************
 * Temporal ne
 *****************************************************************************/

-- Temporal boolean

CREATE FUNCTION tempNe(boolean, tbool)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tne_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempNe(tbool, boolean)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tne_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempNe(tbool, tbool)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tne_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR #<> (
  PROCEDURE = tempNe,
  LEFTARG = boolean, RIGHTARG = tbool,
  COMMUTATOR = #<>
);
CREATE OPERATOR #<> (
  PROCEDURE = tempNe,
  LEFTARG = tbool, RIGHTARG = boolean,
  COMMUTATOR = #<>
);
CREATE OPERATOR #<> (
  PROCEDURE = tempNe,
  LEFTARG = tbool, RIGHTARG = tbool,
  COMMUTATOR = #<>
);

/*****************************************************************************/

-- Temporal integer

CREATE FUNCTION tempNe(integer, tint)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tne_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempNe(tint, integer)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tne_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempNe(tint, tint)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tne_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR #<> (
  PROCEDURE = tempNe,
  LEFTARG = integer, RIGHTARG = tint,
  COMMUTATOR = #<>
);
CREATE OPERATOR #<> (
  PROCEDURE = tempNe,
  LEFTARG = tint, RIGHTARG = integer,
  COMMUTATOR = #<>
);
CREATE OPERATOR #<> (
  PROCEDURE = tempNe,
  LEFTARG = tint, RIGHTARG = tint,
  COMMUTATOR = #<>
);

/*****************************************************************************/

-- Temporal float

CREATE FUNCTION tempNe(float, tfloat)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tne_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempNe(tfloat, float)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tne_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempNe(tfloat, tfloat)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tne_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR #<> (
  PROCEDURE = tempNe,
  LEFTARG = float, RIGHTARG = tfloat,
  COMMUTATOR = #<>
);
CREATE OPERATOR #<> (
  PROCEDURE = tempNe,
  LEFTARG = tfloat, RIGHTARG = float,
  COMMUTATOR = #<>
);
CREATE OPERATOR #<> (
  PROCEDURE = tempNe,
  LEFTARG = tfloat, RIGHTARG = tfloat,
  COMMUTATOR = #<>
);

/*****************************************************************************/

-- Temporal text

CREATE FUNCTION tempNe(text, ttext)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tne_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempNe(ttext, text)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tne_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempNe(ttext, ttext)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tne_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR #<> (
  PROCEDURE = tempNe,
  LEFTARG = text, RIGHTARG = ttext,
  COMMUTATOR = #<>
);
CREATE OPERATOR #<> (
  PROCEDURE = tempNe,
  LEFTARG = ttext, RIGHTARG = text,
  COMMUTATOR = #<>
);
CREATE OPERATOR #<> (
  PROCEDURE = tempNe,
  LEFTARG = ttext, RIGHTARG = ttext,
  COMMUTATOR = #<>
);

/*****************************************************************************
 * Temporal lt
 *****************************************************************************/

-- Temporal integer

CREATE FUNCTION tempLt(integer, tint)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tlt_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempLt(tint, integer)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tlt_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempLt(tint, tint)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tlt_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR #< (
  PROCEDURE = tempLt,
  LEFTARG = integer, RIGHTARG = tint,
  COMMUTATOR = #>
);
CREATE OPERATOR #< (
  PROCEDURE = tempLt,
  LEFTARG = tint, RIGHTARG = integer,
  COMMUTATOR = #>
);
CREATE OPERATOR #< (
  PROCEDURE = tempLt,
  LEFTARG = tint, RIGHTARG = tint,
  COMMUTATOR = #>
);

/*****************************************************************************/

-- Temporal float

CREATE FUNCTION tempLt(float, tfloat)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tlt_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempLt(tfloat, float)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tlt_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempLt(tfloat, tfloat)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tlt_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR #< (
  PROCEDURE = tempLt,
  LEFTARG = float, RIGHTARG = tfloat,
  COMMUTATOR = #>
);
CREATE OPERATOR #< (
  PROCEDURE = tempLt,
  LEFTARG = tfloat, RIGHTARG = float,
  COMMUTATOR = #>
);
CREATE OPERATOR #< (
  PROCEDURE = tempLt,
  LEFTARG = tfloat, RIGHTARG = tfloat,
  COMMUTATOR = #>
);

/*****************************************************************************/

-- Temporal text

CREATE FUNCTION tempLt(text, ttext)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tlt_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempLt(ttext, text)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tlt_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempLt(ttext, ttext)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tlt_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR #< (
  PROCEDURE = tempLt,
  LEFTARG = text, RIGHTARG = ttext,
  COMMUTATOR = #>
);
CREATE OPERATOR #< (
  PROCEDURE = tempLt,
  LEFTARG = ttext, RIGHTARG = text,
  COMMUTATOR = #>
);
CREATE OPERATOR #< (
  PROCEDURE = tempLt,
  LEFTARG = ttext, RIGHTARG = ttext,
  COMMUTATOR = #>
);

/*****************************************************************************
 * Temporal gt
 *****************************************************************************/

-- Temporal integer

CREATE FUNCTION tempGt(integer, tint)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tgt_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempGt(tint, integer)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tgt_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempGt(tint, tint)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tgt_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR #> (
  PROCEDURE = tempGt,
  LEFTARG = integer, RIGHTARG = tint,
  COMMUTATOR = #<
);
CREATE OPERATOR #> (
  PROCEDURE = tempGt,
  LEFTARG = tint, RIGHTARG = integer,
  COMMUTATOR = #<
);
CREATE OPERATOR #> (
  PROCEDURE = tempGt,
  LEFTARG = tint, RIGHTARG = tint,
  COMMUTATOR = #<
);

/*****************************************************************************/

-- Temporal float

CREATE FUNCTION tempGt(float, tfloat)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tgt_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempGt(tfloat, integer)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tgt_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempGt(tfloat, float)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tgt_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempGt(tfloat, tfloat)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tgt_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR #> (
  PROCEDURE = tempGt,
  LEFTARG = float, RIGHTARG = tfloat,
  COMMUTATOR = #<
);
CREATE OPERATOR #> (
  PROCEDURE = tempGt,
  LEFTARG = tfloat, RIGHTARG = float,
  COMMUTATOR = #<
);
CREATE OPERATOR #> (
  PROCEDURE = tempGt,
  LEFTARG = tfloat, RIGHTARG = tfloat,
  COMMUTATOR = #<
);

/*****************************************************************************/

-- Temporal text

CREATE FUNCTION tempGt(text, ttext)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tgt_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempGt(ttext, text)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tgt_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempGt(ttext, ttext)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tgt_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR #> (
  PROCEDURE = tempGt,
  LEFTARG = text, RIGHTARG = ttext,
  COMMUTATOR = #<
);
CREATE OPERATOR #> (
  PROCEDURE = tempGt,
  LEFTARG = ttext, RIGHTARG = text,
  COMMUTATOR = #<
);
CREATE OPERATOR #> (
  PROCEDURE = tempGt,
  LEFTARG = ttext, RIGHTARG = ttext,
  COMMUTATOR = #<
);

/*****************************************************************************
 * Temporal le
 *****************************************************************************/

-- Temporal integer

CREATE FUNCTION tempLe(integer, tint)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tle_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempLe(tint, integer)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tle_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempLe(tint, tint)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tle_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR #<= (
  PROCEDURE = tempLe,
  LEFTARG = integer, RIGHTARG = tint,
  COMMUTATOR = #>=
);
CREATE OPERATOR #<= (
  PROCEDURE = tempLe,
  LEFTARG = tint, RIGHTARG = integer,
  COMMUTATOR = #>=
);
CREATE OPERATOR #<= (
  PROCEDURE = tempLe,
  LEFTARG = tint, RIGHTARG = tint,
  COMMUTATOR = #>=
);

/*****************************************************************************/

-- Temporal float

CREATE FUNCTION tempLe(float, tfloat)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tle_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempLe(tfloat, float)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tle_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempLe(tfloat, tfloat)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tle_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR #<= (
  PROCEDURE = tempLe,
  LEFTARG = float, RIGHTARG = tfloat,
  COMMUTATOR = #>=
);
CREATE OPERATOR #<= (
  PROCEDURE = tempLe,
  LEFTARG = tfloat, RIGHTARG = float,
  COMMUTATOR = #>=
);
CREATE OPERATOR #<= (
  PROCEDURE = tempLe,
  LEFTARG = tfloat, RIGHTARG = tfloat,
  COMMUTATOR = #>=
);

/*****************************************************************************/

-- Temporal text

CREATE FUNCTION tempLe(text, ttext)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tle_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempLe(ttext, text)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tle_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempLe(ttext, ttext)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tle_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR #<= (
  PROCEDURE = tempLe,
  LEFTARG = text, RIGHTARG = ttext,
  COMMUTATOR = #>=
);
CREATE OPERATOR #<= (
  PROCEDURE = tempLe,
  LEFTARG = ttext, RIGHTARG = text,
  COMMUTATOR = #>=
);
CREATE OPERATOR #<= (
  PROCEDURE = tempLe,
  LEFTARG = ttext, RIGHTARG = ttext,
  COMMUTATOR = #>=
);

/*****************************************************************************
 * Temporal ge
 *****************************************************************************/

-- Temporal integer

CREATE FUNCTION tempGe(integer, tint)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tge_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempGe(tint, integer)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tge_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempGe(tint, tint)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tge_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR #>= (
  PROCEDURE = tempGe,
  LEFTARG = integer, RIGHTARG = tint,
  COMMUTATOR = #<=
);
CREATE OPERATOR #>= (
  PROCEDURE = tempGe,
  LEFTARG = tint, RIGHTARG = integer,
  COMMUTATOR = #<=
);
CREATE OPERATOR #>= (
  PROCEDURE = tempGe,
  LEFTARG = tint, RIGHTARG = tint,
  COMMUTATOR = #<=
);

/*****************************************************************************/

-- Temporal float

CREATE FUNCTION tempGe(float, tfloat)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tge_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempGe(tfloat, integer)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tge_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempGe(tfloat, float)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tge_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempGe(tfloat, tfloat)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tge_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR #>= (
  PROCEDURE = tempGe,
  LEFTARG = float, RIGHTARG = tfloat,
  COMMUTATOR = #<=
);
CREATE OPERATOR #>= (
  PROCEDURE = tempGe,
  LEFTARG = tfloat, RIGHTARG = float,
  COMMUTATOR = #<=
);
CREATE OPERATOR #>= (
  PROCEDURE = tempGe,
  LEFTARG = tfloat, RIGHTARG = tfloat,
  COMMUTATOR = #<=
);

/*****************************************************************************/

-- Temporal text

CREATE FUNCTION tempGe(text, ttext)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tge_base_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempGe(ttext, text)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tge_temporal_base'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tempGe(ttext, ttext)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'Tge_temporal_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR #>= (
  PROCEDURE = tempGe,
  LEFTARG = text, RIGHTARG = ttext,
  COMMUTATOR = #<=
);
CREATE OPERATOR #>= (
  PROCEDURE = tempGe,
  LEFTARG = ttext, RIGHTARG = text,
  COMMUTATOR = #<=
);
CREATE OPERATOR #>= (
  PROCEDURE = tempGe,
  LEFTARG = ttext, RIGHTARG = ttext,
  COMMUTATOR = #<=
);

/*****************************************************************************/
