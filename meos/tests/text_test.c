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
 * gcc -Wall -g -I/usr/local/include -o text_test text_test.c -L/usr/local/lib -lmeos
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

  /* Create two boolean values to test the bool functions of the API */
  bool b1 = bool_in("true");
  char *b1_out = bool_out(b1);
  int32 int32_in1 = 32;
  int64 int64_in1 = 64;
  text *text_in1 = text_in("abcdef");
  text *text_in2 = text_in("ghijkl");
  char *text_out1 = text_out(text_in1);
  char *text_out2 = text_out(text_in2);

  /* Create the result types for the bool functions of the API */
  bool bool_result;
  int32 int32_result;
  uint32 uint32_result;
  uint64 uint64_result;
  char *char_result;
  text *text_result;
  
  /* Execute and print the result for the bool functions of the API */

  text_result = bool_to_text(b1);
  char_result = text_out(text_result);
  printf("bool_to_text(%s): %s\n", b1_out, char_result);
  free(text_result); free(char_result);

  uint32_result = char_hash('c');
  printf("char_hash('c'): %d\n", uint32_result);

  uint64_result = char_hash_extended('c', 1);
  printf("char_hash_extended('c', 1): %lud\n", uint64_result);

  text_result = cstring_to_text("azerty");
  char_result = text_out(text_result);
  printf("cstring_to_text(\"azerty\"): %s\n", char_result);
  free(text_result); free(char_result);

  text_result = icu_unicode_version();
  char_result = text_result ? text_out(text_result) : "NULL";
  printf("icu_unicode_version(): %s\n", char_result);
  if (text_result)
  {
    free(text_result);
    free(char_result);
  }

  text_result = int32_to_bin(int32_in1);
  char_result = text_out(text_result);
  printf("int32_to_bin(%d): %s\n", int32_in1, char_result);
  free(text_result); free(char_result);

  text_result = int32_to_hex(int32_in1);
  char_result = text_out(text_result);
  printf("int32_to_hex(%d): %s\n", int32_in1, char_result);
  free(text_result); free(char_result);

  text_result = int32_to_oct(int32_in1);
  char_result = text_out(text_result);
  printf("int32_to_oct(%d): %s\n", int32_in1, char_result);
  free(text_result); free(char_result);

  text_result = int64_to_bin(int64_in1);
  char_result = text_out(text_result);
  printf("int64_to_bin(%ld): %s\n", int64_in1, char_result);
  free(text_result); free(char_result);

  text_result = int64_to_hex(int64_in1);
  char_result = text_out(text_result);
  printf("int64_to_hex(%ld): %s\n", int64_in1, char_result);
  free(text_result); free(char_result);

  text_result = int64_to_oct(int64_in1);
  char_result = text_out(text_result);
  printf("int64_to_oct(%ld): %s\n", int64_in1, char_result);
  free(text_result); free(char_result);

  char_result = text_to_cstring(text_in1);
  printf("text2cstring(%s): %s\n", text_out1, char_result);
  free(char_result);

  text_result = text_cat(text_in1, text_in2);
  char_result = text_out(text_result);
  printf("text_cat(%s, %s): %s\n", text_out1, text_out2, char_result);
  free(text_result); free(char_result);

  uint32_result = text_cmp(text_in1, text_in2, 100);
  printf("text_cmp(%s, %s, 100): %d\n", text_out1, text_out2, uint32_result);

  // text *textarr[2] = {text_in1, text_in2};
  // text_result = text_concat((text **) textarr, 2);
  // char_result = text_out(text_result);
  // printf("text_concat([%s, %s], 2): %s\n", text_out1, text_out2, char_result);
  // free(char_result);

  // text *sep = text_in(", ");
  // text_result = text_concat_ws(textarr, 2, sep);
  // char_result = text_out(text_result);
  // printf("text_concat_ws([%s, %s], 2, \", \"): %s\n", text_out1, text_out2, char_result);
  // free(sep); free(char_result);

  text_result = text_copy(text_in1);
  char_result = text_out(text_result);
  printf("text_copy(%s): %s\n", text_out1, char_result);
  free(char_result);

  bool_result = text_eq(text_in1, text_in2);
  printf("text_eq(%s, %s): %c\n", text_out1, text_out2, bool_result ? 't' : 'n');

  bool_result = text_ge(text_in1, text_in2);
  printf("text_ge(%s, %s): %c\n", text_out1, text_out2, bool_result ? 't' : 'n');

  bool_result = text_gt(text_in1, text_in2);
  printf("text_gt(%s, %s): %c\n", text_out1, text_out2, bool_result ? 't' : 'n');

  uint32_result = text_hash(text_in1, 100);
  printf("text_hash(%s, 100): %d\n", text_out1, uint32_result);

  uint64_result = text_hash_extended(text_in1, 1, 100);
  printf("text_hash_extended(%s, 1, 100): %lud\n", text_out1, uint64_result);

  text_result = text_in("azerty");
  char_result = text_out(text_result);
  printf("text_in(\"azerty\"): %s\n", char_result);
  free(text_result); free(char_result);

  text_result = text_initcap(text_in1);
  char_result = text_out(text_result);
  printf("text_initcap(%s): %s\n", text_out1, char_result);
  free(text_result); free(char_result);

  text_result = text_larger(text_in1, text_in2);
  char_result = text_out(text_result);
  printf("text_larger(%s, %s): %s\n", text_out1, text_out2, char_result);
  free(text_result); free(char_result);

  bool_result = text_le(text_in1, text_in2);
  printf("text_le(%s, %s): %c\n", text_out1, text_out2, bool_result ? 't' : 'n');

  text_result = text_left(text_in1, 3);
  char_result = text_out(text_result);
  printf("text_left(%s, 3): %s\n", text_out1, char_result);
  free(text_result); free(char_result);

  int32_result = text_len(text_in1);
  printf("text_len(%s): %d\n", text_out1, int32_result);

  text_result = text_lower(text_in1);
  char_result = text_out(text_result);
  printf("text_lower(%s): %s\n", text_out1, char_result);
  free(text_result); free(char_result);

  bool_result = text_lt(text_in1, text_in2);
  printf("text_lt(%s, %s): %c\n", text_out1, text_out2, bool_result ? 't' : 'n');

  bool_result = text_ne(text_in1, text_in2);
  printf("text_ne(%s, %s): %c\n", text_out1, text_out2, bool_result ? 't' : 'n');

  char_result = text_out(text_in1);
  printf("text_out(%s): %s\n", text_out1, char_result);
  free(char_result);

  text_result = text_overlay(text_in1, text_in2, 2, 2);
  char_result = text_out(text_result);
  printf("text_overlay(%s, %s, 2, 2): %s\n", text_out1, text_out2, char_result);
  free(text_result); free(char_result);

  text_result = text_overlay_no_len(text_in1, text_in2, 2);
  char_result = text_out(text_result);
  printf("text_overlay_no_len(%s, %s, 2): %s\n", text_out1, text_out2, char_result);
  free(text_result); free(char_result);

  bool_result = text_pattern_ge(text_in1, text_in2);
  printf("text_pattern_ge(%s, %s): %c\n", text_out1, text_out2, bool_result ? 't' : 'n');

  bool_result = text_pattern_gt(text_in1, text_in2);
  printf("text_pattern_gt(%s, %s): %c\n", text_out1, text_out2, bool_result ? 't' : 'n');

  bool_result = text_pattern_le(text_in1, text_in2);
  printf("text_pattern_le(%s, %s): %c\n", text_out1, text_out2, bool_result ? 't' : 'n');

  bool_result = text_pattern_lt(text_in1, text_in2);
  printf("text_pattern_lt(%s, %s): %c\n", text_out1, text_out2, bool_result ? 't' : 'n');

  text *search = text_in("c");
  int32_result = text_pos(text_in1, search);
  printf("text_pos(%s, \"c\"): %s\n", text_out1, char_result);

  text *from = text_in("c");
  text *to = text_in("X");
  char *from_out = text_to_cstring(from);
  char *to_out = text_to_cstring(to);
  text_result = text_replace(text_in1, from, to);
  char_result = text_out(text_result);
  printf("text_replace(%s, %s, %s): %s\n", text_out1, from_out, to_out, char_result);
  free(from); free(from_out); free(to); free(to_out);
  free(text_result); free(char_result);

  text_result = text_reverse(text_in1);
  char_result = text_out(text_result);
  printf("text_reverse(%s): %s\n", text_out1, char_result);
  free(text_result); free(char_result);

  text_result = text_right(text_in1, 3);
  char_result = text_out(text_result);
  printf("text_right(%s, 3): %s\n", text_out1, char_result);
  free(text_result); free(char_result);

  text_result = text_smaller(text_in1, text_in2);
  char_result = text_out(text_result);
  printf("text_smaller(%s, %s): %s\n", text_out1, text_out2, char_result);
  free(text_result); free(char_result);

  text *text_sep = text_in(", ");
  text_result = text_split_part(text_in1, text_sep, 1);
  char_result = text_out(text_result);
  printf("text_split_part(%s, \", \", 1): %s\n", text_out1, char_result);
  free(text_result); free(char_result);

  bool_result = text_starts_with(text_in1, text_in2);
  printf("text_starts_with(%s, %s): %c\n", text_out1, text_out2, bool_result ? 't' : 'n');

  text_result = text_substr(text_in1, 3, 2);
  char_result = text_out(text_result);
  printf("text_substr(%s, 3, 2): %s\n", text_out1, char_result);
  free(text_result); free(char_result);

  text_result = text_substr_no_len(text_in1, 3);
  char_result = text_out(text_result);
  printf("text_substr_no_len(%s, 3): %s\n", text_out1, char_result);
  free(text_result); free(char_result);

  text_result = text_upper(text_in1);
  char_result = text_out(text_result);
  printf("text_upper(%s): %s\n", text_out1, char_result);
  free(text_result); free(char_result);

  text_result = textcat_text_text(text_in1, text_in2);
  char_result = text_out(text_result);
  printf("textcat_text_text(%s, %s): %s\n", text_out1, text_out2, char_result);
  free(text_result); free(char_result);

  // bool_result = unicode_assigned(text_in1);
  // printf("unicode_assigned(%s): %c\n", text_out1, bool_result ? 't' : 'n');

  // text *fmt = text_in("NFC");
  // bool_result = unicode_is_normalized(text_in1, fmt);
  // printf("unicode_is_normalized(%s, \"NFC\"): %c\n", text_out1, bool_result ? 't' : 'n');

  // text_result = unicode_normalize_func(text_in1, fmt);
  // char_result = text_out(text_result);
  // printf("unicode_normalize_func(%s, \"NFC\"): %s\n", text_out1, char_result);
  // free(fmt); free(text_result); free(char_result);

  text_result = unicode_version();
  char_result = text_out(text_result);
  printf("unicode_version(): %s\n", char_result);
  free(text_result); free(char_result);

  text_result = unistr(text_in1);
  char_result = text_out(text_result);
  printf("unistr(%s): %s\n", text_out1, char_result);
  free(text_result); free(char_result);

  /* Finalize MEOS */
  meos_finalize();

  return 0;
}
