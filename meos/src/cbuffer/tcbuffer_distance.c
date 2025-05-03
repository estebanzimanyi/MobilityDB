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
 * @brief Temporal distance for temporal circular buffers
 */

#include "cbuffer/tcbuffer_distance.h"

/* MEOS */
#include <meos.h>
#include <meos_cbuffer.h>
#include <meos_internal.h>
#include <meos_cbuffer.h>
#include "temporal/lifting.h"
#include "geo/postgis_funcs.h"
#include "geo/tgeo_spatialfuncs.h"
#include "cbuffer/cbuffer.h"
#include "cbuffer/tcbuffer_spatialfuncs.h"

/*****************************************************************************
 * Distance
 *****************************************************************************/

/**
 * @ingroup meos_cbuffer_dist
 * @brief Return the distance between a circular buffer and a geometry
 * @return On error return -1.0
 * @csqlfn #Distance_cbuffer_geo()
 */
double
distance_cbuffer_geo(const Cbuffer *cb, const GSERIALIZED *gs)
{
  VALIDATE_NOT_NULL(cb, -1.0); VALIDATE_NOT_NULL(gs, -1.0);
  /* Ensure the validity of the arguments */
  if (! ensure_valid_cbuffer_geo(cb, gs) || gserialized_is_empty(gs))
    return -1.0;

  GSERIALIZED *geo = cbuffer_geom(cb);
  double result = geom_distance2d(geo, gs);
  pfree(geo);
  return result;
}

/**
 * @ingroup meos_cbuffer_dist
 * @brief Return the distance between a circular buffer and a spatiotemporal box
 * @return On error return -1.0
 * @csqlfn #Distance_cbuffer_stbox()
 */
double
distance_cbuffer_stbox(const Cbuffer *cb, const STBox *box)
{
  VALIDATE_NOT_NULL(cb, -1.0); VALIDATE_NOT_NULL(box, -1.0);
  /* Ensure the validity of the arguments */
  if (! ensure_valid_cbuffer_stbox(cb, box))
    return -1.0;

  GSERIALIZED *geo1 = cbuffer_geom(cb);
  GSERIALIZED *geo2 = stbox_geo(box);
  double result = geom_distance2d(geo1, geo2);
  pfree(geo1); pfree(geo2); 
  return result;
}

/**
 * @ingroup meos_cbuffer_dist
 * @brief Return the distance between two circular buffers
 * @return On error return -1.0
 * @csqlfn #Distance_cbuffer_cbuffer()
 */
double
distance_cbuffer_cbuffer(const Cbuffer *cb1, const Cbuffer *cb2)
{
  VALIDATE_NOT_NULL(cb1, -1.0); VALIDATE_NOT_NULL(cb2, -1.0);
  /* Ensure the validity of the arguments */
  if (! ensure_valid_cbuffer_cbuffer(cb1, cb2))
    return -1.0;
  /* The following function assumes that all validity tests have been done */
  return cbuffer_distance(cb1, cb2);
}

/*****************************************************************************
 * Temporal distance
 * **** TO BE IMPLEMENTED ****  
 *****************************************************************************/

/**
 * @brief Return the TWO timestamps at which a temporal circular buffer segment
 * and a geometry point are at the minimum distance
 * @details These are the turning points when computing the temporal distance.
 * @param[in] start,end Instants defining the first segment
 * @param[in] value Circular buffer
 * @param[in] basetype Base type
 * @param[out] value1,value2 Minimum distances at turning points
 * @param[out] t1,t2 Timestamp at turning points
 * @pre The segment is not constant.
 * @note The parameter basetype is not needed for temporal circular buffers
 */
