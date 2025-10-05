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
 * @brief A simple program that reads the `pg_conversion.csv` file obtained
 * by exporting the PostgreSQL `pg_conversion` table in CSV format
 *
 * The program can be build as follows
 * @code
 * gcc -Wall -g -I/usr/local/include -o read_pg_conversion read_pg_conversion.c -L/usr/local/lib -lmeos
 * @endcode
 */

#include <string.h>   /* for memset */
#include <stdio.h>    /* for printf */
#include <stdlib.h>   /* for free */
/* Include the MEOS API header */
#include <meos.h>

#define MAX_CONV_LEN  512
#define spibufferlen 512

/*****************************************************************************
 * Definitions for reading the pg_conversion.csv file
 *****************************************************************************/

/* Maximum length in characters of a header record in the input CSV file */
#define MAX_LENGTH_HEADER 1024
/* Location of the pg_conversion.csv file */
#define PG_CONVERSION_CSV "/usr/local/share/pg_conversion.csv"

/**
 * @brief Utility structure to get many potential string representations
 * from pg_conversion query.
 */
typedef struct
{
  int32_t convoid;
  char conname[64];
  char conforencoding[16];
  char contoencoding[16];
  char conproc[64];
} ConvFuncs;

int main()
{
  /* Initialize MEOS */
  meos_initialize();
  const char *convfn = "koi8r_to_mic";
  ConvFuncs conv = GetConvFuncs(convfn);

  /* Output the information found */
  if (conv.oid)
  {
    printf("conname: %s\n", conv.conname);
    printf("conforencoding: %s\n", conv.conforencoding);
    printf("contoencoding: %s\n", conv.contoencoding);
    printf("conproc: %s\n", conv.conproc);
  }

  /* Finalize MEOS */
  meos_finalize();

  /* Free memory */
  free(conv.conname);
  free(conv.conforencoding);
  free(conv.contoencoding);
  free(conv.conproc);

  /* Return */
  return EXIT_SUCCESS;
}

/*****************************************************************************/