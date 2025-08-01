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
 * @brief A simple program that reads AIS data from a CSV file and outputs
 * a few of these records converted into temporal values.
 *
 * In the input file located in the `data/` subdirectory it is assumed that
 * - The coordinates are given in the WGS84 geographic coordinate system, and
 * - The timestamps are given in the GMT time zone.
 * This simple program does not cope with erroneous inputs, such as missing
 * fields or invalid timestamp values.
 *
 * The program can be build as follows
 * @code
 * gcc -Wall -g -I/usr/local/include -o 02_ais_read 02_ais_read.c -L/usr/local/lib -lmeos
 * @endcode
 */

#include <stdio.h>
#include <stdlib.h>
#include <meos.h>
#include <meos_geo.h>

/* Maximum length in characters of a header record in the input CSV file */
#define MAX_LENGTH_HEADER 1024
/* Maximum length in characters of a point in the input data */
#define MAX_LENGTH_POINT 64
/* Maximum length in characters of a timestamp in the input data */
#define MAX_LENGTH_TIMESTAMP 32

typedef struct
{
  Timestamp T;
  long int MMSI;
  double Latitude;
  double Longitude;
  double SOG;
} AIS_record;

/* Main program */
int main(void)
{
  /* Initialize MEOS */
  meos_initialize();
  meos_initialize_timezone("UTC");

  /* You may substitute the full file path in the first argument of fopen */
  FILE *file = fopen("data/ais_instants.csv", "r");
  if (! file)
  {
    printf("Error opening input file\n");
    meos_finalize();
    return EXIT_FAILURE;
  }

  AIS_record rec;
  int no_records = 0;
  int no_nulls = 0;
  char header_buffer[MAX_LENGTH_HEADER];
  char timestamp_buffer[MAX_LENGTH_TIMESTAMP];

  /* Read the first line of the file with the headers */
  fscanf(file, "%1023s\n", header_buffer);

  /* Continue reading the file */
  do
  {
    int read = fscanf(file, "%31[^,],%ld,%lf,%lf,%lf\n",
      timestamp_buffer, &rec.MMSI, &rec.Latitude, &rec.Longitude, &rec.SOG);
    if (ferror(file))
    {
      printf("Error reading input file\n");
      fclose(file);
      meos_finalize();
      return EXIT_FAILURE;
    }
    if (read != 5)
    {
      printf("Record with missing values ignored\n");
      no_nulls++;
      continue;
    }

    no_records++;

    /* Transform the string representing the timestamp into a timestamp value */
    rec.T = timestamp_in(timestamp_buffer, -1);

    /* Print only 1 out of 1000 records */
    if (no_records % 1000 == 0)
    {
      /* See above the assumptions made wrt the input data in the file */
      GSERIALIZED *gs = geogpoint_make2d(4326, rec.Longitude, rec.Latitude);
      TInstant *inst1 = tpointinst_make(gs, rec.T);
      free(gs);
      char *inst1_out = tspatial_as_text((Temporal *) inst1, 2);

      TInstant *inst2 = tfloatinst_make(rec.SOG, rec.T);
      char *inst2_out = tfloat_out((Temporal *) inst2, 2);
      printf("MMSI: %ld, Location: %s SOG : %s\n",
        rec.MMSI, inst1_out, inst2_out);

      free(inst1); free(inst1_out);
      free(inst2); free(inst2_out);
    }

  } while (!feof(file));

  /* Close the file */
  fclose(file);

  printf("\n%d no_records read.\n%d incomplete records ignored.\n",
    no_records, no_nulls);

  /* Finalize MEOS */
  meos_finalize();

  return EXIT_SUCCESS;
}
