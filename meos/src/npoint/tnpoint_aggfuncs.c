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
 * @brief Aggregate functions for temporal network points
 * @note The only function currently provided is temporal centroid
 */

/* PostgreSQL */
#include <postgres.h>
/* MEOS */
#include <meos.h>
#include <meos_internal.h>
#include "temporal/set.h"
#include "temporal/skiplist.h"
#include "temporal/temporal_aggfuncs.h"
#include "temporal/type_util.h"
#include "geo/tgeo_aggfuncs.h"
#include "geo/tspatial_parser.h"

/*****************************************************************************/

/**
 * @ingroup meos_npoint_agg
 * @brief Transition function for temporal centroid aggregation of temporal
 * network points
 * @param[in] state Current aggregate value
 * @param[in] temp Temporal network point
 * @csqlfn #Tnpoint_tcentroid_transfn()
 */
SkipList *
tnpoint_tcentroid_transfn(SkipList *state, Temporal *temp)
{
  /* Null temporal: return state */
  if (! temp)
    return state;
  bool hasz = MEOS_FLAGS_GET_Z(temp->flags);
  /* Ensure the validity of the arguments */
  if (! ensure_geoaggstate(state, tspatial_srid(temp), hasz))
    return NULL;
  Temporal *temp1 = tnpoint_to_tgeompoint(temp);
  datum_func2 func = MEOS_FLAGS_GET_Z(temp1->flags) ?
    &datum_sum_double4 : &datum_sum_double3;

  int count;
  Temporal **temparr = tpoint_transform_tcentroid(temp1, &count);
  if (! state)
  {
    state = skiplist_make();
    struct GeoAggregateState extra =
    {
      .srid = tspatial_srid(temp1),
      .hasz = MEOS_FLAGS_GET_Z(temp1->flags) != 0
    };
    aggstate_set_extra(state, &extra, sizeof(struct GeoAggregateState));
  }
  skiplist_splice(state, (void **) temparr, count, func, false);

  pfree_array((void **) temparr, count);
  pfree(temp1);
  return state;
}

/*****************************************************************************/
