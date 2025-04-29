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
 * @brief Spatiotemporal relationships for temporal circular buffers
 * @details These relationships are applied at each instant and result in a
 * temporal Boolean.
 *
 * The following relationships are supported: `tcontains`, `tcovers`,
 * `tdisjoint`, `tintersects`, `ttouches`, and `tdwithin`.
 */

#include "geo/tgeo_tempspatialrels.h"

/* C */
#include <assert.h>
#include <math.h>
/* PostgreSQL */
/* PostGIS */
#include <liblwgeom.h>
/* MEOS */
#include <meos.h>
#include <meos_cbuffer.h>
#include <meos_internal.h>
#include "temporal/lifting.h"
#include "temporal/tbool_ops.h"
#include "temporal/temporal_compops.h"
#include "temporal/tinstant.h"
#include "temporal/tsequence.h"
#include "temporal/type_util.h"
#include "geo/postgis_funcs.h"
#include "geo/tgeo_spatialfuncs.h"
#include "geo/tpoint_restrfuncs.h"
#include "geo/tgeo_spatialrels.h"
#include "cbuffer/cbuffer.h"
#include "cbuffer/tcbuffer_boxops.h"
#include "cbuffer/tcbuffer_spatialrels.h"

/*****************************************************************************
 * `tintersects` and `tdisjoint` functions
 *****************************************************************************/

/**
 * @brief Return a temporal Boolean that states whether a temporal geo instant
 * and a geometry intersect or are disjoint
 * @param[in] inst Temporal circular buffer
 * @param[in] gs Geometry
 * @param[in] tinter True when computing tintersects, false for tdisjoint
 * @param[in] func PostGIS function to be used
 */
TInstant *
tinterrel_tcbufferinst_geom(const TInstant *inst, const GSERIALIZED *gs,
  bool tinter, datum_func2 func)
{
  /* Result depends on whether we are computing tintersects or tdisjoint */
  bool result = DatumGetBool(func(tinstant_value_p(inst),
    GserializedPGetDatum(gs)));
  /* Invert the result for disjoint */
  if (! tinter)
    result = ! result;
  return tinstant_make(BoolGetDatum(result), T_TBOOL, inst->t);
}

/**
 * @brief Return a temporal Boolean that states whether a temporal circular
 * buffer sequence and a geometry intersect or are disjoint
 * @param[in] seq Temporal circular buffer
 * @param[in] gs Geometry
 * @param[in] tinter True when computing tintersects, false for tdisjoint
 * @param[in] func PostGIS function to be used
 */
TSequence *
tinterrel_tcbufferseq_discstep_geom(const TSequence *seq,
  const GSERIALIZED *gs, bool tinter, datum_func2 func)
{
  interpType interp = MEOS_FLAGS_GET_INTERP(seq->flags);
  assert(interp == DISCRETE || interp == STEP);
  TInstant **instants = palloc(sizeof(TInstant *) * seq->count);
  for (int i = 0; i < seq->count; i++)
  {
    const TInstant *inst = TSEQUENCE_INST_N(seq, i);
    bool result = DatumGetBool(func(tinstant_value_p(inst),
      GserializedPGetDatum(gs)));
    /* Invert the result for disjoint */
    if (! tinter)
      result = ! result;
    instants[i] = tinstant_make(BoolGetDatum(result), T_TBOOL, inst->t);
  }
  return tsequence_make_free(instants, seq->count, seq->period.lower_inc,
     seq->period.upper_inc, interp, NORMALIZE_NO);
}

/**
 * @brief Return an array of temporal Boolean sequences that state whether a
 * temporal circular buffer sequence with linear interpolation that is simple
 * and a geometry intersect or are disjoint
 * @param[in] seq Temporal point
 * @param[in] gs Geometry
 * @param[in] box Bounding box of the geometry
 * @param[in] tinter True when computing tintersects, false for tdisjoint
 * @param[out] count Number of elements in the resulting array
 * @pre The temporal circular buffer has linear interpolation and is simple,
 * that is, it is non self-intersecting
 */
static TSequence **
tinterrel_tcbufferseq_simple_geom(const TSequence *seq, const GSERIALIZED *gs,
  const STBox *box, bool tinter, int *count)
{
  /* The temporal sequence has at least 2 instants since
   * (1) the instantaneous full sequence test is done in the calling function
   * (2) the simple components of a non self-intersecting sequence have at
   *     least two instants */
  assert(seq->count > 1); assert(MEOS_FLAGS_LINEAR_INTERP(seq->flags));
  TSequence **result;
  /* Result depends on whether we are computing tintersects or tdisjoint */
  Datum datum_yes = tinter ? BoolGetDatum(true) : BoolGetDatum(false);
  Datum datum_no = tinter ? BoolGetDatum(false) : BoolGetDatum(true);

  /* Bounding box test */
  STBox *box1 = TSEQUENCE_BBOX_PTR(seq);
  if (! overlaps_stbox_stbox(box1, box))
  {
    result = palloc(sizeof(TSequence *));
    result[0] = tsequence_from_base_tstzspan(datum_no, T_TBOOL, &seq->period,
      STEP);
    *count = 1;
    return result;
  }

  GSERIALIZED *traj = tpointseq_linear_trajectory(seq, UNARY_UNION);
  GSERIALIZED *inter = geom_intersection2d(traj, gs);
  if (gserialized_is_empty(inter))
  {
    result = palloc(sizeof(TSequence *));
    result[0] = tsequence_from_base_tstzspan(datum_no, T_TBOOL, &seq->period,
      STEP);
    *count = 1;
    pfree(inter);
    return result;
  }

  const TInstant *start = TSEQUENCE_INST_N(seq, 0);
  const TInstant *end = TSEQUENCE_INST_N(seq, seq->count - 1);
  /* If the trajectory is a point the result is true due to the
   * non-empty intersection test above */
  if (seq->count == 2 &&
    datum_point_eq(tinstant_value_p(start), tinstant_value_p(end)))
  {
    result = palloc(sizeof(TSequence *));
    result[0] = tsequence_from_base_tstzspan(datum_yes, T_TBOOL, &seq->period,
      STEP);
    *count = 1;
    pfree(inter);
    return result;
  }

  /* Get the periods at which the temporal circular buffer intersects the
   * geometry */
  int npers;
  Span *periods = tpointseq_interperiods(seq, inter, &npers);
  if (npers == 0)
  {
    result = palloc(sizeof(TSequence *));
    result[0] = tsequence_from_base_tstzspan(datum_no, T_TBOOL, &seq->period,
      STEP);
    *count = 1;
    pfree(inter);
    return result;
  }
  SpanSet *ss;
  if (npers == 1)
    ss = minus_span_span(&seq->period, &periods[0]);
  else
  {
    /* It is necessary to sort the periods */
    SpanSet *ps1 = spanset_make_exp(periods, npers, npers, NORMALIZE, ORDER);
    ss = minus_span_spanset(&seq->period, ps1);
    pfree(ps1);
  }
  int nseqs = npers;
  if (ss)
    nseqs += ss->count;
  result = palloc(sizeof(TSequence *) * nseqs);
  for (int i = 0; i < npers; i++)
    result[i] = tsequence_from_base_tstzspan(datum_yes, T_TBOOL, &periods[i],
      STEP);
  if (ss)
  {
    for (int i = 0; i < ss->count; i++)
      result[i + npers] = tsequence_from_base_tstzspan(datum_no, T_TBOOL,
        SPANSET_SP_N(ss, i), STEP);
    tseqarr_sort(result, nseqs);
    pfree(ss);
  }
  *count = nseqs;
  pfree(periods);
  return result;
}

/**
 * @brief Return an array of temporal Boolean sequences that state whether a
 * temporal circular buffer sequence with linear interpolation and a geometry
 * intersect or are disjoint (iterator function)
 * @details The function splits the temporal circular buffer in an array of
 * fragments that are simple (that is, not self-intersecting) and loops for
 * each fragment
 * @param[in] seq Temporal point
 * @param[in] gs Geometry
 * @param[in] box Bounding box of the geometry
 * @param[in] tinter True when computing tintersects, false for tdisjoint
 * @param[in] func PostGIS function to be used
 * @param[out] count Number of elements in the output array
 */
static TSequence **
tinterrel_tcbufferseq_linear_geom_iter(const TSequence *seq,
  const GSERIALIZED *gs, const STBox *box, bool tinter, datum_func2 func,
  int *count)
{
  assert(seq); assert(box); assert(count); assert(tpoint_type(seq->temptype));
  assert(MEOS_FLAGS_LINEAR_INTERP(seq->flags));

  /* Instantaneous sequence */
  if (seq->count == 1)
  {
    TInstant *inst = tinterrel_tcbufferinst_geom(TSEQUENCE_INST_N(seq, 0), gs,
      tinter, func);
    TSequence **result = palloc(sizeof(TSequence *));
    result[0] = tinstant_to_tsequence_free(inst, STEP);
    *count = 1;
    return result;
  }

  /* Split the temporal circular buffer in an array of non self-intersecting
   * temporal points */
  int nsimple;
  TSequence **simpleseqs = tpointseq_make_simple(seq, &nsimple);
  TSequence ***sequences = palloc(sizeof(TSequence *) * nsimple);
  /* palloc0 used to initialize the counters to 0 */
  int *countseqs = palloc0(sizeof(int) * nsimple);
  int totalcount = 0;
  for (int i = 0; i < nsimple; i++)
  {
    sequences[i] = tinterrel_tcbufferseq_simple_geom(simpleseqs[i], gs, box,
      tinter, &countseqs[i]);
    totalcount += countseqs[i];
  }
  *count = totalcount;
  return tseqarr2_to_tseqarr(sequences, countseqs, nsimple, totalcount);
}