int
tcbuffer_cbuffer_min_dist_at_timestamptz(const TInstant *start,
  const TInstant *end, Datum value, meosType basetype __attribute__((unused)),
  Datum *value1, Datum *value2, TimestampTz *t1, TimestampTz *t2)
{
  /* Extract the two CBUFFER values */
  Cbuffer *ca1 = DatumGetCbufferP(tinstant_value(start));
  const GSERIALIZED *gs1 = cbuffer_point(ca1);
  const POINT2D *p1 = GSERIALIZED_POINT2D_P(gs1);
  Cbuffer *ca2 = DatumGetCbufferP(tinstant_value(end));
  const GSERIALIZED *gs2 = cbuffer_point(ca2);
  const POINT2D *p2 = GSERIALIZED_POINT2D_P(gs2);

  /* Extract the circular buffer value */
  Cbuffer *cb = DatumGetCbufferP(value);
  const GSERIALIZED *gs = cbuffer_point(cb);
  const POINT2D *p = GSERIALIZED_POINT2D_P(gs);

  /* Extract coordinates and radius at the two instants */
  double xa1 = p1->x;
  double ya1 = p1->y;
  double ra1 = ca1->radius;
  double xa2 = p2->x;
  double ya2 = p2->y;
  double ra2 = ca2->radius;

  /* Extract timestamps */
  TimestampTz ta1 = start->t;
  TimestampTz ta2 = end->t;

  /* Extract static cbuffer coordinates */
  double xb = p->x;
  double yb = p->y;
  double rb = cb->radius;

  /* Compute total duration in seconds */
  double total_duration = (double) (ta2 - ta1) / USECS_PER_SEC;

  /* Initial relative position and radius at ta1 */
  double dx0 = xa1 - xb;
  double dy0 = ya1 - yb;
  double dr0 = ra1 + rb;

  /* Compute relative velocities */
  double vx = (xa2 - xa1) / total_duration;
  double vy = (ya2 - ya1) / total_duration;
  double vr = (ra2 - ra1) / total_duration;

  /* Coefficients of the derivative of (distance - radius)^2 */
  double a = vx * vx + vy * vy - vr * vr;
  double b = dx0 * vx + dy0 * vy - dr0 * vr;

  /* Compute relative time (in seconds) where derivative is zero */
  double t_rel;
  if (a == 0.0 || b == 0.0)
    t_rel = 0.0;
  else
    t_rel = -b / a;

  /* Clamp t_rel within [0, total_duration] */
  if (t_rel < 0.0)
    t_rel = 0.0;
  else if (t_rel > total_duration)
    t_rel = total_duration;

  /* Compute the timestamp at the turning point */
  TimestampTz t_turn = ta1 + (TimestampTz) (t_rel * USECS_PER_SEC);

  /* Check if the turning point is truly internal */
  if (t_turn == ta1 || t_turn == ta2)
  {
    /* No true internal turning point */
    *t1 = (TimestampTz) 0;
    *t2 = (TimestampTz) 0;
    *value1 = (Datum) 0;
    *value2 = (Datum) 0;
    return 0;
  }

  /* Interpolate position and radius at the turning point */
  double x_turn = xa1 + vx * t_rel;
  double y_turn = ya1 + vy * t_rel;
  double r_turn = ra1 + vr * t_rel;

  /* Compute the distance to the static point minus the radius */
  double dx = x_turn - xb;
  double dy = y_turn - yb;
  double dist = sqrt(dx * dx + dy * dy) - r_turn - rb;

  /* Interpolate the distances at start and end */
  double dx_start = xa1 - xb;
  double dy_start = ya1 - yb;
  double dist_start = sqrt(dx_start * dx_start + dy_start * dy_start) - ra1 - rb;

  double dx_end = xa2 - xb;
  double dy_end = ya2 - yb;
  double dist_end = sqrt(dx_end * dx_end + dy_end * dy_end) - ra2 - rb;

  if (dist > 0.0)
  {
    /* Single turning point: return t1 and value1 */
    *t1 = t_turn;
    *value1 = Float8GetDatum(dist);
    *t2 = (TimestampTz) 0;
    *value2 = (Datum) 0;
    return 1;
  }
  else
  {
    /* Crossing zero: compute entrance and exit times */
    double alpha_in = (0.0 - dist_start) / (dist - dist_start);
    double alpha_out = (0.0 - dist) / (dist_end - dist);

    double t_in_secs = 0.0 + t_rel * alpha_in;
    double t_out_secs = t_rel + (total_duration - t_rel) * alpha_out;

    TimestampTz t_in = ta1 + (TimestampTz) (t_in_secs * USECS_PER_SEC);
    TimestampTz t_out = ta1 + (TimestampTz) (t_out_secs * USECS_PER_SEC);

    /* Clamp inside [ta1, ta2] */
    if (t_in < ta1) t_in = ta1;
    if (t_in > ta2) t_in = ta2;
    if (t_out < ta1) t_out = ta1;
    if (t_out > ta2) t_out = ta2;

    *t1 = t_in;
    *t2 = t_out;
    *value1 = Float8GetDatum(0.0);
    *value2 = Float8GetDatum(0.0);
    return 2;
  }
}

