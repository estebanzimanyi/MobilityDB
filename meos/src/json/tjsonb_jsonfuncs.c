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

/* C */
#include <assert.h>
#include <float.h>
/* PostgreSQL */
#include <postgres.h>
#include "utils/varlena.h"
/* MEOS */
#include <meos.h>
#include <meos_json.h>
#include <meos_internal.h>
#include <pgtypes.h>
#include "temporal/meos_catalog.h"
#include "temporal/temporal.h"
#include "temporal/lifting.h"
#include "temporal/type_parser.h"
#include "temporal/type_util.h"
#include "json/tjsonb.h"

/*****************************************************************************
 * Datum functions on JSON
 * These functions  are used by the lifting infrastructure
 *****************************************************************************/

/**
 * @brief Return the concatenation of the two JSONB values (objects ou arrays)
 * @note Derived from the PostgreSQL function @p jsonb_concat(PG_FUNCTION_ARGS)
 */
Jsonb *
concat_jsonb_jsonb(const Jsonb *jb1, const Jsonb *jb2)
{
  return pg_jsonb_concat(jb1, jb2);
}

/**
 * @brief Return the concatenation of the two JSONB values (objects ou arrays)
 */
Datum
datum_jsonb_concat(Datum l, Datum r)
{
  return PointerGetDatum(pg_jsonb_concat(DatumGetJsonbP(l),
    DatumGetJsonbP(r)));
}

/*****************************************************************************/

/**
 * @brief Return true if the first JSON value is contained into the second one
 */
Datum
datum_jsonb_contained(Datum l, Datum r)
{
  return BoolGetDatum(pg_jsonb_contained(DatumGetJsonbP(l),
    DatumGetJsonbP(r)));
}

/**
 * @brief Return true if the first JSON value contains the second one
 */
Datum
datum_jsonb_contains(Datum l, Datum r)
{
  return BoolGetDatum(pg_jsonb_contains(DatumGetJsonbP(l), DatumGetJsonbP(r)));
}

/*****************************************************************************/

/**
 * @brief Return a copy of the JSONB value with the indicated item removed
 */
Datum
datum_jsonb_delete(Datum jb, Datum key)
{
  return PointerGetDatum(pg_jsonb_delete(DatumGetJsonbP(jb),
    DatumGetTextP(key)));
}

/**
 * @brief Return a copy of the JSONB value with the indicated item removed
 */
Datum
datum_jsonb_delete_array(Datum jb, Datum array, Datum count)
{
  Jsonb *res = pg_jsonb_delete_array(DatumGetJsonbP(jb),
    (text **) DatumGetPointer(array), DatumGetInt32(count));
  if (! res)
    return (Datum) NULL;
  Jsonb *empty = pg_jsonb_in("{}");
  bool isempty = pg_jsonb_eq(res, empty);
  pfree(empty);
  if (isempty)
  {
    pfree(res);
    return (Datum) NULL;
  }
  return PointerGetDatum(res);
}

/**
 * @brief Return a copy of the JSONB value with the indicated index removed
 */
Datum
datum_jsonb_delete_idx(Datum jb, Datum idx)
{
  Jsonb *res = pg_jsonb_delete_idx(DatumGetJsonbP(jb), DatumGetInt32(idx));
  if (! res)
    return (Datum) NULL;
  Jsonb *empty = pg_jsonb_in("[]");
  bool isempty = pg_jsonb_eq(res, empty);
  pfree(empty);
  if (isempty)
  {
    pfree(res);
    return (Datum) NULL;
  }
  return PointerGetDatum(res);
}

/*****************************************************************************/

/**
 * @brief Extract an element from a JSONB value array
 */
Datum
datum_json_array_element(Datum txt, Datum element)
{
  return PointerGetDatum(pg_json_array_element(DatumGetTextP(txt),
    DatumGetInt32(element)));
}

/**
 * @brief Extract an element from a JSONB value array
 */
Datum
datum_jsonb_array_element(Datum jb, Datum element)
{
  return PointerGetDatum(pg_jsonb_array_element(DatumGetJsonbP(jb),
    DatumGetInt32(element)));
}

/**
 * @brief Extract an element from a JSONB value array
 */
Datum
datum_json_array_element_text(Datum txt, Datum element)
{
  return PointerGetDatum(pg_json_array_element_text(DatumGetTextP(txt),
    DatumGetInt32(element)));
}

/**
 * @brief Extract an element from a JSONB value array
 */
Datum
datum_jsonb_array_element_text(Datum jb, Datum element)
{
  return PointerGetDatum(pg_jsonb_array_element_text(DatumGetJsonbP(jb),
    DatumGetInt32(element)));
}

/*****************************************************************************/

/**
 * @brief Return true if the text string exist as a top-level key or array
 * element within the JSON value
 */
Datum
datum_jsonb_exists(Datum l, Datum r)
{
  return BoolGetDatum(pg_jsonb_exists(DatumGetJsonbP(l), DatumGetTextP(r)));
}

/**
 * @brief Return a copy of the JSONB value with the indicated item removed
 */
Datum
datum_jsonb_exists_array(Datum value, Datum array, Datum count, Datum any)
{
  return BoolGetDatum(jsonb_exists_array(DatumGetJsonbP(value),
    (text **) DatumGetPointer(array), DatumGetInt32(count),
    DatumGetBool(any)));
}

