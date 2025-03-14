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
 * @file
 * @brief Spatial functions for temporal network points
 */

#include "npoint/tnpoint_spatialfuncs.h"

/* C */
#include <assert.h>
/* MEOS */
#include <meos.h>
#include <meos_internal.h>
#include "general/type_util.h"
#include "geo/pgis_types.h"
#include "geo/tgeo_spatialfuncs.h"

/*****************************************************************************
 * Parameter tests
 *****************************************************************************/

/**
 * @brief Ensure that two temporal network point instants have the same route
 * identifier
 */
bool
ensure_same_rid_tnpointinst(const TInstant *inst1, const TInstant *inst2)
{
  if (tnpointinst_route(inst1) != tnpointinst_route(inst2))
  {
    meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
      "All network points composing a temporal sequence must have same route identifier");
    return false;
  }
  return true;
}

/*****************************************************************************
 * Interpolation functions defining functionality required by tsequence.c
 * that must be implemented by each temporal type
 *****************************************************************************/

/**
 * @brief Return a npoint interpolated from a npoint segment with respect to 
 * the fraction of its total length
 * @param[in] start,end Circular buffers defining the segment
 * @param[in] ratio Float between 0 and 1 representing the fraction of the
 * total length of the segment where the interpolated buffer must be located
 */
Datum
npointsegm_interpolate(Datum start, Datum end, long double ratio)
{
  Npoint *np1 = DatumGetNpointP(start);
  Npoint *np2 = DatumGetNpointP(end);
  double pos = np1->pos + (double) ((long double)(np2->pos - np1->pos) * ratio);
  Npoint *result = npoint_make(np1->rid, pos);
  return PointerGetDatum(result);
}

/**
 * @brief Return true if a segment of a temporal network point value intersects
 * a base value at the timestamp
 * @param[in] inst1,inst2 Temporal instants defining the segment
 * @param[in] value Base value
 * @param[out] t Timestamp
 */
bool
tnpointsegm_intersection_value(const TInstant *inst1, const TInstant *inst2,
  Datum value, TimestampTz *t)
{
  const Npoint *np1 = DatumGetNpointP(tinstant_val(inst1));
  const Npoint *np2 = DatumGetNpointP(tinstant_val(inst2));
  const Npoint *np = DatumGetNpointP(value);
  double min = Min(np1->pos, np2->pos);
  double max = Max(np1->pos, np2->pos);
  /* if value is to the left or to the right of the range */
  if ((np->rid != np1->rid) ||
    (np->pos < np1->pos && np->pos < np2->pos) ||
    (np->pos > np1->pos && np->pos > np2->pos))
  // if (np->rid != np1->rid || (np->pos < min && np->pos > max))
    return false;

  double range = (max - min);
  double partial = (np->pos - min);
  double fraction = np1->pos < np2->pos ? partial / range : 1 - partial / range;
  if (fabs(fraction) < MEOS_EPSILON || fabs(fraction - 1.0) < MEOS_EPSILON)
    return false;

  if (t)
  {
    double duration = (double) (inst2->t - inst1->t);
    *t = inst1->t + (long) (duration * fraction);
  }
  return true;
}

/*****************************************************************************
 * NPoints Functions
 * Return the network points covered by a temporal network point
 *****************************************************************************/

/**
 * @brief Return the network points covered by a temporal network point
 * @param[in] seq Temporal network point
 * @param[out] count Number of elements of the output array
 * @note Only the particular cases returning points are covered
 */
static Npoint **
tnpointseq_discstep_npoints(const TSequence *seq, int *count)
{
  Npoint **result = palloc(sizeof(Npoint *) * seq->count);
  for (int i = 0; i < seq->count; i++)
    result[i] = DatumGetNpointP(tinstant_val(TSEQUENCE_INST_N(seq, i)));
  *count = seq->count;
  return result;
}

/**
 * @brief Return the pointers to the network points covered by a temporal
 * network point
 * @param[in] ss Temporal network point
 * @param[out] count Number of elements of the output array
 * @note Only the particular cases returning points are covered
 */
