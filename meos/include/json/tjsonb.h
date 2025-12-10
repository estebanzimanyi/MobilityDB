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

#ifndef __TJSONB_FUNCS_H__
#define __TJSONB_FUNCS_H__

/* PostgreSQL */
#include <postgres.h>
#include <utils/jsonb.h>
/* MEOS */
#include <meos.h>
#include "temporal/temporal.h"

/* Operations available for setPath */
#define JB_PATH_CREATE               0x0001
#define JB_PATH_DELETE               0x0002
#define JB_PATH_REPLACE              0x0004
#define JB_PATH_INSERT_BEFORE        0x0008
#define JB_PATH_INSERT_AFTER         0x0010
#define JB_PATH_FILL_GAPS            0x0020
#define JB_PATH_CONSISTENT_POSITION  0x0040
#define JB_PATH_CREATE_OR_INSERT \
  (JB_PATH_INSERT_BEFORE | JB_PATH_INSERT_AFTER | JB_PATH_CREATE)

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

/*****************************************************************************
 * JSONB internal operations
 *****************************************************************************/

extern JsonbValue *setPath(JsonbIterator **it, Datum *path_elems,
  bool *path_nulls, int path_len, JsonbParseState **st, int level,
  JsonbValue *newval, int op_type);
extern void setPathObject(JsonbIterator **it, Datum *path_elems,
  bool *path_nulls, int path_len, JsonbParseState **st, int level,
  JsonbValue *newval, uint32 npairs, int op_type);
extern void setPathArray(JsonbIterator **it, Datum *path_elems,
  bool *path_nulls, int path_len, JsonbParseState **st, int level,
  JsonbValue *newval, uint32 nelems, int op_type);

/*****************************************************************************
 * Datum‐level JSONB operations
 * Only those used in several files
 *****************************************************************************/

extern Datum datum_jsonb_concat(Datum l, Datum r);
extern Datum datum_jsonb_delete(Datum jb, Datum key);
extern Datum datum_jsonb_to_text(Datum jb);
extern Datum datum_text_to_jsonb(Datum txt);

/*****************************************************************************
 * Temporal JSONB operations
 *****************************************************************************/

extern Temporal *tjsonb_object_field(const Temporal *temp, const text *key, bool astext);
extern Temporal *concat_tjsonb_jsonb(const Temporal *temp, const Jsonb *jb, bool invert);
extern Temporal *concat_tjsonb_tjsonb(const Temporal *temp1, const Temporal *temp2);
extern Temporal *contains_tjsonb_jsonb(const Temporal *temp, const Jsonb *jb, bool invert);
extern Temporal *contains_tjsonb_tjsonb(const Temporal *temp1, const Temporal *temp2);
extern Temporal *tjsonb_to_tbool(const Temporal *temp, const char *key);
extern Temporal *tjsonb_to_tint(const Temporal *temp, const char *key);
extern Temporal *tjsonb_to_tfloat(const Temporal *temp, const char *key);
extern Temporal *tjsonb_to_ttext_key(const Temporal *temp, const char *key);
extern Temporal *tjsonb_delete_idx(const Temporal *temp, int idx);
extern Temporal *tjsonb_delete_key(const Temporal *temp, const text *key);
extern Temporal *tjsonb_delete_key_array(const Temporal *temp, text **keys, int count);
extern Temporal *tjsonb_exists(const Temporal *temp, const text *key);
extern Temporal *tjsonb_exists_array(const Temporal *temp, text **keys, int count, bool any);
extern Temporal *tjsonb_set(const Temporal *temp, text **keys, int count, Jsonb *newjb, bool create, const text *handle_null, bool lax);
extern Temporal *tjsonb_insert(const Temporal *temp, text **keys, int count, Jsonb *newjb, bool after);
extern Temporal *tjsonb_delete_path(const Temporal *temp, text **path_elems, int path_len);
extern Temporal *tjsonb_to_ttext(const Temporal *temp);
extern Temporal *ttext_to_tjsonb(const Temporal *temp);
extern Temporal *tjsonb_extract_path(const Temporal *temp, text **path_elems, int path_len, bool astext);

// Internal
extern Temporal *tjsonb_to_talphanum(const Temporal *temp, const char *key, meosType temptype);

/*****************************************************************************
 * Set wrappers for JSONB operations
 *****************************************************************************/

extern Set *jsonbfunc_jsonbset(const Set *s, datum_func1 func, meosType intype,
  meosType restype);
extern Set *jsonbfunc_jsonbset_jsonb(const Set *s, const Jsonb *jb,
  datum_func2 func, bool invert);
extern Set *jsonbfunc_jsonbset_text(const Set *s, const text *txt,
  datum_func2 func);

/*****************************************************************************
 * Temporal wrappers for JSONB operations
 *****************************************************************************/

extern bool jsonb_exists_array(const Jsonb *jb, text **keys_elems,
  int keys_len, bool any);

/*****************************************************************************/

#endif /* __TJSONB_FUNCS_H__ */