/*****************************************************************************/

/**
 * @brief Extract a JSON object field with the given key
 */
Datum
datum_json_object_field(Datum txt, Datum key)
{
  return PointerGetDatum(pg_json_object_field(DatumGetTextP(txt),
    DatumGetTextP(key)));
}

/**
 * @brief Extract a JSONB object field with the given key
 */
Datum
datum_jsonb_object_field(Datum jb, Datum key)
{
  return PointerGetDatum(pg_jsonb_object_field(DatumGetJsonbP(jb),
    DatumGetTextP(key)));
}

/**
 * @brief Extract a JSON object field with the given key
 */
Datum
datum_json_object_field_text(Datum txt, Datum key)
{
  return PointerGetDatum(pg_json_object_field_text(DatumGetTextP(txt),
    DatumGetTextP(key)));
}

/**
 * @brief Extract a JSONB object field with the given key
 */
Datum
datum_jsonb_object_field_text(Datum jb, Datum key)
{
  return PointerGetDatum(pg_jsonb_object_field_text(DatumGetJsonbP(jb),
    DatumGetTextP(key)));
}

/*****************************************************************************/

/**
 * @brief Return the text representation of a jsonb value
 */
Datum
datum_json_strip_nulls(Datum txt, Datum strip_in_arrays)
{
  return PointerGetDatum(pg_json_strip_nulls(DatumGetTextP(txt),
    DatumGetBool(strip_in_arrays)));
}

/**
 * @brief Return the text representation of a jsonb value
 */
Datum
datum_jsonb_strip_nulls(Datum jb, Datum strip_in_arrays)
{
  return PointerGetDatum(pg_jsonb_strip_nulls(DatumGetJsonbP(jb),
    DatumGetBool(strip_in_arrays)));
}

/*****************************************************************************/


/**
 * @brief Convert a temporal JSONB value to pretty-printed, indented text
 */
Datum
datum_jsonb_pretty(Datum jb)
{
  return PointerGetDatum(pg_jsonb_pretty(DatumGetJsonbP(jb)));
}

/*****************************************************************************/

/**
 * @brief Extract a JSON value specified by a path
 */
Datum
datum_json_extract_path(Datum txt, Datum path_elems, Datum path_len)
{
  return PointerGetDatum(pg_json_extract_path(DatumGetTextP(txt),
    (text **) DatumGetPointer(path_elems), DatumGetInt32(path_len)));
}

/**
 * @brief Extract a JSONB value specified by a path
 */
Datum
datum_jsonb_extract_path(Datum jb, Datum path_elems, Datum path_len)
{
  return PointerGetDatum(pg_jsonb_extract_path(DatumGetJsonbP(jb),
    (text **) DatumGetPointer(path_elems), DatumGetInt32(path_len)));
}

/**
 * @brief Extract a JSON value specified by a path as text
 */
Datum
datum_json_extract_path_text(Datum txt, Datum path_elems, Datum path_len)
{
  return PointerGetDatum(pg_json_extract_path_text(DatumGetTextP(txt),
    (text **) DatumGetPointer(path_elems), DatumGetInt32(path_len)));
}

/**
 * @brief Extract a JSONB value specified by a path as text
 */
Datum
datum_jsonb_extract_path_text(Datum jb, Datum path_elems, Datum path_len)
{
  return PointerGetDatum(pg_jsonb_extract_path_text(DatumGetJsonbP(jb),
    (text **) DatumGetPointer(path_elems), DatumGetInt32(path_len)));
}

/*****************************************************************************/

/**
 * @brief Replace a JSONB value specified by a path with a new value
 */
Datum
datum_jsonb_set(Datum jb, Datum keys, Datum count, Datum newjb, Datum create)
{
  return PointerGetDatum(pg_jsonb_set(DatumGetJsonbP(jb),
    (text **) DatumGetPointer(keys), DatumGetInt32(count),
    DatumGetJsonbP(newjb), DatumGetBool(create)));
}

/**
 * @brief Replace a JSONB value specified by a path with a new value using the
 * lax mode
 */
Datum
datum_jsonb_set_lax(Datum jb, Datum keys, Datum count, Datum newjb,
  Datum create, Datum handle_null)
{
  return PointerGetDatum(pg_jsonb_set_lax(DatumGetJsonbP(jb),
    (text **) DatumGetPointer(keys), DatumGetInt32(count),
    DatumGetJsonbP(newjb), DatumGetBool(create), DatumGetTextP(handle_null)));
}

/*****************************************************************************/

/**
 * @brief Replace a JSONB value specified by a path with a new value using the
 * lax mode
 */
Datum
datum_jsonb_delete_path(Datum jb, Datum keys, Datum count)
{
  return PointerGetDatum(pg_jsonb_delete_path(DatumGetJsonbP(jb),
    (text **) DatumGetPointer(keys), DatumGetInt32(count)));
}

/*****************************************************************************/

/**
 * @brief Insert a new value into a JSONB value specified by a path
 */
