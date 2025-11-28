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
 * @brief Spatial functions for temporal rigid geometries
 */

/* C */
#include <assert.h>
/* MEOS */
#include <meos.h>
#include <meos_geo.h>
#include <meos_rgeo.h>
#include <meos_internal.h>
#include "temporal/lifting.h"
#include "temporal/tsequence.h"
#include "temporal/type_util.h"
#include "geo/postgis_funcs.h"
#include "geo/tgeo_spatialfuncs.h"
#include "geo/tgeo_spatialrels.h"
#include "rgeo/lwgeom_utils.h"
#include "rgeo/trgeo.h"
#include "rgeo/trgeo_spatialfuncs.h"
#include "rgeo/trgeo_transform.h"
#include "rgeo/trgeo_utils.h"

/*****************************************************************************
 * SRID Functions
 *****************************************************************************/

/**
 * @ingroup meos_geo_srid
 * @brief Return a temporal rigid geometry with the coordinates set to an SRID
 * @param[in] temp Temporal rigid geometry
 * @param[in] srid SRID
 * @return On error return @p NULL
 * @csqlfn #Trgeo_set_srid()
 */
Temporal *
trgeo_set_srid(const Temporal *temp, int32_t srid)
{
  /* Ensure the validity of the arguments */
  VALIDATE_TRGEOMETRY(temp, NULL);
  if (! ensure_srid_known(srid))
    return NULL;

  GSERIALIZED *geo = geo_copy(trgeo_geom_p(temp));
  if (! spatial_set_srid(PointerGetDatum(geo), T_GEOMETRY, srid))
    return NULL;
  Temporal *tpose = trgeo_to_tpose(temp);
  Temporal *res = tspatial_set_srid(tpose, srid);
  Temporal *result = geo_tpose_to_trgeo(geo, res);
  pfree(geo); pfree(tpose); pfree(res);
  return result;
}

/**
 * @ingroup meos_rgeo_srid
 * @brief Return a spatiotemporal value transformed to another SRID
 * @param[in] temp Spatiotemporal value
 * @param[in] srid Target SRID
 */
Temporal *
trgeo_transform(const Temporal *temp, int32_t srid)
{
  /* Ensure the validity of the arguments */
  VALIDATE_TRGEOMETRY(temp, NULL);
  int32_t srid_from = tspatial_srid(temp);
  if (! ensure_srid_known(srid_from) || ! ensure_srid_known(srid))
    return NULL;

  /* Input and output SRIDs are equal, noop */
  if (srid_from == srid)
    return temporal_copy(temp);

  /* Transform the temporal rigid geometry */
  GSERIALIZED *geo = geo_transform(trgeo_geom_p(temp), srid);
  Temporal *tpose = trgeo_to_tpose(temp);
  Temporal *res = tspatial_transform(tpose, srid);
  Temporal *result = geo_tpose_to_trgeo(geo, res);
  pfree(geo); pfree(tpose); pfree(res);
  return result;
}

/**
 * @ingroup meos_rgeo_srid
 * @brief Return a spatiotemporal value transformed to another SRID using a
 * pipeline
 * @param[in] temp Spatiotemporal value
 * @param[in] pipeline Pipeline string
 * @param[in] srid Target SRID, may be @p SRID_UNKNOWN
 * @param[in] is_forward True when the transformation is forward
 */
Temporal *
trgeo_transform_pipeline(const Temporal *temp, const char *pipeline,
  int32_t srid, bool is_forward)
{
  /* Ensure the validity of the arguments */
  VALIDATE_TRGEOMETRY(temp, NULL); VALIDATE_NOT_NULL(pipeline, NULL);
  if (! ensure_srid_known(srid))
    return NULL;

  /* Transform the temporal rigid geometry */
  GSERIALIZED *geo = geo_transform_pipeline(trgeo_geom_p(temp), pipeline,
    srid, is_forward);
  Temporal *tpose = trgeo_to_tpose(temp);
  Temporal *res = tspatial_transform_pipeline(tpose, pipeline, srid, is_forward);
  Temporal *result = geo_tpose_to_trgeo(geo, res);
  pfree(geo); pfree(tpose); pfree(res);
  return result;
}

/*****************************************************************************
 * Trajectory Functions
 *****************************************************************************/

