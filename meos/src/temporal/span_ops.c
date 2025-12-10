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
 * @brief Operators for span types
 */

/* C */
#include <assert.h>
#include <float.h>
#include <math.h>
/* PostgreSQL */
#include <postgres.h>
#include <utils/timestamp.h>
/* MEOS */
#include <meos.h>
#include <meos_internal.h>
#include "temporal/span.h"
#include "temporal/temporal.h"
#include "temporal/type_util.h"

/*****************************************************************************
 * Generic functions
 *****************************************************************************/

/**
 * @brief Return the minimum value of two span base values
 */
Datum
span_min_value(Datum l, Datum r, meosType type)
{
  assert(span_basetype(type));
  switch (type)
  {
    case T_TIMESTAMPTZ:
      return TimestampTzGetDatum(Min(DatumGetTimestampTz(l),
        DatumGetTimestampTz(r)));
    case T_DATE:
      return DateADTGetDatum(Min(DatumGetDateADT(l), DatumGetDateADT(r)));
    case T_INT4:
      return Int32GetDatum(Min(DatumGetInt32(l), DatumGetInt32(r)));
    case T_INT8:
      return Int64GetDatum(Min(DatumGetInt64(l), DatumGetInt64(r)));
    case T_FLOAT8:
      return Float8GetDatum(Min(DatumGetFloat8(l), DatumGetFloat8(r)));
    default: /* Error! */
      meos_error(ERROR, MEOS_ERR_INTERNAL_TYPE_ERROR,
        "Unknown base type for minimum value function: %s",
        meostype_name(type));
    return 0;
  }
}

/**
 * @brief Return the maximum value of two span base values
 */
Datum
span_max_value(Datum l, Datum r, meosType type)
{
  assert(span_basetype(type));
  switch (type)
  {
    case T_TIMESTAMPTZ:
      return TimestampTzGetDatum(Max(DatumGetTimestampTz(l),
        DatumGetTimestampTz(r)));
    case T_DATE:
      return DateADTGetDatum(Max(DatumGetDateADT(l), DatumGetDateADT(r)));
    case T_INT4:
      return Int32GetDatum(Max(DatumGetInt32(l), DatumGetInt32(r)));
    case T_INT8:
      return Int64GetDatum(Max(DatumGetInt64(l), DatumGetInt64(r)));
    case T_FLOAT8:
      return Float8GetDatum(Max(DatumGetFloat8(l), DatumGetFloat8(r)));
    default: /* Error! */
      meos_error(ERROR, MEOS_ERR_INTERNAL_TYPE_ERROR,
        "Unknown base type for maximum value function: %s",
        meostype_name(type));
    return 0;
  }
}

/*****************************************************************************
 * Contains
 *****************************************************************************/

/**
 * @ingroup meos_internal_setspan_topo
 * @brief Return true if a span contains a value
 * @param[in] sp Span
 * @param[in] value Value
 */
bool
contains_span_value(const Span *sp, Datum value)
{
  assert(sp);
  int cmp = datum_cmp(sp->lower, value, sp->basetype);
  if (cmp > 0 || (cmp == 0 && ! sp->lower_inc))
    return false;

  cmp = datum_cmp(sp->upper, value, sp->basetype);
  if (cmp < 0 || (cmp == 0 && ! sp->upper_inc))
    return false;

  return true;
}

/**
 * @ingroup meos_setspan_topo
 * @brief Return true if a span contains a timestamptz
 * @param[in] sp Span
 * @param[in] t Value
 * @csqlfn #Contains_span_value()
 */
bool
contains_span_timestamptz(const Span *sp, TimestampTz t)
{
  /* Ensure the validity of the arguments */
  VALIDATE_TSTZSPAN(sp, false); 
  return contains_span_value(sp, TimestampTzGetDatum(t));
}

/**
 * @ingroup meos_setspan_topo
 * @brief Return true if the first span contains the second one
 * @param[in] sp1,sp2 Spans
 * @csqlfn #Contains_span_span()
 */
bool
contains_span_span(const Span *sp1, const Span *sp2)
{
  /* Ensure the validity of the arguments */
  if (! ensure_valid_span_span(sp1, sp2))
    return false;

  int cmp1 = datum_cmp(sp1->lower, sp2->lower, sp1->basetype);
  int cmp2 = datum_cmp(sp1->upper, sp2->upper, sp1->basetype);
  if (
    (cmp1 < 0 || (cmp1 == 0 && (sp1->lower_inc || ! sp2->lower_inc))) &&
    (cmp2 > 0 || (cmp2 == 0 && (sp1->upper_inc || ! sp2->upper_inc))) )
    return true;
  return false;}

