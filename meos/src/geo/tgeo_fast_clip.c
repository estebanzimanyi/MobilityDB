/***********************************************************************
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
 * @brief Fast trajectory clipping against polygon geometries
 * @details Features
 *   - Avoid processing in GEOS
 *   - parity sweep (no midpoint classification)
 *   - supports holes / multipolygons
 *   - Z span filtering
 */

/* C */
#include <math.h>
/* PostgreSQL */
#include "postgres.h"
/* PostGIS */
#include "liblwgeom.h"
/* MEOS */
#include "meos.h"
#include "meos_internal_geo.h"
#include "temporal/temporal.h"
#include "geo/tgeo.h"
#include "geo/tgeo_spatialfuncs.h"

/* Defined in liblwgeom_internal.h */
#define POSTGIS_FP_TOLERANCE 1e-12

/*****************************************************************************
 * Data structures
 *****************************************************************************/

/**
 * @brief Structure keeping a 3D trajectory point
 */
typedef struct
{
  double x, y, z;      /**< Coordinates of the 3D point */
  TimestampTz t;       /**< Timestamp */
} TrajPoint;

/**
 * @brief Structure keeping the edges of a (multi)polygon
 */
typedef struct
{
  double x1, y1;      /**< Coordinates of the start 2D point */
  double x2, y2;      /**< Coordinates of the end 2D point */
  double xmin, xmax;  /**< Minimum and maximum X coordinate */
  double ymin, ymax;  /**< Minimum and maximum Y coordinate */
  int parity;         /**< Parity flag */
} Edge;

/*****************************************************************************
 * Utilities
 *****************************************************************************/

/**
 * @brief Return the interpolated value between the two values with respect to
 * a factor in [0,1]
 */
static inline double
double_interpolate(double a, double b, double u)
{
  return a + u * (b - a);
}

/**
 * @brief Return the trajectory point obtained between two trajectory points
 * with respect to a factor in [0,1]
 */
static TrajPoint
trajpoint_interpolate(TrajPoint a, TrajPoint b, double u)
{
  TrajPoint p;
  p.x = double_interpolate(a.x, b.x, u);
  p.y = double_interpolate(a.y, b.y, u);
  p.z = double_interpolate(a.z, b.z, u);
  p.t = a.t + (TimestampTz) llround((b.t - a.t) * u);
  return p;
}

/**
 * @brief Return the edges of a (multi)polygon in a dynamic array
 */
static void
poly_extract_edges(const LWGEOM *geom, MeosArray *edge_array)
{
  assert(geom->type == POLYGONTYPE || geom->type == MULTIPOLYGONTYPE);
  if (geom->type == POLYGONTYPE)
  {
    const LWPOLY *poly = (LWPOLY *) geom;
    for (uint32_t r = 0; r < poly->nrings; r++)
    {
      POINTARRAY *pa = poly->rings[r];
      int parity = (r == 0) ? 1 : -1;
      for (uint32_t i = 0; i < pa->npoints - 1; i++)
      {
        POINT2D a,b;
        getPoint2d_p(pa, i, &a);
        getPoint2d_p(pa, i + 1, &b);
        Edge e;
        e.x1 = a.x;
        e.y1 = a.y;
        e.x2 = b.x;
        e.y2 = b.y;
        e.xmin = fmin(a.x, b.x);
        e.xmax = fmax(a.x, b.x);
        e.ymin = fmin(a.y, b.y);
        e.ymax = fmax(a.y, b.y);
        e.parity = parity;
        meos_array_add(edge_array, &e);
      }
    }
  }
  else if (geom->type == MULTIPOLYGONTYPE)
  {
    const LWCOLLECTION *col = (LWCOLLECTION *) geom;
    for (uint32_t i = 0; i < col->ngeoms; i++)
      poly_extract_edges(col->geoms[i], edge_array);
  }
  return;
}

/*****************************************************************************
 * Segment clipping
 *****************************************************************************/

/**
 * @brief Structure keeping a clip event 
 */
typedef struct
{
  double t;       /**< Fraction in [0,1] where the clip occurs */
  int parity;     /**< Parity flag */
} ClipEvent;

/**
 * @brief Comparator function for clipping events
 */
int
clip_event_cmp(const void *a, const void *b)
{
  const ClipEvent *ea = a;
  const ClipEvent *eb = b;
  return (ea->t > eb->t) - (ea->t < eb->t);
}

/**
 * @brief Return 1 if the segments represented by the four points intersect,
 * return 0 otherwise
 * @param[in] ax,ay,bx,by,cx,cy,dx,dy Coordinates defining the two segments
 * @param[out] t Fraction in [0,1] determining the intersection point 
 */