/**
 * @ingroup meos_rgeo_accessor
 * @brief Return the trajectory of the center of rotation a temporal rigid
 * geometry
 * @csqlfn #Trgeo_trajectory_center()
 */
static TInstant *
trgeoinst_trajectory_center(const TInstant *inst, const GSERIALIZED *gs)
{
  assert(inst->temptype == T_TRGEOMETRY);
  uint32_t geo_type = gserialized_get_type(gs);
  assert(geo_type == POLYGONTYPE || geo_type == POLYHEDRALSURFACETYPE);
  LWGEOM *geom = lwgeom_from_gserialized(gs);
  LWGEOM *lwcentroid = (geo_type == POLYGONTYPE) ? lwgeom_centroid(geom) :
    (LWGEOM *) lwpsurface_centroid((LWPSURFACE *) geom);
  GSERIALIZED *centroid = geo_serialize(lwcentroid);
  TInstant *result = tinstant_make_free(PointerGetDatum(centroid),
    T_TGEOMPOINT, inst->t);
  lwgeom_free(geom); lwgeom_free(lwcentroid);
  return result;
}

/**
 * @ingroup meos_rgeo_accessor
 * @brief Return the trajectory of the center of rotation a temporal rigid
 * geometry
 * @csqlfn #Trgeo_trajectory_center()
 */
static TSequence *
trgeoseq_trajectory_center(const TSequence *seq, const GSERIALIZED *gs)
{
  assert(seq->temptype == T_TRGEOMETRY);
  TInstant **instants = palloc(sizeof(TInstant *) * seq->count);
  for (int i = 0; i < seq->count; i++)
    instants[i] = trgeoinst_trajectory_center(TSEQUENCE_INST_N(seq, i), gs);
  return tsequence_make_free(instants, seq->count, seq->period.lower_inc,
      seq->period.upper_inc, MEOS_FLAGS_GET_INTERP(seq->flags), NORMALIZE);
}

/**
 * @ingroup meos_rgeo_accessor
 * @brief Return the trajectory of the center of rotation a temporal rigid
 * geometry
 * @csqlfn #Trgeo_trajectory_center()
 */
static TSequenceSet *
trgeoseqset_trajectory_center(const TSequenceSet *ss, const GSERIALIZED *gs)
{
  assert(ss->temptype == T_TRGEOMETRY);
  TSequence **sequences = palloc(sizeof(TSequence *) * ss->count);
  for (int i = 0; i < ss->count; i++)
    sequences[i] = trgeoseq_trajectory_center(TSEQUENCESET_SEQ_N(ss, i), gs);
  return tsequenceset_make_free(sequences, ss->count, NORMALIZE);
}

/**
 * @ingroup meos_rgeo_accessor
 * @brief Return the trajectory of the center of rotation a temporal rigid
 * geometry
 * @csqlfn #Trgeo_trajectory_center()
 */
Temporal *
trgeo_trajectory_center(const Temporal *temp)
{
  assert(temp->temptype == T_TRGEOMETRY);
  const GSERIALIZED *gs = trgeo_geom_p(temp);
  Temporal *result;
  assert(temptype_subtype(temp->subtype));
  if (temp->subtype == TINSTANT)
    result = (Temporal *) trgeoinst_trajectory_center((TInstant *) temp, gs);
  else if (temp->subtype == TSEQUENCE)
    result = (Temporal *) trgeoseq_trajectory_center((TSequence *) temp, gs);
  else /* temp->subtype == TSEQUENCESET */
    result = (Temporal *) trgeoseqset_trajectory_center((TSequenceSet *) temp,
      gs);
  return result;
}

/*****************************************************************************
 * Traversed area
 *****************************************************************************/

/**
 * @brief Return the traversed area of a temporal rigid geometry 
 * @param[in] inst Temporal rigid geometry
 * @param[in] refgeom Reference geometry,
 */
static LWGEOM *
trgeoinst_traversed_area_iter(const TInstant *inst, const LWGEOM *refgeom)
{
  assert(inst->temptype == T_TRGEOMETRY); assert(refgeom);
  const Pose *pose = DatumGetPoseP(tinstant_value_p(inst));
  LWGEOM *result = lwgeom_clone_deep(refgeom);
  lwgeom_apply_pose(pose, result);
  if (result->bbox)
    lwgeom_refresh_bbox(result);
  return result;
}

