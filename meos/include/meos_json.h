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
// #include <pg_json.h>

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
