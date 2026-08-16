/***********************************************************************
 *
 * This MobilityDB code is provided under The PostgreSQL License.
 * Copyright (c) 2016-2026, Université libre de Bruxelles and MobilityDB
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
 * @brief Fast 2D/3D temporal point clipping against 2D geometries
 * @details Support (multi)point, (multi)line, triangle, (multi)polygons with
 * holes and islands inside holes (recursively), and collection of the above
 * @note Processing is done natively to improve performance
 */

/* C */
#include <math.h>
/* PostgreSQL */
#include "postgres.h"
#include <utils/float.h>
#include <utils/timestamp.h>
/* PostGIS */
#include "liblwgeom.h"
#include "liblwgeom_internal.h"
/* MEOS */
#include "meos.h"
#include "meos_internal_geo.h"
#include "temporal/span.h"
#include "temporal/temporal.h"
#include "temporal/temporal_restrict.h"
#include "geo/tgeo.h"
#include "geo/tgeo_spatialfuncs.h"
#include "geo/postgis_funcs.h"

/* Minimum number of edges to use an R-tree index in order to compensate the
 * overhead of the tree construction and destruction */
#define RTREE_MIN_NUMBER_ELEMS 100

/*****************************************************************************
 * Data structures
 *****************************************************************************/

/* Per-thread arrays for accumulating the results of the clipping process.
 * MEOS_TLS is required: concurrent callers from different threads would
 * otherwise race on these file-scope pointers, causing heap corruption. */
static MEOS_TLS MeosArray *events = NULL;
static MEOS_TLS MeosArray *intervals = NULL;
static MEOS_TLS MeosArray *periods = NULL;
static MEOS_TLS MeosArray *rtree_results = NULL;

/**
 * @brief Enumeration defining the edge types 
 */
typedef enum
{
  EDGE_POINT = 0,
  EDGE_LINE,
  EDGE_POLY,
  EDGE_ARC,
  EDGE_POLYARC
} EdgeType;

/**
 * @brief Structure keeping a geometry edge
 */
typedef struct
{
  double x1, y1, x2, y2;         /**< Coordinates of the start/end 2D points */
  double xmin, ymin, xmax, ymax; /**< Precomputed bounding box of the edge */
  double dx, dy, length;         /**< Precomputed dx, dy, and length */
  double cx, cy, radius;         /**< Arc center and radius (EDGE_ARC only) */
  double theta0, theta1;         /**< Arc start/end angles (EDGE_ARC only) */
  bool ccw;                      /**< Arc traversed counterclockwise (EDGE_ARC) */
  EdgeType etype;                /**< Edge type */
} Edge;

/**
 * @brief Enumeration defining the intersection types 
 */
typedef enum
{
  INTERSECT_NONE = 0,
  INTERSECT_POINT,
  INTERSECT_OVERLAP
} IntersectType;

/**
 * @brief Structure keeping an intersection result
 */
typedef struct
{
  IntersectType type;  /**< Intersection type */
  double t0;           /**< Always valid if type != NONE */
  double t1;           /**< Only valid for OVERLAP */
} IntersectResult;

/*****************************************************************************
 * Line segment intersection
 *****************************************************************************/

/**
 * @brief Return the intersection value obtained by computing the intersection 
 * of a line segment defined by two 2D points intersects an edge
 * @details Possible result values
 * - No intersection: INTERSECT_NONE -> t0 and t1 undefined
 * - Single point: INTERSECT_POINT -> t1 in [0,1], t1 ignored
 * - Overlap segment: INTERSECT_OVERLAP -> t0 <= t1 in [0,1]
 * Invariants:
 * - 0 <= t0 <= 1
 * - 0 <= t1 <= 1
 * - t0 <= t1
 * - Overlap must satisfy: t1 - t0 > FP_TOLERANCE
 * @param[in] ax,ay Coordinates of the first point defining the first segment
 * @param[in] rx,ry Vector AB
 * @param[in] cx,cy,dx,dy Coordinates of the points defining the second segment
 * @note To avoid recomputing vector AB in EVERY call to the functions,
 * we pass the vector instead of the second point b computed as follows
 * @code
 * double rx = bx - ax, ry = by - ay;
 * @endcode
 */
static inline IntersectResult
linesegm_intersect(double ax, double ay, double rx, double ry,
  double cx, double cy, double dx, double dy)
{
  IntersectResult res = {INTERSECT_NONE, 0, 0};
  double sx = dx - cx, sy = dy - cy; /* vector CD */
  /* Where is the start of the second segment relative to the first? */
  double qpx = cx - ax, qpy = cy - ay;

  /* Are the two segments parallel?  */
  double rxs = rx * sy - ry * sx;

  /* Collinear / parallel */
  if (fabs(rxs) < FP_TOLERANCE)
  {
    /* Is point C aligned with segment AB? */
    double qpxr = qpx * ry - qpy * rx;
    /* If qpxr != 0: parallel, if qpxr == 0: collinear */
    if (fabs(qpxr) > FP_TOLERANCE)
      return res;

    /* Collinear case */
    double r2 = rx * rx + ry * ry;
    if (r2 < FP_TOLERANCE)
      return res;

    double t0 = (qpx * rx + qpy * ry) / r2;
    double t1 = t0 + (sx * rx + sy * ry) / r2;

    /* Order t0 < t1 */
    if (t0 > t1) { double tmp = t0; t0 = t1; t1 = tmp; }
    /* No intersection */
    if (t1 < 0 || t0 > 1)
      return res;

    /* Clamp values */
    if (t0 < 0) t0 = 0;
    if (t1 > 1) t1 = 1;

    if (fabs(t1 - t0) < FP_TOLERANCE)
    {
      res.type = INTERSECT_POINT;
      res.t0 = t0;
      return res;
    }

    res.type = INTERSECT_OVERLAP;
    res.t0 = t0;
    res.t1 = t1;
    return res;
  }

  /* Proper intersection */
  double t = (qpx * sy - qpy * sx) / rxs;
  double u = (qpx * ry - qpy * rx) / rxs;

  if (t < -FP_TOLERANCE || t > 1 + FP_TOLERANCE ||
      u < -FP_TOLERANCE || u > 1 + FP_TOLERANCE)
    return res;

  /* Clamp values */
  if (fabs(t) < FP_TOLERANCE) t = 0;
  if (fabs(t - 1) < FP_TOLERANCE) t = 1;

  res.type = INTERSECT_POINT;
  res.t0 = t;
  return res;
}

/*****************************************************************************
 * Circular arc intersection
 *****************************************************************************/

/**
 * @brief Normalize an angle into the range [0, 2*pi)
 */
static inline double
angle_normalize(double a)
{
  double r = fmod(a, 2 * M_PI);
  if (r < 0)
    r += 2 * M_PI;
  return r;
}

/**
 * @brief Return true if an angle lies within the angular span of an arc edge
 * @details The span is traversed from #theta0 to #theta1, counterclockwise
 * when #ccw is true and clockwise otherwise
 */
static bool
arc_contains_angle(const Edge *e, double phi)
{
  double sweep = e->ccw ?
    angle_normalize(e->theta1 - e->theta0) :
    angle_normalize(e->theta0 - e->theta1);
  double off = e->ccw ?
    angle_normalize(phi - e->theta0) :
    angle_normalize(e->theta0 - phi);
  return off <= sweep + FP_TOLERANCE;
}

/**
 * @brief Set the bounding box of an arc edge
 * @details The box spans the two endpoints plus any of the four cardinal
 * extreme points of the circle that fall within the arc's angular span
 */
static void
arc_set_bbox(Edge *e)
{
  double xmin = FP_MIN(e->x1, e->x2), xmax = FP_MAX(e->x1, e->x2);
  double ymin = FP_MIN(e->y1, e->y2), ymax = FP_MAX(e->y1, e->y2);
  const double ang[4] = {0.0, M_PI_2, M_PI, -M_PI_2};
  const double ex[4] = {e->cx + e->radius, e->cx, e->cx - e->radius, e->cx};
  const double ey[4] = {e->cy, e->cy + e->radius, e->cy, e->cy - e->radius};
  for (int k = 0; k < 4; k++)
    if (arc_contains_angle(e, ang[k]))
    {
      if (ex[k] < xmin) xmin = ex[k];
      if (ex[k] > xmax) xmax = ex[k];
      if (ey[k] < ymin) ymin = ey[k];
      if (ey[k] > ymax) ymax = ey[k];
    }
  e->xmin = xmin; e->xmax = xmax; e->ymin = ymin; e->ymax = ymax;
  return;
}

/**
 * @brief Return true if a point is located on an arc edge
 */
static bool
point_on_arc(double px, double py, const Edge *e)
{
  double d = hypot(px - e->cx, py - e->cy);
  if (fabs(d - e->radius) > FP_TOLERANCE)
    return false;
  return arc_contains_angle(e, atan2(py - e->cy, px - e->cx));
}

/**
 * @brief Return the trajectory parameters at which a trajectory segment
 * intersects an arc edge
 * @details Solves |A + t*R - C|^2 = r^2 for the trajectory parameter t in
 * [0,1], keeping only the roots whose point lies within the arc's angular
 * span. A straight segment meets a circle in at most two points, so the
 * result is never an overlap
 * @param[in] ax,ay Coordinates of the start of the trajectory segment
 * @param[in] rx,ry Vector of the trajectory segment
 * @param[in] e Arc edge
 * @param[out] out Accepted trajectory parameters, ordered as found
 * @return Number of accepted parameters (0, 1, or 2)
 */
static int
arcsegm_intersect(double ax, double ay, double rx, double ry, const Edge *e,
  double out[2])
{
  double aa = rx * rx + ry * ry;
  /* Degenerate (zero-length) trajectory segment */
  if (aa < FP_TOLERANCE)
    return 0;

  double wx = ax - e->cx, wy = ay - e->cy;
  double bb = 2 * (wx * rx + wy * ry);
  double cc = wx * wx + wy * wy - e->radius * e->radius;
  double disc = bb * bb - 4 * aa * cc;
  /* No real root */
  if (disc < -FP_TOLERANCE)
    return 0;
  if (disc < 0)
    disc = 0;

  double sq = sqrt(disc);
  double roots[2];
  int nroots = 0;
  roots[nroots++] = (-bb - sq) / (2 * aa);
  /* Distinct second root only when the line is not tangent */
  if (sq > FP_TOLERANCE)
    roots[nroots++] = (-bb + sq) / (2 * aa);

  int n = 0;
  for (int k = 0; k < nroots; k++)
  {
    double t = roots[k];
    if (t < -FP_TOLERANCE || t > 1 + FP_TOLERANCE)
      continue;
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    double px = ax + t * rx, py = ay + t * ry;
    if (arc_contains_angle(e, atan2(py - e->cy, px - e->cx)))
      out[n++] = t;
  }
  return n;
}

/**
 * @brief Return true if two circular arc edges intersect
 * @details The supporting circles of two arcs meet on their radical line at
 * `a = (d^2 + r1^2 - r2^2) / (2 d)` from the first centre, at a half-chord
 * `h = sqrt(r1^2 - a^2)` off the centre line, giving at most two candidate
 * points; a candidate is a genuine arc intersection only when it lies within
 * the angular span of both arcs, tested with #arc_contains_angle. Concentric
 * arcs of equal radius lie on the same circle: they meet iff their angular
 * spans share an endpoint.
 */
static bool
arcarc_intersect(const Edge *e1, const Edge *e2)
{
  double dx = e2->cx - e1->cx, dy = e2->cy - e1->cy;
  double d = hypot(dx, dy);
  double r1 = e1->radius, r2 = e2->radius;

  /* Concentric supporting circles */
  if (d < FP_TOLERANCE)
  {
    if (fabs(r1 - r2) > FP_TOLERANCE)
      return false;
    /* Same circle: the arcs meet iff their spans share an endpoint angle */
    return arc_contains_angle(e2, e1->theta0) ||
      arc_contains_angle(e2, e1->theta1) ||
      arc_contains_angle(e1, e2->theta0) ||
      arc_contains_angle(e1, e2->theta1);
  }
  /* Circles too far apart or one strictly inside the other */
  if (d > r1 + r2 + FP_TOLERANCE || d < fabs(r1 - r2) - FP_TOLERANCE)
    return false;

  double a = (d * d + r1 * r1 - r2 * r2) / (2 * d);
  double h2 = r1 * r1 - a * a;
  if (h2 < 0)
    h2 = 0;
  double h = sqrt(h2);
  double ux = dx / d, uy = dy / d;         /* Unit vector from c1 to c2 */
  double mx = e1->cx + a * ux, my = e1->cy + a * uy; /* Foot on the centre line */

  /* Candidate points m +/- h * perp(u), tested against both arcs' spans */
  for (int k = 0; k < 2; k++)
  {
    double px = mx + (k ? h : -h) * (-uy);
    double py = my + (k ? h : -h) * ux;
    if (arc_contains_angle(e1, atan2(py - e1->cy, px - e1->cx)) &&
        arc_contains_angle(e2, atan2(py - e2->cy, px - e2->cx)))
      return true;
    /* A tangency has a single candidate point */
    if (h < FP_TOLERANCE)
      break;
  }
  return false;
}

/*****************************************************************************
 * Compute the intervals in [0,1] resulting from the intersection of a
 * trajectory segment and an array of edges obtained from a (collection of)
 * polygon/line/point geometries
 *****************************************************************************/

/**
 * @brief Return true if a point is located on a segment
 */
static bool
point_on_segment(double px, double py, double x1, double y1, double x2,
  double y2)
{
  /* Vectors AP and AB */
  double apx = px - x1;
  double apy = py - y1;
  double abx = x2 - x1;
  double aby = y2 - y1;

  /* Fast bounding-box rejection */
  if ((px < fmin(x1, x2) - FP_TOLERANCE) ||
      (px > fmax(x1, x2) + FP_TOLERANCE) ||
      (py < fmin(y1, y2) - FP_TOLERANCE) ||
      (py > fmax(y1, y2) + FP_TOLERANCE))
    return false;

  /* Collinearity check via cross product */
  double cross = apx * aby - apy * abx;
  if (fabs(cross) > FP_TOLERANCE)
    return false;

  /* Projection check via dot product */
  double dot = apx * abx + apy * aby;
  if (dot < -FP_TOLERANCE)
    return false;

  /* Check if P lies between A and B */
  double ab2 = abx * abx + aby * aby;
  if (dot > ab2 + FP_TOLERANCE)
    return false;
  return true;
}

/**
 * @brief Return true if a point is located in a polygon 
 */
static inline int
point_in_polygon_impl(double x, double y, Edge **edges, int nedges,
  const RTree *rtree, int32_t srid, double xmax)
{
  int inside = 0;
  int n = nedges;
  if (rtree)
  {
    /* Only edges whose bounding box meets the +x ray from (x,y) can cross it
     * or contain the point; querying the R-tree for those instead of scanning
     * every edge turns the O(nedges) test into O(log nedges + candidates).
     * The even-odd parity is order-independent and every excluded edge lies
     * left of x or off the ray's height, so it can neither cross the +x ray
     * nor contain the point -- the result is identical to the full scan. */
    STBox query;
    double xhi = (x > xmax) ? x : xmax;
    stbox_set(true, false, false, srid, x, xhi, y, y, 0, 0, NULL, &query);
    n = rtree_search(rtree, RTREE_OVERLAPS, &query, rtree_results);
  }
  for (int i = 0; i < n; i++)
  {
    const Edge *restrict e = rtree ?
      edges[*(int *) meos_array_get(rtree_results, i)] : edges[i];

    /* Only polygon boundary edges bound a region. Point, line, and standalone
     * (1D) arc edges are ignored by the even-odd containment test */
    if (e->etype == EDGE_POLYARC)
    {
      /* Boundary check */
      if (point_on_arc(x, y, e))
        return 1;
      /* Cast a ray towards +x. The horizontal line at height y meets the
       * supporting circle at cx +/- sqrt(r^2 - (y - cy)^2); flip the parity
       * for each crossing that lies strictly to the right of the point and
       * within the arc's angular span. A ray that only grazes the circle
       * tangentially (h2 ~ 0) does not cross the boundary */
      const double dyc = y - e->cy;
      const double h2 = e->radius * e->radius - dyc * dyc;
      if (h2 <= FP_TOLERANCE)
        continue;
      const double h = sqrt(h2);
      const double xhit[2] = {e->cx - h, e->cx + h};
      /* Forward traversal direction of the arc in the angle parameter */
      const double s = e->ccw ? 1.0 : -1.0;
      for (int k = 0; k < 2; k++)
      {
        const double xi = xhit[k];
        if (xi <= x)
          continue;
        const double phi = atan2(dyc, xi - e->cx);
        if (! arc_contains_angle(e, phi))
          continue;
        /* Half-open ownership, mirroring the straight-edge
         * (y1 > y) != (y2 > y) rule below: a crossing shared with a
         * neighbouring edge (a ring junction lying exactly on the ray) must be
         * counted once. A crossing at an arc endpoint is owned by this edge
         * only if the arc's interior rises above the ray there; an interior
         * crossing is always transversal and always counted */
        const bool at_ep0 = fabs(xi - e->x1) < FP_TOLERANCE &&
          fabs(y - e->y1) < FP_TOLERANCE;
        const bool at_ep1 = fabs(xi - e->x2) < FP_TOLERANCE &&
          fabs(y - e->y2) < FP_TOLERANCE;
        if (at_ep0 || at_ep1)
        {
          const double theta_e = at_ep0 ? e->theta0 : e->theta1;
          const double dtheta_in = at_ep0 ? s : -s;
          if (dtheta_in * cos(theta_e) <= 0)
            continue;
        }
        inside ^= 1;
      }
      continue;
    }

    if (e->etype != EDGE_POLY)
      continue;

    const double dx  = e->dx;
    const double dy  = e->dy;
    const double x1  = e->x1;
    const double y1  = e->y1;

    const double dxp = x - x1;
    const double dyp = y - y1;

    /* Boundary check */
    const double cross = dx * dyp - dy * dxp;
    if (fabs(cross) < FP_TOLERANCE)
    {
      const double dot = dxp * dx + dyp * dy;
      if (dot >= -FP_TOLERANCE && dot <= (e->length) + FP_TOLERANCE)
        return 1;
    }

    /* Ray casting */
    if ((y1 > y) != ((y1 + dy) > y))
    {
      const double rhs = dx * dyp;
      const double lhs = dxp * dy;
      inside ^= ((dy > 0) ? (rhs > lhs) : (rhs < lhs));
    }
  }
  return inside;
}

/**
 * @brief Return true if a point is located in a polygon, scanning every edge
 */
static inline int
point_in_polygon(double x, double y, Edge **edges, int nedges)
{
  return point_in_polygon_impl(x, y, edges, nedges, NULL, 0, 0.0);
}

/**
 * @brief Compute the intersection intervals of a trajectory segment with an
 * array of point edges
 * @details A segment that does not move carries no direction to solve the
 * parameter along, and meets a point edge exactly where it stands, for the
 * whole of its extent
 */
static void
intervals_from_points(const POINT2D *a, const POINT2D *b, Edge **edges,
  int nedges)
{
  assert(a); assert(b); assert(edges); assert(nedges >= 0);

  /* Segment vector */
  double dx = b->x - a->x;
  double dy = b->y - a->y;

  /* Improve performance by removing the division inside the loop */
  bool use_x = fabs(dx) >= fabs(dy);
  double inv = use_x ?
    ((fabs(dx) > FP_TOLERANCE) ? 1.0 / dx : 0.0) :
    ((fabs(dy) > FP_TOLERANCE) ? 1.0 / dy : 0.0);

  /* The segment stands still */
  if (inv == 0.0)
  {
    for (int i = 0; i < nedges; i++)
    {
      const Edge *e = edges[i];
      if (e->etype != EDGE_POINT)
        continue;
      if (fabs(a->x - e->x1) < FP_TOLERANCE &&
          fabs(a->y - e->y1) < FP_TOLERANCE)
      {
        Span in;
        span_set(Float8GetDatum(0.0), Float8GetDatum(1.0), true, true,
          T_FLOAT8, T_FLOATSPAN, &in);
        meos_array_add(intervals, &in);
      }
    }
    return;
  }

  /* Iterate through the points */
  for (int i = 0; i < nedges; i++)
  {
    const Edge *e = edges[i]; 
    /* Iterate only for the points */
    if (e->etype != EDGE_POINT)
      continue;
    // assert(e->x1 == e->x2 && e->y1 == e->y2);

    /* Solve parameter t */
    double t = use_x ? (e->x1 - a->x) * inv : (e->y1 - a->y) * inv;
    /* Check bounds */
    if (t < -FP_TOLERANCE || t > 1.0 + FP_TOLERANCE)
      continue;

    /* Reconstruct point and add interval */
    double x = a->x + t * dx;
    double y = a->y + t * dy;
    if (fabs(x - e->x1) < FP_TOLERANCE && fabs(y - e->y1) < FP_TOLERANCE)
    {
      Span in;
      span_set(Float8GetDatum(t), Float8GetDatum(t), true, true,
        T_FLOAT8, T_FLOATSPAN, &in);
      meos_array_add(intervals, &in);
    }
  }
  return;
}

/**
 * @brief Compute the intersection intervals of a trajectory segment with an
 * array of linear or point edges
 */
static void
intervals_from_lines(const POINT2D *a, const POINT2D *b, Edge **edges,
  int nedges)
{
  assert(a); assert(b); assert(edges); assert(nedges >= 0);

  const double ax = a->x, ay = a->y;
  const double bx = b->x, by = b->y;

  /* Segment bounding box */
  const double seg_xmin = FP_MIN(ax, bx);
  const double seg_xmax = FP_MAX(ax, bx);
  const double seg_ymin = FP_MIN(ay, by);
  const double seg_ymax = FP_MAX(ay, by);
  /* Segment vector */
  const double rx = bx - ax;  
  const double ry = by - ay;  

  bool has_intersection = false;
  Span in;

  /* Iterate through the lines */
  for (int i = 0; i < nedges; i++)
  {
    const Edge *e = edges[i];
    /* Iterate only for the line edges */
    if (e->etype != EDGE_LINE)
      continue;

    /* Bounding box filter */
    if (e->xmax < seg_xmin || e->xmin > seg_xmax ||
        e->ymax < seg_ymin || e->ymin > seg_ymax)
      continue;

    /* Compute the intersection */
    IntersectResult r = linesegm_intersect(ax, ay, rx, ry,
      e->x1, e->y1, e->x2, e->y2);
    /* If there is no intersection  */
    if (r.type == INTERSECT_NONE)
      continue;

    /* Intersection found: compute the interval */
    has_intersection = true;
    if (r.type == INTERSECT_POINT)
      span_set(Float8GetDatum(r.t0), Float8GetDatum(r.t0), true, true,
        T_FLOAT8, T_FLOATSPAN, &in);
    else
      span_set(Float8GetDatum(r.t0), Float8GetDatum(r.t1), true, true,
        T_FLOAT8, T_FLOATSPAN, &in);
    meos_array_add(intervals, &in);
  }

  /* Full collinear segment */
  if (! has_intersection)
  {
    /* Test midpoint */
    double mx = (ax + bx) * 0.5;
    double my = (ay + by) * 0.5;
    for (int i = 0; i < nedges; i++)
    {
      const Edge *e = edges[i];
      /* Iterate only for the lines edges */
      if (e->etype != EDGE_LINE)
        continue;

      /* Fast bbox check first */
      if (mx < e->xmin - FP_TOLERANCE || mx > e->xmax + FP_TOLERANCE ||
          my < e->ymin - FP_TOLERANCE || my > e->ymax + FP_TOLERANCE)
        continue;

      if (point_on_segment(mx, my, e->x1, e->y1, e->x2, e->y2))
      {
        span_set(Float8GetDatum(0.0), Float8GetDatum(1.0), true, true,
          T_FLOAT8, T_FLOATSPAN, &in);
        meos_array_add(intervals, &in);
        return;
      }
    }
  }
  return;
}

/**
 * @brief Compute the intersection intervals of a trajectory segment with an
 * array of arc edges
 */
static void
intervals_from_arcs(const POINT2D *a, const POINT2D *b, Edge **edges,
  int nedges)
{
  assert(a); assert(b); assert(edges); assert(nedges >= 0);

  const double ax = a->x, ay = a->y;
  const double bx = b->x, by = b->y;
  /* Segment bounding box */
  const double seg_xmin = FP_MIN(ax, bx);
  const double seg_xmax = FP_MAX(ax, bx);
  const double seg_ymin = FP_MIN(ay, by);
  const double seg_ymax = FP_MAX(ay, by);
  /* Segment vector */
  const double rx = bx - ax;
  const double ry = by - ay;

  Span in;
  /* Iterate through the arc edges */
  for (int i = 0; i < nedges; i++)
  {
    const Edge *e = edges[i];
    if (e->etype != EDGE_ARC)
      continue;

    /* Bounding box filter */
    if (e->xmax < seg_xmin || e->xmin > seg_xmax ||
        e->ymax < seg_ymin || e->ymin > seg_ymax)
      continue;

    /* Compute the intersection: at most two point crossings */
    double t[2];
    int n = arcsegm_intersect(ax, ay, rx, ry, e, t);
    for (int k = 0; k < n; k++)
    {
      span_set(Float8GetDatum(t[k]), Float8GetDatum(t[k]), true, true,
        T_FLOAT8, T_FLOATSPAN, &in);
      meos_array_add(intervals, &in);
    }
  }
  return;
}

/**
 * @brief Comparison function for sorting float8 values
 */
static int
float8_qsort_cmp(const void *a1, const void *a2)
{
  double diff = *(const double *)a1 - *(const double *)a2;
  return (diff > 0) - (diff < 0);
}