/**
 * @ingroup meos_cbuffer_dist
 * @brief Return the temporal distance between a temporal circular buffer and
 * a circular buffer
 * @csqlfn #Distance_tcbuffer_cbuffer()
 */
Temporal *
distance_tcbuffer_cbuffer(const Temporal *temp, const Cbuffer *cb)
{
  /* Ensure the validity of the arguments */
  if (! ensure_valid_tcbuffer_cbuffer(temp, cb))
    return NULL;

  LiftedFunctionInfo lfinfo;
  memset(&lfinfo, 0, sizeof(LiftedFunctionInfo));
  lfinfo.func = (varfunc) datum_cbuffer_distance;
  lfinfo.numparam = 0;
  lfinfo.argtype[0] = temp->temptype;
  lfinfo.argtype[1] = temptype_basetype(temp->temptype);
  lfinfo.restype = T_TFLOAT;
  lfinfo.reslinear = MEOS_FLAGS_LINEAR_INTERP(temp->flags);
  lfinfo.invert = INVERT_NO;
  lfinfo.discont = CONTINUOUS;
  lfinfo.tpfunc_base = lfinfo.reslinear ?
    &tcbuffer_cbuffer_min_dist_at_timestamptz : NULL;
  lfinfo.tpfunc = NULL;
  return tfunc_temporal_base(temp, PointerGetDatum(cb), &lfinfo);
}

/**
 * @ingroup meos_cbuffer_dist
 * @brief Return the temporal distance between a temporal circular buffer and
 * a geometry
 * @csqlfn #Distance_tcbuffer_geo()
 */
Temporal *
distance_tcbuffer_geo(const Temporal *temp, const GSERIALIZED *gs)
{
  /* Ensure the validity of the arguments */
  if (! ensure_valid_tcbuffer_geo(temp, gs) || gserialized_is_empty(gs))
    return NULL;

  Cbuffer *cb = geom_cbuffer(gs);
  Temporal *result = distance_tcbuffer_cbuffer(temp, cb);
  pfree(cb);
  return result;
}


/**
 * @brief Return the TWO timestamps at which two temporal circular buffers 
 * segment are at the minimum distance
 * @details These are the turning points when computing the temporal distance.
 * @param[in] start1,end1 Instants defining the first segment
 * @param[in] start2,end2 Instants defining the second segment
 * @param[in] basetype Base type
 * @param[out] value1,value2 Minimum distances at turning points
 * @param[out] t1,t2 Timestamp at turning points
 * @pre The segment is not constant.
 * @note The parameter basetype is not needed for temporal circular buffers
 */