static Npoint **
tnpointseqset_step_npoints(const TSequenceSet *ss, int *count)
{
  Npoint **result = palloc(sizeof(Npoint *) * ss->totalcount);
  int npoints = 0;
  for (int i = 0; i < ss->count; i++)
  {
    const TSequence *seq = TSEQUENCESET_SEQ_N(ss, i);
    for (int j = 0; j < seq->count; j++)
      result[npoints++] = DatumGetNpointP(
        tinstant_val(TSEQUENCE_INST_N(seq, j)));
  }
  *count = npoints;
  return result;
}

/*****************************************************************************
 * Geometric positions (Trajectotry) functions
 * Return the geometric positions covered by a temporal network point
 *****************************************************************************/

/**
 * @brief Return the trajectory of a temporal network point
 * @param[in] inst Temporal network point
 */
GSERIALIZED *
tnpointinst_trajectory(const TInstant *inst)
{
  const Npoint *np = DatumGetNpointP(tinstant_val(inst));
  return npoint_geom(np);
}

/**
 * @brief Return the trajectory a temporal network point
 * @param[in] seq Temporal network point
 */
GSERIALIZED *
tnpointseq_trajectory(const TSequence *seq)
{
  /* Instantaneous sequence */
  if (seq->count == 1)
    return tnpointinst_trajectory(TSEQUENCE_INST_N(seq, 0));

  GSERIALIZED *result;
  if (MEOS_FLAGS_LINEAR_INTERP(seq->flags))
  {
    Nsegment *segment = tnpointseq_linear_positions(seq);
    result = nsegment_geom(segment);
    pfree(segment);
  }
  else
  {
    int count;
    /* The following function does not remove duplicate values */
    Npoint **points = tnpointseq_discstep_npoints(seq, &count);
    result = npointarr_geom(points, count);
    pfree(points);
  }
  return result;
}

/**
 * @brief Return the trajectory of a temporal network point
 * @param[in] ss Temporal network point
 */
GSERIALIZED *
tnpointseqset_trajectory(const TSequenceSet *ss)
{
  /* Singleton sequence set */
  if (ss->count == 1)
    return tnpointseq_trajectory(TSEQUENCESET_SEQ_N(ss, 0));

  int count;
  GSERIALIZED *result;
  if (MEOS_FLAGS_LINEAR_INTERP(ss->flags))
  {
    Nsegment **segments = tnpointseqset_positions(ss, &count);
    result = nsegmentarr_geom(segments, count);
    pfree_array((void **) segments, count);
  }
  else
  {
    Npoint **points = tnpointseqset_step_npoints(ss, &count);
    result = npointarr_geom(points, count);
    pfree(points);
  }
  return result;
}

/**
 * @ingroup meos_temporal_spatial_transf
 * @brief Return the geometry covered by a temporal network point
 * @param[in] temp Temporal network point
 * @csqlfn #Tnpoint_trajectory()
 */
GSERIALIZED *
tnpoint_trajectory(const Temporal *temp)
{
  assert(temptype_subtype(temp->subtype));
  switch (temp->subtype)
  {
    case TINSTANT:
      return tnpointinst_trajectory((TInstant *) temp);
    case TSEQUENCE:
      return tnpointseq_trajectory((TSequence *) temp);
    default: /* TSEQUENCESET */
      return tnpointseqset_trajectory((TSequenceSet *) temp);
  }
}

/**
 * @brief Return the trajectory of two network points
 * @param[in] np1, np2 Network points
 */
static Datum
tnpointseqsegm_trajectory(const Npoint *np1, const Npoint *np2)
{
  assert(np1->rid == np2->rid && np1->pos != np2->pos);
  GSERIALIZED *line = route_geom(np1->rid);
  if ((np1->pos == 0 && np2->pos == 1) || (np2->pos == 0 && np1->pos == 1))
    return PointerGetDatum(line);

  GSERIALIZED *traj;
  if (np1->pos < np2->pos)
    traj = line_substring(line, np1->pos, np2->pos);
  else /* np1->pos >= np2->pos */
  {
    GSERIALIZED *traj2 = line_substring(line, np2->pos, np1->pos);
    traj = geo_reverse(traj2);
    pfree(traj2);
  }
  pfree(line);
  return PointerGetDatum(traj);
}

