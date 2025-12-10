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
 * @brief General functions for spans (a.k.a. ranges) composed of two `Datum`
 * values and two `Boolean` values stating whether the bounds are inclusive
 */

#include "temporal/span.h"

/* C */
#include <assert.h>
#include <float.h>
#include <limits.h>
/* PostgreSQL */
#include <postgres.h>
#include <common/hashfn.h>
#include "port/pg_bitutils.h"
#include <utils/float.h>
#include <utils/timestamp.h>
/* MEOS */
#include <meos.h>
#include <meos_internal.h>
#include "temporal/meos_catalog.h"
#include "temporal/set.h"
#include "temporal/temporal.h"
#include "temporal/tnumber_mathfuncs.h"
#include "temporal/type_parser.h"
#include "temporal/type_inout.h"
#include "temporal/type_util.h"

#include <utils/jsonb.h>
#include <utils/numeric.h>
#include <pgtypes.h>

/*****************************************************************************
 * Parameter tests
 *****************************************************************************/

/**
 * @brief Ensure that a span is of a given span type
 */
bool
ensure_span_isof_type(const Span *sp, meosType spantype)
{
  if (sp->spantype == spantype)
    return true;
  meos_error(ERROR, MEOS_ERR_INVALID_ARG_TYPE,
    "The span must be of type %s", meostype_name(spantype));
  return false;
}

/**
 * @brief Ensure that a span is of a given base type
 */
bool
ensure_span_isof_basetype(const Span *sp, meosType basetype)
{
  if (sp->basetype == basetype)
    return true;
  meos_error(ERROR, MEOS_ERR_INVALID_ARG_TYPE,
    "Operation on mixed span and base types: %s and %s",
    meostype_name(sp->spantype), meostype_name(basetype));
  return false;
}

/**
 * @brief Ensure that the spans have the same type
 */
bool
ensure_same_span_type(const Span *sp1, const Span *sp2)
{
  if (sp1->spantype == sp2->spantype)
    return true;
  meos_error(ERROR, MEOS_ERR_INVALID_ARG_TYPE,
    "Operation on mixed span types: %s and %s",
    meostype_name(sp1->spantype), meostype_name(sp2->spantype));
  return false;
}

/**
 * @brief Ensure that two span sets are of the same span type
 */
bool
ensure_valid_span_span(const Span *sp1, const Span *sp2)
{
  VALIDATE_NOT_NULL(sp1, false); VALIDATE_NOT_NULL(sp2, false);
  if (! ensure_same_span_type(sp1, sp2))
    return false;
  return true;
}

/*****************************************************************************
 * General functions
 *****************************************************************************/

/**
 * @brief Deconstruct a span
 * @param[in] sp Span value
 * @param[out] lower,upper Bounds
 */
void
span_deserialize(const Span *sp, SpanBound *lower, SpanBound *upper)
{
  if (lower)
  {
    lower->val = sp->lower;
    lower->inclusive = sp->lower_inc;
    lower->lower = true;
    lower->spantype = sp->spantype;
    lower->basetype = sp->basetype;
  }
  if (upper)
  {
    upper->val = sp->upper;
    upper->inclusive = sp->upper_inc;
    upper->lower = false;
    upper->spantype = sp->spantype;
    upper->basetype = sp->basetype;
  }
  return;
}

/**
 * @brief Compare two span boundary points, returning <0, 0, or >0 according to
 * whether the first one is less than, equal to, or greater than the second one
 * @details The boundaries can be any combination of upper and lower; so it is
 * useful for a variety of operators.
 *
 * The simple case is when b1 and b2 are both inclusive, in which
 * case the result is just a comparison of the values held in b1 and b2.
 *
 * If a bound is exclusive, then we need to know whether it is a lower bound,
 * in which case we treat the boundary point as "just greater than" the held
 * value; or an upper bound, in which case we treat the boundary point as
 * "just less than" the held value.
 *
 * There is only one case where two boundaries compare equal but are not
 * identical: when both bounds are inclusive and hold the same value,
 * but one is an upper bound and the other a lower bound.
 */
int
span_bound_cmp(const SpanBound *b1, const SpanBound *b2)
{
  assert(b1); assert(b2); assert(b1->basetype == b2->basetype);
  /* Compare the values */
  int32_t result = datum_cmp(b1->val, b2->val, b1->basetype);

  /*
   * If the comparison is not equal and the bounds are both inclusive or
   * both exclusive, we're done. If they compare equal, we still have to
   * consider whether the boundaries are inclusive or exclusive.
  */
  if (result == 0)
  {
    if (! b1->inclusive && ! b2->inclusive)
    {
      /* both bounds are exclusive */
      if (b1->lower == b2->lower)
        /* both are lower bound */
        return 0;
      else
        return b1->lower ? 1 : -1;
    }
    else if (! b1->inclusive)
      return b1->lower ? 1 : -1;
    else if (! b2->inclusive)
      return b2->lower ? -1 : 1;
  }

  return result;
}

/**
 * @brief Comparison function for sorting span bounds
 */
int
span_bound_qsort_cmp(const void *a1, const void *a2)
{
  SpanBound *b1 = (SpanBound *) a1;
  SpanBound *b2 = (SpanBound *) a2;
  return span_bound_cmp(b1, b2);
}

/**
 * @brief Compare the lower bounds of two spans, returning <0, 0, or >0 according to
 * whether the first bound is less than, equal to, or greater than the second one
 * @note The function is equivalent to #span_bound_cmp but avoids
 * deserializing the spans into lower and upper bounds
 */
int
span_lower_cmp(const Span *sp1, const Span *sp2)
{
  assert(sp1); assert(sp2); assert(sp1->basetype == sp2->basetype);
  int result = datum_cmp(sp1->lower, sp2->lower, sp1->basetype);
  if (result != 0)
    return result;
  /* The bound values are equal */
  if (sp1->lower_inc == sp2->lower_inc)
    /* both are inclusive or exclusive */
    return 0;
  else if (sp1->lower_inc)
    /* first is inclusive and second is exclusive */
    return 1;
  else
    /* first is exclusive and second is inclusive */
    return -1;
}

/**
 * @brief Compare the upper bounds of two spans, returning <0, 0, or >0
 * according to whether the first bound is less than, equal to, or greater than
 * the second one.
 * @note The function is equivalent to #span_bound_cmp but avoids
 * deserializing the spans into lower and upper bounds
 */