/**
 * @brief Return a temporal Boolean that states whether a temporal circular
 * buffer with linear interpolation and a geometry intersect or are disjoint
 * @details The function splits the temporal circular buffer in an array of
 * temporal sequences that are simple (that is, not self-intersecting) and
 * loops for each piece.
 * @param[in] seq Temporal point
 * @param[in] gs Geometry
 * @param[in] box Bounding box of the geometry
 * @param[in] func PostGIS function to be used
 * @param[in] tinter True when computing tintersects, false for tdisjoint
 */
TSequenceSet *
tinterrel_tcbufferseq_linear_geom(const TSequence *seq, const GSERIALIZED *gs,
  const STBox *box, bool tinter, datum_func2 func)
{
  assert(seq); assert(box); assert(tpoint_type(seq->temptype));
  assert(MEOS_FLAGS_GET_INTERP(seq->flags) != DISCRETE);

  /* Split the temporal circular buffer in an array of non self-intersecting
   * temporal circular buffers */
  int count;
  TSequence **sequences = tinterrel_tcbufferseq_linear_geom_iter(seq, gs, box,
    tinter, func, &count);
  return tsequenceset_make_free(sequences, count, NORMALIZE);
}

/**
 * @brief Return a temporal Boolean that states whether a temporal circular
 * buffer sequence set and a geometry intersect or are disjoint
 * @param[in] ss Temporal circular buffer
 * @param[in] gs Geometry
 * @param[in] box Bounding box of the geometry
 * @param[in] tinter True when computing tintersects, false for tdisjoint
 * @param[in] func PostGIS function to be used
 */
TSequenceSet *
tinterrel_tcbufferseqset_geom(const TSequenceSet *ss, const GSERIALIZED *gs,
  const STBox *box, bool tinter, datum_func2 func)
{
  /* Singleton sequence set */
  if (ss->count == 1)
  {
    if (MEOS_FLAGS_LINEAR_INTERP(ss->flags))
      return tinterrel_tcbufferseq_linear_geom(TSEQUENCESET_SEQ_N(ss, 0), gs,
        box, tinter, func);
    TSequence *res = tinterrel_tcbufferseq_discstep_geom(
      TSEQUENCESET_SEQ_N(ss, 0), gs, tinter, func);
    TSequenceSet *result = tsequence_to_tsequenceset(res);
    pfree(res);
    return result;
  }

  int totalcount;
  TSequence **allseqs;

  /* Linear interpolation */
  if (MEOS_FLAGS_LINEAR_INTERP(ss->flags))
  {
    TSequence ***sequences = palloc(sizeof(TSequence *) * ss->count);
    /* palloc0 used to initialize the counters to 0 */
    int *countseqs = palloc0(sizeof(int) * ss->count);
    totalcount = 0;
    for (int i = 0; i < ss->count; i++)
    {
      sequences[i] = tinterrel_tcbufferseq_linear_geom_iter(
        TSEQUENCESET_SEQ_N(ss, i), gs, box, tinter, func, &countseqs[i]);
      totalcount += countseqs[i];
    }
    allseqs = tseqarr2_to_tseqarr(sequences, countseqs, ss->count, totalcount);
  }
  else
  {
    allseqs = palloc(sizeof(TSequence *) * ss->count);
    for (int i = 0; i < ss->count; i++)
      allseqs[i] = tinterrel_tcbufferseq_discstep_geom(
        TSEQUENCESET_SEQ_N(ss, i), gs, tinter, func);
    totalcount = ss->count;
  }
  return tsequenceset_make_free(allseqs, totalcount, NORMALIZE);
}

/**
 * @brief Return a temporal Boolean that states whether a temporal circular
 * buffer and a geometry intersect or are disjoint
 * @param[in] temp Temporal circular buffer
 * @param[in] cb Circular buffer
 * @param[in] tinter True when computing tintersects, false for tdisjoint
 * @param[in] restr True if the atValue function is applied to the result
 * @param[in] atvalue Value to be used for the atValue function
 */
Temporal *
tinterrel_tcbuffer_cbuffer(const Temporal *temp, const Cbuffer *cb,
  bool tinter, bool restr, bool atvalue)
{
  VALIDATE_TCBUFFER(temp, NULL); VALIDATE_NOT_NULL(cb, NULL);
  /* Ensure the validity of the arguments */
  if (! ensure_valid_tcbuffer_cbuffer(temp, cb))
    return NULL;

  /* Bounding box test */
  STBox box1, box2;
  tspatial_set_stbox(temp, &box1);
  /* Non-empty geometries have a bounding box */
  cbuffer_set_stbox(cb, &box2);
  if (! overlaps_stbox_stbox(&box1, &box2))
  {
    if (tinter)
      /* Computing intersection */
      return restr && atvalue ? NULL :
        temporal_from_base_temp(BoolGetDatum(false), T_TBOOL, temp);
    else
      /* Computing disjoint */
      return restr && ! atvalue ? NULL :
        temporal_from_base_temp(BoolGetDatum(true), T_TBOOL, temp);
  }

  datum_func2 func = &datum_geom_intersects2d;
  /* Cheat the compiler to avoid warnings before having the implementation */
  assert(func); 
  Temporal *result = NULL;
  assert(temptype_subtype(temp->subtype));
  // switch (temp->subtype)
  // {
    // case TINSTANT:
      // result = (Temporal *) tinterrel_tcbufferinst_cbuffer((TInstant *) temp,
        // cb, tinter, func);
      // break;
    // case TSEQUENCE:
      // result = MEOS_FLAGS_LINEAR_INTERP(temp->flags) ?
        // (Temporal *) tinterrel_tcbufferseq_linear_cbuffer((TSequence *) temp,
          // cb, &box2, tinter, func) :
        // (Temporal *) tinterrel_tcbufferseq_discstep_cbuffer((TSequence *) temp,
          // cb, tinter, func);
      // break;
    // default: /* TSEQUENCESET */
      // result = (Temporal *) tinterrel_tcbufferseqset_cbuffer(
        // (TSequenceSet *) temp, cb, &box2, tinter, func);
  // }

  /* Restrict the result to the Boolean value in the last argument if any */
  if (result && restr)
  {
    Temporal *atresult = temporal_restrict_value(result, atvalue, REST_AT);
    pfree(result);
    result = atresult;
  }
  return result;
}

/**
 * @brief Return a temporal Boolean that states whether a temporal circular
 * buffer and a geometry intersect or are disjoint
 * @param[in] temp Temporal circular buffer
 * @param[in] gs Geometry
 * @param[in] tinter True when computing tintersects, false for tdisjoint
 * @param[in] restr True if the atValue function is applied to the result
 * @param[in] atvalue Value to be used for the atValue function
 */
Temporal *
tinterrel_tcbuffer_geo(const Temporal *temp, const GSERIALIZED *gs,
  bool tinter, bool restr, bool atvalue)
{
  VALIDATE_TCBUFFER(temp, NULL); VALIDATE_NOT_NULL(gs, NULL);
  /* Ensure the validity of the arguments */
  if (! ensure_valid_tcbuffer_geo(temp, gs) || gserialized_is_empty(gs))
    return NULL;

  /* Bounding box test */
  STBox box1, box2;
  tspatial_set_stbox(temp, &box1);
  /* Non-empty geometries have a bounding box */
  geo_set_stbox(gs, &box2);
  if (! overlaps_stbox_stbox(&box1, &box2))
  {
    if (tinter)
      /* Computing intersection */
      return restr && atvalue ? NULL :
        temporal_from_base_temp(BoolGetDatum(false), T_TBOOL, temp);
    else
      /* Computing disjoint */
      return restr && ! atvalue ? NULL :
        temporal_from_base_temp(BoolGetDatum(true), T_TBOOL, temp);
  }

  datum_func2 func = &datum_geom_intersects2d;
  Temporal *result = NULL;
  assert(temptype_subtype(temp->subtype));
  switch (temp->subtype)
  {
    case TINSTANT:
      result = (Temporal *) tinterrel_tcbufferinst_geom((TInstant *) temp, gs,
        tinter, func);
      break;
    case TSEQUENCE:
      result = MEOS_FLAGS_LINEAR_INTERP(temp->flags) ?
        (Temporal *) tinterrel_tcbufferseq_linear_geom((TSequence *) temp,
          gs, &box2, tinter, func) :
        (Temporal *) tinterrel_tcbufferseq_discstep_geom((TSequence *) temp,
          gs, tinter, func);
      break;
    default: /* TSEQUENCESET */
      result = (Temporal *) tinterrel_tcbufferseqset_geom(
        (TSequenceSet *) temp, gs, &box2, tinter, func);
  }
  /* Restrict the result to the Boolean value in the last argument if any */
  if (result && restr)
  {
    Temporal *atresult = temporal_restrict_value(result, atvalue, REST_AT);
    pfree(result);
    result = atresult;
  }
  return result;
}