/**
 * @brief Return true if a point lies on the boundary of a polygonal component
 * @details Only the polygon boundary edges are considered, straight
 * (#EDGE_POLY) and circular (#EDGE_POLYARC). The candidate array filtered by
 * the box of a segment suffices for a point lying on that segment: a boundary
 * edge through such a point has a box meeting the segment box, so it is in the
 * candidates
 */
static bool
point_on_poly_boundary(double px, double py, Edge **edges, int nedges)
{
  for (int i = 0; i < nedges; i++)
  {
    const Edge *e = edges[i];
    if (e->etype == EDGE_POLY)
    {
      if (point_on_segment(px, py, e->x1, e->y1, e->x2, e->y2))
        return true;
    }
    else if (e->etype == EDGE_POLYARC)
    {
      if (point_on_arc(px, py, e))
        return true;
    }
  }
  return false;
}

/**
 * @brief Compute the intersection intervals of a trajectory segment with an
 * array of polygon edges
 * @details The ray-casting cannot use the edges filtered by the segment box,
 * since an edge outside it still crosses the ray and the even-odd containment
 * test would break. It is given the full array and its own R-tree query
 * instead, the +x ray of #point_in_polygon_impl, which excludes only the edges
 * that lie left of the point or off the ray's height and so is identical to
 * the full scan
 * @param[in] rtree R-tree over @p all_edges, or NULL to scan them all
 * @param[in] srid,xmax SRID and geometry bounding-box maximum abscissa, used
 * to query @p rtree
 */
static void
intervals_from_polygons(const POINT2D *a, const POINT2D *b, Edge **edges,
  int nedges, Edge **all_edges, int all_nedges, const RTree *rtree,
  int32_t srid, double xmax)
{
  assert(a); assert(b); assert(edges); assert(nedges >= 0);

  /* Reset event array */
  events->count = 0;

  const double ax = a->x, ay = a->y;
  const double bx = b->x, by = b->y;

  /* Segment bounding box */
  const double seg_xmin = FP_MIN(ax, bx);
  const double seg_xmax = FP_MAX(ax, bx);
  const double seg_ymin = FP_MIN(ay, by);
  const double seg_ymax = FP_MAX(ay, by);
  /* Segment vector */
  const double rx = bx - ax;
  const double ry = by - ay;

  /* Check whether any polygon boundary edges exist using the full edge array.
   * A curve polygon contributes straight (EDGE_POLY) and arc (EDGE_POLYARC)
   * boundary edges */
  bool has_polys = false;
  for (int i = 0; i < all_nedges; i++)
  {
    EdgeType et = all_edges[i]->etype;
    if (et == EDGE_POLY || et == EDGE_POLYARC)
    {
      has_polys = true;
      break;
    }
  }
  /* If no polygon edges have been found, we do not continue */
  if (! has_polys)
    return;

  /* Collect all intersection parameters from the (possibly filtered) edges */
  for (int i = 0; i < nedges; i++)
  {
    const Edge *e = edges[i];
    /* Iterate only for the polygon boundary edges (straight or arc) */
    if (e->etype != EDGE_POLY && e->etype != EDGE_POLYARC)
      continue;

    /* Bounding box filter */
    if (e->xmax < seg_xmin || e->xmin > seg_xmax ||
        e->ymax < seg_ymin || e->ymin > seg_ymax)
      continue;

    if (e->etype == EDGE_POLY)
    {
      /* Compute the crossing with the straight boundary segment */
      IntersectResult r = linesegm_intersect(ax, ay, rx, ry,
        e->x1, e->y1, e->x2, e->y2);
      if (r.type == INTERSECT_POINT)
      {
        double t = r.t0;
        if (t >= -FP_TOLERANCE && t <= 1.0 + FP_TOLERANCE)
          meos_array_add(events, &t);
      }
    }
    else
    {
      /* Compute the crossings with the arc boundary edge (at most two) */
      double t[2];
      int n = arcsegm_intersect(ax, ay, rx, ry, e, t);
      for (int k = 0; k < n; k++)
        if (t[k] >= -FP_TOLERANCE && t[k] <= 1.0 + FP_TOLERANCE)
          meos_array_add(events, &t[k]);
    }
  }

  /* Add endpoints */
  double t0 = 0.0, t1 = 1.0;
  meos_array_add(events, &t0);
  meos_array_add(events, &t1);

  /* Sort */
  qsort(events->elems, events->count, sizeof(double), float8_qsort_cmp);

  /* Deduplicate */
  int newcount = 0;
  double *evtarr = (double *) events->elems;
  for (int i = 0; i < (int) events->count; i++)
  {
    if (i == 0 ||
        fabs(evtarr[i] - evtarr[newcount - 1]) > FP_TOLERANCE)
    {
      evtarr[newcount++] = evtarr[i];
    }
  }
  events->count = newcount;

  /* Build intervals using midpoint test, recording for each event whether it
   * bounds an interval the point spends in the polygon interior */
  int nevents = (int) events->count;
  bool *bounded = palloc0(sizeof(bool) * (nevents > 0 ? nevents : 1));
  for (int i = 0; i < nevents - 1; i++)
  {
    double ta = evtarr[i];
    double tb = evtarr[i + 1];
    if (tb - ta <= FP_TOLERANCE)
      continue;

    /* Midpoint test */
    double tm = (ta + tb) * 0.5;
    double x = ax + tm * rx;
    double y = ay + tm * ry;
    if (point_in_polygon_impl(x, y, all_edges, all_nedges, rtree, srid, xmax))
    {
      Span in;
      span_set(Float8GetDatum(ta), Float8GetDatum(tb), true, true,
        T_FLOAT8, T_FLOATSPAN, &in);
      meos_array_add(intervals, &in);
      bounded[i] = bounded[i + 1] = true;
    }
  }

  /* A segment that meets the boundary without entering the interior is in the
   * closure of the polygon at that instant alone. Such a contact bounds no
   * interval, so it is emitted as the instantaneous interval that it is */
  for (int i = 0; i < nevents; i++)
  {
    if (bounded[i])
      continue;
    double t = evtarr[i];
    if (t < 0.0)
      t = 0.0;
    else if (t > 1.0)
      t = 1.0;
    double x = ax + t * rx;
    double y = ay + t * ry;
    if (! point_on_poly_boundary(x, y, edges, nedges))
      continue;
    Span in;
    span_set(Float8GetDatum(t), Float8GetDatum(t), true, true,
      T_FLOAT8, T_FLOATSPAN, &in);
    meos_array_add(intervals, &in);
  }
  pfree(bounded);
  return;
}

/*****************************************************************************/

/**
 * @brief Return true if a trajectory point intersects with an array of point
 * and linear edges
 */
static bool
point_inter_points_lines(const POINT2D *a, Edge **edges, int nedges)
{
  assert(a); assert(edges); assert(nedges >= 0);

  const double ax = a->x, ay = a->y;

  /* Iterate only through the point and linear edges */
  for (int i = 0; i < nedges; i++)
  {
    const Edge *e = edges[i];
    if (e->etype == EDGE_POINT)
    {
      if (fabs(e->x1 - ax) < FP_TOLERANCE && fabs(e->y1 - ay) < FP_TOLERANCE)
        return true;
    }
    else if (e->etype == EDGE_LINE)
    {
      if (point_on_segment(ax, ay, e->x1, e->y1, e->x2, e->y2))
        return true;
    }
    else if (e->etype == EDGE_ARC)
    {
      if (point_on_arc(ax, ay, e))
        return true;
    }
  }
  return false;
}

/*****************************************************************************
 * Clip a temporal geometry point
 *****************************************************************************/

/**
 * @brief Clip a 2D/3D trajectory with linear interpolation with respect to a
 * geometry
 * @param[in] inst Temporal sequence
 * @param[in] edges Array of geometry edges
 * @param[in] nedges Number of edges in the array
 * @param[in] rtree R-tree for the edges, may be `NULL` if no index is used
 * @param[in] cand_edges Edge array buffer of size `nedges` for storing the
 * result of an R-tree look up, may be `NULL` if no index is used
 */
static void
tpointinst_clip_edges(const TInstant *inst, Edge **edges, int nedges,
  const RTree *rtree, Edge **cand_edges, double xmax)
{
  assert(inst); assert(edges); assert(nedges > 0);
  assert(inst->temptype == T_TGEOMPOINT);

  const POINT2D *a = DATUM_POINT2D_P(tinstant_value_p(inst));

  /* Edges to process: all of them (default) or those filtered by an R-tree */
  Edge **sel_edges = edges;
  int sel_nedges = nedges;
  bool use_index = (rtree != NULL && cand_edges != NULL);
  if (use_index)
  {
    /* Build the segment bounding box */
    STBox query;
    int32_t srid = tspatial_srid((Temporal *) inst);
    stbox_set(true, false, false, srid, a->x, a->x, a->y, a->y, 0, 0, NULL,
      &query);
    /* Query the R-tree */
    int cand_nedges = rtree_search(rtree, RTREE_OVERLAPS, &query, rtree_results);

    /* Convert the result of an R-tree look up into an edge pointer array */
    for (int j = 0; j < cand_nedges; j++)
      cand_edges[j] = edges[*(int *) meos_array_get(rtree_results, j)];
    sel_edges = cand_edges;
    sel_nedges = cand_nedges;
  }

  /* Reset the interval array */
  intervals->count = 0;
  /* Compute the intervals for the points, lines, and polygon edges */
  bool found = point_inter_points_lines(a, sel_edges, sel_nedges);
  if (! found)
  {
    intervals_from_polygons(a, a, sel_edges, sel_nedges, edges, nedges, rtree,
      tspatial_srid((Temporal *) inst), xmax);
    if (intervals->count == 0)
      return;
  }
  
  /* Generate the instantantaneous span */
  Span s;
  span_set(TimestampTzGetDatum(inst->t), TimestampTzGetDatum(inst->t),
    true, true, T_TIMESTAMPTZ, T_TSTZSPAN, &s);
  meos_array_add(periods, &s);
  return;
}

/**
 * @brief Clip a 2D/3D trajectory with linear interpolation with respect to a
 * geometry
 * @param[in] seq Temporal sequence
 * @param[in] edges Array of geometry edges
 * @param[in] nedges Number of edges in the array
 * @param[in] rtree R-tree for the edges, may be `NULL` if no index is used
 * @param[in] cand_edges Edge array buffer of size `nedges` for storing the
 * result of an R-tree look up, may be `NULL` if no index is used
 */
static void
tpointseq_clip_edges(const TSequence *seq, Edge **edges, int nedges,
  const RTree *rtree, Edge **cand_edges, double xmax)
{
  assert(seq); assert(edges); assert(nedges > 0);
  assert(seq->temptype == T_TGEOMPOINT);
  assert(MEOS_FLAGS_LINEAR_INTERP(seq->flags));

  /* Singleton sequence */
  if (seq->count == 1)
    return tpointinst_clip_edges(TSEQUENCE_INST_N(seq, 0), edges, nedges,
      rtree, cand_edges, xmax);

  bool use_index = (rtree != NULL && cand_edges != NULL);
  int32_t srid = tspatial_srid((Temporal *) seq);

  /* Initialize variables for the loop */
  const TInstant *inst1 = TSEQUENCE_INST_N(seq, 0);
  const POINT2D *a = DATUM_POINT2D_P(tinstant_value_p(inst1));
  bool lower_inc = seq->period.lower_inc;
  /* Edges to process: either all of them or those filtered by an R-tree */
  Edge **sel_edges = edges;
  int sel_nedges = nedges;
  /* Loop for each segment */
  for (int i = 1; i < seq->count; i++)
  {
    const TInstant *inst2 = TSEQUENCE_INST_N(seq, i);
    const POINT2D *b = DATUM_POINT2D_P(tinstant_value_p(inst2));
    bool upper_inc = (i < seq->count - 1) ? false : seq->period.upper_inc;

    /* Filter the edges to process by a R-tree, if any */
    if (use_index)
    {
      /* Build the segment bounding box */
      STBox query;
      stbox_set(true, false, false, srid, FP_MIN(a->x, b->x),
        FP_MAX(a->x, b->x), FP_MIN(a->y, b->y), FP_MAX(a->y, b->y),
        0, 0, NULL, &query);
      /* Query the R-tree */
      int cand_nedges = rtree_search(rtree, RTREE_OVERLAPS, &query, rtree_results);

      /* Convert the result of an R-tree look up into an edge pointer array */
      for (int j = 0; j < cand_nedges; j++)
        cand_edges[j] = edges[*(int *) meos_array_get(rtree_results, j)];
      sel_edges = cand_edges;
      sel_nedges = cand_nedges;
    }

    /* Reset the interval array */
    intervals->count = 0;
    /* Compute the intervals for the points, lines, and polygon edges */
    intervals_from_points(a, b, sel_edges, sel_nedges);
    intervals_from_lines(a, b, sel_edges, sel_nedges);
    intervals_from_arcs(a, b, sel_edges, sel_nedges);
    intervals_from_polygons(a, b, sel_edges, sel_nedges, edges, nedges, rtree,
      srid, xmax);
    /* The array is declared before the jump below, which would otherwise skip
     * its initializer while `next_segment` reads it */
    Span *intervarr = NULL;
    if (intervals->count == 0)
      goto next_segment;

    /* Normalize the intervals */
    int count;
    if (intervals->count > 1)
      intervarr = spanarr_normalize(intervals->elems, intervals->count,
        ORDER_NO, &count);
    else
    {
      intervarr = intervals->elems;
      count = 1;
    }

    /* Generate the periods from the float spans taking into account exclusive
     * temporal bounds */
    double duration = (double) (inst2->t - inst1->t);
    for (int j = 0; j < count; j++)
    {
      Span s;
      double lower = DatumGetFloat8(intervarr[j].lower);
      double upper = DatumGetFloat8(intervarr[j].upper);
      if (fabs(upper - lower) < FP_TOLERANCE)
      {
        /* Remove intersection points on exclusive lower and upper bounds */
        if (! lower_inc && fabs(lower) < FP_TOLERANCE &&
            fabs(upper) < FP_TOLERANCE)
          continue;
        if (! upper_inc && fabs(lower - 1.0) < FP_TOLERANCE &&
            fabs(upper - 1.0) < FP_TOLERANCE)
          continue;

        /* Interpolate only if 0 < lower/upper < 1 */
        TimestampTz t = (lower == 0.0) ?
          inst1->t : inst1->t + (TimestampTz) (duration * lower);
        span_set(TimestampTzGetDatum(t), TimestampTzGetDatum(t), true, true,
          T_TIMESTAMPTZ, T_TSTZSPAN, &s);
        meos_array_add(periods, &s);
      }
      else
      {
        TimestampTz t1 = (lower == 0.0) ?
          inst1->t : inst1->t + (TimestampTz) (duration * lower);
        TimestampTz t2 = (upper == 1.0) ?
          inst2->t : inst1->t + (TimestampTz) (duration * upper);
        span_set(TimestampTzGetDatum(t1), TimestampTzGetDatum(t2), true, true,
          T_TIMESTAMPTZ, T_TSTZSPAN, &s);
        meos_array_add(periods, &s);
      }
    }
    
next_segment:
    /* Prepare the next iteration */
    if (intervarr && intervals->count > 1)
      pfree(intervarr);
    inst1 = inst2;
    a = b;
  }
  return;
}

/*****************************************************************************
 * Extract edges from a geometry that can be of type point, line, polygon or
 * collection of these
 *****************************************************************************/

/**
 * @brief Add to the dynamic array in the last argument the edges obtained
 * from a ring
 */
static void
emit_ring_edges(const POINTARRAY *pa, MeosArray *edges, EdgeType etype)
{
  for (int i = 0; i < (int) pa->npoints - 1; i++)
  {
    POINT4D a, b;
    (void) getPoint4d_p(pa, i, &a);
    (void) getPoint4d_p(pa, i + 1, &b);
    Edge e;
    e.x1 = a.x; e.y1 = a.y;
    e.x2 = b.x; e.y2 = b.y;
    e.xmin = FP_MIN(e.x1, e.x2); e.xmax = FP_MAX(e.x1, e.x2);
    e.ymin = FP_MIN(e.y1, e.y2); e.ymax = FP_MAX(e.y1, e.y2);
    e.dx = e.x2 - e.x1; e.dy = e.y2 - e.y1;
    e.length = e.dx * e.dx + e.dy * e.dy;
    e.etype = etype;
    meos_array_add(edges, &e);
  }
  return;
}

/**
 * @brief Add to the dynamic array in the last argument the edge obtained
 * from a point
 */
static void
extract_point(const LWPOINT *pt, MeosArray *edges)
{
  /* An empty point (e.g. a component of a multipoint or the boundary of a
   * closed trajectory) has no vertex to read; it contributes no edge. */
  if (! pt->point || pt->point->npoints < 1)
    return;
  POINT4D p;
  (void) getPoint4d_p(pt->point, 0, &p);
  Edge e;
  e.x1 = e.x2 = e.xmin = e.xmax = p.x;
  e.y1 = e.y2 = e.ymin = e.ymax = p.y;
  e.dx = e.dy = e.length = 0;
  e.etype = EDGE_POINT;
  meos_array_add(edges, &e);
  return;
}

/**
 * @brief Add to the dynamic array in the last argument the edges obtained
 * from a multipoint
 */
static void
extract_mpoint(const LWMPOINT *mp, MeosArray *edges)
{
  for (int i = 0; i < (int) mp->ngeoms; i++)
    extract_point((const LWPOINT *) mp->geoms[i], edges);
  return;
}

/**
 * @brief Add to the dynamic array in the last argument the segments obtained
 * from a line
 */
static void
extract_line(const LWLINE *line, MeosArray *edges)
{
  emit_ring_edges(line->points, edges, EDGE_LINE);
  return;
}

/**
 * @brief Add to the dynamic array in the last argument the segments obtained
 * from a multiline
 */
static void
extract_mline(const LWMLINE *ml, MeosArray *edges)
{
  for (int i = 0; i < (int) ml->ngeoms; i++)
    extract_line(ml->geoms[i], edges);
  return;
}

/**
 * @brief Add to the dynamic array in the last argument the edges obtained
 * from a polygon
 */
static void
extract_poly(const LWPOLY *poly, MeosArray *edges)
{
  for (int r = 0; r < (int) poly->nrings; r++)
    emit_ring_edges(poly->rings[r], edges, EDGE_POLY);
  return;
}

/**
 * @brief Add to the dynamic array in the last argument the edges obtained
 * from a multipolygon
 */
static void
extract_mpoly(const LWMPOLY *mp, MeosArray *edges)
{
  for (int i = 0; i < (int) mp->ngeoms; i++)
    extract_poly(mp->geoms[i], edges);
  return;
}

/**
 * @brief Add to the dynamic array in the last argument the edges obtained
 * from a triangle
 * @details In PostGIS a triangle has a single (outer) ring stored as
 * POINTARRAY, which is already closed or implicitly closed
 */
static void
extract_triangle(const LWTRIANGLE *tri, MeosArray *edges)
{
  emit_ring_edges(tri->points, edges, EDGE_POLY);
  return;
}

/**
 * @brief Add to the dynamic array in the last argument the edge obtained from
 * three consecutive points of a circular string
 * @details The three points are the start, an intermediate point, and the end
 * of the arc. Three collinear points degenerate to straight segments and are
 * emitted as line edges
 */
static void
emit_arc_edge(const POINT4D *pa, const POINT4D *pb, const POINT4D *pc,
  MeosArray *edges, EdgeType line_etype, EdgeType arc_etype)
{
  double ax = pa->x, ay = pa->y;
  double bx = pb->x, by = pb->y;
  double cx = pc->x, cy = pc->y;
  /* Twice the signed area of the triangle; zero when the points are collinear */
  double d = 2 * (ax * (by - cy) + bx * (cy - ay) + cx * (ay - by));

  /* Collinear points: emit straight line edges */
  if (fabs(d) < FP_TOLERANCE)
  {
    const double px[3] = {ax, bx, cx}, py[3] = {ay, by, cy};
    for (int i = 0; i < 2; i++)
    {
      Edge e;
      e.x1 = px[i]; e.y1 = py[i];
      e.x2 = px[i + 1]; e.y2 = py[i + 1];
      e.xmin = FP_MIN(e.x1, e.x2); e.xmax = FP_MAX(e.x1, e.x2);
      e.ymin = FP_MIN(e.y1, e.y2); e.ymax = FP_MAX(e.y1, e.y2);
      e.dx = e.x2 - e.x1; e.dy = e.y2 - e.y1;
      e.length = e.dx * e.dx + e.dy * e.dy;
      e.etype = line_etype;
      meos_array_add(edges, &e);
    }
    return;
  }

  double a2 = ax * ax + ay * ay;
  double b2 = bx * bx + by * by;
  double c2 = cx * cx + cy * cy;
  Edge e;
  /* Circumcenter of the three points */
  e.cx = (a2 * (by - cy) + b2 * (cy - ay) + c2 * (ay - by)) / d;
  e.cy = (a2 * (cx - bx) + b2 * (ax - cx) + c2 * (bx - ax)) / d;
  e.radius = hypot(ax - e.cx, ay - e.cy);
  e.x1 = ax; e.y1 = ay;
  e.x2 = cx; e.y2 = cy;
  e.theta0 = atan2(ay - e.cy, ax - e.cx);
  e.theta1 = atan2(cy - e.cy, cx - e.cx);
  /* Traversal orientation from the signed area of (start, mid, end) */
  e.ccw = ((bx - ax) * (cy - ay) - (by - ay) * (cx - ax)) > 0;
  e.dx = e.dy = e.length = 0;
  e.etype = arc_etype;
  arc_set_bbox(&e);
  meos_array_add(edges, &e);
  return;
}

/**
 * @brief Add to the dynamic array in the last argument the arc edges obtained
 * from a circular string, emitting them with the given line/arc edge types
 * @details Straight components (collinear point triples) are emitted with
 * @p line_etype and genuine arcs with @p arc_etype. A standalone circular
 * string uses the 1D types (#EDGE_LINE / #EDGE_ARC); a circular string that
 * bounds a curve polygon ring uses the region types (#EDGE_POLY /
 * #EDGE_POLYARC)
 */
static void
emit_circstring_edges(const LWCIRCSTRING *circ, MeosArray *edges,
  EdgeType line_etype, EdgeType arc_etype)
{
  const POINTARRAY *pa = circ->points;
  int np = (int) pa->npoints;
  for (int i = 0; i + 2 < np; i += 2)
  {
    POINT4D pa4, pb4, pc4;
    (void) getPoint4d_p(pa, i, &pa4);
    (void) getPoint4d_p(pa, i + 1, &pb4);
    (void) getPoint4d_p(pa, i + 2, &pc4);
    emit_arc_edge(&pa4, &pb4, &pc4, edges, line_etype, arc_etype);
  }
  return;
}

/**
 * @brief Add to the dynamic array in the last argument the arc edges obtained
 * from a circular string
 */
static void
extract_circstring(const LWCIRCSTRING *circ, MeosArray *edges)
{
  emit_circstring_edges(circ, edges, EDGE_LINE, EDGE_ARC);
  return;
}

/**
 * @brief Add to the dynamic array in the last argument the region-boundary
 * edges obtained from a ring of a curve polygon
 * @details A ring is a line string, a circular string, or a compound curve
 * chaining both. Every edge is emitted with polygon (region) semantics
 * (#EDGE_POLY / #EDGE_POLYARC) so that the even-odd containment test in
 * #point_in_polygon treats it as a boundary rather than a 1D feature
 */
static void
extract_curvepoly_ring(const LWGEOM *ring, MeosArray *edges)
{
  switch (ring->type)
  {
    case LINETYPE:
      emit_ring_edges(((const LWLINE *) ring)->points, edges, EDGE_POLY);
      break;

    case CIRCSTRINGTYPE:
      emit_circstring_edges((const LWCIRCSTRING *) ring, edges, EDGE_POLY,
        EDGE_POLYARC);
      break;

    /* A compound curve is a chain of line strings and circular strings; it
     * shares the collection memory layout, so its components are processed as
     * ring pieces in the same way */
    case COMPOUNDTYPE:
    {
      const LWCOLLECTION *col = (const LWCOLLECTION *) ring;
      for (int i = 0; i < (int) col->ngeoms; i++)
        extract_curvepoly_ring(col->geoms[i], edges);
      break;
    }

    /* Unsupported ring type */
    default:
      meos_error(ERROR, MEOS_ERR_FEATURE_NOT_SUPPORTED,
        "Unsupported curve polygon ring type");
      break;
  }
  return;
}

/**
 * @brief Add to the dynamic array in the last argument the edges obtained
 * from a curve polygon
 */
static void
extract_curvepoly(const LWCURVEPOLY *cp, MeosArray *edges)
{
  for (int r = 0; r < (int) cp->nrings; r++)
    extract_curvepoly_ring(cp->rings[r], edges);
  return;
}

/**
 * @brief Return the edges of a geometry in a dynamic array (iterator)
 */