Datum
datum_jsonb_insert(Datum jb, Datum keys, Datum count, Datum newjb, Datum after)
{
  return PointerGetDatum(pg_jsonb_set(DatumGetJsonbP(jb),
    (text **) DatumGetPointer(keys), DatumGetInt32(count),
    DatumGetJsonbP(newjb), DatumGetBool(after)));
}

/*****************************************************************************
 * Temporal JSON functions
 *****************************************************************************/

/**
 * @ingroup meos_json_json
 * @brief Extract a JSON object field with the given key
 * @param[in] temp Temporal JSON value
 * @param[in] key Key
 * @param[in] astext True when the output is a temporal text, otherwise is a
 * jsonb
 * @csqlfn #Tjsonb_object_field()
 */
Temporal *
tjson_object_field(const Temporal *temp, const text *key, bool astext)
{
  /* Ensure the validity of the arguments */
  VALIDATE_TTEXT(temp, NULL); VALIDATE_NOT_NULL(key, NULL);

  LiftedFunctionInfo lfinfo;
  memset(&lfinfo, 0, sizeof(LiftedFunctionInfo));
  lfinfo.func = astext ? (varfunc) datum_json_object_field_text :
    (varfunc) datum_json_object_field;
  lfinfo.argtype[1] = T_TEXT;
  lfinfo.numparam = 1;
  lfinfo.param[0] = PointerGetDatum(key);
  lfinfo.restype = T_TTEXT;
  lfinfo.reserrcheck = true; /* Check for NULL result */
  lfinfo.reslinear = false;
  lfinfo.invert = INVERT_NO;
  lfinfo.discont = CONTINUOUS;
  return tfunc_temporal(temp, &lfinfo);
}

/**
 * @ingroup meos_json_json
 * @brief Extract a JSONB object field with the given key
 * @param[in] temp Temporal JSONB value
 * @param[in] key Key
 * @param[in] astext True when the output is a temporal text, otherwise is a
 * temporal jsonb
 * @csqlfn #Tjsonb_object_field()
 */
Temporal *
tjsonb_object_field(const Temporal *temp, const text *key, bool astext)
{
  /* Ensure the validity of the arguments */
  VALIDATE_TJSONB(temp, NULL); VALIDATE_NOT_NULL(key, NULL);

  LiftedFunctionInfo lfinfo;
  memset(&lfinfo, 0, sizeof(LiftedFunctionInfo));
  lfinfo.func = astext ? (varfunc) datum_jsonb_object_field_text :
    (varfunc) datum_jsonb_object_field;
  lfinfo.argtype[0] = T_TJSONB;
  lfinfo.numparam = 1;
  lfinfo.param[0] = PointerGetDatum(key);
  lfinfo.restype = astext ? T_TTEXT : T_TJSONB;
  lfinfo.reserrcheck = true; /* Check for NULL result */
  lfinfo.reslinear = false;
  lfinfo.invert = INVERT_NO;
  lfinfo.discont = CONTINUOUS;
  return tfunc_temporal(temp, &lfinfo);
}

/*****************************************************************************/

/**
 * @ingroup meos_json_json
 * @brief Return the concatenation of a temporal JSONB and a JSONB
 * @param[in] temp Temporal JSONB
 * @param[in] jb JSONB
 * @param[in] invert True if the arguments must be inverted
 * @csqlfn #Concat_tjsonb_tjsonb()
 */
Temporal *
concat_tjsonb_jsonb(const Temporal *temp, const Jsonb *jb, bool invert)
{
  /* Ensure the validity of the arguments */
  VALIDATE_TJSONB(temp, NULL); VALIDATE_NOT_NULL(jb, NULL);

  LiftedFunctionInfo lfinfo;
  memset(&lfinfo, 0, sizeof(LiftedFunctionInfo));
  lfinfo.func = (varfunc) datum_jsonb_concat;
  lfinfo.argtype[0] = T_TJSONB;
  lfinfo.argtype[1] = T_JSONB;
  lfinfo.restype = T_TJSONB;
  lfinfo.reslinear = false;
  lfinfo.invert = invert;
  lfinfo.discont = CONTINUOUS;
  return tfunc_temporal_base(temp, PointerGetDatum(jb), &lfinfo);
}

/**
 * @ingroup meos_json_json
 * @brief Return the concatenation of two temporal JSONB
 * @param[in] temp1,temp2 Temporal JSONBs
 * @csqlfn #Concat_tjsonb_tjsonb()
 */
Temporal *
concat_tjsonb_tjsonb(const Temporal *temp1, const Temporal *temp2)
{
  /* Ensure the validity of the arguments */
  VALIDATE_TJSONB(temp1, NULL); VALIDATE_TJSONB(temp2, NULL);

  LiftedFunctionInfo lfinfo;
  memset(&lfinfo, 0, sizeof(LiftedFunctionInfo));
  lfinfo.func = (varfunc) datum_jsonb_concat;
  lfinfo.argtype[0] = T_TJSONB;
  lfinfo.argtype[1] = T_TJSONB;
  lfinfo.restype = T_TJSONB;
  lfinfo.reslinear = false;
  lfinfo.invert = INVERT_NO;
  lfinfo.discont = CONTINUOUS;
  return tfunc_temporal_temporal(temp1, temp2, &lfinfo);
}

/*****************************************************************************/