/*****************************************************************************
 * Approximate equality for network points
 *****************************************************************************/

/**
 * @brief Return true if two network points are approximately equal with
 * respect to an epsilon value
 * @details Two network points may be have different route identifier but
 * represent the same spatial point at the intersection of the two route
 * identifiers
 */
bool
npoint_same(const Npoint *np1, const Npoint *np2)
{
  /* Equal route identifier and same position */
  if (np1->rid == np2->rid && fabs(np1->pos - np2->pos) > MEOS_EPSILON)
    return false;
  /* Same point */
  Datum point1 = PointerGetDatum(npoint_geom(np1));
  Datum point2 = PointerGetDatum(npoint_geom(np2));
  bool result = datum_point_same(point1, point2);
  pfree(DatumGetPointer(point1)); pfree(DatumGetPointer(point2));
  return result;
}

/*****************************************************************************
 * Length functions
 *****************************************************************************/

/**
 * @brief Length traversed by a temporal network point
 */
double
tnpointseq_length(const TSequence *seq)
{
  /* Instantaneous sequence */
  if (seq->count == 1)
    return 0;

  const TInstant *inst = TSEQUENCE_INST_N(seq, 0);
  const Npoint *np1 = DatumGetNpointP(tinstant_val(inst));
  double length = route_length(np1->rid);
  double fraction = 0;
  for (int i = 1; i < seq->count; i++)
  {
    inst = TSEQUENCE_INST_N(seq, i);
    const Npoint *np2 = DatumGetNpointP(tinstant_val(inst));
    fraction += fabs(np2->pos - np1->pos);
    np1 = np2;
  }
  return length * fraction;
}

/**
 * @brief Length traversed by a temporal network point
 */
double
tnpointseqset_length(const TSequenceSet *ss)
{
  double result = 0.0;
  for (int i = 0; i < ss->count; i++)
    result += tnpointseq_length(TSEQUENCESET_SEQ_N(ss, i));
  return result;
}

/**
 * @ingroup meos_temporal_spatial_accessor
 * @brief Length traversed by a temporal network point
 * @param[in] temp Temporal point
 * @csqlfn #Tnpoint_length()
 */
double
tnpoint_length(const Temporal *temp)
{
  assert(temptype_subtype(temp->subtype));
  if (! MEOS_FLAGS_LINEAR_INTERP(temp->flags))
    return 0.0;
  else if (temp->subtype == TSEQUENCE)
    return tnpointseq_length((TSequence *) temp);
  else /* TSEQUENCESET */
    return tnpointseqset_length((TSequenceSet *) temp);
}

/*****************************************************************************/

/**
 * @brief Return the cumulative length traversed by a temporal point
 * @pre The sequence has linear interpolation
 */
static TSequence *
tnpointseq_cumulative_length(const TSequence *seq, double prevlength)
{
  assert(seq); assert(MEOS_FLAGS_LINEAR_INTERP(seq->flags));

  /* Instantaneous sequence */
  if (seq->count == 1)
  {
    TInstant *inst = tinstant_make(Float8GetDatum(prevlength), T_TFLOAT,
      TSEQUENCE_INST_N(seq, 0)->t);
    return tinstant_to_tsequence_free(inst, LINEAR);
  }

  /* General case */
  TInstant **instants = palloc(sizeof(TInstant *) * seq->count);
  const TInstant *inst1 = TSEQUENCE_INST_N(seq, 0);
  Npoint *np1 = DatumGetNpointP(tinstant_val(inst1));
  double rlength = route_length(np1->rid);
  double length = prevlength;
  instants[0] = tinstant_make(Float8GetDatum(length), T_TFLOAT, inst1->t);
  for (int i = 1; i < seq->count; i++)
  {
    const TInstant *inst2 = TSEQUENCE_INST_N(seq, i);
    Npoint *np2 = DatumGetNpointP(tinstant_val(inst2));
    length += fabs(np2->pos - np1->pos) * rlength;
    instants[i] = tinstant_make(Float8GetDatum(length), T_TFLOAT, inst2->t);
    np1 = np2;
  }
  return tsequence_make_free(instants, seq->count, seq->period.lower_inc,
    seq->period.upper_inc, LINEAR, NORMALIZE);
}