static void
geom_extract_edges_iter(const LWGEOM *geom, MeosArray *edges)
{
  /* Skip empty (sub-)geometries: an empty component contributes no edges, and
   * extracting one would read vertex 0 of an empty point array. This covers
   * empty parts nested inside a multi-geometry or collection too. */
  if (! geom || lwgeom_is_empty(geom))
    return;

  switch (geom->type)
  {
    case POINTTYPE:
      extract_point((const LWPOINT *) geom, edges);
      break;

    case MULTIPOINTTYPE:
      extract_mpoint((const LWMPOINT *) geom, edges);
      break;

    case LINETYPE:
      extract_line((const LWLINE *) geom, edges);
      break;

    case MULTILINETYPE:
      extract_mline((const LWMLINE *) geom, edges);
      break;

    case POLYGONTYPE:
      extract_poly((const LWPOLY *) geom, edges);
      break;

    case MULTIPOLYGONTYPE:
      extract_mpoly((const LWMPOLY *) geom, edges);
      break;

    case TRIANGLETYPE:
      extract_triangle((const LWTRIANGLE *) geom, edges);
      break;

    /* A compound curve (chain of line/circular strings), a multicurve
     * (collection of line/circular/compound curves) and a multisurface
     * (collection of polygons/curve polygons) all share the collection memory
     * layout, so their components are extracted the same way as a collection */
    case COMPOUNDTYPE:
    case MULTICURVETYPE:
    case MULTISURFACETYPE:
    case COLLECTIONTYPE:
    {
      const LWCOLLECTION *col = (const LWCOLLECTION *) geom;
      for (int i = 0; i < (int) col->ngeoms; i++)
        geom_extract_edges_iter(col->geoms[i], edges);
      break;
    }

    case CIRCSTRINGTYPE:
      extract_circstring((const LWCIRCSTRING *) geom, edges);
      break;

    case CURVEPOLYTYPE:
      extract_curvepoly((const LWCURVEPOLY *) geom, edges);
      break;

    /* Unsupported type */
    default:
      meos_error(ERROR, MEOS_ERR_FEATURE_NOT_SUPPORTED,
        "Unsupported geometry type");
      break;
  }
  return;
}

/**
 * @brief Return the edges of a geometry in a dynamic array 
 */
static MeosArray *
geom_extract_edges(const LWGEOM *geom)
{
  MeosArray *edges = meos_array_create(sizeof(Edge));
  geom_extract_edges_iter(geom, edges);
  return edges;
}

/**
 * @brief Return true if a geometry is composed solely of the types the clip
 * engine can extract into edges
 * @details Mirrors the type dispatch of #geom_extract_edges_iter. Geometries
 * containing any other type (curved polygons, TIN, polyhedral surfaces, ...)
 * are not supported and must be handled by the caller
 */
bool
geom_clip_supported(const LWGEOM *geom)
{
  if (! geom)
    return false;
  switch (geom->type)
  {
    case POINTTYPE:
    case MULTIPOINTTYPE:
    case LINETYPE:
    case MULTILINETYPE:
    case POLYGONTYPE:
    case MULTIPOLYGONTYPE:
    case TRIANGLETYPE:
    case CIRCSTRINGTYPE:
      return true;
    case COMPOUNDTYPE:
    case MULTICURVETYPE:
    case MULTISURFACETYPE:
    case COLLECTIONTYPE:
    {
      /* A multicurve/multisurface is supported when every component is: its
       * components are line/circular/compound curves and polygons/curve
       * polygons, each validated by the recursive call */
      const LWCOLLECTION *col = (const LWCOLLECTION *) geom;
      for (uint32_t i = 0; i < col->ngeoms; i++)
        if (! geom_clip_supported(col->geoms[i]))
          return false;
      return true;
    }
    case CURVEPOLYTYPE:
    {
      /* Mirrors the ring dispatch of #extract_curvepoly_ring: a ring must be a
       * line string, a circular string, or a compound curve of those */
      const LWCURVEPOLY *cp = (const LWCURVEPOLY *) geom;
      for (uint32_t r = 0; r < cp->nrings; r++)
      {
        uint8_t rt = cp->rings[r]->type;
        if (rt != LINETYPE && rt != CIRCSTRINGTYPE && rt != COMPOUNDTYPE)
          return false;
        if (rt == COMPOUNDTYPE && ! geom_clip_supported(cp->rings[r]))
          return false;
      }
      return true;
    }
    default:
      return false;
  }
}

/**
 * @brief Build an R-tree from edges
 */
static RTree *
build_edge_rtree(const Edge *edges, int nedges, int32_t srid)
{
  RTree *rtree = rtree_create_stbox();
  for (int i = 0; i < nedges; i++)
  {
    const Edge *e = &edges[i];
    STBox box;
    stbox_set(true, false, false, srid, e->xmin, e->xmax, e->ymin, e->ymax,
      0, 0, NULL, &box);
    /* Store pointer to edge */
    rtree_insert(rtree, &box, i);
  }
  return rtree;
}

/*****************************************************************************
 * Geometry clip context
 *
 * Everything the engine derives from a geometry -- its bounding box, its edge
 * decomposition, and the R-tree indexing those edges -- depends on that
 * geometry alone. A context keeps that work so that the many operations
 * resolved against one geometry share it, instead of each rebuilding the
 * decomposition and the index. The `_ctx` functions below take a context and
 * the operations named without the suffix build one, use it, and free it, so
 * a single operation costs exactly what it did before.
 *****************************************************************************/

/**
 * @brief Structure keeping the reusable decomposition of a geometry
 */
typedef struct
{
  STBox box;           /**< Bounding box of the geometry */
  int32_t srid;        /**< SRID of the geometry */
  MeosArray *edges;    /**< Edges of the geometry */
  Edge **edge_ptrs;    /**< Pointers to the edges, as the kernels expect them */
  int nedges;          /**< Number of edges */
  RTree *rtree;        /**< Index over the edges, NULL when there are too few
                            of them to amortize its construction */
  Edge **cand_edges;   /**< Buffer receiving the edges selected by the index,
                            NULL when there is no index */
} GeoClipCtx;

/**
 * @brief Return the clip context of a geometry, or NULL if the geometry is
 * empty
 * @details The context owns the edges of the geometry and, when they are
 * numerous enough to amortize its construction, an R-tree indexing them
 * @note At most one context may be alive per thread, since the buffer
 * collecting the results of an index search is the thread-local
 * `rtree_results` shared with the clip kernels, created and destroyed with the
 * index (the same lifetime the operations gave it when each built its own)
 */
void *
geo_clip_ctx_make(const GSERIALIZED *gs)
{
  assert(gs);
  if (gserialized_is_empty(gs))
    return NULL;

  GeoClipCtx *ctx = palloc0(sizeof(GeoClipCtx));
  geo_set_stbox(gs, &ctx->box);
  ctx->srid = gserialized_get_srid(gs);
  /* Extract the edges */
  LWGEOM *geom = lwgeom_from_gserialized(gs);
  ctx->edges = geom_extract_edges(geom);
  lwgeom_free(geom);
  ctx->nedges = (int) ctx->edges->count;
  /* Transform the edge array into an edge pointer array */
  ctx->edge_ptrs = palloc(sizeof(Edge *) * ctx->nedges);
  for (int i = 0; i < ctx->nedges; i++)
    ctx->edge_ptrs[i] = (Edge *) meos_array_get(ctx->edges, i);

  /* Index the edges only when there are enough of them to compensate the
   * overhead of the tree construction and destruction */
  if (ctx->nedges > RTREE_MIN_NUMBER_ELEMS)
  {
    ctx->rtree = build_edge_rtree(ctx->edges->elems, ctx->nedges, ctx->srid);
    if (! ctx->rtree)
    {
      /* Release what the context holds before reporting the error, which may
       * not return control here */
      meos_array_destroy(ctx->edges);
      pfree(ctx->edge_ptrs); pfree(ctx);
      meos_error(ERROR, MEOS_ERR_INTERNAL_ERROR,
        "Error when creating R-tree");
      return NULL;
    }
    ctx->cand_edges = palloc(sizeof(Edge *) * ctx->nedges);
    /* Array for collecting the ids resulting from an R-tree search */
    rtree_results = meos_array_create(sizeof(int));
  }
  return ctx;
}

/**
 * @brief Free a clip context built by #geo_clip_ctx_make
 */
void
geo_clip_ctx_free(void *ctxv)
{
  if (! ctxv)
    return;
  GeoClipCtx *ctx = (GeoClipCtx *) ctxv;
  if (ctx->rtree)
  {
    rtree_free(ctx->rtree);
    pfree(ctx->cand_edges);
    meos_array_destroy(rtree_results);
    rtree_results = NULL;
  }
  meos_array_destroy(ctx->edges);
  pfree(ctx->edge_ptrs);
  pfree(ctx);
  return;
}

/*****************************************************************************/

/**
 * @brief Return true if two geometry edges intersect
 * @details Dispatches on the edge-type pair, reusing the straight-segment
 * (#linesegm_intersect), segment/arc (#arcsegm_intersect), and arc/arc
 * (#arcarc_intersect) primitives. A point edge (#EDGE_POINT) has no extent,
 * so it meets another edge iff it lies on it.
 */
static bool
edge_intersect(const Edge *e1, const Edge *e2)
{
  /* Bounding-box reject */
  if (e1->xmax < e2->xmin - FP_TOLERANCE || e2->xmax < e1->xmin - FP_TOLERANCE ||
      e1->ymax < e2->ymin - FP_TOLERANCE || e2->ymax < e1->ymin - FP_TOLERANCE)
    return false;

  bool arc1 = (e1->etype == EDGE_ARC || e1->etype == EDGE_POLYARC);
  bool arc2 = (e2->etype == EDGE_ARC || e2->etype == EDGE_POLYARC);

  /* A point edge meets another edge only by lying on it */
  if (e1->etype == EDGE_POINT && e2->etype == EDGE_POINT)
    return fabs(e1->x1 - e2->x1) < FP_TOLERANCE &&
      fabs(e1->y1 - e2->y1) < FP_TOLERANCE;
  if (e1->etype == EDGE_POINT)
    return arc2 ? point_on_arc(e1->x1, e1->y1, e2) :
      point_on_segment(e1->x1, e1->y1, e2->x1, e2->y1, e2->x2, e2->y2);
  if (e2->etype == EDGE_POINT)
    return arc1 ? point_on_arc(e2->x1, e2->y1, e1) :
      point_on_segment(e2->x1, e2->y1, e1->x1, e1->y1, e1->x2, e1->y2);

  /* Arc/arc, segment/arc, or segment/segment */
  if (arc1 && arc2)
    return arcarc_intersect(e1, e2);
  if (arc1)
  {
    double out[2];
    return arcsegm_intersect(e2->x1, e2->y1, e2->dx, e2->dy, e1, out) > 0;
  }
  if (arc2)
  {
    double out[2];
    return arcsegm_intersect(e1->x1, e1->y1, e1->dx, e1->dy, e2, out) > 0;
  }
  IntersectResult r = linesegm_intersect(e1->x1, e1->y1, e1->dx, e1->dy,
    e2->x1, e2->y1, e2->x2, e2->y2);
  return r.type != INTERSECT_NONE;
}

/**
 * @brief Return true if the edge array contains a polygon (area) edge
 */
static bool
edges_have_area(Edge **edges, int nedges)
{
  for (int i = 0; i < nedges; i++)
    if (edges[i]->etype == EDGE_POLY || edges[i]->etype == EDGE_POLYARC)
      return true;
  return false;
}

/**
 * @brief Return true if a 2D geometry intersects the geometry of a clip
 * context, computed natively
 * @details Native counterpart of PostGIS `ST_Intersects` for the geometry
 * types the clip engine extracts into edges: two geometries meet when a
 * boundary edge of one crosses a boundary edge of the other, or when a
 * vertex of one lies inside the polygonal interior of the other. Points,
 * (multi)lines, (multi)polygons with holes, triangles, circular strings,
 * curve polygons, and collections of these are supported. The candidate edge
 * pairs are pruned with the R-tree the context keeps over its edges, mirroring
 * #tpoint_linear_inter_geom_ctx. Testing many geometries against one geometry
 * builds that index once, since the context outlives the call
 * @pre The arguments have the same SRID
 */
bool
geo_intersects2d_ctx(const GSERIALIZED *gs, const void *ctxv)
{
  assert(gs); assert(ctxv);
  const GeoClipCtx *ctx = (const GeoClipCtx *) ctxv;
  /* An empty geometry intersects nothing, matching PostGIS ST_Intersects.
   * Callers such as the touches predicates pass the (possibly empty) boundary
   * of a geometry or trajectory, so the leaf must tolerate empty input. */
  if (gserialized_is_empty(gs))
    return false;
  /* Bounding box test */
  STBox box;
  geo_set_stbox(gs, &box);
  if (! overlaps_stbox_stbox(&box, &ctx->box))
    return false;

  /* Extract the edges of the geometry given, those of the context geometry
   * being already extracted and indexed */
  LWGEOM *lw = lwgeom_from_gserialized(gs);
  MeosArray *edges = geom_extract_edges(lw);
  lwgeom_free(lw);
  int n = (int) edges->count;
  Edge **ptr = palloc(sizeof(Edge *) * n);
  for (int i = 0; i < n; i++)
    ptr[i] = (Edge *) meos_array_get(edges, i);

  bool result = false;

  /* Phase 1: a boundary edge of the geometry crosses a boundary edge of the
   * context geometry */
  for (int i = 0; i < n && ! result; i++)
  {
    const Edge *e = ptr[i];
    if (ctx->rtree)
    {
      STBox query;
      stbox_set(true, false, false, ctx->srid, e->xmin, e->xmax, e->ymin,
        e->ymax, 0, 0, NULL, &query);
      int nc = rtree_search(ctx->rtree, RTREE_OVERLAPS, &query, rtree_results);
      for (int j = 0; j < nc; j++)
        ctx->cand_edges[j] =
          ctx->edge_ptrs[*(int *) meos_array_get(rtree_results, j)];
      for (int j = 0; j < nc && ! result; j++)
        if (edge_intersect(e, ctx->cand_edges[j]))
          result = true;
    }
    else
    {
      for (int j = 0; j < ctx->nedges && ! result; j++)
        if (edge_intersect(e, ctx->edge_ptrs[j]))
          result = true;
    }
  }

  /* Phase 2: containment -- a vertex of one geometry inside the other's
   * polygonal interior (only meaningful when the other has area) */
  if (! result && edges_have_area(ctx->edge_ptrs, ctx->nedges))
    for (int i = 0; i < n && ! result; i++)
      if (point_in_polygon_impl(ptr[i]->x1, ptr[i]->y1, ctx->edge_ptrs,
            ctx->nedges, ctx->rtree, ctx->srid, ctx->box.xmax))
        result = true;
  if (! result && edges_have_area(ptr, n))
    for (int i = 0; i < ctx->nedges && ! result; i++)
      if (point_in_polygon(ctx->edge_ptrs[i]->x1, ctx->edge_ptrs[i]->y1, ptr,
            n))
        result = true;

  /* Clean up */
  meos_array_destroy(edges);
  pfree(ptr);
  return result;
}

/**
 * @brief Return true if two 2D geometries intersect, computed natively
 * @details Builds the clip context of the second geometry, which is the one
 * whose edges are indexed, and resolves the relationship with
 * #geo_intersects2d_ctx
 * @pre The arguments have the same SRID
 */
bool
geo_intersects2d(const GSERIALIZED *gs1, const GSERIALIZED *gs2)
{
  assert(gs1); assert(gs2);
  /* An empty geometry intersects nothing, matching PostGIS ST_Intersects.
   * Callers such as the touches predicates pass the (possibly empty) boundary
   * of a geometry or trajectory, so the leaf must tolerate empty input. */
  if (gserialized_is_empty(gs1) || gserialized_is_empty(gs2))
    return false;
  /* Bounding box test, made before extracting the edges of the second
   * geometry so that a rejected pair does not pay for its decomposition */
  STBox box1, box2;
  geo_set_stbox(gs1, &box1);
  geo_set_stbox(gs2, &box2);
  if (! overlaps_stbox_stbox(&box1, &box2))
    return false;

  void *ctx = geo_clip_ctx_make(gs2);
  if (! ctx)
    return false;
  bool result = geo_intersects2d_ctx(gs1, ctx);
  geo_clip_ctx_free(ctx);
  return result;
}

/*****************************************************************************
 * Native planar covers predicate
 *****************************************************************************/

/**
 * @brief Return true if a point lies in the closure (interior or boundary) of
 * the geometry whose edges are given
 * @details A point is in the closure when it lies on any extracted edge -- the
 * 1D extent of a point/line geometry or the boundary of a polygon -- or, when
 * the geometry has an areal component, inside its polygonal interior. The
 * #point_in_polygon test already reports points on a polygon boundary as
 * inside, so the explicit on-edge scan only adds the lower-dimensional
 * (point/line/standalone-arc) closure that the even-odd test ignores.
 */
static bool
edges_contain_point(double px, double py, Edge **edges, int nedges,
  bool has_area)
{
  for (int i = 0; i < nedges; i++)
  {
    const Edge *e = edges[i];
    if (e->etype == EDGE_POINT)
    {
      if (fabs(px - e->x1) < FP_TOLERANCE && fabs(py - e->y1) < FP_TOLERANCE)
        return true;
    }
    else if (e->etype == EDGE_ARC || e->etype == EDGE_POLYARC)
    {
      if (point_on_arc(px, py, e))
        return true;
    }
    else /* EDGE_LINE, EDGE_POLY */
    {
      if (point_on_segment(px, py, e->x1, e->y1, e->x2, e->y2))
        return true;
    }
  }
  if (has_area && point_in_polygon(px, py, edges, nedges))
    return true;
  return false;
}

/**
 * @brief Return true if a straight edge lies entirely within the closure of
 * the geometry whose edges are given
 * @details The edge is split at every crossing with an edge of the covering
 * geometry, and the midpoint of each resulting sub-segment is tested for
 * closure membership. An edge whose endpoints are both in the closure can
 * still leave it between two crossings (through a concavity or a hole), which
 * this per-sub-segment test detects.
 */
static bool
segment_within_closure(const Edge *e, Edge **aedges, int na, bool has_area)
{
  double *ts = palloc(sizeof(double) * (size_t) (2 * na + 2));
  int nt = 0;
  ts[nt++] = 0.0;
  ts[nt++] = 1.0;
  for (int i = 0; i < na; i++)
  {
    const Edge *ea = aedges[i];
    if (ea->etype == EDGE_ARC || ea->etype == EDGE_POLYARC)
    {
      double out[2];
      int m = arcsegm_intersect(e->x1, e->y1, e->dx, e->dy, ea, out);
      for (int k = 0; k < m; k++)
        ts[nt++] = out[k];
    }
    else
    {
      IntersectResult r = linesegm_intersect(e->x1, e->y1, e->dx, e->dy,
        ea->x1, ea->y1, ea->x2, ea->y2);
      if (r.type == INTERSECT_POINT)
        ts[nt++] = r.t0;
      else if (r.type == INTERSECT_OVERLAP)
      {
        ts[nt++] = r.t0;
        ts[nt++] = r.t1;
      }
    }
  }
  qsort(ts, (size_t) nt, sizeof(double), float8_qsort_cmp);
  bool result = true;
  for (int i = 0; i < nt - 1 && result; i++)
  {
    if (ts[i + 1] - ts[i] < FP_TOLERANCE)
      continue;
    double tm = 0.5 * (ts[i] + ts[i + 1]);
    double mx = e->x1 + tm * e->dx, my = e->y1 + tm * e->dy;
    if (! edges_contain_point(mx, my, aedges, na, has_area))
      result = false;
  }
  pfree(ts);
  return result;
}

/**
 * @brief Return true if an arc edge lies entirely within the closure of the
 * geometry whose edges are given
 * @details The arc is sampled at interior angles across its span; each sample
 * must lie in the closure. Endpoints are tested by the caller.
 */
static bool
arc_within_closure(const Edge *e, Edge **aedges, int na, bool has_area)
{
  const int nsamp = 16;
  double sweep = e->ccw ? angle_normalize(e->theta1 - e->theta0) :
    - angle_normalize(e->theta0 - e->theta1);
  for (int k = 1; k < nsamp; k++)
  {
    double phi = e->theta0 + sweep * ((double) k / nsamp);
    double px = e->cx + e->radius * cos(phi);
    double py = e->cy + e->radius * sin(phi);
    if (! edges_contain_point(px, py, aedges, na, has_area))
      return false;
  }
  return true;
}

/**
 * @brief Return true if a geometry is a (multi)point
 */
static bool
geo_is_point(const GSERIALIZED *gs)
{
  int type = gserialized_get_type(gs);
  return type == POINTTYPE || type == MULTIPOINTTYPE;
}

/**
 * @brief Return true if a geometry is a (multi)polygon
 */
static bool
geo_is_poly(const GSERIALIZED *gs)
{
  int type = gserialized_get_type(gs);
  return type == POLYGONTYPE || type == MULTIPOLYGONTYPE;
}

/**
 * @brief Return true if the first 2D geometry covers the second, computed
 * natively
 * @details Geometry A covers geometry B when every point of B lies in the
 * closure of A, that is, B has no point in A's exterior (the DE-9IM
 * `T*****FF*` family). Every vertex of B, and the midpoint of every
 * sub-segment obtained by splitting each edge of B at its crossings with A,
 * must lie in A's closure. Supports the geometry types the clip engine
 * extracts into edges: points, (multi)lines, (multi)polygons with holes,
 * triangles, circular strings, curve polygons, and collections of these.
 * The dispatch mirrors #geom_spatialrel: an empty operand and a (multi)polygon
 * covering a (multi)point are handled by the same native
 * #meos_point_in_polygon short-circuit, so only the general case replaces the
 * GEOS covers call.
 * @pre The arguments have the same SRID
 */
bool
geo_covers2d(const GSERIALIZED *gs1, const GSERIALIZED *gs2)
{
  assert(gs1); assert(gs2);
  /* An empty geometry covers nothing and is covered by nothing, matching
   * PostGIS ST_Covers */
  if (gserialized_is_empty(gs1) || gserialized_is_empty(gs2))
    return false;
  /* Covers is reflexive: every non-empty geometry covers itself (it is a subset
   * of its own closure). Short-circuit byte-identical operands, mirroring
   * #geo_equals. This is exact and FP-free, hence environment-independent —
   * unlike the general edge test, whose point-on-boundary classification sits on
   * a floating-point knife edge for a degenerate self-covering geometry (e.g. an
   * antimeridian-wrapping H3 cell boundary), where -O2 coverage instrumentation
   * can flip the result */
  if (VARSIZE(gs1) == VARSIZE(gs2) && ! memcmp(gs1, gs2, VARSIZE(gs1)))
    return true;
  /* Bounding-box reject: covering requires the 2D boxes to overlap. Covers is a
   * planar (2D) predicate, so the reject must ignore Z; use the same canonical
   * 2D box overlap #geom_spatialrel applies before delegating to GEOS, rather
   * than #overlaps_stbox_stbox which compares Z for a 3D/3D pair and would
   * wrongly reject geometries whose X/Y overlap but whose Z ranges are disjoint */
  GBOX box1, box2;
  memset(&box1, 0, sizeof(GBOX));
  memset(&box2, 0, sizeof(GBOX));
  if (gserialized_get_gbox_p(gs1, &box1) && gserialized_get_gbox_p(gs2, &box2) &&
      gbox_overlaps_2d(&box1, &box2) == LW_FALSE)
    return false;
  /* A (multi)polygon covering a (multi)point is resolved by the native
   * point-in-polygon test, exactly as #geom_spatialrel does before delegating
   * to GEOS. That test answers in the direction of the polygon, so the reverse
   * pair, a (multi)point asked to cover a (multi)polygon, keeps the general
   * path below */
  if (geo_is_poly(gs1) && geo_is_point(gs2))
    return meos_point_in_polygon(gs1, gs2, COVERS);

  /* Extract the edges of both geometries */
  LWGEOM *lw1 = lwgeom_from_gserialized(gs1);
  LWGEOM *lw2 = lwgeom_from_gserialized(gs2);
  MeosArray *edges1 = geom_extract_edges(lw1);
  MeosArray *edges2 = geom_extract_edges(lw2);
  lwgeom_free(lw1); lwgeom_free(lw2);
  int na = (int) edges1->count, nb = (int) edges2->count;
  Edge **aedges = palloc(sizeof(Edge *) * na);
  Edge **bedges = palloc(sizeof(Edge *) * nb);
  for (int i = 0; i < na; i++)
    aedges[i] = (Edge *) meos_array_get(edges1, i);
  for (int i = 0; i < nb; i++)
    bedges[i] = (Edge *) meos_array_get(edges2, i);
  bool has_area = edges_have_area(aedges, na);

  bool result = true;
  /* Every vertex of B must lie in A's closure */
  for (int i = 0; i < nb && result; i++)
  {
    const Edge *e = bedges[i];
    if (! edges_contain_point(e->x1, e->y1, aedges, na, has_area))
      result = false;
    else if (e->etype != EDGE_POINT &&
      ! edges_contain_point(e->x2, e->y2, aedges, na, has_area))
      result = false;
  }
  /* Every edge of B must stay within A's closure */
  for (int i = 0; i < nb && result; i++)
  {
    const Edge *e = bedges[i];
    if (e->etype == EDGE_POINT)
      continue;
    if (e->etype == EDGE_ARC || e->etype == EDGE_POLYARC)
    {
      if (! arc_within_closure(e, aedges, na, has_area))
        result = false;
    }
    else if (! segment_within_closure(e, aedges, na, has_area))
      result = false;
  }

  meos_array_destroy(edges1); meos_array_destroy(edges2);
  pfree(aedges); pfree(bedges);
  return result;
}

/**
 * @brief Return the temporal intersection/intersects of a temporal geometric
 * point with linear interpolation and the geometry of a clip context
 * @details The temporal geometric point may be in 2D or 3D and the Z dimension
 * is also computed. Clipping several temporal points against one geometry
 * extracts and indexes its edges once, since the context outlives the call
 * @note For performance reasons the intersection is computed natively
 * instead of through ST_Intersection
 * @pre The arguments have the same SRID, the geometry is 2D and is not empty.
 * This is verified in #tgeo_restrict_geom
 */