int
span_upper_cmp(const Span *sp1, const Span *sp2)
{
  assert(sp1); assert(sp2); assert(sp1->basetype == sp2->basetype);
  int result = datum_cmp(sp1->upper, sp2->upper, sp1->basetype);
  if (result != 0)
    return result;
  /* The bound values are equal */
  if (sp1->upper_inc == sp2->upper_inc)
    /* both are inclusive or exclusive */
    return 0;
  else if (sp1->upper_inc)
    /* first is inclusive and second is exclusive */
    return 1;
  else
    /* first is exclusive and second is inclusive */
    return -1;
}

/**
 * @brief Return the bound increased by 1 for accounting for canonicalized spans
 */
Datum
span_incr_bound(Datum lower, meosType basetype)
{
  Datum result;
  switch (basetype)
  {
    case T_INT4:
      result = Int32GetDatum(DatumGetInt32(lower) + (int32_t) 1);
      break;
    case T_INT8:
      result = Int64GetDatum(DatumGetInt64(lower) + (int64_t) 1);
      break;
    case T_DATE:
      result = DateADTGetDatum(DatumGetDateADT(lower) + 1);
      break;
    default:
      result = lower;
  }
  return result;
}

/**
 * @brief Return the bound decreased by 1 for accounting for canonicalized spans
 */
Datum
span_decr_bound(Datum lower, meosType basetype)
{
  Datum result;
  switch (basetype)
  {
    case T_INT4:
      result = Int32GetDatum(DatumGetInt32(lower) - (int32_t) 1);
      break;
    case T_INT8:
      result = Int64GetDatum(DatumGetInt64(lower) - (int64_t) 1);
      break;
    case T_DATE:
      result = DateADTGetDatum(DatumGetDateADT(lower) - 1);
      break;
    default:
      result = lower;
  }
  return result;
}

/**
 * @brief Normalize an array of spans
 * @details The input spans may overlap and may be non contiguous.
 * The normalized spans are new spans that must be freed.
 * @param[in] spans Array of spans
 * @param[in] count Number of elements in the input array
 * @param[in] order True if the spans should be ordered
 * @param[out] newcount Number of elements in the output array
 * @pre @p count is greater than 0
 */
Span *
spanarr_normalize(Span *spans, int count, bool order, int *newcount)
{
  assert(spans); assert(count > 0); assert(newcount);
  /* Sort the spans if they are not ordered */
  if (order)
    spanarr_sort(spans, count);
  int nspans = 0;
  Span *result = palloc(sizeof(Span) * count);
  Span *current = &spans[0];
  for (int i = 1; i < count; i++)
  {
    Span *next = &spans[i];
    if (ovadj_span_span(current, next))
      /* Compute the union of the spans */
      span_expand(next, current);
    else
    {
      result[nspans++] = *current;
      current = next;
    }
  }
  result[nspans++] = *current;
  /* Set the output parameter */
  *newcount = nspans;
  return result;
}

/*****************************************************************************
 * Input/output functions
 *****************************************************************************/

/**
 * @ingroup meos_internal_setspan_inout
 * @brief Return a span from its Well-Known Text (WKT) representation
 * @param[in] str String
 * @param[in] spantype Span type
 */
Span *
span_in(const char *str, meosType spantype)
{
  assert(str);
  Span result;
  if (! span_parse(&str, spantype, true, &result))
    return NULL;
  return span_copy(&result);
}

/**
 * @brief Remove the quotes from the Well-Known Text (WKT) representation of a
 * span
 */
static char *
unquote(char *str)
{
  /* Save the initial pointer */
  char *result = str;
  char *last = str;
  while (*str != '\0')
  {
    if (*str != '"')
      *last++ = *str;
    str++;
  }
  *last = '\0';
  return result;
}

/**
 * @ingroup meos_internal_setspan_inout
 * @brief Return the Well-Known Text (WKT) representation of a span
 * @param[in] sp Span
 * @param[in] maxdd Maximum number of decimal digits
 */
char *
span_out(const Span *sp, int maxdd)
{
  assert(sp);
  /* Ensure the validity of the arguments */
  if (! ensure_not_negative(maxdd))
    return NULL;

  char *lower = unquote(basetype_out(sp->lower, sp->basetype, maxdd));
  char *upper = unquote(basetype_out(sp->upper, sp->basetype, maxdd));
  char open = sp->lower_inc ? (char) '[' : (char) '(';
  char close = sp->upper_inc ? (char) ']' : (char) ')';
  size_t size = strlen(lower) + strlen(upper) + 5;
  char *result = palloc(size);
  snprintf(result, size, "%c%s, %s%c", open, lower, upper, close);
  pfree(lower); pfree(upper);
  return result;
}

/*****************************************************************************
 * Constructor functions
 *****************************************************************************/

/**
 * @ingroup meos_internal_setspan_constructor
 * @brief Return a span from the bounds
 * @param[in] lower,upper Bounds
 * @param[in] lower_inc,upper_inc True when the bounds are inclusive
 * @param[in] basetype Type of the bounds
 */
Span *
span_make(Datum lower, Datum upper, bool lower_inc, bool upper_inc,
  meosType basetype)
{
  Span *sp = palloc(sizeof(Span));
  meosType spantype = basetype_spantype(basetype);
  span_set(lower, upper, lower_inc, upper_inc, basetype, spantype, sp);
  return sp;
}

/**
 * @ingroup meos_internal_setspan_constructor
 * @brief Return in the last argument a span constructed from the given
 * arguments
 * @param[in] lower,upper Bounds
 * @param[in] lower_inc,upper_inc True when the bounds are inclusive
 * @param[in] basetype Base type
 * @param[in] spantype Span type
 * @param[out] result Result span
 * @see #span_make()
 */
void
span_set(Datum lower, Datum upper, bool lower_inc, bool upper_inc,
  meosType basetype, meosType spantype, Span *result)
{
  assert(result); assert(basetype_spantype(basetype) == spantype);
  /* Canonicalize */
  if (span_canon_basetype(basetype))
  {
    if (! lower_inc)
    {
      lower = span_incr_bound(lower, basetype);
      lower_inc = true;
    }
    if (upper_inc)
    {
      upper = span_incr_bound(upper, basetype);
      upper_inc = false;
    }
  }

  int cmp = datum_cmp(lower, upper, basetype);
  /* error check: if lower bound value is above upper, it's wrong */
  if (cmp > 0)
  {
    meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
      "Span lower bound must be less than or equal to span upper bound");
    return;
  }

  /* error check: if bounds are equal, and not both inclusive, span is empty */
  if (cmp == 0 && ! (lower_inc && upper_inc))
  {
    meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE, "Span cannot be empty");
    return;
  }

  /* Note: zero-fill is required here, just as in heap tuples */
  memset(result, 0, sizeof(Span));
  /* Fill in the span */
  result->lower = lower;
  result->upper = upper;
  result->lower_inc = lower_inc;
  result->upper_inc = upper_inc;
  result->spantype = spantype;
  result->basetype = basetype;
  return;
}