static inline int
segm_intersection(double ax, double ay, double bx, double by,
  double cx, double cy, double dx, double dy, double *t_out)
{
  const double eps = POSTGIS_FP_TOLERANCE;
  double rx = bx - ax;
  double ry = by - ay;
  double sx = dx - cx;
  double sy = dy - cy;
  double den = rx * sy - ry * sx;
  double qx = cx - ax;
  double qy = cy - ay;

  /* Parallel or collinear case */
  if (den > -eps && den < eps)
  {
    /* Check collinearity */
    double cross = qx * ry - qy * rx;
    if (cross > eps || cross < -eps)
      return 0; /* parallel, non-intersecting */

    /* Collinear: project onto dominant axis */
    double r2 = rx * rx + ry * ry;
    if (r2 < eps)
      return 0; /* degenerate segment */

    /* Project endpoints of CD onto AB */
    double t0 = (qx * rx + qy * ry) / r2;
    double t1 = ((dx - ax) * rx + (dy - ay) * ry) / r2;

    /* Sort interval */
    if (t0 > t1)
    {
      double tmp = t0; t0 = t1; t1 = tmp;
    }

    /* Check overlap */
    if (t1 < 0.0 || t0 > 1.0)
      return 0;

    /* Return entry point (clamped) */
    double t = (t0 < 0.0) ? 0.0 : t0;
    if (t > 1.0) t = 1.0;

    *t_out = t;
    return 1;
  }

  /* Proper intersection  */
  double inv = 1.0 / den;
  double t = (qx * sy - qy * sx) * inv;
  double u = (qx * ry - qy * rx) * inv;

  /* Range test */
  if (t < -eps || t > 1.0 + eps || u < -eps || u > 1.0 + eps)
    return 0;

  /* Clamp to [0,1] */
  if (t < 0.0) t = 0.0;
  else if (t > 1.0) t = 1.0;

  /* Half-open rule for vertex hits: Reject upper endpoint to avoid double
   * counting */
  if (t > 1.0 - eps)
    return 0;

  *t_out = t;
  return 1;
}

/**
 * @brief Return in the dynamic array passed as last argument the result of 
 * clipping the segment defined by two trajectory points with respect to the
 * edges of a polygon 
 */
static void
clip_segment(TrajPoint a, TrajPoint b, const Edge *edges, int nedges,
  MeosArray *trajpts)
{
  /* Segment bounding box */
  double xmin = fmin(a.x, b.x);
  double xmax = fmax(a.x, b.x);
  double ymin = fmin(a.y, b.y);
  double ymax = fmax(a.y, b.y);

  /* Precompute deltas for interpolation */
  double dx = b.x - a.x;
  double dy = b.y - a.y;
  double dz = b.z - a.z;
  double dt = (double)(b.t - a.t);

  /* Stack buffer used when the number of intersections is small */
  #define STACK_MAX 32
  ClipEvent stack_events[STACK_MAX];
  ClipEvent *events = stack_events;
  int capacity = STACK_MAX;
  int n = 0;

  /* Ensure capacity: use the stack when small number of intersections, 
   * use the heap otherwise */
  #define ENSURE_CAP() \
    do { \
      if (n >= capacity) { \
        capacity *= 2; \
        if (events == stack_events) { \
          events = palloc(sizeof(ClipEvent) * capacity); \
          memcpy(events, stack_events, sizeof(ClipEvent) * n); \
        } else { \
          events = repalloc(events, sizeof(ClipEvent) * capacity); \
        } \
      } \
    } while (0)

  /* Add endpoints (see below) */
  events[n++] = (ClipEvent){.t = 0.0, .parity = 0};
  events[n++] = (ClipEvent){.t = 1.0, .parity = 0};
  /* Variable used for testing with respect to epsilon values */
  double diff;

  /* Loop for all the edges */
  for (int i = 0; i < nedges; i++)
  {
    const Edge e = edges[i];

    /* Skip horizontal edges and perform bounding box test */
    diff = e.y1 - e.y2;
    if (diff > POSTGIS_FP_TOLERANCE || diff < -POSTGIS_FP_TOLERANCE ||
        xmax < e.xmin || xmin > e.xmax || ymax < e.ymin || ymin > e.ymax)
      continue;

    /* Y-span reject with bitwise operations */
    if ((a.y < e.ymin && b.y < e.ymin) | (a.y > e.ymax && b.y > e.ymax))
      continue;

    double t;
    if (! segm_intersection(a.x, a.y, b.x, b.y, e.x1, e.y1, e.x2, e.y2, &t))
      continue;

    /* Clamp without branching */
    t = (t < 0.0) ? 0.0 : t;
    t = (t > 1.0) ? 1.0 : t;

    ENSURE_CAP();
    events[n++] = (ClipEvent){.t = t, .parity = e.parity};
  }

  /* Sort events by t */
  qsort(events, n, sizeof(ClipEvent), clip_event_cmp);

  /* Deduplicate in-place: the following assumes that n >=1 which is ensured
   * since we added the endpoints above */
  assert(n >= 1);
  int w = 1;
  for (int r = 1; r < n; r++)
  {
    diff = events[r].t - events[w - 1].t;
    if (diff > POSTGIS_FP_TOLERANCE || diff < -POSTGIS_FP_TOLERANCE)
      events[w++] = events[r];
  }
  n = w;

  /* Parity sweep */
  int inside = 0;
  for (int i = 0; i < n - 1; i++)
  {
    inside += events[i].parity;
    double t0 = events[i].t;
    double t1 = events[i + 1].t;
    if (inside > 0)
    {
      double diff = t1 - t0;
      if (diff < POSTGIS_FP_TOLERANCE && diff > -POSTGIS_FP_TOLERANCE)
        continue;

      /* Interpolate first point */
      TrajPoint p;
      p.x = a.x + t0 * dx;
      p.y = a.y + t0 * dy;
      p.z = a.z + t0 * dz;
      p.t = a.t + (TimestampTz)(t0 * dt);
      meos_array_add(trajpts, &p);

      /* Interpolate second point */
      p.x = a.x + t1 * dx;
      p.y = a.y + t1 * dy;
      p.z = a.z + t1 * dz;
      p.t = a.t + (TimestampTz) llround(t1 * dt);
      meos_array_add(trajpts, &p);
    }
  }

  /* Free only if heap was used */
  if (events != stack_events)
    pfree(events);
}

