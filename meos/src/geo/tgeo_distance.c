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
 * @brief Distance functions for temporal geos
 */

// #include "geo/tgeo_distance.h"

/* PostgreSQL */
#include <postgres.h>
/* PostGIS */
// #include <lwgeodetic_tree.h>
#include <measures.h>
#include <measures3d.h>
/* MEOS */
#include <meos.h>
#include <meos_internal.h>
#include "general/lifting.h"
#include "general/tinstant.h"
#include "general/tsequence.h"
#include "point/pgis_types.h"
#include "point/geography_funcs.h"
#include "point/tpoint_distance.h"
#include "point/tpoint_spatialfuncs.h"
#include "geo/tgeo_spatialfuncs.h"

/*****************************************************************************
 * Temporal distance
 *****************************************************************************/

/**
 * @ingroup meos_temporal_dist
 * @brief Return the temporal distance between a temporal geo and a
 * geometry/geography
 * @param[in] temp Temporal geo
 * @param[in] gs Geometry/geography
 * @csqlfn #Distance_tgeo_geo()
 */
Temporal *
distance_tgeo_geo(const Temporal *temp, const GSERIALIZED *gs)
{
  /* Ensure validity of the arguments */
  if (! ensure_not_null((void *) temp) || ! ensure_not_null((void *) gs) ||
      ! ensure_valid_tgeo_geo(temp, gs) || gserialized_is_empty(gs) ||
      ! ensure_same_dimensionality_tspatial_gs(temp, gs))
    return NULL;

  LiftedFunctionInfo lfinfo;
  memset(&lfinfo, 0, sizeof(LiftedFunctionInfo));
  lfinfo.func = (varfunc) distance_fn(temp->flags);
  lfinfo.numparam = 0;
  lfinfo.argtype[0] = temp->temptype;
  lfinfo.argtype[1] = temptype_basetype(temp->temptype);
  lfinfo.restype = T_TFLOAT;
  lfinfo.reslinear = false;
  lfinfo.invert = INVERT_NO;
  lfinfo.discont = CONTINUOUS;
  lfinfo.tpfunc_base = NULL;
  lfinfo.tpfunc = NULL;
  return tfunc_temporal_base(temp, PointerGetDatum(gs), &lfinfo);
}

/**
 * @ingroup meos_temporal_dist
 * @brief Return the temporal distance between two temporal geos
 * @param[in] temp1,temp2 Temporal geos
 * @csqlfn #Distance_tgeo_tgeo()
 */
Temporal *
distance_tgeo_tgeo(const Temporal *temp1, const Temporal *temp2)
{
  /* Ensure validity of the arguments */
  if (! ensure_not_null((void *) temp1) || ! ensure_not_null((void *) temp2) ||
      ! ensure_valid_tgeo_tgeo(temp1, temp2) ||
      ! ensure_same_dimensionality(temp1->flags, temp2->flags))
    return NULL;

  LiftedFunctionInfo lfinfo;
  memset(&lfinfo, 0, sizeof(LiftedFunctionInfo));
  lfinfo.func = (varfunc) pt_distance_fn(temp1->flags);
  lfinfo.numparam = 0;
  lfinfo.argtype[0] = lfinfo.argtype[1] = temp1->temptype;
  lfinfo.restype = T_TFLOAT;
  lfinfo.reslinear = false;
  lfinfo.invert = INVERT_NO;
  lfinfo.discont = CONTINUOUS;
  lfinfo.tpfunc_base = NULL;
  lfinfo.tpfunc = NULL;
  return tfunc_temporal_temporal(temp1, temp2, &lfinfo);
}

/*****************************************************************************
 * Nearest approach instant (NAI)
 *****************************************************************************/

/**
 * @brief Return the new current nearest approach instant between a temporal
 * sequence point with step interpolation and a geometry/geography
 * (iterator function)
 * @param[in] seq Temporal geo
 * @param[in] geo Geometry/geography
 * @param[in] mindist Current minimum distance, it is set at DBL_MAX at the
 * begining but contains the minimum distance found in the previous
 * sequences of a temporal sequence set
 * @param[out] result Instant with the minimum distance
 * @return Minimum distance
 */
static double
nai_tgeoseq_geo_iter(const TSequence *seq, const GSERIALIZED *geo,
  double mindist, const TInstant **result)
{
  for (int i = 0; i < seq->count; i++)
  {
    const TInstant *inst = TSEQUENCE_INST_N(seq, i);
    GSERIALIZED *gs = DatumGetGserializedP(tinstant_val(inst));
    datum_func2 func = distance_fn(seq->flags);
    double dist = func(PointerGetDatum(gs), PointerGetDatum(geo));
    if (dist < mindist)
    {
      mindist = dist;
      *result = inst;
    }
  }
  return mindist;
}

/**
 * @brief Return the nearest approach instant between a temporal sequence
 * point with step interpolation and a geometry/geography
 * @param[in] seq Temporal geo
 * @param[in] geo Geometry/geography
 */
