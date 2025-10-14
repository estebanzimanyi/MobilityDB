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
 * gcc -Wall -g -I/usr/local/include -o bool_test bool_test.c -L/usr/local/lib -lmeos
 * @endcode
 */

#include <stdio.h>
#include <stdlib.h>
#include <meos.h>
#include <pg_bool.h>
#include <pg_date.h>
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
  DateADT date_in1 = date_in("2025-03-01");
  DateADT date_in2 = date_in("2025-04-01");
  char *date_out1 = date_out(date_in1);
  char *date_out2 = date_out(date_in2);
  Timestamp ts_in = timestamp_in("2025-03-01 08:00:00", -1);
  char *ts_out = timestamp_out(ts_in);
  Timestamp tstz_in = timestamptz_in("2025-03-01 08:00:00", -1);
  char *tstz_out = timestamptz_out(tstz_in);
  Interval *interv_in = interval_in("8 hours", -1);
  char *interv_out = interval_out(interv_in);

  /* Create the result types for the bool functions of the API */
  bool bool_result;
  int32 int32_result;
  uint32 uint32_result;
  uint64 uint64_result;
  Numeric numeric_result;
  DateADT date_result;
  char *char_result;
  Timestamp ts_result;
  TimestampTz tstz_result;

  /* Execute and print the result for the bool functions of the API */

  date_result = add_date_int(date_in1, 15);
  char_result = date_out(date_result);
  printf("add_date_int(%s, 15): %s\n", date_out1, char_result);
  free(char_result);

  date_result = add_date_interval(date_in1, interv_in);
  char_result = date_out(date_result);
  printf("add_date_interval(%s, %s): %s\n", date_out1, interv_out, char_result);
  free(char_result);

  int32_result = cmp_date_timestamp(date_in1, tstz_in);
  printf("cmp_date_timestamp(%s, %s): %d\n", date_out1, tstz_out, int32_result);

  int32_result = cmp_date_date(date_in1, date_in2);
  printf("cmp_date_date(%s, %s): %d\n", date_out1, date_out2, int32_result);

  int32_result = cmp_date_timestamptz(date_in1, tstz_in);
  printf("cmp_date_timestamptz(%s, %s): %d\n", date_out1, tstz_out, int32_result);

  int32_result = cmp_timestamp_date(ts_in, date_in1);
  printf("cmp_timestamp_date(%s, %s): %d\n", ts_out, date_out1, int32_result);

  int32_result = cmp_timestamptz_date(tstz_in, date_in1);
  printf("cmp_timestamptz_date(%s, %s): %d\n", tstz_out, date_out1, int32_result);

  text *units = text_in("days");
  numeric_result = date_extract(date_in1, units);
  char_result = numeric_out(numeric_result);
  printf("date_extract(%s, \"days\"): %s\n", date_out1, char_result);
  free(units); free(numeric_result); free(char_result);

  uint32_result = date_hash(date_in1);
  printf("date_hash(%s): %ud\n", date_out1, uint32_result);

  uint64_result = date_hash_extended(date_in1, 1);
  printf("date_hash_extended(%s, 1): %lu\n", date_out1, uint64_result);

  bool_result = date_is_finite(date_in1);
  printf("date_is_finite(%s): %c\n", date_out1, bool_result ? 't' : 'f');

  ts_result = date_to_timestamp(date_in1);
  char_result = timestamp_out(ts_result);
  printf("date_to_timestamp(%s): %s\n", date_out1, char_result);
  free(char_result);

  tstz_result = date_to_timestamptz(date_in1);
  char_result = timestamptz_out(tstz_result);
  printf("date_to_timestamptz(%s): %s\n", date_out1, char_result);
  free(char_result);

  bool_result = eq_date_date(date_in1, date_in2);
  printf("eq_date_date(%s, %s): %c\n", date_out1, date_out2, bool_result ? 't' : 'f');

  bool_result = eq_date_timestamp(date_in1, tstz_in);
  printf("eq_date_timestamp(%s, %s): %c\n", date_out1, tstz_out, bool_result ? 't' : 'f');

  bool_result = eq_date_timestamptz(date_in1, tstz_in);
  printf("eq_date_timestamptz(%s, %s): %c\n", date_out1, tstz_out, bool_result ? 't' : 'f');

  bool_result = eq_timestamp_date(ts_in, date_in1);
  printf("eq_timestamp_date(%s, %s): %c\n", ts_out, date_out1, bool_result ? 't' : 'f');

  bool_result = eq_timestamptz_date(tstz_in, date_in1);
  printf("eq_timestamptz_date(%s, %s): %c\n", tstz_out, date_out1, bool_result ? 't' : 'f');

  bool_result = ge_date_date(date_in1, date_in2);
  printf("ge_date_date(%s, %s): %c\n", date_out1, date_out2, bool_result ? 't' : 'f');

  bool_result = ge_date_timestamp(date_in1, tstz_in);
  printf("ge_date_timestamp(%s, %s): %c\n", date_out1, tstz_out, bool_result ? 't' : 'f');

  bool_result = ge_date_timestamptz(date_in1, tstz_in);
  printf("ge_date_timestamptz(%s, %s): %c\n", date_out1, tstz_out, bool_result ? 't' : 'f');

  bool_result = ge_timestamp_date(ts_in, date_in1);
  printf("ge_timestamp_date(%s, %s): %c\n", ts_out, date_out1, bool_result ? 't' : 'f');

  bool_result = ge_timestamptz_date(tstz_in, date_in1);
  printf("ge_timestamptz_date(%s, %s): %c\n", tstz_out, date_out1, bool_result ? 't' : 'f');

  bool_result = gt_date_date(date_in1, date_in2);
  printf("gt_date_date(%s, %s): %c\n", date_out1, date_out2, bool_result ? 't' : 'f');

  bool_result = gt_date_timestamp(date_in1, tstz_in);
  printf("gt_date_timestamp(%s, %s): %c\n", date_out1, tstz_out, bool_result ? 't' : 'f');

  bool_result = gt_date_timestamptz(date_in1, tstz_in);
  printf("gt_date_timestamptz(%s, %s): %c\n", date_out1, tstz_out, bool_result ? 't' : 'f');

  bool_result = gt_timestamp_date(ts_in, date_in1);
  printf("gt_timestamp_date(%s, %s): %c\n", ts_out, date_out1, bool_result ? 't' : 'f');

  bool_result = gt_timestamptz_date(tstz_in, date_in1);
  printf("gt_timestamptz_date(%s, %s): %c\n", tstz_out, date_out1, bool_result ? 't' : 'f');

  bool_result = le_date_date(date_in1, date_in2);
  printf("le_date_date(%s, %s): %c\n", date_out1, date_out2, bool_result ? 't' : 'f');

  bool_result = le_date_timestamp(date_in1, tstz_in);
  printf("le_date_timestamp(%s, %s): %c\n", date_out1, tstz_out, bool_result ? 't' : 'f');

  bool_result = le_date_timestamptz(date_in1, tstz_in);
  printf("le_date_timestamptz(%s, %s): %c\n", date_out1, tstz_out, bool_result ? 't' : 'f');

  bool_result = le_timestamp_date(ts_in, date_in1);
  printf("le_timestamp_date(%s, %s): %c\n", ts_out, date_out1, bool_result ? 't' : 'f');

  bool_result = le_timestamptz_date(tstz_in, date_in1);
  printf("le_timestamptz_date(%s, %s): %c\n", tstz_out, date_out1, bool_result ? 't' : 'f');

  bool_result = lt_date_date(date_in1, date_in2);
  printf("lt_date_date(%s, %s): %c\n", date_out1, date_out2, bool_result ? 't' : 'f');

  bool_result = lt_date_timestamp(date_in1, tstz_in);
  printf("lt_date_timestamp(%s, %s): %c\n", date_out1, tstz_out, bool_result ? 't' : 'f');

  bool_result = lt_date_timestamptz(date_in1, tstz_in);
  printf("lt_date_timestamptz(%s, %s): %c\n", date_out1, tstz_out, bool_result ? 't' : 'f');

  bool_result = lt_timestamp_date(ts_in, date_in1);
  printf("lt_timestamp_date(%s, %s): %c\n", ts_out, date_out1, bool_result ? 't' : 'f');

  bool_result = lt_timestamptz_date(tstz_in, date_in1);
  printf("lt_timestamptz_date(%s, %s): %c\n", tstz_out, date_out1, bool_result ? 't' : 'f');

  int32_result = minus_date_date(date_in1, date_in2);
  printf("minus_date_date(%s, %s): %d\n", date_out1, date_out2, int32_result);

  date_result = minus_date_int(date_in1, 3);
  char_result = date_out(date_result);
  printf("minus_date_int(%s, 3): %s\n", date_out1, char_result);
  free(char_result);

  date_result = minus_date_interval(date_in1, interv_in);
  char_result = date_out(date_result);
  printf("minus_date_interval(%s, %s): %s\n", date_out1, interv_out, char_result);
  free(char_result);

  bool_result = ne_date_date(date_in1, date_in2);
  printf("ne_date_date(%s, %s): %c\n", date_out1, date_out2, bool_result ? 't' : 'f');

  bool_result = ne_date_timestamp(date_in1, tstz_in);
  printf("ne_date_timestamp(%s, %s): %c\n", date_out1, tstz_out, bool_result ? 't' : 'f');

  bool_result = ne_date_timestamptz(date_in1, tstz_in);
  printf("ne_date_timestamptz(%s, %s): %c\n", date_out1, tstz_out, bool_result ? 't' : 'f');

  bool_result = ne_timestamp_date(ts_in, date_in1);
  printf("ne_timestamp_date(%s, %s): %c\n", ts_out, date_out1, bool_result ? 't' : 'f');

  bool_result = ne_timestamptz_date(tstz_in, date_in1);
  printf("ne_timestamptz_date(%s, %s): %c\n", tstz_out, date_out1, bool_result ? 't' : 'f');

  date_result = date_in("2025-03-01");
  char_result = date_out(date_result);
  printf("date_in(\"2025-03-01\"): %s\n", char_result);
  free(char_result);

  date_result = date_larger(date_in1, date_in2);
  char_result = date_out(date_result);
  printf("date_larger(%s, %s): %s\n", date_out1, date_out2, char_result);
  free(char_result);

  date_result = date_make(2025, 3, 1);
  char_result = date_out(date_result);
  printf("date_make(2025, 3, 1): %s\n", char_result);
  free(char_result);

  date_result = date_smaller(date_in1, date_in2);
  char_result = date_out(date_result);
  printf("date_smaller(%s, %s): %s\n", date_out1, date_out2, char_result);
  free(char_result);

  char_result = date_out(date_in1);
  printf("date_out(%s): %s\n", date_out1, char_result);
  free(char_result);

  date_result = timestamp_to_date(ts_in);
  char_result = date_out(date_result);
  printf("timestamp_to_date(%s): %s\n", ts_out, char_result);
  free(char_result);

  date_result = timestamptz_to_date(tstz_in);
  char_result = date_out(date_result);
  printf("timestamptz_to_date(%s): %s\n", tstz_out, char_result);
  free(char_result);

  /* Finalize MEOS */
  meos_finalize();

  return 0;
}
