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
 * @brief General functions for temporal rigid geometries
 */

#include "rgeo/trgeo.h"

/* C */
#include <assert.h>
/* PostGIS */
#include "liblwgeom.h"
/* MEOS */
#include <meos.h>
#include <meos_internal.h>
#include "general/lifting.h"
#include "general/meos_catalog.h"
#include "general/temporal.h"
#include "general/type_util.h"
#include "geo/tgeo_spatialfuncs.h"
#include "pose/pose.h"
#include "rgeo/trgeo_temporaltypes.h"
#include "rgeo/trgeo_out.h"
#include "rgeo/trgeo_utils.h"

/*****************************************************************************
 * Parameter tests
 *****************************************************************************/

/**
 * @brief Ensure that a trgeometry has a geometry
 */
bool
ensure_has_geom(int16 flags)
{
  if (MEOS_FLAGS_GET_GEOM(flags))
    return true;
  meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
    "Cannot access geometry from temporal rigid geometry");
  return false;
}


/*****************************************************************************/

/**
 * @brief Returns the reference geometry of the temporal value
 */
Datum
trgeo_geom_p(const Temporal *temp)
{
  Datum result;
  assert(temptype_subtype(temp->subtype));
  if (temp->subtype == TINSTANT)
    result = trgeoinst_geom_p((const TInstant *) temp);
  else if (temp->subtype == TSEQUENCE)
    result = trgeoseq_geom_p((const TSequence *) temp);
  else /* temp->subtype == TSEQUENCESET */
    result = trgeoseqset_geom_p((const TSequenceSet *) temp);
  return result;
}

inline Datum
trgeo_geom(const Temporal *temp)
{
  return datum_copy(trgeo_geom_p(temp), T_GEOMETRY);
}

/*****************************************************************************
 * Input/output functions
 *****************************************************************************/



/*****************************************************************************
 * Conversion functions
 *****************************************************************************/

/**
 * @brief Returns a new temporal pose obtained by removing the reference
 * geometry of a temporal rigid geometry instant
 */
Temporal *
trgeo_tpose(const Temporal *temp)
{
  /* Ensure validity of the arguments */
  if (! ensure_has_geom(temp->flags))
    return NULL;

  assert(temptype_subtype(temp->subtype));
  if (temp->subtype == TINSTANT)
    return (Temporal *) trgeoinst_tposeinst((TInstant *) temp);
  else if (temp->subtype == TSEQUENCE)
    return (Temporal *) trgeoseq_tposeseq((TSequence *) temp);
  else /* temp->subtype == TSEQUENCESET */
    return (Temporal *) trgeoseqset_tposeseqset((TSequenceSet *) temp);
}

/**
 * @brief Convert a temporal rigid geometry into a temporal point
 */
Temporal *
trgeo_tpoint(const Temporal *temp)
{
  LiftedFunctionInfo lfinfo;
  memset(&lfinfo, 0, sizeof(LiftedFunctionInfo));
  lfinfo.func = (varfunc) &datum_pose_point;
  lfinfo.numparam = 0;
  lfinfo.argtype[0] = temptype_basetype(temp->temptype);
  lfinfo.restype = T_TGEOMPOINT;
  lfinfo.tpfunc_base = NULL;
  lfinfo.tpfunc = NULL;
  Temporal *result = tfunc_temporal(temp, &lfinfo);
  return result;
}

/*****************************************************************************
 * Constructor functions
 *****************************************************************************/

TInstant *
geo_tposeinst_to_trgeo(const GSERIALIZED *gs, const TInstant *inst)
{
  /* Ensure validity of the arguments */
#if MEOS
  if (! ensure_not_null((void **) gs) || ! ensure_not_null((void **) inst))
    return NULL;
#else
  assert(gs); assert(inst);
#endif /* MEOS */
  if (! ensure_not_empty(gs) || ! ensure_has_not_M_geo(gs))
    return NULL;

  return trgeoinst_make(PointerGetDatum(gs), tinstant_value_p(inst),
    T_TRGEOMETRY, inst->t);
}

TSequence *
geo_tposeseq_to_trgeo(const GSERIALIZED *gs, const TSequence *seq)
{
  /* Ensure validity of the arguments */
#if MEOS
  if (! ensure_not_null((void **) gs) || ! ensure_not_null((void **) seq))
    return NULL;
#else
  assert(gs); assert(seq);
#endif /* MEOS */
  if (! ensure_not_empty(gs) || ! ensure_has_not_M_geo(gs))
    return NULL;

  TInstant **instants = palloc(sizeof(TInstant *) * seq->count);
  for (int i = 0; i < seq->count; i++)
    instants[i] = geo_tposeinst_to_trgeo(gs, TSEQUENCE_INST_N(seq, i));
  return trgeoseq_make_free(PointerGetDatum(gs), instants, seq->count,
    seq->period.lower_inc, seq->period.upper_inc,
    MEOS_FLAGS_GET_INTERP(seq->flags), NORMALIZE_NO);
}