static TInstant *
nai_tgeoseq_geo(const TSequence *seq, const GSERIALIZED *geo)
{
  const TInstant *inst = NULL; /* make compiler quiet */
  nai_tgeoseq_geo_iter(seq, geo, DBL_MAX, &inst);
  return tinstant_copy(inst);
}

/**
 * @brief Return the nearest approach instant between a temporal sequence set
 * point with step interpolation and a geometry/geography
 * @param[in] ss Temporal geo
 * @param[in] geo Geometry/geography
 */
static TInstant *
nai_tgeoseqset_step_geo(const TSequenceSet *ss, const GSERIALIZED *geo)
{
  const TInstant *inst = NULL; /* make compiler quiet */
  double mindist = DBL_MAX;
  for (int i = 0; i < ss->count; i++)
    mindist = nai_tgeoseq_geo_iter(TSEQUENCESET_SEQ_N(ss, i), geo,
      mindist, &inst);
  assert(inst != NULL);
  return tinstant_copy(inst);
}

/*****************************************************************************/

/**
 * @ingroup meos_temporal_dist
 * @brief Return the nearest approach instant between a temporal geo and
 * a geometry
 * @param[in] temp Temporal geometry
 * @param[in] gs Geometry
 * @csqlfn #NAI_tgeo_geo()
 */
TInstant *
nai_tgeo_geo(const Temporal *temp, const GSERIALIZED *gs)
{
  /* Ensure validity of the arguments */
  if (! ensure_not_null((void *) temp) || ! ensure_not_null((void *) gs) ||
      ! ensure_valid_tgeo_geo(temp, gs) || gserialized_is_empty(gs) ||
      ! ensure_same_dimensionality_tspatial_gs(temp, gs))
    return NULL;

  TInstant *result;
  assert(temptype_subtype(temp->subtype));
  switch (temp->subtype)
  {
    case TINSTANT:
      result = tinstant_copy((TInstant *) temp);
      break;
    case TSEQUENCE:
      result = nai_tgeoseq_geo((TSequence *) temp, gs);
      break;
    default: /* TSEQUENCESET */
      result = nai_tgeoseqset_step_geo((TSequenceSet *) temp, gs);
  }
  return result;
}

/**
 * @ingroup meos_temporal_dist
 * @brief Return the nearest approach instant between two temporal geos
 * @param[in] temp1,temp2 Temporal geos
 * @csqlfn #NAI_tgeo_tgeo()
 */
TInstant *
nai_tgeo_tgeo(const Temporal *temp1, const Temporal *temp2)
{
  /* Ensure validity of the arguments */
  if (! ensure_not_null((void *) temp1) || ! ensure_not_null((void *) temp2) ||
      ! ensure_valid_tgeo_tgeo(temp1, temp2) ||
      ! ensure_same_dimensionality(temp1->flags, temp2->flags))
    return NULL;

  /* Compute the temporal distance, it may be NULL if the points do not
   * intersect on time */
  Temporal *dist = distance_tgeo_tgeo(temp1, temp2);
  if (dist == NULL)
    return NULL;

  const TInstant *min = temporal_min_instant(dist);
  pfree(dist);
  /* The closest point may be at an exclusive bound => 3rd argument = false */
  Datum value;
  temporal_value_at_timestamptz(temp1, min->t, false, &value);
  return tinstant_make_free(value, temp1->temptype, min->t);
}

/*****************************************************************************
 * Nearest approach distance (NAD)
 *****************************************************************************/

/**
 * @ingroup meos_temporal_dist
 * @brief Return the nearest approach distance between a temporal geo
 * and a geometry
 * @param[in] temp Temporal geo
 * @param[in] gs Geometry
 * @csqlfn #NAD_tgeo_geo()
 */
double
nad_tgeo_geo(const Temporal *temp, const GSERIALIZED *gs)
{
  /* Ensure validity of the arguments */
  if (! ensure_not_null((void *) temp) || ! ensure_not_null((void *) gs) ||
      ! ensure_valid_tgeo_geo(temp, gs) || gserialized_is_empty(gs) ||
      ! ensure_same_dimensionality_tspatial_gs(temp, gs))
    return -1.0;

  datum_func2 func = distance_fn(temp->flags);
  Datum traj = PointerGetDatum(tgeo_traversed_area(temp));
  double result = DatumGetFloat8(func(traj, PointerGetDatum(gs)));
  pfree(DatumGetPointer(traj));
  return result;
}

/**
 * @ingroup meos_temporal_dist
 * @brief Return the nearest approach distance between a temporal point
 * and a spatiotemporal box
 * @param[in] temp Temporal point
 * @param[in] box Spatiotemporal box
 * @return On error return -1.0
 * @csqlfn #NAD_tgeo_stbox()
 */