Temporal *
tpoint_linear_inter_geom_ctx(const Temporal *temp, const void *ctxv, bool clip)
{
  assert(temp); assert(ctxv); assert(temp->temptype == T_TGEOMPOINT);
  assert(MEOS_FLAGS_LINEAR_INTERP(temp->flags));
  assert(temp->subtype != TINSTANT);
  assert(! MEOS_FLAGS_GET_GEODETIC(temp->flags));
  const GeoClipCtx *ctx = (const GeoClipCtx *) ctxv;

  /* Bounding box test */
  STBox box1;
  tspatial_set_stbox(temp, &box1);
  if (! overlaps_stbox_stbox(&box1, &ctx->box))
  {
    if (clip)
      return NULL;
    SpanSet *ss = temporal_time(temp);
    Temporal *result = (Temporal *) tsequenceset_from_base_tstzspanset(
      BoolGetDatum(false), T_TBOOL, ss, STEP);
    pfree(ss);
    return result;
  }

  /* Initialize result to NULL to quickly clean up and return */
  Temporal *result = NULL;

  /* Initialize the static global arrays accumulating the clipping results */
  events = meos_array_create(sizeof(double));
  intervals = meos_array_create(sizeof(Span));
  periods = meos_array_create(sizeof(Span));

  /* Collect the clipping periods */
  assert(temptype_subtype(temp->subtype));
  switch (temp->subtype)
  {
    case TINSTANT:
      tpointinst_clip_edges((TInstant *) temp, ctx->edge_ptrs, ctx->nedges,
        ctx->rtree, ctx->cand_edges, ctx->box.xmax);
      break;
    case TSEQUENCE:
      tpointseq_clip_edges((TSequence *) temp, ctx->edge_ptrs, ctx->nedges,
        ctx->rtree, ctx->cand_edges, ctx->box.xmax);
      break;
    default: /* TSEQUENCESET */
    {
      /* Loop for each segment */
      TSequenceSet *ss = (TSequenceSet *) temp;
      for (int i = 0; i < ss->count; i++)
        tpointseq_clip_edges(TSEQUENCESET_SEQ_N(ss, i), ctx->edge_ptrs,
          ctx->nedges, ctx->rtree, ctx->cand_edges, ctx->box.xmax);
    }
  }

  SpanSet *ss;
  if (periods->count == 0)
  {
    if (clip)
      goto cleanup_return;
    ss = temporal_time(temp);
    result = (Temporal *) tsequenceset_from_base_tstzspanset(
      BoolGetDatum(false), T_TBOOL, ss, STEP);
    pfree(ss);
  }
  else
  {
    ss = spanset_make_exp(periods->elems, periods->count,
      periods->count, NORMALIZE, ORDER);
    if (clip)
      result = temporal_restrict_tstzspanset(temp, ss, REST_AT);
    else
    {
      SpanSet *ss1 = temporal_time(temp);
      Temporal *temp1 = (Temporal *) tsequenceset_from_base_tstzspanset(
        BoolGetDatum(false), T_TBOOL, ss1, STEP);
      Temporal *temp2 = temporal_restrict_tstzspanset(temp1, ss, REST_MINUS);
      if (temp2)
      {
        Temporal *temp3 = (Temporal *) tsequenceset_from_base_tstzspanset(
          BoolGetDatum(true), T_TBOOL, ss, STEP);
        result = temporal_merge(temp2, temp3);
        pfree(temp2); pfree(temp3);
      }
      else
        result = (Temporal *) tsequenceset_from_base_tstzspanset(
          BoolGetDatum(true), T_TBOOL, ss1, STEP);
      pfree(ss1); pfree(temp1);
    }
    pfree(ss);
  }
  
  /* Clean up and return */
cleanup_return:
  meos_array_destroy(events); meos_array_destroy(intervals);
  meos_array_destroy(periods);
  return result;
}

/**
 * @ingroup meos_internal_geo
 * @brief Return the temporal intersection/intersects of a temporal geometric
 * point with linear interpolation and a 2D geometry
 * @details Builds the clip context of the geometry and resolves the
 * relationship with #tpoint_linear_inter_geom_ctx
 * @pre The arguments have the same SRID, the geometry is 2D and is not empty.
 * This is verified in #tgeo_restrict_geom
 */
Temporal *
tpoint_linear_inter_geom(const Temporal *temp, const GSERIALIZED *gs,
  bool clip)
{
  assert(temp); assert(gs); assert(! gserialized_is_empty(gs));
  void *ctx = geo_clip_ctx_make(gs);
  if (! ctx)
    return NULL;
  Temporal *result = tpoint_linear_inter_geom_ctx(temp, ctx, clip);
  geo_clip_ctx_free(ctx);
  return result;
}

/*****************************************************************************
 * Within-distance (tDwithin / ever-always dwithin) native engine
 *
 * Distance-threshold sibling of the exact intersection engine above. The
 * within region of a geometry at distance @p dist is its Minkowski sum with a
 * closed disc of radius @p dist: a capsule around each segment, a disc around
 * each point, an annular sector around each arc, and the filled polygon
 * dilated by @p dist. For each moving-point segment the candidate boundary
 * crossing times are solved in closed form per edge (the roots of
 * dist(seg(t), edge) = dist), then each sub-interval is classified by the
 * exact interior-aware unit distance at its midpoint. This mirrors the
 * within-roots + midpoint-classification spanset assembler of the merged
 * temporal circular-buffer engine (tcbuffer_distance.c) specialized to a
 * moving point, i.e. a moving disc with radius r(t) = 0.
 *
 * A zero distance is exactly the temporal intersects relationship and is
 * delegated to #tpoint_linear_inter_geom so that tDwithin(., ., 0) is
 * bit-identical to tIntersects (including isolated contact instants, which are
 * measure-zero and therefore dropped by the positive-distance midpoint
 * classification, exactly as in the temporal circular-buffer engine).
 *****************************************************************************/

/**
 * @brief Return the squared distance from a point to a segment
 */
static double
point_seg_dist2(double px, double py, double x1, double y1, double x2,
  double y2)
{
  const double ux = x2 - x1, uy = y2 - y1;
  const double l2 = ux * ux + uy * uy;
  if (l2 < FP_TOLERANCE)
  {
    const double dx = px - x1, dy = py - y1;
    return dx * dx + dy * dy;
  }
  double s = ((px - x1) * ux + (py - y1) * uy) / l2;
  if (s < 0.0) s = 0.0; else if (s > 1.0) s = 1.0;
  const double qx = x1 + s * ux, qy = y1 + s * uy;
  const double dx = px - qx, dy = py - qy;
  return dx * dx + dy * dy;
}

/**
 * @brief Return the squared distance from a point to an arc edge
 * @details When the point projects within the arc's angular span the distance
 * is the difference to the supporting circle, otherwise it is the distance to
 * the nearer arc endpoint
 */
static double
point_arc_dist2(double px, double py, const Edge *e)
{
  const double dxc = px - e->cx, dyc = py - e->cy;
  const double dc = hypot(dxc, dyc);
  if (arc_contains_angle(e, atan2(dyc, dxc)))
  {
    const double dd = dc - e->radius;
    return dd * dd;
  }
  const double d0x = px - e->x1, d0y = py - e->y1;
  const double d1x = px - e->x2, d1y = py - e->y2;
  const double d0 = d0x * d0x + d0y * d0y;
  const double d1 = d1x * d1x + d1y * d1y;
  return FP_MIN(d0, d1);
}

/**
 * @brief Return the squared distance from a point to a single edge
 */
static double
point_edge_dist2(double px, double py, const Edge *e)
{
  switch (e->etype)
  {
    case EDGE_POINT:
    {
      const double dx = px - e->x1, dy = py - e->y1;
      return dx * dx + dy * dy;
    }
    case EDGE_LINE:
    case EDGE_POLY:
      return point_seg_dist2(px, py, e->x1, e->y1, e->x2, e->y2);
    default: /* EDGE_ARC / EDGE_POLYARC */
      return point_arc_dist2(px, py, e);
  }
}

/**
 * @brief Return true if a point is within @p dist of the geometry, taking the
 * polygon interior into account (a point inside a polygon is at distance 0),
 * pruning both tests with an R-tree over the edges when one is given
 * @details Only an edge whose bounding box meets the square of side 2 * @p dist
 * centred on the point can be within that distance of it; querying the R-tree
 * for those instead of scanning every edge turns the O(nedges) test into
 * O(log nedges + candidates). An excluded edge is farther than @p dist from the
 * point along one axis alone, so it cannot satisfy the distance test and the
 * result is identical to the full scan. The interior test inherits the same
 * pruning from #point_in_polygon_impl
 */
static bool
point_geom_within(double px, double py, Edge **edges, int nedges,
  double dist, const RTree *rtree, int32_t srid, double xmax)
{
  const double d2 = dist * dist;
  if (rtree)
  {
    STBox query;
    stbox_set(true, false, false, srid, px - dist, px + dist, py - dist,
      py + dist, 0, 0, NULL, &query);
    int nc = rtree_search(rtree, RTREE_OVERLAPS, &query, rtree_results);
    for (int i = 0; i < nc; i++)
      if (point_edge_dist2(px, py,
            edges[*(int *) meos_array_get(rtree_results, i)]) <=
          d2 + FP_TOLERANCE)
        return true;
  }
  else
  {
    for (int i = 0; i < nedges; i++)
      if (point_edge_dist2(px, py, edges[i]) <= d2 + FP_TOLERANCE)
        return true;
  }
  return point_in_polygon_impl(px, py, edges, nedges, rtree, srid, xmax) ?
    true : false;
}

/**
 * @brief Append a candidate crossing time to the event array if it lies in
 * [0,1] (clamping tiny out-of-range values to the endpoints)
 */
static void
add_within_root(double t, MeosArray *ev)
{
  if (t > -FP_TOLERANCE && t < 1.0 + FP_TOLERANCE)
  {
    if (t < 0.0) t = 0.0; else if (t > 1.0) t = 1.0;
    meos_array_add(ev, &t);
  }
}

/**
 * @brief Append the [0,1] roots of the quadratic @p A t^2 + @p B t + @p C to
 * the event array
 */
static void
add_within_quad_roots(double A, double B, double C, MeosArray *ev)
{
  if (fabs(A) < FP_TOLERANCE)
  {
    if (fabs(B) > FP_TOLERANCE)
      add_within_root(-C / B, ev);
    return;
  }
  const double disc = B * B - 4.0 * A * C;
  if (disc < 0.0)
    return;
  const double sq = sqrt(disc);
  add_within_root((-B - sq) / (2.0 * A), ev);
  add_within_root((-B + sq) / (2.0 * A), ev);
}

/**
 * @brief Append to @p ev the trajectory-segment times at which the moving
 * point crosses the distance-@p dist boundary of one edge
 * @details The boundary of the edge's within region is composed of: for a
 * point, the disc of radius @p dist; for a segment, the two endpoint caps and
 * the two parallel offset lines; for an arc, the inner/outer offset circles
 * and the two endpoint caps. The candidate set is a superset (offset lines are
 * infinite, offset circles ignore the angular span); spurious candidates are
 * filtered out by the exact midpoint distance classification
 */
static void
within_roots_from_edge(double ax, double ay, double rx, double ry,
  const Edge *e, double dist, MeosArray *ev)
{
  const double A = rx * rx + ry * ry;
  const double d2 = dist * dist;
  switch (e->etype)
  {
    case EDGE_POINT:
    {
      const double wx = ax - e->x1, wy = ay - e->y1;
      add_within_quad_roots(A, 2.0 * (wx * rx + wy * ry),
        wx * wx + wy * wy - d2, ev);
      return;
    }
    case EDGE_LINE:
    case EDGE_POLY:
    {
      /* Endpoint caps: discs of radius dist around each segment endpoint */
      const double w0x = ax - e->x1, w0y = ay - e->y1;
      add_within_quad_roots(A, 2.0 * (w0x * rx + w0y * ry),
        w0x * w0x + w0y * w0y - d2, ev);
      const double w1x = ax - e->x2, w1y = ay - e->y2;
      add_within_quad_roots(A, 2.0 * (w1x * rx + w1y * ry),
        w1x * w1x + w1y * w1y - d2, ev);
      /* Parallel offset lines at distance dist on both sides. The signed
       * perpendicular distance is (k0 + t k1) / sqrt(l2) */
      const double ux = e->x2 - e->x1, uy = e->y2 - e->y1;
      const double l2 = ux * ux + uy * uy;
      if (l2 > FP_TOLERANCE)
      {
        const double k0 = w0x * uy - w0y * ux;
        const double k1 = rx * uy - ry * ux;
        if (fabs(k1) > FP_TOLERANCE)
        {
          const double off = dist * sqrt(l2);
          add_within_root((off - k0) / k1, ev);
          add_within_root((-off - k0) / k1, ev);
        }
      }
      return;
    }
    default: /* EDGE_ARC / EDGE_POLYARC */
    {
      const double wx = ax - e->cx, wy = ay - e->cy;
      const double B = 2.0 * (wx * rx + wy * ry);
      const double C0 = wx * wx + wy * wy;
      const double ro = e->radius + dist;
      add_within_quad_roots(A, B, C0 - ro * ro, ev);
      const double ri = e->radius - dist;
      if (ri > 0.0)
        add_within_quad_roots(A, B, C0 - ri * ri, ev);
      /* Endpoint caps: discs of radius dist around each arc endpoint */
      const double e0x = ax - e->x1, e0y = ay - e->y1;
      add_within_quad_roots(A, 2.0 * (e0x * rx + e0y * ry),
        e0x * e0x + e0y * e0y - d2, ev);
      const double e1x = ax - e->x2, e1y = ay - e->y2;
      add_within_quad_roots(A, 2.0 * (e1x * rx + e1y * ry),
        e1x * e1x + e1y * e1y - d2, ev);
      return;
    }
  }
}

/**
 * @brief Collect into the interval array the [0,1] sub-intervals of one
 * trajectory segment along which the moving point is within @p dist of the
 * geometry
 * @param[in] a,b Endpoints of the trajectory segment
 * @param[in] sel_edges,sel_nedges Edges to gather crossing candidates from
 * (possibly R-tree filtered)
 * @param[in] all_edges,all_nedges Full edge array, used for the interior-aware
 * midpoint classification (the polygon ray-cast needs every edge)
 * @param[in] dist Distance threshold
 * @param[in] rtree R-tree over @p all_edges, or NULL to scan them all
 * @param[in] srid,xmax SRID and geometry bounding-box maximum abscissa, used
 * to query @p rtree
 */
static void
intervals_within_edges(const POINT2D *a, const POINT2D *b, Edge **sel_edges,
  int sel_nedges, Edge **all_edges, int all_nedges, double dist,
  const RTree *rtree, int32_t srid, double xmax)
{
  events->count = 0;
  const double ax = a->x, ay = a->y;
  const double rx = b->x - ax, ry = b->y - ay;
  const double seg_xmin = FP_MIN(a->x, b->x), seg_xmax = FP_MAX(a->x, b->x);
  const double seg_ymin = FP_MIN(a->y, b->y), seg_ymax = FP_MAX(a->y, b->y);

  /* Gather boundary crossing candidates from the (filtered) edges */
  for (int i = 0; i < sel_nedges; i++)
  {
    const Edge *e = sel_edges[i];
    /* Bounding-box filter expanded by dist: the moving point may be within
     * dist of an edge whose own box does not overlap the segment box */
    if (e->xmax + dist < seg_xmin || e->xmin - dist > seg_xmax ||
        e->ymax + dist < seg_ymin || e->ymin - dist > seg_ymax)
      continue;
    within_roots_from_edge(ax, ay, rx, ry, e, dist, events);
  }
  /* Add the segment endpoints */
  double t0 = 0.0, t1 = 1.0;
  meos_array_add(events, &t0);
  meos_array_add(events, &t1);

  /* Sort and deduplicate the candidates */
  qsort(events->elems, events->count, sizeof(double), float8_qsort_cmp);
  int newcount = 0;
  double *ev = (double *) events->elems;
  for (int i = 0; i < (int) events->count; i++)
    if (i == 0 || fabs(ev[i] - ev[newcount - 1]) > FP_TOLERANCE)
      ev[newcount++] = ev[i];
  events->count = newcount;

  /* Keep each sub-interval whose midpoint is within dist of the geometry */
  for (int i = 0; i < (int) events->count - 1; i++)
  {
    const double ta = ev[i], tb = ev[i + 1];
    if (tb - ta <= FP_TOLERANCE)
      continue;
    const double tm = 0.5 * (ta + tb);
    const double x = ax + tm * rx, y = ay + tm * ry;
    if (point_geom_within(x, y, all_edges, all_nedges, dist, rtree,
          srid, xmax))
    {
      Span in;
      span_set(Float8GetDatum(ta), Float8GetDatum(tb), true, true,
        T_FLOAT8, T_FLOATSPAN, &in);
      meos_array_add(intervals, &in);
    }
  }

  /* Isolated within instants: a trajectory that only grazes the distance
   * boundary tangentially touches the within region at a single time (a double
   * root, where the distance equals dist exactly) which the midpoint test
   * above cannot see. Emit a degenerate interval for each candidate time that
   * is within dist (inclusive). The span normalization absorbs the ones that
   * coincide with an interval endpoint, leaving only the genuine isolated
   * touches. This is what keeps the distance-inclusive semantics exact and, at
   * a zero distance, matches the isolated contact points of the intersection
   * engine (which the zero-distance path delegates to anyway). */
  for (int i = 0; i < (int) events->count; i++)
  {
    const double t = ev[i];
    const double x = ax + t * rx, y = ay + t * ry;
    if (point_geom_within(x, y, all_edges, all_nedges, dist, rtree,
          srid, xmax))
    {
      Span in;
      span_set(Float8GetDatum(t), Float8GetDatum(t), true, true,
        T_FLOAT8, T_FLOATSPAN, &in);
      meos_array_add(intervals, &in);
    }
  }
}

/**
 * @brief Add the within-distance instantaneous period of a temporal instant
 * point to the period array
 */
static void
tpointinst_dwithin_edges(const TInstant *inst, Edge **edges, int nedges,
  double dist, const RTree *rtree, int32_t srid, double xmax)
{
  assert(inst); assert(edges); assert(nedges > 0);
  assert(inst->temptype == T_TGEOMPOINT);
  const POINT2D *a = DATUM_POINT2D_P(tinstant_value_p(inst));
  if (! point_geom_within(a->x, a->y, edges, nedges, dist, rtree, srid, xmax))
    return;
  Span s;
  span_set(TimestampTzGetDatum(inst->t), TimestampTzGetDatum(inst->t),
    true, true, T_TIMESTAMPTZ, T_TSTZSPAN, &s);
  meos_array_add(periods, &s);
}

/**
 * @brief Add to the period array the sub-periods of a temporal sequence point
 * with linear interpolation during which it is within @p dist of a geometry
 */
static void
tpointseq_dwithin_edges(const TSequence *seq, Edge **edges, int nedges,
  const RTree *rtree, Edge **cand_edges, double dist, double xmax)
{
  assert(seq); assert(edges); assert(nedges > 0);
  assert(seq->temptype == T_TGEOMPOINT);
  assert(MEOS_FLAGS_LINEAR_INTERP(seq->flags));

  /* Singleton sequence */
  if (seq->count == 1)
    return tpointinst_dwithin_edges(TSEQUENCE_INST_N(seq, 0), edges, nedges,
      dist, rtree, tspatial_srid((Temporal *) seq), xmax);

  bool use_index = (rtree != NULL && cand_edges != NULL);
  int32_t srid = tspatial_srid((Temporal *) seq);
  const TInstant *inst1 = TSEQUENCE_INST_N(seq, 0);
  const POINT2D *a = DATUM_POINT2D_P(tinstant_value_p(inst1));
  bool lower_inc = seq->period.lower_inc;
  Edge **sel_edges = edges;
  int sel_nedges = nedges;
  /* Loop for each segment */
  for (int i = 1; i < seq->count; i++)
  {
    const TInstant *inst2 = TSEQUENCE_INST_N(seq, i);
    const POINT2D *b = DATUM_POINT2D_P(tinstant_value_p(inst2));
    bool upper_inc = (i < seq->count - 1) ? false : seq->period.upper_inc;

    /* Filter the edges by an R-tree, expanding the query box by dist */
    if (use_index)
    {
      STBox query;
      stbox_set(true, false, false, srid, FP_MIN(a->x, b->x) - dist,
        FP_MAX(a->x, b->x) + dist, FP_MIN(a->y, b->y) - dist,
        FP_MAX(a->y, b->y) + dist, 0, 0, NULL, &query);
      int cand_nedges = rtree_search(rtree, RTREE_OVERLAPS, &query,
        rtree_results);
      for (int j = 0; j < cand_nedges; j++)
        cand_edges[j] = edges[*(int *) meos_array_get(rtree_results, j)];
      sel_edges = cand_edges;
      sel_nedges = cand_nedges;
    }

    /* Reset and compute the within intervals for this segment */
    intervals->count = 0;
    intervals_within_edges(a, b, sel_edges, sel_nedges, edges, nedges, dist,
      rtree, srid, xmax);
    /* The array is declared before the jump below, which would otherwise skip
     * its initializer while `next_segment` reads it */
    Span *intervarr = NULL;
    if (intervals->count == 0)
      goto next_segment;

    /* Normalize the intervals (sort: the midpoint intervals and the isolated
     * within points are appended in two separate passes, not globally sorted) */
    int count;
    if (intervals->count > 1)
      intervarr = spanarr_normalize(intervals->elems, intervals->count,
        ORDER, &count);
    else
    {
      intervarr = intervals->elems;
      count = 1;
    }

    /* Generate the periods from the float spans taking into account exclusive
     * temporal bounds */
    double duration = (double) (inst2->t - inst1->t);
    for (int j = 0; j < count; j++)
    {
      Span s;
      double lower = DatumGetFloat8(intervarr[j].lower);
      double upper = DatumGetFloat8(intervarr[j].upper);
      if (fabs(upper - lower) < FP_TOLERANCE)
      {
        /* Remove within points on exclusive lower and upper bounds */
        if (! lower_inc && fabs(lower) < FP_TOLERANCE &&
            fabs(upper) < FP_TOLERANCE)
          continue;
        if (! upper_inc && fabs(lower - 1.0) < FP_TOLERANCE &&
            fabs(upper - 1.0) < FP_TOLERANCE)
          continue;
        TimestampTz t = (lower == 0.0) ?
          inst1->t : inst1->t + (TimestampTz) (duration * lower);
        span_set(TimestampTzGetDatum(t), TimestampTzGetDatum(t), true, true,
          T_TIMESTAMPTZ, T_TSTZSPAN, &s);
        meos_array_add(periods, &s);
      }
      else
      {
        TimestampTz t1 = (lower == 0.0) ?
          inst1->t : inst1->t + (TimestampTz) (duration * lower);
        TimestampTz t2 = (upper == 1.0) ?
          inst2->t : inst1->t + (TimestampTz) (duration * upper);
        span_set(TimestampTzGetDatum(t1), TimestampTzGetDatum(t2), true, true,
          T_TIMESTAMPTZ, T_TSTZSPAN, &s);
        meos_array_add(periods, &s);
      }
    }

next_segment:
    if (intervarr && intervals->count > 1)
      pfree(intervarr);
    inst1 = inst2;
    a = b;
  }
  return;
}

/**
 * @ingroup meos_internal_geo
 * @brief Return a temporal Boolean that states whether a temporal geometric
 * point with linear interpolation is within a distance of a 2D geometry
 * @details Native counterpart of the polygonal-buffer approximation:
 * for a zero distance it is exactly #tpoint_linear_inter_geom_ctx
 * (tIntersects), otherwise it solves the per-segment within-distance
 * sub-intervals in closed form. The result is a temporal Boolean defined over
 * the whole time of the temporal point. Testing several temporal points
 * against one geometry extracts and indexes its edges once, since the context
 * outlives the call
 * @pre The arguments have the same SRID, are 2D and planar, and the geometry
 * is not empty and is supported by the clip engine. This is verified by the
 * caller
 */
Temporal *
tpoint_linear_dwithin_geom_ctx(const Temporal *temp, const void *ctxv,
  double dist)
{
  assert(temp); assert(ctxv); assert(temp->temptype == T_TGEOMPOINT);
  assert(MEOS_FLAGS_LINEAR_INTERP(temp->flags));
  assert(temp->subtype != TINSTANT);
  assert(! MEOS_FLAGS_GET_GEODETIC(temp->flags));
  const GeoClipCtx *ctx = (const GeoClipCtx *) ctxv;

  /* A zero distance is exactly the temporal intersects relationship */
  if (dist <= 0.0)
    return tpoint_linear_inter_geom_ctx(temp, ctxv, false);

  /* Bounding box test: the geometry box expanded by dist must overlap the
   * temporal point box, otherwise the relationship is false throughout */
  STBox box1, box2e;
  tspatial_set_stbox(temp, &box1);
  stbox_expand_space_set(&ctx->box, dist, &box2e);
  bool overlap = overlaps_stbox_stbox(&box1, &box2e);
  if (! overlap)
  {
    SpanSet *ss = temporal_time(temp);
    Temporal *result = (Temporal *) tsequenceset_from_base_tstzspanset(
      BoolGetDatum(false), T_TBOOL, ss, STEP);
    pfree(ss);
    return result;
  }

  /* Initialize the static global arrays accumulating the results */
  events = meos_array_create(sizeof(double));
  intervals = meos_array_create(sizeof(Span));
  periods = meos_array_create(sizeof(Span));

  /* Collect the within-distance periods */
  assert(temptype_subtype(temp->subtype));
  switch (temp->subtype)
  {
    case TSEQUENCE:
      tpointseq_dwithin_edges((TSequence *) temp, ctx->edge_ptrs, ctx->nedges,
        ctx->rtree, ctx->cand_edges, dist, ctx->box.xmax);
      break;
    default: /* TSEQUENCESET */
    {
      TSequenceSet *ss = (TSequenceSet *) temp;
      for (int i = 0; i < ss->count; i++)
        tpointseq_dwithin_edges(TSEQUENCESET_SEQ_N(ss, i), ctx->edge_ptrs,
          ctx->nedges, ctx->rtree, ctx->cand_edges, dist, ctx->box.xmax);
    }
  }

  /* Assemble the temporal Boolean over the whole time of the temporal point */
  Temporal *result;
  if (periods->count == 0)
  {
    SpanSet *ss = temporal_time(temp);
    result = (Temporal *) tsequenceset_from_base_tstzspanset(
      BoolGetDatum(false), T_TBOOL, ss, STEP);
    pfree(ss);
  }
  else
  {
    SpanSet *ss = spanset_make_exp(periods->elems, periods->count,
      periods->count, NORMALIZE, ORDER);
    SpanSet *ss1 = temporal_time(temp);
    Temporal *temp1 = (Temporal *) tsequenceset_from_base_tstzspanset(
      BoolGetDatum(false), T_TBOOL, ss1, STEP);
    Temporal *temp2 = temporal_restrict_tstzspanset(temp1, ss, REST_MINUS);
    if (temp2)
    {
      Temporal *temp3 = (Temporal *) tsequenceset_from_base_tstzspanset(
        BoolGetDatum(true), T_TBOOL, ss, STEP);
      result = temporal_merge(temp2, temp3);
      pfree(temp2); pfree(temp3);
    }
    else
      result = (Temporal *) tsequenceset_from_base_tstzspanset(
        BoolGetDatum(true), T_TBOOL, ss1, STEP);
    pfree(ss1); pfree(temp1); pfree(ss);
  }

  /* Clean up and return */
  meos_array_destroy(events); meos_array_destroy(intervals);
  meos_array_destroy(periods);
  return result;
}

