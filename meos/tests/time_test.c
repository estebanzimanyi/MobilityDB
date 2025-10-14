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
 * gcc -Wall -g -I/usr/local/include -o time_test time_test.c -L/usr/local/lib -lmeos
 * @endcode
 */

#include <stdio.h>
#include <stdlib.h>
#include <meos.h>
#include <pg_bool.h>
#include <pg_date.h>
#include <pg_time.h>
#include <pg_text.h>
#include <pg_numeric.h>

/* Main program */
int main(void)
{
  /* Initialize MEOS */
  meos_initialize();
  meos_initialize_timezone("UTC");

  /* Create two boolean values to test the bool functions of the API */
  DateADT date_in1 = date_in("2025-03-01");
  char *date_out1 = date_out(date_in1);
  TimeADT time_in1 = time_in("08:00:00", -1);
  TimeADT time_in2 = time_in("10:00:00", -1);
  TimeADT time_in3 = time_in("09:00:00", -1);
  TimeADT time_in4 = time_in("11:00:00", -1);
  char *time_out1 = time_out(time_in1);
  char *time_out2 = time_out(time_in2);
  char *time_out3 = time_out(time_in3);
  char *time_out4 = time_out(time_in4);
  TimeTzADT *timetz_in1 = timetz_in("08:00:00", -1);
  TimeTzADT *timetz_in2 = timetz_in("10:00:00", -1);
  TimeTzADT *timetz_in3 = timetz_in("09:00:00", -1);
  TimeTzADT *timetz_in4 = timetz_in("11:00:00", -1);
  char *timetz_out1 = timetz_out(timetz_in1);
  char *timetz_out2 = timetz_out(timetz_in2);
  char *timetz_out3 = timetz_out(timetz_in3);
  char *timetz_out4 = timetz_out(timetz_in4);
  Timestamp ts_in = timestamp_in("2025-03-01 08:00:00", -1);
  char *ts_out = timestamp_out(ts_in);
  TimestampTz tstz_in = timestamptz_in("2025-03-01 08:00:00", -1);
  char *tstz_out = timestamptz_out(tstz_in);
  Interval *interv_in = interval_in("3 hours", -1);
  char *interv_out = interval_out(interv_in);

  /* Create the result types for the bool functions of the API */
  bool bool_result;
  int32 int32_result;
  uint32 uint32_result;
  uint64 uint64_result;
  float8 float8_result;
  char *char_result;
  Numeric numeric_result;
  TimeADT time_result;
  TimeTzADT timetz_result;
  Interval *interv_result;
  TimestampTz tstz_result;

  /* Execute and print the result for the bool functions of the API */

  bool_result = time_eq(time_in1, time_in2);
  printf("time_eq(%s, %s): %s\n", time_out1, time_out2, bool_result ? "t" : "f");

  tstz_result = date_timetz_to_timestamptz(date_in1, timetz_in1);
  char_result = timestamptz_out(tstz_result);
  printf("timetz_out1(%s, %s): %s\n", date_out1, timetz_out1, char_result);
  free(char_result);

  time_result = interval_to_time(interv_in);
  char_result = time_out(time_result);
  printf("interval_to_time(%s, %s): %s\n", interv_out, char_result);
  free(char_result);

  time_result = minus_time_interval(time_in1, interv_in);
  char_result = time_out(time_result);
  printf("minus_time_interval(%s, %s): %s\n", time_out1, interv_out, char_result);
  free(char_result);

  interv_result = minus_time_time(time_in1, time_in2);
  char_result = time_out(time_result);
  printf("minus_time_time(%s, %s): %s\n", time_out1, time_out2, char_result);
  free(interv_result); free(char_result);

  timetz_result = minus_timetz_interval(timetz_in1, interv_in);
  char_result = timetz_out(timetz_result);
  printf("minus_timetz_interval(%s, %s): %s\n", timetz_out1, interv_out, bool_result ? "t" : "f");
  free(char_result);

  time_result = plus_time_interval(time_in1, interv_in);
  char_result = time_out(time_result);
  printf("plus_time_interval(%s, %s): %s\n", time_out1, interv_out, char_result);
  free(char_result);

  timetz_result = plus_timetz_interval(timetz_in1, interv_in);
  char_result = timetz_out(timetz_result);
  printf("plus_timetz_interval(%s, %s): %s\n", timetz_out1, interv_out, char_result);
  free(char_result);

  int32_result = time_cmp(time_in1, time_in2);
  printf("time_cmp(%s, %s): %d\n", time_out1, time_out2, int32_result);

  bool_result = time_eq(time_in1, time_in2);
  printf("time_eq(%s, %s): %s\n", time_out1, time_out2, bool_result ? "t" : "f");

  text *units = text_in("seconds");
  numeric_result = time_extract(time_in1, units);
  char_result = numeric_out(numeric_result);
  printf("time_extract(%s, \"seconds\"): %s\n", time_out1, char_result);
  free(units); free(numeric_result); free(char_result);

  bool_result = time_ge(time_in1, time_in2);
  printf("time_ge(%s, %s): %s\n", time_out1, time_out2, bool_result ? "t" : "f");

  bool_result = time_gt(time_in1, time_in2);
  printf("time_gt(%s, %s): %s\n", time_out1, time_out2, bool_result ? "t" : "f");

  uint32_result = time_hash(time_in1);
  printf("time_hash(%s): %s\n", time_out1, bool_result ? "t" : "f");

  uint64_result = time_hash_extended(time_in1, 1);
  printf("time_hash_extended(%s, 1): %s\n", time_out1, bool_result ? "t" : "f");

  time_result = time_in("08:00:00", -1);
  char_result = time_out(time_result);
  printf("time_in(\"08:00:00\"): %s\n", char_result);
  free(char_result);

  time_result = time_larger(time_in1, time_in2);
  char_result = time_out(time_result);
  printf("time_larger(%s, %s): %s\n", time_out1, time_out2, char_result);
  free(char_result);

  bool_result = time_le(time_in1, time_in2);
  printf("time_le(%s, %s): %s\n", time_out1, time_out2, bool_result ? "t" : "f");

  bool_result = time_lt(time_in1, time_in2);
  printf("time_lt(%s, %s): %s\n", time_out1, time_out2, bool_result ? "t" : "f");

  time_result = time_make(3, 3, 3);
  char_result = time_out(time_result);
  printf("time_make(3, 3, 3): %s\n", char_result);
  free(char_result);

  bool_result = time_ne(time_in1, time_in2);
  printf("time_ne(%s, %s): %s\n", time_out1, time_out2, bool_result ? "t" : "f");

  char_result = time_out(time_in1);
  printf("time_out(%s): %s\n", time_out1, char_result);
  free(char_result);

  bool_result = time_overlaps(time_in1, time_in2, time_in3, time_in4);
  printf("time_overlaps(%s, %s, %s, %s): %s\n", time_out1, time_out2, time_out3, time_out4, bool_result ? "t" : "f");

  units = text_in("seconds");
  float8_result = time_part(time_in1, units);
  printf("time_part(%s, \"seconds\"): %lf\n", time_out1, float8_result);
  free(units);

  time_result = time_scale(date_in1, -1);
  char_result = time_out(time_result);
  printf("time_scale(%s, -1): %s\n", date_out1, char_result);
  free(char_result);

  time_result = time_smaller(time_in1, time_in2);
  char_result = time_out(time_result);
  printf("time_smaller(%s, %s): %s\n", time_out1, time_out2, char_result);
  free(char_result);

  interv_result = time_to_interval(time_in1);
  char_result = interval_out(interv_result);
  printf("time_to_interval(%s): %s\n", time_out1, char_result);
  free(char_result);

  timetz_result = time_to_timetz(time_in1);
  char_result = timetz_out(timetz_result);
  printf("time_to_timetz(%s): %s\n", time_out1, char_result);
  free(char_result);

  time_result = timestamp_to_time(ts_in);
  char_result = time_out(time_result);
  printf("timestamp_to_time(%s): %s\n", ts_out, char_result);
  free(char_result);

  time_result = timestamptz_to_time(tstz_in);
  char_result = time_out(time_result);
  printf("timestamptz_to_time(%s): %s\n", tstz_out, char_result);
  free(char_result);

  timetz_result = timestamptz_to_timetz(tstz_in);
  char_result = timetz_out(timetz_result);
  printf("timestamptz_to_timetz(%s): %s\n", tstz_out, char_result);
  free(char_result);

  timetz_result = timetz_at_local(timetz_in1);
  char_result = timetz_out(timetz_result);
  printf("timetz_at_local(%s): %s\n", timetz_out1, char_result);
  free(char_result);

  int32_result = timetz_cmp(timetz_in1, timetz_in2);
  printf("timetz_cmp(%s, %s): %d\n", timetz_out1, timetz_out2, int32_result);

  bool_result = timetz_eq(timetz_in1, timetz_in2);
  printf("timetz_eq(%s, %s): %s\n", timetz_out1, timetz_out2, bool_result ? "t" : "f");

  units = text_in("seconds");
  numeric_result = timetz_extract(timetz_in1, units);
  char_result = numeric_out(numeric_result);
  printf("timetz_extract(%s, \"seconds\"): %s\n", timetz_out1, char_result);
  free(units); free(numeric_result); free(char_result);

  bool_result = timetz_ge(timetz_in1, timetz_in2);
  printf("timetz_ge(%s, %s): %s\n", timetz_out1, timetz_out2, bool_result ? "t" : "f");

  bool_result = timetz_gt(timetz_in1, timetz_in2);
  printf("timetz_gt(%s, %s): %s\n", timetz_out1, timetz_out2, bool_result ? "t" : "f");

  uint32_result = timetz_hash(timetz_in1);
  printf("timetz_hash(%s): %ud\n", timetz_out1, uint32_result);

  uint64_result = timetz_hash_extended(timetz_in1, 1);
  printf("timetz_hash_extended(%s, 1): %lu\n", timetz_out1, uint64_result);

  timetz_result = timetz_in("2025-03-01", -1);
  char_result = timetz_out(timetz_result);
  printf("timetz_in(\"2025-03-01\", -1): %s\n", char_result);
  free(char_result);

  timetz_result = timetz_izone(timetz_in1, interv_in);
  char_result = timetz_out(timetz_result);
  printf("timetz_izone(%s, %s): %s\n", timetz_out1, interv_out, char_result);
  free(char_result);

  timetz_result = timetz_larger(timetz_in1, timetz_in2);
  char_result = timetz_out(timetz_result);
  printf("timetz_larger(%s, %s): %s\n", timetz_out1, timetz_out2, char_result);
  free(char_result);

  bool_result = timetz_le(timetz_in1, timetz_in2);
  printf("timetz_le(%s, %s): %s\n", timetz_out1, timetz_out2, bool_result ? "t" : "f");

  bool_result = timetz_lt(timetz_in1, timetz_in2);
  printf("timetz_lt(%s, %s): %s\n", timetz_out1, timetz_out2, bool_result ? "t" : "f");

  bool_result = timetz_ne(timetz_in1, timetz_in2);
  printf("timetz_ne(%s, %s): %s\n", timetz_out1, timetz_out2, bool_result ? "t" : "f");

  char_result = timetz_out(timetz_in1);
  printf("timetz_out(%s): %s\n", timetz_out1, char_result);
  free(char_result);

  bool_result = timetz_overlaps(timetz_in1, timetz_in2, timetz_in3, timetz_in4);
  printf("timetz_overlaps(%s, %s, %s, %s): %s\n", timetz_out1, timetz_out2, timetz_out3, timetz_out4, bool_result ? "t" : "f");

  units = text_in("seconds");
  float8_result = timetz_part(timetz_in1, units);
  printf("timetz_part(%s, \"seconds\"): %lf\n", timetz_out1, float8_result);
  free(units);

  timetz_result = timetz_scale(timetz_in1, -1);
  char_result = timetz_out(timetz_result);
  printf("timetz_scale(%s, -1): %s\n", timetz_out1, char_result);
  free(char_result);

  timetz_result = timetz_smaller(timetz_in1, timetz_in2);
  char_result = timetz_out(timetz_result);
  printf("timetz_smaller(%s, %s): %s\n", timetz_out1, timetz_out2, char_result);
  free(char_result);

  time_result = timetz_to_time(timetz_in1);
  char_result = time_out(time_result);
  printf("timetz_to_time(%s): %s\n", timetz_out1, char_result);
  free(char_result);

  text *zone = text_in("Europe/Brussels");
  timetz_result = timetz_zone(timetz_in1, zone);
  char_result = timetz_out(timetz_result);
  printf("timetz_zone(%s, \"Europe/Brussels\"): %s\n", timetz_out1, char_result);
  free(zone); free(char_result);


  /* Finalize MEOS */
  meos_finalize();

  return 0;
}