/**
 * @ingroup meos_setspan_constructor
 * @brief Return a copy of a span
 * @param[in] sp Span
 */
Span *
span_copy(const Span *sp)
{
  /* Ensure the validity of the arguments */
  VALIDATE_NOT_NULL(sp, NULL);
  Span *result = palloc(sizeof(Span));
  memcpy((char *) result, (char *) sp, sizeof(Span));
  return result;
}

/*****************************************************************************
 * Conversion functions
 *****************************************************************************/

/**
 * @ingroup meos_internal_setspan_conversion
 * @brief Return in the last argument a span constructed from a value
 * @param[in] value Value
 * @param[in] basetype Type of the value
 * @param[out] sp Result span
*/
void
value_set_span(Datum value, meosType basetype, Span *sp)
{
  assert(sp); assert(span_basetype(basetype));
  meosType spantype = basetype_spantype(basetype);
  span_set(value, value, true, true, basetype, spantype, sp);
  return;
}

/**
 * @ingroup meos_internal_setspan_conversion
 * @brief Convert a value into a span
 * @param[in] value Value
 * @param[in] basetype Type of the value
 */
Span *
value_span(Datum value, meosType basetype)
{
  Span *result = palloc(sizeof(Span));
  value_set_span(value, basetype, result);
  return result;
}

/**
 * @ingroup meos_internal_setspan_conversion
 * @brief Return in the last argument the bounding span of a set
 * @param[in] s Set
 * @param[in] fromidx,toidx From and to indexes of the values used for the span
 * bounds
 * @param[in] result Span
 */
void
set_set_subspan(const Set *s, int fromidx, int toidx, Span *result)
{
  assert(s); assert(result);
  meosType spantype = basetype_spantype(s->basetype);
  span_set(SET_VAL_N(s, fromidx), SET_VAL_N(s, toidx), true, true,
    s->basetype, spantype, result);
  return;
}

/**
 * @ingroup meos_internal_setspan_conversion
 * @brief Return in the last argument the bounding span of a set
 * @param[in] s Set
 * @param[in] result Span
 */
void
set_set_span(const Set *s, Span *result)
{
  assert(s); assert(result);
  return set_set_subspan(s, 0, s->count - 1, result);
}

/**
 * @ingroup meos_internal_setspan_conversion
 * @brief Convert a set into a span
 * @param[in] s Set
 * @csqlfn #Set_to_span()
 */
Span *
set_span(const Set *s)
{
  /* Ensure the validity of the arguments */
  assert(s); assert(set_spantype(s->settype));
  Span *result = palloc(sizeof(Span));
  set_set_span(s, result);
  return result;
}

#if MEOS
/**
 * @ingroup meos_setspan_conversion
 * @brief Convert a set into a span
 * @param[in] s Set
 * @csqlfn #Set_to_span()
 */
Span *
set_to_span(const Set *s)
{
  /* Ensure the validity of the arguments */
  VALIDATE_NOT_NULL(s, NULL);
  if (! ensure_set_spantype(s->settype))
    return NULL;
  return set_span(s);
}
#endif /* MEOS */

/**
 * @ingroup meos_internal_setspan_conversion
 * @brief Return the second span initialized with the first one transformed to
 * a float span
 * @param[in] sp1,sp2 Spans
 */
void
intspan_set_floatspan(const Span *sp1, Span *sp2)
{
  assert(sp1); assert(sp2); assert(sp1->spantype == T_INTSPAN);
  Datum lower = Float8GetDatum((double) DatumGetInt32(sp1->lower));
  Datum upper = Float8GetDatum((double) (DatumGetInt32(sp1->upper) - 1));
  span_set(lower, upper, true, true, T_FLOAT8, T_FLOATSPAN, sp2);
  return;
}

/**
 * @ingroup meos_setspan_conversion
 * @brief Convert an integer span into a float span
 * @param[in] sp Span
 * @return On error return @p NULL
 */
Span *
intspan_to_floatspan(const Span *sp)
{
  /* Ensure the validity of the arguments */
  VALIDATE_INTSPAN(sp, NULL);
  Span *result = palloc(sizeof(Span));
  intspan_set_floatspan(sp, result);
  return result;
}

/**
 * @ingroup meos_internal_setspan_conversion
 * @brief Return the second span initialized with the first one transformed to
 * an integer span
 * @param[in] sp1,sp2 Spans
 */
void
floatspan_set_intspan(const Span *sp1, Span *sp2)
{
  assert(sp1); assert(sp2); assert(sp1->spantype == T_FLOATSPAN);
  Datum lower = Int32GetDatum((int) DatumGetFloat8(sp1->lower));
  Datum upper = Int32GetDatum((int) (DatumGetFloat8(sp1->upper)));
  span_set(lower, upper, sp1->lower_inc, sp1->upper_inc, T_INT4, T_INTSPAN, sp2);
  return;
}

/**
 * @ingroup meos_setspan_conversion
 * @brief Convert a float span into an integer span
 * @param[in] sp Span
 * @return On error return @p NULL
 */
Span *
floatspan_to_intspan(const Span *sp)
{
  VALIDATE_FLOATSPAN(sp, NULL);
  Span *result = palloc(sizeof(Span));
  floatspan_set_intspan(sp, result);
  return result;
}

/**
 * @ingroup meos_internal_setspan_conversion
 * @brief Return the second span initialized with the first one transformed to
 * a timetstamptz span
 * @param[in] sp1,sp2 Spans
 */
void
datespan_set_tstzspan(const Span *sp1, Span *sp2)
{
  assert(sp1); assert(sp2); assert(sp1->spantype == T_DATESPAN);
  Datum lower =
    TimestampTzGetDatum(date_to_timestamptz(DatumGetDateADT(sp1->lower)));
  Datum upper =
    TimestampTzGetDatum(date_to_timestamptz(DatumGetDateADT(sp1->upper)));
  /* Date spans are always canonicalized */
  span_set(lower, upper, true, false, T_TIMESTAMPTZ, T_TSTZSPAN, sp2);
  return;
}

/**
 * @ingroup meos_setspan_conversion
 * @brief Convert a date span into a timestamptz span
 * @param[in] sp Span
 * @return On error return @p NULL
 */
Span *
datespan_to_tstzspan(const Span *sp)
{
  /* Ensure the validity of the arguments */
  VALIDATE_DATESPAN(sp, NULL);
  Span *result = palloc(sizeof(Span));
  datespan_set_tstzspan(sp, result);
  return result;
}