/*****************************************************************************
 * Contained
 *****************************************************************************/

/**
 * @ingroup meos_internal_setspan_topo
 * @brief Return true if a value is contained in a span
 * @param[in] value Value
 * @param[in] sp Span
 */
inline bool
contained_value_span(Datum value, const Span *sp)
{
  return contains_span_value(sp, value);
}

/**
 * @ingroup meos_setspan_topo
 * @brief Return true if the first span is contained in the second one
 * @param[in] sp1,sp2 Spans
 * @csqlfn #Contained_value_span()
 */
inline bool
contained_span_span(const Span *sp1, const Span *sp2)
{
  return contains_span_span(sp2, sp1);
}

/*****************************************************************************
 * Overlaps
 *****************************************************************************/

/**
 * @ingroup meos_setspan_topo
 * @brief Return true if two spans overlap
 * @param[in] sp1,sp2 Spans
 * @csqlfn #Overlaps_span_span()
 */
bool
overlaps_span_span(const Span *sp1, const Span *sp2)
{
  /* Ensure the validity of the arguments */
  if (! ensure_valid_span_span(sp1, sp2))
    return false;

  int cmp1 = datum_cmp(sp1->lower, sp2->upper, sp1->basetype);
  int cmp2 = datum_cmp(sp2->lower, sp1->upper, sp1->basetype);
  if (
    (cmp1 < 0 || (cmp1 == 0 && sp1->lower_inc && sp2->upper_inc)) &&
    (cmp2 < 0 || (cmp2 == 0 && sp2->lower_inc && sp1->upper_inc)) )
    return true;
  return false;
}

/**
 * @ingroup meos_internal_setspan_topo
 * @brief Return true if two spans overlap or are adjacent
 * @param[in] sp1,sp2 Spans
 * @note This function is used for avoiding normalization in span operations
 */
bool
ovadj_span_span(const Span *sp1, const Span *sp2)
{
  assert(sp1); assert(sp2); assert(sp1->spantype == sp2->spantype);
  int cmp1 = datum_cmp(sp1->lower, sp2->upper, sp1->basetype);
  int cmp2 = datum_cmp(sp2->lower, sp1->upper, sp1->basetype);
  if (
    (cmp1 < 0 || (cmp1 == 0 && (sp1->lower_inc || sp2->upper_inc))) &&
    (cmp2 < 0 || (cmp2 == 0 && (sp2->lower_inc || sp1->upper_inc))) )
    return true;
  return false;
}

/*****************************************************************************
 * Adjacent to (but not overlapping)
 *****************************************************************************/

/**
 * @ingroup meos_internal_setspan_topo
 * @brief Return true if a span and a value are adjacent
 * @param[in] sp Span
 * @param[in] value Value
 * @csqlfn #Adjacent_span_value()
 */
bool
adjacent_span_value(const Span *sp, Datum value)
{
  assert(sp);
  Span sp1;
  span_set(value, value, true, true, sp->basetype, sp->spantype, &sp1);
  return adjacent_span_span(sp, &sp1);
}

/**
 * @ingroup meos_setspan_topo
 * @brief Return true if two spans are adjacent
 * @param[in] sp1,sp2 Spans
 * @csqlfn #Adjacent_span_span()
 */
bool
adjacent_span_span(const Span *sp1, const Span *sp2)
{
  /* Ensure the validity of the arguments */
  if (! ensure_valid_span_span(sp1, sp2))
    return false;

  /*
   * Two spans A..B and C..D are adjacent if and only if
   * B is adjacent to C, or D is adjacent to A.
   */
  return (
    (datum_eq(sp1->upper, sp2->lower, sp1->basetype) &&
      sp1->upper_inc != sp2->lower_inc) ||
    (datum_eq(sp2->upper, sp1->lower, sp1->basetype) &&
      sp2->upper_inc != sp1->lower_inc) );
}

/*****************************************************************************
 * Left of
 *****************************************************************************/

/**
 * @ingroup meos_internal_setspan_pos
 * @brief Return true if a value is to the left of a span
 * @param[in] value Value
 * @param[in] sp Span
 */