/*****************************************************************************/

/**
 * @brief Return a temporal Boolean that states whether two temporal circular
 * buffers intersect or are disjoint
 * @param[in] temp1,temp2 Temporal circular buffers
 * @param[in] tinter True when computing tintersects, false for tdisjoint
 * @param[in] restr True if the atValue function is applied to the result
 * @param[in] atvalue Value to be used for the atValue function
 */
Temporal *
tinterrel_tcbuffer_tcbuffer(const Temporal *temp1, const Temporal *temp2,
  bool tinter, bool restr, bool atvalue)
{
  VALIDATE_TCBUFFER(temp1, NULL); VALIDATE_TCBUFFER(temp2, NULL);
  /* Ensure the validity of the arguments */
  if (! ensure_valid_tcbuffer_tcbuffer(temp1, temp2))
    return NULL;

  Temporal *result = tinter ?
      tcomp_temporal_temporal(temp1, temp2, &datum2_eq) :
      tcomp_temporal_temporal(temp1, temp2, &datum2_ne);

  /* Restrict the result to the Boolean value in the last argument if any */
  if (result && restr)
  {
    Temporal *atresult = temporal_restrict_value(result, BoolGetDatum(atvalue),
      REST_AT);
    pfree(result);
    result = atresult;
  }
  return result;
}

/*****************************************************************************
 * Generic ever/always spatiotemporal relationship functions
 *****************************************************************************/

/**
 * @brief Generic spatiotemporal relationship for a temporal circular buffer
 * and a geometry
 * @param[in] temp Temporal circular buffer
 * @param[in] gs Geometry
 * @param[in] param Parameter
 * @param[in] func PostGIS function to be called
 * @param[in] numparam Number of parameters of the function
 * @param[in] invert True if the arguments should be inverted
 * @return On error return `NULL`
 */
static Temporal *
tspatialrel_tcbuffer_geo(const Temporal *temp, const GSERIALIZED *gs,
  Datum param, varfunc func, int numparam, bool invert)
{
  assert(temp); assert(gs); assert(! gserialized_is_empty(gs));
  assert(temp->temptype == T_TCBUFFER);
  /* Fill the lifted structure */
  LiftedFunctionInfo lfinfo;
  memset(&lfinfo, 0, sizeof(LiftedFunctionInfo));
  lfinfo.func = func;
  lfinfo.numparam = numparam;
  lfinfo.param[0] = param;
  lfinfo.argtype[0] = temp->temptype;
  lfinfo.argtype[1] = T_GEOMETRY;
  lfinfo.restype = T_TBOOL;
  lfinfo.reslinear = MEOS_FLAGS_LINEAR_INTERP(temp->flags);
  lfinfo.invert = invert;
  lfinfo.discont = false;
  return tfunc_temporal_base(temp, PointerGetDatum(gs), &lfinfo);
}

/**
 * @brief Generic spatiotemporal relationship for a temporal circular buffer
 * and a circular buffer
 * @param[in] temp Temporal circular buffer
 * @param[in] cb Circular buffer
 * @param[in] param Parameter
 * @param[in] func PostGIS function to be called
 * @param[in] numparam Number of parameters of the function
 * @param[in] invert True if the arguments should be inverted
 * @return On error return `NULL`
 */
static Temporal *
tspatialrel_tcbuffer_cbuffer(const Temporal *temp, const Cbuffer *cb,
  Datum param, varfunc func, int numparam, bool invert)
{
  assert(temp); assert(cb); assert(temp->temptype == T_TCBUFFER);
  /* Fill the lifted structure */
  LiftedFunctionInfo lfinfo;
  memset(&lfinfo, 0, sizeof(LiftedFunctionInfo));
  lfinfo.func = func;
  lfinfo.numparam = numparam;
  lfinfo.param[0] = param;
  lfinfo.argtype[0] = temp->temptype;
  lfinfo.argtype[1] = temptype_basetype(temp->temptype);
  lfinfo.restype = T_TBOOL;
  lfinfo.reslinear = MEOS_FLAGS_LINEAR_INTERP(temp->flags);
  lfinfo.invert = invert;
  lfinfo.discont = false;
  return tfunc_temporal_base(temp, PointerGetDatum(cb), &lfinfo);
}

/*****************************************************************************/

/**
 * @brief Generic spatiotemporal relationship for two temporal circular buffers
 * @param[in] temp1,temp2 Temporal circular buffers
 * @param[in] param Parameter
 * @param[in] func PostGIS function to be called
 * @param[in] numparam Number of parameters of the function
 * @param[in] invert True if the arguments should be inverted
 * @return On error return `NULL`
 */
static Temporal *
tspatialrel_tcbuffer_tcbuffer(const Temporal *temp1, const Temporal *temp2,
  Datum param, varfunc func, int numparam, bool invert)
{
  assert(temp1); assert(temp2); assert(temp1->temptype == T_TCBUFFER);
  assert(temp2->temptype == T_TCBUFFER);
  /* Fill the lifted structure */
  LiftedFunctionInfo lfinfo;
  memset(&lfinfo, 0, sizeof(LiftedFunctionInfo));
  lfinfo.func = func;
  lfinfo.numparam = numparam;
  lfinfo.param[0] = param;
  lfinfo.argtype[0] = lfinfo.argtype[1] = temp1->temptype;
  lfinfo.restype = T_TBOOL;
  lfinfo.reslinear = MEOS_FLAGS_LINEAR_INTERP(temp1->flags) ||
    MEOS_FLAGS_LINEAR_INTERP(temp2->flags);
  lfinfo.invert = invert;
  lfinfo.discont = false;
  return tfunc_temporal_temporal(temp1, temp2, &lfinfo);
}

/*****************************************************************************
 * Temporal contains
 *****************************************************************************/

/**
 * @ingroup meos_cbuffer_rel_temp
 * @brief Return a temporal Boolean that states whether a geometry contains a
 * temporal circular buffer
 * @param[in] gs Geometry
 * @param[in] temp Temporal circular buffer
 * @param[in] restr True when the result is restricted to a value
 * @param[in] atvalue Value to restrict
 * @csqlfn #Tcontains_geo_tcbuffer()
 */
Temporal *
tcontains_geo_tcbuffer(const GSERIALIZED *gs, const Temporal *temp, bool restr,
  bool atvalue)
{
  VALIDATE_TCBUFFER(temp, NULL); VALIDATE_NOT_NULL(gs, NULL);
  /* Ensure the validity of the arguments */
  if (! ensure_valid_tcbuffer_geo(temp, gs) || gserialized_is_empty(gs) ||
      ! ensure_not_geodetic_geo(gs) || ! ensure_has_not_Z_geo(gs))
    return NULL;

  Temporal *result = tspatialrel_tcbuffer_geo(temp, gs, (Datum) NULL,
    (varfunc) &datum_geom_contains, 0, INVERT);

  /* Restrict the result to the Boolean value in the last argument if any */
  if (result && restr)
  {
    Temporal *atresult = temporal_restrict_value(result, atvalue, REST_AT);
    pfree(result);
    result = atresult;
  }
  return result;
}

/**
 * @ingroup meos_cbuffer_rel_temp
 * @brief Return a temporal Boolean that states whether a temporal circular
 * buffer contains a geometry
 * @param[in] temp Temporal circular buffer
 * @param[in] gs Geometry
 * @param[in] restr True when the result is restricted to a value
 * @param[in] atvalue Value to restrict
 * @csqlfn #Tcontains_geo_tcbuffer()
 */
Temporal *
tcontains_tcbuffer_geo(const Temporal *temp, const GSERIALIZED *gs, bool restr,
  bool atvalue)
{
  VALIDATE_TCBUFFER(temp, NULL); VALIDATE_NOT_NULL(gs, NULL);
  /* Ensure the validity of the arguments */
  if (! ensure_valid_tcbuffer_geo(temp, gs) || gserialized_is_empty(gs) ||
      ! ensure_not_geodetic_geo(gs) || ! ensure_has_not_Z_geo(gs))
    return NULL;

  Temporal *result = tspatialrel_tcbuffer_geo(temp, gs, (Datum) NULL,
    (varfunc) &datum_geom_contains, 0, INVERT_NO);

  /* Restrict the result to the Boolean value in the last argument if any */
  if (result && restr)
  {
    Temporal *atresult = temporal_restrict_value(result, atvalue, REST_AT);
    pfree(result);
    result = atresult;
  }
  return result;
}

/*****************************************************************************/

/**
 * @ingroup meos_cbuffer_rel_temp
 * @brief Return a temporal Boolean that states whether a circular buffer
 * contains a temporal circular buffer
 * @param[in] cb Circular buffer
 * @param[in] temp Temporal circular buffer
 * @param[in] restr True when the result is restricted to a value
 * @param[in] atvalue Value to restrict
 * @csqlfn #Tcontains_cbuffer_tcbuffer()
 */
Temporal *
tcontains_cbuffer_tcbuffer(const Cbuffer *cb, const Temporal *temp, bool restr,
  bool atvalue)
{
  VALIDATE_TCBUFFER(temp, NULL); VALIDATE_NOT_NULL(cb, NULL);
  /* Ensure the validity of the arguments */
  if (! ensure_valid_tcbuffer_cbuffer(temp, cb))
    return NULL;

  Temporal *result = tspatialrel_tcbuffer_cbuffer(temp, cb, (Datum) NULL,
    (varfunc) &datum_cbuffer_contains, 0, INVERT);

  /* Restrict the result to the Boolean value in the last argument if any */
  if (result && restr)
  {
    Temporal *atresult = temporal_restrict_value(result, atvalue, REST_AT);
    pfree(result);
    result = atresult;
  }
  return result;
}