int
tcbuffer_tcbuffer_min_dist_at_timestamptz(
  const TInstant *start1,const TInstant *end1, 
  const TInstant *start2, const TInstant *end2,
  Datum *value1, TimestampTz *t1, TimestampTz *t2)
{
  /* Extract the two CBUFFER values for the first TCBUFFER */
  Cbuffer *ca1 = DatumGetCbufferP(tinstant_value(start1));
  const GSERIALIZED *gsa1 = cbuffer_point(ca1);
  const POINT2D *pa1 = GSERIALIZED_POINT2D_P(gsa1);
  Cbuffer *ca2 = DatumGetCbufferP(tinstant_value(end1));
  const GSERIALIZED *gsa2 = cbuffer_point(ca2);
  const POINT2D *pa2 = GSERIALIZED_POINT2D_P(gsa2);

  /* Extract the two CBUFFER values for the second TCBUFFER */
  Cbuffer *cb1 = DatumGetCbufferP(tinstant_value(start2));
  const GSERIALIZED *gs1b = cbuffer_point(cb1);
  const POINT2D *pb1 = GSERIALIZED_POINT2D_P(gs1b);
  Cbuffer *cb2 = DatumGetCbufferP(tinstant_value(end2));
  const GSERIALIZED *gsb2 = cbuffer_point(cb2);
  const POINT2D *pb2 = GSERIALIZED_POINT2D_P(gsb2);

  /* Extract coordinates and radius at the two instants */
  double xa1 = pa1->x;
  double ya1 = pa1->y;
  double ra1 = ca1->radius;
  double xa2 = pa2->x;
  double ya2 = pa2->y;
  double ra2 = ca2->radius;

  double xb1 = pb1->x;
  double yb1 = pb1->y;
  double rb1 = cb1->radius;
  double xb2 = pb2->x;
  double yb2 = pb2->y;
  double rb2 = cb2->radius;

  /* Extract timestamps */
  TimestampTz ta1 = start1->t;
  TimestampTz ta2 = end1->t;

  TimestampTz tb1 = start2->t;
  TimestampTz tb2 = end2->t;

  TimestampTz t_start = ta1 > tb1 ? ta1 : tb1;
  TimestampTz t_end = ta2 < tb2 ? ta2 : tb2;

  double total_duration = (double) (t_end - t_start) / USECS_PER_SEC;

  /* Compute durations of each interval (in seconds) */
  double dt_tcb1 = (double)(ta2 - ta1) / USECS_PER_SEC;
  double dt_tcb2 = (double)(tb2 - tb1) / USECS_PER_SEC;

  /* Compute time offsets from interval start to t_start (in seconds) */
  double offset_tcb1 = (double)(t_start - ta1) / USECS_PER_SEC;
  double offset_tcb2 = (double)(t_start - tb1) / USECS_PER_SEC;

  /* Interpolate positions at t_start */
  double xa_start = xa1 + (xa2 - xa1) * (offset_tcb1/dt_tcb1);
  double ya_start = ya1 + (ya2 - ya1) * (offset_tcb1/dt_tcb1);
  double xb_start = xb1 + (xb2 - xb1) * (offset_tcb2/dt_tcb2);
  double yb_start = yb1 + (yb2 - yb1) * (offset_tcb2/dt_tcb2);

  /* Interpolate radii at t_start */
  double ra_start = ra1 + (ra2 - ra1) * (offset_tcb1/dt_tcb1);
  double rb_start = rb1 + (rb2 - rb1) * (offset_tcb2/dt_tcb2);

  /* Compute relative position and combined radius at t_start */
  double dx0 = xa_start - xb_start;
  double dy0 = ya_start - yb_start;
  double dr0 = ra_start + rb_start;

  /* Compute absolute velocities of centroids and radii */
  double vx_a = dt_tcb1 > 0.0 ? (xa2 - xa1) / dt_tcb1 : 0.0;
  double vy_a = dt_tcb1 > 0.0 ? (ya2 - ya1) / dt_tcb1 : 0.0;
  double vr_a = dt_tcb1 > 0.0 ? (ra2 - ra1) / dt_tcb1 : 0.0;

  double vx_b = dt_tcb2 > 0.0 ? (xb2 - xb1) / dt_tcb2 : 0.0;
  double vy_b = dt_tcb2 > 0.0 ? (yb2 - yb1) / dt_tcb2 : 0.0;
  double vr_b = dt_tcb2 > 0.0 ? (rb2 - rb1) / dt_tcb2 : 0.0;

  /* Compute relative velocities */
  double vx = vx_a - vx_b;
  double vy = vy_a - vy_b;
  double vr = vr_a + vr_b;


  /* Compute coefficients of the derivative of (distance - combined_radius)^2 */
  double a = vx * vx + vy * vy - vr * vr;
  double b = dx0 * vx + dy0 * vy - dr0 * vr;

  /* Compute relative time (in seconds) where derivative is zero */
  double t_rel;
  if (a == 0.0 || b == 0.0)
    t_rel = 0.0;
  else
    t_rel = -b / a;

  /* Clamp t_rel within [0, total_duration] */
  if (t_rel < 0.0)
    t_rel = 0.0;
  else if (t_rel > total_duration)
    t_rel = total_duration;

  /* Compute the timestamp at the turning point */
  TimestampTz t_turn = ta1 + (TimestampTz) (t_rel * USECS_PER_SEC);
  
  /* Check if the turning point is truly internal */
  if (t_turn == ta1 || t_turn == ta2)
  {
    /* No true internal turning point */
    *t1 = (TimestampTz) 0;
    *t2 = (TimestampTz) 0;
    *value1 = (Datum) 0;
    //*value2 = (Datum) 0;
    return 0;
  }
  
  /* Interpolate position and radius at the turning point */
  double xa_turn = xa_start + vx_a * t_rel;
  double ya_turn = ya_start + vy_a * t_rel;
  double ra_turn = ra_start + vr_a * t_rel;

  double xb_turn = xb_start + vx_b * t_rel;
  double yb_turn = yb_start + vy_b * t_rel;
  double rb_turn = rb_start + vr_b * t_rel;

  /* Compute the distance between centroids minus the combined radius */
  double dx_turn = xa_turn - xb_turn;
  double dy_turn = ya_turn - yb_turn;
  double dist_turn = sqrt(dx_turn * dx_turn + dy_turn * dy_turn) - ra_turn - rb_turn;

  /* Compute distance at t_start */
  double dx_start = xa_start - xb_start;
  double dy_start = ya_start - yb_start;
  double dist_start = sqrt(dx_start * dx_start + dy_start * dy_start) - ra_start - rb_start;

  /* Interpolate positions and radii at t_end */
  double alpha_tcb1_end = dt_tcb1 > 0.0 ? ((double)(t_end - ta1) / USECS_PER_SEC) / dt_tcb1 : 0.0;
  double alpha_tcb2_end = dt_tcb2 > 0.0 ? ((double)(t_end - tb1) / USECS_PER_SEC) / dt_tcb2 : 0.0;

  double xa_end = xa1 + (xa2 - xa1) * alpha_tcb1_end;
  double ya_end = ya1 + (ya2 - ya1) * alpha_tcb1_end;
  double ra_end = ra1 + (ra2 - ra1) * alpha_tcb1_end;

  double xb_end = xb1 + (xb2 - xb1) * alpha_tcb2_end;
  double yb_end = yb1 + (yb2 - yb1) * alpha_tcb2_end;
  double rb_end = rb1 + (rb2 - rb1) * alpha_tcb2_end;

  double dx_end = xa_end - xb_end;
  double dy_end = ya_end - yb_end;
  double dist_end = sqrt(dx_end * dx_end + dy_end * dy_end) - ra_end - rb_end;


  if (dist_turn > 0.0) 
  {
    /* Single turning point */
    *t1 = t_turn;
    *value1 = Float8GetDatum(dist_turn);
    *t2 = (TimestampTz) 0;
    //*value2 = (Datum) 0;
    return 1;
  }
  else 
  {
    /* Crossing: compute entrance and exit times */
    double alpha_in = (0.0 - dist_start) / (dist_turn - dist_start);
    double alpha_out = (0.0 - dist_turn) / (dist_end - dist_turn);
    double t_in_secs = 0.0 + t_rel * alpha_in;
    double t_out_secs = t_rel + (total_duration - t_rel) * alpha_out;

    TimestampTz t_in = t_start + (TimestampTz) (t_in_secs * USECS_PER_SEC);
    TimestampTz t_out = t_start + (TimestampTz) (t_out_secs * USECS_PER_SEC);

    /* Clamp inside [t_start, t_end] */
    if (t_in < t_start) t_in = t_start;
    if (t_in > t_end) t_in = t_end;
    if (t_out < t_start) t_out = t_start;
    if (t_out > t_end) t_out = t_end;

    *t1 = t_in;
    *t2 = t_out;
    *value1 = Float8GetDatum(0.0);
    //*value2 = Float8GetDatum(0.0);
    return 2;
  }
}

