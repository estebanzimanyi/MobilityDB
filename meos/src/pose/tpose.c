/*****************************************************************************
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
 * @brief General functions for temporal pose objects.
 */

#include "pose/tpose.h"

/* C */
#include <assert.h>
/* Postgres */
#include <postgres.h>
#include <common/hashfn.h>
#include <utils/timestamp.h>
/* MEOS */
#include <meos.h>
#include <meos_pose.h>
#include <meos_internal.h>
#include "general/lifting.h"
#include "general/set.h"
#include "general/span.h"
#include "general/spanset.h"
#include "general/type_util.h"
#include "geo/tspatial_parser.h"
#include "pose/pose.h"

/*****************************************************************************
 * Input/output in WKT and EWKT format
 *****************************************************************************/

#if MEOS
/**
 * @ingroup meos_internal_temporal_inout
 * @brief Return a temporal pose instant from its Well-Known Text (WKT)
 * representation
 * @param[in] str String
 */
TInstant *
tposeinst_in(const char *str)
{
  assert(str);
  /* Call the superclass function to read the SRID at the beginning (if any) */
  Temporal *temp = tspatial_parse(&str, T_TPOSE);
  assert(temp->subtype == TINSTANT);
  return (TInstant *) temp;
}

/**
 * @ingroup meos_internal_temporal_inout
 * @brief Return a temporal pose sequence from its Well-Known Text (WKT)
 * representation
 * @param[in] str String
 * @param[in] interp Interpolation
 */
TSequence *
tposeseq_in(const char *str, interpType interp __attribute__((unused)))
{
  assert(str);
  /* Call the superclass function to read the SRID at the beginning (if any) */
  Temporal *temp = tspatial_parse(&str, T_TPOSE);
  if (! temp)
    return NULL;
  assert (temp->subtype == TSEQUENCE);
  return (TSequence *) temp;
}

/**
 * @ingroup meos_internal_temporal_inout
 * @brief Return a temporal pose sequence set from its Well-Known
 * Text (WKT) representation
 * @param[in] str String
 */
TSequenceSet *
tposeseqset_in(const char *str)
{
  assert(str);
  /* Call the superclass function to read the SRID at the beginning (if any) */
  Temporal *temp = tspatial_parse(&str, T_TPOSE);
  assert(temp->subtype == TSEQUENCESET);
  return (TSequenceSet *) temp;
}
#endif /* MEOS */

/*****************************************************************************/

#if MEOS
/**
 * @ingroup meos_pose_inout
 * @brief Return a temporal pose from its Well-Known Text (WKT) representation
 * @param[in] str String
 */
Temporal *
tpose_in(const char *str)
{
  /* Ensure validity of the arguments */
  if (! ensure_not_null((void *) str))
    return NULL;
  return tspatial_parse(&str, T_TPOSE);
}

/**
 * @ingroup meos_pose_inout
 * @brief Return the Well-Known Text (WKT) representation of a temporal pose
 * @param[in] temp Temporal pose
 * @param[in] maxdd Maximum number of decimal digits
 */
char *
tpose_out(const Temporal *temp, int maxdd)
{
  /* Ensure validity of the arguments */
  if (! ensure_not_null((void *) temp) || ! ensure_not_negative(maxdd) ||
      ! ensure_temporal_isof_type(temp, T_TPOSE))
    return NULL;
  return temporal_out(temp, maxdd);
}
#endif /* MEOS */

/*****************************************************************************
 * Conversion functions
 *****************************************************************************/

/**
 * @ingroup meos_pose_conversion
 * @brief Return a geometry point from a temporal pose
 * @param[in] temp Temporal pose
 */