/**
 * @ingroup meos_cbuffer_rel_temp
 * @brief Return a temporal Boolean that states whether a temporal circular
 * buffer contains a geometry
 * @param[in] temp Temporal circular buffer
 * @param[in] cb Circular buffer
 * @param[in] restr True when the result is restricted to a value
 * @param[in] atvalue Value to restrict
 * @csqlfn #Tcontains_cbuffer_tcbuffer()
 */
Temporal *
tcontains_tcbuffer_cbuffer(const Temporal *temp, const Cbuffer *cb, bool restr,
  bool atvalue)
{
  VALIDATE_TCBUFFER(temp, NULL); VALIDATE_NOT_NULL(cb, NULL);
  /* Ensure the validity of the arguments */
  if (! ensure_valid_tcbuffer_cbuffer(temp, cb))
    return NULL;

  Temporal *result = tspatialrel_tcbuffer_cbuffer(temp, cb, (Datum) NULL,
    (varfunc) &datum_cbuffer_contains, 0, INVERT_NO);

  /* Restrict the result to the Boolean value in the last argument if any */
  if (result && restr)
  {
    Temporal *atresult = temporal_restrict_value(result, atvalue, REST_AT);
    pfree(result);
    result = atresult;
  }
  return result;
}

/**
 * @ingroup meos_cbuffer_rel_temp
 * @brief Return a temporal Boolean that states whether a temporal circular
 * buffer contains another one
 * @param[in] temp1,temp2 Temporal circular buffermetries
 * @param[in] restr True when the result is restricted to a value
 * @param[in] atvalue Value to restrict
 * @csqlfn #Tcontains_geo_tcbuffer()
 */
Temporal *
tcontains_tcbuffer_tcbuffer(const Temporal *temp1, const Temporal *temp2,
  bool restr, bool atvalue)
{
  VALIDATE_TCBUFFER(temp1, NULL); VALIDATE_TCBUFFER(temp2, NULL);
  /* Ensure the validity of the arguments */
  if (! ensure_valid_tcbuffer_tcbuffer(temp1, temp2))
    return NULL;

  Temporal *result = tspatialrel_tcbuffer_tcbuffer(temp1, temp2, (Datum) NULL,
    (varfunc) &datum_geom_contains, 0, INVERT_NO);

  /* Restrict the result to the Boolean value in the last argument if any */
  if (result && restr)
  {
    Temporal *atresult = temporal_restrict_value(result, atvalue, REST_AT);
    pfree(result);
    result = atresult;
  }
  return result;
}

/*****************************************************************************
 * Temporal covers
 *****************************************************************************/

/**
 * @ingroup meos_cbuffer_rel_temp
 * @brief Return a temporal Boolean that states whether a geometry covers a
 * temporal circular buffer
 * @param[in] gs Geometry
 * @param[in] temp Temporal circular buffer
 * @param[in] restr True when the result is restricted to a value
 * @param[in] atvalue Value to restrict
 * @csqlfn #Tcovers_geo_tcbuffer()
 */
Temporal *
tcovers_geo_tcbuffer(const GSERIALIZED *gs, const Temporal *temp, bool restr,
  bool atvalue)
{
  VALIDATE_TCBUFFER(temp, NULL); VALIDATE_NOT_NULL(gs, NULL);
  /* Ensure the validity of the arguments */
  if (! ensure_valid_tcbuffer_geo(temp, gs) || gserialized_is_empty(gs) ||
      ! ensure_not_geodetic_geo(gs) || ! ensure_has_not_Z_geo(gs))
    return NULL;

  Temporal *result;
  /* Temporal point case */
  if (tpoint_type(temp->temptype))
  {
    Temporal *inter = tinterrel_tcbuffer_geo(temp, gs, TINTERSECTS, restr,
      atvalue);
    GSERIALIZED *gsbound = geom_boundary(gs);
    if (! gserialized_is_empty(gsbound))
    {
      Temporal *inter_bound = tinterrel_tcbuffer_geo(temp, gsbound,
        TINTERSECTS, restr, atvalue);
      Temporal *not_inter_bound = tnot_tbool(inter_bound);
      result = boolop_tbool_tbool(inter, not_inter_bound, &datum_and);
      pfree(inter); pfree(gsbound); pfree(inter_bound); pfree(not_inter_bound);
    }
    else
      result = inter;
  }
  else
  /* Temporal circular buffermetry case */
  {
    result = tspatialrel_tcbuffer_geo(temp, gs, (Datum) NULL,
      (varfunc) &datum_geom_covers, 0, INVERT);
  }

  /* Restrict the result to the Boolean value in the last argument if any */
  if (result && restr)
  {
    Temporal *atresult = temporal_restrict_value(result, atvalue, REST_AT);
    pfree(result);
    result = atresult;
  }
  return result;
}

/**
 * @ingroup meos_cbuffer_rel_temp
 * @brief Return a temporal Boolean that states whether a temporal circular
 * buffer covers a geometry
 * @param[in] temp Temporal circular buffer
 * @param[in] gs Geometry
 * @param[in] restr True when the result is restricted to a value
 * @param[in] atvalue Value to restrict
 * @csqlfn #Tcovers_geo_tcbuffer()
 */
Temporal *
tcovers_tcbuffer_geo(const Temporal *temp, const GSERIALIZED *gs, bool restr,
  bool atvalue)
{
  VALIDATE_TCBUFFER(temp, NULL); VALIDATE_NOT_NULL(gs, NULL);
  /* Ensure the validity of the arguments */
  if (! ensure_valid_tcbuffer_geo(temp, gs) || gserialized_is_empty(gs) ||
      ! ensure_not_geodetic_geo(gs) || ! ensure_has_not_Z_geo(gs))
    return NULL;

  Temporal *result = tspatialrel_tcbuffer_geo(temp, gs, (Datum) NULL,
    (varfunc) &datum_geom_covers, 0, INVERT_NO);

  /* Restrict the result to the Boolean value in the last argument if any */
  if (result && restr)
  {
    Temporal *atresult = temporal_restrict_value(result, atvalue, REST_AT);
    pfree(result);
    result = atresult;
  }
  return result;
}

/*****************************************************************************/

/**
 * @ingroup meos_cbuffer_rel_temp
 * @brief Return a temporal Boolean that states whether a circular buffer
 * covers a temporal circular buffer
 * @param[in] cb Circular buffer
 * @param[in] temp Temporal circular buffer
 * @param[in] restr True when the result is restricted to a value
 * @param[in] atvalue Value to restrict
 * @csqlfn #Tcovers_cbuffer_tcbuffer()
 */
Temporal *
tcovers_cbuffer_tcbuffer(const Cbuffer *cb, const Temporal *temp, bool restr,
  bool atvalue)
{
  VALIDATE_TCBUFFER(temp, NULL); VALIDATE_NOT_NULL(cb, NULL);
  /* Ensure the validity of the arguments */
  if (! ensure_valid_tcbuffer_cbuffer(temp, cb))
    return NULL;

  Temporal *result = tspatialrel_tcbuffer_cbuffer(temp, cb, (Datum) NULL,
    (varfunc) &datum_cbuffer_covers, 0, INVERT);

  /* Restrict the result to the Boolean value in the last argument if any */
  if (result && restr)
  {
    Temporal *atresult = temporal_restrict_value(result, atvalue, REST_AT);
    pfree(result);
    result = atresult;
  }
  return result;
}

/**
 * @ingroup meos_cbuffer_rel_temp
 * @brief Return a temporal Boolean that states whether a temporal circular
 * buffer covers a geometry
 * @param[in] temp Temporal circular buffer
 * @param[in] cb Circular buffer
 * @param[in] restr True when the result is restricted to a value
 * @param[in] atvalue Value to restrict
 * @csqlfn #Tcovers_cbuffer_tcbuffer()
 */
Temporal *
tcovers_tcbuffer_cbuffer(const Temporal *temp, const Cbuffer *cb, bool restr,
  bool atvalue)
{
  VALIDATE_TCBUFFER(temp, NULL); VALIDATE_NOT_NULL(cb, NULL);
  /* Ensure the validity of the arguments */
  if (! ensure_valid_tcbuffer_cbuffer(temp, cb))
    return NULL;

  Temporal *result = tspatialrel_tcbuffer_cbuffer(temp, cb, (Datum) NULL,
    (varfunc) &datum_cbuffer_covers, 0, INVERT_NO);

  /* Restrict the result to the Boolean value in the last argument if any */
  if (result && restr)
  {
    Temporal *atresult = temporal_restrict_value(result, atvalue, REST_AT);
    pfree(result);
    result = atresult;
  }
  return result;
}

/**
 * @ingroup meos_cbuffer_rel_temp
 * @brief Return a temporal Boolean that states whether a temporal circular
 * buffer covers another one
 * @param[in] temp1,temp2 Temporal circular buffermetries
 * @param[in] restr True when the result is restricted to a value
 * @param[in] atvalue Value to restrict
 * @csqlfn #Tcovers_tcbuffer_tcbuffer()
 */