/**
 * @ingroup meos_cbuffer_dist
 * @brief Return the temporal distance between two temporal circular buffers
 * @param[in] temp1,temp2 Temporal circular buffers
 * @csqlfn #Distance_tcbuffer_tcbuffer()
 */
Temporal *
distance_tcbuffer_tcbuffer(const Temporal *temp1, const Temporal *temp2)
{
  /* Ensure the validity of the arguments */
  if (! ensure_valid_tcbuffer_tcbuffer(temp1, temp2))
    return NULL;

  LiftedFunctionInfo lfinfo;
  memset(&lfinfo, 0, sizeof(LiftedFunctionInfo));
  lfinfo.func = (varfunc) datum_cbuffer_distance;
  lfinfo.numparam = 0;
  lfinfo.argtype[0] = temp1->temptype;
  lfinfo.argtype[1] = temp2->temptype;
  lfinfo.restype = T_TFLOAT;
  lfinfo.reslinear = MEOS_FLAGS_LINEAR_INTERP(temp1->flags) &&
                     MEOS_FLAGS_LINEAR_INTERP(temp2->flags);
  lfinfo.invert = INVERT_NO;
  lfinfo.discont = CONTINUOUS;
  lfinfo.tpfunc_base = NULL;
  lfinfo.tpfunc = lfinfo.reslinear ?
    &tcbuffer_tcbuffer_min_dist_at_timestamptz : NULL;
  return tfunc_temporal_temporal(temp1, temp2, &lfinfo);
}