TSequenceSet *
geo_tposeseqset_to_trgeo(const GSERIALIZED *gs, const TSequenceSet *ss)
{
  /* Ensure validity of the arguments */
#if MEOS
  if (! ensure_not_null((void **) gs) || ! ensure_not_null((void **) ss))
    return NULL;
#else
  assert(gs); assert(ss);
#endif /* MEOS */
  if (! ensure_not_empty(gs) || ! ensure_has_not_M_geo(gs))
    return NULL;

  TSequence **sequences = palloc(sizeof(TSequence *) * ss->count);
  for (int i = 0; i < ss->count; i++)
    sequences[i] = geo_tposeseq_to_trgeo(gs, TSEQUENCESET_SEQ_N(ss, i));
  return trgeoseqset_make_free(PointerGetDatum(gs), sequences, ss->count,
    NORMALIZE_NO);
}

/*****************************************************************************/

Temporal *
geo_tpose_to_trgeo(const GSERIALIZED *gs, const Temporal *temp)
{
  /* Ensure validity of the arguments */
#if MEOS
  if (! ensure_not_null((void **) gs) || ! ensure_not_null((void **) temp))
    return NULL;
#else
  assert(gs); assert(temp);
#endif /* MEOS */
  if (! ensure_not_empty(gs) || ! ensure_has_not_M_geo(gs))
    return NULL;

  assert(temptype_subtype(temp->subtype));
  if (temp->subtype == TINSTANT)
    return (Temporal *) geo_tposeinst_to_trgeo(gs, (TInstant *) temp);
  else if (temp->subtype == TSEQUENCE)
    return (Temporal *) geo_tposeseq_to_trgeo(gs, (TSequence *) temp);
  else /* temp->subtype == TSEQUENCESET */
    return (Temporal *) geo_tposeseqset_to_trgeo(gs, (TSequenceSet *) temp);
}

/*****************************************************************************
 * Accessor functions
 *****************************************************************************/

/**
 * @brief Return a geometry obtained by appling a pose to a geometry
 * @param[in] geom Geometry
 * @param[in] pose Pose
 */
GSERIALIZED *
geom_apply_pose(const Pose *pose, GSERIALIZED *gs)
{
  LWGEOM *geom = lwgeom_from_gserialized(gs);
  LWGEOM *result_geom = lwgeom_clone_deep(geom);
  lwgeom_apply_pose(pose, result_geom);
  if (result_geom->bbox)
    lwgeom_refresh_bbox(result_geom);
  GSERIALIZED *result = geo_serialize(result_geom);
  lwgeom_free(geom); lwgeom_free(result_geom);
  return result;
}

/**
 * @brief Return a copy of the start value of a temporal rigid geometry
 * @param[in] temp Temporal value
 * @csqlfn #Trgeometry_start_value()
 */
Datum
trgeo_start_value(const Temporal *temp)
{
  assert(temp); assert(temp->temptype == T_TRGEOMETRY);
  Datum pose;
  assert(temptype_subtype(temp->subtype));
  switch (temp->subtype)
  {
    case TINSTANT:
      pose = tinstant_value((TInstant *) temp);
      break;
    case TSEQUENCE:
      pose = tinstant_value(TSEQUENCE_INST_N((TSequence *) temp, 0));
      break;
    default: /* TSEQUENCESET */
      pose = tinstant_value(
        TSEQUENCE_INST_N(TSEQUENCESET_SEQ_N((TSequenceSet *) temp, 0), 0));
  }
  GSERIALIZED *gs = DatumGetGserializedP(trgeo_geom(temp));
  GSERIALIZED *res = geom_apply_pose(DatumGetPoseP(pose), gs);
  return GserializedPGetDatum(res);
}

/**
 * @ingroup meos_internal_temporal_accessor
 * @brief Return a copy of the end base value of a temporal value
 * @param[in] temp Temporal value
 */