/**
 * @ingroup meos_internal_geo
 * @brief Return a temporal Boolean that states whether a temporal geometric
 * point with linear interpolation is within a distance of a 2D geometry
 * @details Builds the clip context of the geometry and resolves the
 * relationship with #tpoint_linear_dwithin_geom_ctx
 * @pre The arguments have the same SRID, are 2D and planar, and the geometry
 * is not empty and is supported by the clip engine. This is verified by the
 * caller
 */
Temporal *
tpoint_linear_dwithin_geom(const Temporal *temp, const GSERIALIZED *gs,
  double dist)
{
  assert(temp); assert(gs); assert(! gserialized_is_empty(gs));
  void *ctx = geo_clip_ctx_make(gs);
  if (! ctx)
    return NULL;
  Temporal *result = tpoint_linear_dwithin_geom_ctx(temp, ctx, dist);
  geo_clip_ctx_free(ctx);
  return result;
}

/*****************************************************************************
 * Temporal distance (tDistance) native engine
 *
 * Distance-value sibling of the within-distance engine above. It produces the
 * temporal float distance from a moving point to a whole (possibly curved) 2D
 * geometry, lifting the point-operand-only restriction of the generic lifting
 * path (whose per-segment turning-point function can only represent the
 * distance to a single static point, i.e. at most one interior extremum).
 *
 * For each trajectory segment the distance to the geometry is the pointwise
 * minimum, over all edges, of the exact point-to-edge distance. Its turning
 * points are the union of the per-edge critical times: the perpendicular-foot
 * and endpoint-closest-approach times of a straight edge, the radial extremum
 * and angular-sector crossing times of an arc edge, and the region-boundary
 * times where the nearest feature of an edge changes. At every such time the
 * exact distance to the whole geometry is emitted as a temporal float instant,
 * with linear interpolation in between, exactly as the point-to-point temporal
 * distance samples its analytic turning points. The global minimum of the
 * distance over a segment is min over edges of the per-edge minimum over the
 * segment (the two minimisations commute), so emitting every per-edge extremum
 * makes minValue exact.
 *****************************************************************************/

/**
 * @brief Return the exact distance from a point to the whole geometry, taking
 * the polygon interior into account (a point inside a filled polygon is at
 * distance zero)
 */
static double
point_geom_dist(double px, double py, Edge **edges, int nedges)
{
  double best = point_edge_dist2(px, py, edges[0]);
  for (int i = 1; i < nedges; i++)
  {
    const double d2 = point_edge_dist2(px, py, edges[i]);
    if (d2 < best)
      best = d2;
  }
  /* On (or numerically on) the boundary: distance is zero */
  if (best <= FP_TOLERANCE)
    return 0.0;
  /* Strictly inside a filled polygon: distance is zero. The horizontal-ray
   * even-odd test of #point_in_polygon miscounts when the query height aligns
   * exactly with a vertex or an arc junction, which the turning-point sampler
   * can hit deterministically. Take the majority vote of the test at the point
   * and at two tiny vertical nudges that move the ray off any aligned junction;
   * the nudge is far below any real feature size so a strictly interior or
   * strictly exterior point is unaffected */
  const double eps = 1e-9 * FP_MAX(1.0, fabs(py));
  int inside = point_in_polygon(px, py, edges, nedges) +
    point_in_polygon(px, py + eps, edges, nedges) +
    point_in_polygon(px, py - eps, edges, nedges);
  return (inside >= 2) ? 0.0 : sqrt(best);
}

/**
 * @brief Append to the event array the [0,1] trajectory-segment critical times
 * of the distance from the moving point to one edge
 * @details The candidates are the local extrema and nearest-feature switch
 * times of the exact point-to-edge distance: for a point the single closest
 * approach; for a straight edge the perpendicular-foot time, the two
 * endpoint-closest-approach times, and the two foot-parameter region
 * boundaries; for an arc edge the radial extremum, the supporting-circle
 * crossings (where the distance reaches its zero minimum), the two
 * endpoint-closest approaches, and the two angular-sector boundary crossings.
 * Spurious candidates are harmless because the distance value emitted at each
 * time is the exact distance to the whole geometry
 */
static void
distance_cands_from_edge(double ax, double ay, double rx, double ry,
  const Edge *e, MeosArray *ev)
{
  const double A = rx * rx + ry * ry;
  /* Constant (zero-length) trajectory segment: no interior turning point */
  if (A < FP_TOLERANCE)
    return;
  switch (e->etype)
  {
    case EDGE_POINT:
    {
      const double wx = ax - e->x1, wy = ay - e->y1;
      add_within_root(-(wx * rx + wy * ry) / A, ev);
      return;
    }
    case EDGE_LINE:
    case EDGE_POLY:
    {
      const double w0x = ax - e->x1, w0y = ay - e->y1;
      const double w1x = ax - e->x2, w1y = ay - e->y2;
      /* Closest approach to each segment endpoint */
      add_within_root(-(w0x * rx + w0y * ry) / A, ev);
      add_within_root(-(w1x * rx + w1y * ry) / A, ev);
      const double ux = e->x2 - e->x1, uy = e->y2 - e->y1;
      const double l2 = ux * ux + uy * uy;
      if (l2 > FP_TOLERANCE)
      {
        /* Perpendicular-foot time (moving point on the supporting line) */
        const double k1 = rx * uy - ry * ux;
        if (fabs(k1) > FP_TOLERANCE)
          add_within_root(-(w0x * uy - w0y * ux) / k1, ev);
        /* Foot-parameter region boundaries (s = 0 and s = 1) */
        const double ru = rx * ux + ry * uy;
        if (fabs(ru) > FP_TOLERANCE)
        {
          const double w0u = w0x * ux + w0y * uy;
          add_within_root(-w0u / ru, ev);
          add_within_root((l2 - w0u) / ru, ev);
        }
      }
      return;
    }
    default: /* EDGE_ARC / EDGE_POLYARC */
    {
      const double wx = ax - e->cx, wy = ay - e->cy;
      /* Radial extremum: the time at which || P(t) - center || is stationary
       * (the distance-to-arc minimum when the segment stays on one side of the
       * supporting circle) */
      add_within_root(-(wx * rx + wy * ry) / A, ev);
      /* Supporting-circle crossings, where the distance to the arc reaches its
       * zero minimum (a kink not seen by the radial extremum): the roots of
       * || P(t) - center ||^2 = radius^2 */
      add_within_quad_roots(A, 2.0 * (wx * rx + wy * ry),
        wx * wx + wy * wy - e->radius * e->radius, ev);
      /* Closest approach to each arc endpoint */
      const double w0x = ax - e->x1, w0y = ay - e->y1;
      const double w1x = ax - e->x2, w1y = ay - e->y2;
      add_within_root(-(w0x * rx + w0y * ry) / A, ev);
      add_within_root(-(w1x * rx + w1y * ry) / A, ev);
      /* Angular-sector boundary crossings (rays from the center through the
       * arc endpoints) */
      const double d0x = e->x1 - e->cx, d0y = e->y1 - e->cy;
      const double den0 = rx * d0y - ry * d0x;
      if (fabs(den0) > FP_TOLERANCE)
        add_within_root(-(wx * d0y - wy * d0x) / den0, ev);
      const double d1x = e->x2 - e->cx, d1y = e->y2 - e->cy;
      const double den1 = rx * d1y - ry * d1x;
      if (fabs(den1) > FP_TOLERANCE)
        add_within_root(-(wx * d1y - wy * d1x) / den1, ev);
      return;
    }
  }
}

/**
 * @brief Return the temporal float distance of one temporal sequence point
 * with linear interpolation to a geometry given as an edge array
 */
static TSequence *
tpointseq_distance_geom(const TSequence *seq, Edge **edges, int nedges)
{
  assert(seq); assert(edges); assert(nedges > 0);
  assert(seq->temptype == T_TGEOMPOINT);
  assert(MEOS_FLAGS_LINEAR_INTERP(seq->flags));

  /* Singleton sequence */
  if (seq->count == 1)
  {
    const TInstant *inst = TSEQUENCE_INST_N(seq, 0);
    const POINT2D *p = DATUM_POINT2D_P(tinstant_value_p(inst));
    double d = point_geom_dist(p->x, p->y, edges, nedges);
    TInstant *resinst = tinstant_make(Float8GetDatum(d), T_TFLOAT, inst->t);
    TSequence *res = tsequence_make(&resinst, 1, true, true, LINEAR, NORMALIZE);
    pfree(resinst);
    return res;
  }

  /* Upper bound on the number of result instants: the two endpoints of every
   * segment plus up to six interior turning points per edge and per segment */
  int maxinsts = 1 + (seq->count - 1) * (nedges * 6 + 3);
  TInstant **instants = palloc(sizeof(TInstant *) * maxinsts);
  int ninsts = 0;
  const TInstant *inst1 = TSEQUENCE_INST_N(seq, 0);
  const POINT2D *a = DATUM_POINT2D_P(tinstant_value_p(inst1));
  instants[ninsts++] = tinstant_make(
    Float8GetDatum(point_geom_dist(a->x, a->y, edges, nedges)), T_TFLOAT,
    inst1->t);
  /* Loop for each segment */
  for (int i = 1; i < seq->count; i++)
  {
    const TInstant *inst2 = TSEQUENCE_INST_N(seq, i);
    const POINT2D *b = DATUM_POINT2D_P(tinstant_value_p(inst2));
    const double ax = a->x, ay = a->y, rx = b->x - ax, ry = b->y - ay;

    /* Gather the interior turning points of the distance to every edge */
    events->count = 0;
    for (int j = 0; j < nedges; j++)
      distance_cands_from_edge(ax, ay, rx, ry, edges[j], events);

    /* Sort the candidate parameters and emit an instant for each interior one
     * with the exact distance to the whole geometry */
    qsort(events->elems, events->count, sizeof(double), float8_qsort_cmp);
    const double *ev = (double *) events->elems;
    const double duration = (double) (inst2->t - inst1->t);
    TimestampTz prevt = inst1->t;
    for (int k = 0; k < (int) events->count; k++)
    {
      const double p = ev[k];
      if (p <= FP_TOLERANCE || p >= 1.0 - FP_TOLERANCE)
        continue;
      if (k > 0 && fabs(p - ev[k - 1]) < FP_TOLERANCE)
        continue;
      TimestampTz t = inst1->t + (TimestampTz) (duration * p);
      /* Keep the instants strictly increasing and off the segment endpoints */
      if (t <= prevt || t >= inst2->t)
        continue;
      const double x = ax + p * rx, y = ay + p * ry;
      instants[ninsts++] = tinstant_make(
        Float8GetDatum(point_geom_dist(x, y, edges, nedges)), T_TFLOAT, t);
      prevt = t;
    }
    /* End instant of the segment */
    instants[ninsts++] = tinstant_make(
      Float8GetDatum(point_geom_dist(b->x, b->y, edges, nedges)), T_TFLOAT,
      inst2->t);
    inst1 = inst2;
    a = b;
  }

  return tsequence_make_free(instants, ninsts, seq->period.lower_inc,
    seq->period.upper_inc, LINEAR, NORMALIZE);
}

/**
 * @ingroup meos_internal_geo
 * @brief Return the temporal float distance between a temporal geometric point
 * with linear interpolation and a 2D geometry
 * @details Native counterpart of the generic distance lifting for a
 * non-point geometry operand: the distance to a multi-edge or curved target
 * has an arbitrary number of turning points per segment which the point-only
 * base turning-point function cannot represent. The result is a temporal float
 * with linear interpolation whose values at the analytic turning points and at
 * the trajectory instants are the exact distance to the geometry
 * @pre The arguments have the same SRID, are 2D and planar, and the geometry
 * is not empty and is supported by the clip engine. This is verified by the
 * caller
 */
Temporal *
tpoint_linear_distance_geom(const Temporal *temp, const GSERIALIZED *gs)
{
  assert(temp); assert(gs); assert(temp->temptype == T_TGEOMPOINT);
  assert(MEOS_FLAGS_LINEAR_INTERP(temp->flags));
  assert(temp->subtype != TINSTANT);
  assert(! MEOS_FLAGS_GET_GEODETIC(temp->flags));
  assert(! gserialized_is_empty(gs));

  /* Extract the edges */
  LWGEOM *geom = lwgeom_from_gserialized(gs);
  MeosArray *edges = geom_extract_edges(geom);
  lwgeom_free(geom);
  Edge **edge_ptrs = palloc(sizeof(Edge *) * edges->count);
  for (int i = 0; i < (int) edges->count; i++)
    edge_ptrs[i] = (Edge *) meos_array_get(edges, i);

  /* Static array accumulating the per-segment candidate turning times */
  events = meos_array_create(sizeof(double));

  Temporal *result;
  assert(temptype_subtype(temp->subtype));
  if (temp->subtype == TSEQUENCE)
    result = (Temporal *) tpointseq_distance_geom((TSequence *) temp,
      edge_ptrs, edges->count);
  else /* TSEQUENCESET */
  {
    const TSequenceSet *ss = (TSequenceSet *) temp;
    TSequence **sequences = palloc(sizeof(TSequence *) * ss->count);
    for (int i = 0; i < ss->count; i++)
      sequences[i] = tpointseq_distance_geom(TSEQUENCESET_SEQ_N(ss, i),
        edge_ptrs, edges->count);
    result = (Temporal *) tsequenceset_make_free(sequences, ss->count,
      NORMALIZE);
  }

  /* Clean up and return */
  meos_array_destroy(events); meos_array_destroy(edges); pfree(edge_ptrs);
  return result;
}

/**
 * @brief Return a temporal geometric point with linear interpolation
 * restricted to a 2D geometry
 * @details The temporal point may be 2D or 3D and the Z dimension is also
 * computed
 * @pre The arguments have the same SRID, the geometry is 2D and is not empty.
 * This is verified in #tgeo_restrict_geom
 */
Temporal *
tpoint_linear_restrict_geom(const Temporal *temp, const GSERIALIZED *gs,
  bool atfunc)
{
  assert(temp); assert(gs); assert(MEOS_FLAGS_LINEAR_INTERP(temp->flags));

  /* Compute atGeometry for the temporal point */
  Temporal *result_at = tpoint_linear_inter_geom(temp, gs, true);

  /* If "at" restriction, return */
  if (atfunc)
    return result_at;

  /* If "minus" restriction, compute the complement wrt time */
  if (! result_at)
    /* Nothing intersects the geometry, so the result is the whole value. Return
     * it in the same container the partial-minus path below produces (a
     * continuous sequence yields a sequence set) so that minusGeometry is
     * container-consistent with tgeo_restrict_geom whether or not the geometry
     * is met. */
    return (temp->subtype == TSEQUENCE) ?
      (Temporal *) tsequence_as_tsequenceset((const TSequence *) temp) :
      temporal_copy(temp);

  SpanSet *ss = temporal_time(result_at);
  Temporal *result = temporal_restrict_tstzspanset(temp, ss, atfunc);
  pfree(ss); pfree(result_at);
  return result;
}

/*****************************************************************************
 * Convex hull and minimum rotated rectangle
 * Arc-aware implementation where circular arcs are NOT linearized/polygonized.
 * The convex hull is constructed from the exact extremal points of the
 * geometry. Arc extrema at the four cardinal directions are added when
 * they lie on the arc.
 *****************************************************************************/

/**
 * @brief Compare two 2D points lexicographically
 */
static int
point2d_cmp(const void *a, const void *b)
{
  const POINT2D *pa = (const POINT2D *) a;
  const POINT2D *pb = (const POINT2D *) b;
  if (pa->x < pb->x)
    return -1;
  if (pa->x > pb->x)
    return 1;
  if (pa->y < pb->y)
    return -1;
  if (pa->y > pb->y)
    return 1;
  return 0;
}

/**
 * @brief Cross product of AB and AC
 */
static inline double
cross_product(const POINT2D *a, const POINT2D *b, const POINT2D *c)
{
  return (b->x - a->x) * (c->y - a->y) - (b->y - a->y) * (c->x - a->x);
}

/**
 * @brief Add a point to an array
 */
static inline void
add_point(POINT2D *points, uint32_t *npoints, double x, double y)
{
  points[*npoints].x = x;
  points[*npoints].y = y;
  (*npoints)++;
}

/**
 * @brief Add the geometrically relevant points of an edge
 * @details For a line, only the two endpoints are needed. For an arc, the two
 * endpoints and every cardinal point of the supporting circle lying on the arc
 * are added. The latter are essential: an arc may attain its X/Y extrema in
 * its interior.
 */
static void
add_edge_points(const Edge *e, POINT2D *points, uint32_t *npoints)
{
  add_point(points, npoints, e->x1, e->y1);
  if (fabs(e->x2 - e->x1) > FP_TOLERANCE ||
      fabs(e->y2 - e->y1) > FP_TOLERANCE)
  {
    add_point(points, npoints, e->x2, e->y2);
  }

  /* A circular arc can have its extremal X/Y coordinate at one
   * of the four cardinal directions of its supporting circle.
   * arc_contains_angle() is already implemented in this file and
   * therefore the exact angular extent of the arc is respected. */
  if (e->etype == EDGE_ARC || e->etype == EDGE_POLYARC)
  {
    const double angles[4] = { 0.0, M_PI_2, M_PI, -M_PI_2 };
    for (int i = 0; i < 4; i++)
    {
      double theta = angles[i];
      if (! arc_contains_angle(e, theta))
        continue;
      double x = e->cx + e->radius * cos(theta);
      double y = e->cy + e->radius * sin(theta);
      /* Avoid adding a point which is numerically equal to an endpoint.
       * Duplicates are harmless but avoiding them keeps the hull smaller. */
      if ((fabs(x - e->x1) > FP_TOLERANCE ||
           fabs(y - e->y1) > FP_TOLERANCE) &&
          (fabs(x - e->x2) > FP_TOLERANCE ||
           fabs(y - e->y2) > FP_TOLERANCE))
      {
        add_point(points, npoints, x, y);
      }
    }
  }
}

/**
 * @brief Compute the convex hull of a set of 2D points
 * @details The returned hull is not closed.
 * The implementation uses Andrew's monotone-chain algorithm.
 */
static uint32_t
convex_hull(const POINT2D *points, uint32_t npoints, POINT2D **hull)
{
  assert(points); assert(hull);
  *hull = NULL;
  if (npoints == 0)
    return 0;

  /* Sort a copy of the points */
  POINT2D *sorted = palloc(sizeof(POINT2D) * npoints);
  memcpy(sorted, points, sizeof(POINT2D) * npoints);
  qsort(sorted, npoints, sizeof(POINT2D), point2d_cmp);

  /* Remove duplicate points */
  uint32_t n = 0;
  for (uint32_t i = 0; i < npoints; i++)
  {
    if (n == 0 || sorted[i].x != sorted[n - 1].x ||
        sorted[i].y != sorted[n - 1].y)
    {
      sorted[n++] = sorted[i];
    }
  }

  /* Point or line */
  if (n <= 2)
  {
    *hull = sorted;
    return n;
  }

  /* Maximum size is 2*n */
  POINT2D *h = palloc(sizeof(POINT2D) * (2 * n));
  uint32_t nhull = 0;

  /* Lower hull */
  for (uint32_t i = 0; i < n; i++)
  {
    while (nhull >= 2 &&
      cross_product(&h[nhull - 2], &h[nhull - 1], &sorted[i]) <= 0.0)
    {
      nhull--;
    }
    h[nhull++] = sorted[i];
  }

  /* Upper hull */
  uint32_t lower = nhull;
  for (int i = (int) n - 2; i >= 0; i--)
  {
    while (nhull > lower &&
      cross_product(&h[nhull - 2], &h[nhull - 1], &sorted[i]) <= 0.0)
    {
      nhull--;
    }
    h[nhull++] = sorted[i];
  }

  /* Last point duplicates the first */
  nhull--;
  pfree(sorted);

  *hull = palloc(sizeof(POINT2D) * nhull);
  memcpy(*hull, h, sizeof(POINT2D) * nhull);
  pfree(h);
  return nhull;
}

/**
 * @brief Compute the rectangle defined by a given orientation
 * @details The rectangle axes are:
 *   u = (ux,uy)
 *   v = (-uy,ux)
 * The function computes the bounding rectangle of the supplied
 * convex-hull points in that coordinate system.
 */
static double
mrr_rectangle_for_direction(const POINT2D *points, uint32_t npoints,
  double ux, double uy, POINT2D rect[5])
{
  double vx = -uy;
  double vy = ux;
  double min_u = DBL_MAX;
  double max_u = -DBL_MAX;
  double min_v = DBL_MAX;
  double max_v = -DBL_MAX;
  for (uint32_t i = 0; i < npoints; i++)
  {
    double u = points[i].x * ux + points[i].y * uy;
    double v = points[i].x * vx + points[i].y * vy;
    if (u < min_u)
      min_u = u;
    if (u > max_u)
      max_u = u;
    if (v < min_v)
      min_v = v;
    if (v > max_v)
      max_v = v;
  }

  double width = max_u - min_u;
  double height = max_v - min_v;
  /* Convert the corners back to XY */
  rect[0].x = min_u * ux + min_v * vx;
  rect[0].y = min_u * uy + min_v * vy;
  rect[1].x = max_u * ux + min_v * vx;
  rect[1].y = max_u * uy + min_v * vy;
  rect[2].x = max_u * ux + max_v * vx;
  rect[2].y = max_u * uy + max_v * vy;
  rect[3].x = min_u * ux + max_v * vx;
  rect[3].y = min_u * uy + max_v * vy;
  rect[4] = rect[0];
  return width * height;
}

/**
 * @brief Add candidate rectangle directions generated by an edge
 * @details For a straight edge, the rectangle orientation only needs the edge
 * direction. For a circular arc, its tangent direction varies continuously.
 * The cardinal directions of the supporting circle delimit the intervals over
 * which the support point changes continuously. We therefore add:
 *   - the arc endpoint tangent directions
 *   - the four cardinal tangent directions when they occur on the arc
 * This keeps the calculation entirely analytic and avoids polygonizing the arc
 */
static void
mrr_add_edge_directions(const Edge *e, double *angles, uint32_t *nangles)
{
  if (e->etype != EDGE_ARC && e->etype != EDGE_POLYARC)
  {
    /* Straight edge */
    double angle = atan2(e->y2 - e->y1, e->x2 - e->x1);
    angles[(*nangles)++] = angle;
    return;
  }

  /* Circular arc: For CCW traversal the tangent direction at angle theta is:
   *   (-sin(theta), cos(theta))
   * We only need orientations, so adding pi is equivalent. */
  double theta[6];
  int ntheta = 0;
  theta[ntheta++] = e->theta0;
  theta[ntheta++] = e->theta1;

  /* Cardinal points split the circle into analytically simple
   * support-function intervals */
  const double cardinal[4] = { 0.0, M_PI_2, M_PI, -M_PI_2 };
  for (int i = 0; i < 4; i++)
  {
    if (arc_contains_angle(e, cardinal[i]))
      theta[ntheta++] = cardinal[i];
  }
  for (int i = 0; i < ntheta; i++)
  {
    double angle = theta[i] + (e->ccw ? M_PI_2 : -M_PI_2);
    /* Rectangle orientation has period pi */
    angle = fmod(angle, M_PI);
    if (angle < 0)
      angle += M_PI;
    angles[(*nangles)++] = angle;
  }
}

/**
 * @brief Construct a POINT/LINESTRING/POLYGON from an MRR
 */
static LWGEOM *
mrr_make_geometry(int32_t srid, const POINT2D rect[5], uint32_t nhull)
{
  /* Point */
  if (nhull == 1)
    return lwpoint_as_lwgeom(lwpoint_make2d(srid, rect[0].x, rect[0].y));

  /* Line */
  if (nhull == 2)
  {
    POINTARRAY *pa = ptarray_construct_empty(0, 0, 2);
    POINT4D p;
    p.z = 0.0;
    p.m = 0.0;
    p.x = rect[0].x;
    p.y = rect[0].y;
    ptarray_append_point(pa, &p, LW_TRUE);
    p.x = rect[1].x;
    p.y = rect[1].y;
    ptarray_append_point(pa, &p, LW_TRUE);
    return lwline_as_lwgeom(lwline_construct(srid, NULL, pa));
  }

  /* Polygon */
  POINTARRAY *pa = ptarray_construct_empty(0, 0, 5);
  POINT4D p;
  p.z = 0.0;
  p.m = 0.0;
  for (int i = 0; i < 5; i++)
  {
    p.x = rect[i].x;
    p.y = rect[i].y;
    ptarray_append_point(pa, &p, LW_TRUE);
  }
  LWPOLY *poly = lwpoly_construct_empty(srid, 0, 0);
  lwpoly_add_ring(poly, pa);
  return lwpoly_as_lwgeom(poly);
}

