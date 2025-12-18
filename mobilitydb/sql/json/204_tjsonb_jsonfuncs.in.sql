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
 * @brief Temporal JSON functions derived from the PostgreSQL JSON functions
 */

/*****************************************************************************
 * JSONB Functions
 *****************************************************************************/

CREATE FUNCTION tjson_object_field(ttext, text)
  RETURNS ttext
  AS 'MODULE_PATHNAME', 'Tjson_object_field'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tjsonb_object_field(tjsonb, text)
  RETURNS tjsonb
  AS 'MODULE_PATHNAME', 'Tjsonb_object_field'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tjson_object_field_text(ttext, text)
  RETURNS ttext
  AS 'MODULE_PATHNAME', 'Tjson_object_field_text'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tjsonb_object_field_text(tjsonb, text)
  RETURNS ttext
  AS 'MODULE_PATHNAME', 'Tjsonb_object_field_text'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR -> (
  PROCEDURE = tjson_object_field,
  LEFTARG   = ttext, RIGHTARG = text
);
CREATE OPERATOR -> (
  PROCEDURE = tjsonb_object_field,
  LEFTARG   = tjsonb, RIGHTARG = text
);
CREATE OPERATOR ->> (
  PROCEDURE = tjson_object_field_text,
  LEFTARG   = ttext, RIGHTARG = text
);
CREATE OPERATOR ->> (
  PROCEDURE = tjsonb_object_field_text,
  LEFTARG   = tjsonb, RIGHTARG = text
);

CREATE FUNCTION tjson_extract_path(temp ttext, path text[])
RETURNS ttext
AS 'MODULE_PATHNAME', 'Tjson_extract_path'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tjsonb_extract_path(temp tjsonb, path text[])
RETURNS tjsonb
AS 'MODULE_PATHNAME', 'Tjsonb_extract_path'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tjson_extract_path_text(temp ttext, path text[])
RETURNS ttext
AS 'MODULE_PATHNAME', 'Tjson_extract_path_text'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tjsonb_extract_path_text(temp tjsonb, path text[])
RETURNS tjsonb
AS 'MODULE_PATHNAME', 'Tjsonb_extract_path_text'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR #> (
  PROCEDURE = tjson_extract_path,
  LEFTARG   = ttext, RIGHTARG = text[]
);
CREATE OPERATOR #> (
  PROCEDURE = tjsonb_extract_path,
  LEFTARG   = tjsonb, RIGHTARG = text[]
);
CREATE OPERATOR #>> (
  PROCEDURE = tjson_extract_path_text,
  LEFTARG   = ttext, RIGHTARG = text[]
);
CREATE OPERATOR #>> (
  PROCEDURE = tjsonb_extract_path_text,
  LEFTARG   = tjsonb, RIGHTARG = text[]
);

CREATE FUNCTION tjson_array_element(ttext, integer)
  RETURNS ttext
  AS 'MODULE_PATHNAME', 'Tjson_array_element'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tjsonb_array_element(tjsonb, integer)
  RETURNS tjsonb
  AS 'MODULE_PATHNAME', 'Tjsonb_array_element'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tjsonb_array_element_text(tjsonb, integer)
  RETURNS tjsonb
  AS 'MODULE_PATHNAME', 'Tjsonb_array_element_text'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION tbool(tjsonb, text) 
RETURNS tbool
AS 'MODULE_PATHNAME', 'Tjsonb_to_tbool'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION tint(tjsonb, text) 
RETURNS tint
AS 'MODULE_PATHNAME', 'Tjsonb_to_tint'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION tfloat(tjsonb, text, interp text DEFAULT 'linear') 
RETURNS tfloat
AS 'MODULE_PATHNAME', 'Tjsonb_to_tfloat'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION ttext(tjsonb, text) 
RETURNS ttext
AS 'MODULE_PATHNAME', 'Tjsonb_to_ttext_key'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

/*****************************************************************************/

CREATE FUNCTION tjsonb_concat(jsonb, tjsonb)
  RETURNS tjsonb
  AS 'MODULE_PATHNAME', 'Concat_jsonb_tjsonb'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tjsonb_concat(tjsonb, jsonb)
  RETURNS tjsonb
  AS 'MODULE_PATHNAME', 'Concat_tjsonb_jsonb'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tjsonb_concat(tjsonb, tjsonb)
  RETURNS tjsonb
  AS 'MODULE_PATHNAME', 'Concat_tjsonb_tjsonb'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR || (
  PROCEDURE = tjsonb_concat,
  LEFTARG   = jsonb, RIGHTARG = tjsonb
);
CREATE OPERATOR || (
  PROCEDURE = tjsonb_concat,
  LEFTARG   = tjsonb, RIGHTARG = jsonb
);
CREATE OPERATOR || (
  PROCEDURE = tjsonb_concat,
  LEFTARG   = tjsonb, RIGHTARG = tjsonb
);

CREATE FUNCTION tjsonb_delete(tjsonb, text)
  RETURNS tjsonb
  AS 'MODULE_PATHNAME', 'Tjsonb_delete_key'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tjsonb_delete_array(tjsonb, text[])
  RETURNS tjsonb
  AS 'MODULE_PATHNAME', 'Tjsonb_delete_key_array'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tjsonb_delete_idx(tjsonb, integer)
  RETURNS tjsonb
  AS 'MODULE_PATHNAME', 'Tjsonb_delete_idx'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR - (
  PROCEDURE = tjsonb_delete,
  LEFTARG   = tjsonb, RIGHTARG = text
);
CREATE OPERATOR - (
  PROCEDURE = tjsonb_delete_array,
  LEFTARG   = tjsonb, RIGHTARG = text[]
);
CREATE OPERATOR - (
  PROCEDURE = tjsonb_delete_idx,
  LEFTARG   = tjsonb, RIGHTARG = integer
);

CREATE FUNCTION tjsonb_delete_path(temp tjsonb, path text[])
RETURNS tjsonb
AS 'MODULE_PATHNAME', 'Tjsonb_delete_path'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR #- (
  PROCEDURE = tjsonb_delete_path,
  LEFTARG   = tjsonb, RIGHTARG = text[]
);

/*****************************************************************************/

CREATE FUNCTION tjsonb_set(temp tjsonb, path text[], val jsonb,
  create_missing boolean DEFAULT true)
RETURNS tjsonb
AS 'MODULE_PATHNAME', 'Tjsonb_set'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tjsonb_set_lax(temp tjsonb, path text[], val jsonb,
  create_missing boolean DEFAULT true, handle_null text DEFAULT '')
RETURNS tjsonb
AS 'MODULE_PATHNAME', 'Tjsonb_set_lax'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION tjsonb_insert(temp tjsonb, path text[], val jsonb,
  after boolean DEFAULT false)
RETURNS tjsonb
AS 'MODULE_PATHNAME', 'Tjsonb_insert'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION tjsonb_extract_jsonpath(tjsonb, jsonpath)
RETURNS tjsonb
AS 'MODULE_PATHNAME', 'Tjsonb_extract_jsonpath'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION tjson_strip_nulls(ttext, bool DEFAULT FALSE) 
RETURNS ttext
AS 'MODULE_PATHNAME', 'Tjson_strip_nulls'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tjsonb_strip_nulls(tjsonb, bool DEFAULT FALSE) 
RETURNS tjsonb
AS 'MODULE_PATHNAME', 'Tjsonb_strip_nulls'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION tjsonb_pretty(tjsonb) 
RETURNS ttext
AS 'MODULE_PATHNAME', 'Tjsonb_pretty'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

/*****************************************************************************/