/**
 * @brief Return the traversed area of a temporal rigid geometry 
 * @param[in] seq Temporal rigid geometry
 * @param[in] refgeom Reference geometry,
 * @param[in] unary_union True when the result is a single geometry, not a
 * collection. It is only used when the interpolation is not linear.
 * @param[out] result Array of component geometries,
 * @return Number of geometries in the output array
 */
static int
trgeoseq_discstep_traversed_area_iter(const TSequence *seq,
  const LWGEOM *refgeom, bool unary_union, LWGEOM **result)
{
  /* Ensure the validity of the arguments */
  assert(seq->temptype == T_TRGEOMETRY); assert(refgeom); assert(result);
  LWGEOM *geom;
  const Pose *pose;

  /* Accumulate the geometries in an array */
  if (! unary_union)
  {
    for (int i = 0; i < seq->count; ++i)
    {
      pose = DatumGetPoseP(tinstant_value_p(TSEQUENCE_INST_N(seq, i)));
      geom = lwgeom_clone_deep(refgeom);
      lwgeom_apply_pose(pose, geom);
      if (geom->bbox)
        lwgeom_refresh_bbox(geom);
      result[i] = geom;
    }
    return seq->count;
  }

  /* Perform the union of the geometries */
  pose = DatumGetPoseP(tinstant_value_p(TSEQUENCE_INST_N(seq, 0)));
  geom = lwgeom_clone_deep(refgeom);
  lwgeom_apply_pose(pose, geom);
  if (geom->bbox)
    lwgeom_refresh_bbox(geom);
  for (int i = 1; i < seq->count; ++i)
  {
    LWGEOM *geom1 = geom;
    pose = DatumGetPoseP(tinstant_value_p(TSEQUENCE_INST_N(seq, i)));
    LWGEOM *geom2 = lwgeom_clone_deep(refgeom);
    lwgeom_apply_pose(pose, geom2);
    geom = lwgeom_union(geom1, geom2);
    lwgeom_free(geom1); lwgeom_free(geom2);
  }
  result[0] = geom;
  lwgeom_simplify_in_place(geom, MEOS_EPSILON, LW_TRUE);
  return 1;
}

/**
 * @brief Return the traversed area of a temporal rigid geometry 
 * @param[in] seq Temporal rigid geometry
 * @param[in] refgeom Reference geometry,
 * @param[out] result Array of component geometries,
 * @return Number of geometries in the output array
 */
static int
trgeoseq_linear_traversed_area_iter(const TSequence *seq,
  const LWGEOM *refgeom, LWGEOM **result)
{
  /* Ensure the validity of the arguments */
  assert(seq->temptype == T_TRGEOMETRY); assert(refgeom); assert(result);

  /* Instantaneous sequence */
  if (seq->count == 1)
  {
    result[0] = trgeoinst_traversed_area_iter(TSEQUENCE_INST_N(seq, 0),
      refgeom);
    return 1;
  }

  /* General case */
  const GSERIALIZED *gs = trgeo_geom_p((Temporal *) seq);
  LWGEOM *geom1 = lwgeom_from_gserialized(gs);
  LWGEOM *geom2 = NULL;
  LWGEOM *lwgeom_result = lwgeom_construct_empty(POLYGONTYPE,
    tspatial_srid((Temporal *) seq), false, false);
  Pose *value_prev = NULL;
  for (int i = 1; i < seq->count; ++i)
  {
    double theta;
    if (i == 1)
    {
      Pose *pose = DatumGetPoseP(tinstant_value_p(TSEQUENCE_INST_N(seq, i)));
      theta = pose->data[2];
    }
    else
    {
      const TInstant *inst = TSEQUENCE_INST_N(seq, i);
      Pose *value_prev_invert = pose_invert(value_prev);
      Pose *value = DatumGetPoseP(tinstant_value_p(inst));
      Pose *pose = pose_combine(value, value_prev_invert);
      theta = pose->data[2];
      value_prev = value;
    }

    int n = ceil(fabs(theta) / THETA_MAX) - 1;

    for (int j = 1; j < n + 1; ++j)
    {
      const TInstant *inst1 = TSEQUENCE_INST_N(seq, i - 1);
      const TInstant *inst2 = TSEQUENCE_INST_N(seq, i);
      Datum value1 = tinstant_value_p(inst1);
      Datum value2 = tinstant_value_p(inst2);
      if (i == 1)
        inst1 = trgeoinst_pose_zero(inst1->t, FLAGS_GET_Z(refgeom->flags),
          refgeom->srid);

      double duration = (inst2->t - inst1->t);
      double ratio = (double) j / (double) (n + 1);
      TimestampTz tj = inst1->t + (long) (duration * ratio);
      Pose *pose = DatumGetPoseP(tsegment_value_at_timestamptz(value1, value2,
        T_TPOSE, inst1->t, inst2->t, tj));
      geom2 = lwgeom_clone_deep(refgeom);
      lwgeom_apply_pose(pose, geom2);
      pfree(pose);
      if (i == 1)
        pfree((TInstant *) inst1);

      LWGEOM *old_result = lwgeom_result;
      LWGEOM *partial_result = lwgeom_traversed_area(geom1, geom2);
      lwgeom_result = lwgeom_union(old_result, partial_result);
      lwgeom_free(old_result); lwgeom_free(partial_result); lwgeom_free(geom1);
      geom1 = geom2;
    }

    const Pose *pose = DatumGetPoseP(tinstant_value_p(TSEQUENCE_INST_N(seq, i)));
    geom2 = lwgeom_clone_deep(refgeom);
    lwgeom_apply_pose(pose, geom2);

    LWGEOM *old_result = lwgeom_result;
    LWGEOM *partial_result = lwgeom_traversed_area(geom1, geom2);
    lwgeom_result = lwgeom_union(old_result, partial_result);
    lwgeom_free(old_result); lwgeom_free(partial_result); lwgeom_free(geom1);
    geom1 = geom2;
  }
  lwgeom_free(geom1);
  lwgeom_simplify_in_place(lwgeom_result, MEOS_EPSILON, LW_TRUE);
  result[0] = lwgeom_result;
  return 1;
}