/**
 * @ingroup meos_json_json
 * @brief Return the temporal contains of a temporal JSONB and a JSONB
 * @param[in] temp Temporal JSONB
 * @param[in] jb JSONB
 * @param[in] invert True if the arguments must be inverted
 * @csqlfn #Contains_tjsonb_jsonb()
 */
Temporal *
contains_tjsonb_jsonb(const Temporal *temp, const Jsonb *jb, bool invert)
{
  /* Ensure the validity of the arguments */
  VALIDATE_TJSONB(temp, NULL); VALIDATE_NOT_NULL(jb, NULL);

  LiftedFunctionInfo lfinfo;
  memset(&lfinfo, 0, sizeof(LiftedFunctionInfo));
  lfinfo.func = (varfunc) datum_jsonb_contains;
  lfinfo.argtype[0] = T_TJSONB;
  lfinfo.argtype[1] = T_JSONB;
  lfinfo.restype = T_TJSONB;
  lfinfo.reslinear = false;
  lfinfo.invert = invert;
  lfinfo.discont = CONTINUOUS;
  return tfunc_temporal_base(temp, PointerGetDatum(jb), &lfinfo);
}

/**
 * @ingroup meos_json_json
 * @brief Return the temporal contains of two temporal JSONB
 * @param[in] temp1,temp2 Temporal JSONBs
 * @csqlfn #Contains_tjsonb_tjsonb()
 */
Temporal *
contains_tjsonb_tjsonb(const Temporal *temp1, const Temporal *temp2)
{
  /* Ensure the validity of the arguments */
  VALIDATE_TJSONB(temp1, NULL); VALIDATE_TJSONB(temp2, NULL);

  LiftedFunctionInfo lfinfo;
  memset(&lfinfo, 0, sizeof(LiftedFunctionInfo));
  lfinfo.func = (varfunc) datum_jsonb_contains;
  lfinfo.argtype[0] = T_TJSONB;
  lfinfo.argtype[1] = T_TJSONB;
  lfinfo.restype = T_TJSONB;
  lfinfo.reslinear = false;
  lfinfo.invert = INVERT_NO;
  lfinfo.discont = CONTINUOUS;
  return tfunc_temporal_temporal(temp1, temp2, &lfinfo);
}

/*****************************************************************************/

/**
 * @ingroup meos_json_json
 * @brief Extract an element from a temporal JSON array
 * @param[in] temp Temporal JSON value
 * @param[in] element Element number
 * @param[in] astext True when the output is a temporal text
 */
Temporal *
tjson_array_element(const Temporal *temp, int element, bool astext)
{
  /* Ensure the validity of the arguments */
  VALIDATE_TTEXT(temp, NULL);

  LiftedFunctionInfo lfinfo;
  memset(&lfinfo, 0, sizeof(LiftedFunctionInfo));
  lfinfo.func = (varfunc) &datum_json_array_element;
  lfinfo.argtype[0] = T_TTEXT;
  lfinfo.numparam = 1;
  lfinfo.param[0] = DatumGetInt32(element);
  lfinfo.restype = T_TTEXT ;
  lfinfo.reserrcheck = true; /* Check for NULL result */
  return tfunc_temporal(temp, &lfinfo);
}

/**
 * @ingroup meos_json_json
 * @brief Extract an element from a temporal JSONB array
 * @param[in] temp Temporal JSONB value
 * @param[in] element Element number
 * @param[in] astext True when the output is a temporal text
 */
Temporal *
tjsonb_array_element(const Temporal *temp, int element, bool astext)
{
  /* Ensure the validity of the arguments */
  VALIDATE_TJSONB(temp, NULL);

  LiftedFunctionInfo lfinfo;
  memset(&lfinfo, 0, sizeof(LiftedFunctionInfo));
  lfinfo.func = astext ? (varfunc) &datum_jsonb_array_element_text :
    (varfunc) &datum_jsonb_array_element;
  lfinfo.argtype[0] = T_TJSONB;
  lfinfo.numparam = 1;
  lfinfo.param[0] = DatumGetInt32(element);
  lfinfo.restype = astext ? T_TTEXT : T_TJSONB;
  lfinfo.reserrcheck = true; /* Check for NULL result */
  return tfunc_temporal(temp, &lfinfo);
}

/*****************************************************************************/

/**
 * @ingroup meos_json_json
 * @brief Delete a key from a temporal JSONB value
 * @param[in] temp Temporal JSONB value
 * @param[in] idx Index
 */
Temporal *
tjsonb_delete_idx(const Temporal *temp, int idx)
{
  /* Ensure the validity of the arguments */
  VALIDATE_TJSONB(temp, NULL);

  LiftedFunctionInfo lfinfo;
  memset(&lfinfo, 0, sizeof(LiftedFunctionInfo));
  lfinfo.func = (varfunc) &datum_jsonb_delete_idx;
  lfinfo.argtype[0] = T_TJSONB;
  lfinfo.numparam = 1;
  lfinfo.param[0] = DatumGetInt32(idx);
  lfinfo.restype = T_TJSONB;
  lfinfo.reserrcheck = true; /* Check for NULL result */
  return tfunc_temporal(temp, &lfinfo);
}

/*****************************************************************************/

/**
 * @ingroup meos_json_json
 * @brief Delete a key from a temporal JSONB value
 * @param[in] temp Temporal JSONB value
 * @param[in] key Key
 */