Temporal *
tcovers_tcbuffer_tcbuffer(const Temporal *temp1, const Temporal *temp2,
  bool restr, bool atvalue)
{
  VALIDATE_TCBUFFER(temp1, NULL); VALIDATE_TCBUFFER(temp2, NULL);
  /* Ensure the validity of the arguments */
  if (! ensure_valid_tcbuffer_tcbuffer(temp1, temp2))
    return NULL;

  Temporal *result = tspatialrel_tcbuffer_tcbuffer(temp1, temp2, (Datum) NULL,
    (varfunc) &datum_geom_covers, 0, INVERT_NO);

  /* Restrict the result to the Boolean value in the last argument if any */
  if (result && restr)
  {
    Temporal *atresult = temporal_restrict_value(result, atvalue, REST_AT);
    pfree(result);
    result = atresult;
  }
  return result;
}

/*****************************************************************************
 * Temporal disjoint
 *****************************************************************************/

/**
 * @ingroup meos_cbuffer_rel_temp
 * @brief Return a temporal Boolean that states whether a temporal circular
 * buffer and a geometry are disjoint
 * @param[in] temp Temporal circular buffer
 * @param[in] gs Geometry
 * @param[in] restr True when the result is restricted to a value
 * @param[in] atvalue Value to restrict
 * @csqlfn #Tdisjoint_tcbuffer_geo()
 */
inline Temporal *
tdisjoint_tcbuffer_geo(const Temporal *temp, const GSERIALIZED *gs,
  bool restr, bool atvalue)
{
  return tinterrel_tcbuffer_geo(temp, gs, TDISJOINT, restr, atvalue);
}

/**
 * @ingroup meos_cbuffer_rel_temp
 * @brief Return a temporal Boolean that states whether two temporal circular
 * buffers are disjoint
 * @param[in] temp1,temp2 Temporal circular buffers
 * @param[in] restr True when the result is restricted to a value
 * @param[in] atvalue Value to restrict
 * @csqlfn #Tdisjoint_tcbuffer_tcbuffer()
 */
inline Temporal *
tdisjoint_tcbuffer_tcbuffer(const Temporal *temp1, const Temporal *temp2,
  bool restr, bool atvalue)
{
  return tinterrel_tcbuffer_tcbuffer(temp1, temp2, TDISJOINT, restr, atvalue);
}

/*****************************************************************************
 * Temporal intersects
 *****************************************************************************/

/**
 * @ingroup meos_cbuffer_rel_temp
 * @brief Return a temporal Boolean that states whether a temporal circular
 * buffer and a geometry intersect
 * @param[in] temp Temporal circular buffer
 * @param[in] gs Geometry
 * @param[in] restr True when the result is restricted to a value
 * @param[in] atvalue Value to restrict
 * @csqlfn #Tintersects_tcbuffer_geo()
 */
inline Temporal *
tintersects_tcbuffer_geo(const Temporal *temp, const GSERIALIZED *gs,
  bool restr, bool atvalue)
{
  return tinterrel_tcbuffer_geo(temp, gs, TINTERSECTS, restr, atvalue);
}

/**
 * @ingroup meos_cbuffer_rel_temp
 * @brief Return a temporal Boolean that states whether two temporal circular
 * buffers intersect
 * @param[in] temp1,temp2 Temporal circular buffers
 * @param[in] restr True when the result is restricted to a value
 * @param[in] atvalue Value to restrict
 * @csqlfn #Tintersects_tcbuffer_tcbuffer()
 */
inline Temporal *
tintersects_tcbuffer_tcbuffer(const Temporal *temp1, const Temporal *temp2,
  bool restr, bool atvalue)
{
  return tinterrel_tcbuffer_tcbuffer(temp1, temp2, TINTERSECTS, restr, atvalue);
}

/*****************************************************************************
 * Temporal touches
 *****************************************************************************/

/**
 * @ingroup meos_cbuffer_rel_temp
 * @brief Return a temporal Boolean that states whether a temporal circular
 * buffer touches a geometry
 * @param[in] temp Temporal circular buffer
 * @param[in] gs Geometry
 * @param[in] restr True when the result is restricted to a value
 * @param[in] atvalue Value to restrict
 * @csqlfn #Ttouches_tcbuffer_geo()
 */
Temporal *
ttouches_tcbuffer_geo(const Temporal *temp, const GSERIALIZED *gs, bool restr,
  bool atvalue)
{
  VALIDATE_TCBUFFER(temp, NULL); VALIDATE_NOT_NULL(gs, NULL);
  /* Ensure the validity of the arguments */
  if (! ensure_valid_tcbuffer_geo(temp, gs) || gserialized_is_empty(gs) ||
      ! ensure_not_geodetic_geo(gs) || ! ensure_has_not_Z_geo(gs))
    return NULL;

  Temporal *result;
  /* Temporal point case */
  if (tpoint_type(temp->temptype))
  {
    GSERIALIZED *gsbound = geom_boundary(gs);
    if (! gserialized_is_empty(gsbound))
    {
      result = tinterrel_tcbuffer_geo(temp, gsbound, TINTERSECTS, restr,
        atvalue);
      pfree(gsbound);
    }
    else
      result = temporal_from_base_temp(BoolGetDatum(false), T_TBOOL, temp);
  }
  else
  /* Temporal circular buffermetry case */
  {
    result = tspatialrel_tcbuffer_geo(temp, gs, (Datum) NULL,
      (varfunc) &datum_geom_touches, 0, INVERT_NO);
  }

  /* Restrict the result to the Boolean value in the last argument if any */
  if (result && restr)
  {
    Temporal *atresult = temporal_restrict_value(result, atvalue, REST_AT);
    pfree(result);
    result = atresult;
  }
  return result;
}

/*****************************************************************************/

/**
 * @ingroup meos_cbuffer_rel_temp
 * @brief Return a temporal Boolean that states whether a circular buffer
 * touches a temporal circular buffer
 * @param[in] cb Circular buffer
 * @param[in] temp Temporal circular buffer
 * @param[in] restr True when the result is restricted to a value
 * @param[in] atvalue Value to restrict
 * @csqlfn #Ttouches_cbuffer_tcbuffer()
 */
Temporal *
ttouches_cbuffer_tcbuffer(const Cbuffer *cb, const Temporal *temp, bool restr,
  bool atvalue)
{
  VALIDATE_TCBUFFER(temp, NULL); VALIDATE_NOT_NULL(cb, NULL);
  /* Ensure the validity of the arguments */
  if (! ensure_valid_tcbuffer_cbuffer(temp, cb))
    return NULL;

  Temporal *result = tspatialrel_tcbuffer_cbuffer(temp, cb, (Datum) NULL,
    (varfunc) &datum_cbuffer_touches, 0, INVERT);

  /* Restrict the result to the Boolean value in the last argument if any */
  if (result && restr)
  {
    Temporal *atresult = temporal_restrict_value(result, atvalue, REST_AT);
    pfree(result);
    result = atresult;
  }
  return result;
}

/**
 * @ingroup meos_cbuffer_rel_temp
 * @brief Return a temporal Boolean that states whether a temporal circular
 * buffer touches a geometry
 * @param[in] temp Temporal circular buffer
 * @param[in] cb Circular buffer
 * @param[in] restr True when the result is restricted to a value
 * @param[in] atvalue Value to restrict
 * @csqlfn #Ttouches_cbuffer_tcbuffer()
 */
Temporal *
ttouches_tcbuffer_cbuffer(const Temporal *temp, const Cbuffer *cb, bool restr,
  bool atvalue)
{
  VALIDATE_TCBUFFER(temp, NULL); VALIDATE_NOT_NULL(cb, NULL);
  /* Ensure the validity of the arguments */
  if (! ensure_valid_tcbuffer_cbuffer(temp, cb))
    return NULL;

  Temporal *result = tspatialrel_tcbuffer_cbuffer(temp, cb, (Datum) NULL,
    (varfunc) &datum_cbuffer_touches, 0, INVERT_NO);

  /* Restrict the result to the Boolean value in the last argument if any */
  if (result && restr)
  {
    Temporal *atresult = temporal_restrict_value(result, atvalue, REST_AT);
    pfree(result);
    result = atresult;
  }
  return result;
}

/**
 * @ingroup meos_cbuffer_rel_temp
 * @brief Return a temporal Boolean that states whether a temporal circular
 * buffer touches another one
 * @param[in] temp1,temp2 Temporal circular buffer
 * @param[in] restr True when the result is restricted to a value
 * @param[in] atvalue Value to restrict
 * @csqlfn #Ttouches_tcbuffer_geo()
 */
Temporal *
ttouches_tcbuffer_tcbuffer(const Temporal *temp1, const Temporal *temp2,
  bool restr, bool atvalue)
{
  VALIDATE_TCBUFFER(temp1, NULL); VALIDATE_TCBUFFER(temp2, NULL);
  /* Ensure the validity of the arguments */
  if (! ensure_valid_tcbuffer_tcbuffer(temp1, temp2))
    return NULL;

  Temporal *result =  tspatialrel_tcbuffer_tcbuffer(temp1, temp2, (Datum) NULL,
    (varfunc) &datum_geom_touches, 0, INVERT_NO);

  /* Restrict the result to the Boolean value in the last argument if any */
  if (result && restr)
  {
    Temporal *atresult = temporal_restrict_value(result, atvalue, REST_AT);
    pfree(result);
    result = atresult;
  }
  return result;
}

