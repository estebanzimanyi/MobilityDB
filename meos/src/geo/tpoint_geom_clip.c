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
 * @brief Fast 2D/3D temporal point clipping against 2D geometries
 * @details Support (multi)point, (multi)line, triangle, (multi)polygons
 * with holes and islands inside holes (and recursion) ,and collection of the
 * above
 * @note Avoid processing in GEOS since it is too slow
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
  double x, y, z;  /**< Coordinates of the 3D point */
  TimestampTz t;   /**< Timestamp */
} TrajPoint;

/**
 * @brief Structure keeping a geometry edge
 */
typedef struct
{
  double x1, y1, x2, y2; /**< Coordinates of the start and end 2D points */
  int ring_type;         /**< +1 outer, -1 hole, etc */
  int delta;             /**< Scanline crossing contribution */
} Edge;

/**
 * @brief Structure keeping an intersection event 
 */
typedef struct
{
  double t;   /**< Fraction in [0,1] where the intersection occurs */
  int delta;  /**< Delta */
} Event;

/**
 * @brief Structure keeping a double interval
 */
typedef struct
{
  double t0, t1;   /**< Fractions in [0,1] where the clip occurs */
} ClipInterval;
 
/*****************************************************************************
 * Extract geometry edges
 *****************************************************************************/

/**
 * @brief Add to the dynamic array in the last argument the edges obtained
 * from a ring
 */
static void
emit_ring_edges(const POINTARRAY *pa, int ring_type, MeosArray *edges)
{
  const POINT4D *pts = (const POINT4D *) getPoint_internal(pa, 0);
  for (int i = 0; i < (int) pa->npoints - 1; i++)
  {
    const POINT4D *a = &pts[i];
    const POINT4D *b = &pts[i + 1];

    /* Skip horizontal edges as in PostGIS */
    if (fabs(b->y - a->y) < POSTGIS_FP_TOLERANCE)
      continue; 

    Edge e;
    e.x1 = a->x;
    e.y1 = a->y;
    e.x2 = b->x;
    e.y2 = b->y;
    e.ring_type = ring_type;
    e.delta = (b->y > a->y) ? +1 : -1;
    meos_array_add(edges, &e);
  }
  return;
}

/**
 * @brief Add to the dynamic array in the last argument the edges obtained
 * from a point
 * @details Since a linestring is NOT polygonal -> ring_type = 0
 */
static void
extract_point(const LWPOINT *pt, MeosArray *edges)
{
  const POINT4D *p = (const POINT4D *) getPoint_internal(pt->point, 0);
  Edge e;
  e.x1 = p->x;
  e.y1 = p->y;
  e.x2 = p->x;
  e.y2 = p->y;
  e.ring_type = 0;
  e.delta = 0;
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
  {
    extract_point((const LWPOINT *) mp->geoms[i], edges);
  }
  return;
}

/**
 * @brief Add to the dynamic array in the last argument the segments obtained
 * from a line
 * @details Since a linestring is NOT polygonal -> ring_type = 0
 */
static void
extract_line(const LWLINE *line, MeosArray *edges)
{
  const POINTARRAY *pa = line->points;
  emit_ring_edges(pa, 0, edges);
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
  {
    const LWLINE *line = (const LWLINE *) ml->geoms[i];
    extract_line(line, edges);
  }
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
  {
    const POINTARRAY *pa = poly->rings[r];
    int ring_type = (r == 0) ? +1 : -1;
    emit_ring_edges(pa, ring_type, edges);
  }
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
  {
    const LWPOLY *poly = (const LWPOLY *) mp->geoms[i];
    extract_poly(poly, edges);
  }
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
  const POINTARRAY *pa = tri->points;
  emit_ring_edges(pa, +1, edges);
  return;
}

/**
 * @brief Return the edges of a geometry in a dynamic array (iterator)
 */