/**
 * @brief Return the traversed area of a temporal rigid geometry 
 * @param[in] seq Temporal rigid geometry
 * @param[in] refgeom Reference geometry,
 * @param[in] unary_union True when the result is a single geometry, not a
 * collection. It is only used when the interpolation is not linear.
 * @param[out] result Array of component geometries,
 * @return Number of geometries in the output array
 */
static int
trgeoseq_traversed_area_iter(const TSequence *seq, const LWGEOM *refgeom,
  bool unary_union, LWGEOM **result)
{
  /* Ensure the validity of the arguments */
  assert(seq->temptype == T_TRGEOMETRY); assert(refgeom); assert(result);
  return (MEOS_FLAGS_GET_INTERP(seq->flags) == LINEAR) ?
    trgeoseq_linear_traversed_area_iter(seq, refgeom, result) :
    trgeoseq_discstep_traversed_area_iter(seq, refgeom, unary_union, result);
}

/**
 * @brief Return the traversed area of a temporal rigid geometry 
 * @param[in] seq Temporal rigid geometry
 * @param[in] unary_union True when the result is a single geometry, not a
 * collection. It is only used when the interpolation is not linear.
 */
static GSERIALIZED *
trgeoseq_traversed_area(const TSequence *seq, bool unary_union)
{
  assert(seq->temptype == T_TRGEOMETRY);
  /* Instantaneous case */
  if (seq->count == 1)
    return trgeoinst_geom(TSEQUENCE_INST_N(seq, 0));

  /* General case */
  const GSERIALIZED *gs = trgeo_geom_p((Temporal *) seq);
  LWGEOM *refgeom = lwgeom_from_gserialized(gs);
  LWGEOM **geoms = palloc(sizeof(LWGEOM *) * (unary_union ? 1 : seq->count));
  int ngeoms = trgeoseq_traversed_area_iter(seq, refgeom, unary_union,
    &geoms[0]);
  lwgeom_free(refgeom);

  /* Construct the result */
  GSERIALIZED *result;
  if (ngeoms == 1)
  {
    result = gserialized_from_lwgeom(geoms[0], NULL);
    pfree(geoms);
    return result;
  }
  int32_t colltype = MEOS_FLAGS_GET_Z(seq->flags) ?
    COLLECTIONTYPE : MULTIPOLYGONTYPE;
  int32_t srid = tspatial_srid((Temporal *) seq);
  LWCOLLECTION *coll = lwcollection_construct(colltype, srid, NULL, ngeoms,
    geoms);
  result = gserialized_from_lwgeom((LWGEOM *) coll, NULL);
  pfree(coll);
  return result;
}

