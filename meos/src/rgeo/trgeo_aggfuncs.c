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
 * @brief Aggregate functions for temporal rigid geometries
 * @note The only function currently provided is temporal merge
 */

/* PostgreSQL */
#include <postgres.h>
/* MEOS */
#include <meos.h>
#include <meos_rgeo.h>
#include <meos_internal.h>
#include "temporal/set.h"
#include "temporal/skiplist.h"
#include "temporal/temporal_aggfuncs.h"
#include "temporal/type_util.h"
#include "geo/tgeo_aggfuncs.h"
#include "geo/tspatial_parser.h"
#include "rgeo/trgeo.h"
#include "rgeo/trgeo_utils.h"

#if ! MEOS
  extern FunctionCallInfo fetch_fcinfo();
  extern void store_fcinfo(FunctionCallInfo fcinfo);
  extern MemoryContext set_aggregation_context(FunctionCallInfo fcinfo);
  extern void unset_aggregation_context(MemoryContext ctx);
#endif /* ! MEOS */

/*****************************************************************************/

/**
 * @brief Check the validity a geometry and a skiplist for aggregation
 */
bool
ensure_trgeoaggstate(const SkipList *state, const GSERIALIZED *gs)
{
  if (! state)
    return true;
  GSERIALIZED *ref_geom = (GSERIALIZED *) state->extra;
  if (! ensure_same_geom(gs, ref_geom))
    return false;
  return true;
}

/**
 * @brief Check the validity of two skiplists for aggregation
 */
bool
ensure_trgeoaggstate_state(const SkipList *state1, const SkipList *state2)
{
  if(! state2)
    return true;
  GSERIALIZED *ref_geom2 = (GSERIALIZED *) state2->extra;
  return ensure_trgeoaggstate(state1, ref_geom2);
}

/*****************************************************************************/

/**
 * @ingroup meos_rgeo_agg
 * @brief Transition function for temporal merge aggregation of temporal rigid
 * geometries
 * @param[in] state Current aggregate value
 * @param[in] temp Temporal rigid geometry
 * @csqlfn #Trgeo_merge_transfn()
 */
SkipList *
trgeo_merge_transfn(SkipList *state, Temporal *temp)
{
  /* Null temporal: return state */
  if (! temp)
    return state;
  /* Ensure the validity of the arguments */
  if (! ensure_trgeoaggstate(state, trgeo_geom_p(temp)))
    return NULL;
  Temporal *tpose = trgeo_to_tpose(temp);

  if (! state)
  {
    state = temporal_skiplist_make();
#if ! MEOS
    MemoryContext ctx = set_aggregation_context(fetch_fcinfo());
#endif /* ! MEOS */
    const GSERIALIZED *geom = trgeo_geom_p(temp);
    skiplist_set_extra(state, (void *) geom, VARSIZE(geom));
#if ! MEOS
    unset_aggregation_context(ctx);
#endif /* ! MEOS */
  }
  temporal_skiplist_splice(state, (void **) &tpose, 1, NULL, false);
  return state;
}

/**
 * @ingroup meos_rgeo_agg
 * @brief Combine function for merging temporal rigid geometries
 * @param[in] state1, state2 State values
 */
SkipList *
trgeo_merge_combinefn(SkipList *state1, SkipList *state2)
{
  if (! state1)
    return state2;
  if (! state2)
    return state1;

  if (state1->length == 0)
    return state2;
  if (state2->length == 0)
    return state1;

  int count2 = state2->length;
  void **values2 = skiplist_values(state2);
  temporal_skiplist_splice(state1, values2, count2, NULL, false);
  pfree(values2);
  return state1;
}

/**
 * @ingroup meos_rgeo_agg
 * @brief Final function for aggregating temporal rigid geometries
 * @param[in] state Current aggregate state
 * @csqlfn #Trgeo_merge_finalfn()
 */
Temporal *
trgeo_merge_finalfn(SkipList *state)
{
  if (! state || state->length == 0)
    return NULL;
  /* A copy of the values is needed for switching from aggregate context,
   * for this reason the function #skiplist_values cannot be used */
  Temporal **values = (Temporal **) skiplist_temporal_values(state);
  Temporal *tpose = NULL;
  assert(values[0]->subtype == TINSTANT || values[0]->subtype == TSEQUENCE);
  if (values[0]->subtype == TINSTANT)
    tpose = (Temporal *) tsequence_make_free((TInstant **) values,
      state->length, true, true, DISCRETE, NORMALIZE_NO);
  else /* values[0]->subtype == TSEQUENCE */
    tpose = (Temporal *) tsequenceset_make_free((TSequence **) values,
      state->length, NORMALIZE);
  GSERIALIZED *geo = (GSERIALIZED *) state->extra;
  Temporal *result = geo_tpose_to_trgeo(geo, tpose);
  skiplist_free(state); pfree(tpose);
  return result;
}

/*****************************************************************************/