/**
 * @ingroup meos_internal_setspan_conversion
 * @brief Return the last span initialized with the first one transformed to a
 * date span
 * @param[in] sp1,sp2 Spans
 */
void
tstzspan_set_datespan(const Span *sp1, Span *sp2)
{
  assert(sp1); assert(sp2); assert(sp1->spantype == T_TSTZSPAN);
  DateADT lower = timestamptz_to_date(DatumGetTimestampTz(sp1->lower));
  DateADT upper = timestamptz_to_date(DatumGetTimestampTz(sp1->upper));
  bool lower_inc = sp1->lower_inc;
  bool upper_inc = sp1->upper_inc;
  /* Both bounds are set to true when the resulting dates are equal, e.g.,
   * (2001-10-18 19:46:00, 2001-10-18 19:50:00) -> [2001-10-18, 2001-10-18] */
  if (lower == upper)
  {
    lower_inc = upper_inc = true;
  }
  /* Canonicalization takes place in the following function */
  span_set(DateADTGetDatum(lower), DateADTGetDatum(upper), lower_inc,
    upper_inc, T_DATE, T_DATESPAN, sp2);
  return;
}

/**
 * @ingroup meos_setspan_conversion
 * @brief Convert a timestamptz span into a date span
 * @param[in] sp Span
 * @return On error return @p NULL
 */
Span *
tstzspan_to_datespan(const Span *sp)
{
  /* Ensure the validity of the arguments */
  VALIDATE_TSTZSPAN(sp, NULL);
  Span *result = palloc(sizeof(Span));
  tstzspan_set_datespan(sp, result);
  return result;
}

/*****************************************************************************
 * Accessor functions
 *****************************************************************************/

/**
 * @ingroup meos_internal_setspan_accessor
 * @brief Return the width of a span
 * @param[in] sp Span
 * @csqlfn #Numspan_width()
 */
Datum
numspan_width(const Span *sp)
{
  assert(sp);
  return distance_value_value(sp->upper, sp->lower, sp->basetype);
}

/**
 * @ingroup meos_setspan_accessor
 * @brief Return the duration of a date span as an interval
 * @param[in] sp Span
 * @csqlfn #Datespan_duration()
 */
Interval *
datespan_duration(const Span *sp)
{
  /* Ensure the validity of the arguments */
  VALIDATE_DATESPAN(sp, NULL);
  Interval *result = palloc0(sizeof(Interval));
  result->day = DateADTGetDatum(sp->upper) - DateADTGetDatum(sp->lower);
  return result;
}

/**
 * @ingroup meos_setspan_accessor
 * @brief Return the duration of a timestamptz span as an interval
 * @param[in] sp Span
 * @csqlfn #Tstzspan_duration()
 */
Interval *
tstzspan_duration(const Span *sp)
{
  /* Ensure the validity of the arguments */
  VALIDATE_TSTZSPAN(sp, NULL);
  return minus_timestamptz_timestamptz(sp->upper, sp->lower);
}

/*****************************************************************************
 * Transformation functions
 *****************************************************************************/

/**
 * @ingroup meos_internal_setspan_transf
 * @brief Return in the last argument a float span with the precision set to a
 * number of decimal places
 * @param[in] sp Span
 * @param[in] maxdd Maximum number of decimal digits
 * @param[out] result Result span
 */
void
floatspan_round_set(const Span *sp, int maxdd, Span *result)
{
  assert(sp); assert(sp->spantype == T_FLOATSPAN); assert(result);
  /* Set precision of bounds */
  double lower = float8_round(DatumGetFloat8(sp->lower), maxdd);
  double upper = float8_round(DatumGetFloat8(sp->upper), maxdd);
  /* Fix the bounds */
  bool lower_inc, upper_inc;
  if (float8_eq(lower, upper))
  {
    lower_inc = upper_inc = true;
  }
  else
  {
    lower_inc = sp->lower_inc; upper_inc = sp->upper_inc;
  }
  /* Set resulting span */
  span_set(Float8GetDatum(lower), Float8GetDatum(upper), lower_inc, upper_inc,
    sp->basetype, sp->spantype, result);
  return;
}

/**
 * @ingroup meos_setspan_transf
 * @brief Return a float span with the precision of the bounds set to a
 * number of decimal places
 * @param[in] sp Span
 * @param[in] maxdd Maximum number of decimal digits
 * @return On error return @p NULL
 */
Span *
floatspan_round(const Span *sp, int maxdd)
{
  /* Ensure the validity of the arguments */
  VALIDATE_FLOATSPAN(sp, NULL);
  if (! ensure_not_negative(maxdd))
    return NULL;

  Span *result = palloc(sizeof(Span));
  floatspan_round_set(sp, maxdd, result);
  return result;
}

/*****************************************************************************/

/**
 * @brief Round down a span to the nearest integer
 */
void
floatspan_floor_ceil_iter(Span *sp, datum_func1 func)
{
  assert(sp);
  Datum lower = func(sp->lower);
  Datum upper = func(sp->upper);
  bool lower_inc = sp->lower_inc;
  bool upper_inc = sp->upper_inc;
  if (datum_eq(lower, upper, sp->basetype))
    lower_inc = upper_inc = true;
  span_set(lower, upper, lower_inc, upper_inc, sp->basetype, sp->spantype, sp);
  return;
}

/**
 * @ingroup meos_setspan_transf
 * @brief Return a float span rounded down to the nearest integer
 * @csqlfn #Floatspan_floor()
 */
Span *
floatspan_floor(const Span *sp)
{
  /* Ensure the validity of the arguments */
  VALIDATE_FLOATSPAN(sp, NULL);
  Span *result = span_copy(sp);
  floatspan_floor_ceil_iter(result, &datum_floor);
  return result;
}

/**
 * @ingroup meos_setspan_transf
 * @brief Return a float span rounded up to the nearest integer
 * @csqlfn #Floatspan_ceil()
 */
Span *
floatspan_ceil(const Span *sp)
{
  /* Ensure the validity of the arguments */
  VALIDATE_FLOATSPAN(sp, NULL);
  Span *result = span_copy(sp);
  floatspan_floor_ceil_iter(result, &datum_ceil);
  return result;
}

/**
 * @ingroup meos_setspan_transf
 * @brief Return a float span with the values converted to degrees
 * @param[in] sp Span
 * @param[in] normalize True when the result must be normalized
 * @csqlfn #Floatspan_degrees()
 */
Span *
floatspan_degrees(const Span *sp, bool normalize)
{
  /* Ensure the validity of the arguments */
  VALIDATE_FLOATSPAN(sp, NULL);
  Span *result = span_copy(sp);
  result->lower = datum_degrees(sp->lower, normalize);
  result->upper = datum_degrees(sp->upper, normalize);
  return result;
}

