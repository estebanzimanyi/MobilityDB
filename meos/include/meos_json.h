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
 * @brief External API of the Mobility Engine Open Source (MEOS) library
 */

#ifndef __MEOS_JSON_H__
#define __MEOS_JSON_H__

/* C */
#include <stdbool.h>
#include <stdint.h>
/* JSON-C */
#include <json-c/json.h>
/* MEOS */
#include <meos.h>
#include <meos_internal.h>
#include <pgtypes.h>

/*****************************************************************************
 * Validity macros
 *****************************************************************************/

/**
 * @brief Macro for ensuring that the set passed as argument is a JSONB set
 */
#if MEOS
  #define VALIDATE_JSONBSET(set, ret) \
    do { \
          if (! ensure_not_null((void *) set) || \
              ! ensure_set_isof_type((set), T_JSONBSET) ) \
           return (ret); \
    } while (0)
#else
  #define VALIDATE_JSONBSET(set, ret) \
    do { \
      assert(set); \
      assert((set)->settype == T_JSONBSET); \
    } while (0)
#endif

/**
 * @brief Macro for ensuring that the temporal value passed as argument is a
 * temporal JSONB
 * @note The macro works for the Temporal type and its subtypes TInstant,
 * TSequence, and TSequenceSet
 */
#if MEOS
  #define VALIDATE_TJSONB(temp, ret) \
    do { \
          if (! ensure_not_null((void *) (temp)) || \
              ! ensure_temporal_isof_type((Temporal *) (temp), T_TJSONB) ) \
           return (ret); \
    } while (0)
#else
  #define VALIDATE_TJSONB(temp, ret) \
    do { \
      assert(temp); \
      assert(((Temporal *) (temp))->temptype == T_TJSONB); \
    } while (0)
#endif

/******************************************************************************
 * Functions for JSON
 ******************************************************************************/

// /* Input and output functions */

// extern text *json_in(const char *str);
// extern char *json_out(const text *json);
// extern Jsonb *jsonb_from_text(const text *txt, bool unique_keys);
// extern Jsonb *jsonb_in(const char *str);
// extern char *jsonb_out(const Jsonb *jb);

// /* Constructor functions */

// extern text *json_make(text **keyvalarr, int count);
// extern text *json_make_two_arg(text **keys, text **values, int count);
// extern Jsonb *jsonb_copy(const Jsonb *jb);
// extern Jsonb *jsonb_make(text **keys_vals, int count);
// extern Jsonb *jsonb_make_two_arg(text **keys, text **values, int count);

// /* Conversion functions */

// extern Jsonb *cstring_to_jsonb(const char *str);
// extern bool jsonb_to_bool(const Jsonb *jb);
// extern char *jsonb_to_cstring(const Jsonb *jb);
// extern float4 jsonb_to_float4(const Jsonb *jb);
// extern float8 jsonb_to_float8(const Jsonb *jb);
// extern int16 jsonb_to_int16(const Jsonb *jb);
// extern int32 jsonb_to_int32(const Jsonb *jb);
// extern int64 jsonb_to_int64(const Jsonb *jb);
// extern Numeric jsonb_to_numeric(const Jsonb *jb);

/* Accessor functions */

// extern text **json_array_elements(const text *json, int *count);
// extern text **json_array_elements_text(const text *json, int *count);
// extern int json_array_length(const text *json);
// extern text **json_each(const text *json, text **values, int *count);
// extern text **json_each_text(const text *json, text **values, int *count);
// extern text **json_object_keys(const text *json, int *count);
// extern text *json_typeof(const text *json);

// extern Jsonb **jsonb_array_elements(const Jsonb *jb, int *count);
// extern text **jsonb_array_elements_text(const Jsonb *jb, int *count);
// extern bool jsonb_contained(const Jsonb *jb1, const Jsonb *jb2); 
// extern bool jsonb_contains(const Jsonb *jb1, const Jsonb *jb2); 
// extern text **jsonb_each(const Jsonb *jb, Jsonb **values, int *count); 
// extern text **jsonb_each_text(const Jsonb *jb, text **values, int *count); 
// extern bool jsonb_exists(const Jsonb *jb, const text *key); 
// extern uint32 jsonb_hash(const Jsonb *jb);
// extern uint64 jsonb_hash_extended(const Jsonb *jb, uint64 seed);
// extern text **jsonb_object_keys(const Jsonb *jb, int *count);

/* Transformation functions */