/**
 * @brief Cumulative length traversed by a temporal network point
 */
static TSequenceSet *
tnpointseqset_cumulative_length(const TSequenceSet *ss)
{
  TSequence **sequences = palloc(sizeof(TSequence *) * ss->count);
  double length = 0;
  for (int i = 0; i < ss->count; i++)
  {
    const TSequence *seq = TSEQUENCESET_SEQ_N(ss, i);
    sequences[i] = tnpointseq_cumulative_length(seq, length);
    const TInstant *end = TSEQUENCE_INST_N(sequences[i], seq->count - 1);
    length += DatumGetFloat8(tinstant_val(end));
  }
  return tsequenceset_make_free(sequences, ss->count, NORMALIZE_NO);
}

/**
 * @ingroup meos_temporal_spatial_accessor
 * @brief Cumulative length traversed by a temporal network point
 * @param[in] temp Temporal point
 * @csqlfn #Tnpoint_cumulative_length()
 */
Temporal *
tnpoint_cumulative_length(const Temporal *temp)
{
  assert(temptype_subtype(temp->subtype));
  if (! MEOS_FLAGS_LINEAR_INTERP(temp->flags))
    return temporal_from_base_temp(Float8GetDatum(0.0), T_TFLOAT, temp);
  else if (temp->subtype == TSEQUENCE)
    return (Temporal *) tnpointseq_cumulative_length((TSequence *) temp, 0);
  else /* TSEQUENCESET */
    return (Temporal *) tnpointseqset_cumulative_length((TSequenceSet *) temp);
}

/*****************************************************************************
 * Speed functions
 *****************************************************************************/

/**
 * @brief Speed of a temporal network point
 * @csqlfn #Tnpoint_speed()
 */
static TSequence *
tnpointseq_speed(const TSequence *seq)
{
  assert(seq); assert(tspatial_type(seq->temptype));
  assert(MEOS_FLAGS_LINEAR_INTERP(seq->flags));

  /* Instantaneous sequence */
  if (seq->count == 1)
    return NULL;

  /* General case */
  TInstant **instants = palloc(sizeof(TInstant *) * seq->count);
  const TInstant *inst1 = TSEQUENCE_INST_N(seq, 0);
  Npoint *np1 = DatumGetNpointP(tinstant_val(inst1));
  double rlength = route_length(np1->rid);
  const TInstant *inst2 = NULL; /* make the compiler quiet */
  double speed = 0; /* make the compiler quiet */
  for (int i = 0; i < seq->count - 1; i++)
  {
    inst2 = TSEQUENCE_INST_N(seq, i + 1);
    Npoint *np2 = DatumGetNpointP(tinstant_val(inst2));
    double length = fabs(np2->pos - np1->pos) * rlength;
    speed = length / (((double)(inst2->t) - (double)(inst1->t)) / 1000000);
    instants[i] = tinstant_make(Float8GetDatum(speed), T_TFLOAT, inst1->t);
    inst1 = inst2;
    np1 = np2;
  }
  instants[seq->count-1] = tinstant_make(Float8GetDatum(speed), T_TFLOAT,
    inst2->t);
  /* The resulting sequence has step interpolation */
  return tsequence_make_free(instants, seq->count,
    seq->period.lower_inc, seq->period.upper_inc, STEP, true);
}

/**
 * @brief Speed of a temporal network point
 */
static TSequenceSet *
tnpointseqset_speed(const TSequenceSet *ss)
{
  assert(ss); assert(tspatial_type(ss->temptype));
  assert(MEOS_FLAGS_LINEAR_INTERP(ss->flags));
  TSequence **sequences = palloc(sizeof(TSequence *) * ss->count);
  int nseqs = 0;
  for (int i = 0; i < ss->count; i++)
  {
    TSequence *seq = tnpointseq_speed(TSEQUENCESET_SEQ_N(ss, i));
    if (seq)
      sequences[nseqs++] = seq;
  }
  /* The resulting sequence set has step interpolation */
  return tsequenceset_make_free(sequences, nseqs, STEP);
}

/**
 * @ingroup meos_temporal_spatial_accessor
 * @brief Speed of a temporal network point
 * @param[in] temp Temporal point
 * @csqlfn #Tnpoint_speed()
 */