Temporal *
tjsonb_delete_key(const Temporal *temp, const text *key)
{
  /* Ensure the validity of the arguments */
  VALIDATE_TJSONB(temp, NULL); VALIDATE_NOT_NULL(key, NULL);

  LiftedFunctionInfo lfinfo;
  memset(&lfinfo, 0, sizeof(LiftedFunctionInfo));
  lfinfo.func = (varfunc) &datum_jsonb_delete;
  lfinfo.argtype[0] = T_TJSONB;
  lfinfo.numparam = 1;
  lfinfo.param[0] = PointerGetDatum(key);
  lfinfo.restype = T_TJSONB;
  lfinfo.reserrcheck = true; /* Check for NULL result */
  return tfunc_temporal(temp, &lfinfo);
}

/**
 * @ingroup meos_json_json
 * @brief Delete an array of keys from a temporal JSONB value
 * @param[in] temp Temporal JSONB value
 * @param[in] keys Keys
 * @param[in] count Number of elements in the input array
 */
Temporal *
tjsonb_delete_key_array(const Temporal *temp, text **keys, int count)
{
  /* Ensure the validity of the arguments */
  VALIDATE_TJSONB(temp, NULL); VALIDATE_NOT_NULL(keys, NULL);
  if (! ensure_positive(count))
    return NULL;

  LiftedFunctionInfo lfinfo;
  memset(&lfinfo, 0, sizeof(LiftedFunctionInfo));
  lfinfo.func = (varfunc) &datum_jsonb_delete_array;
  lfinfo.argtype[0] = T_TJSONB;
  lfinfo.numparam = 2;
  lfinfo.param[0] = PointerGetDatum(keys);
  lfinfo.param[1] = Int32GetDatum(count);
  lfinfo.restype = T_TJSONB;
  lfinfo.reserrcheck = true; /* Check for NULL result */
  lfinfo.reserrcheck = true; /* Check for NULL result */
  return tfunc_temporal(temp, &lfinfo);
}

/*****************************************************************************/

/**
 * @ingroup meos_json_json
 * @brief Delete a key from a temporal JSONB value
 * @param[in] temp Temporal JSONB value
 * @param[in] key Key
 */
Temporal *
tjsonb_exists(const Temporal *temp, const text *key)
{
  /* Ensure the validity of the arguments */
  VALIDATE_TJSONB(temp, NULL); VALIDATE_NOT_NULL(key, NULL);

  LiftedFunctionInfo lfinfo;
  memset(&lfinfo, 0, sizeof(LiftedFunctionInfo));
  lfinfo.func = (varfunc) &datum_jsonb_exists;
  lfinfo.argtype[0] = T_TJSONB;
  lfinfo.numparam = 1;
  lfinfo.param[0] = PointerGetDatum(key);
  lfinfo.restype = T_TBOOL;
  return tfunc_temporal(temp, &lfinfo);
}

/**
 * @ingroup meos_json_json
 * @brief Delete an array of keys from a temporal JSONB value
 * @param[in] temp Temporal JSONB value
 * @param[in] keys Keys
 * @param[in] count Number of elements in the input array
 * @param[in] any True for the 'any' semantics, false for the 'all' semantics
 */
Temporal *
tjsonb_exists_array(const Temporal *temp, text **keys, int count, bool any)
{
  /* Ensure the validity of the arguments */
  VALIDATE_TJSONB(temp, NULL); VALIDATE_NOT_NULL(keys, NULL);
  if (! ensure_positive(count))
    return NULL;

  LiftedFunctionInfo lfinfo;
  memset(&lfinfo, 0, sizeof(LiftedFunctionInfo));
  lfinfo.func = (varfunc) &datum_jsonb_exists_array;
  lfinfo.argtype[0] = T_TJSONB;
  lfinfo.numparam = 3;
  lfinfo.param[0] = PointerGetDatum(keys);
  lfinfo.param[1] = Int32GetDatum(count);
  lfinfo.param[2] = BoolGetDatum(any);
  lfinfo.restype = T_TBOOL;
  return tfunc_temporal(temp, &lfinfo);
}



/**
 * @ingroup meos_json_json
 * @brief Replace a JSONB value specified by a path with a new value
 * @param[in] temp Temporal JSONB value
 * @param[in] keys Keys
 * @param[in] count Number of elements in the input array
 * @param[in] newjb New value
 * @param[in] create When true, if the last path step is an array index that
 * is out of range, the new value is added at the beginning of the array if
 * the index is negative, or at the end of the array if it is positive
 * @param[in] handle_null States the null_value_treatment which must be one of
 * 'raise_exception', 'use_json_null', 'delete_key', or 'return_target'
 * @param[in] lax True when the lax mode is used
 */