/**
 * @ingroup meos_setspan_transf
 * @brief Return a float span with the values converted to radians
 * @param[in] sp Span
 * @csqlfn #Floatspan_radians()
 */
Span *
floatspan_radians(const Span *sp)
{
  /* Ensure the validity of the arguments */
  VALIDATE_FLOATSPAN(sp, NULL);
  Span *result = span_copy(sp);
  result->lower = datum_radians(sp->lower);
  result->upper = datum_radians(sp->upper);
  return result;
}

/*****************************************************************************/

/**
 * @ingroup meos_internal_setspan_transf
 * @brief Return the second span expanded with the first one
 * @param[in] sp1,sp2 Spans
 */
void
span_expand(const Span *sp1, Span *sp2)
{
  assert(sp1); assert(sp2); assert(sp1->spantype == sp2->spantype);

  int cmp1 = datum_cmp(sp2->lower, sp1->lower, sp1->basetype);
  int cmp2 = datum_cmp(sp2->upper, sp1->upper, sp1->basetype);
  bool lower1 = cmp1 < 0 || (cmp1 == 0 && (sp2->lower_inc || ! sp1->lower_inc));
  bool upper1 = cmp2 > 0 || (cmp2 == 0 && (sp2->upper_inc || ! sp1->upper_inc));
  sp2->lower = lower1 ? sp2->lower : sp1->lower;
  sp2->lower_inc = lower1 ? sp2->lower_inc : sp1->lower_inc;
  sp2->upper = upper1 ? sp2->upper : sp1->upper;
  sp2->upper_inc = upper1 ? sp2->upper_inc : sp1->upper_inc;
  return;
}

/*****************************************************************************/

/**
 * @ingroup meos_internal_setspan_transf
 * @brief Return a number span with its bounds expanded/decreased by a value
 * @param[in] sp Span
 * @param[in] value Value
 * @csqlfn #Numspan_expand()
 * @note This function can be seen as a 1-dimensional version of the PostGIS
 * function `ST_Buffer`
 */
Span *
numspan_expand(const Span *sp, Datum value)
{
  /* Ensure the validity of the arguments */
  VALIDATE_NUMSPAN(sp, NULL);
  /* When the value is negative, return NULL if the span resulting by
   * shifting the bounds with the value is empty */ 
  if (datum_cmp(value, (Datum) 0, sp->basetype) <= 0)
  {
    Datum width = numspan_width(sp);
    Datum value2 = datum_add(value, value, sp->basetype);
    /* We avoid taking the absolute value by adding the two values */
    Datum add = datum_add(value2, width, sp->basetype);
    int cmp = datum_cmp(add, (Datum) 0, sp->basetype);
    if (cmp < 0 || (cmp == 0 && (! sp->lower_inc || ! sp->upper_inc)))
      return NULL;
  }
  Span *result = span_copy(sp);
  result->lower = datum_sub(sp->lower, value, sp->basetype);
  result->upper = datum_add(sp->upper, value, sp->basetype);
  return result;
}

#if MEOS
/**
 * @ingroup meos_setspan_transf
 * @brief Return an integer span with its bounds expanded/decreased by a value
 * @param[in] sp Span
 * @param[in] i Value
 * @csqlfn #Numspan_expand()
 */
Span *
intspan_expand(const Span *sp, int i)
{
  return numspan_expand(sp, Int32GetDatum(i));
}

/**
 * @ingroup meos_setspan_transf
 * @brief Return a big integer span with its bounds expanded/decreased by a
 * value
 * @param[in] sp Span
 * @param[in] i Value
 * @csqlfn #Numspan_expand()
 */
Span *
bigintspan_expand(const Span *sp, int64_t i)
{
  return numspan_expand(sp, Int64GetDatum(i));
}

/**
 * @ingroup meos_setspan_transf
 * @brief Return a float span with its bounds expanded/decreased by a value
 * @param[in] sp Span
 * @param[in] d Value
 * @csqlfn #Numspan_expand()
 */
Span *
floatspan_expand(const Span *sp, double d)
{
  return numspan_expand(sp, Float8GetDatum(d));
}
#endif /* MEOS */

/**
 * @ingroup meos_setspan_transf
 * @brief Return a timestamptz span with its bounds expanded/decreased by an
 * interval
 * @param[in] sp Span
 * @param[in] interv Interval
 * @csqlfn #Tstzspan_expand()
 * @note This function can be seen as a 1-dimensional version of the PostGIS
 * function `ST_Buffer`
 */
Span *
tstzspan_expand(const Span *sp, const Interval *interv)
{
  /* Ensure the validity of the arguments */
  VALIDATE_NOT_NULL(sp, NULL); VALIDATE_NOT_NULL(interv, NULL);
  /* When the interval is negative, return NULL if the span resulting by
   * shifting the bounds with the interval is empty */ 
  Interval intervalzero;
  memset(&intervalzero, 0, sizeof(Interval));
  bool negative = pg_interval_cmp(interv, &intervalzero) <= 0;
  Interval *interv_neg;
  if (negative)
  {
    Interval *duration = tstzspan_duration(sp);
    /* Negate the interval */
    interv_neg = interval_negate(interv);
    Interval *interv_neg2 = mul_interval_float8(interv_neg, 2.0);
    int cmp = pg_interval_cmp(duration, interv_neg2);
    pfree(duration); pfree(interv_neg2);
    if (cmp < 0 || (cmp == 0 && (! sp->lower_inc || ! sp->upper_inc)))
    {
      pfree(interv_neg);
      return NULL;
    }
  }

  Span *result = span_copy(sp);
  TimestampTz tmin = negative ?
    add_timestamptz_interval(DatumGetTimestampTz(sp->lower), interv_neg) :
    minus_timestamptz_interval(DatumGetTimestampTz(sp->lower),
      (Interval *) interv);
  TimestampTz tmax = add_timestamptz_interval(DatumGetTimestampTz(sp->upper),
    (Interval *) interv);
  result->lower = TimestampTzGetDatum(tmin);
  result->upper = TimestampTzGetDatum(tmax);
  if (negative)
    pfree(interv_neg);
  return result;
}

/*****************************************************************************/

/**
 * @brief Shift and/or scale the span bounds by two values
 * @param[in] shift Value for shifting the bounds
 * @param[in] width Width of the result
 * @param[in] basetype Type of the values
 * @param[in] hasshift True when the shift argument is given
 * @param[in] haswidth True when the width argument is given
 * @param[in,out] lower,upper Bounds of the period
 */
