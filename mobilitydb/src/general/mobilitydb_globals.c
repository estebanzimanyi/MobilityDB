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
 * @brief MobilityDB global variables
 */

/* PostgreSQL */
#include <postgres.h>
#include <fmgr.h>
/* MobilityDB */
#include "general/meos_catalog.h"
/* MobilityDB */
#include "pg_general/meos_catalog.h"
#include "pg_general/temporal_analyze.h"

/*****************************************************************************/

/* File /mobilitydb/src/general/meos_catalog.c */

/* Global to hold all the run-time constants */

mobilitydb_constants *MOBILITYDB_CONSTANTS = NULL;

/*****************************************************************************/

/* File /mobilitydb/src/general/temporal.c */

/**
 * @brief Global variable that saves the PostgreSQL fcinfo
 *
 * This is needed when we need to change the PostgreSQL context, for example,
 * in PostGIS functions such as #transform, #geography_distance, or
 * #geography_azimuth that need to access the proj cache
 */
FunctionCallInfo MOBDB_PG_FCINFO;

/*****************************************************************************/

/* File /mobilitydb/src/general/temporal_analyze.c */

/*
 * Global variable for extra data for the compute_stats function.
 * While statistic functions are running, we keep a pointer to the extra data
 * here for use by assorted subroutines.  The functions doesn't currently need
 * to be re-entrant, so avoiding this is not worth the extra notational cruft
 * that would be needed.
 */
TemporalAnalyzeExtraData *temporal_extra_data;

/*****************************************************************************/