/*****************************************************************************
 * Nearest approach instant (NAI)
 *****************************************************************************/

/**
 * @ingroup meos_cbuffer_dist
 * @brief Return the nearest approach instant of the temporal circular buffer
 * and a geometry
 * @param[in] temp Temporal circular buffer
 * @param[in] gs Geometry
 * @csqlfn #NAI_tcbuffer_geo()
 */
TInstant *
nai_tcbuffer_geo(const Temporal *temp, const GSERIALIZED *gs)
{
  /* Ensure the validity of the arguments */
  if (! ensure_valid_tcbuffer_geo(temp, gs) || gserialized_is_empty(gs))
    return NULL;

  Temporal *tpoint = tcbuffer_tgeompoint(temp);
  TInstant *resultgeom = nai_tgeo_geo(tpoint, gs);
  /* We do not call the function tgeompointinst_tcbufferinst to avoid
   * roundoff errors. The closest point may be at an exclusive bound. */
  Datum value;
  temporal_value_at_timestamptz(temp, resultgeom->t, false, &value);
  TInstant *result = tinstant_make_free(value, temp->temptype, resultgeom->t);
  pfree(tpoint); pfree(resultgeom);
  return result;
}

/**
 * @ingroup meos_cbuffer_dist
 * @brief Return the nearest approach instant of the circular buffer and a
 * temporal circular buffer
 * @param[in] temp Temporal circular buffer
 * @param[in] cb Circular buffer
 * @csqlfn #NAI_tcbuffer_cbuffer()
 */
TInstant *
nai_tcbuffer_cbuffer(const Temporal *temp, const Cbuffer *cb)
{
  /* Ensure the validity of the arguments */
  if (! ensure_valid_tcbuffer_cbuffer(temp, cb))
    return NULL;

  GSERIALIZED *geom = cbuffer_geom(cb);
  Temporal *tpoint = tcbuffer_tgeompoint(temp);
  TInstant *resultgeom = nai_tgeo_geo(tpoint, geom);
  /* We do not call the function tgeompointinst_tcbufferinst to avoid
   * roundoff errors. The closest point may be at an exclusive bound. */
  Datum value;
  temporal_value_at_timestamptz(temp, resultgeom->t, false, &value);
  TInstant *result = tinstant_make_free(value, temp->temptype, resultgeom->t);
  pfree(tpoint); pfree(resultgeom); pfree(geom);
  return result;
}