bool
left_value_span(Datum value, const Span *sp)
{
  assert(sp);
  int cmp = datum_cmp(value, sp->lower, sp->basetype);
  return (cmp < 0 || (cmp == 0 && ! sp->lower_inc));
}

/**
 * @ingroup meos_internal_setspan_pos
 * @brief Return true if a span is to the left of a value
 * @param[in] sp Span
 * @param[in] value Value
 */
bool
left_span_value(const Span *sp, Datum value)
{
  assert(sp);
  int cmp = datum_cmp(sp->upper, value, sp->basetype);
  return (cmp < 0 || (cmp == 0 && ! sp->upper_inc));
}

/**
 * @ingroup meos_setspan_pos
 * @brief Return true if the first span is to the left of the second one
 * @param[in] sp1,sp2 Spans
 * @csqlfn #Left_span_span()
 */
bool
left_span_span(const Span *sp1, const Span *sp2)
{
  /* Ensure the validity of the arguments */
  if (! ensure_valid_span_span(sp1, sp2))
    return false;

  int cmp = datum_cmp(sp1->upper, sp2->lower, sp1->basetype);
  return (cmp < 0 || (cmp == 0 && (! sp1->upper_inc || ! sp2->lower_inc)));
}

/**
 * @ingroup meos_internal_setspan_pos
 * @brief Return true if the first span is to the left and not adjacent to the
 * second one
 * @param[in] sp1,sp2 Spans
 * @note This function is used for avoiding normalization in span operations
 */
bool
lfnadj_span_span(const Span *sp1, const Span *sp2)
{
  assert(sp1); assert(sp2); assert(sp1->spantype == sp2->spantype);
  int cmp = datum_cmp(sp1->upper, sp2->lower, sp1->basetype);
  return (cmp < 0 || (cmp == 0 && ! sp1->upper_inc && ! sp2->lower_inc));
}

/*****************************************************************************
 * Right of
 *****************************************************************************/

/**
 * @ingroup meos_internal_setspan_pos
 * @brief Return true if a value is to the right of a span
 * @param[in] value Value
 * @param[in] sp Span
 */
inline bool
right_value_span(Datum value, const Span *sp)
{
  return left_span_value(sp, value);
}

/**
 * @ingroup meos_internal_setspan_pos
 * @brief Return true if a span is to the right of a value
 * @param[in] sp Span
 * @param[in] value Value
 */
inline bool
right_span_value(const Span *sp, Datum value)
{
  return left_value_span(value, sp);
}

/**
 * @ingroup meos_setspan_pos
 * @brief Return true if the first span is to right the of the second one
 * @param[in] sp1,sp2 Spans
 * @csqlfn #Right_span_span()
 */
inline bool
right_span_span(const Span *sp1, const Span *sp2)
{
  return left_span_span(sp2, sp1);
}

/*****************************************************************************
 * Does not extend to right of
 *****************************************************************************/

/**
 * @ingroup meos_internal_setspan_pos
 * @brief Return true if a value does not extend to the right of a span
 * @param[in] value Value
 * @param[in] sp Span
 */
bool
overleft_value_span(Datum value, const Span *sp)
{
  assert(sp);
  int cmp = datum_cmp(value, sp->upper, sp->basetype);
  return (cmp < 0 || (cmp == 0 && sp->upper_inc));
}

/**
 * @ingroup meos_internal_setspan_pos
 * @brief Return true if a span does not extend to the right of a value
 * @param[in] sp Span
 * @param[in] value Value
 */
bool
overleft_span_value(const Span *sp, Datum value)
{
  assert(sp);
  /* Integer spans are canonicalized and thus their upper bound is exclusive.
   * Therefore, we cannot simply check that sp->upper <= value */
  Span sp1;
  span_set(value, value, true, true, sp->basetype, sp->spantype, &sp1);
  return overleft_span_span(sp, &sp1);
}

/**
 * @ingroup meos_setspan_pos
 * @brief Return true if the first span does not extend to the right of the
 * second one
 * @param[in] sp1,sp2 Spans
 * @csqlfn #Overleft_span_span()
 */