/**
 * @brief Return the traversed area of a temporal rigid geometry
 * @param[in] ss Temporal rigid geometry
 * @param[in] unary_union True when the result is a single geometry, not a
 * collection. It is only used when the interpolation is not linear.
 */
static GSERIALIZED *
trgeoseqset_traversed_area(const TSequenceSet *ss, bool unary_union)
{
  const GSERIALIZED *gs = trgeo_geom_p((Temporal *) ss);
  LWGEOM *refgeom = lwgeom_from_gserialized(gs);
  GSERIALIZED *result;

  /* Perform the union of the resulting geometries */
  if (unary_union || MEOS_FLAGS_GET_INTERP(ss->flags == LINEAR))
  {
    LWGEOM *geoms[1];
    trgeoseq_traversed_area_iter(TSEQUENCESET_SEQ_N(ss, 0), refgeom,
      unary_union, geoms);
    LWGEOM *lwgeom_result = geoms[0];
    for (int i = 1; i < ss->count; ++i)
    {
      LWGEOM *geom1 = lwgeom_result;
      trgeoseq_traversed_area_iter(TSEQUENCESET_SEQ_N(ss, i), refgeom,
        unary_union, geoms);
      LWGEOM *geom2 = geoms[0];
      lwgeom_result = lwgeom_union(geom1, geom2);
      lwgeom_free(geom1); lwgeom_free(geom2);
    }
    lwgeom_simplify_in_place(lwgeom_result, MEOS_EPSILON, LW_TRUE);
    result = gserialized_from_lwgeom(lwgeom_result, NULL);
    lwgeom_free(lwgeom_result); lwgeom_free(refgeom);
    return result;
  }

  /* Collect the resulting geometries */
  LWGEOM **geoms = palloc(sizeof(LWGEOM *) * ss->totalcount);
  int ngeoms = 0;
  for (int i = 0; i < ss->count; ++i)
    ngeoms += trgeoseq_traversed_area_iter(TSEQUENCESET_SEQ_N(ss, i),
      refgeom, unary_union, &geoms[ngeoms]);

  /* Construct the result */
  if (ngeoms == 1)
  {
    result = gserialized_from_lwgeom(geoms[0], NULL);
    pfree(geoms);
    return result;
  }
  int32_t colltype = MEOS_FLAGS_GET_Z(ss->flags) ?
    COLLECTIONTYPE : MULTIPOLYGONTYPE;
  int32_t srid = tspatial_srid((Temporal *) ss);
  LWCOLLECTION *coll = lwcollection_construct(colltype, srid, NULL, ngeoms,
    geoms);
  result = gserialized_from_lwgeom((LWGEOM *) coll, NULL);
  pfree(coll);
  return result;
}

/**
 * @ingroup meos_rgeo_spatial_accessor
 * @brief Return the traversed area of a temporal rigid geometry
 * @param[in] temp Temporal rigid geometry
 * @param[in] unary_union True when the result is a single geometry, not a
 * collection. It is only used when the interpolation is not linear.
 * @csqlfn #Trgeo_traversed_area()
 */
GSERIALIZED *
trgeo_traversed_area(const Temporal *temp, bool unary_union)
{
  /* Ensure the validity of the arguments */
  VALIDATE_TRGEOMETRY(temp, NULL);

  assert(temptype_subtype(temp->subtype));
  switch (temp->subtype)
  {
    case TINSTANT:
      return trgeoinst_geom((TInstant *) temp);
      break;
    case TSEQUENCE:
      return trgeoseq_traversed_area((TSequence *) temp, unary_union);
      break;
    default: /* TSEQUENCESET */
      return trgeoseqset_traversed_area((TSequenceSet *) temp, unary_union);
  }
}

/*****************************************************************************
 * Restriction functions
 *****************************************************************************/

/**
 * @ingroup meos_internal_rgeo_restrict
 * @brief Return a temporal rigid geometry restricted to (the complement of) a
 * geometry
 */
