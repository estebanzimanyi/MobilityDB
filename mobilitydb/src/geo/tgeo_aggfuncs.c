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
 * @brief Aggregate functions for temporal geos
 * @details The only functions currently provided are extent and temporal
 * centroid for temporal points
 */

#include "geo/tgeo_aggfuncs.h"

/* C */
#include <assert.h>
/* MEOS */
#include <meos.h>
#include "temporal/skiplist.h"
#include "temporal/temporal_aggfuncs.h"
#include "geo/stbox.h"
/* MobilityDB */
#include "pg_temporal/skiplist.h"

/*****************************************************************************
 * Extent
 *****************************************************************************/

PGDLLEXPORT Datum Tspatial_extent_transfn(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tspatial_extent_transfn);
/**
 * @ingroup mobilitydb_geo_agg
 * @brief Transition function for temporal extent aggregation of temporal 
 * spatial values
 * @sqlfn extent()
 */
Datum
Tspatial_extent_transfn(PG_FUNCTION_ARGS)
{
  STBox *box = PG_ARGISNULL(0) ? NULL : PG_GETARG_STBOX_P(0);
  Temporal *temp = PG_ARGISNULL(1) ? NULL : PG_GETARG_TEMPORAL_P(1);
  STBox *result = tspatial_extent_transfn(box, temp);
  if (! result)
    PG_RETURN_NULL();
  PG_RETURN_STBOX_P(result);
}

/*****************************************************************************
 * Centroid
 *****************************************************************************/

PGDLLEXPORT Datum Tpoint_tcentroid_transfn(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tpoint_tcentroid_transfn);
/**
 * @ingroup mobilitydb_geo_agg
 * @brief Transition function for temporal centroid aggregation of temporal
 * points
 * @sqlfn tCentroid()
 */
Datum
Tpoint_tcentroid_transfn(PG_FUNCTION_ARGS)
{
  SkipList *state;
  INPUT_AGG_TRANS_STATE(fcinfo, state);
  Temporal *temp = PG_GETARG_TEMPORAL_P(1);
  store_fcinfo(fcinfo);
  state = tpoint_tcentroid_transfn(state, temp);
  PG_FREE_IF_COPY(temp, 1);
  PG_RETURN_TEMPORAL_P(state);
}

/*****************************************************************************/

PGDLLEXPORT Datum Tpoint_tcentroid_combinefn(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tpoint_tcentroid_combinefn);
/**
 * @ingroup mobilitydb_geo_agg
 * @brief Combine function for temporal centroid aggregation of temporal points
 * @sqlfn tCentroid()
 */
Datum
Tpoint_tcentroid_combinefn(PG_FUNCTION_ARGS)
{
  SkipList *state1 = PG_ARGISNULL(0) ? NULL :
    (SkipList *) PG_GETARG_POINTER(0);
  SkipList *state2 = PG_ARGISNULL(1) ? NULL :
    (SkipList *) PG_GETARG_POINTER(1);

  store_fcinfo(fcinfo);
  if (! ensure_geoaggstate_state(state1, state2))
    return PointerGetDatum(NULL);

  struct GeoAggregateState *extra = NULL;
  if (state1 && state1->extra)
    extra = state1->extra;
  if (state2 && state2->extra)
    extra = state2->extra;
  assert(extra);
  datum_func2 func = extra->hasz ? &datum_sum_double4 : &datum_sum_double3;
  PG_RETURN_SKIPLIST_P(temporal_tagg_combinefn(state1, state2, func, false));
}

/*****************************************************************************/

PGDLLEXPORT Datum Tpoint_tcentroid_finalfn(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tpoint_tcentroid_finalfn);
/**
 * @ingroup mobilitydb_geo_agg
 * @brief Final function for temporal centroid aggregation of temporal points
 * @sqlfn tCentroid()
 */
Datum
Tpoint_tcentroid_finalfn(PG_FUNCTION_ARGS)
{
  SkipList *state = (SkipList *) PG_GETARG_POINTER(0);
  Temporal *result = tpoint_tcentroid_finalfn(state);
  if (! result)
    PG_RETURN_NULL();
  PG_RETURN_TEMPORAL_P(result);
}

/*****************************************************************************/