void
span_bounds_shift_scale_value(Datum shift, Datum width, meosType basetype,
  bool hasshift, bool haswidth, Datum *lower, Datum *upper)
{
  assert(hasshift || haswidth); assert(lower); assert(upper);
  assert(! haswidth || positive_datum(width, basetype));

  bool instant = datum_eq(*lower, *upper, basetype);
  if (hasshift)
  {
    *lower = datum_add(*lower, shift, basetype);
    if (instant)
      *upper = *lower;
    else
      *upper = datum_add(*upper, shift, basetype);
  }
  if (haswidth && ! instant)
  {
    /* Integer and date spans have exclusive upper bound */
    if (span_canon_basetype(basetype))
      width = datum_add(width, 1, basetype);
    *upper = datum_add(*lower, width, basetype);
  }
  return;
}

/**
 * @brief Shift and/or scale period bounds by two intervals
 * @param[in] shift Interval to shift the bounds, may be NULL
 * @param[in] duration Interval for the duration of the result, may be NULL
 * @param[in,out] lower,upper Bounds of the period
 */
void
span_bounds_shift_scale_time(const Interval *shift, const Interval *duration,
  TimestampTz *lower, TimestampTz *upper)
{
  assert(shift || duration); assert(lower); assert(upper);
  assert(! duration || positive_duration(duration));

  bool instant = (*lower == *upper);
  if (shift)
  {
    *lower = add_timestamptz_interval(*lower, (Interval *) shift);
    if (instant)
      *upper = *lower;
    else
      *upper = add_timestamptz_interval(*upper, (Interval *) shift);
  }
  if (duration && ! instant)
    *upper = add_timestamptz_interval(*lower, (Interval *) duration);
  return;
}

/**
 * @brief Shift and/or scale a span by a delta and a scale (iterator function)
 */
void
numspan_delta_scale_iter(Span *sp, Datum origin, Datum delta, bool hasdelta,
  double scale)
{
  assert(sp);

  meosType type = sp->basetype;
  /* The default value when shift is not given is 0 */
  if (hasdelta)
  {
    sp->lower = datum_add(sp->lower, delta, type);
    sp->upper = datum_add(sp->upper, delta, type);
  }
  /* Shifted lower and upper */
  Datum lower = sp->lower;
  Datum upper = sp->upper;
  /* The default value when scale is not given is 1.0 */
  if (scale != 1.0)
  {
    /* The potential shift has been already taken care in the previous if */
    sp->lower = datum_add(origin, double_datum(
      datum_double(datum_sub(lower, origin, type), type) * scale, type), type);
    if (datum_eq(lower, upper, type))
      sp->upper = sp->lower;
    else
    {
      /* Integer spans have exclusive upper bound */
      Datum upper1 = span_decr_bound(sp->upper, sp->basetype);
      sp->upper = datum_add(origin,
        double_datum(
          datum_double(datum_sub(upper1, origin, type), type) * scale,
          type), type);
      /* Integer spans have exclusive upper bound */
      if (span_canon_basetype(type))
        sp->upper = datum_add(sp->upper, 1, type);
    }
  }
  return;
}

/**
 * @brief Shift and/or scale a timestamptz span by a delta and a scale
 */
void
tstzspan_delta_scale_iter(Span *sp, TimestampTz origin, TimestampTz delta,
  double scale)
{
  assert(sp);

  TimestampTz lower = DatumGetTimestampTz(sp->lower);
  TimestampTz upper = DatumGetTimestampTz(sp->upper);
  /* The default value when there is not shift is 0 */
  if (delta != 0)
  {
    sp->lower = TimestampTzGetDatum(lower + delta);
    sp->upper = TimestampTzGetDatum(upper + delta);
  }
  /* Shifted lower and upper */
  lower = DatumGetTimestampTz(sp->lower);
  upper = DatumGetTimestampTz(sp->upper);
  /* The default value when there is not scale is 1.0 */
  if (scale != 1.0)
  {
    /* The potential shift has been already taken care in the previous if */
    sp->lower = TimestampTzGetDatum(
      origin + (TimestampTz) ((lower - origin) * scale));
    if (lower == upper)
      sp->upper = sp->lower;
    else
      sp->upper = TimestampTzGetDatum(
        origin + (TimestampTz) ((upper - origin) * scale));
  }
  return;
}

/**
 * @brief Return a number span shifted and/or scaled by two values (iterator
 * function)
 * @param[in] sp Span
 * @param[in] shift Value for shifting the bounds
 * @param[in] width Width of the result
 * @param[in] hasshift True when the shift argument is given
 * @param[in] haswidth True when the width argument is given
 * @param[out] delta,scale Delta and scale of the transformation
 */
void
numspan_shift_scale_iter(Span *sp, Datum shift, Datum width, bool hasshift,
  bool haswidth, Datum *delta, double *scale)
{
  assert(sp); assert(delta); assert(scale);
  Datum lower = sp->lower;
  Datum upper = sp->upper;
  meosType type = sp->basetype;
  span_bounds_shift_scale_value(shift, width, type, hasshift, haswidth,
    &lower, &upper);
  /* Compute delta and scale before overwriting sp->lower and sp->upper */
  *delta = 0;   /* Default value when shift is not given */
  *scale = 1.0; /* Default value when width is not given */
  if (hasshift)
    *delta = datum_sub(lower, sp->lower, type);
  /* If the period is instantaneous we cannot scale */
  if (haswidth && ! datum_eq(sp->lower, sp->upper, type))
  {
    /* Integer spans have exclusive upper bound */
    Datum upper1, upper2;
    if (span_canon_basetype(type))
    {
      upper1 = datum_sub(upper, 1, type);
      upper2 = datum_sub(sp->upper, 1, type);
    }
    else
    {
      upper1 = upper;
      upper2 = sp->upper;
    }
    *scale = datum_double(datum_sub(upper1, lower, type), type) /
      datum_double(datum_sub(upper2, sp->lower, type), type);
  }
  sp->lower = lower;
  sp->upper = upper;
  return;
}

/**
 * @brief Return a timestamptz span shifted and/or scaled by two intervals
 * @note Return the delta and scale of the transformation
 */
void
tstzspan_shift_scale1(Span *sp, const Interval *shift, const Interval *duration,
  TimestampTz *delta, double *scale)
{
  assert(sp); assert(delta); assert(scale);
  TimestampTz lower = DatumGetTimestampTz(sp->lower);
  TimestampTz upper = DatumGetTimestampTz(sp->upper);
  span_bounds_shift_scale_time(shift, duration, &lower, &upper);
  /* Compute delta and scale before overwriting sp->lower and sp->upper */
  *delta = 0;   /* Default value when shift == NULL */
  *scale = 1.0; /* Default value when duration == NULL */
  if (shift)
    *delta = lower - DatumGetTimestampTz(sp->lower);
  /* If the period is instantaneous we cannot scale */
  if (duration && sp->lower != sp->upper)
    *scale = (double) (upper - lower) /
      (double) (DatumGetTimestampTz(sp->upper) - DatumGetTimestampTz(sp->lower));
  sp->lower = TimestampTzGetDatum(lower);
  sp->upper = TimestampTzGetDatum(upper);
  return;
}

