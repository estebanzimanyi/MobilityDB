/*****************************************************************************
 *
 * This MobilityDB code is provided under The PostgreSQL License.
 * Copyright (c) 2016-2024, Université libre de Bruxelles and MobilityDB
 * contributors
 *
 * MobilityDB includes portions of PostGIS version 3 source code released
 * under the GNU General Public License (GPLv2 or later).
 * Copyright (c) 2001-2024, PostGIS contributors
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
 * @brief A simple program that for testing the expandable bounding box array.
 *
 * The program can be build as follows
 * @code
 * gcc -Wall -g -I/usr/local/include -o array_expand array_expand.c -L/usr/local/lib -lmeos
 * @endcode
 *
 * The output of the program when MEOS is built with the flag -DDEBUG_EXPAND=1
 * to show debug messages for the expandable data structures is as follows
 * @code
 * * Array -> 8
 * * Array -> 16
 * ** Array -> 32
 * **** Array -> 64
 * ******** Array -> 128
 * Total number of boxes in the array: 64
 * Maximum number of boxes in the array: 128
 * Median bounding box: STBOX XT(((1,1),(33,33)),[2000-01-01 00:00:00+01, 2000-01-02 00:00:00+01])
 * Last bounding box: STBOX XT(((1,1),(64,64)),[2000-01-01 00:00:00+01, 2000-01-02 00:00:00+01])
 * The program took 0.000000 seconds to execute
 * @endcode
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <meos.h>
/* The expandable functions are in the internal MEOS API */
#include <meos_internal.h>

/* Maximum number of boxes */
#define MAX_BOXES 5000000
/* Initial number of boxes allocated when creating an array */
#define INITIAL_NO_BOXES 64
/* Number of boxes in a batch for printing a marker */
#define NO_BOXES_BATCH 50000

/* Main program */
int main(void)
{
  /* Initialize MEOS */
  meos_initialize(NULL, NULL);

  /* Get start time */
  clock_t tm;
  tm = clock();

  /* Create the bounding box array */
  BoxArray *array = boxarray_make(T_STBOX, INITIAL_NO_BOXES);
  /* Iterator variable */
  int i;
  /* Bounding box string buffer */
  char box_str[256];
  
  printf("Total number of boxes generated: %d\n", MAX_BOXES);
  printf("Generating the boxes (one '*' marker every %d boxes)\n",
    NO_BOXES_BATCH);

  for (i = 0; i < MAX_BOXES; i++)
  {
    /* Generate a new box */
    snprintf(box_str, sizeof(box_str), 
      "STBOX XT(((1,1),(%d,%d)),[2000-01-01 00:00:00+01, 2000-01-02 00:00:00+01])", 
      i + 1, i + 1);
    STBox *box = stbox_in(box_str);
    /* Add the box to the array */
    boxarray_add(array, box);
    free(box);
    
    /* Print a '*' marker every X boxes generated */
    if (i % NO_BOXES_BATCH == 0)
    {
      printf("*");
      fflush(stdout);
    }
  }

  /* Print information about the array */
  printf("\nTotal number of boxes in the array: %d\n", array->count);
  printf("Maximum number of boxes in the array: %d\n", array->maxcount);

  /* Print information about the median box */
  void *medbox = malloc(sizeof(STBox));
  memset(medbox, 0, sizeof(STBox));
  boxarray_n(array, array->count / 2, medbox);
  char *medbox_str = stbox_out(medbox, 3);
  printf("Median bounding box: %s\n", medbox_str);

  /* Print information about the last box */
  void *lastbox = malloc(sizeof(STBox));
  memset(lastbox, 0, sizeof(STBox));
  boxarray_n(array, array->count - 1, lastbox);
  char *lastbox_str = stbox_out(lastbox, 3);
  printf("Last bounding box: %s\n", lastbox_str);

  /* Free memory */
  free(medbox); free(medbox_str);
  free(lastbox); free(lastbox_str);
  boxarray_free(array);

  /* Calculate the elapsed time */
  tm = clock() - tm;
  double time_taken = ((double) tm) / CLOCKS_PER_SEC;
  printf("The program took %f seconds to execute\n", time_taken);

  /* Finalize MEOS */
  meos_finalize();

  return 0;
}