// extern text *json_array_element(const text *json, int element);
// extern text *json_array_element_text(const text *json, int element);
// extern text *json_extract_path(const text *json, text **path_elems, int path_len);
// extern text *json_extract_path_text(const text *json, text **path_elems, int path_len);
// extern text *json_object_field(const text *json, const text *key);
// extern text *json_object_field_text(const text *json, const text *key);
// extern text *json_strip_nulls(const text *json, bool strip_in_arrays);
// extern Jsonb *jsonb_array_element(const Jsonb *jb, int element);
// extern text *jsonb_array_element_text(const Jsonb *jb, int element);
// extern Jsonb *jsonb_concat(const Jsonb *jb1, const Jsonb *jb2);
// extern Jsonb *jsonb_delete(const Jsonb *jb, const text *key);
// extern Jsonb *jsonb_delete_array(const Jsonb *jb, text **keys_elems, int keys_len);
// extern Jsonb *jsonb_delete_idx(const Jsonb *jb, int idx);
// extern Jsonb *jsonb_delete_path(const Jsonb *jb, text **path_elems, int path_len);
// extern Jsonb *jsonb_extract_path(const Jsonb *jb, text **path_elems, int path_len);
// extern text *jsonb_extract_path_text(const Jsonb *jb, text **path_elems, int path_len);
// extern Jsonb *jsonb_insert(const Jsonb *jb, text **path_elems, int path_len, Jsonb *newjb, bool after);
// extern Jsonb *jsonb_object_field(const Jsonb *jb, const text *key);
// extern text *jsonb_object_field_text(const Jsonb *jb, const text *key);
// extern text *jsonb_pretty(const Jsonb *jb);
// extern Jsonb *jsonb_set(const Jsonb *jb, text **path_elems, int path_len, Jsonb *newjb, bool create);
// extern Jsonb *jsonb_set_lax(const Jsonb *jb, text **path_elems, int path_len, Jsonb *newjb, bool create, const text *handle_null);
// extern Jsonb *jsonb_strip_nulls (const Jsonb *jb, bool strip_in_arrays);

/* Bounding box functions */

/* Comparison functions */

// extern int jsonb_cmp(const Jsonb *jb1, const Jsonb *jb2);
// extern bool jsonb_eq(const Jsonb *jb1, const Jsonb *jb2);
// extern bool jsonb_le(const Jsonb *jb1, const Jsonb *jb2);
// extern bool jsonb_lt(const Jsonb *jb1, const Jsonb *jb2);
// extern bool jsonb_ne(const Jsonb *jb1, const Jsonb *jb2);
// extern bool jsonb_ge(const Jsonb *jb1, const Jsonb *jb2);
// extern bool jsonb_gt(const Jsonb *jb1, const Jsonb *jb2);

/******************************************************************************
 * Functions for JSONB sets
 ******************************************************************************/

/* Input and output functions */

extern Set *jsonbset_in(const char *str);
extern char *jsonbset_out(const Set *s, int maxdd);

/* Constructor functions */

extern Set *jsonbset_make(const Jsonb **values, int count);

/* Conversion functions */

extern Set *jsonb_to_set(const Jsonb *jb);

/* Accessor functions */

extern Jsonb *jsonbset_end_value(const Set *s);
extern Jsonb *jsonbset_start_value(const Set *s);
extern bool jsonbset_value_n(const Set *s, int n, Jsonb **result);
extern Jsonb **jsonbset_values(const Set *s);

/* Set operations */

extern bool contained_jsonb_set(const Jsonb *jb, const Set *s);
extern bool contains_set_jsonb(const Set *s, Jsonb *jb);
extern Set *intersection_jsonb_set(const Jsonb *jb, const Set *s);
extern Set *intersection_set_jsonb(const Set *s, const Jsonb *jb);
extern Set *minus_jsonb_set(const Jsonb *jb, const Set *s);
extern Set *minus_set_jsonb(const Set *s, const Jsonb *jb);
extern Set *jb_union_transfn(Set *state, const Jsonb *jb);
extern Set *union_jsonb_set(const Jsonb *jb, const Set *s);
extern Set *union_set_jsonb(const Set *s, const Jsonb *jb);

/*===========================================================================*
 * Functions for temporal JSONB
 *===========================================================================*/

/*****************************************************************************
 * Input/output functions
 *****************************************************************************/

extern Temporal *tjson_in(const char *str); // <---
extern char *tjson_out(const text *json); // <---
extern Temporal *tjsonb_from_text(const text *txt, bool unique_keys); // <---


