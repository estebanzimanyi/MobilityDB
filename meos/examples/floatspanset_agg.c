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
 * @brief A simple program that reads float spanset values from a CSV file and
 * aggregates them into a given number of groups.
 *
 * The program can be build as follows
 * @code
 * gcc -Wall -g -I/usr/local/include -o floatspanset_agg floatspanset_agg.c -L/usr/local/lib -lmeos
 * @endcode
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <meos.h>
/* The aggregate functions are in the internal MEOS API */
#include <meos_internal.h>

/* Maximum length in characters of a header record in the input CSV file */
#define MAX_LENGTH_HEADER 1024
/* Maximum length in characters of a span set in the input data */
#define MAX_LENGTH_SPANSET 1024
/* Number of groups for accumulating the input span sets */
#define NUMBER_GROUPS 10

typedef struct
{
  int k;
  SpanSet *ss;
} floatspanset_record;

/* Main program */
int main(void)
{
  /* Initialize MEOS */
  meos_initialize();

  /* Get start time */
  clock_t t;
  t = clock();

  /* Array of state values for aggregating the spans */
  SpanSet *state[NUMBER_GROUPS] = {0};

  /* Substitute the full file path in the first argument of fopen */
  FILE *file = fopen("data/floatspanset.csv", "r");

  if (! file)
  {
    printf("Error opening input file\n");
    return EXIT_FAILURE;
  }

  floatspanset_record rec;
  int no_records = 0;
  int no_nulls = 0;
  char header_buffer[MAX_LENGTH_HEADER];
  char spanset_buffer[MAX_LENGTH_SPANSET];

  /* Read the first line of the file with the headers */
  fscanf(file, "%1023s\n", header_buffer);

  /* Continue reading the file */
  do
  {
    int read = fscanf(file, "%d,\"%1023[^\"]\"\n", &rec.k, spanset_buffer);
    if (ferror(file))
    {
      printf("Error reading input file\n");
      fclose(file);
    }
    if (read != 2)
    {
      printf("Record with missing values ignored\n");
      no_nulls++;
      continue;
    }

    no_records++;

    /* Transform the string representing the span set into a span set value */
    rec.ss = floatspanset_in(spanset_buffer);

    state[rec.k%NUMBER_GROUPS] = 
      spanset_union_transfn(state[rec.k%NUMBER_GROUPS], rec.ss);

    /* Output the float spanset value read */
    char *spanset_out = floatspanset_out(rec.ss, 3);
    printf("k: %d, spanset: %s\n", rec.k, spanset_out);
    free(spanset_out);
    free(rec.ss);
  } while (!feof(file));

  /* Close the file */
  fclose(file);

  printf("\n%d records read.\n%d incomplete records ignored.\n",
    no_records, no_nulls);

  /* Compute the final result */
  for (int i = 0; i < NUMBER_GROUPS; i++)
  {
    SpanSet *final = spanset_union_finalfn(state[i]);
    /* Print the accumulated span set */
    printf("----------\n");
    printf("Group: %d\n", i + 1);
    printf("----------\n");
    char *spanset_out = floatspanset_out(final, 3);
    printf("spanset: %s\n", spanset_out);
    free(spanset_out);
    free(state[i]);
    free(final);
  }

  /* Calculate the elapsed time */
  t = clock() - t;
  double time_taken = ((double) t) / CLOCKS_PER_SEC;
  printf("The program took %f seconds to execute\n", time_taken);

  /* Finalize MEOS */
  meos_finalize();

  return EXIT_SUCCESS;
}