static void
geom_extract_edges_iter(const LWGEOM *geom, MeosArray *edges)
{
  if (! geom)
    return;

  switch (geom->type)
  {
    /* ---------------- Point ---------------- */
    case POINTTYPE:
      extract_point((const LWPOINT *) geom, edges);
      break;

    case MULTIPOINTTYPE:
      extract_mpoint((const LWMPOINT *) geom, edges);
      break;

    /* ---------------- Line ---------------- */
    case LINETYPE:
      extract_line((const LWLINE *) geom, edges);
      break;

    case MULTILINETYPE:
      extract_mline((const LWMLINE *) geom, edges);
      break;

    /* ---------------- Polygon ---------------- */
    case POLYGONTYPE:
      extract_poly((const LWPOLY *) geom, edges);
      break;

    case MULTIPOLYGONTYPE:
      extract_mpoly((const LWMPOLY *) geom, edges);
      break;

    case TRIANGLETYPE:
      extract_triangle((const LWTRIANGLE *) geom, edges);
      break;

    /* ---------------- Collection ---------------- */
    case COLLECTIONTYPE:
    {
      const LWCOLLECTION *col = (const LWCOLLECTION *) geom;
      for (int i = 0; i < (int) col->ngeoms; i++)
      {
        const LWGEOM *sub = col->geoms[i];
        geom_extract_edges_iter(sub, edges);
      }
      break;
    }

    /* ---------------- Unsupported geometry type ---------------- */
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
  MeosArray *edges = meos_array_init(sizeof(Edge));
  geom_extract_edges_iter(geom, edges);
  return edges;
}

/*****************************************************************************
 * Segment intersection
 *****************************************************************************/

/**
 * @brief Return the double obtained by clamping a double to [0,1]
 */
static inline double
clamp01(double t)
{
  return t < 0 ? 0 : (t > 1 ? 1 : t);
}

// /**
 // * @brief Return true if a segment defined by two points intersects an edge,
 // * false otherwise
 // * @param[in] a,b Points defining the segment
 // * @param[in] e Edge
 // * @param[out] t Fraction in [0,1] determining the intersection point 
 // */
// static int
// linesegm_intersect_old(TrajPoint a, TrajPoint b, const Edge *e, double *t0,
  // double *t1)
// {
  // double dx = b.x - a.x;
  // double dy = b.y - a.y;
  // double ex = e->x2 - e->x1;
  // double ey = e->y2 - e->y1;
  // double cx = e->x1 - a.x;
  // double cy = e->y1 - a.y;

  // /* Determinant */
  // double det = dx * (-ey) + dy * ex;

  // /* Case 1: Parallel or collinear */
  // if (fabs(det) < POSTGIS_FP_TOLERANCE)
  // {
    // /* Not collinear -> reject */
    // if (fabs(dx * cy - dy * cx) > POSTGIS_FP_TOLERANCE)
      // return 0;

    // /* Choose dominant axis for stability */
    // double tA0, tA1;
    // if (fabs(dx) >= fabs(dy))
    // {
      // if (fabs(dx) < POSTGIS_FP_TOLERANCE)
        // return 0;
      // tA0 = (e->x1 - a.x) / dx;
      // tA1 = (e->x2 - a.x) / dx;
    // }
    // else
    // {
      // if (fabs(dy) < POSTGIS_FP_TOLERANCE)
        // return 0;
      // tA0 = (e->y1 - a.y) / dy;
      // tA1 = (e->y2 - a.y) / dy;
    // }

    // if (tA0 > tA1)
    // {
      // double tmp = tA0;
      // tA0 = tA1;
      // tA1 = tmp;
    // }

    // /* Clamp BEFORE comparison to avoid FP noise */
    // double lo = fmax(0.0, tA0);
    // double hi = fmin(1.0, tA1);
    // if (hi + POSTGIS_FP_TOLERANCE < lo)
      // return 0;

    // *t0 = clamp01(lo);
    // *t1 = clamp01(hi);
    // return (hi > lo) ? 2 : 1; /* OVERLAP : INTERSECT */
  // }

   // /* Case 2: Proper segment intersection */
  // double inv_det = 1.0 / det;

  // double t = (cx * (-ey) + cy * ex) * inv_det;
  // double u = (cx * dy - cy * dx) * inv_det;

  // /* Robust acceptance with tolerance */
  // if (t < -POSTGIS_FP_TOLERANCE || t > 1.0 + POSTGIS_FP_TOLERANCE ||
      // u < -POSTGIS_FP_TOLERANCE || u > 1.0 + POSTGIS_FP_TOLERANCE)
    // return 0;

  // /* Clamp to valid segment range */
  // t = clamp01(t);
  // u = clamp01(u);

  // *t0 = t;
  // *t1 = u;
  // return 1;
// }









static inline int
linesegm_intersect(double ax, double ay, double bx, double by,
  double cx, double cy, double dx, double dy, double *t_out, double *u_out)
{
  /* --- vectors --- */
  double r_x = bx - ax;
  double r_y = by - ay;
  double s_x = dx - cx;
  double s_y = dy - cy;

  double qpx = cx - ax;
  double qpy = cy - ay;

  /* --- orientation / cross products --- */
  double rxs = r_x * s_y - r_y * s_x;
  double qpxr = qpx * r_y - qpy * r_x;

  // double qpxs = qpx * s_y - qpy * s_x;

  /* =========================================================
   * CASE 1: COLLINEAR OR NEAR-COLLINEAR
   * ========================================================= */
  if (fabs(rxs) < POSTGIS_FP_TOLERANCE)
  {
    /* Parallel case */
    if (fabs(qpxr) > POSTGIS_FP_TOLERANCE)
      return 0; /* not collinear → no intersection */

    /* Collinear: project onto dominant axis */
    double r2 = r_x * r_x + r_y * r_y;

    if (r2 < POSTGIS_FP_TOLERANCE)
      return 0; /* degenerate segment */

    double t0 = (qpx * r_x + qpy * r_y) / r2;
    double t1 = t0 + (s_x * r_x + s_y * r_y) / r2;

    /* order */
    if (t0 > t1)
    {
      double tmp = t0; t0 = t1; t1 = tmp;
    }

    /* intersection exists if overlap */
    if (t1 < 0 || t0 > 1)
      return 0;

    /* clamp */
    if (t_out) *t_out = t0 < 0 ? 0 : (t0 > 1 ? 1 : t0);
    if (u_out) *u_out = 0; /* undefined in collinear case */

    return 1;
  }

  /* =========================================================
   * CASE 2: STANDARD INTERSECTION
   * ========================================================= */

  double t = (qpx * s_y - qpy * s_x) / rxs;
  double u = (qpxr) / rxs;

  /* --- epsilon normalization --- */
  if (fabs(t) < POSTGIS_FP_TOLERANCE) t = 0;
  if (fabs(u) < POSTGIS_FP_TOLERANCE) u = 0;

  if (fabs(t - 1.0) < POSTGIS_FP_TOLERANCE) t = 1;
  if (fabs(u - 1.0) < POSTGIS_FP_TOLERANCE) u = 1;

  /* --- rejection --- */
  if (t < 0 || t > 1 || u < 0 || u > 1)
    return 0;

  if (t_out) *t_out = t;
  if (u_out) *u_out = u;

  return 1;
}









/*****************************************************************************
 * Events
 *****************************************************************************/

/**
 * @brief Return true if the edges come from a polygon, false otherwise
 */
static bool
edges_are_polygonal(const Edge *edges, int n)
{
  for (int i = 0; i < n; i++)
  {
    if (edges[i].ring_type != 0)
      return true;
  }
  return false;
}

/**
 * @brief Build the events obtained by computing the intersections between a
 * segment and an array of geometry edges
 * @param[in] a,b Points defining the segment
 * @param[in] e Array of geometry edges
 * @param[in] n Number of elements in the array
 * @param[inout] Dynamic array of the collected events 
 */
// static void
// build_events(TrajPoint a, TrajPoint b, const Edge *edges, int n,
  // MeosArray *events)
// {
  // for (int i = 0; i < n; i++)
  // {
    // double t0, t1;
    // int type = linesegm_intersect(a, b, &edges[i], &t0, &t1);
    // if (! type)
      // continue;
    // if (type == 1)
    // {
      // /* single intersection */
      // Event e = {t0, edges[i].delta};
      // meos_array_add(events, &e);
    // }
    // else /* type == 2 */
    // {
      // if (t0 > t1)
      // {
        // double tmp = t0;
        // t0 = t1;
        // t1 = tmp;
      // }

      // /* Skip degenerate overlaps ONLY here */
      // if (fabs(t1 - t0) < POSTGIS_FP_TOLERANCE)
        // continue;

      // /* Overlap: always use +1 / -1 */
      // Event e1 = {t0, +1};
      // Event e2 = {t1, -1};
      // meos_array_add(events, &e1);
      // meos_array_add(events, &e2);
    // }
  // }
  // return;
// }

static void
build_events(
    TrajPoint a,
    TrajPoint b,
    const Edge *edges,
    int n,
    MeosArray *events)
{
  const double EPS = 1e-14;

  for (int i = 0; i < n; i++)
  {
    double t, u;

    /* robust intersection kernel */
    int ok = linesegm_intersect(
        a.x, a.y,
        b.x, b.y,
        edges[i].x1, edges[i].y1,
        edges[i].x2, edges[i].y2,
        &t, &u);

    if (!ok)
      continue;

    /* normalize (CRITICAL for sweep stability) */
    if (fabs(t) < EPS) t = 0;
    if (fabs(u) < EPS) u = 0;
    if (fabs(t - 1) < EPS) t = 1;
    if (fabs(u - 1) < EPS) u = 1;

    /* -----------------------------------------------------
     * CASE 1: POINT INTERSECTION (t == u)
     * ----------------------------------------------------- */
    if (fabs(t - u) < EPS)
    {
      Event e = {t, edges[i].delta};
      meos_array_add(events, &e);
      continue;
    }

    /* -----------------------------------------------------
     * CASE 2: SEGMENT OVERLAP (interval on param axis)
     * ----------------------------------------------------- */
    double t0 = t;
    double t1 = u;

    if (t0 > t1)
    {
      double tmp = t0;
      t0 = t1;
      t1 = tmp;
    }

    /* skip degenerate overlaps */
    if (fabs(t1 - t0) < EPS)
    {
      Event e = {t0, edges[i].delta};
      meos_array_add(events, &e);
      continue;
    }

    /* clamp for sweep stability */
    if (t0 < 0) t0 = 0;
    if (t1 > 1) t1 = 1;

    /* overlap becomes +1 / -1 event pair */
    Event e1 = {t0, +1};
    Event e2 = {t1, -1};

    meos_array_add(events, &e1);
    meos_array_add(events, &e2);
  }
}


/**
 * @brief Comparator for intersecting events
 */
static int
event_cmp(const void *a, const void *b)
{
  const Event *e1 = a;
  const Event *e2 = b;

  if (e1->t < e2->t) return -1;
  if (e1->t > e2->t) return 1;
  return e2->delta - e1->delta;
}

/*****************************************************************************
 * Sweep XY -> intervals (with initial winding)
 *****************************************************************************/

/**
 * @brief Compute the intervals during which an intersecting event occur
 * @param[in]
 * @param[inout]
 */
static void
sweep_line(MeosArray *events, MeosArray *intervals)
{
  Event *ev = events->elems;
  int n = events->count;

  if (n == 0)
    return;

  qsort(ev, n, sizeof(Event), event_cmp);

  int coverage = 0;
  double start = 0.0;

  for (int i = 0; i < n; i++)
  {
    int prev = coverage;
    coverage += ev[i].delta;

    if (prev == 0 && coverage > 0)
    {
      start = ev[i].t;
    }
    else if (prev > 0 && coverage == 0)
    {
      ClipInterval in = {start, ev[i].t};
      meos_array_add(intervals, &in);
    }
  }
}

/*****************************************************************************
 * Sweep XY -> intervals (with initial winding)
 *****************************************************************************/

/**
 * @brief Compute the intervals during which an intersecting event occur
 * @param[in]
 * @param[inout]
 */
static void
sweep_xy(MeosArray *events, MeosArray *intervals, int initial_coverage)
{
  Event *ev = events->elems;
  int n = events->count;

  qsort(ev, n, sizeof(Event), event_cmp);

  int coverage = initial_coverage;
  double start = 0.0;
  bool inside = (coverage != 0);
  if (inside)
    start = 0.0;

  for (int i = 0; i < n; i++)
  {
    int prev = coverage;
    coverage += ev[i].delta;
    /* Entering region */
    if (prev == 0 && coverage != 0)
    {
      start = ev[i].t;
      inside = true;
    }
    /* Leaving region */
    else if (prev != 0 && coverage == 0)
    {
      ClipInterval in = {start, ev[i].t};
      meos_array_add(intervals, &in);
      inside = false;
    }
  }
  /* Close trailing interval if still inside */
  if (inside)
  {
    ClipInterval in = {start, 1.0};
    meos_array_add(intervals, &in);
  }
  return;
}


/*****************************************************************************
 * Clip a temporal sequence
 *****************************************************************************/

/**
 * @brief Interpolate a trajectory point from two trajectory points and a
 * factor in [0,1]
 */
TrajPoint
trajpoint_interpolate(TrajPoint a, TrajPoint b, double u)
{
  TrajPoint p;
  p.x = a.x + u * (b.x - a.x);
  p.y = a.y + u * (b.y - a.y);
  p.z = a.z + u * (b.z - a.z);
  p.t = a.t + (TimestampTz) llround((b.t - a.t) * u);
  return p;
}

/**
 * @brief Return the initial winding of a 2D point wrt an array of edges
 */
static int
initial_winding(double x, double y, const Edge *edges, int nedges)
{
  int winding = 0;
  for (int i = 0; i < nedges; i++)
  {
    const Edge *e = &edges[i];

    /* ignore horizontal edges as in PostGIS  */
    if (fabs(e->y1 - e->y2) < POSTGIS_FP_TOLERANCE)
      continue;

    /* check if scanline crosses edge */
    if ((e->y1 <= y && e->y2 > y) || (e->y2 <= y && e->y1 > y))
    {
      double dy = (e->y2 - e->y1);
      if (fabs(dy) < POSTGIS_FP_TOLERANCE)
        continue;
      double xint =
        e->x1 + (y - e->y1) * (e->x2 - e->x1) / dy;
      if (x < xint)
        winding += (e->ring_type > 0) ? 1 : -1;
    }
  }
  return winding;
}

/**
 * @brief Return true if two trajectory points are similar
 */
static bool
trajpoint_same(const TrajPoint *a, const TrajPoint *b)
{
  return fabs(a->x - b->x) < POSTGIS_FP_TOLERANCE &&
         fabs(a->y - b->y) < POSTGIS_FP_TOLERANCE &&
         fabs(a->z - b->z) < POSTGIS_FP_TOLERANCE &&
         a->t == b->t;
}

/**
 * @brief Add a trajectory point to the dynamic array while avoiding to add
 * duplicate points
 */
static void
add_new_point(MeosArray *arr, const TrajPoint *p)
{
  if (arr->count == 0)
  {
    meos_array_add(arr, (void *) p);
    return;
  }

  /* Skip duplicates, that is, same point + same timestamp*/
  TrajPoint *last = &((TrajPoint *)arr->elems)[arr->count - 1];
  if (trajpoint_same(last, (void *) p))
    return;

  /* Enforce strictly increasing timestamps */
  if (p->t <= last->t)
    return;

  meos_array_add(arr, (void *) p);
  return;
}

/**
 * @brief Return a trajectory point from a 2D/3D temporal point
 */
static int
trajpoint_time_cmp(const void *a, const void *b)
{
  const TrajPoint *p1 = a;
  const TrajPoint *p2 = b;

  if (p1->t < p2->t) return -1;
  if (p1->t > p2->t) return 1;
  return 0;
}

/**
 * @brief Construct a temporal sequence from a sequence of trajectory points
 */
static TSequence *
tsequence_from_trajpoints(TrajPoint *pts, int n, int32_t srid, bool hasz,
  meosType temptype)
{

  TInstant **instants = palloc(sizeof(TInstant *) * n);
  for (int i = 0; i < n; i++)
  {
    LWPOINT *pt = hasz ?
      lwpoint_make3dz(srid, pts[i].x, pts[i].y, pts[i].z) :
      lwpoint_make2d(srid, pts[i].x, pts[i].y);
    GSERIALIZED *gs = geo_serialize((LWGEOM *) pt);
    instants[i] = tinstant_make(PointerGetDatum(gs), temptype, pts[i].t);
    lwpoint_free(pt);
  }
  return tsequence_make_free(instants, n, true, true, LINEAR, NORMALIZE);
}

/**
 * @brief Construct a temporal sequence from a sequence of trajectory points
 */
static void
sequence_flush(MeosArray *tpts, MeosArray *sequences, int32_t srid, bool hasz,
  int temptype)
{
  if (tpts->count < 2)
  {
    tpts->count = 0;
    return;
  }
  /* Sort the trajectory points */
  qsort(tpts->elems, tpts->count, sizeof(TrajPoint), trajpoint_time_cmp);
  /* Build the temporal sequence */
  TSequence *s = tsequence_from_trajpoints(tpts->elems, tpts->count, srid,
    hasz, temptype);
  meos_array_add(sequences, &s);
  tpts->count = 0;
}

/**
 * @brief Return a trajectory point from a 2D/3D temporal point
 */
static inline TrajPoint
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
 * geometry
 * @param[in] seq Temporal sequence
 * @param[in] edges Array of geometry edges
 * @param[in] nedges Number of edges in the array
 */
static TSequenceSet *
tpointseq_clip(const TSequence *seq, const Edge *edges, int nedges)
{
  assert(MEOS_FLAGS_GET_INTERP(seq->flags) == LINEAR);
  assert(! MEOS_FLAGS_GET_GEODETIC(seq->flags));

  /* Get spatial characteristics of the points */
  int32_t srid = tspatial_srid((Temporal *) seq);
  bool hasz = MEOS_FLAGS_GET_Z(seq->flags);
  /* Initialize the dynamic arrays needed in the processing */
  MeosArray *sequences = meos_array_init(sizeof(TSequence *));
  MeosArray *tpts      = meos_array_init(sizeof(TrajPoint));
  MeosArray *events    = meos_array_init(sizeof(Event));
  MeosArray *intervals = meos_array_init(sizeof(ClipInterval));

  /* Loop for all trajectory segments */
  TrajPoint a = tinstant_to_trajpoint(TSEQUENCE_INST_N(seq, 0));
  for (int i = 1; i < seq->count; i++)
  {
    TrajPoint b = tinstant_to_trajpoint(TSEQUENCE_INST_N(seq, i));

    /* Restart the arrays */
    events->count = 0;
    intervals->count = 0;

    /* XY clipping */
    build_events(a, b, edges, nedges, events);
    if (edges_are_polygonal(edges, nedges))
    {
      int init = initial_winding(a.x, a.y, edges, nedges);
      sweep_xy(events, intervals, init);
    }
    else
    {
      sweep_line(events, intervals);
    }
    /* If no intersection */
    if (intervals->count == 0)
    {
      sequence_flush(tpts, sequences, srid, hasz, seq->temptype);
      a = b;
      continue;
    }

    ClipInterval *arr = intervals->elems;
    for (int j = 0; j < (int) intervals->count; j++)
    {
      double t0 = arr[j].t0;
      double t1 = arr[j].t1;

      /* Skip degenerate intervals */
      if (fabs(t1 - t0) < POSTGIS_FP_TOLERANCE)
        continue;

      TrajPoint p0 = trajpoint_interpolate(a, b, t0);
      TrajPoint p1 = trajpoint_interpolate(a, b, t1);

      /* Start new sequence if needed  */
      if (tpts->count == 0)
      {
        add_new_point(tpts, &p0);
      }
      else
      {
        TrajPoint *last = &((TrajPoint *)tpts->elems)[tpts->count - 1];
        /* Discontinuity -> flush */
        if (! trajpoint_same(last, &p0))
        {
          sequence_flush(tpts, sequences, srid, hasz, seq->temptype);
          add_new_point(tpts, &p0);
        }
      }
      add_new_point(tpts, &p1);
    }
    a = b;
  }

  /* Final flush */
  sequence_flush(tpts, sequences, srid, hasz, seq->temptype);

  if (sequences->count == 0)
  {
    meos_array_destroy(tpts, false);
    meos_array_destroy(events, false);
    meos_array_destroy(intervals, false);
    meos_array_destroy(sequences, false);
    return NULL;
  }

  TSequenceSet *result = tsequenceset_make_free(sequences->elems,
    sequences->count, NORMALIZE);

  meos_array_destroy(tpts, false);
  meos_array_destroy(events, false);
  meos_array_destroy(intervals, false);
  meos_array_destroy(sequences, false);

  return result;
}

/**
 * @brief Return a temporal point sequence with linear interpolation
 * restricted to a geometry
 * @details For performance reasons we avoid the call to ST_Intersection
 * which delegates the computation to GEOS. 
 * @pre The arguments have the same SRID, the geometry is 2D and is not empty.
 * This is verified in #tgeo_restrict_geom
 */
TSequenceSet *
tpointseq_linear_at_geom(const TSequence *seq, const GSERIALIZED *gs)
{
  assert(MEOS_FLAGS_LINEAR_INTERP(seq->flags)); assert(seq->count > 1);
  assert(! gserialized_is_empty(gs)); 
  assert(! MEOS_FLAGS_GET_GEODETIC(seq->flags));

  /* Bounding box test */
  STBox box1, box2;
  tspatialseq_set_stbox(seq, &box1);
  /* Non-empty geometries have a bounding box */
  geo_set_stbox(gs, &box2);
  if (! overlaps_stbox_stbox(&box1, &box2))
    return NULL;

  /* Perform the clipping */
  LWGEOM *geom = lwgeom_from_gserialized(gs);
  MeosArray *edges = geom_extract_edges(geom);
  TSequenceSet *result = tpointseq_clip(seq, edges->elems, edges->count);
  /* Clean up and return */
  lwgeom_free(geom);  
  meos_array_destroy(edges, true);
  return result;  
}

/*****************************************************************************/
