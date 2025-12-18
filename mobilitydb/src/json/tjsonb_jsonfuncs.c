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
 * @brief Basic functions for temporal JSONB
 */

/* PostgreSQL */
#include <stdbool.h>

/* PostgreSQL */
#include <postgres.h>
#include "utils/array.h"
#include "utils/jsonb.h"
#include "utils/jsonpath.h"
#include "executor/spi.h"

#include "utils/jsonpath.h"
#include "executor/spi.h"

/* MEOS */
#include <meos.h>
#include <meos_json.h>
#include "temporal/span.h"
#include "json/tjsonb.h"
#include <pgtypes.h>
/* MobilityDB */
#include "pg_temporal/temporal.h"

/*****************************************************************************
 * Temporal JSONB casting to/from temporal text
 *****************************************************************************/

PGDLLEXPORT Datum Jsonb_as_text(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Jsonb_as_text);
/**
 * @ingroup mobilitydb_json_conv
 * @brief Transform a JSONB value into a text value
 * @sqlfn text()
 * @sqlop @p ::
 */
Datum
Jsonb_as_text(PG_FUNCTION_ARGS)
{
  Jsonb *jb = PG_GETARG_JSONB_P(0);
  char *str = pg_jsonb_out(jb);
  text *result = cstring_to_text(str);
  pfree(str);
  PG_RETURN_TEXT_P(result);
}

PGDLLEXPORT Datum Tjsonb_as_ttext(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tjsonb_as_ttext);
/**
 * @ingroup mobilitydb_json_conv
 * @brief Transform a temporal jsonb value into a temporal text value
 * @sqlfn ttext()
 * @sqlop @p ::
 */
Datum
Tjsonb_as_ttext(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  Temporal *result = tjsonb_to_ttext(temp);
  PG_FREE_IF_COPY(temp, 0);
  PG_RETURN_TEMPORAL_P(result);
}

PGDLLEXPORT Datum Ttext_as_tjsonb(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Ttext_as_tjsonb);
/**
 * @ingroup mobilitydb_json_conv
 * @brief Transform a temporal jsonb value into a temporal text value
 * @sqlfn ttext()
 * @sqlop @p ::
 */
Datum
Ttext_as_tjsonb(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  Temporal *result = ttext_to_tjsonb(temp);
  PG_FREE_IF_COPY(temp, 0);
  if (! result)
    PG_RETURN_NULL();
  PG_RETURN_TEMPORAL_P(result);
}

/*****************************************************************************
 * JSON functions
 *****************************************************************************/

PGDLLEXPORT Datum Tjson_object_field(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tjson_object_field);
/**
 * @ingroup mobilitydb_json_json
 * @brief Extract a field from a temporal JSON value
 * @sqlfn tjson_object_field()
 * @sqlop @p ->
 */
Datum
Tjson_object_field(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  text *key = PG_GETARG_TEXT_P(1);
  Temporal *result = tjson_object_field(temp, key, false);
  PG_FREE_IF_COPY(temp, 0);
  PG_FREE_IF_COPY(key, 1);
  if (! result)
    PG_RETURN_NULL();
  PG_RETURN_TEMPORAL_P(result);
}

PGDLLEXPORT Datum Tjsonb_object_field(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tjsonb_object_field);
/**
 * @ingroup mobilitydb_json_json
 * @brief Extract a field from a temporal JSONB value
 * @sqlfn tjsonb_object_field()
 * @sqlop @p ->
 */
Datum
Tjsonb_object_field(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  text *key = PG_GETARG_TEXT_P(1);
  Temporal *result = tjsonb_object_field(temp, key, false);
  PG_FREE_IF_COPY(temp, 0);
  PG_FREE_IF_COPY(key, 1);
  if (! result)
    PG_RETURN_NULL();
  PG_RETURN_TEMPORAL_P(result);
}

PGDLLEXPORT Datum Tjson_object_field_text(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tjson_object_field_text);
/**
 * @ingroup mobilitydb_json_json
 * @brief Extract a field from a temporal JSON value as text
 * @sqlfn tjson_object_field_text()
 * @sqlop @p ->>
 */