/**
 * @ingroup meos_internal_setspan_transf
 * @brief Return a number span shifted and/or scaled by two values
 * @param[in] sp Span
 * @param[in] shift Value for shifting the bounds
 * @param[in] width Width of the result
 * @param[in] hasshift True when the shift argument is given
 * @param[in] haswidth True when the width argument is given
 * @csqlfn #Numspan_shift(), #Numspan_scale(), #Numspan_shift_scale()
 */
Span *
numspan_shift_scale(const Span *sp, Datum shift, Datum width, bool hasshift,
  bool haswidth)
{
  /* Ensure the validity of the arguments */
  VALIDATE_NOT_NULL(sp, NULL);
  if (! ensure_one_true(hasshift, haswidth) ||
      (haswidth && ! ensure_positive_datum(width, sp->basetype)))
    return NULL;

  /* Copy the input span to the result */
  Span *result = span_copy(sp);
  /* Shift and/or scale the resulting span */
  span_bounds_shift_scale_value(shift, width, sp->basetype, hasshift, haswidth,
    &result->lower, &result->upper);
  return result;
}

/**
 * @ingroup meos_base_timestamp
 * @brief Return a timestamptz shifted by an interval
 * @param[in] t Timestamp
 * @param[in] interv Interval to shift the instant
 * @return On error return `DT_NOEND`
 * @csqlfn #Timestamptz_shift()
 */
TimestampTz
timestamptz_shift(TimestampTz t, const Interval *interv)
{
  /* Ensure the validity of the arguments */
  VALIDATE_NOT_NULL(interv, DT_NOEND);
  return add_timestamptz_interval(t, (Interval *) interv);
}

/**
 * @ingroup meos_setspan_transf
 * @brief Return a timestamptz span shifted and/or scaled by two intervals
 * @param[in] sp Span
 * @param[in] shift Interval to shift the bounds, may be NULL
 * @param[in] duration Duation of the result, may be NULL
 * @csqlfn #Tstzspan_shift(), #Tstzspan_scale(), #Tstzspan_shift_scale()
 */
Span *
tstzspan_shift_scale(const Span *sp, const Interval *shift,
  const Interval *duration)
{
  /* Ensure the validity of the arguments */
  VALIDATE_TSTZSPAN(sp, NULL);
  if (! ensure_one_not_null((void *) shift, (void *) duration) ||
      (duration && ! ensure_positive_duration(duration)))
    return NULL;

  /* Copy the input period to the result */
  Span *result = span_copy(sp);
  /* Shift and/or scale the resulting period */
  TimestampTz lower = DatumGetTimestampTz(sp->lower);
  TimestampTz upper = DatumGetTimestampTz(sp->upper);
  span_bounds_shift_scale_time(shift, duration, &lower, &upper);
  result->lower = TimestampTzGetDatum(lower);
  result->upper = TimestampTzGetDatum(upper);
  return result;
}

/*****************************************************************************
 * Spans function
 *****************************************************************************/

/**
 * @ingroup meos_setspan_bbox_split
 * @brief Return an array of spans from the values of a set
 * @param[in] s Set
 * @return On error return @p NULL
 * @csqlfn #Set_spans()
 */
Span *
set_spans(const Set *s)
{
  /* Ensure the validity of the arguments */
  VALIDATE_NOT_NULL(s, NULL);
  /* Output the composing spans */
  Span *result = palloc(sizeof(Span) * s->count);
  for (int i = 0; i < s->count; i++)
    set_set_subspan(s, i, i, &result[i]);
  return result;
}

/**
 * @ingroup meos_setspan_bbox_split
 * @brief Return an array of N spans from the values of a set
 * @param[in] s Set
 * @param[in] span_count Number of spans
 * @param[out] count Number of elements in the output array
 * @return On error return @p NULL
 * @csqlfn #Set_split_n_spans()
 */
Span *
set_split_n_spans(const Set *s, int span_count, int *count)
{
  /* Ensure the validity of the arguments */
  VALIDATE_NUMSET(s, NULL); VALIDATE_NOT_NULL(count, NULL);
  if (! ensure_positive(span_count))
    return NULL;

  Span *result = palloc(sizeof(Span) * s->count);
  /* Output the composing spans */
  if (s->count <= span_count)
  {
    for (int i = 0; i < s->count; i++)
      set_set_subspan(s, i, i, &result[i]);
    *count = s->count;
    return result;
  }

  /* Merge consecutive values to reach the maximum number of span */
  /* Minimum number of values merged together in an output span */
  int size = s->count / span_count;
  /* Number of output spans that result from merging (size + 1) values */
  int remainder = s->count % span_count;
  int i = 0; /* Loop variable for input values */
  for (int k = 0; k < span_count; k++)
  {
    int j = i + size;
    if (k < remainder)
      j++;
    set_set_subspan(s, i, j - 1, &result[k]);
    i = j;
  }
  assert(i == s->count);
  *count = span_count;
  return result;
}

/**
 * @ingroup meos_setspan_bbox_split
 * @brief Return an array of spans from a set obtained by merging consecutive
 * elements
 * @param[in] s Set
 * @param[in] elems_per_span Number of elements merged into an ouput span
 * @param[out] count Number of elements in the output array
 * @return On error return @p NULL
 * @csqlfn #Set_split_each_n_spans()
 */
Span *
set_split_each_n_spans(const Set *s, int elems_per_span, int *count)
{
  /* Ensure the validity of the arguments */
  VALIDATE_NUMSET(s, NULL); VALIDATE_NOT_NULL(count, NULL);
  if (! ensure_positive(elems_per_span))
    return NULL;

  int nspans = ceil((double) s->count / (double) elems_per_span);
  Span *result = palloc(sizeof(Span) * nspans);
  int k = 0;
  for (int i = 0; i < s->count; ++i)
  {
    if (i % elems_per_span == 0)
      value_set_span(SET_VAL_N(s, i), s->basetype, &result[k++]);
    else
    {
      Span span;
      value_set_span(SET_VAL_N(s, i), s->basetype, &span);
      span_expand(&span, &result[k - 1]);
    }
  }
  assert(k == nspans);
  *count = k;
  return result;
}