Datum
trgeo_end_value(const Temporal *temp)
{
  assert(temp); assert(temp->temptype == T_TRGEOMETRY);
  Datum pose;
  assert(temptype_subtype(temp->subtype));
  switch (temp->subtype)
  {
    case TINSTANT:
      pose = tinstant_value((TInstant *) temp);
      break;
    case TSEQUENCE:
      pose = tinstant_value(TSEQUENCE_INST_N((TSequence *) temp,
        ((TSequence *) temp)->count - 1));
      break;
    default: /* TSEQUENCESET */
    {
      const TSequence *seq = TSEQUENCESET_SEQ_N((TSequenceSet *) temp,
        ((TSequenceSet *) temp)->count - 1);
      pose = tinstant_value(TSEQUENCE_INST_N(seq, seq->count - 1));
    }
  }
  GSERIALIZED *gs = geo_copy(DatumGetGserializedP(trgeo_geom_p(temp)));
  return GserializedPGetDatum(geom_apply_pose(DatumGetPoseP(pose), gs));
}

/**
 * @ingroup meos_temporal_accessor
 * @brief Return in the last argument a copy of the n-th value of a temporal
 * value 
 * @param[in] temp Temporal value
 * @param[in] n Number (1-based)
 * @param[out] result Resulting timestamp
 * @return On error return false
 * @csqlfn #Trgeometry_value_n()
 */
bool
trgeo_value_n(const Temporal *temp, int n, Datum *result)
{
  /* Ensure validity of the arguments */
#if MEOS
  if (! ensure_not_null((void *) temp) || ! ensure_not_null((void *) result))
    return false;
#else
  assert(temp); assert(result);
#endif /* MEOS */
  if (! ensure_positive(n))
    return false;

  Datum pose;
  assert(temptype_subtype(temp->subtype));
  switch (temp->subtype)
  {
    case TINSTANT:
    {
      if (n != 1)
        return false;
      pose = tinstant_value((TInstant *) temp);
      break;
    }
    case TSEQUENCE:
    {
      if (n < 1 || n > ((TSequence *) temp)->count)
        return false;
      pose = tinstant_value(TSEQUENCE_INST_N((TSequence *) temp, n - 1));
      break;
    }
    default: /* TSEQUENCESET */
      if (! tsequenceset_value_n((TSequenceSet *) temp, n, &pose))
        return false;
  } 
  GSERIALIZED *gs = DatumGetGserializedP(trgeo_geom(temp));
  GSERIALIZED *res = geom_apply_pose(DatumGetPoseP(pose), gs);
  *result = GserializedPGetDatum(res);
  return true;
}

/**
 * @ingroup libmeos_internal_rgeo_restrict
 * @brief Return the value of a temporal rigid geometry at a timestamptz
 * @sqlfn valueAtTimestamp
 */
bool
trgeo_value_at_timestamptz(const Temporal *temp, TimestampTz t, bool strict,
  Datum *result)
{
  assert(temp->temptype == T_TRGEOMETRY);
  Datum pose_datum;
  bool found = temporal_value_at_timestamptz(temp, t, strict, &pose_datum);
  if (found)
  {
    /* Apply pose to reference geometry */
    const Pose *pose = DatumGetPoseP(pose_datum);
    GSERIALIZED *gs = geo_copy(DatumGetGserializedP(trgeo_geom_p(temp)));
    GSERIALIZED *result_gs = geom_apply_pose(pose, gs);
    *result = PointerGetDatum(result_gs);
  }
  return found;
}

/*****************************************************************************
 * Restriction functions
 *****************************************************************************/

/**
 * @ingroup meos_rgeo_transf
 * @brief Return a temporal value transformed to a temporal instant
 * @param[in] temp Temporal value
 * @csqlfn #Trgeometry_to_tinstant()
 */
TInstant *
trgeo_to_tinstant(const Temporal *temp)
{
  /* Ensure validity of the arguments */
#if MEOS
  if (! ensure_not_null((void *) temp))
    return NULL;
#else
  assert(temp);
#endif /* MEOS */

  assert(temptype_subtype(temp->subtype));
  switch (temp->subtype)
  {
    case TINSTANT:
      return tinstant_copy((TInstant *) temp);
    case TSEQUENCE:
      return trgeoseq_to_tinstant((TSequence *) temp);
    default: /* TSEQUENCESET */
      return trgeoseqset_to_tinstant((TSequenceSet *) temp);
  }
}

/**
 * @ingroup meos_internal_temporal_transf
 * @brief Return a temporal value transformed to a temporal sequence
 * @param[in] temp Temporal value
 * @param[in] interp Interpolation
 * @csqlfn #Trgeometry_to_tsequence()
 */