Temporal *
trgeo_restrict_geom(const Temporal *temp, const GSERIALIZED *gs, bool atfunc)
{
  /* Ensure the validity of the arguments */
  VALIDATE_TRGEOMETRY(temp, NULL); VALIDATE_NOT_NULL(gs, NULL);
  if (! ensure_same_srid(tspatial_srid(temp), gserialized_get_srid(gs)) ||
      ! ensure_has_not_Z_geo(gs))
    return NULL;

  /* Empty geometry */
  if (gserialized_is_empty(gs))
    return atfunc ? NULL : temporal_copy(temp);
  
  meos_error(NOTICE,  MEOS_ERR_INTERNAL_ERROR, "Function not yet implemented");
  return NULL;

  // Temporal *tpose = trgeo_to_tpose(temp);
  // Temporal *res = tgeo_restrict_geom(tpose, gs, NULL, atfunc);
  // Temporal *result = NULL;
  // if (res)
  // {
    // /* We do not call the function tgeompoint_to_trgeo to avoid
     // * roundoff errors */
    // SpanSet *ss = temporal_time(res);
    // result = temporal_restrict_tstzspanset(temp, ss, REST_AT);
    // pfree(res);
    // pfree(ss);
  // }
  // pfree(tpose);
  // return result;
}

#if MEOS
/**
 * @ingroup meos_rgeo_restrict
 * @brief Return a temporal rigid geometry restricted to a geometry
 * @param[in] temp Temporal point
 * @param[in] gs Geometry
 * @csqlfn #Tnpoint_at_geom()
 */
inline Temporal *
trgeo_at_geom(const Temporal *temp, const GSERIALIZED *gs)
{
  return trgeo_restrict_geom(temp, gs, REST_AT);
}

/**
 * @ingroup meos_rgeo_restrict
 * @brief Return a temporal rigid geometry restricted to (the complement of) a geometry
 * @param[in] temp Temporal point
 * @param[in] gs Geometry
 * @csqlfn #Tnpoint_minus_geom()
 */
inline Temporal *
trgeo_minus_geom(const Temporal *temp, const GSERIALIZED *gs)
{
  return trgeo_restrict_geom(temp, gs, REST_MINUS);
}
#endif /* MEOS */

/*****************************************************************************/

/**
 * @ingroup meos_internal_rgeo_restrict
 * @brief Return a temporal rigid geometry restricted to (the complement of) a
 * spatiotemporal box
 * @param[in] temp Temporal rigid geometry
 * @param[in] box Spatiotemporal box
 * @param[in] border_inc True when the box contains the upper border
 * @param[in] atfunc True if the restriction is `at`, false for `minus`
 */
Temporal *
trgeo_restrict_stbox(const Temporal *temp, const STBox *box, bool border_inc UNUSED,
  bool atfunc UNUSED)
{
  /* Ensure the validity of the arguments */
  VALIDATE_TRGEOMETRY(temp, NULL); VALIDATE_NOT_NULL(box, NULL);

  meos_error(NOTICE, MEOS_ERR_INTERNAL_ERROR, "Function not yet implemented");
  return NULL;

  // Temporal *tpose = trgeo_to_tpose(temp);
  // Temporal *res = tpose_restrict_stbox(tpose, box, border_inc, atfunc);
  // pfree(tpose);
  // Temporal *result = NULL;
  // if (res)
  // {
    // result = geo_tpose_to_trgeo(trgeo_geom_p(temp), res);
    // pfree(res);
  // }
  // return result;
}

#if MEOS
/**
 * @ingroup meos_rgeo_restrict
 * @brief Return a temporal rigid geometry restricted to a geometry
 * @param[in] temp Temporal rigid geometry
 * @param[in] box Spatiotemporal box
 * @param[in] border_inc True when the box contains the upper border
 * @sqlfn #Tnpoint_at_stbox()
 */
inline Temporal *
trgeo_at_stbox(const Temporal *temp, const STBox *box, bool border_inc)
{
  return trgeo_restrict_stbox(temp, box, border_inc, REST_AT);
}

/**
 * @ingroup meos_rgeo_restrict
 * @brief Return a temporal rigid geometry restricted to the complement of a
 * geometry
 * @param[in] temp Temporal rigid geometry
 * @param[in] box Spatiotemporal box
 * @param[in] border_inc True when the box contains the upper border
 * @sqlfn #Tnpoint_minus_stbox()
 */
inline Temporal *
trgeo_minus_stbox(const Temporal *temp, const STBox *box, bool border_inc)
{
  return trgeo_restrict_stbox(temp, box, border_inc, REST_MINUS);
}
#endif /* MEOS */

/*****************************************************************************/