Datum
Tjson_object_field_text(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  text *key = PG_GETARG_TEXT_P(1);
  Temporal *result = tjson_object_field(temp, key, true);
  PG_FREE_IF_COPY(temp, 0);
  PG_FREE_IF_COPY(key, 1);
  if (! result)
    PG_RETURN_NULL();
  PG_RETURN_TEMPORAL_P(result);
}

PGDLLEXPORT Datum Tjsonb_object_field_text(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tjsonb_object_field_text);
/**
 * @ingroup mobilitydb_json_json
 * @brief Extract a field from a temporal JSONB value as text
 * @sqlfn tjsonb_object_field_text()
 * @sqlop @p ->>
 */
Datum
Tjsonb_object_field_text(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  text *key = PG_GETARG_TEXT_P(1);
  Temporal *result = tjsonb_object_field(temp, key, true);
  PG_FREE_IF_COPY(temp, 0);
  PG_FREE_IF_COPY(key, 1);
  if (! result)
    PG_RETURN_NULL();
  PG_RETURN_TEMPORAL_P(result);
}

/*****************************************************************************/

PGDLLEXPORT Datum Concat_jsonb_tjsonb(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Concat_jsonb_tjsonb);
/**
 * @ingroup mobilitydb_json_json
 * @brief Concat a JSONB constant with a temporal JSONB
 * @sqlfn tjsonb_concat()
 * @sqlop @p ||
 */
Datum
Concat_jsonb_tjsonb(PG_FUNCTION_ARGS)
{
  Jsonb *jb = PG_GETARG_JSONB_P(0);
  Temporal *temp = PG_GETARG_TEMPORAL_P(1);
  Temporal *result = concat_tjsonb_jsonb(temp, jb, INVERT);
  PG_FREE_IF_COPY(jb, 0);
  PG_FREE_IF_COPY(temp, 1);
  PG_RETURN_TEMPORAL_P(result);
}

PGDLLEXPORT Datum Concat_tjsonb_jsonb(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Concat_tjsonb_jsonb);
/**
 * @ingroup mobilitydb_json_json
 * @brief Concat a temporal JSONB with a JSONB constant
 * @sqlfn jsonb_concat()
 * @sqlop @p ||
 */
Datum
Concat_tjsonb_jsonb(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  Jsonb *jb = PG_GETARG_JSONB_P(1);
  Temporal *result = concat_tjsonb_jsonb(temp, jb, INVERT_NO);
  PG_FREE_IF_COPY(temp, 0);
  PG_FREE_IF_COPY(jb, 1);
  PG_RETURN_TEMPORAL_P(result);
}

PGDLLEXPORT Datum Concat_tjsonb_tjsonb(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Concat_tjsonb_tjsonb);
/**
 * @ingroup mobilitydb_json_json
 * @brief Concat two temporal jsonb values
 * @sqlfn jsonb_concat()
 * @sqlop @p ||
 */
Datum
Concat_tjsonb_tjsonb(PG_FUNCTION_ARGS)
{
  Temporal *temp1 = PG_GETARG_TEMPORAL_P(0);
  Temporal *temp2 = PG_GETARG_TEMPORAL_P(1);
  Temporal *result = concat_tjsonb_tjsonb(temp1, temp2);
  PG_FREE_IF_COPY(temp1, 0);
  PG_FREE_IF_COPY(temp2, 1);
  if (! result)
    PG_RETURN_NULL();
  PG_RETURN_TEMPORAL_P(result);
}

/*****************************************************************************/

PGDLLEXPORT Datum Tjsonb_delete_key(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tjsonb_delete_key);
/**
 * @ingroup mobilitydb_json_json
 * @brief Delete a key from a temporal jsonb value
 * @sqlfn jsonb_delete()
 * @sqlop @p -
 */
