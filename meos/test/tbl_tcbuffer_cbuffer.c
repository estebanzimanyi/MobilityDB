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
 * @brief A simple program that reads two CSV files, the first one containing
 * temporal circular buffers and the second containing circular buffers and
 * applies a function to the temporal circular buffer and the circular buffers.
 *
 * The corresponding SQL query would be
 * @code
 * SELECT numInstants(<FUNCTION>(temp, cb))
 * FROM tbl_tcbuffer, tbl_cbuffer
 * @endcode
 *
 * The program can be build as follows
 * @code
 * gcc -Wall -g -I/usr/local/include -o tbl_tcbuffer_cbuffer tbl_tcbuffer_cbuffer.c -L/usr/local/lib -lmeos
 * @endcode
 * The following command can be used to test for memory leaks
 * @code
 * valgrind -s --leak-check=full --show-leak-kinds=all ./tbl_tcbuffer_cbuffer
 * @endcode
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <meos.h>
#include <meos_geo.h>
#include <meos_cbuffer.h>

/* Maximum length in characters of a header record in the input CSV file1 */
#define MAX_LEN_HEADER 1024
/* Maximum length in characters of a circular buffer in the input data as computed by
 * the following query on the corresponding table
 * SELECT MAX(length(cb::text)) FROM tbl_cbuffer;
 * -- 77
 */
#define MAX_LEN_CBUFFER 128
/* Maximum length in characters of a circular buffer in the input
 * data as computed by the following query on the corresponding table
 * SELECT MAX(length(temp::text)) FROM tbl_tcbuffer;
 * -- 7449
 */
#define MAX_LEN_TCBUFFER 7501

/* Main program */
int main(void)
{
  /* Initialize MEOS */
  meos_initialize();
  meos_initialize_timezone("UTC");

  /* You may substitute the full file1 path in the first argument of fopen */
  FILE *file1 = fopen("csv/tbl_tcbuffer.csv", "r");
  if (! file1)
  {
    printf("Error opening first input file\n");
    return 1;
  }

  /* You may substitute the full file1 path in the first argument of fopen */
  FILE *file2 = fopen("csv/tbl_cbuffer.csv", "r");
  if (! file2)
  {
    printf("Error opening second input file\n");
    fclose(file2);
    return 1;
  }

  char header_buffer[MAX_LEN_HEADER];
  char line1[MAX_LEN_TCBUFFER];
  char line2[MAX_LEN_CBUFFER];
  char tcbuffer_buffer[MAX_LEN_TCBUFFER];
  char cbuffer_buffer[MAX_LEN_CBUFFER];

  /* Read the first line of the first file with the headers */
  if (!fgets(header_buffer, sizeof(header_buffer), file1))
  {
    printf("Error reading header of file1\n");
    fclose(file1);
    fclose(file2);
    return 1;
  }

  /* Continue reading the first file */
  while (fgets(line1, sizeof(line1), file1) != NULL)
  {
    int k1;

    if (sscanf(line1, "%d,%7500[^\n]", &k1, tcbuffer_buffer) != 2)
    {
      printf("Skipping malformed line in file1\n");
      continue;
    }

    /* Transform the string read into a tcbuffer value */
    Temporal *temp = tcbuffer_in(tcbuffer_buffer);

    /* Rewind the second file to the beginning */
    rewind(file2);

    /* Read the first line of the second file with the headers */
    if (!fgets(header_buffer, sizeof(header_buffer), file2))
    {
      printf("Error reading header of file2\n");
      free(temp);
      fclose(file1);
      fclose(file2);
      return 1;
    }

    /* Loop through file2 */
    while (fgets(line2, sizeof(line2), file2) != NULL)
    {
      int k2;

      if (sscanf(line2, "%d,%12000[^\n]", &k2, cbuffer_buffer) != 2)
      {
        printf("Skipping malformed line in file2\n");
        continue;
      }

      /* Print only 1 out of 100 records */
      if (k1 % 10 == 0 && k2 % 10 == 0)
      {
        /* Transform the string read into a cbuffer value */
        Cbuffer *cb = cbuffer_in(cbuffer_buffer);

        /* Compute the function */
        // bool result = ever_eq_tcbuffer_cbuffer(temp, cb);
        bool result = always_eq_tcbuffer_cbuffer(temp, cb);
        // bool result = econtains_tcbuffer_cbuffer(temp, cb);
        // bool result = ecovers_tcbuffer_cbuffer(temp, cb);
        // bool result = edisjoint_tcbuffer_cbuffer(temp, cb);
        // bool result = edwithin_tcbuffer_cbuffer(temp, cb, 10);
        // bool result = eintersects_tcbuffer_cbuffer(temp, cb);
        // bool result = etouches_tcbuffer_cbuffer(temp, cb);
        // bool result = acontains_tcbuffer_cbuffer(temp, cb);
        // bool result = acovers_tcbuffer_cbuffer(temp, cb);
        // bool result = adisjoint_tcbuffer_cbuffer(temp, cb);
        // bool result = adwithin_tcbuffer_cbuffer(temp, cb, 10);
        // bool result = aintersects_tcbuffer_cbuffer(temp, cb);
        // bool result = atouches_tcbuffer_cbuffer(temp, cb);
        // Temporal *result = tcontains_tcbuffer_cbuffer(temp, cb, false, false);
        // Temporal *result = tcovers_tcbuffer_cbuffer(temp, cb, false, false);
        // Temporal *result = tdisjoint_tcbuffer_cbuffer(temp, cb, false, false);
        // Temporal *result = tdwithin_tcbuffer_cbuffer(temp, cb, 10, false, false);
        // Temporal *result = tintersects_tcbuffer_cbuffer(temp, cb, false, false);
        // Temporal *result = ttouches_tcbuffer_cbuffer(temp, cb, false, false);
        // double result = nad_tcbuffer_cbuffer(temp, cb);
        // TInstant *result = nai_tcbuffer_cbuffer(temp, cb);
        // GSERIALIZED *result = shortestline_tcbuffer_cbuffer(temp, cb);
        // Temporal *result = tdistance_tcbuffer_cbuffer(temp, cb);
        /* Get the number of instants of the result */
        // int count = result ? temporal_num_instants(result) : 0;
        // char *result_out = geo_as_ewkt(result, 3);
        // free(result);
        free(cb);

        printf("k1: %d, k2: %d: Result: %s\n",
          k1, k2, result ? "true" : "false");
        // printf("k1: %d, k2: %d: Number of instants of the result: %d\n",
          // k1, k2, count);
        // printf("k1: %d, k2: %d: Result: %lf\n",
          // k1, k2, result);
        // printf("k1: %d, k2: %d: Result: %s\n",
          // k1, k2, result_out);
        // free(result_out);
      }
    }

    free(temp);

  /* Close the files */
    if (ferror(file2))
    {
      printf("Error reading input file2\n");
      fclose(file1);
      fclose(file2);
      return 1;
    }
  }

  if (ferror(file1))
  {
    printf("Error reading input file1\n");
    fclose(file1);
    fclose(file2);
    return 1;
  }

  fclose(file1);
  fclose(file2);

  /* Finalize MEOS */
  meos_finalize();

  return 0;
}
