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
 * @brief A simple program that reads from a CSV file synthetic trip data in
 * Brussels generated by the MobilityDB-BerlinMOD generator,
 * https://github.com/MobilityDB/MobilityDB-BerlinMOD
 * and performs a temporal count aggregation.
 *
 * Please read the assumptions made about the input file `berlinmod_trips.csv`
 * in the file `05_berlinmod_disassemble.c` in the same directory.
 *
 * The program can be build as follows
 * @code
 * gcc -Wall -g -I/usr/local/include -o 09_berlinmod_aggregate 09_berlinmod_aggregate.c -L/usr/local/lib -lmeos
 * @endcode
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <meos.h>
#include <meos_geo.h>
#include <meos_internal.h>

typedef struct
{
  int tripid;
  int vehid;
  DateADT day;
  int seq;
  Temporal *trip;
} trip_record;

trip_record trip_rec;

/**
 * Maximum length in characters of a trip in the input data. 
 * This value is set according to the following query executed in the database
 * created by the MobilityDB-BerlinMOD generator.
 * @code
 * SELECT MAX(length(asHexEWKB(trip))) FROM trips;
 * -- 328178
 * @endcode
 */
#define MAX_LENGTH_TRIP 400001
/* Maximum length in characters of a header in the input CSV file */
#define MAX_LENGTH_HEADER 1024
/* Maximum length in characters of a date in the input data */
#define MAX_LENGTH_DATE 12

/* Main program */
int main(void)
{
  /* Variables to read the input CSV file */
  char header_buffer[MAX_LENGTH_HEADER];
  char date_buffer[MAX_LENGTH_DATE];
  char trip_buffer[MAX_LENGTH_TRIP];

  /* Get start time */
  clock_t t;
  t = clock();

  /* Initialize MEOS */
  meos_initialize();

  /* You may substitute the full file path in the first argument of fopen */
  FILE *file = fopen("data/berlinmod_trips.csv", "r");

  if (! file)
  {
    printf("Error opening intput file\n");
    return EXIT_FAILURE;
  }

  int no_records = 0, i;

  /* Read the first line of the file with the headers */
  fscanf(file, "%1023s\n", header_buffer);

  /* Variable keeping the current aggregate state */
  SkipList *state = NULL;
  STBox *extent = NULL;
  Interval *interval = interval_in("1 hour", -1);
  TimestampTz origin = timestamptz_in("2020-06-01", -1);

  /* Continue reading the file */
  do
  {
    int read = fscanf(file, "%d,%d,%10[^,],%d,%400000[^\n]\n",
      &trip_rec.tripid, &trip_rec.vehid, date_buffer, &trip_rec.seq, trip_buffer);
    if (ferror(file))
    {
      printf("Error reading input file\n");
      fclose(file);
      /* Free memory */
      free(trip_rec.trip);
      return EXIT_FAILURE;
    }
    if (read != 5)
    {
      printf("Trip record with missing values\n");
      fclose(file);
      /* Free memory */
      free(trip_rec.trip);
      return EXIT_FAILURE;
    }

    no_records++;
    
    /* Transform the string representing the date into a date value */
    DateADT day = date_in(date_buffer);
    trip_rec.day = day;
    /* Transform the string representing the trip into a temporal value */
    trip_rec.trip = temporal_from_hexwkb(trip_buffer);

    /* Add the current value to the running aggregates */
    STBox *new_extent = tspatial_extent_transfn(extent, trip_rec.trip);
    if (extent)
      free(extent);
    extent = new_extent;
    /* Get the time of the trip at an hour granularity */
    SpanSet *temptime = temporal_time(trip_rec.trip);
    SpanSet *ps = tstzspanset_tprecision(temptime, interval, origin);
    /* Aggregate the time of the trip */
    state = tstzspanset_tcount_transfn(state, ps);
    /* Free memory */
    free(trip_rec.trip); free(temptime); free(ps);
  } while (!feof(file));

  /* Close the input file */
  fclose(file);

  printf("\n%d trip records read\n\n", no_records);

  char *out_str;

  printf("Extent\n");
  printf("------\n\n");
  out_str = stbox_out(extent, 6);
  printf("\%s\n\n", out_str);
  free(out_str);

  printf("Temporal count\n");
  printf("--------------\n\n");
  int seqcount;
  Temporal *tcount = temporal_tagg_finalfn(state);
  Temporal **tcount_seqs = (Temporal **) temporal_sequences(tcount, &seqcount);
  for (i = 0; i < seqcount; i++)
  {
    out_str = tint_out(tcount_seqs[i]);
    printf("\%s\n", out_str);
    free(out_str);
  }

  /* Calculate the elapsed time */
  t = clock() - t;
  double time_taken = ((double) t) / CLOCKS_PER_SEC;
  printf("The program took %f seconds to execute\n", time_taken);

  /* Free memory */
  free(extent);
  free(interval);
  free(tcount);
  for (i = 0; i < seqcount; i++)
    free(tcount_seqs[i]);
  free(tcount_seqs);

  /* Finalize MEOS */
  meos_finalize();

  return EXIT_SUCCESS;
}