Datum
Tjsonb_delete_key(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  text *key = PG_GETARG_TEXT_P(1);
  Temporal *result = tjsonb_delete_key(temp, key);
  PG_FREE_IF_COPY(temp, 0);
  PG_FREE_IF_COPY(key, 1);
  if (! result)
    PG_RETURN_NULL();
  PG_RETURN_TEMPORAL_P(result);
}

PGDLLEXPORT Datum Tjsonb_delete_key_array(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tjsonb_delete_key_array);
/**
 * @ingroup mobilitydb_json_json
 * @brief Delete an array of keys from a temporal jsonb value
 * @sqlfn jsonb_delete_array()
 * @sqlop @p -
 */
Datum
Tjsonb_delete_key_array(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  ArrayType *keys = PG_GETARG_ARRAYTYPE_P(1);
  if (ARR_NDIM(keys) > 1)
    ereport(ERROR, (errcode(ERRCODE_ARRAY_SUBSCRIPT_ERROR),
       errmsg("wrong number of array subscripts")));

  /* Extract the keys from the array */
  int keys_len;
  Datum *keys_elems;
  bool *keys_nulls;
  deconstruct_array(keys, TEXTOID, -1, false, 'd', &keys_elems, &keys_nulls,
    &keys_len);
  if (keys_len == 0)
    PG_RETURN_TEMPORAL_P(temp);

  /* Compute the result */
  Temporal *result = tjsonb_delete_key_array(temp, (text **) keys_elems,
    keys_len);
  pfree(keys_elems); pfree(keys_nulls);
  PG_FREE_IF_COPY(temp, 0);
  PG_FREE_IF_COPY(keys, 1);
  if (! result)
    PG_RETURN_NULL();
  PG_RETURN_TEMPORAL_P(result);
}

PGDLLEXPORT Datum Tjsonb_delete_idx(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tjsonb_delete_idx);
/**
 * @ingroup mobilitydb_json_json
 * @brief Delete a key from a temporal jsonb value
 * @sqlfn jsonb_delete()
 * @sqlop @p -
 */
Datum
Tjsonb_delete_idx(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  int idx = PG_GETARG_INT32(1);
  Temporal *result = tjsonb_delete_idx(temp, idx);
  PG_FREE_IF_COPY(temp, 0);
  if (! result)
    PG_RETURN_NULL();
  PG_RETURN_TEMPORAL_P(result);
}

/*****************************************************************************/

PGDLLEXPORT Datum Tjson_array_element(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tjson_array_element);
/**
 * @ingroup mobilitydb_json_json
 * @brief Extract an element from a temporal jsonb array
 * @sqlfn tjson_array_element()
 */
Datum
Tjson_array_element(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  int element = PG_GETARG_INT32(1);
  Temporal *result = tjson_array_element(temp, element, false);
  PG_FREE_IF_COPY(temp, 0);
  if (! result)
    PG_RETURN_NULL();
  PG_RETURN_TEMPORAL_P(result);
}

PGDLLEXPORT Datum Tjsonb_array_element(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tjsonb_array_element);
/**
 * @ingroup mobilitydb_json_json
 * @brief Extract an element from a temporal jsonb array
 * @sqlfn tjsonb_array_element()
 */
Datum
Tjsonb_array_element(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  int element = PG_GETARG_INT32(1);
  Temporal *result = tjsonb_array_element(temp, element, false);
  PG_FREE_IF_COPY(temp, 0);
  if (! result)
    PG_RETURN_NULL();
  PG_RETURN_TEMPORAL_P(result);
}

PGDLLEXPORT Datum Tjson_array_element_text(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tjson_array_element_text);
/**
 * @ingroup mobilitydb_json_json
 * @brief Extract an element from a temporal JSON array as text
 * @sqlfn tjson_array_element_text()
 */
Datum
Tjson_array_element_text(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  int element = PG_GETARG_INT32(1);
  Temporal *result = tjson_array_element(temp, element, true);
  PG_FREE_IF_COPY(temp, 0);
  if (! result)
    PG_RETURN_NULL();
  PG_RETURN_TEMPORAL_P(result);
}

