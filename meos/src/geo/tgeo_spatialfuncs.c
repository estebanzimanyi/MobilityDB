/***********************************************************************
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
 * @brief Spatial functions for temporal geos
 */

#include "geo/tgeo_spatialfuncs.h"

/* PostgreSQL */
#include <utils/float.h>
#if POSTGRESQL_VERSION_NUMBER >= 160000
  #include "varatt.h"
#endif
/* PostGIS */
#include <liblwgeom.h>
#include <liblwgeom_internal.h>
#include <lwgeodetic.h>
#include <lwgeom_geos.h>
/* MEOS */
#include <meos.h>
#include <meos_internal.h>
#include "general/pg_types.h"
#include "general/lifting.h"
#include "general/temporal_compops.h"
#include "general/tnumber_mathfuncs.h"
#include "general/tsequence.h"
#include "general/type_util.h"
#include "point/pgis_types.h"
#include "point/stbox.h"
#include "point/tpoint.h"
#include "point/tpoint_distance.h"
#include "point/tpoint_spatialfuncs.h"

/*****************************************************************************
 * Traversed area functions
 *****************************************************************************/

/**
 * @brief Return the trajectory of a temporal geo sequence(iterator)
 * @param[in] seq Temporal sequence
 * @note All the asserts are taken care in #tgeoseq_traversed_area
 */
static bool
tgeoseq_traversed_area_iter(const TSequence *seq, GSERIALIZED **result)
{
  /* Instantaneous sequence */
  if (seq->count == 1)
  {
    *result = geo_copy(DatumGetGserializedP(
      tinstant_value(TSEQUENCE_INST_N(seq, 0))));
    return true;
  }

  /* General case */
  GSERIALIZED **geoarr = palloc(sizeof(GSERIALIZED *) * seq->count);
  int ngeos = 0;
  for (int i = 0; i < seq->count; i++)
  {
    GSERIALIZED *gs =
      DatumGetGserializedP(tinstant_val(TSEQUENCE_INST_N(seq, i)));
    /* Remove two consecutive geometries if they are equal */
    if (ngeos == 0)
      geoarr[ngeos++] = gs;
    else
    {
      int is_equal = geo_equals(gs, geoarr[ngeos - 1]);
      if (is_equal < 0)
      {
        pfree(geoarr);
        return false;
      }
      else if (! is_equal)
        geoarr[ngeos++] = gs;
    }
  }
  /* Set the output parameter */
  if (ngeos == 1)
    *result = geo_copy(geoarr[0]);
  else
    *result =geo_collect_garray(geoarr, ngeos);
  /* Clean up and return */
  pfree(geoarr);
  return true;
}

/**
 * @ingroup meos_internal_temporal_spatial_accessor
 * @brief Return the trajectory of a temporal geo sequence
 * @param[in] seq Temporal sequence
 * @note Since the sequence has been already validated there is no verification
 * of the input in this function, in particular for geographies it is supposed
 * that the composing points are geodetic
 * @csqlfn #Tpoint_traversed_area()
 */
GSERIALIZED *
tgeoseq_traversed_area(const TSequence *seq)
{
  assert(seq); assert(tgeo_type(seq->temptype));
  interpType interp = MEOS_FLAGS_GET_INTERP(seq->flags);
  assert(interp == DISCRETE || interp == STEP);
  /* Instantaneous sequence */
  if (seq->count == 1)
    return geo_copy(DatumGetGserializedP(
      tinstant_value(TSEQUENCE_INST_N(seq, 0))));

  /* General case */
  GSERIALIZED *result;
  if (! tgeoseq_traversed_area_iter(seq, &result))
    return NULL;
  return result;
}

/**
 * @ingroup meos_internal_temporal_spatial_accessor
 * @brief Return the trajectory of a temporal geo sequence set
 * @param[in] ss Temporal sequence set
 * @csqlfn #Tpoint_traversed_area()
 */
GSERIALIZED *
tgeoseqset_traversed_area(const TSequenceSet *ss)
{
  assert(ss); assert(tgeo_type(ss->temptype));
  assert(ss->count > 1); assert(! MEOS_FLAGS_LINEAR_INTERP(ss->flags));
  GSERIALIZED **geoarr = palloc(sizeof(GSERIALIZED *) * ss->count);
  /* Iterate as in #tpointseq_traversed_area accumulating the results */
  for (int i = 0; i < ss->count; i++)
  {
    const TSequence *seq = TSEQUENCESET_SEQ_N(ss, i);
    if (! tgeoseq_traversed_area_iter(seq, &geoarr[i]))
    {
      for (int j = 0; j < i; j++)
        pfree(geoarr[i]);
      pfree(geoarr);
      return NULL;
    }
  }
  /* Collect the result from the segments */
  GSERIALIZED *result = geo_collect_garray(geoarr, ss->count);
  /* Set the bounding box of the sequence set */
  STBox box;
  memset(&box, 0, sizeof(box));
  tspatialseqset_set_stbox(ss, &box);
  /* Clean up and return */
  for (int i = 0; i < ss->count; i++)
    pfree(geoarr[i]);
  pfree(geoarr);
  return result;
}

/**
 * @ingroup meos_temporal_spatial_accessor
 * @brief Return the traversed area of a temporal geo
 * @param[in] temp Temporal geo
 * @csqlfn #Tgeo_traversed_area()
 */
GSERIALIZED *
tgeo_traversed_area(const Temporal *temp)
{
  /* Ensure validity of the arguments */
  if (! ensure_not_null((void *) temp) || ! ensure_tgeo_type(temp->temptype) ||
      ! ensure_nonlinear_interp(temp->flags))
    return NULL;

  assert(temptype_subtype(temp->subtype));
  switch (temp->subtype)
  {
    case TINSTANT:
      return DatumGetGserializedP(tinstant_value((TInstant *) temp));
    case TSEQUENCE:
      return tgeoseq_traversed_area((TSequence *) temp);
    default: /* TSEQUENCESET */
      return tgeoseqset_traversed_area((TSequenceSet *) temp);
  }
}

/*****************************************************************************/