/**
 * @brief Return the minimum-area rotated rectangle
 * @details Works directly on the exact circular arcs represented by the Edge
 * structure. No curve-to-line conversion and no polygonization is performed.
 */
LWGEOM *
minimum_rotated_rectangle(const LWGEOM *geom)
{
  assert(geom);

  /* Empty input */
  if (lwgeom_is_empty(geom))
    return lwpoly_as_lwgeom(lwpoly_construct_empty(geom->srid, 0, 0));

  /* Extract the geometry */
  MeosArray *edge_array = geom_extract_edges(geom);
  uint32_t nedges = edge_array->count;
  if (nedges == 0)
  {
    meos_array_destroy(edge_array);
    return lwpoly_as_lwgeom(lwpoly_construct_empty(geom->srid, 0, 0));
  }

  /* Every edge contributes at most:
   *   line: 2 points
   *   arc : 2 endpoints + 4 cardinal points
   * Allocate the maximum possible number */
  uint32_t maxpoints = 6 * nedges;
  POINT2D *points = palloc(sizeof(POINT2D) * maxpoints);
  uint32_t npoints = 0;

  /* Extract the exact extremal points */
  for (uint32_t i = 0; i < nedges; i++)
  {
    Edge *e = (Edge *) meos_array_get(edge_array, i);
    add_edge_points(e, points, &npoints);
  }

  /* We no longer need the edge array for the convex hull */
  meos_array_destroy(edge_array);

  /* Convex hull */
  POINT2D *hull = NULL;
  uint32_t nhull = convex_hull(points, npoints, &hull);
  pfree(points);
  if (nhull == 0)
    return lwpoly_as_lwgeom(lwpoly_construct_empty(geom->srid, 0, 0));

  /* Degenerate cases */
  if (nhull == 1)
  {
    POINT2D rect[5];
    for (int i = 0; i < 5; i++)
      rect[i] = hull[0];
    LWGEOM *result = mrr_make_geometry(geom->srid, rect, nhull);
    pfree(hull);
    return result;
  }

  if (nhull == 2)
  {
    POINT2D rect[5];
    rect[0] = hull[0];
    rect[1] = hull[1];
    rect[2] = hull[1];
    rect[3] = hull[0];
    rect[4] = hull[0];
    LWGEOM *result = mrr_make_geometry(geom->srid, rect, nhull);
    pfree(hull);
    return result;
  }

  /*
   * Generate candidate orientations.
   * - For a polygonal convex hull these are simply the hull-edge
   *   orientations.
   * - For an arc, its tangent direction changes continuously. We therefore
   *   inspect the analytically significant tangent directions of the arc.
   * Because the rectangle orientation is periodic modulo pi,
   * all angles are normalized to [0,pi).
   */

  /*
   * Maximum number of candidate directions. Every hull edge contributes one
   * direction. The actual Edge array may have more entries than the hull, but
   * using a generous bound keeps this implementation simple.
   */
  uint32_t maxangles = 8 * nedges + 8 * nhull;
  double *angles = palloc(sizeof(double) * maxangles);
  uint32_t nangles = 0;

  /*
   * Re-extract the edges. This is inexpensive compared with the convex-hull
   * computation and keeps the code independent from any Edge pointer retained
   * after the first array was freed.
   */
  edge_array = geom_extract_edges(geom);
  nedges = edge_array->count;
  for (uint32_t i = 0; i < nedges; i++)
  {
    Edge *e = (Edge *) meos_array_get(edge_array, i);
    /* Only directions which can define a support side need to be considered */
    mrr_add_edge_directions(e, angles, &nangles);
  }
  meos_array_destroy(edge_array);

  /* Also add all convex-hull edge directions.
   * This is important because the hull itself is the object whose
   * bounding rectangle is being computed. */
  for (uint32_t i = 0; i < nhull; i++)
  {
    const POINT2D *a = &hull[i];
    const POINT2D *b = &hull[(i + 1) % nhull];
    double dx = b->x - a->x;
    double dy = b->y - a->y;
    if (hypot(dx, dy) <= FP_TOLERANCE)
      continue;
    double angle = atan2(dy, dx);
    angle = fmod(angle, M_PI);
    if (angle < 0)
      angle += M_PI;
    angles[nangles++] = angle;
  }

  /* Find the minimum-area rectangle */
  double best_area = DBL_MAX;
  POINT2D best_rect[5];
  for (uint32_t i = 0; i < nangles; i++)
  {
    double angle = angles[i];
    double ux = cos(angle);
    double uy = sin(angle);
    POINT2D rect[5];
    double area = mrr_rectangle_for_direction(hull, nhull, ux, uy, rect);
    if (area < best_area)
    {
      best_area = area;
      for (int j = 0; j < 5; j++)
        best_rect[j] = rect[j];
    }
  }

  /* Clean up and return */
  pfree(angles); pfree(hull);
  return mrr_make_geometry(geom->srid, best_rect, 4);
}

/*****************************************************************************
 * DE-9IM / ST_Relate
 *****************************************************************************/

/*
 * @brief DE-9IM cell dimensions:
 *   -1 = F (empty)
 *    0 = point
 *    1 = line
 *    2 = area
 */
typedef struct
{
  int8_t ii;
  int8_t ib;
  int8_t ie;
  int8_t bi;
  int8_t bb;
  int8_t be;
  int8_t ei;
  int8_t eb;
  int8_t ee;
} MeosDE9IM;

/**
 * @brief Set all cells of a DE-9IM matrix to F
 */
static inline void
de9im_init(MeosDE9IM *m)
{
  m->ii = -1;
  m->ib = -1;
  m->ie = -1;
  m->bi = -1;
  m->bb = -1;
  m->be = -1;
  m->ei = -1;
  m->eb = -1;
  m->ee = -1;
  return;
}

static POINT2D *relate_linear_boundary_points(Edge **edges, int nedges,
  int *count);

/**
 * @brief Accumulate a dimension into a DE-9IM cell
 * @details A cell records the @b maximum dimension of the corresponding
 * intersection, so a contribution can only raise it. Assigning instead of
 * accumulating lets a later zero-dimensional contribution overwrite an
 * earlier one-dimensional one, which makes the matrix depend on the order in
 * which the edge pairs happen to be visited
 */
static inline void
de9im_add(int8_t *cell, int8_t dim)
{
  if (dim > *cell)
    *cell = dim;
  return;
}

/**
 * @brief Convert a DE-9IM dimension to its character representation
 */
static inline char
de9im_dim_char(int8_t dim)
{
  switch (dim)
  {
    case -1: return 'F';
    case  0: return '0';
    case  1: return '1';
    case  2: return '2';
    default: return 'F';
  }
}

/**
 * @brief Convert a DE-9IM matrix to its 9-character representation
 */
static void
de9im_to_string(const MeosDE9IM *m, char result[10])
{
  result[0] = de9im_dim_char(m->ii);
  result[1] = de9im_dim_char(m->ib);
  result[2] = de9im_dim_char(m->ie);

  result[3] = de9im_dim_char(m->bi);
  result[4] = de9im_dim_char(m->bb);
  result[5] = de9im_dim_char(m->be);

  result[6] = de9im_dim_char(m->ei);
  result[7] = de9im_dim_char(m->eb);
  result[8] = de9im_dim_char(m->ee);

  result[9] = '\0';
  return;
}

/**
 * @brief Return true if a DE-9IM matrix satisfies a pattern
 * @details Pattern characters:
 *   T = any non-empty intersection
 *   F = empty intersection
 *   0 = point
 *   1 = line
 *   2 = area
 *   * = don't care
 */
static bool
de9im_match(const char matrix[10], const char pattern[10])
{
  for (int i = 0; i < 9; i++)
  {
    char p = pattern[i];
    if (p == '*')
      continue;
    if (p == 'T')
    {
      if (matrix[i] == 'F')
        return false;
      continue;
    }
    if (p == 'F')
    {
      if (matrix[i] != 'F')
        return false;
      continue;
    }
    if (p != matrix[i])
      return false;
  }
  return true;
}

/*****************************************************************************
 * Geometry classification
 *****************************************************************************/

/**
 * @brief Return true if a geometry contains a 2-dimensional region
 */
static bool
relate_is_areal(const LWGEOM *geom)
{
  if (! geom || lwgeom_is_empty(geom))
    return false;

  switch (geom->type)
  {
    case POLYGONTYPE:
    case MULTIPOLYGONTYPE:
    case TRIANGLETYPE:
    case CURVEPOLYTYPE:
    case MULTISURFACETYPE:
      return true;
    case COLLECTIONTYPE:
    {
      const LWCOLLECTION *col = (const LWCOLLECTION *) geom;
      for (uint32_t i = 0; i < col->ngeoms; i++)
      {
        if (relate_is_areal(col->geoms[i]))
          return true;
      }
      return false;
    }
    default:
      return false;
  }
}


/**
 * @brief Return true if a geometry is a point geometry
 */
static bool
relate_is_point(const LWGEOM *geom)
{
  if (! geom || lwgeom_is_empty(geom))
    return false;
  return geom->type == POINTTYPE || geom->type == MULTIPOINTTYPE;
}

/**
 * @brief Return true if a geometry contains 1-dimensional features
 */
static bool
relate_is_linear(const LWGEOM *geom)
{
  if (! geom || lwgeom_is_empty(geom))
    return false;

  switch (geom->type)
  {
    case LINETYPE:
    case MULTILINETYPE:
    case CIRCSTRINGTYPE:
    case COMPOUNDTYPE:
    case MULTICURVETYPE:
      return true;
    case COLLECTIONTYPE:
    {
      const LWCOLLECTION *col = (const LWCOLLECTION *) geom;
      for (uint32_t i = 0; i < col->ngeoms; i++)
      {
        if (relate_is_linear(col->geoms[i]))
          return true;
      }
      return false;
    }
    default:
      return false;
  }
}

/**
 * @brief Return the topological dimension of a geometry
 */
static int
relate_dimension(const LWGEOM *geom)
{
  if (relate_is_areal(geom))
    return 2;
  if (relate_is_linear(geom))
    return 1;
  if (relate_is_point(geom))
    return 0;
  return -1;
}

/*****************************************************************************
 * Point classification
 *****************************************************************************/

/**
 * @brief Return true if a point lies on the boundary of a geometry
 * @brief Uses the exact line/arc engine
 */
static bool
relate_point_on_boundary(double x, double y, Edge **edges, int nedges)
{
  for (int i = 0; i < nedges; i++)
  {
    const Edge *e = edges[i];
    switch (e->etype)
    {
      case EDGE_POLY:
        if (point_on_segment(x, y, e->x1, e->y1, e->x2, e->y2))
          return true;
        break;
      case EDGE_POLYARC:
        if (point_on_arc(x, y, e))
          return true;
        break;
      default:
        break;
    }
  }
  return false;
}

/**
 * @brief Classify a point with respect to an areal geometry
 * @details Return:
 *   0 = interior
 *   1 = boundary
 *   2 = exterior
 */
static int
relate_point_in_area(double x, double y, Edge **edges, int nedges)
{
  if (relate_point_on_boundary(x, y, edges, nedges))
    return 1;
  return point_in_polygon(x, y, edges, nedges) ? 0 : 2;
}

/*****************************************************************************
 * Point / Point
 *****************************************************************************/

/**
 * @brief 
 */
static void
relate_point_point(const LWGEOM *g1, const LWGEOM *g2, MeosDE9IM *m)
{
  const LWPOINT *p1 = (const LWPOINT *) g1;
  const LWPOINT *p2 = (const LWPOINT *) g2;

  /* Empty points have empty interiors and therefore everything is exterior */
  if (! p1->point || ! p2->point || p1->point->npoints == 0 ||
      p2->point->npoints == 0)
  {
    m->ee = 2;
    return;
  }

  POINT4D a, b;
  getPoint4d_p(p1->point, 0, &a);
  getPoint4d_p(p2->point, 0, &b);
  if (fabs(a.x - b.x) <= FP_TOLERANCE && fabs(a.y - b.y) <= FP_TOLERANCE)
  {
    m->ii = 0;
    m->ee = 2;
  }
  else
  {
    m->ie = 0;
    m->ei = 0;
    m->ee = 2;
  }
}

/*****************************************************************************
 * Linear geometry boundary handling
 *****************************************************************************/

/**
 * @brief Return true if two points are equal within the MEOS tolerance.
 */
static inline bool
relate_same_point(double x1, double y1, double x2, double y2)
{
  return fabs(x1 - x2) <= FP_TOLERANCE && fabs(y1 - y2) <= FP_TOLERANCE;
}

/**
 * @brief Return true if an edge has non-zero length.
 */
static inline bool
relate_edge_nonempty(const Edge *e)
{
  return !relate_same_point(e->x1, e->y1, e->x2, e->y2);
}

/**
 * @brief Return true if a point is an endpoint of an edge.
 */
static inline bool
relate_point_is_edge_endpoint(double x, double y, const Edge *e)
{
  return relate_same_point(x, y, e->x1, e->y1) ||
         relate_same_point(x, y, e->x2, e->y2);
}

/**
 * @brief Return true if a point occurs an odd number of times among the
 * endpoints of a linear geometry.
 * @details This implements the endpoint parity rule used for the boundary of
 * linear geometries:
 * - odd number of occurrences -> boundary
 * - even number of occurrences -> interior
 * Closed lines consequently have an empty boundary.
 * The function works on the extracted Edge representation, so circular arcs
 * remain exact.
 */
static bool
relate_point_on_linear_boundary(double x, double y, Edge **edges, int nedges)
{
  int count = 0;
  for (int i = 0; i < nedges; i++)
  {
    const Edge *e = edges[i];
    if (e->etype != EDGE_LINE && e->etype != EDGE_ARC)
      continue;
    if (!relate_edge_nonempty(e))
      continue;
    if (relate_same_point(x, y, e->x1, e->y1))
      count++;
    if (relate_same_point(x, y, e->x2, e->y2))
      count++;
  }
  return (count & 1) != 0;
}

/**
 * @brief Classify a point with respect to a complete linear geometry.
 * @details Return:
 *   0 = interior
 *   1 = boundary
 *   2 = exterior
 * Note that an endpoint of an individual edge does NOT automatically make the
 * point part of the geometry boundary. Endpoint parity is evaluated over the
 * complete linear geometry.
 */
static int
relate_point_in_linear(double x, double y, Edge **edges, int nedges)
{
  bool found = false;
  for (int i = 0; i < nedges; i++)
  {
    const Edge *e = edges[i];
    if (e->etype != EDGE_LINE && e->etype != EDGE_ARC)
      continue;
    bool on = false;
    if (e->etype == EDGE_LINE)
      on = point_on_segment(x, y, e->x1, e->y1, e->x2, e->y2);
    else if (e->etype == EDGE_ARC)
      on = point_on_arc(x, y, e);
    if (on)
    {
      found = true;
      break;
    }
  }

  if (! found)
    return 2;
  if (relate_point_on_linear_boundary(x, y, edges, nedges))
    return 1;
  return 0;
}

/*****************************************************************************
 * Point / Linear
 *****************************************************************************/

/**
 * @brief 
 */
static void
relate_point_linear(const LWGEOM *point_geom, const LWGEOM *line_geom,
  MeosDE9IM *m)
{
  const LWPOINT *point = (const LWPOINT *) point_geom;
  if (!point->point || point->point->npoints == 0)
  {
    m->ee = 2;
    return;
  }

  POINT4D p;
  getPoint4d_p(point->point, 0, &p);
  MeosArray *arr = geom_extract_edges(line_geom);
  int nedges = (int) arr->count;
  Edge **edges = palloc(sizeof(Edge *) * nedges);
  for (int i = 0; i < nedges; i++)
    edges[i] = (Edge *) meos_array_get(arr, i);

  /* Classify the point against the complete linear geometry */
  int loc = relate_point_in_linear(p.x, p.y, edges, nedges);
  switch (loc)
  {
    case 0:
      /* Point is in the interior of the line */
      de9im_add(&m->ii, 0);
      break;
    case 1:
      /* Point is on the boundary of the line */
      de9im_add(&m->ib, 0);
      break;
    case 2:
      /* Point is in the exterior of the line */
      de9im_add(&m->ie, 0);
      break;
  }

  /* Removing a single point from a linear geometry leaves a 1-dimensional
   * part of its interior outside the point */
  de9im_add(&m->ei, 1);

  /* A point has an empty boundary, so the whole boundary row stays F. Each
   * Mod-2 boundary point of the linear geometry other than the point itself
   * lies in the exterior of the point */
  int nb;
  POINT2D *bpts = relate_linear_boundary_points(edges, nedges, &nb);
  for (int i = 0; i < nb; i++)
  {
    if (relate_same_point(p.x, p.y, bpts[i].x, bpts[i].y))
      continue;
    de9im_add(&m->eb, 0);
    break;
  }
  pfree(bpts);

  /* The exterior of a non-empty linear geometry is 2-dimensional */
  de9im_add(&m->ee, 2);

  pfree(edges); meos_array_destroy(arr);
  return;
}

/*****************************************************************************
 * Linear / Point
 *****************************************************************************/

/**
 * @brief 
 */
static void
relate_linear_point(const LWGEOM *line_geom, const LWGEOM *point_geom,
  MeosDE9IM *m)
{
  MeosDE9IM tmp;
  de9im_init(&tmp);
  relate_point_linear(point_geom, line_geom, &tmp);

  /*
   * Transpose:
   *   II -> II
   *   IB -> BI
   *   IE -> EI
   *   BI -> IB
   *   BB -> BB
   *   BE -> EB
   *   EI -> IE
   *   EB -> BE
   *   EE -> EE
   */
  m->ii = tmp.ii;
  m->ib = tmp.bi;
  m->ie = tmp.ei;

  m->bi = tmp.ib;
  m->bb = tmp.bb;
  m->be = tmp.eb;

  m->ei = tmp.ie;
  m->eb = tmp.be;
  m->ee = tmp.ee;

  return;
}

/*****************************************************************************
 * Linear / Linear
 *****************************************************************************/

/**
 * @brief Return true if an intersection between two linear edges contains
 * a one-dimensional portion, and report the covered parameter interval
 * @details The function preserves the exact line/arc intersection
 * @param[in] a,b Edges to intersect
 * @param[out] t0,t1 Interval of @p a covered by @p b, only set on success
 */
static bool
relate_linear_edges_overlap(const Edge *a, const Edge *b, double *t0,
  double *t1)
{
  if (a->etype == EDGE_LINE && b->etype == EDGE_LINE)
  {
    IntersectResult r = linesegm_intersect(a->x1, a->y1, a->dx, a->dy,
        b->x1, b->y1, b->x2, b->y2);
    if (r.type != INTERSECT_OVERLAP)
      return false;
    /* The parameters are expressed on the first edge */
    *t0 = r.t0;
    *t1 = r.t1;
    return true;
  }

  /* A line and a circular arc can intersect only in points unless
   * the line is degenerate, which is excluded here. */
  return false;
}

/*****************************************************************************
 * Point / Area
 *****************************************************************************/

/**
 * @brief 
 */
static void
relate_point_area(const LWGEOM *point_geom, const LWGEOM *area_geom,
  MeosDE9IM *m)
{
  const LWPOINT *point = (const LWPOINT *) point_geom;
  if (! point->point || point->point->npoints == 0)
  {
    m->ee = 2;
    return;
  }

  POINT4D p;
  getPoint4d_p(point->point, 0, &p);
  MeosArray *arr = geom_extract_edges(area_geom);
  int nedges = (int) arr->count;
  Edge **edges = palloc(sizeof(Edge *) * nedges);
  for (int i = 0; i < nedges; i++)
    edges[i] = (Edge *) meos_array_get(arr, i);

  int loc = relate_point_in_area(p.x, p.y, edges, nedges);
  switch (loc)
  {
    case 0:
      m->ii = 0;
      break;
    case 1:
      m->ib = 0;
      break;
    default:
      m->ie = 0;
      break;
  }

  /* The area interior has dimension 2 */
  m->ei = 2;

  /* Boundary has dimension 1 */
  m->eb = 1;
  m->ee = 2;

  pfree(edges); meos_array_destroy(arr);
  return;
}

/*****************************************************************************
 * Area / Point
 *****************************************************************************/

/**
 * @brief 
 */
static void
relate_area_point(const LWGEOM *area_geom, const LWGEOM *point_geom,
  MeosDE9IM *m)
{
  MeosDE9IM tmp;
  de9im_init(&tmp);
  relate_point_area(point_geom, area_geom, &tmp);

  /* Transpose the matrix */
  m->ii = tmp.ii;
  m->ib = tmp.bi;
  m->ie = tmp.ei;

  m->bi = tmp.ib;
  m->bb = tmp.bb;
  m->be = tmp.eb;

  m->ei = tmp.ie;
  m->eb = tmp.be;
  m->ee = tmp.ee;
  return;
}

/*****************************************************************************
 * Linear / Area
 *****************************************************************************/

/**
 * @brief Compute the parameter of a point on an arc.
 * @details The returned value is in [0,1], where 0 corresponds to theta0 and
 * 1 corresponds to theta1 following the orientation of the arc.
 */
static double
relate_arc_parameter(const Edge *e, double x, double y)
{
  double phi = angle_normalize(atan2(y - e->cy, x - e->cx));
  double sweep;
  double off;
  if (e->ccw)
  {
    sweep = angle_normalize(e->theta1 - e->theta0);
    off = angle_normalize(phi - e->theta0);
  }
  else
  {
    sweep = angle_normalize(e->theta0 - e->theta1);
    off = angle_normalize(e->theta0 - phi);
  }
  if (sweep < FP_TOLERANCE)
    return 0.0;
  double t = off / sweep;
  if (t < 0.0)
    t = 0.0;
  if (t > 1.0)
    t = 1.0;
  return t;
}

/**
 * @brief Compute the point at parameter t on an edge.
 * @details This preserves the exact circular representation of an arc.
 */
static void
relate_edge_point(const Edge *e, double t, double *x, double *y)
{
  if (e->etype == EDGE_LINE || e->etype == EDGE_POLY)
  {
    *x = e->x1 + t * (e->x2 - e->x1);
    *y = e->y1 + t * (e->y2 - e->y1);
    return;
  }
  /* Circular arc */
  double sweep = e->ccw ?
    angle_normalize(e->theta1 - e->theta0) :
    angle_normalize(e->theta0 - e->theta1);
  double theta = e->ccw ?
    e->theta0 + t * sweep :
    e->theta0 - t * sweep;
  *x = e->cx + e->radius * cos(theta);
  *y = e->cy + e->radius * sin(theta);
  return;
}

/**
 * @brief Add an intersection parameter to an array.
 */
static void
relate_add_parameter(double t, double *params, int *nparams, int maxparams)
{
  if (t < -FP_TOLERANCE || t > 1.0 + FP_TOLERANCE)
    return;
  if (t < 0.0)
    t = 0.0;
  if (t > 1.0)
    t = 1.0;

  /* Avoid inserting the same intersection several times. This is
   * particularly important at polygon vertices where two boundary
   * edges meet. */
  for (int i = 0; i < *nparams; i++)
  {
    if (fabs(params[i] - t) <= FP_TOLERANCE)
      return;
  }
  if (*nparams < maxparams)
    params[(*nparams)++] = t;
}

/**
 * @brief Return true if two arcs lie on the same supporting circle.
 */
static bool
relate_same_circle(const Edge *a, const Edge *b)
{
  return fabs(a->cx - b->cx) <= FP_TOLERANCE &&
         fabs(a->cy - b->cy) <= FP_TOLERANCE &&
         fabs(a->radius - b->radius) <= FP_TOLERANCE;
}

/**
 * @brief Return true if two circular arcs overlap in a non-zero-length
 * portion.
 * @details  The arcs must lie on the same supporting circle.
 * We use the endpoints of both arcs as candidate split points and test
 * whether an interval between consecutive candidates belongs to both
 * arcs. No polygonization is involved.
 */
static bool
relate_arcs_overlap(const Edge *a, const Edge *b)
{
  if (!relate_same_circle(a, b))
    return false;

  /* Collect the four endpoint parameters of b with respect to a.
   * If an endpoint is inside a, it provides a candidate boundary of
   * the overlap. */
  double p[4];
  int np = 0;
  double t;
  if (point_on_arc(b->x1, b->y1, a))
  {
    t = relate_arc_parameter(a, b->x1, b->y1);
    p[np++] = t;
  }
  if (point_on_arc(b->x2, b->y2, a))
  {
    t = relate_arc_parameter(a, b->x2, b->y2);
    p[np++] = t;
  }
  if (point_on_arc(a->x1, a->y1, b))
  {
    t = relate_arc_parameter(a, a->x1, a->y1);
    p[np++] = t;
  }
  if (point_on_arc(a->x2, a->y2, b))
  {
    t = relate_arc_parameter(a, a->x2, a->y2);
    p[np++] = t;
  }

  /* The arcs can overlap without having an endpoint strictly inside
   * the other arc only when they are effectively coincident. In that
   * case an endpoint of one arc is necessarily on the other arc. */
  if (np < 2)
    return false;

  /* Remove duplicate parameters. */
  for (int i = 0; i < np; i++)
  {
    for (int j = i + 1; j < np;)
    {
      if (fabs(p[i] - p[j]) <= FP_TOLERANCE)
      {
        for (int k = j; k < np - 1; k++)
          p[k] = p[k + 1];
        np--;
      }
      else
        j++;
    }
  }

  /* Test intervals between all candidate points. */
  for (int i = 0; i < np; i++)
  {
    for (int j = i + 1; j < np; j++)
    {
      if (fabs(p[j] - p[i]) <= FP_TOLERANCE)
        continue;
      double tm = (p[i] + p[j]) * 0.5;
      double x, y;
      relate_edge_point(a, tm, &x, &y);
      if (point_on_arc(x, y, b))
        return true;
    }
  }
  return false;
}