PGDLLEXPORT Datum Tjsonb_array_element_text(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tjsonb_array_element_text);
/**
 * @ingroup mobilitydb_json_json
 * @brief Extract an element from a temporal JSONB array as text
 * @sqlfn tjsonb_array_element_text()
 */
Datum
Tjsonb_array_element_text(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  int element = PG_GETARG_INT32(1);
  Temporal *result = tjsonb_array_element(temp, element, true);
  PG_FREE_IF_COPY(temp, 0);
  if (! result)
    PG_RETURN_NULL();
  PG_RETURN_TEMPORAL_P(result);
}

/*****************************************************************************/

/**
 * @brief Extract a path from a temporal jsonb value
 */
Datum
Tjson_extract_path_ext(FunctionCallInfo fcinfo, bool isjsonb, bool astext)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  ArrayType *path = PG_GETARG_ARRAYTYPE_P(1);
  if (ARR_NDIM(path) > 1)
    ereport(ERROR, (errcode(ERRCODE_ARRAY_SUBSCRIPT_ERROR),
       errmsg("wrong number of array subscripts")));

  /* Extract the path from the array */
  int path_len;
  Datum *path_elems;
  bool *path_nulls;
  deconstruct_array(path, TEXTOID, -1, false, 'i', &path_elems, &path_nulls,
    &path_len);
  if (path_len == 0)
    PG_RETURN_TEMPORAL_P(temp);
  /*
   * If the array contains any null elements, return NULL, on the grounds
   * that you'd have gotten NULL if any RHS value were NULL in a nested
   * series of applications of the -> operator.  (Note: because we also
   * return NULL for error cases such as no-such-field, this is true
   * regardless of the contents of the rest of the array.)
   */
  if (array_contains_nulls(path))
    PG_RETURN_NULL();
  
  /* Compute the result */
  Temporal *result = isjsonb ?
    tjsonb_extract_path(temp, (text **) path_elems, path_len, astext) :
    tjson_extract_path(temp, (text **) path_elems, path_len, astext);

  pfree(path_elems); pfree(path_nulls);
  PG_FREE_IF_COPY(temp, 0);
  PG_FREE_IF_COPY(path, 1);
  if (! result)
    PG_RETURN_NULL();
  PG_RETURN_TEMPORAL_P(result);
}

PGDLLEXPORT Datum Tjson_extract_path(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tjson_extract_path);
/**
 * @ingroup mobilitydb_json_json
 * @brief Extract a path from a temporal jsonb value
 * @sqlfn Tjsonb_delete_path()
 */
Datum
Tjson_extract_path(PG_FUNCTION_ARGS)
{
  return Tjson_extract_path_ext(fcinfo, false, false);
}

PGDLLEXPORT Datum Tjson_extract_path_text(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tjson_extract_path_text);
/**
 * @ingroup mobilitydb_json_json
 * @brief Extract a path from a temporal jsonb value
 * @sqlfn Tjsonb_delete_path()
 */
Datum
Tjson_extract_path_text(PG_FUNCTION_ARGS)
{
  return Tjson_extract_path_ext(fcinfo, false, true);
}

PGDLLEXPORT Datum Tjsonb_extract_path(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tjsonb_extract_path);
/**
 * @ingroup mobilitydb_json_json
 * @brief Extract a path from a temporal jsonb value
 * @sqlfn Tjsonb_delete_path()
 */
Datum
Tjsonb_extract_path(PG_FUNCTION_ARGS)
{
  return Tjson_extract_path_ext(fcinfo, true, false);
}

PGDLLEXPORT Datum Tjsonb_extract_path_text(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tjsonb_extract_path_text);
/**
 * @ingroup mobilitydb_json_json
 * @brief Extract a path from a temporal jsonb value
 * @sqlfn Tjsonb_delete_path()
 */
Datum
Tjsonb_extract_path_text(PG_FUNCTION_ARGS)
{
  return Tjson_extract_path_ext(fcinfo, true, true);
}

/*****************************************************************************/