TSequence *
trgeo_tsequence(const Temporal *temp, interpType interp)
{
  /* Ensure validity of the arguments */
#if MEOS
  if (! ensure_not_null((void *) temp))
    return NULL;
#else
  assert(temp);
#endif /* MEOS */
  if (! ensure_valid_interp(temp->temptype, interp))
    return NULL;

  assert(temptype_subtype(temp->subtype));
  switch (temp->subtype)
  {
    case TINSTANT:
      return trgeoinst_to_tsequence((TInstant *) temp, interp);
    case TSEQUENCE:
    {
      interpType interp1 = MEOS_FLAGS_GET_INTERP(temp->flags);
      if (interp1 == DISCRETE && interp != DISCRETE &&
        ((TSequence *) temp)->count > 1)
      {
        /* The first character should be transformed to lowercase */
        char *str = pstrdup(interptype_name(interp));
        str[0] = tolower(str[0]);
        meos_error(ERROR, MEOS_ERR_INVALID_ARG_TYPE,
          "Cannot transform input value to a temporal sequence with %s interpolation",
          str);
        return NULL;
      }
      /* Given the above test, the result subtype is TSequence */
      return (TSequence *) tsequence_set_interp((TSequence *) temp, interp);
    }
    default: /* TSEQUENCESET */
      return trgeoseqset_to_tsequence((TSequenceSet *) temp);
  }
}

/**
 * @ingroup meos_rgeo_transf
 * @brief Return a temporal value transformed to a temporal sequence
 * @param[in] temp Temporal value
 * @param[in] interp_str Interpolation string, may be NULL
 * @csqlfn #Trgeometry_to_tsequence()
 */
TSequence *
trgeo_to_tsequence(const Temporal *temp, const char *interp_str)
{
  /* Ensure validity of the arguments */
#if MEOS
  if (! ensure_not_null((void *) temp))
    return NULL;
#else
  assert(temp);
#endif /* MEOS */

  interpType interp;
  /* If the interpolation is not NULL */
  if (interp_str)
    interp = interptype_from_string(interp_str);
  else
  {
    if (temp->subtype == TSEQUENCE)
      interp = MEOS_FLAGS_GET_INTERP(temp->flags);
    else
      interp = MEOS_FLAGS_GET_CONTINUOUS(temp->flags) ? LINEAR : STEP;
  }
  return trgeo_tsequence(temp, interp);
}

/**
 * @ingroup meos_internal_temporal_transf
 * @brief Return a temporal value transformed to a temporal sequence set
 * @param[in] temp Temporal value
 * @param[in] interp Interpolation
 * @csqlfn #Trgeometry_to_tsequenceset()
 */
TSequenceSet *
trgeo_tsequenceset(const Temporal *temp, interpType interp)
{
  /* Ensure validity of the arguments */
#if MEOS
  if (! ensure_not_null((void *) temp))
    return NULL;
#else
  assert(temp);
#endif /* MEOS */
  if (! ensure_valid_interp(temp->temptype, interp))
    return NULL;
  /* Discrete interpolation is only valid for TSequence */
  if (interp == DISCRETE)
  {
    meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
      "The temporal sequence set cannot have discrete interpolation");
    return NULL;
  }

  assert(temptype_subtype(temp->subtype));
  switch (temp->subtype)
  {
    case TINSTANT:
      return tinstant_to_tsequenceset((TInstant *) temp, interp);
    case TSEQUENCE:
      return tsequence_to_tsequenceset_interp((TSequence *) temp, interp);
    default: /* TSEQUENCESET */
      /* Since interp != DISCRETE the result subtype is TSequenceSet */
      return (TSequenceSet *) tsequenceset_set_interp((TSequenceSet *) temp,
        interp);
  }
}

/**
 * @ingroup meos_rgeo_transf
 * @brief Return a temporal value transformed to a temporal sequence set
 * @param[in] temp Temporal value
 * @param[in] interp_str Interpolation string
 * @csqlfn #Trgeometry_to_tsequenceset()
 */
TSequenceSet *
trgeo_to_tsequenceset(const Temporal *temp, const char *interp_str)
{
  /* Ensure validity of the arguments */
#if MEOS
  if (! ensure_not_null((void *) temp))
    return NULL;
#else
  assert(temp);
#endif /* MEOS */

  interpType interp;
  /* If the interpolation is not NULL */
  if (interp_str)
    interp = interptype_from_string(interp_str);
  else
  {
    interp = MEOS_FLAGS_GET_INTERP(temp->flags);
    if (interp == INTERP_NONE || interp == DISCRETE)
      interp = MEOS_FLAGS_GET_CONTINUOUS(temp->flags) ? LINEAR : STEP;
  }
  return temporal_tsequenceset(temp, interp);
}

/*****************************************************************************
 * Restriction functions
 *****************************************************************************/

/*****************************************************************************/