bool
overleft_span_span(const Span *sp1, const Span *sp2)
{
  /* Ensure the validity of the arguments */
  if (! ensure_valid_span_span(sp1, sp2))
    return false;

  int cmp = datum_cmp(sp1->upper, sp2->upper, sp1->basetype);
  return (cmp < 0 || (cmp == 0 && (! sp1->upper_inc || sp2->upper_inc)));
}

/*****************************************************************************
 * Does not extend to left of
 *****************************************************************************/

/**
 * @ingroup meos_internal_setspan_pos
 * @brief Return true if a value does not extend to the left of a span
 * @param[in] value Value
 * @param[in] sp Span
 */
bool
overright_value_span(Datum value, const Span *sp)
{
  assert(sp);
  int cmp = datum_cmp(sp->lower, value, sp->basetype);
  return (cmp < 0 || (cmp == 0 && sp->lower_inc));
}

/**
 * @ingroup meos_internal_setspan_pos
 * @brief Return true if a span does not extend to the left of a value
 * @param[in] sp Span
 * @param[in] value Value
 */
bool
overright_span_value(const Span *sp, Datum value)
{
  assert(sp);
  return datum_le(value, sp->lower, sp->basetype);
}

/**
 * @ingroup meos_setspan_pos
 * @brief Return true if the first span does not extend to the left of the
 * second one
 * @param[in] sp1,sp2 Spans
 * @csqlfn #Overright_span_span()
 */
bool
overright_span_span(const Span *sp1, const Span *sp2)
{
  /* Ensure the validity of the arguments */
  if (! ensure_valid_span_span(sp1, sp2))
    return false;

  int cmp = datum_cmp(sp2->lower, sp1->lower, sp1->basetype);
  return (cmp < 0 || (cmp == 0 && (! sp1->lower_inc || sp2->lower_inc)));
}

/*****************************************************************************
 * Set union
 *****************************************************************************/

/**
 * @ingroup meos_internal_setspan_set
 * @brief Return in the last argument the bounding box union of two spans
 * @param[in] sp1,sp2 Spans
 * @param[out] result Resulting span
 * @note The result of the function is always a span even if the spans do not
 * overlap
 */
void
bbox_union_span_span(const Span *sp1, const Span *sp2, Span *result)
{
  assert(sp1); assert(sp2); assert(sp1->spantype == sp2->spantype);
  memcpy(result, sp1, sizeof(Span));
  span_expand(sp2, result);
  return;
}

/**
 * @ingroup meos_internal_setspan_set
 * @brief Return the bounding union of two spans
 * @param[in] sp1,sp2 Spans
 * @note The result of the function is always a span even if the spans do not
 * overlap
 * @note This function is similar to #bbox_union_span_span **with** memory
 * allocation
 */
Span *
super_union_span_span(const Span *sp1, const Span *sp2)
{
  assert(sp1); assert(sp2); assert(sp1->spantype == sp2->spantype);
  Span *result = span_copy(sp1);
  span_expand(sp2, result);
  return result;
}

/**
 * @ingroup meos_internal_setspan_set
 * @brief Return the union of a span and a value
 * @param[in] sp Span
 * @param[in] value Value
 * @csqlfn #Union_span_value()
 */
SpanSet *
union_span_value(const Span *sp, Datum value)
{
  assert(sp);
  Span sp1;
  span_set(value, value, true, true, sp->basetype, sp->spantype, &sp1);
  return union_span_span(sp, &sp1);
}

/**
 * @ingroup meos_internal_setspan_set
 * @brief Return the union of a value and a span
 * @param[in] sp Span
 * @param[in] value Value
 */
SpanSet *
union_value_span(Datum value, const Span *sp)
{
  return union_span_value(sp, value);
}

/**
 * @ingroup meos_setspan_set
 * @brief Return the union of two spans
 * @param[in] sp1,sp2 Spans
 * @csqlfn #Union_span_span()
 */
SpanSet *
union_span_span(const Span *sp1, const Span *sp2)
{
  /* Ensure the validity of the arguments */
  if (! ensure_valid_span_span(sp1, sp2))
    return NULL;

  /* If the spans overlap */
  if (ovadj_span_span(sp1, sp2))
  {
    /* Compute the union of the overlapping spans */
    Span sp;
    memcpy(&sp, sp1, sizeof(Span));
    span_expand(sp2, &sp);
    return span_to_spanset(&sp);
  }

  Span spans[2];
  if (datum_lt(sp1->lower, sp2->lower, sp1->basetype))
  {
    spans[0] = *sp1;
    spans[1] = *sp2;
  }
  else
  {
    spans[0] = *sp2;
    spans[1] = *sp1;
  }
  return spanset_make_exp(spans, 2, 2, NORMALIZE_NO, ORDER_NO);
}