/**
 * @brief Replace a JSONB value specified by a path with a new value
 * @sqlfn tjsonb_set(), tjsonb_set_lax()
 */
Datum
Tjsonb_set_ext(FunctionCallInfo fcinfo, bool lax)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  ArrayType *path = PG_GETARG_ARRAYTYPE_P(1);
  Jsonb *newjsonb = PG_GETARG_JSONB_P(2);
  bool create = PG_GETARG_BOOL(3);
  text *handle_null = NULL;
  if (lax)
    handle_null = PG_GETARG_TEXT_P(4);
 
 if (ARR_NDIM(path) > 1)
    ereport(ERROR, (errcode(ERRCODE_ARRAY_SUBSCRIPT_ERROR),
      errmsg("wrong number of array subscripts")));

  Datum *path_elems;
  bool *path_nulls;
  int path_len;
  deconstruct_array(path, TEXTOID, -1, false, 'd', &path_elems, &path_nulls,
    &path_len);
  if (path_len == 0)
    PG_RETURN_TEMPORAL_P(temp);

  /* Compute the result */
  Temporal *result = tjsonb_set(temp, (text **) path_elems, path_len,
    newjsonb, create, handle_null, lax);
  pfree(path_elems); pfree(path_nulls);
  PG_FREE_IF_COPY(temp, 0);
  PG_FREE_IF_COPY(path, 1);
  if (! result)
    PG_RETURN_NULL();
  PG_RETURN_TEMPORAL_P(result);
}

PGDLLEXPORT Datum Tjsonb_set(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tjsonb_set);
/**
 * @ingroup mobilitydb_temporal_jsonb
 * @brief Replace a JSONB value specified by a path with a new value
 * @sqlfn tjsonb_set()
 */
Datum
Tjsonb_set(PG_FUNCTION_ARGS)
{
  return Tjsonb_set_ext(fcinfo, false);
}

PGDLLEXPORT Datum Tjsonb_set_lax(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tjsonb_set_lax);
/**
 * @ingroup mobilitydb_temporal_jsonb
 * @brief Replace a JSONB value specified by a path with a new value using the
 * lax mode
 * @sqlfn tjsonb_set_lax()
 */
Datum
Tjsonb_set_lax(PG_FUNCTION_ARGS)
{
  return Tjsonb_set_ext(fcinfo, true);
}

/*****************************************************************************/

PGDLLEXPORT Datum Tjsonb_delete_path(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tjsonb_delete_path);
/**
 * @ingroup mobilitydb_json_json
 * @brief Delete a path from a temporal jsonb value
 * @sqlfn Tjsonb_delete_path()
 */
Datum
Tjsonb_delete_path(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  ArrayType *path = PG_GETARG_ARRAYTYPE_P(1);
  if (ARR_NDIM(path) > 1)
    ereport(ERROR, (errcode(ERRCODE_ARRAY_SUBSCRIPT_ERROR),
       errmsg("wrong number of array subscripts")));

  /* Extract the path from the array */
  int path_len;
  Datum *path_elems;
  bool *path_nulls;
  deconstruct_array(path, TEXTOID, -1, false, 'd', &path_elems, &path_nulls,
    &path_len);
  if (path_len == 0)
    PG_RETURN_TEMPORAL_P(temp);

  /* Compute the result */
  Temporal *result = tjsonb_delete_path(temp, (text **) path_elems, path_len);

  pfree(path_elems); pfree(path_nulls);
  PG_FREE_IF_COPY(temp, 0);
  PG_FREE_IF_COPY(path, 1);
  if (! result)
    PG_RETURN_NULL();
  PG_RETURN_TEMPORAL_P(result);
}

/*****************************************************************************/

PGDLLEXPORT Datum Tjsonb_insert(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tjsonb_insert);
/**
 * @ingroup mobilitydb_json_json
 * @brief Insert a path into a temporal jsonb value
 * @sqlfn tjsonb_insert()
 */