extern Temporal *tjsonb_from_mfjson(const char *str);
extern Temporal *tjsonb_in(const char *str);
extern char *tjsonb_out(const Temporal *temp);

// Internal
extern TInstant *tjsonbinst_from_mfjson(const json_object *mfjson);
extern TInstant *tjsonbinst_in(const char *str);
extern TSequence *tjsonbseq_from_mfjson(const json_object *mfjson);
extern TSequence *tjsonbseq_in(const char *str, interpType interp);
extern TSequenceSet *tjsonbseqset_from_mfjson(const json_object *mfjson);
extern TSequenceSet *tjsonbseqset_in(const char *str);

/*****************************************************************************
 * Constructor functions
 *****************************************************************************/

extern Temporal *tjsonb_from_base_temp(const Jsonb *jsonb, const Temporal *temp);
extern TInstant *tjsonbinst_make(const Jsonb *jsonb, TimestampTz t);
extern TSequence *tjsonbseq_from_base_tstzset(const Jsonb *jsonb, const Set *s);
extern TSequence *tjsonbseq_from_base_tstzspan(const Jsonb *jsonb, const Span *s);
extern TSequenceSet *tjsonbseqset_from_base_tstzspanset(const Jsonb *jsonb, const SpanSet *ss);

extern text *tjson_make(text **keyvalarr, int count); // <--- // <---
extern text *tjson_make_two_arg(text **keys, text **values, int count); // <---
extern Jsonb *tjsonb_copy(const Jsonb *jb); // <---
extern Jsonb *tjsonb_make(text **keys_vals, int count); // <---
extern Jsonb *tjsonb_make_two_arg(text **keys, text **values, int count); // <---

/*****************************************************************************
 * Conversion functions
 *****************************************************************************/

extern Temporal *tjsonb_to_ttext(const Temporal *temp);
extern Temporal *ttext_to_tjsonb(const Temporal *temp);

/*****************************************************************************
 * Accessor functions
 *****************************************************************************/

extern Jsonb *tjsonb_end_value(const Temporal *temp);
extern Jsonb *tjsonb_start_value(const Temporal *temp);
extern bool tjsonb_value_at_timestamptz(const Temporal *temp, TimestampTz t, bool strict,  Jsonb **value);
extern bool tjsonb_value_n(const Temporal *temp,   int n, Jsonb **result);
extern Jsonb **tjsonb_values(const Temporal *temp,  int *count);

extern Temporal **tjson_array_elements(const Temporal *temp, int *count); // <---
extern Temporal **tjson_array_elements_text(const Temporal *temp, int *count); // <---
extern Temporal *tjson_array_length(const Temporal *temp); // <---
extern Temporal **tjson_each(const Temporal *temp, text **values, int *count); // <---
extern Temporal **tjson_each_text(const Temporal *temp, text **values, int *count); // <---
extern Temporal **tjson_object_keys(const Temporal *temp, int *count); // <---
extern Temporal *tjson_typeof(const Temporal *temp); // <---

extern Temporal **tjsonb_array_elements(const Temporal *temp, int *count); // <---
extern Temporal **tjsonb_array_elements_text(const Temporal *temp, int *count); // <---
extern Temporal *tjsonb_contained(const Temporal *temp1, const Temporal *temp2); // <--- 
extern Temporal *tjsonb_contains(const Temporal *temp1, const Temporal *temp2); // <--- 
extern Temporal **tjsonb_each(const Temporal *temp, Jsonb **values, int *count); // <--- 
extern Temporal **tjsonb_each_text(const Temporal *temp, text **values, int *count); // <--- 
extern Temporal *tjsonb_exists(const Temporal *temp, const text *key); // <--- 
extern Temporal **tjsonb_object_keys(const Temporal *temp, int *count); // <---

/*****************************************************************************
 * Transformation functions
 *****************************************************************************/

extern Temporal *tjson_array_element(const Temporal *temp, int element, bool astext);
extern Temporal *tjson_extract_path(const Temporal *temp, text **path_elems, int path_len, bool astext);
extern Temporal *tjson_object_field(const Temporal *temp, const text *key, bool astext);
extern Temporal *tjson_strip_nulls(const Temporal *temp, bool strip_in_arrays);