/*****************************************************************************
 * Set intersection
 *****************************************************************************/

/**
 * @ingroup meos_internal_setspan_set
 * @brief Return the intersection of a span and a value
 * @param[in] sp Span
 * @param[in] value Value
 */
Span *
intersection_span_value(const Span *sp, Datum value)
{
  assert(sp);
  if (! contains_span_value(sp, value))
    return NULL;
  return value_span(value, sp->basetype);
}

/**
 * @ingroup meos_internal_setspan_set
 * @brief Return the union of a value and a span
 * @param[in] value Value
 * @param[in] sp Span
 */
Span *
intersection_value_span(Datum value, const Span *sp)
{
  return intersection_span_value(sp, value);
}

/**
 * @ingroup meos_internal_setspan_set
 * @brief Return in the last argument the intersection of two spans
 * @param[in] sp1,sp2 Spans
 * @param[out] result Resulting span
 * @note This function is equivalent to #intersection_span_span without
 * memory allocation
 */
bool
inter_span_span(const Span *sp1, const Span *sp2, Span *result)
{
  assert(sp1); assert(sp2); assert(sp1->spantype == sp2->spantype);
  /* Bounding box test */
  if (! overlaps_span_span(sp1, sp2))
    return false;

  Datum lower = span_max_value(sp1->lower, sp2->lower, sp1->basetype);
  Datum upper = span_min_value(sp1->upper, sp2->upper, sp1->basetype);
  bool lower_inc = sp1->lower == sp2->lower ? sp1->lower_inc && sp2->lower_inc :
    ( lower == sp1->lower ? sp1->lower_inc : sp2->lower_inc );
  bool upper_inc = sp1->upper == sp2->upper ? sp1->upper_inc && sp2->upper_inc :
    ( upper == sp1->upper ? sp1->upper_inc : sp2->upper_inc );
  span_set(lower, upper, lower_inc, upper_inc, sp1->basetype, sp1->spantype,
    result);
  return true;
}

/**
 * @ingroup meos_setspan_set
 * @brief Return the intersection of two spans
 * @param[in] sp1,sp2 Spans
 * @csqlfn #Intersection_span_span()
 */
Span *
intersection_span_span(const Span *sp1, const Span *sp2)
{
  /* Ensure the validity of the arguments */
  if (! ensure_valid_span_span(sp1, sp2))
    return NULL;
  Span result;
  if (! inter_span_span(sp1, sp2, &result))
    return NULL;
  return span_copy(&result);
}

/*****************************************************************************
 * Set difference
 *****************************************************************************/

/**
 * @ingroup meos_internal_setspan_set
 * @brief Return the difference of a value and a span
 * @param[in] value Value
 * @param[in] sp Span
 */
SpanSet *
minus_value_span(Datum value, const Span *sp)
{
  assert(sp);
  if (contains_span_value(sp, value))
    return NULL;
  return value_spanset(value, sp->basetype);
}

/**
 * @brief Return in the last argument the difference of a span and a value
 * @param[in] sp Span
 * @param[in] value Value
 * @param[out] result Resulting span
 */
int
mi_span_value(const Span *sp, Datum value, Span *result)
{
  assert(sp);
  /* The span does not contain the value */
  if (! contains_span_value(sp, value))
  {
    result[0] = *sp;
    return 1;
  }

  /* Account for canonicalized spans */
  Datum upper = span_decr_bound(sp->upper, sp->basetype);
  bool lowereq = datum_eq(sp->lower, value, sp->basetype);
  bool uppereq = datum_eq(upper, value, sp->basetype);
  /* The span is equal to the value */
  if (lowereq && uppereq)
    return 0;
  /* The value is equal to a bound */
  if (lowereq)
  {
    span_set(sp->lower, sp->upper, false, sp->upper_inc, sp->basetype,
      sp->spantype, &result[0]);
    return 1;
  }
  if (uppereq)
  {
    span_set(sp->lower, upper, sp->lower_inc, false, sp->basetype,
      sp->spantype, &result[0]);
    return 1;
  }
  /* The span is split into two */
  span_set(sp->lower, value, sp->lower_inc, false, sp->basetype,
     sp->spantype,&result[0]);
  span_set(value, sp->upper, false, sp->upper_inc, sp->basetype,
    sp->spantype, &result[1]);
  return 2;
}