Temporal *
tnpoint_speed(const Temporal *temp)
{
  /* Ensure validity of the arguments */
  if (! ensure_not_null((void *) temp) ||
      ! ensure_tspatial_type(temp->temptype) ||
      ! ensure_linear_interp(temp->flags))
    return NULL;

  assert(temptype_subtype(temp->subtype));
  switch (temp->subtype)
  {
    case TINSTANT:
      return NULL;
    case TSEQUENCE:
      return (Temporal *) tnpointseq_speed((TSequence *) temp);
    default: /* TSEQUENCESET */
      return (Temporal *) tnpointseqset_speed((TSequenceSet *) temp);
  }
}

/*****************************************************************************
 * Time-weighed centroid for temporal network points
 *****************************************************************************/

/**
 * @ingroup meos_temporal_spatial_accessor
 * @brief Return the time-weighed centroid of a temporal network point
 * @param[in] temp Temporal point
 * @csqlfn #Tnpoint_twcentroid()
 */
GSERIALIZED *
tnpoint_twcentroid(const Temporal *temp)
{
  Temporal *tgeom = tnpoint_tgeompoint(temp);
  GSERIALIZED *result = tpoint_twcentroid(tgeom);
  pfree(tgeom);
  return result;
}

/*****************************************************************************
 * Temporal azimuth
 *****************************************************************************/

/**
 * @brief Temporal azimuth of two temporal network point instants (iteration
 * function)
 */
static TInstant **
tnpointsegm_azimuth_iter(const TInstant *inst1, const TInstant *inst2,
  int *count)
{
  const Npoint *np1 = DatumGetNpointP(tinstant_val(inst1));
  const Npoint *np2 = DatumGetNpointP(tinstant_val(inst2));

  /* Constant segment */
  if (np1->pos == np2->pos)
  {
    *count = 0;
    return NULL;
  }

/* Find all vertices in the segment */
  GSERIALIZED *traj = DatumGetGserializedP(tnpointseqsegm_trajectory(np1, np2));
  int countVertices = line_numpoints(traj);
  TInstant **result = palloc(sizeof(TInstant *) * countVertices);
  GSERIALIZED *vertex1 = line_point_n(traj, 1); /* 1-based */
  double azimuth;
  TimestampTz time = inst1->t;
  for (int i = 0; i < countVertices - 1; i++)
  {
    GSERIALIZED *vertex2 = line_point_n(traj, i + 2); /* 1-based */
    double fraction = line_locate_point(traj, vertex2);
    assert(! datum_point_eq(PointerGetDatum(vertex1),
      PointerGetDatum(vertex2)));
    geom_azimuth(vertex1, vertex2, &azimuth);
    result[i] = tinstant_make(Float8GetDatum(azimuth), T_TFLOAT, time);
    pfree(vertex1);
    vertex1 = vertex2;
    time =  inst1->t + (long) ((double) (inst2->t - inst1->t) * fraction);
  }
  pfree(traj);
  pfree(vertex1);
  *count = countVertices - 1;
  return result;
}

/**
 * @brief Helper function to make a sequence from a set of instants
 */
static TSequence *
tsequence_assemble_instants(TInstant ***instants, int *countinsts,
  int totalinsts, int from, int to, bool lower_inc, TimestampTz last_time)
{
  TInstant **allinstants = palloc(sizeof(TInstant *) * (totalinsts + 1));
  int ninsts = 0;
  for (int i = from; i < to; i++)
  {
    for (int j = 0; j < countinsts[i]; j++)
      allinstants[ninsts++] = instants[i][j];
    if (instants[i])
      pfree(instants[i]);
  }
  /* Add closing instant */
  Datum last_value = tinstant_val(allinstants[ninsts - 1]);
  allinstants[ninsts++] = tinstant_make(last_value, T_TFLOAT, last_time);
  /* Resulting sequence has step interpolation */
  return tsequence_make_free(allinstants, ninsts, lower_inc, true, STEP, true);
}

/**
 * @brief Temporal azimuth of a temporal network point of sequence subtype
 * (iterator function)
 */