extern Temporal *tjsonb_array_element(const Temporal *temp, int element, bool astext);
extern Temporal *tjsonb_extract_path(const Temporal *temp, text **path_elems, int path_len, bool astext);
extern Temporal *tjsonb_object_field(const Temporal *temp, const text *key, bool astext);
extern Temporal *tjsonb_pretty(const Temporal *temp);
extern Temporal *tjsonb_set(const Temporal *temp, text **path_elems, int path_len, Jsonb *newjb, bool create, const text *handle_null, bool lax);
extern Temporal *tjsonb_strip_nulls (const Temporal *temp, bool strip_in_arrays);

/*****************************************************************************
 * Aggregate functions
 *****************************************************************************/


/*****************************************************************************
 * Temporal JSON functions
 *****************************************************************************/

extern Temporal *tjsonb_object_field(const Temporal *temp, const text *key, bool astext);
extern Temporal *concat_tjsonb_jsonb(const Temporal *temp, const Jsonb *jb, bool invert);
extern Temporal *concat_tjsonb_tjsonb(const Temporal *temp1, const Temporal *temp2);
extern Temporal *contains_tjsonb_jsonb(const Temporal *temp, const Jsonb *jb, bool invert);
extern Temporal *contains_tjsonb_tjsonb(const Temporal *temp1, const Temporal *temp2);
extern Temporal *tjsonb_to_tbool(const Temporal *temp, const char *key);
extern Temporal *tjsonb_to_tint(const Temporal *temp, const char *key);
extern Temporal *tjsonb_to_tfloat(const Temporal *temp, const char *key, interpType interp);
extern Temporal *tjsonb_to_ttext_key(const Temporal *temp, const char *key);
extern Temporal *tjsonb_delete_idx(const Temporal *temp, int idx);
extern Temporal *tjsonb_delete_key(const Temporal *temp, const text *key);
extern Temporal *tjsonb_delete_key_array(const Temporal *temp, text **keys, int count);
extern Temporal *tjsonb_exists(const Temporal *temp, const text *key);
extern Temporal *tjsonb_exists_array(const Temporal *temp, text **keys, int count, bool any);
extern Temporal *tjsonb_set(const Temporal *temp, text **keys, int count, Jsonb *newjb, bool create, const text *handle_null, bool lax);
extern Temporal *tjsonb_insert(const Temporal *temp, text **keys, int count, Jsonb *newjb, bool after);
extern Temporal *tjsonb_delete_path(const Temporal *temp, text **path_elems, int path_len);
extern Temporal *tjsonb_extract_path(const Temporal *temp, text **path_elems, int path_len, bool astext);

/*****************************************************************************
 * Restriction functions
 *****************************************************************************/

extern Temporal *tjsonb_at_value(const Temporal *temp, Jsonb *jsb);
extern Temporal *tjsonb_minus_value(const Temporal *temp,  Jsonb *jsb);

/*****************************************************************************
 * Comparison functions
 *****************************************************************************/

/* Ever/always and temporal comparison functions */

extern int always_eq_jsonb_tjsonb(const Jsonb *jb, const Temporal *temp);
extern int always_eq_tjsonb_jsonb(const Temporal *temp, const Jsonb *jb);
extern int always_eq_tjsonb_tjsonb(const Temporal *temp1, const Temporal *temp2);
extern int always_ne_jsonb_tjsonb(const Jsonb *jb, const Temporal *temp);
extern int always_ne_tjsonb_jsonb(const Temporal *temp, const Jsonb *jb);
extern int always_ne_tjsonb_tjsonb(const Temporal *temp1, const Temporal *temp2);
extern int ever_eq_jsonb_tjsonb(const Jsonb *jb, const Temporal *temp);
extern int ever_eq_tjsonb_jsonb(const Temporal *temp, const Jsonb *jb);
extern int ever_eq_tjsonb_tjsonb(const Temporal *temp1, const Temporal *temp2);
extern int ever_ne_jsonb_tjsonb(const Jsonb *jb, const Temporal *temp);
extern int ever_ne_tjsonb_jsonb(const Temporal *temp, const Jsonb *jb);
extern int ever_ne_tjsonb_tjsonb(const Temporal *temp1, const Temporal *temp2);

/*****************************************************************************/

extern Temporal *teq_jsonb_tjsonb(const Jsonb *jb, const Temporal *temp);
extern Temporal *teq_tjsonb_jsonb(const Temporal *temp, const Jsonb *jb);
extern Temporal *tne_jsonb_tjsonb(const Jsonb *jb, const Temporal *temp);
extern Temporal *tne_tjsonb_jsonb(const Temporal *temp, const Jsonb *jb);

/*****************************************************************************/

#endif /* __MEOS_JSON_H__ */