Temporal *
tjsonb_set(const Temporal *temp, text **keys, int count, Jsonb *newjb,
  bool create, const text *handle_null, bool lax)
{
  /* Ensure the validity of the arguments */
  VALIDATE_TJSONB(temp, NULL); VALIDATE_NOT_NULL(keys, NULL);
  if (! ensure_positive(count))
    return NULL;

  LiftedFunctionInfo lfinfo;
  memset(&lfinfo, 0, sizeof(LiftedFunctionInfo));
  lfinfo.func = lax ?
    (varfunc) &datum_jsonb_set_lax : (varfunc) &datum_jsonb_set;
  lfinfo.argtype[0] = T_TJSONB;
  lfinfo.numparam = lax ? 5 : 4;
  lfinfo.param[0] = PointerGetDatum(keys);
  lfinfo.param[1] = Int32GetDatum(count);
  lfinfo.param[2] = PointerGetDatum(newjb);
  lfinfo.param[3] = BoolGetDatum(create);
  if (lax)
    lfinfo.param[4] = PointerGetDatum(handle_null);
  lfinfo.restype = T_TJSONB;
  return tfunc_temporal(temp, &lfinfo);
}

/*****************************************************************************/

/**
 * @brief Convert a JSONB value into a base alphanumeric value
 * @param[in] jb JSONB value
 * @param[in] key Key to extract from the JSONB object
 * @param[in] temptype Temporal type
 * @note Supported JSONB types: boolean, numeric, string
 */
Datum
jsonb_to_alphanum(const Jsonb *jb, const char *key, meosType temptype)
{
  assert(jb); assert(key); assert(talphanum_type(temptype));

  /* Lookup key in the JSONB object */
  JsonbValue k, *v;
  k.type = jbvString;
  k.val.string.len = strlen(key);
  k.val.string.val = (char *) key;
  v = findJsonbValueFromContainer(&((Jsonb *) jb)->root, JB_FOBJECT, &k);
  if (v == NULL)
  {
    meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
      "JSON key \"%s\" not found", key);
    return (Datum) 0;
  }
  Datum val;
  switch (v->type)
  {
    case jbvBool:
      if (temptype == T_TBOOL)
        val = v->val.boolean ? true : false;
      else if (temptype == T_TINT)
        val = v->val.boolean ? 1 : 0;
      else if (temptype == T_TFLOAT)
        val = v->val.boolean ? 1.0 : 0.0;
      else /* temptype == T_TEXT */
        val = v->val.boolean ?
          PointerGetDatum(strdup("true")) : PointerGetDatum(strdup("false"));
      break;

    case jbvNumeric:
    {
      if (tnumber_type(temptype))
      {
        /* Convert Numeric to C string to double */
        char *cstr = pg_numeric_out(v->val.numeric);
        char *endptr;
        double dval = strtod(cstr, &endptr);
        pfree(cstr);
        if (endptr == cstr)
        {
          meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
            "Invalid numeric value for key \"%s\"", key);
          return (Datum) 0;
        }
        val = (temptype == T_TINT) ?
          Int32GetDatum((int) dval) : Float8GetDatum(dval);
      }
      else if (temptype == T_TTEXT)
      {
        char *cstr = pg_numeric_out(v->val.numeric);
        val = PointerGetDatum(cstring_to_text(cstr));
        pfree(cstr);
      }
      break;
    }

    case jbvString:
    {
      char *buf = palloc(v->val.string.len + 1);
      memcpy(buf, v->val.string.val, v->val.string.len);
      buf[v->val.string.len] = '\0';
      if (temptype == T_TBOOL)
      {
        val = BoolGetDatum(bool_in(buf));
      }
      else if (tnumber_type(temptype))
      {
        char *endptr;
        double dval = strtod(buf, &endptr);
        if (endptr == buf)
        {
          meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
            "Invalid numeric string for key \"%s\": %s", key, buf);
          return (Datum) 0;
        }
        val = (temptype == T_TINT) ?
          Int32GetDatum((int) dval) : Float8GetDatum(dval);
      }
      else /* temptype == T_TTEXT */
      {
        text *txt = cstring_to_text(buf);
        val = PointerGetDatum(txt);
      }
      pfree(buf);
      break;
    }
    default:
    {
      meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
        "Unsupported JSONB value type for key \"%s\"", key);
      return (Datum) 0;
    }
  }
  return val;
}

/**
 * @brief Convert a JSONB value into a base alphanumeric value
 * @param[in] jb JSONB value
 * @param[in] key Key to extract from the JSONB object
 * @param[in] temptype Temporal type
 * @note Supported JSONB types: boolean, numeric, string
 */
Datum
datum_jsonb_to_alphanum(Datum jb, Datum key, Datum temptype)
{
  return jsonb_to_alphanum(DatumGetJsonbP(jb),
    (const char *) DatumGetPointer(key), (meosType) DatumGetInt32(temptype));
}

/**
 * @ingroup meos_json_json
 * @brief Convert a temporal JSONB into a temporal alphanumeric by extracting
 * one key
 * @param[in] temp Temporal JSONB value
 * @param[in] key Key to extract from the JSONB object
 * @param[in] temptype Temporal type
 * @note Supported JSONB types: boolean, numeric, string
 */
Temporal *
tjsonb_to_talphanum(const Temporal *temp, const char *key, meosType temptype)
{
  /* Ensure the validity of the arguments */
  assert(temp); assert(key); assert(temp->temptype == T_TJSONB);
  assert(talphanum_type(temptype));

  LiftedFunctionInfo lfinfo;
  memset(&lfinfo, 0, sizeof(LiftedFunctionInfo));
  lfinfo.func = (varfunc) &datum_jsonb_to_alphanum;
  lfinfo.argtype[0] = T_JSONB;
  lfinfo.numparam = 2;
  lfinfo.param[0] = PointerGetDatum(key);
  lfinfo.param[1] = temptype;
  lfinfo.restype = temptype;
  return tfunc_temporal(temp, &lfinfo);
}