static int
tnpointseq_azimuth_iter(const TSequence *seq, TSequence **result)
{
  /* Instantaneous sequence */
  if (seq->count == 1)
    return 0;

  /* General case */
  TInstant ***instants = palloc(sizeof(TInstant *) * (seq->count - 1));
  int *ninsts = palloc0(sizeof(int) * (seq->count - 1));
  int totalinsts = 0; /* number of created instants so far */
  int nseqs = 0; /* number of created sequences */
  int m = 0; /* index of the segment from which to assemble instants */
  const TInstant *inst1 = TSEQUENCE_INST_N(seq, 0);
  bool lower_inc = seq->period.lower_inc;
  for (int i = 0; i < seq->count - 1; i++)
  {
    const TInstant *inst2 = TSEQUENCE_INST_N(seq, i + 1);
    instants[i] = tnpointsegm_azimuth_iter(inst1, inst2, &ninsts[i]);
    /* If constant segment */
    if (ninsts[i] == 0)
    {
      if (totalinsts != 0)
      {
        /* Assemble all instants created so far */
        result[nseqs++] = tsequence_assemble_instants(instants, ninsts,
          totalinsts, m, i, lower_inc, inst1->t);
        /* Indicate that we have consommed all instants created so far */
        m = i;
        totalinsts = 0;
      }
    }
    else
    {
      totalinsts += ninsts[i];
    }
    inst1 = inst2;
    lower_inc = true;
  }
  if (totalinsts != 0)
  {
    /* Assemble all instants created so far */
    result[nseqs++] = tsequence_assemble_instants(instants, ninsts,
      totalinsts, m, seq->count - 1, lower_inc, inst1->t);
  }
  pfree(instants);
  pfree(ninsts);
  return nseqs;
}

/**
 * @brief Temporal azimuth of a temporal network point of sequence subtype
 */
static TSequenceSet *
tnpointseq_azimuth(const TSequence *seq)
{
  TSequence **sequences = palloc(sizeof(TSequence *) * (seq->count - 1));
  int count = tnpointseq_azimuth_iter(seq, sequences);
  /* Resulting sequence set has step interpolation */
  return tsequenceset_make_free(sequences, count, true);
}

/**
 * @brief Temporal azimuth of a temporal network point of sequence set subtype
 */
static TSequenceSet *
tnpointseqset_azimuth(const TSequenceSet *ss)
{
  if (ss->count == 1)
    return tnpointseq_azimuth(TSEQUENCESET_SEQ_N(ss, 0));

  TSequence **sequences = palloc(sizeof(TSequence *) * ss->totalcount);
  int nseqs = 0;
  for (int i = 0; i < ss->count; i++)
    nseqs += tnpointseq_azimuth_iter(TSEQUENCESET_SEQ_N(ss, i), &sequences[nseqs]);
  /* Resulting sequence set has step interpolation */
  return tsequenceset_make_free(sequences, nseqs, STEP);
}

/**
 * @ingroup meos_temporal_spatial_accessor
 * @brief Temporal azimuth of a temporal network point
 * @param[in] temp Temporal point
 * @csqlfn #Tnpoint_azimuth()
 */
Temporal *
tnpoint_azimuth(const Temporal *temp)
{
  assert(temptype_subtype(temp->subtype));
  if (! MEOS_FLAGS_LINEAR_INTERP(temp->flags))
    return NULL;
  else if (temp->subtype == TSEQUENCE)
    return (Temporal *) tnpointseq_azimuth((TSequence *) temp);
  else /* TSEQUENCESET */
    return (Temporal *) tnpointseqset_azimuth((TSequenceSet *) temp);
}

/*****************************************************************************
 * Restriction functions
 *****************************************************************************/

/**
 * @ingroup meos_internal_temporal_restrict
 * @brief Return a temporal network point restricted to (the complement of) a
 * geometry
 */