double
nad_tgeo_stbox(const Temporal *temp, const STBox *box)
{
  /* Ensure validity of the arguments */
  if (! ensure_valid_tpoint_box(temp, box) ||
      ! ensure_same_spatial_dimensionality_temp_box(temp->flags, box->flags))
    return -1.0;

  /* Project the temporal point to the timespan of the box */
  bool hast = MEOS_FLAGS_GET_T(box->flags);
  Span p, inter;
  if (hast)
  {
    temporal_set_tstzspan(temp, &p);
    if (! inter_span_span(&p, &box->period, &inter))
      return DBL_MAX;
  }

  /* Select the distance function to be applied */
  datum_func2 func = distance_fn(box->flags);
  /* Convert the stbox to a geometry */
  Datum geo = PointerGetDatum(stbox_to_geo(box));
  Temporal *temp1 = hast ?
    temporal_restrict_tstzspan(temp, &inter, REST_AT) :
    (Temporal *) temp;
  /* Compute the result */
  Datum traj = PointerGetDatum(tpoint_trajectory(temp1));
  double result = DatumGetFloat8(func(traj, geo));

  pfree(DatumGetPointer(traj));
  pfree(DatumGetPointer(geo));
  if (hast)
    pfree(temp1);
  return result;
}


/**
 * @ingroup meos_temporal_dist
 * @brief Return the nearest approach distance between two temporal geos
 * @param[in] temp1,temp2 Temporal geos
 * @csqlfn #NAD_tgeo_tgeo()
 */
double
nad_tgeo_tgeo(const Temporal *temp1, const Temporal *temp2)
{
  /* Ensure validity of the arguments */
  if (! ensure_not_null((void *) temp1) || ! ensure_not_null((void *) temp2) ||
      ! ensure_valid_tgeo_tgeo(temp1, temp2) ||
      ! ensure_same_dimensionality(temp1->flags, temp2->flags))
    return -1.0;

  Temporal *dist = distance_tgeo_tgeo(temp1, temp2);
  if (dist == NULL)
    return -1.0;

  double result = DatumGetFloat8(temporal_min_value(dist));
  pfree(dist);
  return result;
}

/*****************************************************************************
 * ShortestLine
 *****************************************************************************/

/**
 * @ingroup meos_temporal_dist
 * @brief Return the line connecting the nearest approach point between a
 * temporal geo and a geometry
 * @param[in] temp Temporal value
 * @param[in] gs Geometry
 * @csqlfn #Shortestline_tgeo_geo()
 */
GSERIALIZED *
shortestline_tgeo_geo(const Temporal *temp, const GSERIALIZED *gs)
{
  /* Ensure validity of the arguments */
  if (! ensure_not_null((void *) temp) || ! ensure_not_null((void *) gs) ||
      ! ensure_valid_tgeo_geo(temp, gs) || gserialized_is_empty(gs) ||
      ! ensure_same_dimensionality_tspatial_gs(temp, gs) ||
      ! ensure_same_geodetic_gs(temp, gs))
    return NULL;
  bool geodetic = MEOS_FLAGS_GET_GEODETIC(temp->flags);
  if (geodetic && ! ensure_has_not_Z_gs(gs))
    return NULL;

  GSERIALIZED *traj = tpoint_trajectory(temp);
  GSERIALIZED *result;
  if (geodetic)
    /* Notice that geography_shortestline_internal is a MobilityDB function */
    result = geography_shortestline_internal(traj, gs, true);
  else
  {
    result = MEOS_FLAGS_GET_Z(temp->flags) ?
      geom_shortestline3d(traj, gs) : geom_shortestline2d(traj, gs);
  }
  pfree(traj);
  return result;
}

/**
 * @ingroup meos_temporal_dist
 * @brief Return the line connecting the nearest approach point between two
 * temporal geos
 * @param[in] temp1,temp2 Temporal values
 * @csqlfn #Shortestline_tgeo_tgeo()
 */
GSERIALIZED *
shortestline_tgeo_tgeo(const Temporal *temp1, const Temporal *temp2)
{
  /* Ensure validity of the arguments */
  if (! ensure_not_null((void *) temp1) || ! ensure_not_null((void *) temp2) ||
      ! ensure_valid_tgeo_tgeo(temp1, temp2) ||
      ! ensure_same_dimensionality(temp1->flags, temp2->flags))
    return NULL;

  Temporal *dist = distance_tgeo_tgeo(temp1, temp2);
  if (dist == NULL)
    return NULL;
  const TInstant *inst = temporal_min_instant(dist);
  /* Timestamp t may be at an exclusive bound */
  Datum value1, value2;
  temporal_value_at_timestamptz(temp1, inst->t, false, &value1);
  temporal_value_at_timestamptz(temp2, inst->t, false, &value2);
  LWGEOM *line = (LWGEOM *) lwline_make(value1, value2);
  GSERIALIZED *result = geo_serialize(line);
  lwgeom_free(line);
  return result;
}

/*****************************************************************************/