/*****************************************************************************
 * Functions to compute the tdwithin relationship between temporal circular
 * buffer sequences. This requires to determine the instants t1 and t2 at which
 * two temporal sequences have a distance d between each other. This amounts to
 * solve the equation
 *     distance(seg1(t), seg2(t)) = d
 * The function assumes that the two segments are synchronized, that they are
 * not instants, and that they are not both constant.
 *
 * Possible cases
 *
 * Parallel (a == 0) within distance

  SELECT tdwithin(
  tgeompoint '[POINT(0 1)@2000-01-01, POINT(1 2)@2000-01-02]',
  tgeompoint '[POINT(0 0)@2000-01-01, POINT(1 1)@2000-01-02]', 1)
  -- "{[t@2000-01-01, t@2000-01-02]}"

  * Parallel (a == 0) but not within distance

  SELECT tdwithin(
  tgeompoint '[POINT(0 2)@2000-01-01, POINT(1 3)@2000-01-02]',
  tgeompoint '[POINT(0 0)@2000-01-01, POINT(1 1)@2000-01-02]', 1)
  -- "{[f@2000-01-01, f@2000-01-02]}"

 * No solution (root < 0)

  SELECT tdwithin(
  tgeompoint '[POINT(2 3)@2000-01-01, POINT(3 4)@2000-01-03]',
  tgeompoint '[POINT(4 4)@2000-01-01, POINT(6 2)@2000-01-03]', 1)
  -- "{[f@2000-01-01, f@2000-01-03]}"

 * One solution (root == 0)
   - solution within segment

  SELECT tdwithin(
  tgeompoint '[POINT(2 2)@2000-01-01, POINT(1 1)@2000-01-03]',
  tgeompoint '[POINT(3 1)@2000-01-01, POINT(2 2)@2000-01-03]', 1)
  -- "{[f@2000-01-01, t@2000-01-02], (f@2000-01-02, f@2000-01-03]}"

   - solution outside to segment

  SELECT tdwithin(
  tgeompoint '[POINT(3 3)@2000-01-01, POINT(2 2)@2000-01-03]',
  tgeompoint '[POINT(4 0)@2000-01-01, POINT(3 1)@2000-01-03]', 1)
  -- "{[f@2000-01-01, f@2000-01-03]}"

 * Two solutions (root > 0)
 - segments contains solution period

  SELECT tdwithin(
  tgeompoint '[POINT(1 1)@2000-01-01, POINT(5 5)@2000-01-05]',
  tgeompoint '[POINT(1 3)@2000-01-01, POINT(5 3)@2000-01-05]', 1)
  -- "{[f@2000-01-01, t@2000-01-02, t@2000-01-04], (f@2000-01-04, f@2000-01-05]}"

  - solution period contains segment

  SELECT tdwithin(
  tgeompoint '[POINT(2.5 2.5)@2000-01-02 12:00, POINT(3.5 3.5)@2000-01-05 12:00]',
  tgeompoint '[POINT(2.5 3.0)@2000-01-02 12:00, POINT(3.5 3.0)@2000-01-03 12:00]', 1)
  -- "{[t@2000-01-02 12:00:00+00, t@2000-01-03 12:00:00+00]}"

  - solution period overlaps to the left segment

  SELECT tdwithin(
  tgeompoint '[POINT(3 3)@2000-01-03, POINT(5 5)@2000-01-05]',
  tgeompoint '[POINT(3 3)@2000-01-03, POINT(5 3)@2000-01-05]', 1)
  -- "{[t@2000-01-03, f@2000-01-04, f@2000-01-05]}"

  - solution period overlaps to the right segment

  SELECT tdwithin(
  tgeompoint '[POINT(1 1)@2000-01-01, POINT(3 3)@2000-01-03]',
  tgeompoint '[POINT(1 3)@2000-01-01, POINT(3 3)@2000-01-03]', 1)
  -- "{[f@2000-01-01, t@2000-01-02, t@2000-01-03]}"

  - solution period intersects at an instant with the segment

  SELECT tdwithin(
  tgeompoint '[POINT(4 4)@2000-01-04, POINT(5 5)@2000-01-05]',
  tgeompoint '[POINT(4 3)@2000-01-04, POINT(5 3)@2000-01-05]', 1)
  -- "{[t@2000-01-04], (f@2000-01-04, f@2000-01-05]}"

 *****************************************************************************/

/**
 * @brief Construct the result of the tdwithin function of a segment from
 * the solutions of the quadratic equation found previously
 * @return Number of sequences of the result
 */
static int
tdwithin_add_solutions(int solutions, TimestampTz lower, TimestampTz upper,
  bool lower_inc, bool upper_inc, bool upper_inc1, TimestampTz t1,
  TimestampTz t2, TInstant **instants, TSequence **result)
{
  const Datum datum_true = BoolGetDatum(true);
  const Datum datum_false = BoolGetDatum(false);
  int nseqs = 0;
  /* <  F  > */
  if (solutions == 0 ||
  (solutions == 1 && ((t1 == lower && ! lower_inc) ||
    (t1 == upper && ! upper_inc))))
  {
    tinstant_set(instants[0], datum_false, lower);
    tinstant_set(instants[1], datum_false, upper);
    result[nseqs++] = tsequence_make((const TInstant **) instants, 2,
      lower_inc, upper_inc1, STEP, NORMALIZE_NO);
  }
  /*
   *  <  T  >               2 solutions, lower == t1, upper == t2
   *  [T](  F  )            1 solution, lower == t1 (t1 == t2)
   *  [T  T](  F  )         2 solutions, lower == t1, upper != t2
   *  (  F  )[T]            1 solution && upper == t1, (t1 == t2)
   *  (  F  )[T](  F  )     1 solution, lower != t1 (t1 == t2)
   *  (  F  )[T  T]         2 solutions, lower != t1, upper == t2
   *  (  F  )[T  T](  F  )  2 solutions, lower != t1, upper != t2
   */
  else
  {
    int ninsts = 0;
    if (t1 != lower)
      tinstant_set(instants[ninsts++], datum_false, lower);
    tinstant_set(instants[ninsts++], datum_true, t1);
    if (solutions == 2 && t1 != t2)
      tinstant_set(instants[ninsts++], datum_true, t2);
    result[nseqs++] = tsequence_make((const TInstant **) instants, ninsts,
      lower_inc, (t2 != upper) ? true : upper_inc1, STEP, NORMALIZE_NO);
    if (t2 != upper)
    {
      tinstant_set(instants[0], datum_false, t2);
      tinstant_set(instants[1], datum_false, upper);
      result[nseqs++] = tsequence_make((const TInstant **) instants, 2, false,
        upper_inc1, STEP, NORMALIZE_NO);
    }
  }
  return nseqs;
}

/**
 * @brief Return the timestamps at which the segments of two temporal circular
 * buffer sequences are within a distance (iterator function)
 * @param[in] seq1,seq2 Temporal points
 * @param[in] dist Distance
 * @param[out] result Array on which the pointers of the newly constructed
 * sequences are stored
 * @return Number of elements in the resulting array
 * @pre The temporal circular buffers must be synchronized.
 */
static int
tdwithin_tcbufferseq_tcbufferseq_iter(const TSequence *seq1,
  const TSequence *seq2, Datum dist, TSequence **result)
{
  datum_func3 func = &datum_cbuffer_dwithin;
  const TInstant *start1 = TSEQUENCE_INST_N(seq1, 0);
  const TInstant *start2 = TSEQUENCE_INST_N(seq2, 0);
  if (seq1->count == 1)
  {
    TInstant *inst = tinstant_make(func(tinstant_value_p(start1),
      tinstant_value_p(start2), dist), T_TBOOL, start1->t);
    result[0] = tinstant_to_tsequence_free(inst, STEP);
    return 1;
  }

  int nseqs = 0;
  bool linear1 = MEOS_FLAGS_LINEAR_INTERP(seq1->flags);
  bool linear2 = MEOS_FLAGS_LINEAR_INTERP(seq2->flags);
  Datum sv1 = tinstant_value_p(start1);
  Datum sv2 = tinstant_value_p(start2);
  TimestampTz lower = start1->t;
  bool lower_inc = seq1->period.lower_inc;
  const Datum datum_true = BoolGetDatum(true);
  /* We create three temporal instants with arbitrary values that are set in
   * the for loop to avoid creating and freeing the instants each time a
   * segment of the result is computed */
  TInstant *instants[3];
  instants[0] = tinstant_make(datum_true, T_TBOOL, lower);
  instants[1] = tinstant_copy(instants[0]);
  instants[2] = tinstant_copy(instants[0]);
  for (int i = 1; i < seq1->count; i++)
  {
    /* Each iteration of the for loop adds between one and three sequences */
    const TInstant *end1 = TSEQUENCE_INST_N(seq1, i);
    const TInstant *end2 = TSEQUENCE_INST_N(seq2, i);
    Datum ev1 = tinstant_value_p(end1);
    Datum ev2 = tinstant_value_p(end2);
    TimestampTz upper = end1->t;
    bool upper_inc = (i == seq1->count - 1) ? seq1->period.upper_inc : false;

    /* Both segments are constant */
    if (cbuffer_eq(DatumGetCbufferP(sv1), DatumGetCbufferP(ev1)) &&
        cbuffer_eq(DatumGetCbufferP(sv2), DatumGetCbufferP(ev2)))
    {
      Datum value = func(sv1, sv2, dist);
      tinstant_set(instants[0], value, lower);
      tinstant_set(instants[1], value, upper);
      result[nseqs++] = tsequence_make((const TInstant **) instants, 2,
        lower_inc, upper_inc, STEP, NORMALIZE_NO);
    }
    /* General case */
    else
    {
      /* Find the instants t1 and t2 (if any) during which the dwithin
       * function is true */
      TimestampTz t1, t2;
      Datum sev1 = linear1 ? ev1 : sv1;
      Datum sev2 = linear2 ? ev2 : sv2;
      int solutions = tdwithin_tcbuffersegm_tcbuffersegm(sv1, sev1, sv2, sev2,
        lower, upper, DatumGetFloat8(dist), &t1, &t2);
      bool upper_inc1 = linear1 && linear2 && upper_inc;
      nseqs += tdwithin_add_solutions(solutions, lower, upper, lower_inc,
        upper_inc, upper_inc1, t1, t2, instants, &result[nseqs]);
      /* Add extra final point if only one segment is linear */
      if (upper_inc && (! linear1 || ! linear2))
      {
        Datum value = func(ev1, ev2, dist);
        tinstant_set(instants[0], value, upper);
        result[nseqs++] = tinstant_to_tsequence(instants[0], STEP);
      }
    }
    sv1 = ev1;
    sv2 = ev2;
    lower = upper;
    lower_inc = true;
  }
  pfree(instants[0]); pfree(instants[1]); pfree(instants[2]);
  return nseqs;
}

