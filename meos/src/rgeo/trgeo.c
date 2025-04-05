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
trgeo_geom(const Temporal *temp)
{
  Datum result;
  assert(temptype_subtype(temp->subtype));
  if (temp->subtype == TINSTANT)
    result = trgeoinst_geom((const TInstant *) temp);
  else if (temp->subtype == TSEQUENCE)
    result = trgeoseq_geom((const TSequence *) temp);
  else /* temp->subtype == TSEQUENCESET */
    result = trgeoseqset_geom((const TSequenceSet *) temp);
  return result;
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

/*****************************************************************************
 * Restriction functions
 *****************************************************************************/

/**
 * @ingroup libmeos_internal_rgeo_restrict
 * @brief Return the base value of a temporal value at the timestamp
 * @sqlfn valueAtTimestamp
 */
bool
trgeo_value_at_timestamptz(const Temporal *temp, TimestampTz t, bool strict,
  Datum *result)
{
  Datum pose_datum;
  bool found = temporal_value_at_timestamptz(temp, t, strict, &pose_datum);
  if (found)
  {
    /* Apply pose to reference geometry */
    const GSERIALIZED *gs = DatumGetGserializedP(trgeo_geom(temp));
    GSERIALIZED *result_gs;
    Pose *pose = DatumGetPoseP(pose_datum);
    LWGEOM *geom = lwgeom_from_gserialized(gs);
    LWGEOM *result_geom = lwgeom_clone_deep(geom);
    lwgeom_apply_pose(result_geom, pose);
    if (result_geom->bbox)
      lwgeom_refresh_bbox(result_geom);
    lwgeom_free(geom);
    result_gs = geo_serialize(result_geom);
    lwgeom_free(result_geom);
    *result = PointerGetDatum(result_gs);
  }
  return found;
}

/*****************************************************************************/
