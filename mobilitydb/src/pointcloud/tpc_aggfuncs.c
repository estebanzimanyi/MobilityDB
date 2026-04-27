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
 * @brief PG wrappers for TPCBox-based aggregate functions over the
 *   pgPointCloud temporal types — currently @c extent for tpcpoint /
 *   tpcpatch / tpcbox. Mirrors the stbox / tspatial extent surface in
 *   @c mobilitydb/src/geo/tgeo_aggfuncs.c.
 */

/* PostgreSQL */
#include <postgres.h>
#include <fmgr.h>
/* MEOS */
#include <meos.h>
#include <meos_internal.h>
#include <meos_pointcloud.h>
#include "pointcloud/tpc_boxops.h"
#include "pointcloud/tpcbox.h"          /* PG_GETARG_TPCBOX_P, etc. */
#include "temporal/temporal.h"

/*****************************************************************************
 * Extent aggregation
 *
 * One transfn covers tpcpoint and tpcpatch (the second arg is a generic
 * Temporal, dispatched by temporal_set_bbox). A separate transfn handles
 * tpcbox-with-tpcbox aggregation, plus the parallel-combinefn.
 *****************************************************************************/

PGDLLEXPORT Datum Tpc_extent_transfn(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tpc_extent_transfn);
/**
 * @ingroup mobilitydb_pointcloud_agg
 * @brief Transition function for the extent aggregate over tpcpoint /
 *   tpcpatch values.
 * @sqlfn extent()
 */
Datum
Tpc_extent_transfn(PG_FUNCTION_ARGS)
{
  TPCBox *state = PG_ARGISNULL(0) ? NULL : PG_GETARG_TPCBOX_P(0);
  Temporal *temp = PG_ARGISNULL(1) ? NULL : PG_GETARG_TEMPORAL_P(1);
  TPCBox *result = tpcbox_extent_transfn(state, temp);
  if (! result)
    PG_RETURN_NULL();
  PG_RETURN_TPCBOX_P(result);
}

PGDLLEXPORT Datum Tpcbox_extent_transfn(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tpcbox_extent_transfn);
/**
 * @ingroup mobilitydb_pointcloud_agg
 * @brief Transition function for the extent aggregate over tpcbox values.
 *   Doubles as the parallel combine function for the temporal variants.
 * @sqlfn extent()
 */
Datum
Tpcbox_extent_transfn(PG_FUNCTION_ARGS)
{
  TPCBox *box1 = PG_ARGISNULL(0) ? NULL : PG_GETARG_TPCBOX_P(0);
  TPCBox *box2 = PG_ARGISNULL(1) ? NULL : PG_GETARG_TPCBOX_P(1);
  if (! box1 && ! box2) PG_RETURN_NULL();
  if (! box1) PG_RETURN_TPCBOX_P(tpcbox_copy(box2));
  if (! box2) PG_RETURN_TPCBOX_P(tpcbox_copy(box1));
  if (box1->pcid != box2->pcid)
    ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
      errmsg("Extent aggregation across distinct pcids: %u vs %u",
        box1->pcid, box2->pcid)));
  TPCBox *result = tpcbox_copy(box1);
  tpcbox_expand(box2, result);
  PG_RETURN_TPCBOX_P(result);
}

/*****************************************************************************/