#if MEOS
/**
 * @brief Convert a temporal JSONB to a temporal boolean by extracting one key
 * @param[in] temp Temporal JSONB object
 * @param[in] key Key to extract
 * @csqlfn #Tjsonb_to_tbool()
 */
Temporal *
tjsonb_to_tbool(const Temporal *temp, const char *key)
{
  return tjsonb_to_talphanum(temp, key, T_TBOOL);
}

/**
 * @brief Convert a temporal JSONB to a temporal integer by extracting one key
 * @param[in] temp Temporal JSONB object
 * @param[in] key Key to extract
 * @csqlfn #Tjsonb_to_tint()
 */
Temporal *
tjsonb_to_tint(const Temporal *temp, const char *key)
{
  return tjsonb_to_talphanum(temp, key, T_TINT);
}

/**
 * @brief Convert a temporal JSONB to a temporal float by extracting one key
 * @param[in] temp Temporal JSONB object
 * @param[in] key Key to extract
 * @csqlfn #Tjsonb_to_tfloat()
 */
Temporal *
tjsonb_to_tfloat(const Temporal *temp, const char *key)
{
  return tjsonb_to_talphanum(temp, key, T_TFLOAT);
}

/**
 * @brief Convert a temporal JSONB to a temporal text by extracting one key
 * @param[in] temp Temporal JSONB object
 * @param[in] key Key to extract
 * @csqlfn #Tjsonb_to_ttext_key()
 */
Temporal *
tjsonb_to_ttext_key(const Temporal *temp, const char *key)
{
  return tjsonb_to_talphanum(temp, key, T_TTEXT);
}
#endif /* MEOS */

/*****************************************************************************/

/**
 * @ingroup meos_json_json
 * @brief Return a temporal JSON value without nulls
 * @param[in] temp Temporal JSON value
 * @param[in] strip_in_arrays True when nulls are removed from the arrays
 * @csqlfn #Tjson_strip_nulls()
 */
Temporal *
tjson_strip_nulls(const Temporal *temp, bool strip_in_arrays)
{
  /* Ensure the validity of the arguments */
  assert(temp); assert(temp->temptype == T_TTEXT);

  LiftedFunctionInfo lfinfo;
  memset(&lfinfo, 0, sizeof(LiftedFunctionInfo));
  lfinfo.func = (varfunc) &datum_json_strip_nulls;
  lfinfo.argtype[0] = T_TTEXT;
  lfinfo.numparam = 1;
  lfinfo.param[0] = BoolGetDatum(strip_in_arrays);
  lfinfo.restype = T_TTEXT;
  return tfunc_temporal(temp, &lfinfo);
}

/**
 * @ingroup meos_json_json
 * @brief Return a temporal JSONB value without nulls
 * @param[in] temp Temporal JSONB value
 * @param[in] strip_in_arrays True when nulls are removed from the arrays
 * @csqlfn #Tjsonb_strip_nulls()
 */
Temporal *
tjsonb_strip_nulls(const Temporal *temp, bool strip_in_arrays)
{
  /* Ensure the validity of the arguments */
  assert(temp); assert(temp->temptype == T_TJSONB);

  LiftedFunctionInfo lfinfo;
  memset(&lfinfo, 0, sizeof(LiftedFunctionInfo));
  lfinfo.func = (varfunc) &datum_jsonb_strip_nulls;
  lfinfo.argtype[0] = T_TJSONB;
  lfinfo.numparam = 1;
  lfinfo.param[0] = BoolGetDatum(strip_in_arrays);
  lfinfo.restype = T_TJSONB;
  return tfunc_temporal(temp, &lfinfo);
}

/*****************************************************************************/

/**
 * @ingroup meos_json_json
 * @brief Convert a temporal JSONB value to pretty-printed, indented text
 * @param[in] temp Temporal JSONB value
 * @csqlfn #Tjsonb_pretty()
 */
Temporal *
tjsonb_pretty(const Temporal *temp)
{
  /* Ensure the validity of the arguments */
  assert(temp); assert(temp->temptype == T_TJSONB);

  LiftedFunctionInfo lfinfo;
  memset(&lfinfo, 0, sizeof(LiftedFunctionInfo));
  lfinfo.func = (varfunc) &datum_jsonb_pretty;
  lfinfo.argtype[0] = T_TJSONB;
  lfinfo.restype = T_TTEXT;
  return tfunc_temporal(temp, &lfinfo);
}

/*****************************************************************************
* Path functions
 *****************************************************************************/

/**
 * @ingroup meos_json_json
 * @brief Delete a JSONB value specified by a path
 * @param[in] temp Temporal JSONB value
 * @param[in] path_elems Array of path elements
 * @param[in] path_len Number of elements in the input array
 */