Datum
Tjsonb_insert(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  ArrayType *path = PG_GETARG_ARRAYTYPE_P(1);
  Jsonb *newjsonb = PG_GETARG_JSONB_P(2);
  bool after = PG_GETARG_BOOL(3);
  if (ARR_NDIM(path) > 1)
    ereport(ERROR, (errcode(ERRCODE_ARRAY_SUBSCRIPT_ERROR),
      errmsg("wrong number of array subscripts")));

  /* Extract the path from the array */
  int path_len;
  Datum *path_elems;
  bool *path_nulls;
  deconstruct_array(path, TEXTOID, -1, false, 'd', &path_elems, &path_nulls,
    &path_len);
  if (path_len == 0)
    PG_RETURN_TEMPORAL_P(temp);

  /* Compute the result */
  Temporal *result = tjsonb_insert(temp, (text **) path_elems, path_len,
    newjsonb, after);
  pfree(path_elems); pfree(path_nulls);
  PG_FREE_IF_COPY(temp, 0);
  PG_FREE_IF_COPY(path, 1);
  if (! result)
    PG_RETURN_NULL();
  PG_RETURN_TEMPORAL_P(result);
}

/*****************************************************************************/

/**
 * @brief Convert a temporal JSONB into a temporal alphanumeric type
 * @sqlfn tjsonb_to_tint()
 */
Datum
Tjsonb_to_talphanum(FunctionCallInfo fcinfo, meosType temptype)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  text *key_text = PG_GETARG_TEXT_PP(1);
  char *key = pg_text_to_cstring(key_text);
  interpType interp = INTERP_NONE;
  if (PG_NARGS() > 2 && ! PG_ARGISNULL(2))
    interp = input_interp_string(fcinfo, 2);
  Temporal *result = tjsonb_to_talphanum(temp, key, temptype, interp);
  PG_FREE_IF_COPY(temp, 0);
  PG_FREE_IF_COPY(key_text, 1);
  PG_RETURN_POINTER(result);
}

PGDLLEXPORT Datum Tjsonb_to_tbool(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tjsonb_to_tbool);
/**
 * @ingroup mobilitydb_json_json
 * @brief Convert a temporal JSONB into a temporal boolean
 * @sqlfn tbool()
 */
Datum
Tjsonb_to_tbool(PG_FUNCTION_ARGS)
{
  return Tjsonb_to_talphanum(fcinfo, T_TBOOL);
}

PGDLLEXPORT Datum Tjsonb_to_tint(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tjsonb_to_tint);
/**
 * @ingroup mobilitydb_json_json
 * @brief Convert a temporal JSONB into a temporal integer
 * @sqlfn tint()
 */
Datum
Tjsonb_to_tint(PG_FUNCTION_ARGS)
{
  return Tjsonb_to_talphanum(fcinfo, T_TINT);
}

PGDLLEXPORT Datum Tjsonb_to_tfloat(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tjsonb_to_tfloat);
/**
 * @ingroup mobilitydb_json_json
 * @brief Convert a temporal JSONB into a temporal float
 * @sqlfn tfloat()
 */
Datum
Tjsonb_to_tfloat(PG_FUNCTION_ARGS)
{
  return Tjsonb_to_talphanum(fcinfo, T_TFLOAT);
}

PGDLLEXPORT Datum Tjsonb_to_ttext_key(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tjsonb_to_ttext_key);
/**
 * @ingroup mobilitydb_json_json
 * @brief Convert a temporal JSONB into a temporal text
 * @sqlfn ttext()
 */
Datum
Tjsonb_to_ttext_key(PG_FUNCTION_ARGS)
{
  return Tjsonb_to_talphanum(fcinfo, T_TTEXT);
}

/*****************************************************************************/

PGDLLEXPORT Datum Tjson_strip_nulls(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tjson_strip_nulls);
/**
 * @ingroup mobilitydb_json_json
 * @brief Return a temporal JSON value without nulls
 * @sqlfn tjson_strip_nulls()
 */
Datum
Tjson_strip_nulls(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  bool strip_nulls = PG_GETARG_BOOL(1);
  Temporal *result = tjson_strip_nulls(temp, strip_nulls);
  PG_FREE_IF_COPY(temp, 0);
  PG_RETURN_POINTER(result);
}