/**
 * @ingroup meos_internal_setspan_set
 * @brief Return the difference of a span and a value
 * @param[in] sp Span
 * @param[in] value Value
 */
SpanSet *
minus_span_value(const Span *sp, Datum value)
{
  assert(sp);
  Span spans[2];
  int count = mi_span_value(sp, value, spans);
  if (count == 0)
    return NULL;
  return spanset_make_exp(spans, count, count, NORMALIZE_NO, ORDER_NO);
}

/**
 * @ingroup meos_internal_setspan_set
 * @brief Return in the last argument the difference of two spans
 * @param[in] sp1,sp2 Spans
 * @param[out] result Result
 * @return Number of the elements in the output array
 */
int
mi_span_span(const Span *sp1, const Span *sp2, Span *result)
{
  assert(sp1); assert(sp2); assert(result);
  /* Result is a span
   * sp1         |----|
   * sp2  |----|   or    |----|
   * result     |----|
   */
  if (left_span_span(sp1, sp2) || left_span_span(sp2, sp1))
  {
    result[0] = *sp1;
    return 1;
  }

  /* Deserialize the spans to minimize the number of comparisons */
  SpanBound lower1, lower2, upper1, upper2;
  span_deserialize((const Span *) sp1, &lower1, &upper1);
  span_deserialize((const Span *) sp2, &lower2, &upper2);
  int cmp_l1l2 = span_bound_cmp(&lower1, &lower2);
  int cmp_u1u2 = span_bound_cmp(&upper1, &upper2);

  /* Result is empty
   * sp1         |----|
   * sp2      |----------|
   */
  if (cmp_l1l2 >= 0 && cmp_u1u2 <= 0)
    return 0;

  /* Result is a span set
   * sp1      |----------|
   * sp2         |----|
   * result  |--|    |--|
   */
  if (cmp_l1l2 < 0 && cmp_u1u2 > 0)
  {
    span_set(sp1->lower, sp2->lower, sp1->lower_inc, !(sp2->lower_inc),
      sp1->basetype, sp1->spantype, &result[0]);
    span_set(sp2->upper, sp1->upper, !(sp2->upper_inc), sp1->upper_inc,
      sp1->basetype, sp1->spantype, &result[1]);
    return 2;
  }

  /* Result is a span
   * sp1           |-----|
   * sp2               |----|
   * result       |---|
   */
  if (cmp_l1l2 <= 0 && cmp_u1u2 <= 0)
    span_set(sp1->lower, sp2->lower, sp1->lower_inc, !(sp2->lower_inc),
      sp1->basetype, sp1->spantype, &result[0]);

  /* Result is a span
   * sp1         |-----|
   * sp2      |----|
   * result       |---|
   */
  else if (cmp_l1l2 >= 0 && cmp_u1u2 >= 0)
    span_set(sp2->upper, sp1->upper, !(sp2->upper_inc), sp1->upper_inc,
      sp1->basetype, sp1->spantype, &result[0]);

  return 1;
}

/**
 * @ingroup meos_setspan_set
 * @brief Return the difference of two spans
 * @param[in] sp1,sp2 Spans
 * @csqlfn #Minus_span_span()
 */
SpanSet *
minus_span_span(const Span *sp1, const Span *sp2)
{
  /* Ensure the validity of the arguments */
  if (! ensure_valid_span_span(sp1, sp2))
    return NULL;

  Span spans[2];
  int count = mi_span_span(sp1, sp2, spans);
  if (count == 0)
    return NULL;
  return spanset_make_exp(spans, count, count, NORMALIZE_NO, ORDER_NO);
}

/******************************************************************************
 * Distance functions
 ******************************************************************************/

/**
 * @brief Return the distance between two values as a double for the indexes
 * @param[in] l,r Values
 * @param[in] type Type of the values
 * @return On error return -1
 */