/*****************************************************************************
 * Comparison functions for defining B-tree indexes
 *****************************************************************************/

/**
 * @ingroup meos_setspan_comp
 * @brief Return true if two spans are equal
 * @note The function #span_cmp() is not used to increase efficiency
 * @param[in] sp1,sp2 Sets
 * @csqlfn #Span_eq()
 */
bool
span_eq(const Span *sp1, const Span *sp2)
{
  /* Ensure the validity of the arguments */
  if (! ensure_valid_span_span(sp1, sp2))
    return false;

  if (sp1->lower != sp2->lower || sp1->upper != sp2->upper ||
    sp1->lower_inc != sp2->lower_inc || sp1->upper_inc != sp2->upper_inc)
    return false;
  return true;
}

/**
 * @ingroup meos_setspan_comp
 * @brief Return true if the first span is different from the second one
 * @param[in] sp1,sp2 Sets
 * @csqlfn #Span_ne()
 */
inline bool
span_ne(const Span *sp1, const Span *sp2)
{
  return (! span_eq(sp1, sp2));
}

/**
 * @ingroup meos_setspan_comp
 * @brief Return -1, 0, or 1 depending on whether the first span is less than,
 * equal to, or greater than the second one
 * @param[in] sp1,sp2 Sets
 * @return On error return INT_MAX
 * @note Function used for B-tree comparison
 * @csqlfn #Span_cmp()
 */
int
span_cmp(const Span *sp1, const Span *sp2)
{
  /* Ensure the validity of the arguments */
  if (! ensure_valid_span_span(sp1, sp2))
    return INT_MAX;

  int cmp = datum_cmp(sp1->lower, sp2->lower, sp1->basetype);
  if (cmp != 0)
    return cmp;
  if (sp1->lower_inc != sp2->lower_inc)
    return sp1->lower_inc ? -1 : 1;
  cmp = datum_cmp(sp1->upper, sp2->upper, sp1->basetype);
  if (cmp != 0)
    return cmp;
  if (sp1->upper_inc != sp2->upper_inc)
    return sp1->upper_inc ? 1 : -1;
  return 0;
}

/* Inequality operators using the span_cmp function */

/**
 * @ingroup meos_setspan_comp
 * @brief Return true if the first span is less than the second one
 * @param[in] sp1,sp2 Sets
 * @csqlfn #Span_lt()
 */
inline bool
span_lt(const Span *sp1, const Span *sp2)
{
  return span_cmp(sp1, sp2) < 0;
}

/**
 * @ingroup meos_setspan_comp
 * @brief Return true if the first span is less than or equal to the second one
 * @param[in] sp1,sp2 Sets
 * @csqlfn #Span_le()
 */
inline bool
span_le(const Span *sp1, const Span *sp2)
{
  return span_cmp(sp1, sp2) <= 0;
}

/**
 * @ingroup meos_setspan_comp
 * @brief Return true if the first span is greater than or equal to the second
 * one
 * @param[in] sp1,sp2 Sets
 * @csqlfn #Span_gt()
 */
inline bool
span_ge(const Span *sp1, const Span *sp2)
{
  return span_cmp(sp1, sp2) >= 0;
}

/**
 * @ingroup meos_setspan_comp
 * @brief Return true if the first span is greater than the second one
 * @param[in] sp1,sp2 Sets
 * @csqlfn #Span_ge()
 */
inline bool
span_gt(const Span *sp1, const Span *sp2)
{
  return span_cmp(sp1, sp2) > 0;
}

/*****************************************************************************
 * Functions for defining hash indexes
 *****************************************************************************/

/**
 * @ingroup meos_setspan_accessor
 * @brief Return the 32-bit hash of a span
 * @param[in] sp Span
 * @return On error return @p INT_MAX
 * @csqlfn #Span_hash()
 */
uint32_t
span_hash(const Span *sp)
{
  /* Ensure the validity of the arguments */
  VALIDATE_NOT_NULL(sp, INT_MAX);

  /* Create flags from the lower_inc and upper_inc values */
  char flags = '\0';
  if (sp->lower_inc)
    flags |= 0x01;
  if (sp->upper_inc)
    flags |= 0x02;

  /* Create type from the spantype and basetype values */
  uint16 type = ((uint16) (sp->spantype) << 8) | (uint16) (sp->basetype);
  uint32_t type_hash = hash_bytes_uint32((int32_t) type);

  /* Apply the hash function to each bound */
  uint32_t lower_hash = datum_hash(sp->lower, sp->basetype);
  uint32_t upper_hash = datum_hash(sp->upper, sp->basetype);

  /* Merge hashes of flags, type, and bounds */
  uint32_t result = hash_bytes_uint32((uint32_t) flags);
  result ^= type_hash;
#if POSTGRESQL_VERSION_NUMBER >= 150000
  result = pg_rotate_left32(result, 1);
#else
  result =  (result << 1) | (result >> 31);
#endif
  result ^= lower_hash;
#if POSTGRESQL_VERSION_NUMBER >= 150000
  result = pg_rotate_left32(result, 1);
#else
  result =  (result << 1) | (result >> 31);
#endif
  result ^= upper_hash;

  return result;
}

/**
 * @ingroup meos_setspan_accessor
 * @brief Return the 64-bit hash of a span using a seed
 * @param[in] sp Span
 * @param[in] seed Seed
 * @return On error return @p LONG_MAX
 * @csqlfn #Span_hash_extended()
 */
uint64_t
span_hash_extended(const Span *sp, uint64_t seed)
{
  /* Ensure the validity of the arguments */
  VALIDATE_NOT_NULL(sp, LONG_MAX);

  char flags = '\0';
  /* Create flags from the lower_inc and upper_inc values */
  if (sp->lower_inc)
    flags |= 0x01;
  if (sp->upper_inc)
    flags |= 0x02;

  /* Create type from the spantype and basetype values */
  uint16 type = ((uint16) (sp->spantype) << 8) | (uint16) (sp->basetype);
  uint64_t type_hash = hash_uint32_extended((uint32_t) type, seed);

  /* Apply the hash function to each bound */
  uint64_t lower_hash = int64_hash_extended(sp->lower, seed);
  uint64_t upper_hash = int64_hash_extended(sp->upper, seed);

  /* Merge hashes of flags and bounds */
  uint64_t result = hash_uint32_extended((uint32_t) flags, seed);
  result ^= type_hash;
  result = ROTATE_HIGH_AND_LOW_32BITS(result);
  result ^= lower_hash;
  result = ROTATE_HIGH_AND_LOW_32BITS(result);
  result ^= upper_hash;

  return result;
}

/******************************************************************************/
