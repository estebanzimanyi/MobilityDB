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
 * @brief A simple program that tests the boolean functions exposed by the
 * PostgreSQL types embedded in MEOS.
 *
 * The program can be build as follows
 * @code
 * gcc -Wall -g -I/usr/local/include -o timestamp_test timestamp_test.c -L/usr/local/lib -lmeos
 * @endcode
 */

#include <stdio.h>
#include <stdlib.h>
#include <meos.h>
#include <pg_bool.h>
#include <pg_numeric.h>
#include <pg_text.h>
#include <pg_timestamp.h>

/* Main program */
int main(void)
{
  /* Initialize MEOS */
  meos_initialize();
  meos_initialize_timezone("UTC");

  /* Create values to test the functions of the API */
  float8 float8_in1 = 8;
  DateADT date_in1 = date_in("2025-03-01");
  char *date_out1 = date_out(date_in1);
  Interval *interv_in1 = interval_in("8 hours", -1);
  char *interv_out1 = interval_out(interv_in1);
  Timestamp ts_in1 = timestamp_in("2025-03-01 08:00", -1);
  Timestamp ts_in2 = timestamp_in("2025-05-01 08:00", -1);
  Timestamp ts_in3 = timestamp_in("2025-04-01 08:00", -1);
  Timestamp ts_in4 = timestamp_in("2025-06-01 08:00", -1);
  char *ts_out1 = timestamp_out(ts_in1);
  char *ts_out2 = timestamp_out(ts_in2);
  char *ts_out3 = timestamp_out(ts_in3);
  char *ts_out4 = timestamp_out(ts_in4);
  TimestampTz tstz_in1 = timestamptz_in("2025-03-01 08:00", -1);
  TimestampTz tstz_in2 = timestamptz_in("2025-05-01 08:00", -1);
  TimestampTz tstz_in3 = timestamptz_in("2025-04-01 08:00", -1);
  TimestampTz tstz_in4 = timestamptz_in("2025-06-01 08:00", -1);
  char *tstz_out1 = timestamptz_out(tstz_in1);
  char *tstz_out2 = timestamptz_out(tstz_in2);
  char *tstz_out3 = timestamptz_out(tstz_in3);
  char *tstz_out4 = timestamptz_out(tstz_in4);

  /* Create the result types for the bool functions of the API */
  bool bool_result;
  int32 int32_result;
  uint32 uint32_result;
  uint64 uint64_result;
  float8 float8_result;
  char *char_result;
  text *text_result;
  Interval *interv_result;
  Numeric numeric_result;
  Timestamp ts_result;
  TimestampTz tstz_result;
  
  /* Execute and print the result for the bool functions of the API */

  ts_result = add_timestamp_interval(ts_in1, interv_in1);
  char_result = timestamp_out(ts_result);
  printf("add_timestamp_interval(%s, %s): %s\n", ts_out1, interv_out1, char_result);
  free(char_result);

  ts_result = add_timestamptz_interval(tstz_in1, interv_in1);
  char_result = timestamp_out(ts_result);
  printf("add_timestamptz_interval(%s, %s): %s\n", tstz_out1, interv_out1, char_result);
  free(char_result);

  text *zone = text_in("Europe/Brussels");
  ts_result = add_timestamptz_interval_at_zone(tstz_in1, interv_in1, zone);
  char_result = timestamp_out(ts_result);
  printf("add_timestamptz_interval_at_zone(%s, %s, \"Europe/Brussels\"): %s\n",tstz_out1, interv_out1, char_result);
  free(zone); free(char_result);

  int32_result = cmp_timestamp_timestamp(ts_in1, ts_in2);
  printf("cmp_timestamp_timestamp(%s, %s): %d\n", ts_out1, ts_out2, int32_result);

  int32_result = cmp_timestamp_timestamptz(ts_in1, tstz_in1);
  printf("cmp_timestamp_timestamptz(%s, %s): %c\n", ts_out1, tstz_out1, int32_result);

  int32_result = cmp_timestamptz_timestamp(tstz_in1, ts_in1);
  printf("cmp_timestamptz_timestamp(%s, %s): %c\n", tstz_out1, ts_out1, int32_result);

  bool_result = eq_timestamp_date(ts_in1, date_in1);
  printf("eq_timestamp_date(%s, %s): %c\n", ts_out1, date_out1, bool_result ? 't' : 'f');

  bool_result = eq_timestamp_timestamp(ts_in1, ts_in2);
  printf("eq_timestamp_timestamp(%s, %s): %c\n", ts_out1, ts_out2, bool_result ? 't' : 'f');

  bool_result = eq_timestamp_timestamptz(ts_in1, tstz_in1);
  printf("eq_timestamp_timestamptz(%s, %s): %c\n", ts_out1, tstz_out1, bool_result ? 't' : 'f');

  bool_result = eq_timestamptz_date(tstz_in1, date_in1);
  printf("eq_timestamptz_date(%s, %s): %c\n", tstz_out1, date_out1, bool_result ? 't' : 'f');

  bool_result = eq_timestamptz_timestamp(tstz_in1, ts_in1);
  printf("eq_timestamptz_timestamp(%s, %s): %c\n", tstz_out1, ts_out1, bool_result ? 't' : 'f');

  bool_result = eq_timestamptz_timestamptz(tstz_in1, tstz_in2);
  printf("timestamptz_eq(%s, %s): %c\n", tstz_out1, tstz_out2, bool_result ? 't' : 'f');

  tstz_result = float8_to_timestamptz(float8_in1);
  char_result = timestamptz_out(tstz_result);
  printf("float8_to_timestamptz(%lf): %s\n", float8_in1, char_result);
  free(char_result);

  bool_result = gt_timestamp_timestamp(ts_in1, ts_in2);
  printf("gt_timestamp_timestamp(%s, %s): %c\n", ts_out1, ts_out2, bool_result ? 't' : 'f');

  bool_result = gt_timestamp_timestamptz(ts_in1, tstz_in1);
  printf("gt_timestamp_timestamptz(%s, %s): %c\n", ts_out1, tstz_out1, bool_result ? 't' : 'f');

  bool_result = gt_timestamptz_timestamp(tstz_in1, ts_in1);
  printf("gt_timestamptz_timestamp(%s, %s): %c\n", tstz_out1, ts_out1, bool_result ? 't' : 'f');

  bool_result = ge_timestamp_timestamp(ts_in1, ts_in2);
  printf("ge_timestamp_timestamp(%s, %s): %c\n", ts_out1, ts_out2, bool_result ? 't' : 'f');

  bool_result = ge_timestamp_timestamptz(ts_in1, tstz_in1);
  printf("ge_timestamp_timestamptz(%s, %s): %c\n", ts_out1, tstz_out1, bool_result ? 't' : 'f');

  bool_result = ge_timestamptz_timestamp(tstz_in1, ts_in1);
  printf("ge_timestamptz_timestamp(%s, %s): %c\n", tstz_out1, ts_out1, bool_result ? 't' : 'f');

  bool_result = le_timestamp_timestamp(ts_in1, ts_in2);
  printf("le_timestamp_timestamp(%s, %s): %c\n", ts_out1, ts_out2, bool_result ? 't' : 'f');

  bool_result = le_timestamp_timestamptz(ts_in1, tstz_in1);
  printf("le_timestamp_timestamptz(%s, %s): %c\n", ts_out1, tstz_out1, bool_result ? 't' : 'f');

  bool_result = le_timestamptz_timestamp(tstz_in1, ts_in1);
  printf("le_timestamptz_timestamp(%s, %s): %c\n", tstz_out1, ts_out1, bool_result ? 't' : 'f');

  bool_result = lt_timestamp_timestamp(ts_in1, ts_in2);
  printf("lt_timestamp_timestamp(%s, %s): %c\n", tstz_out1, ts_out2, bool_result ? 't' : 'f');

  bool_result = lt_timestamp_timestamptz(ts_in1, tstz_in1);
  printf("lt_timestamp_timestamptz(%s, %s): %c\n", ts_out1, tstz_out1, bool_result ? 't' : 'f');

  bool_result = lt_timestamptz_timestamp(tstz_in1, ts_in1);
  printf("lt_timestamptz_timestamp(%s, %s): %c\n", tstz_out1, ts_out1, bool_result ? 't' : 'f');

  ts_result = minus_timestamp_interval(ts_in1, interv_in1);
  char_result = timestamp_out(ts_result);
  printf("minus_timestamp_interval(%s, %s): %s\n", ts_out1, interv_out1, char_result);
  free(char_result);

  interv_result = minus_timestamp_timestamp(ts_in1, ts_in2);
  char_result = interval_out(interv_result);
  printf("minus_timestamp_timestamp(%s, %s): %s\n", ts_out1, ts_out2, char_result);
  free(char_result);

  tstz_result = minus_timestamptz_interval(tstz_in1, interv_in1);
  char_result = timestamptz_out(tstz_result);
  printf("minus_timestamptz_interval(%s, %s): %s\n", tstz_out1, interv_out1, char_result);
  free(char_result);

  zone = text_in("Europe/Brussels");
  tstz_result = minus_timestamptz_interval_at_zone(tstz_in1, interv_in1, zone);
  char_result = timestamptz_out(tstz_result);
  printf("minus_timestamptz_interval_at_zone(%s, %s, \"Europe/Brussels\"): %s\n", tstz_out1, interv_out1, char_result);
  free(zone); free(char_result);

  interv_result = minus_timestamptz_timestamptz(tstz_in1, tstz_in2);
  char_result = interval_out(interv_result);
  printf("minus_timestamptz_timestamptz(%s, %s): %s\n", tstz_out1, tstz_out2, char_result);
  free(char_result);

  bool_result = ne_timestamp_date(ts_in1, date_in1);
  printf("ne_timestamp_date(%s, %s): %c\n", ts_out1, date_out1, bool_result ? 't' : 'f');

  bool_result = ne_timestamptz_date(tstz_in1, date_in1);
  printf("ne_timestamptz_date(%s, %s): %c\n", tstz_out1, date_out1, bool_result ? 't' : 'f');

  bool_result = ne_timestamp_timestamp(ts_in1, ts_in2);
  printf("ne_timestamp_timestamp(%s, %s): %c\n", ts_out1, ts_out2, bool_result ? 't' : 'f');

  bool_result = ne_timestamp_timestamptz(ts_in1, tstz_in1);
  printf("ne_timestamp_timestamptz(%s, %s): %c\n", ts_out1, tstz_out1, bool_result ? 't' : 'f');

  bool_result = ne_timestamptz_timestamp(tstz_in1, ts_in1);
  printf("ne_timestamptz_timestamp(%s, %s): %c\n", tstz_out1, ts_out1, bool_result ? 't' : 'f');

  interv_result = interval_justify_days(interv_in1);
  char_result = interval_out(interv_result);
  printf("interval_justify_days(%s): %s\n", interv_out1, char_result);
  free(char_result);

  interv_result = interval_justify_hours(interv_in1);
  char_result = interval_out(interv_result);
  printf("interval_justify_hours(%s): %s\n", interv_out1, char_result);
  free(char_result);

  interv_result = interval_justify_interval(interv_in1);
  char_result = interval_out(interv_result);
  printf("interval_justify_interval(%s): %s\n", interv_out1, char_result);
  free(char_result);

  interv_result = timestamp_age(ts_in1, ts_in2);
  char_result = interval_out(interv_result);
  printf("timestamp_age(%s, %s): %s\n", ts_out1, ts_out2, char_result);
  free(char_result);

  tstz_result = timestamp_at_local(ts_in1);
  char_result = timestamptz_out(tstz_result);
  printf("timestamp_at_local(%s): %s\n", ts_out1, char_result);
  free(char_result);

  Interval *stride = interval_in("1 hour", -1);
  Timestamp origin = timestamp_in("2000-01-03", -1);
  ts_result = timestamp_bin(ts_in1, stride, origin);
  char_result = timestamp_out(ts_result);
  printf("timestamp_bin(%s, \"1 hour\", \"2000-01-03\"): %s\n", ts_out1, char_result);
  free(stride); free(char_result);

  uint32_result = timestamp_hash(ts_in1);
  printf("timestamp_hash(%s): %d\n", ts_out1, uint32_result);

  uint64_result = timestamp_hash_extended(ts_in1, 1);
  printf("timestamp_hash_extended(%s, 1): %ld\n", ts_out1, uint64_result);

  ts_result = timestamp_in("2025-03-01 08:00:00", -1);
  char_result = timestamp_out(ts_result);
  printf("timestamp_in(\"2025-03-01 08:00:00\", -1): %s\n", char_result);
  free(char_result);

  Interval *interv_zone = interval_in("2 hours", -1);
  tstz_result = timestamp_izone(ts_in1, interv_zone);
  char_result = timestamptz_out(tstz_result);
  printf("timestamp_izone(%s, \"Europe/Brussels\"): %s\n", ts_out1, char_result);
  free(interv_zone); free(char_result);

  text_result = time_of_day();
  char_result = text_out(text_result);
  printf("time_of_day(): %s\n", char_result);
  free(char_result);

  ts_result = timestamp_larger(ts_in1, ts_in2);
  char_result = timestamp_out(ts_result);
  printf("timestamp_larger(%s, %s): %s\n", ts_out1, ts_out2, char_result);
  free(char_result);

  char_result = timestamp_out(ts_in1);
  printf("timestamp_out(%s,): %s\n", ts_out1, char_result);
  free(char_result);

  text *units = text_in("seconds");
  float8_result = timestamp_part(ts_in1, units);
  printf("timestamp_part(%s, \"seconds\"): %lf\n", ts_out1, float8_result);
  free(units);

  ts_result = timestamp_scale(ts_in1, -1);
  char_result = timestamp_out(ts_result);
  printf("timestamp_scale(%s, -1): %s\n", ts_out1, char_result);
  free(char_result);

  ts_result = timestamp_smaller(ts_in1, ts_in2);
  char_result = timestamp_out(ts_result);
  printf("timestamp_smaller(%s, %s): %s\n", ts_out1, ts_out2, char_result);
  free(char_result);

  units = text_in("seconds");
  ts_result = timestamp_trunc(ts_in1, units);
  char_result = timestamp_out(ts_result);
  printf("timestamp_trunc(%s, \"seconds\"): %s\n", ts_out1, char_result);
  free(units); free(char_result);

  zone = text_in("Europe/Brussels");
  tstz_result = timestamp_zone(ts_in1, zone);
  char_result = timestamptz_out(tstz_result);
  printf("timestamp_zone(%s, \"Europe/Brussels\"): %s\n", ts_out1, char_result);
  free(zone); free(char_result);

  interv_result = timestamptz_age(tstz_in1, tstz_in2);
  char_result = interval_out(interv_result);
  printf("timestamptz_age(%s, %s): %s\n", tstz_out1, tstz_out2, char_result);
  free(char_result);

  stride = interval_in("1 hour", -1);
  origin = timestamp_in("2000-01-03", -1);
  tstz_result = timestamptz_bin(tstz_in1, stride, origin);
  char_result = timestamptz_out(tstz_result);
  printf("timestamptz_bin(%s, \"1 hour\", \"2000-01-03\"): %s\n", tstz_out1, char_result);
  free(stride); free(char_result);

  int32_result = timestamptz_hash(tstz_in1);
  printf("timestamptz_hash(%s): %d\n", tstz_out1, int32_result);

  uint64_result = timestamptz_hash_extended(tstz_in1, 1);
  printf("timestamptz_hash_extended(%s, 1): %ld\n", tstz_out1, uint64_result);

  tstz_result = timestamptz_in("2025-03-01 08:00:00", -1);
  char_result = timestamptz_out(tstz_result);
  printf("timestamptz_in(\"2025-03-01 08:00:00\", -1): %s\n", char_result);
  free(char_result);

  interv_zone = interval_in("2 hours", -1);
  ts_result = timestamptz_izone(tstz_in1, interv_zone);
  char_result = timestamp_out(ts_result);
  printf("timestamptz_izone(%s, \"Europe/Brussels\"): %s\n", tstz_out1, char_result);
  free(interv_zone); free(char_result);

  char_result = timestamptz_out(tstz_in1);
  printf("timestamptz_out(%s): %s\n", tstz_out1, char_result);
  free(char_result);

  units = text_in("seconds");
  float8_result = timestamptz_part(tstz_in1, units);
  printf("timestamptz_part(%s, \"seconds\"): %lf\n", tstz_out1, float8_result);
  free(units);

  tstz_result = timestamptz_scale(tstz_in1, -1);
  char_result = timestamptz_out(tstz_result);
  printf("timestamptz_scale(%s, -1): %s\n", tstz_out1, char_result);
  free(char_result);

  units = text_in("seconds");
  tstz_result = timestamptz_trunc(tstz_in1, units);
  char_result = timestamptz_out(tstz_result);
  printf("timestamptz_trunc(%s, \"seconds\"): %s\n", tstz_out1, char_result);
  free(units); free(char_result);

  units = text_in("seconds");
  zone = text_in("Europe/Brussels");
  tstz_result = timestamptz_trunc_zone(tstz_in1, units, zone);
  char_result = timestamptz_out(tstz_result);
  printf("timestamptz_trunc_zone(%s, \"seconds\", \"Europe/Brussels\"): %s\n", tstz_out1, char_result);
  free(units); free(zone); free(char_result);

  zone = text_in("Europe/Brussels");
  ts_result = timestamptz_zone(tstz_in1, zone);
  char_result = timestamp_out(ts_result);
  printf("timestamptz_zone(%s, \"Europe/Brussels\"): %s\n", tstz_out1, char_result);
  free(zone); free(char_result);

  units = text_in("seconds");
  numeric_result = timestamp_extract(ts_in1, units);
  char_result = numeric_out(numeric_result);
  printf("timestamp_extract(%s, \"seconds\"): %s\n", ts_out1, char_result);
  free(units); free(numeric_result); free(char_result);

  bool_result = timestamp_is_finite(ts_in1);
  printf("timestamp_is_finite(%s): %c\n", ts_out1, bool_result ? 't' : 'f');

  ts_result = timestamp_make(2025, 3, 1, 8, 0, 0);
  char_result = timestamp_out(ts_result);
  printf("timestamp_make(2025, 3, 1, 8, 0, 0): %s\n", char_result);
  free(char_result);

  bool_result = timestamp_overlaps(ts_in1, ts_in2, ts_in3, ts_in4);
  printf("timestamp_overlaps(%s, %s, %s, %s): %c\n", ts_out1, ts_out2, ts_out3, ts_out4, bool_result ? 't' : 'f');

  tstz_result = timestamp_to_timestamptz(ts_in1);
  char_result = timestamptz_out(tstz_result);
  printf("timestamp_to_timestamptz(%s): %s\n", ts_out1, char_result);
  free(char_result);

  tstz_result = timestamptz_at_local(tstz_in1);
  char_result = timestamptz_out(tstz_result);
  printf("timestamptz_at_local(%s): %s\n", tstz_out1, char_result);
  free(char_result);

  units = text_in("seconds");
  interv_result = timestamptz_extract(tstz_in1, units);
  char_result = interval_out(interv_result);
  printf("timestamptz_extract(%s, \"seconds\"): %s\n", tstz_out1, char_result);
  free(units); free(char_result);

  tstz_result = timestamptz_make(2025, 3, 1, 8, 0, 0);
  char_result = timestamptz_out(tstz_result);
  printf("timestamptz_make(2025, 03, 01, 08, 00, 00): %s\n", char_result);
  free(char_result);

  zone = text_in("Europe/Brussels");
  tstz_result = timestamptz_make_at_timezone(2025, 3, 1, 8, 0, 0, zone);
  char_result = timestamptz_out(tstz_result);
  printf("timestamptz_make_at_timezone(2025, 3, 1, 8, 0, 0, \"Europe/Brussels\"): %s\n", char_result);
  free(zone); free(char_result);

  bool_result = timestamptz_overlaps(tstz_in1, tstz_in2, tstz_in3, tstz_in4);
  printf("timestamptz_overlaps(%s, %s, %s, %s): %c\n", tstz_out1, tstz_out2, tstz_out3, tstz_out4, bool_result ? 't' : 'f');

  tstz_result = timestamptz_shift(tstz_in1, interv_in1);
  char_result = timestamptz_out(tstz_result);
  printf("timestamptz_shift(%s, %s): %s\n", tstz_out1, interv_out1, char_result);
  free(char_result);

  ts_result = timestamptz_to_timestamp(tstz_in1);
  char_result = timestamp_out(ts_result);
  printf("timestamptz_to_timestamp(%s): %s\n", tstz_out1, char_result);
  free(char_result);


  /* Finalize MEOS */
  meos_finalize();

  return 0;
}