/**
 * @brief Add the intersection points of two circular arcs.
 * @details Returns the number of point intersections added to x/y.
 * If the arcs overlap over a non-zero-length portion, overlap is set
 * to true and no point is required for the one-dimensional component.
 */
static int
relate_arc_arc_points(const Edge *a, const Edge *b, double x[2], double y[2],
  bool *overlap)
{
  *overlap = false;
  double dx = b->cx - a->cx;
  double dy = b->cy - a->cy;
  double d = hypot(dx, dy);

  /* Coincident supporting circles */
  if (d <= FP_TOLERANCE)
  {
    if (fabs(a->radius - b->radius) > FP_TOLERANCE)
      return 0;
    if (relate_arcs_overlap(a, b))
    {
      *overlap = true;
      return 0;
    }

    /* They may touch at one or more common endpoints without having
     * a one-dimensional overlap. */
    int n = 0;
    if (point_on_arc(a->x1, a->y1, b))
    {
      x[n] = a->x1;
      y[n++] = a->y1;
    }
    if (n < 2 && point_on_arc(a->x2, a->y2, b) &&
        ! relate_same_point(a->x1, a->y1, a->x2, a->y2))
    {
      x[n] = a->x2;
      y[n++] = a->y2;
    }
    return n;
  }

  /* Disjoint supporting circles */
  if (d > a->radius + b->radius + FP_TOLERANCE ||
      d < fabs(a->radius - b->radius) - FP_TOLERANCE)
    return 0;
  double aa = (d * d + a->radius * a->radius - b->radius * b->radius) /
    (2.0 * d);
  double h2 = a->radius * a->radius - aa * aa;
  if (h2 < 0.0)
    h2 = 0.0;
  double h = sqrt(h2);
  double ux = dx / d;
  double uy = dy / d;
  double mx = a->cx + aa * ux;
  double my = a->cy + aa * uy;
  int n = 0;
  for (int k = 0; k < 2; k++)
  {
    double px = mx + (k ? h : -h) * (-uy);
    double py = my + (k ? h : -h) * ux;
    if (! point_on_arc(px, py, a) || ! point_on_arc(px, py, b))
      continue;
    /* Avoid duplicating a tangency point */
    if (n > 0 && relate_same_point(px, py, x[0], y[0]))
      continue;
    x[n] = px;
    y[n] = py;
    n++;
    if (h <= FP_TOLERANCE)
      break;
  }
  return n;
}

/**
 * @brief Process the intersection between one linear edge and one
 * polygon boundary edge.
 * @details Updates the DE-9IM cells corresponding to the intersection between
 * the linear geometry and the polygon boundary. The parameter array is
 * populated with all points at which the linear edge must be split.
 */
static void
relate_linear_area_edge_intersection(const Edge *line, const Edge *boundary,
  Edge **all_lines, int nlines, MeosDE9IM *m, double *params, int *nparams,
  int maxparams)
{
  /* Line / Poly */
  if (line->etype == EDGE_LINE && boundary->etype == EDGE_POLY)
  {
    IntersectResult r =  linesegm_intersect(line->x1, line->y1, line->dx,
      line->dy, boundary->x1, boundary->y1, boundary->x2, boundary->y2);
    if (r.type == INTERSECT_NONE)
      return;
    if (r.type == INTERSECT_OVERLAP)
    {
      /* The interior of the linear geometry overlaps the polygon
       * boundary over a one-dimensional portion */
      m->ib = 1;
      relate_add_parameter(r.t0, params, nparams, maxparams);
      relate_add_parameter(r.t1, params, nparams, maxparams);
      return;
    }

    /* Point intersection */
    double x = line->x1 + r.t0 * line->dx;
    double y = line->y1 + r.t0 * line->dy;
    int lloc = relate_point_in_linear(x, y, all_lines, nlines);
    if (lloc == 0)
      m->ib = 0;
    else if (lloc == 1)
      m->bb = 0;
    relate_add_parameter(r.t0, params, nparams, maxparams);
    return;
  }

  /* Line / PolyArc */
  if (line->etype == EDGE_LINE && boundary->etype == EDGE_POLYARC)
  {
    double roots[2];
    int n = arcsegm_intersect(line->x1, line->y1, line->dx, line->dy,
      boundary, roots);
    for (int i = 0; i < n; i++)
    {
      double t = roots[i];
      double x = line->x1 + t * line->dx;
      double y = line->y1 + t * line->dy;
      int lloc = relate_point_in_linear(x, y, all_lines, nlines);
      if (lloc == 0)
        m->ib = 0;
      else if (lloc == 1)
        m->bb = 0;
      relate_add_parameter(t, params, nparams, maxparams);
    }
    return;
  }

  /*
   * Arc / Poly
   * arcsegm_intersect() parametrizes the straight segment, therefore
   * we call it with the polygon edge as the trajectory and convert
   * the resulting point to the parameter of the linear arc.
   */
  if (line->etype == EDGE_ARC && boundary->etype == EDGE_POLY)
  {
    double roots[2];
    int n = arcsegm_intersect(boundary->x1, boundary->y1,
      boundary->dx, boundary->dy, line, roots);
    for (int i = 0; i < n; i++)
    {
      double x = boundary->x1 + roots[i] * boundary->dx;
      double y = boundary->y1 + roots[i] * boundary->dy;
      double t = relate_arc_parameter(line, x, y);
      int lloc = relate_point_in_linear(x, y, all_lines, nlines);
      if (lloc == 0)
        m->ib = 0;
      else if (lloc == 1)
        m->bb = 0;
      relate_add_parameter(t, params, nparams, maxparams);
    }
    return;
  }

  /* Arc / PolyArc */
  if (line->etype == EDGE_ARC && boundary->etype == EDGE_POLYARC)
  {
    double ix[2], iy[2];
    bool overlap = false;
    int n = relate_arc_arc_points(line, boundary, ix, iy, &overlap);
    if (overlap)
    {
      /* A non-zero-length common arc is a one-dimensional
       * intersection between the line interior and polygon
       * boundary */
      m->ib = 1;
      /* Split at the endpoints of both arcs. This is sufficient
       * to classify the remaining portions of the linear edge. */
      if (point_on_arc(boundary->x1, boundary->y1, line))
        relate_add_parameter(
          relate_arc_parameter(line, boundary->x1, boundary->y1),
          params, nparams, maxparams);
      if (point_on_arc(boundary->x2, boundary->y2, line))
        relate_add_parameter(
          relate_arc_parameter(line, boundary->x2, boundary->y2),
          params, nparams, maxparams);
      if (point_on_arc(line->x1, line->y1, boundary))
        relate_add_parameter(0.0, params, nparams, maxparams);
      if (point_on_arc(line->x2, line->y2, boundary))
        relate_add_parameter(1.0, params, nparams, maxparams);
      return;
    }

    for (int i = 0; i < n; i++)
    {
      int lloc = relate_point_in_linear(ix[i], iy[i], all_lines, nlines);
      if (lloc == 0)
        m->ib = 0;
      else if (lloc == 1)
        m->bb = 0;
      double t = relate_arc_parameter(line, ix[i], iy[i]);
      relate_add_parameter(t, params, nparams, maxparams);
    }
    return;
  }
  return;
}

/**
 * @brief Classify the open portion of a linear edge between two
 * consecutive parameters.
 * @details Since the portion contains no intersection with the polygon
 * boundary, one representative point is sufficient to determine whether the
 * complete open portion belongs to the polygon interior or exterior.
 */
static void
relate_linear_area_interval(const Edge *line, double t0, double t1,
  Edge **area_edges, int narea, MeosDE9IM *m)
{
  if (t1 - t0 <= FP_TOLERANCE)
    return;
  double tm = (t0 + t1) * 0.5;
  double x, y;
  relate_edge_point(line, tm, &x, &y);
  int loc = relate_point_in_area(x, y, area_edges, narea);
  switch (loc)
  {
    case 0:
      /* A non-zero open portion of the linear geometry is inside
       * the area: I/I has dimension 1. */
      de9im_add(&m->ii, 1);
      break;
    case 1:
      /* A non-zero open portion coincides with the area boundary. */
      de9im_add(&m->ib, 1);
      break;
    case 2:
      /* A non-zero open portion is outside the area. */
      de9im_add(&m->ie, 1);
      break;
  }
  return;
}

/**
 * @brief Sort an array of parameters in increasing order.
 */
static int
relate_parameter_cmp(const void *a, const void *b)
{
  const double da = *(const double *) a;
  const double db = *(const double *) b;
  if (da < db)
    return -1;
  if (da > db)
    return 1;
  return 0;
}

/*****************************************************************************
 * Linear / Linear
 *****************************************************************************/

/**
 * @brief Structure keeping a parameter interval of an edge
 */
typedef struct
{
  double t0;    /**< Start parameter, in [0,1] */
  double t1;    /**< End parameter, in [0,1], never less than t0 */
} RelateInterval;

/**
 * @brief Comparator ordering parameter intervals by their start
 */
static int
relate_interval_cmp(const void *a, const void *b)
{
  const RelateInterval *i1 = (const RelateInterval *) a;
  const RelateInterval *i2 = (const RelateInterval *) b;
  if (i1->t0 < i2->t0)
    return -1;
  if (i1->t0 > i2->t0)
    return 1;
  return 0;
}

/**
 * @brief Return true if a set of parameter intervals covers the whole
 * parameter range of an edge
 * @details The intervals are sorted in place and swept once, tracking the
 * furthest parameter reached without a gap. A gap of positive length is
 * a part of the edge that no interval covers
 * @param[in,out] intervals Intervals to test, reordered by the function
 * @param[in] count Number of intervals
 */
static bool
relate_intervals_cover(RelateInterval *intervals, int count)
{
  if (count == 0)
    return false;
  qsort(intervals, (size_t) count, sizeof(RelateInterval),
    relate_interval_cmp);
  if (intervals[0].t0 > FP_TOLERANCE)
    return false;
  double reach = intervals[0].t1;
  for (int i = 1; i < count; i++)
  {
    if (reach >= 1.0 - FP_TOLERANCE)
      break;
    if (intervals[i].t0 > reach + FP_TOLERANCE)
      return false;
    if (intervals[i].t1 > reach)
      reach = intervals[i].t1;
  }
  return reach >= 1.0 - FP_TOLERANCE;
}

/**
 * @brief Return the parameter intervals of an arc covered by another arc
 * @details Two arcs of the same circle can overlap in two disjoint pieces,
 * for example when both sweep more than half of the circle, so every piece
 * is reported separately instead of being merged into one enclosing interval
 * @param[in] a,b Arc edges
 * @param[out] out Covered intervals, expressed on @p a, at most three
 * @return Number of intervals reported
 */
static int
relate_arc_overlap_ranges(const Edge *a, const Edge *b, RelateInterval *out)
{
  if (! relate_same_circle(a, b))
    return 0;

  /* Candidate interval bounds: the ends of a, plus the ends of b that lie
   * on a, all expressed in the parameter space of a */
  double p[4];
  int np = 0;
  p[np++] = 0.0;
  p[np++] = 1.0;
  if (point_on_arc(b->x1, b->y1, a))
    p[np++] = relate_arc_parameter(a, b->x1, b->y1);
  if (point_on_arc(b->x2, b->y2, a))
    p[np++] = relate_arc_parameter(a, b->x2, b->y2);
  qsort(p, (size_t) np, sizeof(double), relate_parameter_cmp);

  int count = 0;
  for (int i = 0; i + 1 < np; i++)
  {
    if (p[i + 1] - p[i] <= FP_TOLERANCE)
      continue;
    /* A piece is covered when its midpoint is on the other arc */
    double x, y;
    relate_edge_point(a, (p[i] + p[i + 1]) * 0.5, &x, &y);
    if (! point_on_arc(x, y, b))
      continue;
    out[count].t0 = p[i];
    out[count].t1 = p[i + 1];
    count++;
  }
  return count;
}

/**
 * @brief Return true if every edge of a linear geometry is entirely covered
 * by the edges of another linear geometry
 * @details An uncovered part of a linear geometry is one-dimensional, so the
 * answer decides the interior/exterior cell of the DE-9IM matrix. Only the
 * one-dimensional intersections cover anything: a line and an arc, and two
 * arcs of different circles, meet in isolated points that cover no length
 */
static bool
relate_linear_covered(Edge **edges, int nedges, Edge **others, int nothers)
{
  /* Two arcs contribute at most three pieces, a line pair exactly one */
  RelateInterval *intervals = palloc(sizeof(RelateInterval) *
    (size_t) (3 * nothers + 1));
  bool result = true;
  for (int i = 0; i < nedges && result; i++)
  {
    const Edge *a = edges[i];
    if (a->etype != EDGE_LINE && a->etype != EDGE_ARC)
      continue;
    if (! relate_edge_nonempty(a))
      continue;
    int count = 0;
    for (int j = 0; j < nothers; j++)
    {
      const Edge *b = others[j];
      if (b->etype != EDGE_LINE && b->etype != EDGE_ARC)
        continue;
      if (a->etype == EDGE_ARC && b->etype == EDGE_ARC)
        count += relate_arc_overlap_ranges(a, b, intervals + count);
      else if (relate_linear_edges_overlap(a, b, &intervals[count].t0,
          &intervals[count].t1))
        count++;
    }
    if (! relate_intervals_cover(intervals, count))
      result = false;
  }
  pfree(intervals);
  return result;
}

/**
 * @brief Return the boundary points of a linear geometry
 * @details Under the OGC Mod-2 rule a point belongs to the boundary of a
 * linear geometry when it is an endpoint of an odd number of the component
 * curves. Counting edge endpoints gives the same parity, because an interior
 * vertex of a chain is shared by exactly two edges and therefore cancels
 * @param[in] edges,nedges Edges of the geometry
 * @param[out] count Number of boundary points, possibly zero for a geometry
 * made of closed components
 */
static POINT2D *
relate_linear_boundary_points(Edge **edges, int nedges, int *count)
{
  POINT2D *result = palloc(sizeof(POINT2D) * (size_t) (2 * nedges + 1));
  *count = 0;
  for (int i = 0; i < nedges; i++)
  {
    const Edge *e = edges[i];
    if (e->etype != EDGE_LINE && e->etype != EDGE_ARC)
      continue;
    if (! relate_edge_nonempty(e))
      continue;
    for (int k = 0; k < 2; k++)
    {
      double x = (k == 0) ? e->x1 : e->x2;
      double y = (k == 0) ? e->y1 : e->y2;
      /* Keep a single entry per distinct point */
      bool seen = false;
      for (int j = 0; j < *count && ! seen; j++)
        seen = relate_same_point(x, y, result[j].x, result[j].y);
      if (seen)
        continue;
      if (! relate_point_on_linear_boundary(x, y, edges, nedges))
        continue;
      result[*count].x = x;
      result[*count].y = y;
      (*count)++;
    }
  }
  return result;
}

/**
 * @brief Append the isolated intersection points of two linear edges
 * @param[in] a,b Edges to intersect
 * @param[out] out Intersection points, at most two
 * @return Number of points appended
 */
static int
relate_linear_edge_points(const Edge *a, const Edge *b, POINT2D *out)
{
  int count = 0;
  if (a->etype == EDGE_LINE && b->etype == EDGE_LINE)
  {
    IntersectResult r = linesegm_intersect(a->x1, a->y1, a->dx, a->dy,
      b->x1, b->y1, b->x2, b->y2);
    if (r.type == INTERSECT_POINT)
    {
      relate_edge_point(a, r.t0, &out[count].x, &out[count].y);
      count++;
    }
  }
  else if (a->etype == EDGE_LINE && b->etype == EDGE_ARC)
  {
    double roots[2];
    int n = arcsegm_intersect(a->x1, a->y1, a->dx, a->dy, b, roots);
    for (int k = 0; k < n; k++)
    {
      relate_edge_point(a, roots[k], &out[count].x, &out[count].y);
      count++;
    }
  }
  else if (a->etype == EDGE_ARC && b->etype == EDGE_LINE)
  {
    double roots[2];
    int n = arcsegm_intersect(b->x1, b->y1, b->dx, b->dy, a, roots);
    for (int k = 0; k < n; k++)
    {
      relate_edge_point(b, roots[k], &out[count].x, &out[count].y);
      count++;
    }
  }
  else if (a->etype == EDGE_ARC && b->etype == EDGE_ARC)
  {
    double x[2], y[2];
    bool overlap = false;
    int n = relate_arc_arc_points(a, b, x, y, &overlap);
    for (int k = 0; k < n; k++)
    {
      out[count].x = x[k];
      out[count].y = y[k];
      count++;
    }
  }
  return count;
}

/**
 * @brief Compute the DE-9IM matrix for two linear geometries
 * @details Every cell has exactly one source, so no cell can be attributed
 * twice or left to the visiting order of the edge pairs:
 * - II comes from the one-dimensional overlaps and from the intersection
 *   points that are interior to both geometries
 * - the boundary row of @p g1 comes from classifying each Mod-2 boundary
 *   point of @p g1 against @p g2, and symmetrically for @p g2
 * - IE and EI come from whether one geometry covers the other entirely,
 *   an uncovered part of a linear geometry being one-dimensional
 * The line/arc and arc/arc intersections stay exact, so a circular string
 * is related without being stroked into segments first
 */
static void
relate_linear_linear(const LWGEOM *g1, const LWGEOM *g2, MeosDE9IM *m)
{
  MeosArray *a1 = geom_extract_edges(g1);
  MeosArray *a2 = geom_extract_edges(g2);
  int n1 = (int) a1->count;
  int n2 = (int) a2->count;
  Edge **e1 = palloc(sizeof(Edge *) * (size_t) (n1 + 1));
  Edge **e2 = palloc(sizeof(Edge *) * (size_t) (n2 + 1));
  for (int i = 0; i < n1; i++)
    e1[i] = (Edge *) meos_array_get(a1, i);
  for (int i = 0; i < n2; i++)
    e2[i] = (Edge *) meos_array_get(a2, i);

  /* The Mod-2 boundary of each operand, empty for closed components */
  int nb1, nb2;
  POINT2D *b1 = relate_linear_boundary_points(e1, n1, &nb1);
  POINT2D *b2 = relate_linear_boundary_points(e2, n2, &nb2);

  /* Interior/interior. A positive-length overlap of any edge pair is a
   * one-dimensional intersection of the two interiors. An intersection point
   * that is on neither Mod-2 boundary is a zero-dimensional one; a point on
   * a boundary is accounted for by the boundary row or column below */
  POINT2D points[2];
  for (int i = 0; i < n1; i++)
  {
    const Edge *a = e1[i];
    if (a->etype != EDGE_LINE && a->etype != EDGE_ARC)
      continue;
    for (int j = 0; j < n2; j++)
    {
      const Edge *b = e2[j];
      if (b->etype != EDGE_LINE && b->etype != EDGE_ARC)
        continue;
      double t0, t1;
      if (a->etype == EDGE_ARC && b->etype == EDGE_ARC)
      {
        RelateInterval iv[3];
        if (relate_arc_overlap_ranges(a, b, iv) > 0)
          de9im_add(&m->ii, 1);
      }
      else if (relate_linear_edges_overlap(a, b, &t0, &t1))
        de9im_add(&m->ii, 1);

      int np = relate_linear_edge_points(a, b, points);
      for (int k = 0; k < np; k++)
      {
        bool on_b1 = false, on_b2 = false;
        for (int p = 0; p < nb1 && ! on_b1; p++)
          on_b1 = relate_same_point(points[k].x, points[k].y, b1[p].x,
            b1[p].y);
        for (int p = 0; p < nb2 && ! on_b2; p++)
          on_b2 = relate_same_point(points[k].x, points[k].y, b2[p].x,
            b2[p].y);
        if (! on_b1 && ! on_b2)
          de9im_add(&m->ii, 0);
      }
    }
  }

  /* Boundary row of g1: each of its boundary points lies on the boundary of
   * g2, in the interior of g2, or outside g2 altogether */
  for (int p = 0; p < nb1; p++)
  {
    int loc = relate_point_in_linear(b1[p].x, b1[p].y, e2, n2);
    if (loc == 1)
      de9im_add(&m->bb, 0);
    else if (loc == 0)
      de9im_add(&m->bi, 0);
    else
      de9im_add(&m->be, 0);
  }

  /* Boundary column of g2, symmetrically */
  for (int p = 0; p < nb2; p++)
  {
    int loc = relate_point_in_linear(b2[p].x, b2[p].y, e1, n1);
    if (loc == 1)
      de9im_add(&m->bb, 0);
    else if (loc == 0)
      de9im_add(&m->ib, 0);
    else
      de9im_add(&m->eb, 0);
  }

  /* Interior/exterior. Whatever of g1 the edges of g2 do not cover is a
   * one-dimensional part of the interior of g1 lying outside g2 */
  if (! relate_linear_covered(e1, n1, e2, n2))
    de9im_add(&m->ie, 1);
  if (! relate_linear_covered(e2, n2, e1, n1))
    de9im_add(&m->ei, 1);

  /* The exterior of a bounded planar geometry is two-dimensional */
  de9im_add(&m->ee, 2);

  pfree(b1); pfree(b2); pfree(e1); pfree(e2);
  meos_array_destroy(a1); meos_array_destroy(a2);
  return;
}

/**
 * @brief Compute the DE-9IM matrix for a linear geometry and an
 * areal geometry.
 */
static void
relate_linear_area(const LWGEOM *line_geom, const LWGEOM *area_geom,
  MeosDE9IM *m)
{
  MeosArray *la = geom_extract_edges(line_geom);
  MeosArray *aa = geom_extract_edges(area_geom);
  int nl = (int) la->count;
  int na = (int) aa->count;
  Edge **lines = palloc(sizeof(Edge *) * nl);
  Edge **area_edges = palloc(sizeof(Edge *) * na);
  for (int i = 0; i < nl; i++)
    lines[i] = (Edge *) meos_array_get(la, i);
  for (int i = 0; i < na; i++)
    area_edges[i] = (Edge *) meos_array_get(aa, i);

  /* Maximum number of split parameters:
   * - every area edge can contribute at most two intersection parameters;
   * - 2 is added for the two endpoints of the linear edge.
   * This is deliberately allocated per linear edge. */
  const int maxparams = 2 * na + 2;
  double *params = palloc(sizeof(double) * maxparams);

  /* The area interior has dimension 2 whenever the area is non-empty. */
  m->ei = 2;
  /* The area boundary has dimension 1. */
  m->eb = 1;
  /* A finite ordinary area geometry has a 2-dimensional exterior. */
  m->ee = 2;
  for (int i = 0; i < nl; i++)
  {
    const Edge *line = lines[i];
    if (line->etype != EDGE_LINE && line->etype != EDGE_ARC)
      continue;
    int nparams = 0;
    /* The edge endpoints delimit the complete edge. */
    params[nparams++] = 0.0;
    params[nparams++] = 1.0;
    /* Intersect this linear edge with every area boundary edge. */
    for (int j = 0; j < na; j++)
    {
      const Edge *boundary = area_edges[j];
      if (boundary->etype != EDGE_POLY && boundary->etype != EDGE_POLYARC)
        continue;
      relate_linear_area_edge_intersection(line, boundary, lines, nl, m,
        params, &nparams, maxparams);
    }

    /* Sort and remove duplicate parameters. */
    qsort(params, nparams, sizeof(double), relate_parameter_cmp);
    int nuniq = 0;
    for (int j = 0; j < nparams; j++)
    {
      if (nuniq == 0 || fabs(params[j] - params[nuniq - 1]) > FP_TOLERANCE)
      {
        params[nuniq++] = params[j];
      }
    }

    /* Classify every open portion between consecutive boundary
     * intersections. */
    for (int j = 0; j < nuniq - 1; j++)
    {
      relate_linear_area_interval(line, params[j], params[j + 1], area_edges,
        na, m);
    }

    /* Classify the two endpoints of the linear edge.
     * These belong to the boundary or interior of the complete
     * linear geometry according to the endpoint parity established
     * by relate_point_in_linear(). */
    for (int endpoint = 0; endpoint < 2; endpoint++)
    {
      double x = endpoint == 0 ? line->x1 : line->x2;
      double y = endpoint == 0 ? line->y1 : line->y2;
      int lloc = relate_point_in_linear(x, y, lines, nl);
      if (lloc == 2)
        continue;
      int aloc = relate_point_in_area(x, y, area_edges, na);
      if (lloc == 0)
      {
        /* Linear interior ∩ area. An endpoint contributes dimension 0, which
         * must not demote the dimension 1 an open portion of the same edge
         * has already contributed to the very same cell */
        if (aloc == 0)
          de9im_add(&m->ii, 0);
        else if (aloc == 1)
          de9im_add(&m->ib, 0);
        else
          de9im_add(&m->ie, 0);
      }
      else
      {
        /* Linear boundary ∩ area */
        if (aloc == 0)
          de9im_add(&m->bi, 0);
        else if (aloc == 1)
          de9im_add(&m->bb, 0);
        else
          de9im_add(&m->be, 0);
      }
    }
  }

  /* If the linear geometry has a non-empty boundary, its boundary
   * is zero-dimensional. We therefore also need to account for the
   * boundary's intersection with the area exterior when the boundary
   * is not completely contained in the area closure. */
  bool has_boundary = false;
  for (int i = 0; i < nl && !has_boundary; i++)
  {
    const Edge *line = lines[i];
    if (line->etype != EDGE_LINE && line->etype != EDGE_ARC)
      continue;
    if (! relate_edge_nonempty(line))
      continue;
    if (relate_point_on_linear_boundary(line->x1, line->y1, lines, nl) ||
        relate_point_on_linear_boundary(line->x2, line->y2, lines, nl))
      has_boundary = true;
  }

  if (has_boundary)
  {
    /*
     * BE is present whenever a linear boundary point lies outside
     * the area.
     * BI and BB have already been set above for boundary points
     * that lie in the area interior or on its boundary.
     */
    for (int i = 0; i < nl; i++)
    {
      const Edge *line = lines[i];
      if (line->etype != EDGE_LINE && line->etype != EDGE_ARC)
        continue;
      double x[2] = {line->x1, line->x2};
      double y[2] = {line->y1, line->y2};
      for (int k = 0; k < 2; k++)
      {
        if (!relate_point_on_linear_boundary(x[k], y[k], lines, nl))
          continue;
        int aloc = relate_point_in_area(x[k], y[k], area_edges, na);
        if (aloc == 2)
          m->be = 0;
      }
    }
  }

  pfree(params); pfree(lines); pfree(area_edges);
  meos_array_destroy(la); meos_array_destroy(aa);
  return;
}

