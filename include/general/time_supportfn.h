/*****************************************************************************
 *
 * This MobilityDB code is provided under The PostgreSQL License.
 * Copyright (c) 2016-2022, Université libre de Bruxelles and MobilityDB
 * contributors
 *
 * MobilityDB includes portions of PostGIS version 3 source code released
 * under the GNU General Public License (GPLv2 or later).
 * Copyright (c) 2001-2022, PostGIS contributors
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
 * @file temporal_supportfn.h
 * Support functions for temporal types.
 */

#ifndef __TIME_SUPPORTFN_H__
#define __TIME_SUPPORTFN_H__

#include <postgres.h>
#include <fmgr.h>
#include <catalog/pg_type.h>

/*
* Depending on the function, we will deploy different index enhancement
* strategies. Containment functions can use a more strict index strategy
* than overlapping functions. We store the metadata to drive these choices
* in the IndexableFunctions array.
*/
typedef struct
{
  const char *fn_name;  /* Name of the function */
  uint16_t index;       /* Position of the strategy in the arrays */
  uint8_t nargs;        /* Expected number of function arguments */
  uint8_t expand_arg;   /* Radius argument for "within distance" search */
} IndexableFunction;

extern Datum time_supportfn(PG_FUNCTION_ARGS);

extern Oid opFamilyAmOid(Oid opfamilyoid);
extern bool func_needs_index(Oid funcid, const IndexableFunction *idxfn,
  IndexableFunction *result);

/*****************************************************************************/

#endif