Temporal *
tjsonb_delete_path(const Temporal *temp, text **path_elems, int path_len)
{
  /* Ensure the validity of the arguments */
  VALIDATE_TJSONB(temp, NULL); VALIDATE_NOT_NULL(path_elems, NULL);
  if (! ensure_positive(path_len))
    return NULL;

  LiftedFunctionInfo lfinfo;
  memset(&lfinfo, 0, sizeof(LiftedFunctionInfo));
  lfinfo.func = (varfunc) &datum_jsonb_delete_path;
  lfinfo.argtype[0] = T_TJSONB;
  lfinfo.numparam = 2;
  lfinfo.param[0] = PointerGetDatum(path_elems);
  lfinfo.param[1] = Int32GetDatum(path_len);
  lfinfo.restype = T_TJSONB;
  return tfunc_temporal(temp, &lfinfo);
}

/*****************************************************************************/

/**
 * @ingroup meos_json_json
 * @brief Extract a JSON value specified by a path
 * @param[in] temp Temporal JSON value
 * @param[in] path_elems Keys
 * @param[in] path_len Number of elements in the input array
 * @param[in] astext True when the output is a temporal text
 */
Temporal *
tjson_extract_path(const Temporal *temp, text **path_elems, int path_len,
  bool astext)
{
  /* Ensure the validity of the arguments */
  VALIDATE_TTEXT(temp, NULL); VALIDATE_NOT_NULL(path_elems, NULL);
  if (! ensure_positive(path_len))
    return NULL;

  LiftedFunctionInfo lfinfo;
  memset(&lfinfo, 0, sizeof(LiftedFunctionInfo));
  lfinfo.func = astext ? (varfunc) &datum_json_extract_path_text :
    (varfunc) &datum_json_extract_path;
  lfinfo.argtype[0] = T_TEXT;
  lfinfo.numparam = 2;
  lfinfo.param[0] = PointerGetDatum(path_elems);
  lfinfo.param[1] = Int32GetDatum(path_len);
  lfinfo.restype = T_TEXT;
  return tfunc_temporal(temp, &lfinfo);
}

/**
 * @ingroup meos_json_json
 * @brief Extract a JSONB value specified by a path
 * @param[in] temp Temporal JSONB value
 * @param[in] path_elems Keys
 * @param[in] path_len Number of elements in the input array
 * @param[in] astext True when the output is a temporal text
 */
Temporal *
tjsonb_extract_path(const Temporal *temp, text **path_elems, int path_len,
  bool astext)
{
  /* Ensure the validity of the arguments */
  VALIDATE_TJSONB(temp, NULL); VALIDATE_NOT_NULL(path_elems, NULL);
  if (! ensure_positive(path_len))
    return NULL;

  LiftedFunctionInfo lfinfo;
  memset(&lfinfo, 0, sizeof(LiftedFunctionInfo));
  lfinfo.func = astext ? (varfunc) &datum_jsonb_extract_path_text :
    (varfunc) &datum_jsonb_extract_path;
  lfinfo.argtype[0] = T_TJSONB;
  lfinfo.numparam = 2;
  lfinfo.param[0] = PointerGetDatum(path_elems);
  lfinfo.param[1] = Int32GetDatum(path_len);
  lfinfo.restype = astext ? T_TEXT : T_TJSONB;
  return tfunc_temporal(temp, &lfinfo);
}

/*****************************************************************************/

/**
 * @ingroup meos_json_json
 * @brief Replace a JSONB value specified by a path with a new value
 * @details If the item designated by the path is an array element, new_value
 * will be inserted before that item if insert_after is false (which is the
 * default), or after it if insert_after is true. If the item designated by
 * the path is an object field, new_value will be inserted only if the object
 * does not already contain that key. All earlier steps in the path must exist,
 * or the target is returned unchanged. As with the path oriented operators,
 * negative integers that appear in the path count from the end of JSON arrays.
 * If the last path step is an array index that is out of range, the new value
 * is added at the beginning of the array if the index is negative, or at the
 * end of the array if it is positive.
 * @param[in] temp Temporal JSONB value
 * @param[in] path_elems Array of path elements
 * @param[in] path_len Number of elements in the input array
 * @param[in] newjb New value
 * @param[in] after When true, if the last path step is an array index that
 * is out of range, the new value is added at the beginning of the array if
 * the index is negative, or at the end of the array if it is positive
 */
Temporal *
tjsonb_insert(const Temporal *temp, text **path_elems, int path_len,
  Jsonb *newjb, bool after)
{
  /* Ensure the validity of the arguments */
  VALIDATE_TJSONB(temp, NULL); VALIDATE_NOT_NULL(path_elems, NULL);
  if (! ensure_positive(path_len))
    return NULL;

  LiftedFunctionInfo lfinfo;
  memset(&lfinfo, 0, sizeof(LiftedFunctionInfo));
  lfinfo.func = (varfunc) &datum_jsonb_insert;
  lfinfo.argtype[0] = T_TJSONB;
  lfinfo.numparam = 4;
  lfinfo.param[0] = PointerGetDatum(path_elems);
  lfinfo.param[1] = Int32GetDatum(path_len);
  lfinfo.param[2] = PointerGetDatum(newjb);
  lfinfo.param[3] = BoolGetDatum(after);
  lfinfo.restype = T_TJSONB;
  return tfunc_temporal(temp, &lfinfo);
}

/*****************************************************************************/