/*****************************************************************************
 * Clip a temporal sequence
 *****************************************************************************/

/**
 * @brief Return a trajectory point from a geometric temporal point
 */
static TrajPoint
tinstant_to_trajpoint(const TInstant *inst)
{
  POINT4D p;
  datum_point4d(tinstant_value(inst), &p);
  TrajPoint tp;
  tp.x = p.x;
  tp.y = p.y;
  tp.z = p.z;
  tp.t = inst->t;
  return tp;
}

/**
 * @brief Clip a 2D/3D trajectory with linear interpolation with respect to a
 * geometry and possibly a Z span
 */
static TSequence *
tpointseq_clip(const TSequence *seq, const Edge *edges, int nedges,
  const Span *zspan)
{
  assert(MEOS_FLAGS_GET_INTERP(seq->flags) == LINEAR);
  assert(! MEOS_FLAGS_GET_GEODETIC(seq->flags));

  int32_t srid = tspatial_srid((Temporal *) seq);
  MeosArray *trajpts = meos_array_init(sizeof(TrajPoint));
  TrajPoint a = tinstant_to_trajpoint(TSEQUENCE_INST_N(seq, 0));
  for (int i = 1; i < seq->count; i++)
  {
    TrajPoint b = tinstant_to_trajpoint(TSEQUENCE_INST_N(seq, i));
    if (zspan && 
        ! contains_span_value(zspan, a.z) && ! contains_span_value(zspan, b.z))
      continue;
    clip_segment(a, b, edges, nedges, trajpts);
    a = b;
  }

  if (trajpts->count == 0)
  {
    meos_array_destroy(trajpts);
    return NULL;
  }
  
  TrajPoint *pts = trajpts->elems;
  bool hasz = MEOS_FLAGS_GET_Z(seq->flags);
  TInstant **instants = palloc(sizeof(TInstant *) * trajpts->count);
  for (int i = 0; i < (int) trajpts->count; i++)
  {
    LWPOINT *pt = hasz ? lwpoint_make3dz(srid, pts[i].x, pts[i].y, pts[i].z) :
      lwpoint_make2d(srid, pts[i].x, pts[i].y);
    GSERIALIZED *gs = geo_serialize((LWGEOM *) pt);
    instants[i] = tinstant_make(PointerGetDatum(gs), seq->temptype, pts[i].t);
    lwpoint_free(pt);
  }
  TSequence *result = tsequence_make_free(instants, trajpts->count,
    seq->period.lower_inc, seq->period.upper_inc, LINEAR, NORMALIZE);
  meos_array_destroy(trajpts);
  return result;
}

/**
 * @brief Return a temporal point sequence with linear interpolation
 * restricted to a (multi)polygon
 * @details For performance reasons we avoid the call to ST_Intersection
 * which delegates the computation to GEOS. 
 * @pre The arguments have the same SRID, the geometry is 2D and is not empty.
 * This is verified in #tgeo_restrict_geom
 */
static TSequenceSet *
tpointseq_linear_at_poly(const TSequence *seq, const GSERIALIZED *gs)
{
  
}


/*****************************************************************************/
