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
 * @file
 * @brief MEOS global variables
 */

/* PostgreSQL */
#include <postgres.h>
#include <utils/datetime.h>
/* GSL */
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
/* Proj */
#include <proj.h>
/* MEOS */
#include <meos.h>

/*****************************************************************************
 * MEOS global variables 
 *****************************************************************************/

/* File /meos/src/general/error.c */

/**
 * @brief Global variable that keeps the last error number
 */
int MEOS_ERR_NO = 0;

/**
 * @brief Global variable that keeps the error handler function
 */
void (*MEOS_ERROR_HANDLER)(int, int, const char *) = NULL;

/*****************************************************************************/

/* File /meos/src/general/meos.c */

/* Global variables for GSL */

bool MEOS_GSL_INITIALIZED = false;
gsl_rng *MEOS_GENERATION_RNG = NULL;
gsl_rng *MEOS_AGGREGATION_RNG = NULL;

/* Global variables keeping Proj context */

PJ_CONTEXT *MEOS_PJ_CONTEXT = NULL;

#if MEOS

/* Global variables with default definitions taken from globals.c */

int DateStyle = USE_ISO_DATES;
int DateOrder = DATEORDER_MDY;
int IntervalStyle = INTSTYLE_POSTGRES;

#endif /* MEOS */

/*****************************************************************************/