Temporal *
tpose_tgeompoint(const Temporal *temp)
{
  /* Ensure validity of the arguments */
#if MEOS
  if (! ensure_not_null((void *) temp) || 
      ! ensure_temporal_isof_type(temp, T_TPOSE))
    return NULL;
#else
  assert(temp); assert(temp->temptype == T_TPOSE);
#endif /* MEOS */

  /* We only need to fill these parameters for tfunc_temporal */
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
 * Accessor functions
 *****************************************************************************/

/**
 * @ingroup meos_pose_accessor
 * @brief Return a copy of the start value of a temporal pose
 * @param[in] temp Temporal value
 * @return On error return @p NULL
 * @csqlfn #Temporal_start_value()
 */
Pose *
tpose_start_value(const Temporal *temp)
{
  /* Ensure validity of the arguments */
#if MEOS
  if (! ensure_not_null((void *) temp) || 
      ! ensure_temporal_isof_type(temp, T_TPOSE))
    return NULL;
#else
  assert(temp); assert(temp->temptype == T_TPOSE);
#endif /* MEOS */
  return DatumGetPoseP(temporal_start_value(temp));
}

/**
 * @ingroup meos_pose_accessor
 * @brief Return a copy of the end value of a temporal pose
 * @param[in] temp Temporal value
 * @return On error return @p NULL
 * @csqlfn #Temporal_end_value()
 */
Pose *
tpose_end_value(const Temporal *temp)
{
  /* Ensure validity of the arguments */
#if MEOS
  if (! ensure_not_null((void *) temp) || 
      ! ensure_temporal_isof_type(temp, T_TPOSE))
    return NULL;
#else
  assert(temp); assert(temp->temptype == T_TPOSE);
#endif /* MEOS */
  return DatumGetPoseP(temporal_end_value(temp));
}

/**
 * @ingroup meos_pose_accessor
 * @brief Return a copy of the n-th value of a temporal pose
 * @param[in] temp Temporal value
 * @param[in] n Number
 * @param[out] result Value
 * @csqlfn #Temporal_value_n()
 */
bool
tpose_value_n(const Temporal *temp, int n, Pose **result)
{
  /* Ensure validity of the arguments */
#if MEOS
  if (! ensure_not_null((void *) temp) || ! ensure_not_null((void *) result) ||
      ! ensure_temporal_isof_type(temp, T_TPOSE))
    return false;
#else
  assert(temp); assert(result); assert(temp->temptype == T_TPOSE);
#endif /* MEOS */
  Datum dresult;
  if (! temporal_value_n(temp, n, &dresult))
    return false;
  *result = DatumGetPoseP(dresult);
  return true;
}

/**
 * @ingroup meos_pose_accessor
 * @brief Return the array of copies of base values of a temporal pose
 * @param[in] temp Temporal value
 * @param[out] count Number of values in the output array
 * @csqlfn #Temporal_valueset()
 */
Pose **
tpose_values(const Temporal *temp, int *count)
{
  /* Ensure validity of the arguments */
#if MEOS
  if (! ensure_not_null((void *) temp) || ! ensure_not_null((void *) count) ||
      ! ensure_temporal_isof_type(temp, T_TPOSE))
    return NULL;
#else
  assert(temp); assert(count); assert(temp->temptype == T_TPOSE);
#endif /* MEOS */

  Datum *datumarr = temporal_vals(temp, count);
  Pose **result = palloc(sizeof(Pose *) * *count);
  for (int i = 0; i < *count; i++)
    result[i] = pose_copy(DatumGetPoseP(datumarr[i]));
  pfree(datumarr);
  return result;
}

/*****************************************************************************/

/**
 * @brief Return the points of a temporal pose
 */
Set *
tposeinst_points(const TInstant *inst)
{
  Pose *pose = DatumGetPoseP(tinstant_val(inst));
  Datum value = PointerGetDatum(pose_point(pose));
  return set_make_exp(&value, 1, 1, T_GEOMETRY, ORDER_NO);
}

/**
 * @brief Return the points of a temporal pose
 */
Set *
tposeseq_points(const TSequence *seq)
{
  Datum *values = palloc(sizeof(Datum) * seq->count);
  for (int i = 0; i < seq->count; i++)
  {
    const Pose *pose = DatumGetPoseP(tinstant_val(TSEQUENCE_INST_N(seq, i)));
    values[i] = PointerGetDatum(pose_point(pose));
  }
  datumarr_sort(values, seq->count, T_GEOMETRY);
  int count = datumarr_remove_duplicates(values, seq->count, T_GEOMETRY);
  return set_make_free(values, count, T_GEOMETRY, ORDER_NO);
}

/**
 * @brief Return the points of a temporal pose
 */
Set *
tposeseqset_points(const TSequenceSet *ss)
{
  Datum *values = palloc(sizeof(int64) * ss->totalcount);
  int nvalues = 0;
  for (int i = 0; i < ss->count; i++)
  {
    const TSequence *seq = TSEQUENCESET_SEQ_N(ss, i);
    for (int j = 0; j < seq->count; j++)
    {
      const TInstant *inst = TSEQUENCE_INST_N(seq, j);
      Pose *pose = DatumGetPoseP(tinstant_val(inst));
      values[nvalues++] = PointerGetDatum(pose_point(pose));
    }
  }
  datumarr_sort(values, ss->totalcount, T_GEOMETRY);
  int count = datumarr_remove_duplicates(values, ss->totalcount, T_GEOMETRY);
  return set_make_free(values, count, T_GEOMETRY, ORDER_NO);
}

/**
 * @ingroup meos_pose_accessor
 * @brief Return the array of points of a temporal pose
 * @csqlfn #Tpose_points()
 */
Set *
tpose_points(const Temporal *temp)
{
  /* Ensure validity of the arguments */
#if MEOS
  if (! ensure_not_null((void *) temp) ||
      ! ensure_temporal_isof_type(temp, T_TPOSE))
    return NULL;
#else
  assert(temp); assert(temp->temptype == T_TPOSE);
#endif /* MEOS */

  assert(temptype_subtype(temp->subtype));
  switch (temp->subtype)
  {
    case TINSTANT:
      return tposeinst_points((TInstant *) temp);
    case TSEQUENCE:
      return tposeseq_points((TSequence *) temp);
    default: /* TSEQUENCESET */
      return tposeseqset_points((TSequenceSet *) temp);
  }
}

/**
 * @ingroup meos_pose_accessor
 * @brief Return the value of a temporal pose at a timestamptz
 * @param[in] temp Temporal value
 * @param[in] t Timestamp
 * @param[in] strict True if the timestamp must belong to the temporal value,
 * false when it may be at an exclusive bound
 * @param[out] value Resulting value
 * @csqlfn #Temporal_value_at_timestamptz()
 */
bool
tpose_value_at_timestamptz(const Temporal *temp, TimestampTz t, bool strict,
  Pose **value)
{
  /* Ensure validity of the arguments */
#if MEOS
  if (! ensure_not_null((void *) temp) || ! ensure_not_null((void *) value) ||
      ! ensure_temporal_isof_type(temp, T_TPOSE))
    return false;
#else
  assert(temp); assert(value); assert(temp->temptype == T_TPOSE);
#endif /* MEOS */

  Datum res;
  bool result = temporal_value_at_timestamptz(temp, t, strict, &res);
  *value = DatumGetPoseP(res);
  return result;
}

/*****************************************************************************
 * Restriction functions
 *****************************************************************************/

/**
 * @ingroup meos_pose_restrict
 * @brief Return a temporal pose restricted to a pose
 * @param[in] temp Temporal value
 * @param[in] pose Value
 * @csqlfn #Temporal_at_value()
 */
Temporal *
tpose_at_value(const Temporal *temp, Pose *pose)
{
  /* Ensure validity of the arguments */
#if MEOS
  if (! ensure_not_null((void *) temp) || ! ensure_not_null((void *) pose) ||
      ! ensure_temporal_isof_type(temp, T_TPOSE))
    return NULL;
#else
  assert(temp); assert(pose); assert(temp->temptype == T_TPOSE);
#endif /* MEOS */
  return temporal_restrict_value(temp, PointerGetDatum(pose), REST_AT);
}

/**
 * @ingroup meos_pose_restrict
 * @brief Return a temporal pose restricted to the complement of a 
 * pose
 * @param[in] temp Temporal value
 * @param[in] pose Value
 * @csqlfn #Temporal_minus_value()
 */
Temporal *
tpose_minus_value(const Temporal *temp, Pose *pose)
{
#if MEOS
  if (! ensure_not_null((void *) temp) || ! ensure_not_null((void *) pose) ||
      ! ensure_temporal_isof_type(temp, T_TPOSE))
    return NULL;
#else
  assert(temp); assert(pose); assert(temp->temptype == T_TPOSE);
#endif /* MEOS */
  return temporal_restrict_value(temp, PointerGetDatum(pose), REST_MINUS);
}

/*****************************************************************************/