/**
 * @ingroup meos_cbuffer_dist
 * @brief Return the nearest approach instant of two temporal circular buffers
 * @param[in] temp1,temp2 Temporal circular buffers
 * @csqlfn #NAI_tcbuffer_tcbuffer()
 */
TInstant *
nai_tcbuffer_tcbuffer(const Temporal *temp1, const Temporal *temp2)
{
  /* Ensure the validity of the arguments */
  if (! ensure_valid_tcbuffer_tcbuffer(temp1, temp2))
    return NULL;

  Temporal *dist = distance_tcbuffer_tcbuffer(temp1, temp2);
  if (dist == NULL)
    return NULL;

  const TInstant *min = temporal_min_instant((const Temporal *) dist);
  pfree(dist);
  /* The closest point may be at an exclusive bound. */
  Datum value;
  temporal_value_at_timestamptz(temp1, min->t, false, &value);
  return tinstant_make_free(value, temp1->temptype, min->t);
}

/*****************************************************************************
 * Nearest approach distance (NAD)
 *****************************************************************************/

/**
 * @ingroup meos_cbuffer_dist
 * @brief Return the nearest approach distance between a circular buffer
 * and a spatiotemporal box
 * @param[in] cb Circular buffer
 * @param[in] box Spatiotemporal box
 * @csqlfn #NAD_cbuffer_stbox()
 */
double
nad_cbuffer_stbox(const Cbuffer *cb, const STBox *box)
{
  /* Ensure the validity of the arguments */
  if (! ensure_valid_cbuffer_stbox(cb, box))
    return -1.0;

  Datum geocbuf = PointerGetDatum(cbuffer_geom(cb));
  Datum geobox = PointerGetDatum(stbox_geo(box));
  double result = DatumGetFloat8(datum_geom_distance2d(geocbuf, geobox));
  pfree(DatumGetPointer(geocbuf)); pfree(DatumGetPointer(geobox)); 
  return result;
}

/*****************************************************************************/

/**
 * @ingroup meos_cbuffer_dist
 * @brief Return the nearest approach distance of a temporal circular buffer
 * and a geometry
 * @param[in] temp Temporal circular buffer
 * @param[in] gs Geometry
 * @csqlfn #NAD_tcbuffer_geo()
 */
double
nad_tcbuffer_geo(const Temporal *temp, const GSERIALIZED *gs)
{
  /* Ensure the validity of the arguments */
  if (! ensure_valid_tcbuffer_geo(temp, gs) || gserialized_is_empty(gs))
    return -1.0;

  GSERIALIZED *trav = tcbuffer_traversed_area(temp);
  double result = geom_distance2d(trav, gs);
  pfree(trav);
  return result;
}

/**
 * @ingroup meos_cbuffer_dist
 * @brief Return the nearest approach distance of a temporal circular buffer
 * and a spatiotemporal box
 * @param[in] temp Temporal circular buffer
 * @param[in] box Spatiotemporal box
 * @csqlfn #NAD_tcbuffer_geo()
 */
double
nad_tcbuffer_stbox(const Temporal *temp, const STBox *box)
{
  /* Ensure the validity of the arguments */
  if (! ensure_valid_tcbuffer_stbox(temp, box))
    return -1.0;

  GSERIALIZED *trav = tcbuffer_traversed_area(temp);
  GSERIALIZED *geo = stbox_geo(box);
  double result = geom_distance2d(trav, geo);
  pfree(trav);
  return result;
}

/**
 * @ingroup meos_cbuffer_dist
 * @brief Return the nearest approach distance of a temporal circular buffer
 * and a circular buffer
 * @param[in] temp Temporal circular buffer
 * @param[in] cb Circular buffer
 * @csqlfn #NAD_tcbuffer_cbuffer()
 */