/**
 * @brief Return the temporal dwithin relationship between two temporal
 $ circular buffer sequences
 * @param[in] seq1,seq2 Temporal points
 * @param[in] dist Distance
 * @param[in] func DWithin function (2D or 3D)
 * @pre The temporal circular buffers must be synchronized.
 */
static TSequenceSet *
tdwithin_tcbufferseq_tcbufferseq(const TSequence *seq1, const TSequence *seq2,
  double dist, datum_func3 func __attribute__((unused)))
{
  TSequence **sequences = palloc(sizeof(TSequence *) * seq1->count * 4);
  int count = tdwithin_tcbufferseq_tcbufferseq_iter(seq1, seq2,
    Float8GetDatum(dist), sequences);
  return tsequenceset_make_free(sequences, count, NORMALIZE);
}

/**
 * @brief Return the timestamps at which the segments of two temporal circular
 * buffer sequence sets are within a distance
 * @param[in] ss1,ss2 Temporal points
 * @param[in] dist Distance
 * @param[in] func DWithin function (2D or 3D)
 * @pre The temporal circular buffers must be synchronized.
 */
static TSequenceSet *
tdwithin_tcbufferseqset_tcbufferseqset(const TSequenceSet *ss1,
  const TSequenceSet *ss2, double dist, datum_func3 func)
{
  /* Singleton sequence set */
  if (ss1->count == 1)
    return tdwithin_tcbufferseq_tcbufferseq(TSEQUENCESET_SEQ_N(ss1, 0),
      TSEQUENCESET_SEQ_N(ss2, 0), dist, func);

  TSequence **sequences = palloc(sizeof(TSequence *) * ss1->totalcount * 4);
  int nseqs = 0;
  for (int i = 0; i < ss1->count; i++)
    nseqs += tdwithin_tcbufferseq_tcbufferseq_iter(TSEQUENCESET_SEQ_N(ss1, i),
      TSEQUENCESET_SEQ_N(ss2, i), Float8GetDatum(dist), &sequences[nseqs]);
  assert(nseqs > 0);
  return tsequenceset_make_free(sequences, nseqs, NORMALIZE);
}

/*****************************************************************************/

/**
 * @brief Return the timestamps at which a temporal circular buffer sequence
 * and a point are within a distance (iterator function)
 * @param[in] seq Temporal point
 * @param[in] point Point
 * @param[in] dist Distance
 * @param[in] func DWithin function (2D or 3D)
 * @param[out] result Array on which the pointers of the newly constructed
 * sequences are stored
 * @return Number of elements in the resulting array
 */
static int
tdwithin_tcbufferseq_point_iter(const TSequence *seq, Datum point, Datum dist,
  datum_func3 func, TSequence **result)
{
  const TInstant *start = TSEQUENCE_INST_N(seq, 0);
  Datum startvalue = tinstant_value_p(start);
  if (seq->count == 1)
  {
    TInstant *inst = tinstant_make(func(startvalue, point, dist), T_TBOOL,
      start->t);
    result[0] = tinstant_to_tsequence_free(inst, STEP);
    return 1;
  }

  int nseqs = 0;
  bool linear = MEOS_FLAGS_LINEAR_INTERP(seq->flags);
  TimestampTz lower = start->t;
  bool lower_inc = seq->period.lower_inc;
  const Datum datum_true = BoolGetDatum(true);
  /* We create three temporal instants with arbitrary values that are set in
   * the for loop to avoid creating and freeing the instants each time a
   * segment of the result is computed */
  TInstant *instants[3];
  instants[0] = tinstant_make(datum_true, T_TBOOL, lower);
  instants[1] = tinstant_copy(instants[0]);
  instants[2] = tinstant_copy(instants[0]);
  for (int i = 1; i < seq->count; i++)
  {
    /* Each iteration of the for loop adds between one and three sequences */
    const TInstant *end = TSEQUENCE_INST_N(seq, i);
    Datum endvalue = tinstant_value_p(end);
    TimestampTz upper = end->t;
    bool upper_inc = (i == seq->count - 1) ? seq->period.upper_inc : false;

    /* Segment is constant or has step interpolation */
    if (datum_point_eq(startvalue, endvalue) || ! linear)
    {
      Datum value = func(startvalue, point, dist);
      tinstant_set(instants[0], value, lower);
      if (! linear && upper_inc)
      {
        Datum value1 = func(endvalue, point, dist);
        tinstant_set(instants[1], value1, upper);
      }
      else
        tinstant_set(instants[1], value, upper);
      result[nseqs++] = tsequence_make((const TInstant **) instants, 2,
        lower_inc, upper_inc, STEP, NORMALIZE_NO);
    }
    /* General case */
    else
    {
      /* Find the instants t1 and t2 (if any) during which the dwithin
       * function is true */
      TimestampTz t1, t2;
      int solutions = tdwithin_tcbuffersegm_tcbuffersegm(startvalue, endvalue,
        point, point, lower, upper, DatumGetFloat8(dist), &t1, &t2);
      bool upper_inc1 = linear && upper_inc;
      nseqs += tdwithin_add_solutions(solutions, lower, upper, lower_inc,
        upper_inc, upper_inc1, t1, t2, instants, &result[nseqs]);
    }
    start = end;
    startvalue = endvalue;
    lower = upper;
    lower_inc = true;
  }
  pfree(instants[0]); pfree(instants[1]); pfree(instants[2]);
  return nseqs;
}

/**
 * @brief Return the timestamps at which a temporal circular buffer sequence
 * and a point are within a distance
 * @param[in] seq Temporal point
 * @param[in] point Point
 * @param[in] dist Distance
 * @param[in] func DWithin function (2D or 3D)
 * @pre The temporal circular buffers must be synchronized.
 */
static TSequenceSet *
tdwithin_tcbufferseq_point(const TSequence *seq, Datum point, Datum dist,
  datum_func3 func)
{
  TSequence **sequences = palloc(sizeof(TSequence *) * seq->count * 4);
  int count = tdwithin_tcbufferseq_point_iter(seq, point, dist, func, sequences);
  /* We are sure that nseqs > 0 since the point is non-empty */
  return tsequenceset_make_free(sequences, count, NORMALIZE);
}

/**
 * @brief Return the timestamps at which a temporal circular buffer sequence
 * set and a point are within a distance
 * @param[in] ss Temporal point
 * @param[in] point Point
 * @param[in] dist Distance
 * @param[in] func DWithin function (2D or 3D)
 */
static TSequenceSet *
tdwithin_tcbufferseqset_point(const TSequenceSet *ss, Datum point, Datum dist,
  datum_func3 func)
{
  /* Singleton sequence set */
  if (ss->count == 1)
    return tdwithin_tcbufferseq_point(TSEQUENCESET_SEQ_N(ss, 0), point, dist,
      func);

  TSequence **sequences = palloc(sizeof(TSequence *) * ss->totalcount * 4);
  int nseqs = 0;
  for (int i = 0; i < ss->count; i++)
    nseqs += tdwithin_tcbufferseq_point_iter(TSEQUENCESET_SEQ_N(ss, i), point,
      dist, func, &sequences[nseqs]);
  assert(nseqs > 0);
  return tsequenceset_make_free(sequences, nseqs, NORMALIZE);
}

/*****************************************************************************
 * Temporal dwithin
 *****************************************************************************/

/**
 * @ingroup meos_cbuffer_rel_temp
 * @brief Return a temporal Boolean that states whether a temporal circular
 * buffer and a geometry are within a distance
 * @param[in] temp Temporal circular buffer
 * @param[in] gs Geometry
 * @param[in] dist Distance
 * @param[in] restr True when the result is restricted to a value
 * @param[in] atvalue Value to restrict
 * @csqlfn #Tdwithin_tcbuffer_geo()
 */
