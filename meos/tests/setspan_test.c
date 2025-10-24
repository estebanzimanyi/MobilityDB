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
 * @brief A simple program that tests the set and span functions exposed by the
 * PostgreSQL types embedded in MEOS.
 *
 * The program can be build as follows
 * @code
 * gcc -Wall -g -I/usr/local/include -o setspan_test setspan_test.c -L/usr/local/lib -lmeos
 * @endcode
 */

#include <stdio.h>
#include <stdlib.h>
#include <meos.h>
#include <pg_bool.h>
#include <pg_text.h>

/* Main program */
int main(void)
{
  /* Initialize MEOS */
  meos_initialize();
  meos_initialize_timezone("UTC");

  /* Create values to test the functions of the API */
  bool b1 = bool_in("true");
  char *b1_out = bool_out(b1);
  int32 in1 = 32;
  int64 int64_in1 = 64;
  text *text1 = text_in("abcdef");
  text *text2 = text_in("ghijkl");
  char *text1_out = text_out(text1);
  char *text2_out = text_out(text2);

  /* Create the result types for the functions of the API */
  bool bool_result;
  int32 int32_result;
  uint32 uint32_result;
  uint64 uint64_result;
  char *char_result;
  text *text_result;
  
  /* Execute and print the result for the functions of the API */

  printf("****************************************************************\n");
  printf("* Set and span types *\n");
  printf("****************************************************************\n");

  /*****************************************************************************
   * Input/output functions for set and span types
   *****************************************************************************/

  /* Set *bigintset_in(const char *str); */
  set_result = bigintset_in(const char *str);

  /* char *bigintset_out(const Set *set); */
  char_result = bigintset_out(const Set *set);

  /* Span *bigintspan_in(const char *str); */
  span_result = bigintspan_in(const char *str);

  /* char *bigintspan_out(const Span *s); */
  char_result = bigintspan_out(const Span *s);

  /* SpanSet *bigintspanset_in(const char *str); */
  spanset_result = bigintspanset_in(const char *str);

  /* char *bigintspanset_out(const SpanSet *ss); */
  char_result = bigintspanset_out(const SpanSet *ss);

  /* Set *dateset_in(const char *str); */
  set_result = dateset_in(const char *str);

  /* char *dateset_out(const Set *s); */
  char_result = dateset_out(const Set *s);

  /* Span *datespan_in(const char *str); */
  span_result = datespan_in(const char *str);

  /* char *datespan_out(const Span *s); */
  char_result = datespan_out(const Span *s);

  /* SpanSet *datespanset_in(const char *str); */
  spanset_result = datespanset_in(const char *str);

  /* char *datespanset_out(const SpanSet *ss); */
  char_result = datespanset_out(const SpanSet *ss);

  /* Set *floatset_in(const char *str); */
  set_result = floatset_in(const char *str);

  /* char *floatset_out(const Set *set, int maxdd); */
  char_result = floatset_out(const Set *set, int maxdd);

  /* Span *floatspan_in(const char *str); */
  span_result = floatspan_in(const char *str);

  /* char *floatspan_out(const Span *s, int maxdd); */
  char_result = floatspan_out(const Span *s, int maxdd);

  /* SpanSet *floatspanset_in(const char *str); */
  spanset_result = floatspanset_in(const char *str);

  /* char *floatspanset_out(const SpanSet *ss, int maxdd); */
  char_result = floatspanset_out(const SpanSet *ss, int maxdd);

  /* Set *intset_in(const char *str); */
  set_result = intset_in(const char *str);

  /* char *intset_out(const Set *set); */
  char_result = intset_out(const Set *set);

  /* Span *intspan_in(const char *str); */
  span_result = intspan_in(const char *str);

  /* char *intspan_out(const Span *s); */
  char_result = intspan_out(const Span *s);

  /* SpanSet *intspanset_in(const char *str); */
  spanset_result = intspanset_in(const char *str);

  /* char *intspanset_out(const SpanSet *ss); */
  char_result = intspanset_out(const SpanSet *ss);

  /* char *set_as_hexwkb(const Set *s, uint8_t variant, size_t *size_out); */
  char_result = set_as_hexwkb(const Set *s, uint8_t variant, size_t *size_out);

  /* uint8_t *set_as_wkb(const Set *s, uint8_t variant, size_t *size_out); */
  binchar_result = set_as_wkb(const Set *s, uint8_t variant, size_t *size_out);

  /* Set *set_from_hexwkb(const char *hexwkb); */
  set_result = set_from_hexwkb(const char *hexwkb);

  /* Set *set_from_wkb(const uint8_t *wkb, size_t size); */
  set_result = set_from_wkb(const uint8_t *wkb, size_t size);

  /* char *span_as_hexwkb(const Span *s, uint8_t variant, size_t *size_out); */
  char_result = span_as_hexwkb(const Span *s, uint8_t variant, size_t *size_out);

  /* uint8_t *span_as_wkb(const Span *s, uint8_t variant, size_t *size_out); */
  binchar_result = span_as_wkb(const Span *s, uint8_t variant, size_t *size_out);

  /* Span *span_from_hexwkb(const char *hexwkb); */
  span_result = span_from_hexwkb(const char *hexwkb);

  /* Span *span_from_wkb(const uint8_t *wkb, size_t size); */
  span_result = span_from_wkb(const uint8_t *wkb, size_t size);

  /* char *spanset_as_hexwkb(const SpanSet *ss, uint8_t variant, size_t *size_out); */
  char_result = spanset_as_hexwkb(const SpanSet *ss, uint8_t variant, size_t *size_out);

  /* uint8_t *spanset_as_wkb(const SpanSet *ss, uint8_t variant, size_t *size_out); */
  binchar_result = spanset_as_wkb(const SpanSet *ss, uint8_t variant, size_t *size_out);

  /* SpanSet *spanset_from_hexwkb(const char *hexwkb); */
  spanset_result = spanset_from_hexwkb(const char *hexwkb);

  /* SpanSet *spanset_from_wkb(const uint8_t *wkb, size_t size); */
  spanset_result = spanset_from_wkb(const uint8_t *wkb, size_t size);

  /* Set *textset_in(const char *str); */
  set_result = textset_in(const char *str);

  /* char *textset_out(const Set *set); */
  char_result = textset_out(const Set *set);

  /* Set *tstzset_in(const char *str); */
  set_result = tstzset_in(const char *str);

  /* char *tstzset_out(const Set *set); */
  char_result = tstzset_out(const Set *set);

  /* Span *tstzspan_in(const char *str); */
  span_result = tstzspan_in(const char *str);

  /* char *tstzspan_out(const Span *s); */
  char_result = tstzspan_out(const Span *s);

  /* SpanSet *tstzspanset_in(const char *str); */
  spanset_result = tstzspanset_in(const char *str);

  /* char *tstzspanset_out(const SpanSet *ss); */
  char_result = tstzspanset_out(const SpanSet *ss);

  /*****************************************************************************
   * Constructor functions for set and span types
   *****************************************************************************/

  /* Set *bigintset_make(const int64 *values, int count); */
  set_result = bigintset_make(const int64 *values, int count);

  /* Span *bigintspan_make(int64 lower, int64 upper, bool lower_inc, bool upper_inc); */
  span_result = bigintspan_make(int64 lower, int64 upper, bool lower_inc, bool upper_inc);

  /* Set *dateset_make(const DateADT *values, int count); */
  set_result = dateset_make(const DateADT *values, int count);

  /* Span *datespan_make(DateADT lower, DateADT upper, bool lower_inc, bool upper_inc); */
  span_result = datespan_make(DateADT lower, DateADT upper, bool lower_inc, bool upper_inc);

  /* Set *floatset_make(const double *values, int count); */
  set_result = floatset_make(const double *values, int count);

  /* Span *floatspan_make(double lower, double upper, bool lower_inc, bool upper_inc); */
  span_result = floatspan_make(double lower, double upper, bool lower_inc, bool upper_inc);

  /* Set *intset_make(const int *values, int count); */
  set_result = intset_make(const int *values, int count);

  /* Span *intspan_make(int lower, int upper, bool lower_inc, bool upper_inc); */
  span_result = intspan_make(int lower, int upper, bool lower_inc, bool upper_inc);

  /* Set *set_copy(const Set *s); */
  set_result = set_copy(const Set *s);

  /* Span *span_copy(const Span *s); */
  span_result = span_copy(const Span *s);

  /* SpanSet *spanset_copy(const SpanSet *ss); */
  spanset_result = spanset_copy(const SpanSet *ss);

  /* SpanSet *spanset_make(Span *spans, int count); */
  spanset_result = spanset_make(Span *spans, int count);

  /* Set *textset_make(const text **values, int count); */
  set_result = textset_make(const text **values, int count);

  /* Set *tstzset_make(const TimestampTz *values, int count); */
  set_result = tstzset_make(const TimestampTz *values, int count);

  /* Span *tstzspan_make(TimestampTz lower, TimestampTz upper, bool lower_inc, bool upper_inc); */
  span_result = tstzspan_make(TimestampTz lower, TimestampTz upper, bool lower_inc, bool upper_inc);

  /*****************************************************************************
   * Conversion functions for set and span types
   *****************************************************************************/

  /* Set *bigint_to_set(int64 i); */
  set_result = bigint_to_set(int64 i);

  /* Span *bigint_to_span(int i); */
  span_result = bigint_to_span(int i);

  /* SpanSet *bigint_to_spanset(int i); */
  spanset_result = bigint_to_spanset(int i);

  /* Set *date_to_set(DateADT d); */
  set_result = date_to_set(DateADT d);

  /* Span *date_to_span(DateADT d); */
  span_result = date_to_span(DateADT d);

  /* SpanSet *date_to_spanset(DateADT d); */
  spanset_result = date_to_spanset(DateADT d);

  /* Set *dateset_to_tstzset(const Set *s); */
  set_result = dateset_to_tstzset(const Set *s);

  /* Span *datespan_to_tstzspan(const Span *s); */
  span_result = datespan_to_tstzspan(const Span *s);

  /* SpanSet *datespanset_to_tstzspanset(const SpanSet *ss); */
  spanset_result = datespanset_to_tstzspanset(const SpanSet *ss);

  /* Set *float_to_set(double d); */
  set_result = float_to_set(double d);

  /* Span *float_to_span(double d); */
  span_result = float_to_span(double d);

  /* SpanSet *float_to_spanset(double d); */
  spanset_result = float_to_spanset(double d);

  /* Set *floatset_to_intset(const Set *s); */
  set_result = floatset_to_intset(const Set *s);

  /* Span *floatspan_to_intspan(const Span *s); */
  span_result = floatspan_to_intspan(const Span *s);

  /* SpanSet *floatspanset_to_intspanset(const SpanSet *ss); */
  spanset_result = floatspanset_to_intspanset(const SpanSet *ss);

  /* Set *int_to_set(int i); */
  set_result = int_to_set(int i);

  /* Span *int_to_span(int i); */
  span_result = int_to_span(int i);

  /* SpanSet *int_to_spanset(int i); */
  spanset_result = int_to_spanset(int i);

  /* Set *intset_to_floatset(const Set *s); */
  set_result = intset_to_floatset(const Set *s);

  /* Span *intspan_to_floatspan(const Span *s); */
  span_result = intspan_to_floatspan(const Span *s);

  /* SpanSet *intspanset_to_floatspanset(const SpanSet *ss); */
  spanset_result = intspanset_to_floatspanset(const SpanSet *ss);

  /* Span *set_to_span(const Set *s); */
  span_result = set_to_span(const Set *s);

  /* SpanSet *set_to_spanset(const Set *s); */
  spanset_result = set_to_spanset(const Set *s);

  /* SpanSet *span_to_spanset(const Span *s); */
  spanset_result = span_to_spanset(const Span *s);

  /* Set *text_to_set(const text *txt); */
  set_result = text_to_set(const text *txt);

  /* Set *timestamptz_to_set(TimestampTz t); */
  set_result = timestamptz_to_set(TimestampTz t);

  /* Span *timestamptz_to_span(TimestampTz t); */
  span_result = timestamptz_to_span(TimestampTz t);

  /* SpanSet *timestamptz_to_spanset(TimestampTz t); */
  spanset_result = timestamptz_to_spanset(TimestampTz t);

  /* Set *tstzset_to_dateset(const Set *s); */
  set_result = tstzset_to_dateset(const Set *s);

  /* Span *tstzspan_to_datespan(const Span *s); */
  span_result = tstzspan_to_datespan(const Span *s);

  /* SpanSet *tstzspanset_to_datespanset(const SpanSet *ss); */
  spanset_result = tstzspanset_to_datespanset(const SpanSet *ss);

  /*****************************************************************************
   * Accessor functions for set and span types
   *****************************************************************************/

  /* int64 bigintset_end_value(const Set *s); */
  int64_result = bigintset_end_value(const Set *s);

  /* int64 bigintset_start_value(const Set *s); */
  int64_result = bigintset_start_value(const Set *s);

  /* bool bigintset_value_n(const Set *s, int n, int64 *result); */
  bool_result = bigintset_value_n(const Set *s, int n, int64 *result);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* int64 *bigintset_values(const Set *s); */
  int64_result = *bigintset_values(const Set *s);

  /* int64 bigintspan_lower(const Span *s); */
  int64_result = bigintspan_lower(const Span *s);

  /* int64 bigintspan_upper(const Span *s); */
  int64_result = bigintspan_upper(const Span *s);

  /* int64 bigintspan_width(const Span *s); */
  int64_result = bigintspan_width(const Span *s);

  /* int64 bigintspanset_lower(const SpanSet *ss); */
  int64_result = bigintspanset_lower(const SpanSet *ss);

  /* int64 bigintspanset_upper(const SpanSet *ss); */
  int64_result = bigintspanset_upper(const SpanSet *ss);

  /* int64 bigintspanset_width(const SpanSet *ss, bool boundspan); */
  int64_result = bigintspanset_width(const SpanSet *ss, bool boundspan);

  /* DateADT dateset_end_value(const Set *s); */
  date_result = dateset_end_value(const Set *s);

  /* DateADT dateset_start_value(const Set *s); */
  date_result = dateset_start_value(const Set *s);

  /* bool dateset_value_n(const Set *s, int n, DateADT *result); */
  bool_result = dateset_value_n(const Set *s, int n, DateADT *result);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* DateADT *dateset_values(const Set *s); */
  date_result = *dateset_values(const Set *s);

  /* Interval *datespan_duration(const Span *s); */
  interval_result = datespan_duration(const Span *s);

  /* DateADT datespan_lower(const Span *s); */
  date_result = datespan_lower(const Span *s);

  /* DateADT datespan_upper(const Span *s); */
  date_result = datespan_upper(const Span *s);

  /* bool datespanset_date_n(const SpanSet *ss, int n, DateADT *result); */
  bool_result = datespanset_date_n(const SpanSet *ss, int n, DateADT *result);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* Set *datespanset_dates(const SpanSet *ss); */
  set_result = datespanset_dates(const SpanSet *ss);

  /* Interval *datespanset_duration(const SpanSet *ss, bool boundspan); */
  interval_result = datespanset_duration(const SpanSet *ss, bool boundspan);

  /* DateADT datespanset_end_date(const SpanSet *ss); */
  date_result = datespanset_end_date(const SpanSet *ss);

  /* int datespanset_num_dates(const SpanSet *ss); */
  int32_result = datespanset_num_dates(const SpanSet *ss);

  /* DateADT datespanset_start_date(const SpanSet *ss); */
  date_result = datespanset_start_date(const SpanSet *ss);

  /* double floatset_end_value(const Set *s); */
  double_result = floatset_end_value(const Set *s);

  /* double floatset_start_value(const Set *s); */
  double_result = floatset_start_value(const Set *s);

  /* bool floatset_value_n(const Set *s, int n, double *result); */
  bool_result = floatset_value_n(const Set *s, int n, double *result);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* double *floatset_values(const Set *s); */
  double_result = *floatset_values(const Set *s);

  /* double floatspan_lower(const Span *s); */
  double_result = floatspan_lower(const Span *s);

  /* double floatspan_upper(const Span *s); */
  double_result = floatspan_upper(const Span *s);

  /* double floatspan_width(const Span *s); */
  double_result = floatspan_width(const Span *s);

  /* double floatspanset_lower(const SpanSet *ss); */
  double_result = floatspanset_lower(const SpanSet *ss);

  /* double floatspanset_upper(const SpanSet *ss); */
  double_result = floatspanset_upper(const SpanSet *ss);

  /* double floatspanset_width(const SpanSet *ss, bool boundspan); */
  double_result = floatspanset_width(const SpanSet *ss, bool boundspan);

  /* int intset_end_value(const Set *s); */
  int32_result = intset_end_value(const Set *s);

  /* int intset_start_value(const Set *s); */
  int32_result = intset_start_value(const Set *s);

  /* bool intset_value_n(const Set *s, int n, int *result); */
  bool_result = intset_value_n(const Set *s, int n, int *result);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* int *intset_values(const Set *s); */
  int32_result = *intset_values(const Set *s);

  /* int intspan_lower(const Span *s); */
  int32_result = intspan_lower(const Span *s);

  /* int intspan_upper(const Span *s); */
  int32_result = intspan_upper(const Span *s);

  /* int intspan_width(const Span *s); */
  int32_result = intspan_width(const Span *s);

  /* int intspanset_lower(const SpanSet *ss); */
  int32_result = intspanset_lower(const SpanSet *ss);

  /* int intspanset_upper(const SpanSet *ss); */
  int32_result = intspanset_upper(const SpanSet *ss);

  /* int intspanset_width(const SpanSet *ss, bool boundspan); */
  int32_result = intspanset_width(const SpanSet *ss, bool boundspan);

  /* uint32 set_hash(const Set *s); */
  uint32 set_hash(const Set *s);

  /* uint64 set_hash_extended(const Set *s, uint64 seed); */
  uint64 set_hash_extended(const Set *s, uint64 seed);

  /* int set_num_values(const Set *s); */
  int32_result = set_num_values(const Set *s);

  /* uint32 span_hash(const Span *s); */
  uint32 span_hash(const Span *s);

  /* uint64 span_hash_extended(const Span *s, uint64 seed); */
  uint64 span_hash_extended(const Span *s, uint64 seed);

  /* bool span_lower_inc(const Span *s); */
  bool_result = span_lower_inc(const Span *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool span_upper_inc(const Span *s); */
  bool_result = span_upper_inc(const Span *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* Span *spanset_end_span(const SpanSet *ss); */
  span_result = spanset_end_span(const SpanSet *ss);

  /* uint32 spanset_hash(const SpanSet *ss); */
  uint32 spanset_hash(const SpanSet *ss);

  /* uint64 spanset_hash_extended(const SpanSet *ss, uint64 seed); */
  uint64 spanset_hash_extended(const SpanSet *ss, uint64 seed);

  /* bool spanset_lower_inc(const SpanSet *ss); */
  bool_result = spanset_lower_inc(const SpanSet *ss);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* int spanset_num_spans(const SpanSet *ss); */
  int32_result = spanset_num_spans(const SpanSet *ss);

  /* Span *spanset_span(const SpanSet *ss); */
  span_result = spanset_span(const SpanSet *ss);

  /* Span *spanset_span_n(const SpanSet *ss, int i); */
  span_result = spanset_span_n(const SpanSet *ss, int i);

  /* Span **spanset_spanarr(const SpanSet *ss); */
  span_result = *spanset_spanarr(const SpanSet *ss);

  /* Span *spanset_start_span(const SpanSet *ss); */
  span_result = spanset_start_span(const SpanSet *ss);

  /* bool spanset_upper_inc(const SpanSet *ss); */
  bool_result = spanset_upper_inc(const SpanSet *ss);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* text *textset_end_value(const Set *s); */
  text_result = textset_end_value(const Set *s);

  /* text *textset_start_value(const Set *s); */
  text_result = textset_start_value(const Set *s);

  /* bool textset_value_n(const Set *s, int n, text **result); */
  bool_result = textset_value_n(const Set *s, int n, text **result);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* text **textset_values(const Set *s); */
  textarr_result = textset_values(const Set *s);

  /* TimestampTz tstzset_end_value(const Set *s); */
  tstz_result = tstzset_end_value(const Set *s);

  /* TimestampTz tstzset_start_value(const Set *s); */
  tstz_result = tstzset_start_value(const Set *s);

  /* bool tstzset_value_n(const Set *s, int n, TimestampTz *result); */
  bool_result = tstzset_value_n(const Set *s, int n, TimestampTz *result);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* TimestampTz *tstzset_values(const Set *s); */
  tstzarr_result = tstzset_values(const Set *s);

  /* Interval *tstzspan_duration(const Span *s); */
  interval_result = tstzspan_duration(const Span *s);

  /* TimestampTz tstzspan_lower(const Span *s); */
  tstz_result = tstzspan_lower(const Span *s);

  /* TimestampTz tstzspan_upper(const Span *s); */
  tstz_result = tstzspan_upper(const Span *s);

  /* Interval *tstzspanset_duration(const SpanSet *ss, bool boundspan); */
  interval_result = tstzspanset_duration(const SpanSet *ss, bool boundspan);

  /* TimestampTz tstzspanset_end_timestamptz(const SpanSet *ss); */
  tstz_result = tstzspanset_end_timestamptz(const SpanSet *ss);

  /* TimestampTz tstzspanset_lower(const SpanSet *ss); */
  tstz_result = tstzspanset_lower(const SpanSet *ss);

  /* int tstzspanset_num_timestamps(const SpanSet *ss); */
  int32_result = tstzspanset_num_timestamps(const SpanSet *ss);

  /* TimestampTz tstzspanset_start_timestamptz(const SpanSet *ss); */
  tstz_result = tstzspanset_start_timestamptz(const SpanSet *ss);

  /* Set *tstzspanset_timestamps(const SpanSet *ss); */
  set_result = tstzspanset_timestamps(const SpanSet *ss);

  /* bool tstzspanset_timestamptz_n(const SpanSet *ss, int n, TimestampTz *result); */
  bool_result = tstzspanset_timestamptz_n(const SpanSet *ss, int n, TimestampTz *result);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* TimestampTz tstzspanset_upper(const SpanSet *ss); */
  tstz_result = tstzspanset_upper(const SpanSet *ss);

  /*****************************************************************************
   * Transformation functions for set and span types
   *****************************************************************************/

  /* Set *bigintset_shift_scale(const Set *s, int64 shift, int64 width, bool hasshift, bool haswidth); */
  set_result = bigintset_shift_scale(const Set *s, int64 shift, int64 width, bool hasshift, bool haswidth);

  /* Span *bigintspan_shift_scale(const Span *s, int64 shift, int64 width, bool hasshift, bool haswidth); */
  span_result = bigintspan_shift_scale(const Span *s, int64 shift, int64 width, bool hasshift, bool haswidth);

  /* SpanSet *bigintspanset_shift_scale(const SpanSet *ss, int64 shift, int64 width, bool hasshift, bool haswidth); */
  spanset_result = bigintspanset_shift_scale(const SpanSet *ss, int64 shift, int64 width, bool hasshift, bool haswidth);

  /* Set *dateset_shift_scale(const Set *s, int shift, int width, bool hasshift, bool haswidth); */
  set_result = dateset_shift_scale(const Set *s, int shift, int width, bool hasshift, bool haswidth);

  /* Span *datespan_shift_scale(const Span *s, int shift, int width, bool hasshift, bool haswidth); */
  span_result = datespan_shift_scale(const Span *s, int shift, int width, bool hasshift, bool haswidth);

  /* SpanSet *datespanset_shift_scale(const SpanSet *ss, int shift, int width, bool hasshift, bool haswidth); */
  spanset_result = datespanset_shift_scale(const SpanSet *ss, int shift, int width, bool hasshift, bool haswidth);

  /* Set *floatset_ceil(const Set *s); */
  set_result = floatset_ceil(const Set *s);

  /* Set *floatset_degrees(const Set *s, bool normalize); */
  set_result = floatset_degrees(const Set *s, bool normalize);

  /* Set *floatset_floor(const Set *s); */
  set_result = floatset_floor(const Set *s);

  /* Set *floatset_radians(const Set *s); */
  set_result = floatset_radians(const Set *s);

  /* Set *floatset_shift_scale(const Set *s, double shift, double width, bool hasshift, bool haswidth); */
  set_result = floatset_shift_scale(const Set *s, double shift, double width, bool hasshift, bool haswidth);

  /* Span *floatspan_ceil(const Span *s); */
  span_result = floatspan_ceil(const Span *s);

  /* Span *floatspan_degrees(const Span *s, bool normalize); */
  span_result = floatspan_degrees(const Span *s, bool normalize);

  /* Span *floatspan_floor(const Span *s); */
  span_result = floatspan_floor(const Span *s);

  /* Span *floatspan_radians(const Span *s); */
  span_result = floatspan_radians(const Span *s);

  /* Span *floatspan_round(const Span *s, int maxdd); */
  span_result = floatspan_round(const Span *s, int maxdd);

  /* Span *floatspan_shift_scale(const Span *s, double shift, double width, bool hasshift, bool haswidth); */
  span_result = floatspan_shift_scale(const Span *s, double shift, double width, bool hasshift, bool haswidth);

  /* SpanSet *floatspanset_ceil(const SpanSet *ss); */
  spanset_result = floatspanset_ceil(const SpanSet *ss);

  /* SpanSet *floatspanset_floor(const SpanSet *ss); */
  spanset_result = floatspanset_floor(const SpanSet *ss);

  /* SpanSet *floatspanset_degrees(const SpanSet *ss, bool normalize); */
  spanset_result = floatspanset_degrees(const SpanSet *ss, bool normalize);

  /* SpanSet *floatspanset_radians(const SpanSet *ss); */
  spanset_result = floatspanset_radians(const SpanSet *ss);

  /* SpanSet *floatspanset_round(const SpanSet *ss, int maxdd); */
  spanset_result = floatspanset_round(const SpanSet *ss, int maxdd);

  /* SpanSet *floatspanset_shift_scale(const SpanSet *ss, double shift, double width, bool hasshift, bool haswidth); */
  spanset_result = floatspanset_shift_scale(const SpanSet *ss, double shift, double width, bool hasshift, bool haswidth);

  /* Set *intset_shift_scale(const Set *s, int shift, int width, bool hasshift, bool haswidth); */
  set_result = intset_shift_scale(const Set *s, int shift, int width, bool hasshift, bool haswidth);

  /* Span *intspan_shift_scale(const Span *s, int shift, int width, bool hasshift, bool haswidth); */
  span_result = intspan_shift_scale(const Span *s, int shift, int width, bool hasshift, bool haswidth);

  /* SpanSet *intspanset_shift_scale(const SpanSet *ss, int shift, int width, bool hasshift, bool haswidth); */
  spanset_result = intspanset_shift_scale(const SpanSet *ss, int shift, int width, bool hasshift, bool haswidth);

  /* Span *numspan_expand(const Span *s, Datum value); */
  span_result = numspan_expand(const Span *s, Datum value);

  /* Span *tstzspan_expand(const Span *s, const Interval *interv); */
  span_result = tstzspan_expand(const Span *s, const Interval *interv);

  /* Set *set_round(const Set *s, int maxdd); */
  set_result = set_round(const Set *s, int maxdd);

  /* Set *textcat_text_textset(const text *txt, const Set *s); */
  set_result = textcat_text_textset(const text *txt, const Set *s);

  /* Set *textcat_textset_text(const Set *s, const text *txt); */
  set_result = textcat_textset_text(const Set *s, const text *txt);

  /* Set *textset_initcap(const Set *s); */
  set_result = textset_initcap(const Set *s);

  /* Set *textset_lower(const Set *s); */
  set_result = textset_lower(const Set *s);

  /* Set *textset_upper(const Set *s); */
  set_result = textset_upper(const Set *s);

  /* TimestampTz timestamptz_tprecision(TimestampTz t, const Interval *duration, TimestampTz torigin); */
  tstz_result = timestamptz_tprecision(TimestampTz t, const Interval *duration, TimestampTz torigin);

  /* Set *tstzset_shift_scale(const Set *s, const Interval *shift, const Interval *duration); */
  set_result = tstzset_shift_scale(const Set *s, const Interval *shift, const Interval *duration);

  /* Set *tstzset_tprecision(const Set *s, const Interval *duration, TimestampTz torigin); */
  set_result = tstzset_tprecision(const Set *s, const Interval *duration, TimestampTz torigin);

  /* Span *tstzspan_shift_scale(const Span *s, const Interval *shift, const Interval *duration); */
  span_result = tstzspan_shift_scale(const Span *s, const Interval *shift, const Interval *duration);

  /* Span *tstzspan_tprecision(const Span *s, const Interval *duration, TimestampTz torigin); */
  span_result = tstzspan_tprecision(const Span *s, const Interval *duration, TimestampTz torigin);

  /* SpanSet *tstzspanset_shift_scale(const SpanSet *ss, const Interval *shift, const Interval *duration); */
  spanset_result = tstzspanset_shift_scale(const SpanSet *ss, const Interval *shift, const Interval *duration);

  /* SpanSet *tstzspanset_tprecision(const SpanSet *ss, const Interval *duration, TimestampTz torigin); */
  spanset_result = tstzspanset_tprecision(const SpanSet *ss, const Interval *duration, TimestampTz torigin);

  /*****************************************************************************
   * Comparison functions for set and span types
   *****************************************************************************/

  /* int set_cmp(const Set *s1, const Set *s2); */
  int32_result = set_cmp(const Set *s1, const Set *s2);

  /* bool set_eq(const Set *s1, const Set *s2); */
  bool_result = set_eq(const Set *s1, const Set *s2);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool set_ge(const Set *s1, const Set *s2); */
  bool_result = set_ge(const Set *s1, const Set *s2);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool set_gt(const Set *s1, const Set *s2); */
  bool_result = set_gt(const Set *s1, const Set *s2);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool set_le(const Set *s1, const Set *s2); */
  bool_result = set_le(const Set *s1, const Set *s2);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool set_lt(const Set *s1, const Set *s2); */
  bool_result = set_lt(const Set *s1, const Set *s2);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool set_ne(const Set *s1, const Set *s2); */
  bool_result = set_ne(const Set *s1, const Set *s2);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* int span_cmp(const Span *s1, const Span *s2); */
  int32_result = span_cmp(const Span *s1, const Span *s2);

  /* bool span_eq(const Span *s1, const Span *s2); */
  bool_result = span_eq(const Span *s1, const Span *s2);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool span_ge(const Span *s1, const Span *s2); */
  bool_result = span_ge(const Span *s1, const Span *s2);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool span_gt(const Span *s1, const Span *s2); */
  bool_result = span_gt(const Span *s1, const Span *s2);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool span_le(const Span *s1, const Span *s2); */
  bool_result = span_le(const Span *s1, const Span *s2);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool span_lt(const Span *s1, const Span *s2); */
  bool_result = span_lt(const Span *s1, const Span *s2);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool span_ne(const Span *s1, const Span *s2); */
  bool_result = span_ne(const Span *s1, const Span *s2);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* int spanset_cmp(const SpanSet *ss1, const SpanSet *ss2); */
  int32_result = spanset_cmp(const SpanSet *ss1, const SpanSet *ss2);

  /* bool spanset_eq(const SpanSet *ss1, const SpanSet *ss2); */
  bool_result = spanset_eq(const SpanSet *ss1, const SpanSet *ss2);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool spanset_ge(const SpanSet *ss1, const SpanSet *ss2); */
  bool_result = spanset_ge(const SpanSet *ss1, const SpanSet *ss2);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool spanset_gt(const SpanSet *ss1, const SpanSet *ss2); */
  bool_result = spanset_gt(const SpanSet *ss1, const SpanSet *ss2);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool spanset_le(const SpanSet *ss1, const SpanSet *ss2); */
  bool_result = spanset_le(const SpanSet *ss1, const SpanSet *ss2);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool spanset_lt(const SpanSet *ss1, const SpanSet *ss2); */
  bool_result = spanset_lt(const SpanSet *ss1, const SpanSet *ss2);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool spanset_ne(const SpanSet *ss1, const SpanSet *ss2); */
  bool_result = spanset_ne(const SpanSet *ss1, const SpanSet *ss2);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /*****************************************************************************
   * Bounding box functions for set and span types
   *****************************************************************************/

  /* Split functions */

  /* Span *set_spans(const Set *s); */
  span_result = set_spans(const Set *s);

  /* Span *set_split_each_n_spans(const Set *s, int elems_per_span, int *count); */
  span_result = set_split_each_n_spans(const Set *s, int elems_per_span, int *count);

  /* Span *set_split_n_spans(const Set *s, int span_count, int *count); */
  span_result = set_split_n_spans(const Set *s, int span_count, int *count);

  /* Span *spanset_spans(const SpanSet *ss); */
  span_result = spanset_spans(const SpanSet *ss);

  /* Span *spanset_split_each_n_spans(const SpanSet *ss, int elems_per_span, int *count); */
  span_result = spanset_split_each_n_spans(const SpanSet *ss, int elems_per_span, int *count);

  /* Span *spanset_split_n_spans(const SpanSet *ss, int span_count, int *count); */
  span_result = spanset_split_n_spans(const SpanSet *ss, int span_count, int *count);

  /* Topological functions */

  /* bool adjacent_span_bigint(const Span *s, int64 i); */
  bool_result = adjacent_span_bigint(const Span *s, int64 i);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool adjacent_span_date(const Span *s, DateADT d); */
  bool_result = adjacent_span_date(const Span *s, DateADT d);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool adjacent_span_float(const Span *s, double d); */
  bool_result = adjacent_span_float(const Span *s, double d);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool adjacent_span_int(const Span *s, int i); */
  bool_result = adjacent_span_int(const Span *s, int i);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool adjacent_span_span(const Span *s1, const Span *s2); */
  bool_result = adjacent_span_span(const Span *s1, const Span *s2);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool adjacent_span_spanset(const Span *s, const SpanSet *ss); */
  bool_result = adjacent_span_spanset(const Span *s, const SpanSet *ss);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool adjacent_span_timestamptz(const Span *s, TimestampTz t); */
  bool_result = adjacent_span_timestamptz(const Span *s, TimestampTz t);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool adjacent_spanset_bigint(const SpanSet *ss, int64 i); */
  bool_result = adjacent_spanset_bigint(const SpanSet *ss, int64 i);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool adjacent_spanset_date(const SpanSet *ss, DateADT d); */
  bool_result = adjacent_spanset_date(const SpanSet *ss, DateADT d);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool adjacent_spanset_float(const SpanSet *ss, double d); */
  bool_result = adjacent_spanset_float(const SpanSet *ss, double d);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool adjacent_spanset_int(const SpanSet *ss, int i); */
  bool_result = adjacent_spanset_int(const SpanSet *ss, int i);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool adjacent_spanset_timestamptz(const SpanSet *ss, TimestampTz t); */
  bool_result = adjacent_spanset_timestamptz(const SpanSet *ss, TimestampTz t);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool adjacent_spanset_span(const SpanSet *ss, const Span *s); */
  bool_result = adjacent_spanset_span(const SpanSet *ss, const Span *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool adjacent_spanset_spanset(const SpanSet *ss1, const SpanSet *ss2); */
  bool_result = adjacent_spanset_spanset(const SpanSet *ss1, const SpanSet *ss2);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool contained_bigint_set(int64 i, const Set *s); */
  bool_result = contained_bigint_set(int64 i, const Set *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool contained_bigint_span(int64 i, const Span *s); */
  bool_result = contained_bigint_span(int64 i, const Span *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool contained_bigint_spanset(int64 i, const SpanSet *ss); */
  bool_result = contained_bigint_spanset(int64 i, const SpanSet *ss);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool contained_date_set(DateADT d, const Set *s); */
  bool_result = contained_date_set(DateADT d, const Set *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool contained_date_span(DateADT d, const Span *s); */
  bool_result = contained_date_span(DateADT d, const Span *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool contained_date_spanset(DateADT d, const SpanSet *ss); */
  bool_result = contained_date_spanset(DateADT d, const SpanSet *ss);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool contained_float_set(double d, const Set *s); */
  bool_result = contained_float_set(double d, const Set *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool contained_float_span(double d, const Span *s); */
  bool_result = contained_float_span(double d, const Span *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool contained_float_spanset(double d, const SpanSet *ss); */
  bool_result = contained_float_spanset(double d, const SpanSet *ss);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool contained_int_set(int i, const Set *s); */
  bool_result = contained_int_set(int i, const Set *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool contained_int_span(int i, const Span *s); */
  bool_result = contained_int_span(int i, const Span *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool contained_int_spanset(int i, const SpanSet *ss); */
  bool_result = contained_int_spanset(int i, const SpanSet *ss);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool contained_set_set(const Set *s1, const Set *s2); */
  bool_result = contained_set_set(const Set *s1, const Set *s2);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool contained_span_span(const Span *s1, const Span *s2); */
  bool_result = contained_span_span(const Span *s1, const Span *s2);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool contained_span_spanset(const Span *s, const SpanSet *ss); */
  bool_result = contained_span_spanset(const Span *s, const SpanSet *ss);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool contained_spanset_span(const SpanSet *ss, const Span *s); */
  bool_result = contained_spanset_span(const SpanSet *ss, const Span *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool contained_spanset_spanset(const SpanSet *ss1, const SpanSet *ss2); */
  bool_result = contained_spanset_spanset(const SpanSet *ss1, const SpanSet *ss2);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool contained_text_set(const text *txt, const Set *s); */
  bool_result = contained_text_set(const text *txt, const Set *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool contained_timestamptz_set(TimestampTz t, const Set *s); */
  bool_result = contained_timestamptz_set(TimestampTz t, const Set *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool contained_timestamptz_span(TimestampTz t, const Span *s); */
  bool_result = contained_timestamptz_span(TimestampTz t, const Span *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool contained_timestamptz_spanset(TimestampTz t, const SpanSet *ss); */
  bool_result = contained_timestamptz_spanset(TimestampTz t, const SpanSet *ss);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool contains_set_bigint(const Set *s, int64 i); */
  bool_result = contains_set_bigint(const Set *s, int64 i);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool contains_set_date(const Set *s, DateADT d); */
  bool_result = contains_set_date(const Set *s, DateADT d);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool contains_set_float(const Set *s, double d); */
  bool_result = contains_set_float(const Set *s, double d);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool contains_set_int(const Set *s, int i); */
  bool_result = contains_set_int(const Set *s, int i);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool contains_set_set(const Set *s1, const Set *s2); */
  bool_result = contains_set_set(const Set *s1, const Set *s2);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool contains_set_text(const Set *s, text *t); */
  bool_result = contains_set_text(const Set *s, text *t);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool contains_set_timestamptz(const Set *s, TimestampTz t); */
  bool_result = contains_set_timestamptz(const Set *s, TimestampTz t);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool contains_span_bigint(const Span *s, int64 i); */
  bool_result = contains_span_bigint(const Span *s, int64 i);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool contains_span_date(const Span *s, DateADT d); */
  bool_result = contains_span_date(const Span *s, DateADT d);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool contains_span_float(const Span *s, double d); */
  bool_result = contains_span_float(const Span *s, double d);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool contains_span_int(const Span *s, int i); */
  bool_result = contains_span_int(const Span *s, int i);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool contains_span_span(const Span *s1, const Span *s2); */
  bool_result = contains_span_span(const Span *s1, const Span *s2);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool contains_span_spanset(const Span *s, const SpanSet *ss); */
  bool_result = contains_span_spanset(const Span *s, const SpanSet *ss);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool contains_span_timestamptz(const Span *s, TimestampTz t); */
  bool_result = contains_span_timestamptz(const Span *s, TimestampTz t);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool contains_spanset_bigint(const SpanSet *ss, int64 i); */
  bool_result = contains_spanset_bigint(const SpanSet *ss, int64 i);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool contains_spanset_date(const SpanSet *ss, DateADT d); */
  bool_result = contains_spanset_date(const SpanSet *ss, DateADT d);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool contains_spanset_float(const SpanSet *ss, double d); */
  bool_result = contains_spanset_float(const SpanSet *ss, double d);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool contains_spanset_int(const SpanSet *ss, int i); */
  bool_result = contains_spanset_int(const SpanSet *ss, int i);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool contains_spanset_span(const SpanSet *ss, const Span *s); */
  bool_result = contains_spanset_span(const SpanSet *ss, const Span *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool contains_spanset_spanset(const SpanSet *ss1, const SpanSet *ss2); */
  bool_result = contains_spanset_spanset(const SpanSet *ss1, const SpanSet *ss2);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool contains_spanset_timestamptz(const SpanSet *ss, TimestampTz t); */
  bool_result = contains_spanset_timestamptz(const SpanSet *ss, TimestampTz t);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overlaps_set_set(const Set *s1, const Set *s2); */
  bool_result = overlaps_set_set(const Set *s1, const Set *s2);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overlaps_span_span(const Span *s1, const Span *s2); */
  bool_result = overlaps_span_span(const Span *s1, const Span *s2);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overlaps_span_spanset(const Span *s, const SpanSet *ss); */
  bool_result = overlaps_span_spanset(const Span *s, const SpanSet *ss);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overlaps_spanset_span(const SpanSet *ss, const Span *s); */
  bool_result = overlaps_spanset_span(const SpanSet *ss, const Span *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overlaps_spanset_spanset(const SpanSet *ss1, const SpanSet *ss2); */
  bool_result = overlaps_spanset_spanset(const SpanSet *ss1, const SpanSet *ss2);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /*****************************************************************************/

  /* Position functions for set and span types */

  /* bool after_date_set(DateADT d, const Set *s); */
  bool_result = after_date_set(DateADT d, const Set *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool after_date_span(DateADT d, const Span *s); */
  bool_result = after_date_span(DateADT d, const Span *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool after_date_spanset(DateADT d, const SpanSet *ss); */
  bool_result = after_date_spanset(DateADT d, const SpanSet *ss);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool after_set_date(const Set *s, DateADT d); */
  bool_result = after_set_date(const Set *s, DateADT d);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool after_set_timestamptz(const Set *s, TimestampTz t); */
  bool_result = after_set_timestamptz(const Set *s, TimestampTz t);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool after_span_date(const Span *s, DateADT d); */
  bool_result = after_span_date(const Span *s, DateADT d);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool after_span_timestamptz(const Span *s, TimestampTz t); */
  bool_result = after_span_timestamptz(const Span *s, TimestampTz t);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool after_spanset_date(const SpanSet *ss, DateADT d); */
  bool_result = after_spanset_date(const SpanSet *ss, DateADT d);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool after_spanset_timestamptz(const SpanSet *ss, TimestampTz t); */
  bool_result = after_spanset_timestamptz(const SpanSet *ss, TimestampTz t);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool after_timestamptz_set(TimestampTz t, const Set *s); */
  bool_result = after_timestamptz_set(TimestampTz t, const Set *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool after_timestamptz_span(TimestampTz t, const Span *s); */
  bool_result = after_timestamptz_span(TimestampTz t, const Span *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool after_timestamptz_spanset(TimestampTz t, const SpanSet *ss); */
  bool_result = after_timestamptz_spanset(TimestampTz t, const SpanSet *ss);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool before_date_set(DateADT d, const Set *s); */
  bool_result = before_date_set(DateADT d, const Set *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool before_date_span(DateADT d, const Span *s); */
  bool_result = before_date_span(DateADT d, const Span *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool before_date_spanset(DateADT d, const SpanSet *ss); */
  bool_result = before_date_spanset(DateADT d, const SpanSet *ss);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool before_set_date(const Set *s, DateADT d); */
  bool_result = before_set_date(const Set *s, DateADT d);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool before_set_timestamptz(const Set *s, TimestampTz t); */
  bool_result = before_set_timestamptz(const Set *s, TimestampTz t);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool before_span_date(const Span *s, DateADT d); */
  bool_result = before_span_date(const Span *s, DateADT d);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool before_span_timestamptz(const Span *s, TimestampTz t); */
  bool_result = before_span_timestamptz(const Span *s, TimestampTz t);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool before_spanset_date(const SpanSet *ss, DateADT d); */
  bool_result = before_spanset_date(const SpanSet *ss, DateADT d);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool before_spanset_timestamptz(const SpanSet *ss, TimestampTz t); */
  bool_result = before_spanset_timestamptz(const SpanSet *ss, TimestampTz t);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool before_timestamptz_set(TimestampTz t, const Set *s); */
  bool_result = before_timestamptz_set(TimestampTz t, const Set *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool before_timestamptz_span(TimestampTz t, const Span *s); */
  bool_result = before_timestamptz_span(TimestampTz t, const Span *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool before_timestamptz_spanset(TimestampTz t, const SpanSet *ss); */
  bool_result = before_timestamptz_spanset(TimestampTz t, const SpanSet *ss);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool left_bigint_set(int64 i, const Set *s); */
  bool_result = left_bigint_set(int64 i, const Set *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool left_bigint_span(int64 i, const Span *s); */
  bool_result = left_bigint_span(int64 i, const Span *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool left_bigint_spanset(int64 i, const SpanSet *ss); */
  bool_result = left_bigint_spanset(int64 i, const SpanSet *ss);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool left_float_set(double d, const Set *s); */
  bool_result = left_float_set(double d, const Set *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool left_float_span(double d, const Span *s); */
  bool_result = left_float_span(double d, const Span *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool left_float_spanset(double d, const SpanSet *ss); */
  bool_result = left_float_spanset(double d, const SpanSet *ss);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool left_int_set(int i, const Set *s); */
  bool_result = left_int_set(int i, const Set *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool left_int_span(int i, const Span *s); */
  bool_result = left_int_span(int i, const Span *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool left_int_spanset(int i, const SpanSet *ss); */
  bool_result = left_int_spanset(int i, const SpanSet *ss);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool left_set_bigint(const Set *s, int64 i); */
  bool_result = left_set_bigint(const Set *s, int64 i);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool left_set_float(const Set *s, double d); */
  bool_result = left_set_float(const Set *s, double d);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool left_set_int(const Set *s, int i); */
  bool_result = left_set_int(const Set *s, int i);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool left_set_set(const Set *s1, const Set *s2); */
  bool_result = left_set_set(const Set *s1, const Set *s2);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool left_set_text(const Set *s, text *txt); */
  bool_result = left_set_text(const Set *s, text *txt);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool left_span_bigint(const Span *s, int64 i); */
  bool_result = left_span_bigint(const Span *s, int64 i);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool left_span_float(const Span *s, double d); */
  bool_result = left_span_float(const Span *s, double d);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool left_span_int(const Span *s, int i); */
  bool_result = left_span_int(const Span *s, int i);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool left_span_span(const Span *s1, const Span *s2); */
  bool_result = left_span_span(const Span *s1, const Span *s2);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool left_span_spanset(const Span *s, const SpanSet *ss); */
  bool_result = left_span_spanset(const Span *s, const SpanSet *ss);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool left_spanset_bigint(const SpanSet *ss, int64 i); */
  bool_result = left_spanset_bigint(const SpanSet *ss, int64 i);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool left_spanset_float(const SpanSet *ss, double d); */
  bool_result = left_spanset_float(const SpanSet *ss, double d);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool left_spanset_int(const SpanSet *ss, int i); */
  bool_result = left_spanset_int(const SpanSet *ss, int i);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool left_spanset_span(const SpanSet *ss, const Span *s); */
  bool_result = left_spanset_span(const SpanSet *ss, const Span *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool left_spanset_spanset(const SpanSet *ss1, const SpanSet *ss2); */
  bool_result = left_spanset_spanset(const SpanSet *ss1, const SpanSet *ss2);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool left_text_set(const text *txt, const Set *s); */
  bool_result = left_text_set(const text *txt, const Set *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overafter_date_set(DateADT d, const Set *s); */
  bool_result = overafter_date_set(DateADT d, const Set *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overafter_date_span(DateADT d, const Span *s); */
  bool_result = overafter_date_span(DateADT d, const Span *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overafter_date_spanset(DateADT d, const SpanSet *ss); */
  bool_result = overafter_date_spanset(DateADT d, const SpanSet *ss);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overafter_set_date(const Set *s, DateADT d); */
  bool_result = overafter_set_date(const Set *s, DateADT d);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overafter_set_timestamptz(const Set *s, TimestampTz t); */
  bool_result = overafter_set_timestamptz(const Set *s, TimestampTz t);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overafter_span_date(const Span *s, DateADT d); */
  bool_result = overafter_span_date(const Span *s, DateADT d);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overafter_span_timestamptz(const Span *s, TimestampTz t); */
  bool_result = overafter_span_timestamptz(const Span *s, TimestampTz t);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overafter_spanset_date(const SpanSet *ss, DateADT d); */
  bool_result = overafter_spanset_date(const SpanSet *ss, DateADT d);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overafter_spanset_timestamptz(const SpanSet *ss, TimestampTz t); */
  bool_result = overafter_spanset_timestamptz(const SpanSet *ss, TimestampTz t);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overafter_timestamptz_set(TimestampTz t, const Set *s); */
  bool_result = overafter_timestamptz_set(TimestampTz t, const Set *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overafter_timestamptz_span(TimestampTz t, const Span *s); */
  bool_result = overafter_timestamptz_span(TimestampTz t, const Span *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overafter_timestamptz_spanset(TimestampTz t, const SpanSet *ss); */
  bool_result = overafter_timestamptz_spanset(TimestampTz t, const SpanSet *ss);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overbefore_date_set(DateADT d, const Set *s); */
  bool_result = overbefore_date_set(DateADT d, const Set *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overbefore_date_span(DateADT d, const Span *s); */
  bool_result = overbefore_date_span(DateADT d, const Span *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overbefore_date_spanset(DateADT d, const SpanSet *ss); */
  bool_result = overbefore_date_spanset(DateADT d, const SpanSet *ss);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overbefore_set_date(const Set *s, DateADT d); */
  bool_result = overbefore_set_date(const Set *s, DateADT d);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overbefore_set_timestamptz(const Set *s, TimestampTz t); */
  bool_result = overbefore_set_timestamptz(const Set *s, TimestampTz t);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overbefore_span_date(const Span *s, DateADT d); */
  bool_result = overbefore_span_date(const Span *s, DateADT d);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overbefore_span_timestamptz(const Span *s, TimestampTz t); */
  bool_result = overbefore_span_timestamptz(const Span *s, TimestampTz t);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overbefore_spanset_date(const SpanSet *ss, DateADT d); */
  bool_result = overbefore_spanset_date(const SpanSet *ss, DateADT d);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overbefore_spanset_timestamptz(const SpanSet *ss, TimestampTz t); */
  bool_result = overbefore_spanset_timestamptz(const SpanSet *ss, TimestampTz t);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overbefore_timestamptz_set(TimestampTz t, const Set *s); */
  bool_result = overbefore_timestamptz_set(TimestampTz t, const Set *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overbefore_timestamptz_span(TimestampTz t, const Span *s); */
  bool_result = overbefore_timestamptz_span(TimestampTz t, const Span *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overbefore_timestamptz_spanset(TimestampTz t, const SpanSet *ss); */
  bool_result = overbefore_timestamptz_spanset(TimestampTz t, const SpanSet *ss);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overleft_bigint_set(int64 i, const Set *s); */
  bool_result = overleft_bigint_set(int64 i, const Set *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overleft_bigint_span(int64 i, const Span *s); */
  bool_result = overleft_bigint_span(int64 i, const Span *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overleft_bigint_spanset(int64 i, const SpanSet *ss); */
  bool_result = overleft_bigint_spanset(int64 i, const SpanSet *ss);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overleft_float_set(double d, const Set *s); */
  bool_result = overleft_float_set(double d, const Set *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overleft_float_span(double d, const Span *s); */
  bool_result = overleft_float_span(double d, const Span *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overleft_float_spanset(double d, const SpanSet *ss); */
  bool_result = overleft_float_spanset(double d, const SpanSet *ss);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overleft_int_set(int i, const Set *s); */
  bool_result = overleft_int_set(int i, const Set *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overleft_int_span(int i, const Span *s); */
  bool_result = overleft_int_span(int i, const Span *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overleft_int_spanset(int i, const SpanSet *ss); */
  bool_result = overleft_int_spanset(int i, const SpanSet *ss);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overleft_set_bigint(const Set *s, int64 i); */
  bool_result = overleft_set_bigint(const Set *s, int64 i);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overleft_set_float(const Set *s, double d); */
  bool_result = overleft_set_float(const Set *s, double d);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overleft_set_int(const Set *s, int i); */
  bool_result = overleft_set_int(const Set *s, int i);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overleft_set_set(const Set *s1, const Set *s2); */
  bool_result = overleft_set_set(const Set *s1, const Set *s2);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overleft_set_text(const Set *s, text *txt); */
  bool_result = overleft_set_text(const Set *s, text *txt);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overleft_span_bigint(const Span *s, int64 i); */
  bool_result = overleft_span_bigint(const Span *s, int64 i);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overleft_span_float(const Span *s, double d); */
  bool_result = overleft_span_float(const Span *s, double d);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overleft_span_int(const Span *s, int i); */
  bool_result = overleft_span_int(const Span *s, int i);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overleft_span_span(const Span *s1, const Span *s2); */
  bool_result = overleft_span_span(const Span *s1, const Span *s2);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overleft_span_spanset(const Span *s, const SpanSet *ss); */
  bool_result = overleft_span_spanset(const Span *s, const SpanSet *ss);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overleft_spanset_bigint(const SpanSet *ss, int64 i); */
  bool_result = overleft_spanset_bigint(const SpanSet *ss, int64 i);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overleft_spanset_float(const SpanSet *ss, double d); */
  bool_result = overleft_spanset_float(const SpanSet *ss, double d);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overleft_spanset_int(const SpanSet *ss, int i); */
  bool_result = overleft_spanset_int(const SpanSet *ss, int i);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overleft_spanset_span(const SpanSet *ss, const Span *s); */
  bool_result = overleft_spanset_span(const SpanSet *ss, const Span *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overleft_spanset_spanset(const SpanSet *ss1, const SpanSet *ss2); */
  bool_result = overleft_spanset_spanset(const SpanSet *ss1, const SpanSet *ss2);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overleft_text_set(const text *txt, const Set *s); */
  bool_result = overleft_text_set(const text *txt, const Set *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overright_bigint_set(int64 i, const Set *s); */
  bool_result = overright_bigint_set(int64 i, const Set *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overright_bigint_span(int64 i, const Span *s); */
  bool_result = overright_bigint_span(int64 i, const Span *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overright_bigint_spanset(int64 i, const SpanSet *ss); */
  bool_result = overright_bigint_spanset(int64 i, const SpanSet *ss);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overright_float_set(double d, const Set *s); */
  bool_result = overright_float_set(double d, const Set *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overright_float_span(double d, const Span *s); */
  bool_result = overright_float_span(double d, const Span *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overright_float_spanset(double d, const SpanSet *ss); */
  bool_result = overright_float_spanset(double d, const SpanSet *ss);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overright_int_set(int i, const Set *s); */
  bool_result = overright_int_set(int i, const Set *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overright_int_span(int i, const Span *s); */
  bool_result = overright_int_span(int i, const Span *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overright_int_spanset(int i, const SpanSet *ss); */
  bool_result = overright_int_spanset(int i, const SpanSet *ss);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overright_set_bigint(const Set *s, int64 i); */
  bool_result = overright_set_bigint(const Set *s, int64 i);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overright_set_float(const Set *s, double d); */
  bool_result = overright_set_float(const Set *s, double d);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overright_set_int(const Set *s, int i); */
  bool_result = overright_set_int(const Set *s, int i);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overright_set_set(const Set *s1, const Set *s2); */
  bool_result = overright_set_set(const Set *s1, const Set *s2);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overright_set_text(const Set *s, text *txt); */
  bool_result = overright_set_text(const Set *s, text *txt);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overright_span_bigint(const Span *s, int64 i); */
  bool_result = overright_span_bigint(const Span *s, int64 i);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overright_span_float(const Span *s, double d); */
  bool_result = overright_span_float(const Span *s, double d);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overright_span_int(const Span *s, int i); */
  bool_result = overright_span_int(const Span *s, int i);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overright_span_span(const Span *s1, const Span *s2); */
  bool_result = overright_span_span(const Span *s1, const Span *s2);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overright_span_spanset(const Span *s, const SpanSet *ss); */
  bool_result = overright_span_spanset(const Span *s, const SpanSet *ss);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overright_spanset_bigint(const SpanSet *ss, int64 i); */
  bool_result = overright_spanset_bigint(const SpanSet *ss, int64 i);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overright_spanset_float(const SpanSet *ss, double d); */
  bool_result = overright_spanset_float(const SpanSet *ss, double d);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overright_spanset_int(const SpanSet *ss, int i); */
  bool_result = overright_spanset_int(const SpanSet *ss, int i);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overright_spanset_span(const SpanSet *ss, const Span *s); */
  bool_result = overright_spanset_span(const SpanSet *ss, const Span *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overright_spanset_spanset(const SpanSet *ss1, const SpanSet *ss2); */
  bool_result = overright_spanset_spanset(const SpanSet *ss1, const SpanSet *ss2);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool overright_text_set(const text *txt, const Set *s); */
  bool_result = overright_text_set(const text *txt, const Set *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool right_bigint_set(int64 i, const Set *s); */
  bool_result = right_bigint_set(int64 i, const Set *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool right_bigint_span(int64 i, const Span *s); */
  bool_result = right_bigint_span(int64 i, const Span *s);

  /* bool right_bigint_spanset(int64 i, const SpanSet *ss); */
  bool_result = right_bigint_spanset(int64 i, const SpanSet *ss);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool right_float_set(double d, const Set *s); */
  bool_result = right_float_set(double d, const Set *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool right_float_span(double d, const Span *s); */
  bool_result = right_float_span(double d, const Span *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool right_float_spanset(double d, const SpanSet *ss); */
  bool_result = right_float_spanset(double d, const SpanSet *ss);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool right_int_set(int i, const Set *s); */
  bool_result = right_int_set(int i, const Set *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool right_int_span(int i, const Span *s); */
  bool_result = right_int_span(int i, const Span *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool right_int_spanset(int i, const SpanSet *ss); */
  bool_result = right_int_spanset(int i, const SpanSet *ss);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool right_set_bigint(const Set *s, int64 i); */
  bool_result = right_set_bigint(const Set *s, int64 i);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool right_set_float(const Set *s, double d); */
  bool_result = right_set_float(const Set *s, double d);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool right_set_int(const Set *s, int i); */
  bool_result = right_set_int(const Set *s, int i);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool right_set_set(const Set *s1, const Set *s2); */
  bool_result = right_set_set(const Set *s1, const Set *s2);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool right_set_text(const Set *s, text *txt); */
  bool_result = right_set_text(const Set *s, text *txt);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool right_span_bigint(const Span *s, int64 i); */
  bool_result = right_span_bigint(const Span *s, int64 i);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool right_span_float(const Span *s, double d); */
  bool_result = right_span_float(const Span *s, double d);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool right_span_int(const Span *s, int i); */
  bool_result = right_span_int(const Span *s, int i);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool right_span_span(const Span *s1, const Span *s2); */
  bool_result = right_span_span(const Span *s1, const Span *s2);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool right_span_spanset(const Span *s, const SpanSet *ss); */
  bool_result = right_span_spanset(const Span *s, const SpanSet *ss);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool right_spanset_bigint(const SpanSet *ss, int64 i); */
  bool_result = right_spanset_bigint(const SpanSet *ss, int64 i);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool right_spanset_float(const SpanSet *ss, double d); */
  bool_result = right_spanset_float(const SpanSet *ss, double d);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool right_spanset_int(const SpanSet *ss, int i); */
  bool_result = right_spanset_int(const SpanSet *ss, int i);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool right_spanset_span(const SpanSet *ss, const Span *s); */
  bool_result = right_spanset_span(const SpanSet *ss, const Span *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool right_spanset_spanset(const SpanSet *ss1, const SpanSet *ss2); */
  bool_result = right_spanset_spanset(const SpanSet *ss1, const SpanSet *ss2);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /* bool right_text_set(const text *txt, const Set *s); */
  bool_result = right_text_set(const text *txt, const Set *s);
  printf("xxx(%s, %s): %c\n", xxx, xxx, bool_result ? 't' : 'n');

  /*****************************************************************************
   * Set functions for set and span types
   *****************************************************************************/

  /* Set *intersection_bigint_set(int64 i, const Set *s); */
  set_result = intersection_bigint_set(int64 i, const Set *s);

  /* Set *intersection_date_set(DateADT d, const Set *s); */
  set_result = intersection_date_set(DateADT d, const Set *s);

  /* Set *intersection_float_set(double d, const Set *s); */
  set_result = intersection_float_set(double d, const Set *s);

  /* Set *intersection_int_set(int i, const Set *s); */
  set_result = intersection_int_set(int i, const Set *s);

  /* Set *intersection_set_bigint(const Set *s, int64 i); */
  set_result = intersection_set_bigint(const Set *s, int64 i);

  /* Set *intersection_set_date(const Set *s, DateADT d); */
  set_result = intersection_set_date(const Set *s, DateADT d);

  /* Set *intersection_set_float(const Set *s, double d); */
  set_result = intersection_set_float(const Set *s, double d);

  /* Set *intersection_set_int(const Set *s, int i); */
  set_result = intersection_set_int(const Set *s, int i);

  /* Set *intersection_set_set(const Set *s1, const Set *s2); */
  set_result = intersection_set_set(const Set *s1, const Set *s2);

  /* Set *intersection_set_text(const Set *s, const text *txt); */
  set_result = intersection_set_text(const Set *s, const text *txt);

  /* Set *intersection_set_timestamptz(const Set *s, TimestampTz t); */
  set_result = intersection_set_timestamptz(const Set *s, TimestampTz t);

  /* Span *intersection_span_bigint(const Span *s, int64 i); */
  span_result = intersection_span_bigint(const Span *s, int64 i);

  /* Span *intersection_span_date(const Span *s, DateADT d); */
  span_result = intersection_span_date(const Span *s, DateADT d);

  /* Span *intersection_span_float(const Span *s, double d); */
  span_result = intersection_span_float(const Span *s, double d);

  /* Span *intersection_span_int(const Span *s, int i); */
  span_result = intersection_span_int(const Span *s, int i);

  /* Span *intersection_span_span(const Span *s1, const Span *s2); */
  span_result = intersection_span_span(const Span *s1, const Span *s2);

  /* SpanSet *intersection_span_spanset(const Span *s, const SpanSet *ss); */
  spanset_result = intersection_span_spanset(const Span *s, const SpanSet *ss);

  /* Span *intersection_span_timestamptz(const Span *s, TimestampTz t); */
  span_result = intersection_span_timestamptz(const Span *s, TimestampTz t);

  /* SpanSet *intersection_spanset_bigint(const SpanSet *ss, int64 i); */
  spanset_result = intersection_spanset_bigint(const SpanSet *ss, int64 i);

  /* SpanSet *intersection_spanset_date(const SpanSet *ss, DateADT d); */
  spanset_result = intersection_spanset_date(const SpanSet *ss, DateADT d);

  /* SpanSet *intersection_spanset_float(const SpanSet *ss, double d); */
  spanset_result = intersection_spanset_float(const SpanSet *ss, double d);

  /* SpanSet *intersection_spanset_int(const SpanSet *ss, int i); */
  spanset_result = intersection_spanset_int(const SpanSet *ss, int i);

  /* SpanSet *intersection_spanset_span(const SpanSet *ss, const Span *s); */
  spanset_result = intersection_spanset_span(const SpanSet *ss, const Span *s);

  /* SpanSet *intersection_spanset_spanset(const SpanSet *ss1, const SpanSet *ss2); */
  spanset_result = intersection_spanset_spanset(const SpanSet *ss1, const SpanSet *ss2);

  /* SpanSet *intersection_spanset_timestamptz(const SpanSet *ss, TimestampTz t); */
  spanset_result = intersection_spanset_timestamptz(const SpanSet *ss, TimestampTz t);

  /* Set *intersection_text_set(const text *txt, const Set *s); */
  set_result = intersection_text_set(const text *txt, const Set *s);

  /* Set *intersection_timestamptz_set(TimestampTz t, const Set *s); */
  set_result = intersection_timestamptz_set(TimestampTz t, const Set *s);

  /* Set *minus_bigint_set(int64 i, const Set *s); */
  set_result = minus_bigint_set(int64 i, const Set *s);

  /* SpanSet *minus_bigint_span(int64 i, const Span *s); */
  spanset_result = minus_bigint_span(int64 i, const Span *s);

  /* SpanSet *minus_bigint_spanset(int64 i, const SpanSet *ss); */
  spanset_result = minus_bigint_spanset(int64 i, const SpanSet *ss);

  /* Set *minus_date_set(DateADT d, const Set *s); */
  set_result = minus_date_set(DateADT d, const Set *s);

  /* SpanSet *minus_date_span(DateADT d, const Span *s); */
  spanset_result = minus_date_span(DateADT d, const Span *s);

  /* SpanSet *minus_date_spanset(DateADT d, const SpanSet *ss); */
  spanset_result = minus_date_spanset(DateADT d, const SpanSet *ss);

  /* Set *minus_float_set(double d, const Set *s); */
  set_result = minus_float_set(double d, const Set *s);

  /* SpanSet *minus_float_span(double d, const Span *s); */
  spanset_result = minus_float_span(double d, const Span *s);

  /* SpanSet *minus_float_spanset(double d, const SpanSet *ss); */
  spanset_result = minus_float_spanset(double d, const SpanSet *ss);

  /* Set *minus_int_set(int i, const Set *s); */
  set_result = minus_int_set(int i, const Set *s);

  /* SpanSet *minus_int_span(int i, const Span *s); */
  spanset_result = minus_int_span(int i, const Span *s);

  /* SpanSet *minus_int_spanset(int i, const SpanSet *ss); */
  spanset_result = minus_int_spanset(int i, const SpanSet *ss);

  /* Set *minus_set_bigint(const Set *s, int64 i); */
  set_result = minus_set_bigint(const Set *s, int64 i);

  /* Set *minus_set_date(const Set *s, DateADT d); */
  set_result = minus_set_date(const Set *s, DateADT d);

  /* Set *minus_set_float(const Set *s, double d); */
  set_result = minus_set_float(const Set *s, double d);

  /* Set *minus_set_int(const Set *s, int i); */
  set_result = minus_set_int(const Set *s, int i);

  /* Set *minus_set_set(const Set *s1, const Set *s2); */
  set_result = minus_set_set(const Set *s1, const Set *s2);

  /* Set *minus_set_text(const Set *s, const text *txt); */
  set_result = minus_set_text(const Set *s, const text *txt);

  /* Set *minus_set_timestamptz(const Set *s, TimestampTz t); */
  set_result = minus_set_timestamptz(const Set *s, TimestampTz t);

  /* SpanSet *minus_span_bigint(const Span *s, int64 i); */
  spanset_result = minus_span_bigint(const Span *s, int64 i);

  /* SpanSet *minus_span_date(const Span *s, DateADT d); */
  spanset_result = minus_span_date(const Span *s, DateADT d);

  /* SpanSet *minus_span_float(const Span *s, double d); */
  spanset_result = minus_span_float(const Span *s, double d);

  /* SpanSet *minus_span_int(const Span *s, int i); */
  spanset_result = minus_span_int(const Span *s, int i);

  /* SpanSet *minus_span_span(const Span *s1, const Span *s2); */
  spanset_result = minus_span_span(const Span *s1, const Span *s2);

  /* SpanSet *minus_span_spanset(const Span *s, const SpanSet *ss); */
  spanset_result = minus_span_spanset(const Span *s, const SpanSet *ss);

  /* SpanSet *minus_span_timestamptz(const Span *s, TimestampTz t); */
  spanset_result = minus_span_timestamptz(const Span *s, TimestampTz t);

  /* SpanSet *minus_spanset_bigint(const SpanSet *ss, int64 i); */
  spanset_result = minus_spanset_bigint(const SpanSet *ss, int64 i);

  /* SpanSet *minus_spanset_date(const SpanSet *ss, DateADT d); */
  spanset_result = minus_spanset_date(const SpanSet *ss, DateADT d);

  /* SpanSet *minus_spanset_float(const SpanSet *ss, double d); */
  spanset_result = minus_spanset_float(const SpanSet *ss, double d);

  /* SpanSet *minus_spanset_int(const SpanSet *ss, int i); */
  spanset_result = minus_spanset_int(const SpanSet *ss, int i);

  /* SpanSet *minus_spanset_span(const SpanSet *ss, const Span *s); */
  spanset_result = minus_spanset_span(const SpanSet *ss, const Span *s);

  /* SpanSet *minus_spanset_spanset(const SpanSet *ss1, const SpanSet *ss2); */
  spanset_result = minus_spanset_spanset(const SpanSet *ss1, const SpanSet *ss2);

  /* SpanSet *minus_spanset_timestamptz(const SpanSet *ss, TimestampTz t); */
  spanset_result = minus_spanset_timestamptz(const SpanSet *ss, TimestampTz t);

  /* Set *minus_text_set(const text *txt, const Set *s); */
  set_result = minus_text_set(const text *txt, const Set *s);

  /* Set *minus_timestamptz_set(TimestampTz t, const Set *s); */
  set_result = minus_timestamptz_set(TimestampTz t, const Set *s);

  /* SpanSet *minus_timestamptz_span(TimestampTz t, const Span *s); */
  spanset_result = minus_timestamptz_span(TimestampTz t, const Span *s);

  /* SpanSet *minus_timestamptz_spanset(TimestampTz t, const SpanSet *ss); */
  spanset_result = minus_timestamptz_spanset(TimestampTz t, const SpanSet *ss);

  /* Set *union_bigint_set(int64 i, const Set *s); */
  set_result = union_bigint_set(int64 i, const Set *s);

  /* SpanSet *union_bigint_span(const Span *s, int64 i); */
  spanset_result = union_bigint_span(const Span *s, int64 i);

  /* SpanSet *union_bigint_spanset(int64 i, SpanSet *ss); */
  spanset_result = union_bigint_spanset(int64 i, SpanSet *ss);

  /* Set *union_date_set(DateADT d, const Set *s); */
  set_result = union_date_set(DateADT d, const Set *s);

  /* SpanSet *union_date_span(const Span *s, DateADT d); */
  spanset_result = union_date_span(const Span *s, DateADT d);

  /* SpanSet *union_date_spanset(DateADT d, SpanSet *ss); */
  spanset_result = union_date_spanset(DateADT d, SpanSet *ss);

  /* Set *union_float_set(double d, const Set *s); */
  set_result = union_float_set(double d, const Set *s);

  /* SpanSet *union_float_span(const Span *s, double d); */
  spanset_result = union_float_span(const Span *s, double d);

  /* SpanSet *union_float_spanset(double d, SpanSet *ss); */
  spanset_result = union_float_spanset(double d, SpanSet *ss);

  /* Set *union_int_set(int i, const Set *s); */
  set_result = union_int_set(int i, const Set *s);

  /* SpanSet *union_int_span(int i, const Span *s); */
  spanset_result = union_int_span(int i, const Span *s);

  /* SpanSet *union_int_spanset(int i, SpanSet *ss); */
  spanset_result = union_int_spanset(int i, SpanSet *ss);

  /* Set *union_set_bigint(const Set *s, int64 i); */
  set_result = union_set_bigint(const Set *s, int64 i);

  /* Set *union_set_date(const Set *s, DateADT d); */
  set_result = union_set_date(const Set *s, DateADT d);

  /* Set *union_set_float(const Set *s, double d); */
  set_result = union_set_float(const Set *s, double d);

  /* Set *union_set_int(const Set *s, int i); */
  set_result = union_set_int(const Set *s, int i);

  /* Set *union_set_set(const Set *s1, const Set *s2); */
  set_result = union_set_set(const Set *s1, const Set *s2);

  /* Set *union_set_text(const Set *s, const text *txt); */
  set_result = union_set_text(const Set *s, const text *txt);

  /* Set *union_set_timestamptz(const Set *s, TimestampTz t); */
  set_result = union_set_timestamptz(const Set *s, TimestampTz t);

  /* SpanSet *union_span_bigint(const Span *s, int64 i); */
  spanset_result = union_span_bigint(const Span *s, int64 i);

  /* SpanSet *union_span_date(const Span *s, DateADT d); */
  spanset_result = union_span_date(const Span *s, DateADT d);

  /* SpanSet *union_span_float(const Span *s, double d); */
  spanset_result = union_span_float(const Span *s, double d);

  /* SpanSet *union_span_int(const Span *s, int i); */
  spanset_result = union_span_int(const Span *s, int i);

  /* SpanSet *union_span_span(const Span *s1, const Span *s2); */
  spanset_result = union_span_span(const Span *s1, const Span *s2);

  /* SpanSet *union_span_spanset(const Span *s, const SpanSet *ss); */
  spanset_result = union_span_spanset(const Span *s, const SpanSet *ss);

  /* SpanSet *union_span_timestamptz(const Span *s, TimestampTz t); */
  spanset_result = union_span_timestamptz(const Span *s, TimestampTz t);

  /* SpanSet *union_spanset_bigint(const SpanSet *ss, int64 i); */
  spanset_result = union_spanset_bigint(const SpanSet *ss, int64 i);

  /* SpanSet *union_spanset_date(const SpanSet *ss, DateADT d); */
  spanset_result = union_spanset_date(const SpanSet *ss, DateADT d);

  /* SpanSet *union_spanset_float(const SpanSet *ss, double d); */
  spanset_result = union_spanset_float(const SpanSet *ss, double d);

  /* SpanSet *union_spanset_int(const SpanSet *ss, int i); */
  spanset_result = union_spanset_int(const SpanSet *ss, int i);

  /* SpanSet *union_spanset_span(const SpanSet *ss, const Span *s); */
  spanset_result = union_spanset_span(const SpanSet *ss, const Span *s);

  /* SpanSet *union_spanset_spanset(const SpanSet *ss1, const SpanSet *ss2); */
  spanset_result = union_spanset_spanset(const SpanSet *ss1, const SpanSet *ss2);

  /* SpanSet *union_spanset_timestamptz(const SpanSet *ss, TimestampTz t); */
  spanset_result = union_spanset_timestamptz(const SpanSet *ss, TimestampTz t);

  /* Set *union_text_set(const text *txt, const Set *s); */
  set_result = union_text_set(const text *txt, const Set *s);

  /* Set *union_timestamptz_set(TimestampTz t, const Set *s); */
  set_result = union_timestamptz_set(TimestampTz t, const Set *s);

  /* SpanSet *union_timestamptz_span(TimestampTz t, const Span *s); */
  spanset_result = union_timestamptz_span(TimestampTz t, const Span *s);

  /* SpanSet *union_timestamptz_spanset(TimestampTz t, SpanSet *ss); */
  spanset_result = union_timestamptz_spanset(TimestampTz t, SpanSet *ss);

  /*****************************************************************************
   * Distance functions for set and span types
   *****************************************************************************/

  int64_result = distance_bigintset_bigintset(const Set *s1, const Set *s2); */
  int64_result = distance_bigintset_bigintset(const Set *s1, const Set *s2);

  /* int64 distance_bigintspan_bigintspan(const Span *s1, const Span *s2); */
  int64_result = distance_bigintspan_bigintspan(const Span *s1, const Span *s2);

  /* int64 distance_bigintspanset_bigintspan(const SpanSet *ss, const Span *s); */
  int64_result = distance_bigintspanset_bigintspan(const SpanSet *ss, const Span *s);

  /* int64 distance_bigintspanset_bigintspanset(const SpanSet *ss1, const SpanSet *ss2); */
  int64_result = distance_bigintspanset_bigintspanset(const SpanSet *ss1, const SpanSet *ss2);

  /* int distance_dateset_dateset(const Set *s1, const Set *s2); */
  int32_result = distance_dateset_dateset(const Set *s1, const Set *s2);

  /* int distance_datespan_datespan(const Span *s1, const Span *s2); */
  int32_result = distance_datespan_datespan(const Span *s1, const Span *s2);

  /* int distance_datespanset_datespan(const SpanSet *ss, const Span *s); */
  int32_result = distance_datespanset_datespan(const SpanSet *ss, const Span *s);

  /* int distance_datespanset_datespanset(const SpanSet *ss1, const SpanSet *ss2); */
  int32_result = distance_datespanset_datespanset(const SpanSet *ss1, const SpanSet *ss2);

  /* double distance_floatset_floatset(const Set *s1, const Set *s2); */
  double_result = distance_floatset_floatset(const Set *s1, const Set *s2);

  /* double distance_floatspan_floatspan(const Span *s1, const Span *s2); */
  double_result = distance_floatspan_floatspan(const Span *s1, const Span *s2);

  /* double distance_floatspanset_floatspan(const SpanSet *ss, const Span *s); */
  double_result = distance_floatspanset_floatspan(const SpanSet *ss, const Span *s);

  /* double distance_floatspanset_floatspanset(const SpanSet *ss1, const SpanSet *ss2); */
  double_result = distance_floatspanset_floatspanset(const SpanSet *ss1, const SpanSet *ss2);

  /* int distance_intset_intset(const Set *s1, const Set *s2); */
  int32_result = distance_intset_intset(const Set *s1, const Set *s2);

  /* int distance_intspan_intspan(const Span *s1, const Span *s2); */
  int32_result = distance_intspan_intspan(const Span *s1, const Span *s2);

  /* int distance_intspanset_intspan(const SpanSet *ss, const Span *s); */
  int32_result = distance_intspanset_intspan(const SpanSet *ss, const Span *s);

  /* int distance_intspanset_intspanset(const SpanSet *ss1, const SpanSet *ss2); */
  int32_result = distance_intspanset_intspanset(const SpanSet *ss1, const SpanSet *ss2);

  /* int64 distance_set_bigint(const Set *s, int64 i); */
  int64_result = distance_set_bigint(const Set *s, int64 i);

  /* int distance_set_date(const Set *s, DateADT d); */
  int32_result = distance_set_date(const Set *s, DateADT d);

  /* double distance_set_float(const Set *s, double d); */
  double_result = distance_set_float(const Set *s, double d);

  /* int distance_set_int(const Set *s, int i); */
  int32_result = distance_set_int(const Set *s, int i);

  /* double distance_set_timestamptz(const Set *s, TimestampTz t); */
  double_result = distance_set_timestamptz(const Set *s, TimestampTz t);

  /* int64 distance_span_bigint(const Span *s, int64 i); */
  int64_result = distance_span_bigint(const Span *s, int64 i);

  /* int distance_span_date(const Span *s, DateADT d); */
  int32_result = distance_span_date(const Span *s, DateADT d);

  /* double distance_span_float(const Span *s, double d); */
  double_result = distance_span_float(const Span *s, double d);

  /* int distance_span_int(const Span *s, int i); */
  int32_result = distance_span_int(const Span *s, int i);

  /* double distance_span_timestamptz(const Span *s, TimestampTz t); */
  double_result = distance_span_timestamptz(const Span *s, TimestampTz t);

  /* int64 distance_spanset_bigint(const SpanSet *ss, int64 i); */
  int64_result = distance_spanset_bigint(const SpanSet *ss, int64 i);

  /* int distance_spanset_date(const SpanSet *ss, DateADT d); */
  int32_result = distance_spanset_date(const SpanSet *ss, DateADT d);

  /* double distance_spanset_float(const SpanSet *ss, double d); */
  double_result = distance_spanset_float(const SpanSet *ss, double d);

  /* int distance_spanset_int(const SpanSet *ss, int i); */
  int32_result = distance_spanset_int(const SpanSet *ss, int i);

  /* double distance_spanset_timestamptz(const SpanSet *ss, TimestampTz t); */
  double_result = distance_spanset_timestamptz(const SpanSet *ss, TimestampTz t);

  /* double distance_tstzset_tstzset(const Set *s1, const Set *s2); */
  double_result = distance_tstzset_tstzset(const Set *s1, const Set *s2);

  /* double distance_tstzspan_tstzspan(const Span *s1, const Span *s2); */
  double_result = distance_tstzspan_tstzspan(const Span *s1, const Span *s2);

  /* double distance_tstzspanset_tstzspan(const SpanSet *ss, const Span *s); */
  double_result = distance_tstzspanset_tstzspan(const SpanSet *ss, const Span *s);

  /* double distance_tstzspanset_tstzspanset(const SpanSet *ss1, const SpanSet *ss2); */
  double_result = distance_tstzspanset_tstzspanset(const SpanSet *ss1, const SpanSet *ss2);

  /*****************************************************************************
   * Aggregate functions for set and span types
   *****************************************************************************/

  /* Span *bigint_extent_transfn(Span *state, int64 i); */
  span_result = bigint_extent_transfn(Span *state, int64 i);

  /* Set *bigint_union_transfn(Set *state, int64 i); */
  set_result = bigint_union_transfn(Set *state, int64 i);

  /* Span *date_extent_transfn(Span *state, DateADT d); */
  span_result = date_extent_transfn(Span *state, DateADT d);

  /* Set *date_union_transfn(Set *state, DateADT d); */
  set_result = date_union_transfn(Set *state, DateADT d);

  /* Span *float_extent_transfn(Span *state, double d); */
  span_result = float_extent_transfn(Span *state, double d);

  /* Set *float_union_transfn(Set *state, double d); */
  set_result = float_union_transfn(Set *state, double d);

  /* Span *int_extent_transfn(Span *state, int i); */
  span_result = int_extent_transfn(Span *state, int i);

  /* Set *int_union_transfn(Set *state, int32 i); */
  set_result = int_union_transfn(Set *state, int32 i);

  /* Span *set_extent_transfn(Span *state, const Set *s); */
  span_result = set_extent_transfn(Span *state, const Set *s);

  /* Set *set_union_finalfn(Set *state);
  set_result = set_union_finalfn(Set *state);

  /* Set *set_union_transfn(Set *state, Set *s); */
  set_result = set_union_transfn(Set *state, Set *s);

  /* Span *span_extent_transfn(Span *state, const Span *s); */
  span_result = span_extent_transfn(Span *state, const Span *s);

  /* SpanSet *span_union_transfn(SpanSet *state, const Span *s); */
  spanset_result = span_union_transfn(SpanSet *state, const Span *s);

  /* Span *spanset_extent_transfn(Span *state, const SpanSet *ss); */
  span_result = spanset_extent_transfn(Span *state, const SpanSet *ss);

  /* SpanSet *spanset_union_finalfn(SpanSet *state); */
  spanset_result = spanset_union_finalfn(SpanSet *state);

  /* SpanSet *spanset_union_transfn(SpanSet *state, const SpanSet *ss); */
  spanset_result = spanset_union_transfn(SpanSet *state, const SpanSet *ss);

  /* Set *text_union_transfn(Set *state, const text *txt); */
  set_result = text_union_transfn(Set *state, const text *txt);

  /* Span *timestamptz_extent_transfn(Span *state, TimestampTz t); */
  span_result = timestamptz_extent_transfn(Span *state, TimestampTz t);

  /* Set *timestamptz_union_transfn(Set *state, TimestampTz t); */
  set_result = timestamptz_union_transfn(Set *state, TimestampTz t);

  /*****************************************************************************
   * Bin functions for span and spanset types
   *****************************************************************************/

  /* int64 bigint_get_bin(int64 value, int64 vsize, int64 vorigin); */
  int64_result = bigint_get_bin(int64 value, int64 vsize, int64 vorigin);

  /* Span *bigintspan_bins(const Span *s, int64 vsize, int64 vorigin, int *count); */
  span_result = bigintspan_bins(const Span *s, int64 vsize, int64 vorigin, int *count);

  /* Span *bigintspanset_bins(const SpanSet *ss, int64 vsize, int64 vorigin, int *count); */
  span_result = bigintspanset_bins(const SpanSet *ss, int64 vsize, int64 vorigin, int *count);

  /* DateADT date_get_bin(DateADT d, const Interval *duration, DateADT torigin); */
  date_result = date_get_bin(DateADT d, const Interval *duration, DateADT torigin);

  /* Span *datespan_bins(const Span *s, const Interval *duration, DateADT torigin, int *count); */
  span_result = datespan_bins(const Span *s, const Interval *duration, DateADT torigin, int *count);

  /* Span *datespanset_bins(const SpanSet *ss, const Interval *duration, DateADT torigin, int *count); */
  span_result = datespanset_bins(const SpanSet *ss, const Interval *duration, DateADT torigin, int *count);

  /* double float_get_bin(double value, double vsize, double vorigin); */
  double_result = float_get_bin(double value, double vsize, double vorigin);

  /* Span *floatspan_bins(const Span *s, double vsize, double vorigin, int *count); */
  span_result = floatspan_bins(const Span *s, double vsize, double vorigin, int *count);

  /* Span *floatspanset_bins(const SpanSet *ss, double vsize, double vorigin, int *count); */
  span_result = floatspanset_bins(const SpanSet *ss, double vsize, double vorigin, int *count);

  /* int int_get_bin(int value, int vsize, int vorigin); */
  int32_result = int_get_bin(int value, int vsize, int vorigin);

  /* Span *intspan_bins(const Span *s, int vsize, int vorigin, int *count); */
  span_result = intspan_bins(const Span *s, int vsize, int vorigin, int *count);

  /* Span *intspanset_bins(const SpanSet *ss, int vsize, int vorigin, int *count); */
  span_result = intspanset_bins(const SpanSet *ss, int vsize, int vorigin, int *count);

  /* TimestampTz timestamptz_get_bin(TimestampTz t, const Interval *duration, TimestampTz torigin); */
  tstz_result = timestamptz_get_bin(TimestampTz t, const Interval *duration, TimestampTz torigin);

  /* Span *tstzspan_bins(const Span *s, const Interval *duration, TimestampTz origin, int *count); */
  span_result = tstzspan_bins(const Span *s, const Interval *duration, TimestampTz origin, int *count);

  /* Span *tstzspanset_bins(const SpanSet *ss, const Interval *duration, TimestampTz torigin, int *count); */
  span_result = tstzspanset_bins(const SpanSet *ss, const Interval *duration, TimestampTz torigin, int *count);

  printf("****************************************************************\n");

  /* Clean up */
  free(b1_out);
  free(text1); free(text2);
  free(text1_out); free(text2_out);
  
  /* Finalize MEOS */
  meos_finalize();

  return 0;
}