double
nad_tcbuffer_cbuffer(const Temporal *temp, const Cbuffer *cb)
{
  /* Ensure the validity of the arguments */
  if (! ensure_valid_tcbuffer_cbuffer(temp, cb))
    return -1.0;

  GSERIALIZED *geom = cbuffer_geom(cb);
  GSERIALIZED *trav = tcbuffer_traversed_area(temp);
  double result = geom_distance2d(trav, geom);
  pfree(trav); pfree(geom);
  return result;
}

/**
 * @ingroup meos_cbuffer_dist
 * @brief Return the nearest approach distance of two temporal circular buffers
 * @param[in] temp1,temp2 Temporal circular buffers
 * @csqlfn #NAD_tcbuffer_tcbuffer()
 */
double
nad_tcbuffer_tcbuffer(const Temporal *temp1, const Temporal *temp2)
{
  /* Ensure the validity of the arguments */
  if (! ensure_valid_tcbuffer_tcbuffer(temp1, temp2))
    return -1.0;

  Temporal *dist = distance_tcbuffer_tcbuffer(temp1, temp2);
  if (dist == NULL)
    return -1.0;
  return DatumGetFloat8(temporal_min_value(dist));
}

/*****************************************************************************
 * ShortestLine
 *****************************************************************************/

/**
 * @ingroup meos_cbuffer_dist
 * @brief Return the line connecting the nearest approach point between a
 * geometry and a temporal circular buffer
 * @param[in] temp Temporal circular buffer
 * @param[in] gs Geometry
 * @csqlfn #Shortestline_tcbuffer_geo()
 */
GSERIALIZED *
shortestline_tcbuffer_geo(const Temporal *temp, const GSERIALIZED *gs)
{
  /* Ensure the validity of the arguments */
  if (! ensure_valid_tcbuffer_geo(temp, gs) || gserialized_is_empty(gs))
    return NULL;

  GSERIALIZED *trav = tcbuffer_traversed_area(temp);
  GSERIALIZED *result = geom_shortestline2d(trav, gs);
  pfree(trav);
  return result;
}

/**
 * @ingroup meos_cbuffer_dist
 * @brief Return the line connecting the nearest approach point between a
 * circular buffer and a temporal circular buffer
 * @param[in] temp Temporal circular buffer
 * @param[in] cb Circular buffer
 * @csqlfn #Shortestline_tcbuffer_cbuffer()
 */
GSERIALIZED *
shortestline_tcbuffer_cbuffer(const Temporal *temp, const Cbuffer *cb)
{
  /* Ensure the validity of the arguments */
  if (! ensure_valid_tcbuffer_cbuffer(temp, cb))
    return NULL;

  GSERIALIZED *geom = cbuffer_geom(cb);
  GSERIALIZED *trav = tcbuffer_traversed_area(temp);
  GSERIALIZED *result = geom_shortestline2d(trav, geom);
  pfree(geom); pfree(trav);
  return result;
}

/**
 * @ingroup meos_cbuffer_dist
 * @brief Return the line connecting the nearest approach point between two
 * temporal circular buffers
 * @param[in] temp1,temp2 Temporal circular buffers
 * @csqlfn #Shortestline_tcbuffer_tcbuffer()
 */
GSERIALIZED *
shortestline_tcbuffer_tcbuffer(const Temporal *temp1, const Temporal *temp2)
{
  /* Ensure the validity of the arguments */
  if (! ensure_valid_tcbuffer_tcbuffer(temp1, temp2))
    return NULL;

  Temporal *tpoint1 = tcbuffer_tgeompoint(temp1);
  Temporal *tpoint2 = tcbuffer_tgeompoint(temp2);
  GSERIALIZED *result = shortestline_tgeo_tgeo(tpoint1, tpoint2);
  pfree(tpoint1); pfree(tpoint2);
  return result;
}

/*****************************************************************************/