/*****************************************************************************
 * Area / Linear
 *****************************************************************************/

/**
 * @brief 
 */
static void
relate_area_linear(const LWGEOM *area_geom, const LWGEOM *line_geom,
  MeosDE9IM *m)
{
  MeosDE9IM tmp;
  de9im_init(&tmp);
  relate_linear_area(line_geom, area_geom, &tmp);

  m->ii = tmp.ii;
  m->ib = tmp.bi;
  m->ie = tmp.ei;

  m->bi = tmp.ib;
  m->bb = tmp.bb;
  m->be = tmp.eb;

  m->ei = tmp.ie;
  m->eb = tmp.be;
  m->ee = tmp.ee;
  return;
}

/*****************************************************************************
 * Area / Area
 *****************************************************************************/

/**
 * @brief Return true if an edge is a polygon boundary edge.
 */
static inline bool
relate_area_boundary_edge(const Edge *e)
{
  return e->etype == EDGE_POLY || e->etype == EDGE_POLYARC;
}

/**
 * @brief Compute a point on an areal boundary edge.
 * @details 
 *   t = 0 -> first endpoint
 *   t = 1 -> second endpoint
 * Circular arcs are evaluated exactly.
 */
static void
relate_area_edge_point(const Edge *e, double t, double *x, double *y)
{
  if (e->etype == EDGE_POLY)
  {
    *x = e->x1 + t * (e->x2 - e->x1);
    *y = e->y1 + t * (e->y2 - e->y1);
    return;
  }

  /* Circular polygon boundary */
  double sweep = e->ccw ?
    angle_normalize(e->theta1 - e->theta0) :
    angle_normalize(e->theta0 - e->theta1);
  double theta = e->ccw ?
    e->theta0 + t * sweep :
    e->theta0 - t * sweep;
  *x = e->cx + e->radius * cos(theta);
  *y = e->cy + e->radius * sin(theta);
  return;
}

/**
 * @brief Return the parameter of a point on a polygon boundary edge.
 */
static double
relate_area_edge_parameter(const Edge *e, double x, double y)
{
  if (e->etype == EDGE_POLY)
  {
    double dx = e->x2 - e->x1;
    double dy = e->y2 - e->y1;
    if (fabs(dx) >= fabs(dy))
    {
      if (fabs(dx) <= FP_TOLERANCE)
        return 0.0;
      return (x - e->x1) / dx;
    }
    else
    {
      if (fabs(dy) <= FP_TOLERANCE)
        return 0.0;
      return (y - e->y1) / dy;
    }
  }
  return relate_arc_parameter(e, x, y);
}

/**
 * @brief Add an intersection parameter to an array.
 */
static void
relate_area_add_parameter(double t, double *params, int *nparams,
  int maxparams)
{
  if (t < -FP_TOLERANCE || t > 1.0 + FP_TOLERANCE)
    return;
  if (t < 0.0)
    t = 0.0;
  if (t > 1.0)
    t = 1.0;
  for (int i = 0; i < *nparams; i++)
  {
    if (fabs(params[i] - t) <= FP_TOLERANCE)
      return;
  }
  if (*nparams < maxparams)
    params[(*nparams)++] = t;
  return;
}

/**
 * @brief Sort area-edge parameters.
 */
static int
relate_area_parameter_cmp(const void *a, const void *b)
{
  const double da = *(const double *) a;
  const double db = *(const double *) b;
  if (da < db)
    return -1;
  if (da > db)
    return 1;
  return 0;
}

/**
 * @brief Return true if two polygon boundary edges are collinear and
 * overlap over a non-zero length.
 */
static bool
relate_area_line_overlap(const Edge *a, const Edge *b)
{
  if (a->etype != EDGE_POLY || b->etype != EDGE_POLY)
    return false;
  IntersectResult r = linesegm_intersect(a->x1, a->y1, a->dx, a->dy,
      b->x1, b->y1, b->x2, b->y2);
  return r.type == INTERSECT_OVERLAP;
}

/**
 * @brief Determine intersections between two polygon boundary edges.
 * @details Return:
 *   0 = no intersection
 *   1 = point intersections
 *   2 = one-dimensional overlap
 * The intersection points are returned in x/y.
 */
static int
relate_area_edge_intersection(const Edge *a, const Edge *b, double ix[2],
  double iy[2])
{
  /* Line / Line */
  if (a->etype == EDGE_POLY && b->etype == EDGE_POLY)
  {
    IntersectResult r = linesegm_intersect(a->x1, a->y1, a->dx, a->dy,
      b->x1, b->y1, b->x2, b->y2);
    if (r.type == INTERSECT_NONE)
      return 0;
    if (r.type == INTERSECT_OVERLAP)
      return 2;
    if (r.type == INTERSECT_POINT)
    {
      ix[0] = a->x1 + r.t0 * a->dx;
      iy[0] = a->y1 + r.t0 * a->dy;
      return 1;
    }
    return 0;
  }

  /* Line / Arc */
  if (a->etype == EDGE_POLY && b->etype == EDGE_POLYARC)
  {
    double roots[2];
    int n = arcsegm_intersect(a->x1, a->y1, a->dx, a->dy, b, roots);
    if (n <= 0)
      return 0;
    int count = 0;
    for (int i = 0; i < n && count < 2; i++)
    {
      ix[count] = a->x1 + roots[i] * a->dx;
      iy[count] = a->y1 + roots[i] * a->dy;
      count++;
    }
    return count;
  }

  /* Arc / Line */
  if (a->etype == EDGE_POLYARC && b->etype == EDGE_POLY)
  {
    double roots[2];
    int n = arcsegm_intersect(b->x1, b->y1, b->dx, b->dy, a, roots);
    if (n <= 0)
      return 0;
    int count = 0;
    for (int i = 0; i < n && count < 2; i++)
    {
      ix[count] = b->x1 + roots[i] * b->dx;
      iy[count] = b->y1 + roots[i] * b->dy;
      count++;
    }
    return count;
  }

  /* Arc / Arc */
  if (a->etype == EDGE_POLYARC && b->etype == EDGE_POLYARC)
  {
    bool overlap = false;
    int n = relate_arc_arc_points(a, b, ix, iy, &overlap);
    if (overlap)
      return 2;
    return n;
  }
  return 0;
}

/**
 * @brief Test whether an open portion of one area boundary lies in the
 * interior of another area.
 * @details This is the key operation for detecting:
 *   BI = boundary(A) / interior(B)
 *   IB = interior(A) / boundary(B)
 * as one-dimensional intersections.
 */
static bool
relate_area_edge_inside_area(const Edge *edge, Edge **area_edges, int narea)
{
  double x, y;
  /* Midpoint of a straight boundary edge */
  if (edge->etype == EDGE_POLY)
  {
    x = (edge->x1 + edge->x2) * 0.5;
    y = (edge->y1 + edge->y2) * 0.5;
  }
  else
  {
    /* Midpoint of the circular arc */
    relate_area_edge_point(edge, 0.5, &x, &y);
  }
  return relate_point_in_area(x, y, area_edges, narea) == 0;
}

/**
 * @brief Classify the open portions of one polygon boundary edge.
 * @details The edge is split at every intersection with the other polygon
 * boundary. Since there is no boundary intersection inside an open
 * interval, one representative point is sufficient.
 */
static void
relate_area_edge_intervals(const Edge *edge, Edge **other_edges, int nother,
  MeosDE9IM *m, bool first)
{
  /* Maximum number of intersections between one edge and one
   * circular/linear boundary edge is two */
  int maxparams = 2 * nother + 2;
  double *params = palloc(sizeof(double) * maxparams);
  int nparams = 0;
  params[nparams++] = 0.0;
  params[nparams++] = 1.0;
  for (int j = 0; j < nother; j++)
  {
    const Edge *other = other_edges[j];
    if (! relate_area_boundary_edge(other))
      continue;
    double ix[2], iy[2];
    int n = relate_area_edge_intersection(edge, other, ix, iy);
    if (n == 2 && relate_area_line_overlap(edge, other))
    {
      /* The two boundary edges overlap over a line. Add the endpoints of the
       * overlapping portion as split parameters. This is mainly needed for the
       * classification of the remaining portions. */
      if (point_on_segment(other->x1, other->y1, edge->x1, edge->y1,
            edge->x2, edge->y2))
      {
        relate_area_add_parameter(relate_area_edge_parameter(edge, other->x1,
          other->y1), params, &nparams, maxparams);
      }

      if (point_on_segment(other->x2, other->y2, edge->x1, edge->y1,
            edge->x2, edge->y2))
      {
        relate_area_add_parameter(relate_area_edge_parameter(edge, other->x2,
          other->y2), params, &nparams, maxparams);
      }
      continue;
    }
    for (int k = 0; k < n; k++)
    {
      double t = relate_area_edge_parameter(edge, ix[k], iy[k]);
      relate_area_add_parameter(t, params, &nparams, maxparams);
    }
  }

  qsort(params, nparams, sizeof(double), relate_area_parameter_cmp);
  /* Remove duplicates */
  int nuniq = 0;
  for (int i = 0; i < nparams; i++)
  {
    if (nuniq == 0 || fabs(params[i] - params[nuniq - 1]) > FP_TOLERANCE)
    {
      params[nuniq++] = params[i];
    }
  }

  /* Classify each open interval */
  for (int i = 0; i < nuniq - 1; i++)
  {
    if (params[i + 1] - params[i] <= FP_TOLERANCE)
      continue;
    double t = (params[i] + params[i + 1]) * 0.5;
    double x, y;
    relate_area_edge_point(edge, t, &x, &y);
    int loc = relate_point_in_area(x, y, other_edges, nother);
    if (loc == 0)
    {
      /* Boundary(A) ∩ Interior(B) or Interior(A) ∩ Boundary(B) */
      if (first)
        m->bi = 1;
      else
        m->ib = 1;
      /* If a non-zero boundary portion of A lies inside B,
       * then the interiors of A and B also intersect in a
       * two-dimensional neighbourhood of that portion. */
      m->ii = 2;
    }
    else if (loc == 2)
    {
      /* Boundary(A) ∩ Exterior(B) */
      if (first)
        m->be = 1;
      else
        m->eb = 1;
    }
  }
  pfree(params);
  return;
}

/**
 * @brief Process all point intersections between two area boundaries.
 */
static void
relate_area_boundary_points(Edge **aedges, int na, Edge **bedges, int nb,
  MeosDE9IM *m)
{
  for (int i = 0; i < na; i++)
  {
    const Edge *a = aedges[i];
    if (!relate_area_boundary_edge(a))
      continue;
    for (int j = 0; j < nb; j++)
    {
      const Edge *b = bedges[j];
      if (!relate_area_boundary_edge(b))
        continue;
      double ix[2], iy[2];
      int n = relate_area_edge_intersection(a, b, ix, iy);

      /*  A one-dimensional overlap */
      if (n == 2 && relate_area_line_overlap(a, b))
      {
        de9im_add(&m->bb, 1);
        continue;
      }

      /* Circular coincident arcs are represented as overlap by
       * relate_arc_arc_points(), and therefore also produce a
       * one-dimensional BB intersection. */
      if (a->etype == EDGE_POLYARC && b->etype == EDGE_POLYARC)
      {
        bool overlap = false;
        (void) relate_arc_arc_points(a, b, ix, iy, &overlap);
        if (overlap)
        {
          de9im_add(&m->bb, 1);
          continue;
        }
      }

      /* Point intersections. The cell keeps the largest dimension found over
       * all the edge pairs, so a shared boundary segment is not demoted by a
       * later pair that meets in a single point */
      if (n > 0)
        de9im_add(&m->bb, 0);
    }
  }
  return;
}

/**
 * @brief Determine whether an area has an interior point in the
 * interior of another area.
 * @details We use boundary vertices first. If a vertex of A lies in the
 * interior of B, then a neighbourhood of that vertex inside A is
 * also inside B, proving II = 2.
 */
static bool
relate_area_has_vertex_interior(Edge **edges, int nedges, Edge **other_edges,
  int nother)
{
  for (int i = 0; i < nedges; i++)
  {
    const Edge *e = edges[i];
    if (!relate_area_boundary_edge(e))
      continue;
    if (relate_point_in_area(e->x1, e->y1, other_edges, nother) == 0)
      return true;
    if (relate_point_in_area(e->x2, e->y2, other_edges, nother) == 0)
      return true;
  }
  return false;
}

/**
 * @brief Find an interior point of an areal geometry.
 * @details The function first tries boundary-edge midpoints with a small
 * perturbation to either side. This avoids requiring a centroid
 * implementation and works directly with circular boundaries.
 */
static bool
relate_area_find_interior_point(Edge **edges, int nedges, double *x,
  double *y)
{
  for (int i = 0; i < nedges; i++)
  {
    const Edge *e = edges[i];
    if (! relate_area_boundary_edge(e))
      continue;
    /* Take the midpoint of the edge */
    double px, py;
    relate_area_edge_point(e, 0.5, &px, &py);
    /* Estimate a local tangent */
    double tx, ty;
    if (e->etype == EDGE_POLY)
    {
      tx = e->x2 - e->x1;
      ty = e->y2 - e->y1;
    }
    else
    {
      /* Tangent to a circular arc at its midpoint */
      double sweep = e->ccw ?
        angle_normalize(e->theta1 - e->theta0) :
        angle_normalize(e->theta0 - e->theta1);
      double theta = e->ccw ?
        e->theta0 + 0.5 * sweep :
        e->theta0 - 0.5 * sweep;
      if (e->ccw)
      {
        tx = -sin(theta);
        ty =  cos(theta);
      }
      else
      {
        tx =  sin(theta);
        ty = -cos(theta);
      }
    }
    double len = hypot(tx, ty);
    if (len <= FP_TOLERANCE)
      continue;
    tx /= len;
    ty /= len;

    /* Two possible normal directions. The actual polygon orientation
     * is irrelevant because we test both sides. */
    double nx = -ty;
    double ny =  tx;

    /* The tolerance is deliberately small relative to the edge
     * length. This is only used to obtain an interior witness
     * point; all actual intersections remain exact. */
    double eps = fmax(FP_TOLERANCE * 10.0, len * 1e-9);
    double qx = px + eps * nx;
    double qy = py + eps * ny;
    if (relate_point_in_area(qx, qy, edges, nedges) == 0)
    {
      *x = qx;
      *y = qy;
      return true;
    }
    qx = px - eps * nx;
    qy = py - eps * ny;
    if (relate_point_in_area(qx, qy, edges, nedges) == 0)
    {
      *x = qx;
      *y = qy;
      return true;
    }
  }
  return false;
}

/**
 * @brief Determine whether the interiors of two areal geometries
 * intersect in dimension 2.
 */
static bool
relate_area_interiors_intersect(Edge **aedges, int na, Edge **bedges, int nb)
{
  /* A vertex of either geometry inside the other geometry proves
   * a two-dimensional interior/interior intersection */
  if (relate_area_has_vertex_interior(aedges, na, bedges, nb))
    return true;
  if (relate_area_has_vertex_interior(bedges, nb, aedges, na))
    return true;

  /* If a boundary portion of either geometry lies in the interior
   * of the other, the interiors necessarily overlap in area */
  for (int i = 0; i < na; i++)
  {
    const Edge *e = aedges[i];
    if (!relate_area_boundary_edge(e))
      continue;
    if (relate_area_edge_inside_area(e, bedges, nb))
      return true;
  }
  for (int i = 0; i < nb; i++)
  {
    const Edge *e = bedges[i];
    if (!relate_area_boundary_edge(e))
      continue;
    if (relate_area_edge_inside_area(e, aedges, na))
      return true;
  }

  /* Finally handle coincident boundaries / complete containment
   * where every tested vertex may lie on the other boundary.
   * Find an interior witness point of A and test it against B. */
  double x, y;
  if (relate_area_find_interior_point(aedges, na, &x, &y))
  {
    if (relate_point_in_area(x, y, bedges, nb) == 0)
      return true;
  }
  if (relate_area_find_interior_point(bedges, nb, &x, &y))
  {
    if (relate_point_in_area(x, y, aedges, na) == 0)
      return true;
  }
  return false;
}

/**
 * @brief Compute the DE-9IM matrix for two areal geometries.
 */
static void
relate_area_area(const LWGEOM *g1, const LWGEOM *g2, MeosDE9IM *m)
{
  MeosArray *a1 = geom_extract_edges(g1);
  MeosArray *a2 = geom_extract_edges(g2);
  int n1 = (int) a1->count;
  int n2 = (int) a2->count;
  Edge **e1 = palloc(sizeof(Edge *) * n1);
  Edge **e2 = palloc(sizeof(Edge *) * n2);
  for (int i = 0; i < n1; i++)
    e1[i] = (Edge *) meos_array_get(a1, i);
  for (int i = 0; i < n2; i++)
    e2[i] = (Edge *) meos_array_get(a2, i);

  /* Two-dimensional interior/interior intersection. */
  if (relate_area_interiors_intersect(e1, n1, e2, n2))
  {
    m->ii = 2;
  }

  /* Boundary(A) / Interior(B) and Interior(A) / Boundary(B) are determined by
   * splitting every boundary edge at the intersections with the other
   * boundary. */
  for (int i = 0; i < n1; i++)
  {
    if (!relate_area_boundary_edge(e1[i]))
      continue;
    relate_area_edge_intervals(e1[i], e2, n2, m, true);
  }
  for (int i = 0; i < n2; i++)
  {
    if (!relate_area_boundary_edge(e2[i]))
      continue;
    relate_area_edge_intervals(e2[i], e1, n1, m, false);
  }

  /* Boundary / Boundary.
   * Point intersections give dimension 0.
   * Coincident/overlapping boundary portions give dimension 1. */
  relate_area_boundary_points(e1, n1, e2, n2, m);

  /* Interior / Exterior, of dimension 2 because a non-empty open region is
   * two-dimensional. Two independent sources answer it, and either one alone
   * misses cases the other catches:
   * - a piece of the boundary of A running outside B, which
   *   #relate_area_edge_intervals has already recorded in BE, has a
   *   neighbourhood inside A that is outside B as well. This covers every
   *   partial overlap, where an interior witness can land inside B
   * - an interior point of A outside B covers the case where the whole
   *   boundary of A lies within B while a hole of B falls inside A */
  if (m->be == 1)
    de9im_add(&m->ie, 2);
  if (m->eb == 1)
    de9im_add(&m->ei, 2);
  double x, y;
  if (relate_area_find_interior_point(e1, n1, &x, &y) &&
      relate_point_in_area(x, y, e2, n2) == 2)
    de9im_add(&m->ie, 2);
  if (relate_area_find_interior_point(e2, n2, &x, &y) &&
      relate_point_in_area(x, y, e1, n1) == 2)
    de9im_add(&m->ei, 2);

  /* Exterior / Exterior.
   * Two ordinary finite areal geometries always have a
   * two-dimensional common exterior. */
  de9im_add(&m->ee, 2);

  pfree(e1); pfree(e2);
  meos_array_destroy(a1); meos_array_destroy(a2);
  return;
}

/*****************************************************************************
 * Main dispatcher
 *****************************************************************************/

/**
 * @brief Return the bitmask of the topological dimensions occurring in a
 * geometry
 * @details Bit @p k is set when the geometry has a component of dimension
 * @p k, so a collection mixing dimensions sets more than one bit. Testing
 * each dimension with a separate predicate cannot distinguish such a
 * collection from a homogeneous one, and routes it to whichever predicate is
 * tried first
 */
static int
relate_dim_mask(const LWGEOM *geom)
{
  if (! geom || lwgeom_is_empty(geom))
    return 0;
  switch (geom->type)
  {
    case POINTTYPE:
    case MULTIPOINTTYPE:
      return 1;
    case LINETYPE:
    case MULTILINETYPE:
    case CIRCSTRINGTYPE:
    case COMPOUNDTYPE:
    case MULTICURVETYPE:
      return 2;
    case POLYGONTYPE:
    case MULTIPOLYGONTYPE:
    case TRIANGLETYPE:
    case CURVEPOLYTYPE:
    case MULTISURFACETYPE:
      return 4;
    case COLLECTIONTYPE:
    {
      const LWCOLLECTION *col = (const LWCOLLECTION *) geom;
      int mask = 0;
      for (uint32_t i = 0; i < col->ngeoms; i++)
        mask |= relate_dim_mask(col->geoms[i]);
      return mask;
    }
    default:
      return 0;
  }
}

/**
 * @brief Return the topological dimension of the boundary of a geometry
 * @details Following the OGC rules a point geometry has an empty boundary,
 * an areal geometry has a one-dimensional boundary, and a linear geometry has
 * a zero-dimensional boundary that is empty when every component is closed,
 * as for a linear ring
 * @return The dimension, or -1 when the boundary is empty
 */
static int8_t
relate_boundary_dimension(const LWGEOM *geom)
{
  if (relate_is_areal(geom))
    return 1;
  if (! relate_is_linear(geom))
    return -1;
  MeosArray *arr = geom_extract_edges(geom);
  int nedges = (int) arr->count;
  Edge **edges = palloc(sizeof(Edge *) * (size_t) (nedges + 1));
  for (int i = 0; i < nedges; i++)
    edges[i] = (Edge *) meos_array_get(arr, i);
  int count;
  POINT2D *points = relate_linear_boundary_points(edges, nedges, &count);
  pfree(points); pfree(edges); meos_array_destroy(arr);
  return (count > 0) ? 0 : -1;
}

/**
 * @brief Compute the DE-9IM intersection matrix
 * @details This is the native MEOS counterpart of ST_Relate over the
 * geometry combinations the engine covers
 * @return true if the geometry pair is supported. A false return means the
 * pair is outside that coverage, @b not that the geometries are unrelated, so
 * a caller must answer it another way rather than read @p result
 */
bool
meos_relate(const LWGEOM *g1, const LWGEOM *g2, char result[10])
{
  assert(g1); assert(g2); assert(result);

  /* The whole engine works on the edge decomposition, so a geometry the clip
   * engine does not decompose is left to the caller */
  if (! geom_clip_supported(g1) || ! geom_clip_supported(g2))
    return false;

  MeosDE9IM m;
  de9im_init(&m);

  /* An empty operand meets nothing, so the interior and the boundary of the
   * other operand fall entirely in its exterior, each keeping its own
   * dimension. The two exteriors meet in dimension 2 */
  bool empty1 = lwgeom_is_empty(g1);
  bool empty2 = lwgeom_is_empty(g2);
  if (empty1 || empty2)
  {
    if (! empty2)
    {
      de9im_add(&m.ei, (int8_t) relate_dimension(g2));
      de9im_add(&m.eb, relate_boundary_dimension(g2));
    }
    if (! empty1)
    {
      de9im_add(&m.ie, (int8_t) relate_dimension(g1));
      de9im_add(&m.be, relate_boundary_dimension(g1));
    }
    de9im_add(&m.ee, 2);
    de9im_to_string(&m, result);
    return true;
  }

  /* A collection mixing dimensions needs each of its parts related
   * separately, which the engine does not do */
  int mask1 = relate_dim_mask(g1);
  int mask2 = relate_dim_mask(g2);
  if (mask1 == 0 || mask2 == 0 || (mask1 & (mask1 - 1)) != 0 ||
      (mask2 & (mask2 - 1)) != 0)
    return false;

  /* On the point side the engine covers a single POINT */
  if ((mask1 == 1 && g1->type != POINTTYPE) ||
      (mask2 == 1 && g2->type != POINTTYPE))
    return false;

  if (mask1 == 1 && mask2 == 1)
    relate_point_point(g1, g2, &m);
  else if (mask1 == 1 && mask2 == 2)
    relate_point_linear(g1, g2, &m);
  else if (mask1 == 2 && mask2 == 1)
    relate_linear_point(g1, g2, &m);
  else if (mask1 == 1 && mask2 == 4)
    relate_point_area(g1, g2, &m);
  else if (mask1 == 4 && mask2 == 1)
    relate_area_point(g1, g2, &m);
  else if (mask1 == 2 && mask2 == 2)
    relate_linear_linear(g1, g2, &m);
  else if (mask1 == 2 && mask2 == 4)
    relate_linear_area(g1, g2, &m);
  else if (mask1 == 4 && mask2 == 2)
    relate_area_linear(g1, g2, &m);
  else
    relate_area_area(g1, g2, &m);

  de9im_to_string(&m, result);
  return true;
}

/**
 * @brief Return true if two geometries satisfy a DE-9IM pattern
 */
bool
meos_relate_pattern(const LWGEOM *g1, const LWGEOM *g2, const char *pattern)
{
  char matrix[10];
  if (! meos_relate(g1, g2, matrix))
    return false;
  return de9im_match(matrix, pattern);
}
/*****************************************************************************/