PGDLLEXPORT Datum Tjsonb_strip_nulls(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tjsonb_strip_nulls);
/**
 * @ingroup mobilitydb_json_json
 * @brief Return a temporal JSON value without nulls
 * @sqlfn tjsonb_strip_nulls()
 */
Datum
Tjsonb_strip_nulls(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  bool strip_nulls = PG_GETARG_BOOL(1);
  Temporal *result = tjsonb_strip_nulls(temp, strip_nulls);
  PG_FREE_IF_COPY(temp, 0);
  PG_RETURN_POINTER(result);
}

/*****************************************************************************/

PGDLLEXPORT Datum Tjsonb_pretty(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tjsonb_pretty);
/**
 * @ingroup mobilitydb_json_json
 * @brief Return a temporal JSONB value without nulls
 * @sqlfn tjsonb_pretty()
 */
Datum
Tjsonb_pretty(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  Temporal *result = tjsonb_pretty(temp);
  PG_FREE_IF_COPY(temp, 0);
  PG_RETURN_POINTER(result);
}

/*****************************************************************************
 * JSONB path query
 *****************************************************************************/

PGDLLEXPORT Datum Tjsonb_extract_jsonpath(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tjsonb_extract_jsonpath);
/**
 * @ingroup mobilitydb_json_json
 * @brief Extract values from a temporal JSONB using a JSONPath expression
 * This function applies a JSONPath expression to each instant of a temporal
 * JSONB and returns a new temporal JSONB with the extracted values.
 * Implementation details:
 * - PostgreSQL 14 version: relies on SPI and calls
 *   @p jsonb_path_query_first($1, $2).
 * - If the path does not exist, a JSON null value is returned.
 * @return A new temporal JSONB sequence containing the extracted values
 * @note In PostgreSQL 15 and higher, @p jsonb_path_query_first_typed can be
 *       used directly instead of going through SPI.
 * @sqlfn tjsonb_extract_jsonpath()
 */
Datum
Tjsonb_extract_jsonpath(PG_FUNCTION_ARGS)
{
    Temporal *temp = PG_GETARG_TEMPORAL_P(0);
    JsonPath *jspath = PG_GETARG_JSONPATH_P(1);
    int count;
    const TInstant **insts = temporal_insts_p(temp, &count);
    TInstant **new_insts = palloc(sizeof(TInstant *) * count);
    /* Start SPI context (needed to call SQL function safely) */
    if (SPI_connect() != SPI_OK_CONNECT)
      ereport(ERROR, (errmsg("SPI_connect failed")));
    Oid argtypes[2] = {JSONBOID, JSONPATHOID};

    for (int i = 0; i < count; i++)
    {
      Datum val = tinstant_value(insts[i]);
      Datum values[2] = {val, PointerGetDatum(jspath)};
      char nulls[2] = {' ', ' '};
      Datum newval;
      int ret = SPI_execute_with_args(
        "SELECT jsonb_path_query_first($1, $2)",
        2, argtypes, values, nulls, true, 1);
      if (ret == SPI_OK_SELECT && SPI_processed > 0)
      {
        HeapTuple tup = SPI_tuptable->vals[0];
        TupleDesc tupdesc = SPI_tuptable->tupdesc;
        bool isnull;
        newval = SPI_getbinval(tup, tupdesc, 1, &isnull);
        if (isnull)
          newval = PointerGetDatum(pg_jsonb_in("null"));
      }
      else
        newval = PointerGetDatum(pg_jsonb_in("null"));
      new_insts[i] = tinstant_make(newval, T_TJSONB, insts[i]->t);
    }
    SPI_finish();
    /* Build result sequence */
    TSequence *result = tsequence_make(new_insts, count, true, true, STEP,
      NORMALIZE);

    PG_FREE_IF_COPY(temp, 0);
    PG_RETURN_TSEQUENCE_P(result);
}

/*****************************************************************************/