double
dist_double_value_value(Datum l, Datum r, meosType type)
{
  assert(span_basetype(type));
  switch (type)
  {
    case T_INT4:
      return Float8GetDatum((double) abs(DatumGetInt32(l) - DatumGetInt32(r)));
    case T_INT8:
      return Float8GetDatum((double) llabs(DatumGetInt64(l) - DatumGetInt64(r)));
    case T_FLOAT8:
      return Float8GetDatum(fabs(DatumGetFloat8(l) - DatumGetFloat8(r)));
    case T_DATE:
      /* Distance in days if the base type is DateADT */
      return Float8GetDatum((double) abs(DatumGetDateADT(l) - DatumGetDateADT(r)));
    case T_TIMESTAMPTZ:
      /* Distance in seconds if the base type is TimestampTz */
      return Float8GetDatum((llabs((DatumGetTimestampTz(l) -
        DatumGetTimestampTz(r)))) / USECS_PER_SEC);
    default:
      meos_error(ERROR, MEOS_ERR_INTERNAL_TYPE_ERROR,
        "Unknown types for distance between values: %s",
        meostype_name(type));
      return -1;
  }
}

/**
 * @ingroup meos_internal_setspan_dist
 * @brief Return the distance between two values
 * @param[in] l,r Values
 * @param[in] type Type of the values
 * @return On error return -1
 */
Datum
distance_value_value(Datum l, Datum r, meosType type)
{
  assert(span_basetype(type));
  switch (type)
  {
    case T_INT4:
      return Int32GetDatum(abs(DatumGetInt32(l) - DatumGetInt32(r)));
    case T_INT8:
      return Int64GetDatum(llabs(DatumGetInt64(l) - DatumGetInt64(r)));
    case T_FLOAT8:
      return Float8GetDatum(fabs(DatumGetFloat8(l) - DatumGetFloat8(r)));
    case T_DATE:
      /* Distance in days if the base type is DateADT */
      return Int32GetDatum(abs(DatumGetDateADT(l) - DatumGetDateADT(r)));
    case T_TIMESTAMPTZ:
      /* Distance in seconds if the base type is TimestampTz */
      return Float8GetDatum((llabs((DatumGetTimestampTz(l) -
        DatumGetTimestampTz(r)))) / USECS_PER_SEC);
    default:
      meos_error(ERROR, MEOS_ERR_INTERNAL_TYPE_ERROR,
        "Unknown types for distance between values: %s",
        meostype_name(type));
      return (Datum) -1;
  }
}

/**
 * @ingroup meos_internal_setspan_dist
 * @brief Return the distance between a span and a value as a double
 * @param[in] sp Span
 * @param[in] value Value
 */
Datum
distance_span_value(const Span *sp, Datum value)
{
  assert(sp);
  /* If the span contains the value return 0 */
  if (contains_span_value(sp, value))
    return (Datum) 0;

  /* If the span is to the right of the value return the distance
   * between the value and the lower bound of the span
   *     value   [---- sp ----] */
  if (right_span_value(sp, value))
    return distance_value_value(value, sp->lower, sp->basetype);

  /* Account for canonicalized spans */
  Datum upper = span_decr_bound(sp->upper, sp->basetype);

  /* If the span is to the left of the value return the distance
   * between the upper bound of the span and value
   *     [---- sp ----]   value */
  return distance_value_value(upper, value, sp->basetype);
}

/**
 * @ingroup meos_internal_setspan_dist
 * @brief Return the distance between two spans as a double
 * @param[in] sp1,sp2 Spans
 * @return On error return -1
 * @csqlfn #Distance_span_span()
 */
Datum
distance_span_span(const Span *sp1, const Span *sp2)
{
  assert(sp1); assert(sp2); assert(sp1->spantype == sp2->spantype);
  /* If the spans intersect return 0 */
  if (overlaps_span_span(sp1, sp2))
    return (Datum) 0;

  /* Account for canonicalized spans */
  Datum upper1 = span_decr_bound(sp1->upper, sp1->basetype);
  Datum upper2 = span_decr_bound(sp2->upper, sp2->basetype);

  /* If the first span is to the left of the second one return the distance
   * between the upper bound of the first and lower bound of the second
   *     [---- sp1 ----]   [---- sp2 ----] */
  if (left_span_span(sp1, sp2))
    return distance_value_value(upper1, sp2->lower, sp1->basetype);

  /* If the first span is to the right of the second one return the distance
   * between the upper bound of the second and the lower bound of the first
   *     [---- sp2 ----]   [---- sp1 ----] */
  return distance_value_value(upper2, sp1->lower, sp1->basetype);
}

/******************************************************************************/