Temporal *
tnpoint_restrict_geom(const Temporal *temp, const GSERIALIZED *gs,
  const Span *zspan, bool atfunc)
{
  /* Ensure validity of the arguments */
  if (! ensure_not_null((void *) temp) || ! ensure_not_null((void *) gs) ||
      ! ensure_same_srid(tspatial_srid(temp), gserialized_get_srid(gs)) ||
      ! ensure_has_not_Z_geo(gs))
    return NULL;

  /* Empty geometry */
  if (gserialized_is_empty(gs))
    return atfunc ? NULL : temporal_copy(temp);

  Temporal *tempgeom = tnpoint_tgeompoint(temp);
  Temporal *resgeom = tgeo_restrict_geom(tempgeom, gs, zspan, atfunc);
  Temporal *result = NULL;
  if (resgeom)
  {
    /* We do not call the function tgeompoint_tnpoint to avoid
     * roundoff errors */
    SpanSet *ss = temporal_time(resgeom);
    result = temporal_restrict_tstzspanset(temp, ss, REST_AT);
    pfree(resgeom);
    pfree(ss);
  }
  pfree(tempgeom);
  return result;
}

#if MEOS
/**
 * @ingroup meos_temporal_restrict
 * @brief Return a temporal network point restricted to a geometry
 * @param[in] temp Temporal point
 * @param[in] gs Geometry
 * @param[in] zspan Span of values to restrict the Z dimension
 * @csqlfn #Tnpoint_at_geom()
 */
Temporal *
tnpoint_at_geom(const Temporal *temp, const GSERIALIZED *gs,
  const Span *zspan)
{
  /* Ensure validity of the arguments */
  if (! ensure_valid_tgeo_geo(temp, gs))
    return NULL;
  return tnpoint_restrict_geom(temp, gs, zspan, REST_AT);
}

/**
 * @ingroup meos_temporal_restrict
 * @brief Return a temporal point restricted to (the complement of) a geometry
 * @param[in] temp Temporal point
 * @param[in] gs Geometry
 * @param[in] zspan Span of values to restrict the Z dimension
 * @csqlfn #Tnpoint_minus_geom()
 */
Temporal *
tnpoint_minus_geom(const Temporal *temp, const GSERIALIZED *gs,
  const Span *zspan)
{
  /* Ensure validity of the arguments */
  if (! ensure_valid_tgeo_geo(temp, gs))
    return NULL;
  return tnpoint_restrict_geom(temp, gs, zspan, REST_MINUS);
}
#endif /* MEOS */

/*****************************************************************************/

/**
 * @ingroup meos_internal_temporal_restrict
 * @brief Return a temporal network point restricted to (the complement of) a
 * spatiotemporal box
 * @param[in] temp Temporal network point
 * @param[in] box Spatiotemporal box
 * @param[in] border_inc True when the box contains the upper border
 * @param[in] atfunc True if the restriction is at, false for minus
 */
Temporal *
tnpoint_restrict_stbox(const Temporal *temp, const STBox *box, bool border_inc,
  bool atfunc)
{
  Temporal *tgeom = tnpoint_tgeompoint(temp);
  Temporal *tgeomres = tgeo_restrict_stbox(tgeom, box, border_inc, atfunc);
  Temporal *result = NULL;
  if (tgeomres)
  {
    result = tgeompoint_tnpoint(tgeomres);
    pfree(tgeomres);
  }
  return result;
}

#if MEOS
/**
 * @ingroup meos_temporal_restrict
 * @brief Return a temporal network point restricted to a geometry
 * @param[in] temp Temporal network point
 * @param[in] box Spatiotemporal box
 * @param[in] border_inc True when the box contains the upper border
 * @sqlfn #Tnpoint_at_stbox()
 */
Temporal *
tnpoint_at_stbox(const Temporal *temp, const STBox *box, bool border_inc)
{
  return tnpoint_restrict_stbox(temp, box, border_inc, REST_AT);
}

/**
 * @ingroup meos_temporal_restrict
 * @brief Return a temporal point restricted to (the complement of) a geometry
 * @param[in] temp Temporal network point
 * @param[in] box Spatiotemporal box
 * @param[in] border_inc True when the box contains the upper border
 * @sqlfn #Tnpoint_minus_stbox()
 */
Temporal *
tnpoint_minus_stbox(const Temporal *temp, const STBox *box, bool border_inc)
{
  return tnpoint_restrict_stbox(temp, box, border_inc, REST_MINUS);
}
#endif /* MEOS */

/*****************************************************************************/
