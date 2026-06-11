/*****************************************************************************
 *
 * This MobilityDB code is provided under The PostgreSQL License.
 * Copyright (c) 2016-2026, Université libre de Bruxelles and MobilityDB
 * contributors
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
 * @brief Standalone (MEOS) round-trip exercise for the vendored JSONB type.
 *
 * This guards the standalone jsonb serialization path (pgtypes
 * convertToJsonb / JsonbValueToJsonb), which the SQL pg_regress suite does
 * NOT cover: the in-extension build uses PostgreSQL's own jsonb, so a
 * regression in the vendored standalone writer is invisible there. A broken
 * writer (e.g. copying the binary jsonb buffer with a NUL-terminated string
 * copy) leaves the container header uninitialised and jsonb_out reports
 * "unknown type of jsonb container"; this test then aborts and fails CTest.
 *
 * The base jsonb in/out surface is declared here explicitly (it lives in the
 * pgtypes pg_json.h header, which is not yet self-contained for direct
 * inclusion) so the test depends only on the public libmeos symbols.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <meos.h>

typedef struct varlena Jsonb;
extern Jsonb *jsonb_in(const char *str);
extern char *jsonb_out(const Jsonb *jb);

/* Parse a jsonb value, output it, and check the round-tripped text. */
static void
check(const char *in, const char *expected)
{
  Jsonb *jb = jsonb_in(in);
  if (jb == NULL)
  {
    fprintf(stderr, "json_test: jsonb_in returned NULL for: %s\n", in);
    exit(1);
  }
  char *out = jsonb_out(jb);
  if (out == NULL || strcmp(out, expected) != 0)
  {
    fprintf(stderr, "json_test: jsonb round-trip mismatch\n"
      "  in:       %s\n  expected: %s\n  got:      %s\n",
      in, expected, out ? out : "(null)");
    exit(1);
  }
}

int main(void)
{
  meos_initialize();

  /* Object, array, nested, scalars -- each must survive the standalone
   * jsonb writer/reader round-trip. (jsonb sorts object keys by length
   * then bytes, hence the reordering below.) */
  check("{\"a\": 1}", "{\"a\": 1}");
  check("{\"geom\": \"Point(1 1)\", \"n\": 42}", "{\"n\": 42, \"geom\": \"Point(1 1)\"}");
  check("[1, 2, 3]", "[1, 2, 3]");
  check("{\"a\": {\"b\": [true, null, 3.14]}}", "{\"a\": {\"b\": [true, null, 3.14]}}");
  check("\"scalar\"", "\"scalar\"");
  check("123", "123");

  meos_finalize();
  printf("json_test: OK\n");
  return 0;
}