Temporal *
tdwithin_tcbuffer_geo(const Temporal *temp, const GSERIALIZED *gs, double dist,
  bool restr, bool atvalue)
{
  VALIDATE_TCBUFFER(temp, NULL); VALIDATE_NOT_NULL(gs, NULL);
  /* Ensure the validity of the arguments */
  if (! ensure_valid_tcbuffer_geo(temp, gs) || gserialized_is_empty(gs) ||
      (tpoint_type(temp->temptype) && ! ensure_point_type(gs)) ||
      ! ensure_not_negative_datum(Float8GetDatum(dist), T_FLOAT8))
    return NULL;

  datum_func3 func =
    /* 3D only if both arguments are 3D */
    MEOS_FLAGS_GET_Z(temp->flags) && FLAGS_GET_Z(gs->gflags) ?
    &datum_geom_dwithin3d : &datum_geom_dwithin2d;
  Temporal *result;
  assert(temptype_subtype(temp->subtype));
  switch (temp->subtype)
  {
    case TINSTANT:
    {
      Datum value = tinstant_value_p((TInstant *) temp);
      result = (Temporal *) tinstant_make(func(value, GserializedPGetDatum(gs),
        Float8GetDatum(dist)), T_TBOOL, ((TInstant *) temp)->t);
      break;
    }
    case TSEQUENCE:
    {
      if (MEOS_FLAGS_LINEAR_INTERP(temp->flags))
        result = (Temporal *) tdwithin_tcbufferseq_point((TSequence *) temp,
            GserializedPGetDatum(gs), Float8GetDatum(dist), func);
      else
      {
        result = tspatialrel_tcbuffer_geo(temp, gs, Float8GetDatum(dist),
          (varfunc) func, 1, INVERT_NO);
      }
      break;
    }
    default: /* TSEQUENCESET */
      if (MEOS_FLAGS_LINEAR_INTERP(temp->flags))
        result = (Temporal *) tdwithin_tcbufferseqset_point(
          (TSequenceSet *) temp, GserializedPGetDatum(gs),
          Float8GetDatum(dist), func);
      else
      {
        result = tspatialrel_tcbuffer_geo(temp, gs, Float8GetDatum(dist),
          (varfunc) func, 1, INVERT_NO);
      }
  }
  /* Restrict the result to the Boolean value in the fourth argument if any */
  if (result && restr)
  {
    Temporal *atresult = temporal_restrict_value(result, atvalue, REST_AT);
    pfree(result);
    result = atresult;
  }
  return result;
}

/*****************************************************************************/

/**
 * @ingroup meos_cbuffer_rel_temp
 * @brief Return a temporal Boolean that states whether a temporal circular
 * buffer and a circular buffer are within a distance
 * @param[in] temp Temporal circular buffer
 * @param[in] cb Circular buffer
 * @param[in] dist Distance
 * @param[in] restr True when the result is restricted to a value
 * @param[in] atvalue Value to restrict
 * @csqlfn #Tdwithin_tcbuffer_cbuffer()
 */
Temporal *
tdwithin_tcbuffer_cbuffer(const Temporal *temp, const Cbuffer *cb, double dist,
  bool restr, bool atvalue)
{
  VALIDATE_TCBUFFER(temp, NULL); VALIDATE_NOT_NULL(cb, NULL);
  /* Ensure the validity of the arguments */
  if (! ensure_valid_tcbuffer_cbuffer(temp, cb) ||
      ! ensure_not_negative_datum(Float8GetDatum(dist), T_FLOAT8))
    return NULL;

  datum_func3 func = &datum_cbuffer_dwithin;
  Temporal *result;
  assert(temptype_subtype(temp->subtype));
  switch (temp->subtype)
  {
    case TINSTANT:
    {
      Datum value = tinstant_value_p((TInstant *) temp);
      result = (Temporal *) tinstant_make(func(value, GserializedPGetDatum(cb),
        Float8GetDatum(dist)), T_TBOOL, ((TInstant *) temp)->t);
      break;
    }
    case TSEQUENCE:
    {
      if (MEOS_FLAGS_LINEAR_INTERP(temp->flags))
        result = (Temporal *) tdwithin_tcbufferseq_point((TSequence *) temp,
            GserializedPGetDatum(cb), Float8GetDatum(dist), func);
      else
      {
        result = tspatialrel_tcbuffer_cbuffer(temp, cb, Float8GetDatum(dist),
          (varfunc) func, 1, INVERT_NO);
      }
      break;
    }
    default: /* TSEQUENCESET */
      if (MEOS_FLAGS_LINEAR_INTERP(temp->flags))
        result = (Temporal *) tdwithin_tcbufferseqset_point(
          (TSequenceSet *) temp, GserializedPGetDatum(cb),
          Float8GetDatum(dist), func);
      else
      {
        result = tspatialrel_tcbuffer_cbuffer(temp, cb, Float8GetDatum(dist),
          (varfunc) func, 1, INVERT_NO);
      }
  }
  /* Restrict the result to the Boolean value in the fourth argument if any */
  if (result && restr)
  {
    Temporal *atresult = temporal_restrict_value(result, atvalue, REST_AT);
    pfree(result);
    result = atresult;
  }
  return result;
}

/**
 * @brief Return a temporal Boolean that states whether two temporal circular
 * buffers are within a distance
 * @pre The temporal circular buffers are synchronized.
 */
Temporal *
tdwithin_tcbuffer_tcbuffer_sync(const Temporal *sync1, const Temporal *sync2,
  double dist, bool restr, bool atvalue)
{
  datum_func3 func = &datum_cbuffer_dwithin;
  Temporal *result;
  assert(temptype_subtype(sync1->subtype));
  switch (sync1->subtype)
  {
    case TINSTANT:
    {
      Datum value1 = tinstant_value_p((TInstant *) sync1);
      Datum value2 = tinstant_value_p((TInstant *) sync2);
      result = (Temporal *) tinstant_make(func(value1, value2,
        Float8GetDatum(dist)), T_TBOOL, ((TInstant *) sync1)->t);
      break;
    }
    case TSEQUENCE:
    {
      interpType interp1 = MEOS_FLAGS_GET_INTERP(sync1->flags);
      interpType interp2 = MEOS_FLAGS_GET_INTERP(sync2->flags);
      if (interp1 == LINEAR || interp2 == LINEAR)
        result = (Temporal *) tdwithin_tcbufferseq_tcbufferseq(
          (TSequence *) sync1, (TSequence *) sync2, dist, func);
      else
      {
        /* Both sequences have either discrete or step interpolation */
        result = tspatialrel_tcbuffer_tcbuffer(sync1, sync2,
          Float8GetDatum(dist), (varfunc) func, 1, INVERT_NO);
      }
      break;
    }
    default: /* TSEQUENCESET */
    {
      interpType interp1 = MEOS_FLAGS_GET_INTERP(sync1->flags);
      interpType interp2 = MEOS_FLAGS_GET_INTERP(sync2->flags);
      if (interp1 == LINEAR || interp2 == LINEAR)
        result = (Temporal *) tdwithin_tcbufferseqset_tcbufferseqset(
          (TSequenceSet *) sync1, (TSequenceSet *) sync2, dist, func);
      else
      {
        /* Both sequence sets have step interpolation */
        result = tspatialrel_tcbuffer_tcbuffer(sync1, sync2,
          Float8GetDatum(dist), (varfunc) func, 1, INVERT_NO);
      }
    }
  }
  /* Restrict the result to the Boolean value in the fourth argument if any */
  if (result && restr)
  {
    Temporal *atresult = temporal_restrict_value(result, atvalue, REST_AT);
    pfree(result);
    result = atresult;
  }
  return result;
}

/**
 * @ingroup meos_cbuffer_rel_temp
 * @brief Return a temporal Boolean that states whether two temporal circular
 * buffers are within a distance
 * @param[in] temp1,temp2 Temporal circular buffers
 * @param[in] dist Distance
 * @param[in] restr True when the result is restricted to a value
 * @param[in] atvalue Value to restrict
 * @csqlfn #Tdwithin_tcbuffer_tcbuffer()
 */
Temporal *
tdwithin_tcbuffer_tcbuffer(const Temporal *temp1, const Temporal *temp2,
  double dist, bool restr, bool atvalue)
{
  VALIDATE_TCBUFFER(temp1, NULL); VALIDATE_TCBUFFER(temp2, NULL);
  /* Ensure the validity of the arguments */
  if (! ensure_valid_tcbuffer_tcbuffer(temp1, temp2) ||
      ! ensure_not_negative_datum(Float8GetDatum(dist), T_FLOAT8))
    return NULL;

  Temporal *sync1, *sync2;
  /* Return false if the temporal circular buffers do not intersect in time
   * The operation is synchronization without adding crossings */
  if (! intersection_temporal_temporal(temp1, temp2, SYNCHRONIZE_NOCROSS,
      &sync1, &sync2))
    return NULL;

  Temporal *result = tdwithin_tcbuffer_tcbuffer_sync(sync1, sync2, dist, restr,
    atvalue);
  pfree(sync1); pfree(sync2);
  return result;
}

/*****************************************************************************/
