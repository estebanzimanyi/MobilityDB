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
 * @brief PostGIS functions implemented in MEOS natively to improve performance
 * or to avoid polygonalization of circular strings
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
#include "temporal/temporal.h"
#include "geo/geo_funcs.h"
#include "geo/postgis_funcs.h"
#include "geo/tgeo_spatialfuncs.h"

/* Minimum number of edges to use an R-tree index in order to compensate the
 * overhead of the tree construction and destruction */
#define RTREE_MIN_NUMBER_ELEMS 100

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
MeosArray *
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
RTree *
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
 * Create a circle
 *****************************************************************************/

/* The following function is not exported in PostGIS */
extern LWCIRCSTRING *lwcircstring_from_lwpointarray(int32_t srid,
  uint32_t npoints, LWPOINT **points);

/**
 * @brief Return a circle created from a central point and a radius
 */
LWGEOM *
lwcircle_make(double x, double y, double radius, int32_t srid)
{
  assert(radius > 0);
  LWPOINT *points[3];
  /* Shift the X coordinate of the point by +- radius */
  points[0] = lwpoint_make2d(srid, x - radius, y);
  points[1] = lwpoint_make2d(srid, x + radius, y);
  points[2] = lwpoint_make2d(srid, x - radius, y);
  /* Construct the circle */
  LWGEOM *ring = lwcircstring_as_lwgeom(
    lwcircstring_from_lwpointarray(srid, 3, points));
  LWCURVEPOLY *result = lwcurvepoly_construct_empty(srid, 0, 0);
  lwcurvepoly_add_ring(result, ring);
  /* Clean up and return */
  lwpoint_free(points[0]); lwpoint_free(points[1]); lwpoint_free(points[2]);
  /* We cannot lwgeom_free(ring); */
  return lwcurvepoly_as_lwgeom(result);
}

/**
 * @brief Return a circle created from a central point and a radius
 */
GSERIALIZED *
geocircle_make(double x, double y, double radius, int32_t srid)
{
  LWGEOM *res = lwcircle_make(x, y, radius, srid);
  GSERIALIZED *result = geo_serialize(res);
  lwgeom_free(res);
  return result;
}

/*****************************************************************************
 * Minimum Enclosing Circle implementation improving the performance of the
 * PostGIS function ST_MinimumBoundingCircle
 *****************************************************************************/

/**
 * @brief Definition of the 2D circle structure
 * @note Equivalent of PostGIS structure LWBOUNDINGCIRCLE
 */
typedef struct
{
  POINT2D center;
  double radius;
} Circle;

/**
 * @brief Return the distance between two points
 */
static inline double
distance_point2d(POINT2D a, POINT2D b)
{
  double dx = a.x - b.x;
  double dy = a.y - b.y;
  return sqrt(dx * dx + dy * dy);
}

/**
 * @brief Return true if a point is inside a circle
 */
static inline bool
mec_inside(POINT2D p, Circle c)
{
  return distance_point2d(p, c.center) <= c.radius + 1e-12;
}

/**
 * @brief Circle constructor for 2 points
 */
static inline Circle
mec_circle2(POINT2D a, POINT2D b)
{
  Circle c;
  c.center.x = (a.x + b.x) * 0.5;
  c.center.y = (a.y + b.y) * 0.5;
  c.radius = distance_point2d(a,b) * 0.5;
  return c;
}

/**
 * @brief Circle constructor for 3 points
 */
static Circle
mec_circle3(POINT2D a, POINT2D b, POINT2D c)
{
  double A = b.x - a.x;
  double B = b.y - a.y;
  double C = c.x - a.x;
  double D = c.y - a.y;
  double E = A * (a.x + b.x) + B * (a.y + b.y);
  double F = C * (a.x + c.x) + D * (a.y + c.y);
  double G = 2.0 * (A * (c.y - b.y) - B * (c.x - b.x));

  /* Zero-init so the early-exit return doesn't leave circ.center
   * uninitialised — cppcheck flags this as `uninitvar`, and a downstream
   * caller that ignored circ.radius == -1 would read garbage. */
  Circle circ = { .center = {0.0, 0.0}, .radius = 0.0 };
  if (fabs(G) < 1e-12)
  {
    circ.radius = -1;
    return circ;
  }
  circ.center.x = (D * E - B * F) / G;
  circ.center.y = (A * F - C * E) / G;
  circ.radius = distance_point2d(circ.center,a);
  return circ;
}

/**
 * @brief Return the minimum enclosing circle for 3 points, handling the
 * collinear case by falling back to the best 2-point circle
 */
static Circle
mec_circle3_safe(POINT2D a, POINT2D b, POINT2D c)
{
  Circle circ = mec_circle3(a, b, c);
  if (circ.radius >= 0)
    return circ;
  /* Collinear: pick the largest 2-point circle */
  Circle c1 = mec_circle2(a, b);
  Circle c2 = mec_circle2(a, c);
  Circle c3 = mec_circle2(b, c);
  if (c2.radius > c1.radius) c1 = c2;
  if (c3.radius > c1.radius) c1 = c3;
  return c1;
}

/**
 * @brief Iterative Welzl algorithm for the minimum enclosing circle
 * @details Equivalent to the recursive Welzl algorithm but uses constant
 * stack space. The three nested loops correspond to the three levels of
 * recursion (boundary set size 0, 1, 2). Despite appearing O(n^3), the
 * expected runtime is O(n) with random shuffling.
 * @param[in] P Array of points (must be shuffled beforehand)
 * @param[in] n Number of points
 * @pre n >= 1
 */
static Circle
mec_welzl(POINT2D *P, int n)
{
  Circle C = (Circle){P[0], 0};
  for (int i = 1; i < n; i++)
  {
    if (! mec_inside(P[i], C))
    {
      C = (Circle){P[i], 0};
      for (int j = 0; j < i; j++)
      {
        if (! mec_inside(P[j], C))
        {
          C = mec_circle2(P[i], P[j]);
          for (int k = 0; k < j; k++)
          {
            if (! mec_inside(P[k], C))
              C = mec_circle3_safe(P[i], P[j], P[k]);
          }
        }
      }
    }
  }
  return C;
}

/**
 * @brief Extract coordinates from LWGEOM
 * @pre The geometry type is one of the supported types as given by function
 * #lwgeom_mec_supported_type
 */
static void
lwgeom_collect_points(const LWGEOM *geom, MeosArray *array)
{
  POINT2D point;
  if (geom->type == POINTTYPE)
  {
    const LWPOINT *p = (LWPOINT *) geom;
    const POINT2D *pt = getPoint2d_cp(p->point, 0);
    point = (POINT2D){pt->x, pt->y};
    meos_array_add(array, &point);
  }
  else if (geom->type == LINETYPE)
  {
    const LWLINE *l = (LWLINE *) geom;
    for (int i = 0; i < (int) l->points->npoints; i++)
    {
      const POINT2D *pt = getPoint2d_cp(l->points, i);
      point = (POINT2D){pt->x, pt->y};
      meos_array_add(array, &point);
    }
  }
  else if (geom->type == TRIANGLETYPE)
  {
    const LWTRIANGLE *tr = (LWTRIANGLE *) geom;
    for (int i = 0; i < (int) tr->points->npoints; i++)
    {
      const POINT2D *pt = getPoint2d_cp(tr->points, i);
      point = (POINT2D){pt->x, pt->y};
      meos_array_add(array, &point);
    }
  }
  else if (geom->type == POLYGONTYPE)
  {
    const LWPOLY *poly = (LWPOLY *) geom;
    for (int r = 0; r < (int) poly->nrings; r++)
    {
      POINTARRAY *pa = poly->rings[r];
      for (int i = 0; i < (int) pa->npoints; i++)
      {
        const POINT2D *pt = getPoint2d_cp(pa, i);
        point = (POINT2D){pt->x, pt->y};
        meos_array_add(array, &point);
      }
    }
  }
  else if (lwgeom_is_collection(geom))
  {
    const LWCOLLECTION *col = (LWCOLLECTION *) geom;
    for (int i = 0; i < (int) col->ngeoms; i++)
      lwgeom_collect_points(col->geoms[i], array);
  }
  return;
}

/**
 * @brief Computation of the Minimum Enclosing Circle
 * @pre The geometry is not empty and is one of the supported gemetry types
 */
static Circle
lwgeom_mec(const LWGEOM *geom)
{
  MeosArray *array = meos_array_create(sizeof(POINT2D));
  lwgeom_collect_points(geom, array);
  /* Ensure that there is at least one point given the precondition */
  assert(array->count > 0);
  /* Reuse the array->elems array that contains the points */
  POINT2D *pts = (POINT2D *) array->elems;

  /* Fisher-Yates shuffle */
  for (int i = array->count - 1; i > 0; i--)
  {
    int j = rand() % (i + 1);
    POINT2D tmp = pts[i];
    pts[i] = pts[j];
    pts[j] = tmp;
  }
  Circle result = mec_welzl(pts, array->count);
  meos_array_destroy(array);
  return result;
}

/**
 * @brief Return true if the geometry type is one of the supported types
 * for the MEOS fast Minimum Bounding Circle
 */
static bool
lwgeom_mec_supported_type(const LWGEOM *geom)
{
  if (geom->type == POINTTYPE || geom->type == LINETYPE ||
      geom->type == TRIANGLETYPE || geom->type == POLYGONTYPE)
    return true;
  else if (lwgeom_is_collection(geom))
  {
    const LWCOLLECTION *col = (LWCOLLECTION *) geom;
    for (int i = 0; i < (int) col->ngeoms; i++)
    {
      if (! lwgeom_mec_supported_type(col->geoms[i]))
        return false;
    }
    return true;
  }
  else
    return false;
}

/**
 * @ingroup meos_geo_base_spatial
 * @brief Return the center point and radius of the smallest circle that
 * contains a geometry
 * @param[in] geom Geometry
 * @param[out] radius Radius
 * @note The corresponding PostGIS function ST_MinimumBoundingCircle is much
 * slower despite it uses the same algorithm
 *   Welzl, Emo (1991), "Smallest enclosing disks (balls and elipsoids)."
 *   New Results and Trends in Computer Science (H. Maurer, Ed.), Lecture Notes
 *   in Computer Science, 555 (1991) 359-370.
 */
GSERIALIZED *
geom_min_bounding_radius(const GSERIALIZED *geom, double *radius)
{
  /* Ensure the validity of the arguments */
  VALIDATE_NOT_NULL(geom, NULL); VALIDATE_NOT_NULL(radius, NULL);

  LWGEOM *input = lwgeom_from_gserialized(geom);
  LWGEOM *center;

  if (lwgeom_is_empty(input))
  {
    center = (LWGEOM *) lwpoint_construct_empty(input->srid, LW_FALSE, LW_FALSE);
    *radius = 0;
  }
  else if (lwgeom_mec_supported_type(input))
  {
    Circle c = lwgeom_mec(input);
    center = (LWGEOM *) lwpoint_make2d(input->srid, c.center.x, c.center.y);
    *radius = c.radius;
  }
  else
  {
    LWBOUNDINGCIRCLE *mbc = lwgeom_calculate_mbc(input);
    if (! (mbc && mbc->center))
    {
      meos_error(ERROR, MEOS_ERR_INTERNAL_ERROR,
        "Error calculating minimum bounding circle");
      lwgeom_free(input);
      return NULL;
    }
    center = (LWGEOM *) lwpoint_make2d(input->srid, mbc->center->x,
      mbc->center->y);
    *radius = mbc->radius;
    lwboundingcircle_destroy(mbc);
  }

  GSERIALIZED *result = geo_serialize(center);
  lwgeom_free(center);
  lwgeom_free(input);
  return result;
}

/*****************************************************************************
 * Oriented envelope (a.k.a minimum rotated rectangle) and convex hull.
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
  assert(e); assert(points); assert(npoints);
  add_point(points, npoints, e->x1, e->y1);
  if (fabs(e->x2 - e->x1) > FP_TOLERANCE || fabs(e->y2 - e->y1) > FP_TOLERANCE)
    add_point(points, npoints, e->x2, e->y2);

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
convex_hull_points(const POINT2D *points, uint32_t npoints, POINT2D **hull)
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
      nhull--;
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
 * @brief Construct a POINT/LINESTRING/POLYGON from a set of points
 */
static LWGEOM *
make_geometry_points(int32_t srid, const POINT2D *points, uint32_t nhull)
{
  /* Point */
  if (nhull == 1)
    return lwpoint_as_lwgeom(lwpoint_make2d(srid, points[0].x, points[0].y));

  /* Line */
  if (nhull == 2)
  {
    POINTARRAY *pa = ptarray_construct_empty(0, 0, 2);
    POINT4D p;
    p.z = 0.0;
    p.m = 0.0;
    p.x = points[0].x;
    p.y = points[0].y;
    ptarray_append_point(pa, &p, LW_TRUE);
    p.x = points[1].x;
    p.y = points[1].y;
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
    p.x = points[i].x;
    p.y = points[i].y;
    ptarray_append_point(pa, &p, LW_TRUE);
  }
  LWPOLY *poly = lwpoly_construct_empty(srid, 0, 0);
  lwpoly_add_ring(poly, pa);
  return lwpoly_as_lwgeom(poly);
}

/**
 * @brief Return the oriented envelop (a.k.a. minimum-area rotated rectangle)
 * of a geometry
 * @details Works directly on the exact circular arcs represented by the Edge
 * structure.
 */
LWGEOM *
meos_oriented_envelope(const LWGEOM *geom)
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
   * - line: 2 points
   * - arc : 2 endpoints + 4 cardinal points
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
  uint32_t nhull = convex_hull_points(points, npoints, &hull);
  pfree(points);
  if (nhull == 0)
    return lwpoly_as_lwgeom(lwpoly_construct_empty(geom->srid, 0, 0));

  /* Degenerate cases */
  if (nhull == 1)
  {
    POINT2D rect[5];
    for (int i = 0; i < 5; i++)
      rect[i] = hull[0];
    LWGEOM *result = make_geometry_points(geom->srid, rect, nhull);
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
    LWGEOM *result = make_geometry_points(geom->srid, rect, nhull);
    pfree(hull);
    return result;
  }

  /*
   * Generate candidate orientations.
   * - For a polygonal convex hull these are simply the hull-edge
   *   orientations.
   * - For an arc, its tangent direction changes continuously. We therefore
   *   inspect the analytically significant tangent directions of the arc.
   * Because the rectangle orientation is periodic modulo pi, all angles are
   * normalized to [0,pi).
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
  return make_geometry_points(geom->srid, best_rect, 4);
}

/**
 * @ingroup meos_geo_base_spatial
 * @brief Return the oriented envelop (a.k.a. minimum-area rotated rectangle)
 * of a geometry
 * @param[in] gs Geometry
 * @note PostGIS function: @p ST_OrientedEnvelope(PG_FUNCTION_ARGS).
 */
GSERIALIZED *
geom_oriented_envelope(const GSERIALIZED *gs)
{
  /* Ensure the validity of the arguments */
  VALIDATE_NOT_NULL(gs, NULL);
  if (! ensure_not_geodetic_geo(gs))
    return NULL;

  LWGEOM *lwgeom = lwgeom_from_gserialized(gs);
  LWGEOM *res = meos_oriented_envelope(lwgeom);
  GSERIALIZED *result = geo_serialize(res);
  lwgeom_free(lwgeom); lwgeom_free(res);
  return result;
}

/******************************************************************************/

/**
 * @brief Return the convex hull of a geometry
 * @details Works directly on the exact circular arcs represented by the Edge
 * structure.
 */
LWGEOM *
convex_hull(const LWGEOM *geom)
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
   * - line: 2 points
   * - arc : 2 endpoints + 4 cardinal points
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
  uint32_t nhull = convex_hull_points(points, npoints, &hull);
  pfree(points);
  if (nhull == 0)
    return lwpoly_as_lwgeom(lwpoly_construct_empty(geom->srid, 0, 0));

  LWGEOM *result = make_geometry_points(geom->srid, points, nhull);
  pfree(hull);
  return result;
}

/**
 * @ingroup meos_geo_base_spatial
 * @brief Return the convex hull of a geometry
 * @param[in] gs Geometry
 * @note PostGIS function: @p ST_ConvexHull(PG_FUNCTION_ARGS). With respect to
 * the original function we do not use the @p prec argument.
 */
GSERIALIZED *
geom_convex_hull_meos(const GSERIALIZED *gs)
{
  /* Ensure the validity of the arguments */
  VALIDATE_NOT_NULL(gs, NULL);
  if (! ensure_not_geodetic_geo(gs))
    return NULL;

  /* Empty.ConvexHull() == Empty */
  if (gserialized_is_empty(gs))
    return geo_copy(gs);

  LWGEOM *lwgeom = lwgeom_from_gserialized(gs);
  LWGEOM *res = convex_hull(lwgeom);
  GSERIALIZED *result = geo_serialize(res);
  lwgeom_free(lwgeom); lwgeom_free(res);
  return result;
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
bool
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
int
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
      m->ii = 0;
      break;
    case 1:
      /* Point is on the boundary of the line */
      m->ib = 0;
      break;
    case 2:
      /* Point is in the exterior of the line */
      m->ie = 0;
      break;
  }

  /* A non-empty linear geometry has a 1-dimensional interior */
  m->ei = 1;

  /* The boundary of a linear geometry is 0-dimensional if non-empty.
   * We determine whether the geometry has at least one odd endpoint. */
  bool has_boundary = false;

  for (int i = 0; i < nedges && !has_boundary; i++)
  {
    const Edge *e = edges[i];
    if (e->etype != EDGE_LINE && e->etype != EDGE_ARC)
      continue;
    if (!relate_edge_nonempty(e))
      continue;
    /* Test the endpoint against the complete linear geometry.
     * If it has odd endpoint parity, it belongs to the boundary. */
    if (relate_point_on_linear_boundary(e->x1, e->y1, edges, nedges) ||
        relate_point_on_linear_boundary(e->x2, e->y2, edges, nedges))
    {
      has_boundary = true;
    }
  }

  if (has_boundary)
    m->bi = 0;

  /* The exterior of a non-empty linear geometry is 2-dimensional. */
  m->ee = 2;

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
 * @brief Determine the location of a point relative to a linear geometry.
 * @details  Convenience wrapper used by the linear/linear DE-9IM computation.
 */
static int
relate_linear_point_location(double x, double y, Edge **edges, int nedges)
{
  return relate_point_in_linear(x, y, edges, nedges);
}

/**
 * @brief Return true if an intersection between two linear edges contains
 * a one-dimensional portion.
 * @details The function preserves the exact line/arc intersection
 */
static bool
relate_linear_edges_overlap(const Edge *a, const Edge *b)
{
  if (a->etype == EDGE_LINE && b->etype == EDGE_LINE)
  {
    IntersectResult r = linesegm_intersect(a->x1, a->y1, a->dx, a->dy,
        b->x1, b->y1, b->x2, b->y2);
    return r.type == INTERSECT_OVERLAP;
  }

  /* A line and a circular arc can intersect only in points unless
   * the line is degenerate, which is excluded here. */
  return false;
}

/**
 * @brief Determine whether two linear edges intersect.
 * @details Return:
 *   0 = no intersection
 *   1 = point intersection
 *   2 = one-dimensional overlap
 */
static int
relate_linear_edges_intersection(const Edge *a, const Edge *b)
{
  /* Line / line */
  if (a->etype == EDGE_LINE && b->etype == EDGE_LINE)
  {
    IntersectResult r = linesegm_intersect(a->x1, a->y1, a->dx, a->dy,
        b->x1, b->y1, b->x2, b->y2);
    if (r.type == INTERSECT_OVERLAP)
      return 2;
    if (r.type == INTERSECT_POINT)
      return 1;
    return 0;
  }

  /* Line / arc */
  if (a->etype == EDGE_LINE && b->etype == EDGE_ARC)
  {
    double roots[2];
    int n = arcsegm_intersect(a->x1, a->y1, a->dx, a->dy, b, roots);
    return n > 0 ? 1 : 0;
  }

  /* Arc / line */
  if (a->etype == EDGE_ARC && b->etype == EDGE_LINE)
  {
    double roots[2];
    int n = arcsegm_intersect(b->x1, b->y1, b->dx, b->dy, a, roots);
    return n > 0 ? 1 : 0;
  }

  /* Arc / arc */
  if (a->etype == EDGE_ARC && b->etype == EDGE_ARC)
  {
    return arcarc_intersect(a, b) ? 1 : 0;
  }
  return 0;
}

/**
 * @brief Compute the DE-9IM matrix for two linear geometries.
 * @details The implementation distinguishes:
 *   II = interior/interior
 *   IB = interior/boundary
 *   BI = boundary/interior
 *   BB = boundary/boundary
 * while retaining exact line/arc intersection.
 */
static void
relate_linear_linear(const LWGEOM *g1, const LWGEOM *g2, MeosDE9IM *m)
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

  /* First determine whether the two interiors have a one-dimensional
  * intersection. */
  bool interior_overlap = false;

  /*
   * Point intersections need further classification:
   *   II
   *   IB
   *   BI
   *   BB
   * depending on whether the intersection point is in the interior or boundary
   * of each complete linear geometry.
   */
  bool ii_point = false;
  bool ib_point = false;
  bool bi_point = false;
  bool bb_point = false;
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
      int type = relate_linear_edges_intersection(a, b);
      if (type == 2)
      {
        /* A line/line overlap is necessarily an interior/interior
         * one-dimensional intersection */
        interior_overlap = true;
        continue;
      }
      if (type != 1)
        continue;

      /*
       * We have a point intersection. The existing intersection
       * routines do not return the point itself, so use the
       * endpoints of the participating edges as candidates.
       *
       * For line/arc and arc/arc intersections, the endpoints alone
       * are insufficient in the general case. Therefore we classify
       * the endpoints first, and leave the dimensional intersection
       * represented by II when the intersection is not a boundary
       * endpoint.
       */

      double candidates_x[4];
      double candidates_y[4];
      int ncandidates = 0;

      candidates_x[ncandidates] = a->x1;
      candidates_y[ncandidates++] = a->y1;

      candidates_x[ncandidates] = a->x2;
      candidates_y[ncandidates++] = a->y2;

      candidates_x[ncandidates] = b->x1;
      candidates_y[ncandidates++] = b->y1;

      candidates_x[ncandidates] = b->x2;
      candidates_y[ncandidates++] = b->y2;

      bool classified = false;
      for (int k = 0; k < ncandidates; k++)
      {
        double x = candidates_x[k];
        double y = candidates_y[k];
        bool on_a = (a->etype == EDGE_LINE) ?
          point_on_segment(x, y, a->x1, a->y1, a->x2, a->y2) :
          point_on_arc(x, y, a);
        bool on_b = (b->etype == EDGE_LINE) ?
          point_on_segment(x, y, b->x1, b->y1, b->x2, b->y2) :
          point_on_arc(x, y, b);
        if (! on_a || ! on_b)
          continue;
        int loc1 = relate_linear_point_location(x, y, e1, n1);
        int loc2 = relate_linear_point_location(x, y, e2, n2);
        if (loc1 == 0 && loc2 == 0)
          ii_point = true;
        else if (loc1 == 0 && loc2 == 1)
          ib_point = true;
        else if (loc1 == 1 && loc2 == 0)
          bi_point = true;
        else if (loc1 == 1 && loc2 == 1)
          bb_point = true;
        classified = true;
      }

      /*
       * If this was a genuine line/arc or arc/arc intersection occurring away
       * from endpoints, it belongs to the interiors. This preserves the exact
       * arc engine rather than polygonizing the arc.
       */
      if (!classified)
        ii_point = true;
    }
  }

  /* II */
  if (interior_overlap)
    m->ii = 1;
  else if (ii_point)
    m->ii = 0;

  /* IB, BI and BB */
  if (ib_point)
    m->ib = 0;
  if (bi_point)
    m->bi = 0;
  if (bb_point)
    m->bb = 0;

  /* Determine whether the boundaries themselves are non-empty */
  bool boundary1 = false;
  bool boundary2 = false;
  for (int i = 0; i < n1 && !boundary1; i++)
  {
    const Edge *e = e1[i];
    if (e->etype != EDGE_LINE && e->etype != EDGE_ARC)
      continue;
    if (!relate_edge_nonempty(e))
      continue;
    if (relate_point_on_linear_boundary(e->x1, e->y1, e1, n1) ||
        relate_point_on_linear_boundary(e->x2, e->y2, e1, n1))
      boundary1 = true;
  }

  for (int i = 0; i < n2 && !boundary2; i++)
  {
    const Edge *e = e2[i];
    if (e->etype != EDGE_LINE && e->etype != EDGE_ARC)
      continue;
    if (!relate_edge_nonempty(e))
      continue;
    if (relate_point_on_linear_boundary(e->x1, e->y1, e2, n2) ||
        relate_point_on_linear_boundary(e->x2, e->y2, e2, n2))
      boundary2 = true;
  }

  /*
   * The boundary/exterior intersections are non-empty whenever the
   * corresponding boundary exists and the other geometry does not contain the
   * entire boundary.
   * For ordinary finite linear geometries the exterior is two-dimensional.
   */
  if (boundary1)
    m->be = 0;
  if (boundary2)
    m->eb = 0;
  m->ee = 2;

  pfree(e1); pfree(e2); meos_array_destroy(a1); meos_array_destroy(a2);
  return;
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
      m->ii = 1;
      break;
    case 1:
      /* A non-zero open portion coincides with the area boundary. */
      m->ib = 1;
      break;
    case 2:
      /* A non-zero open portion is outside the area. */
      m->ie = 1;
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
        /* Linear interior ∩ area */
        if (aloc == 0)
          m->ii = 0;
        else if (aloc == 1)
          m->ib = 0;
        else
          m->ie = 0;
      }
      else
      {
        /* Linear boundary ∩ area */
        if (aloc == 0)
          m->bi = 0;
        else if (aloc == 1)
          m->bb = 0;
        else
          m->be = 0;
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
        m->bb = 1;
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
          m->bb = 1;
          continue;
        }
      }

      /* Point intersections */
      for (int k = 0; k < n; k++)
      {
        /* Boundary/boundary intersection is zero-dimensional
         * unless a one-dimensional overlap was detected above. */
        (void) ix;
        (void) iy;
        m->bb = 0;
      }
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
 * @brief Determine whether an areal geometry has an exterior
 * intersection with another areal geometry.
 */
static bool
relate_area_has_exterior(Edge **edges, int nedges, Edge **other_edges,
  int nother)
{
  /* If a boundary edge has an open portion outside the other geometry,
   * the corresponding area also has an exterior portion */
  for (int i = 0; i < nedges; i++)
  {
    const Edge *e = edges[i];
    if (!relate_area_boundary_edge(e))
      continue;
    double x, y;
    relate_area_edge_point(e, 0.5, &x, &y);
    if (relate_point_in_area(x, y, other_edges, nother) == 2)
      return true;
  }

  /* If a boundary midpoint happens to be on the other boundary,
   * use an interior witness */
  double x, y;
  if (relate_area_find_interior_point(edges, nedges, &x, &y))
  {
    if (relate_point_in_area(x, y, other_edges, nother) != 0)
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

  /* Boundary / Exterior.
   * A polygon boundary has dimension 1. */
  if (relate_area_has_exterior(e1, n1, e2, n2))
  {
    m->be = 1;
  }
  if (relate_area_has_exterior(e2, n2, e1, n1))
  {
    m->eb = 1;
  }

  /* Interior / Exterior.
   * A polygon interior has dimension 2.
   * If A has an interior point outside B, IE = 2.
   * If B has an interior point outside A, EI = 2. */
  double x, y;
  if (relate_area_find_interior_point(e1, n1, &x, &y))
  {
    if (relate_point_in_area(x, y, e2, n2) == 2)
      m->ie = 2;
  }
  if (relate_area_find_interior_point(e2, n2, &x, &y))
  {
    if (relate_point_in_area(x, y, e1, n1) == 2)
      m->ei = 2;
  }

  /* Exterior / Exterior.
   * Two ordinary finite areal geometries always have a
   * two-dimensional common exterior. */
  m->ee = 2;

  pfree(e1); pfree(e2);
  meos_array_destroy(a1); meos_array_destroy(a2);
  return;
}

/*****************************************************************************
 * Main dispatcher
 *****************************************************************************/

/**
 * @brief Compute the DE-9IM intersection matrix
 * @details This is the native MEOS counterpart of ST_Relate for the geometry
 * combinations implemented by this first version
 * @return true if the geometry pair is supported
 */
static bool
meos_relate(const LWGEOM *g1, const LWGEOM *g2, char result[10])
{
  assert(g1); assert(g2); assert(result);
  MeosDE9IM m;
  de9im_init(&m);

  /* Empty geometries */
  if (lwgeom_is_empty(g1) || lwgeom_is_empty(g2))
  {
    int d1 = relate_dimension(g1);
    int d2 = relate_dimension(g2);
    (void) d1;
    (void) d2;

    /* If one operand is empty, its intersection with every
     * non-exterior component is empty. The two exteriors meet
     * in dimension 2 */
    m.ee = 2;
    de9im_to_string(&m, result);
    return true;
  }

  /* Point / Point */
  if (relate_is_point(g1) && relate_is_point(g2))
  {
    /* The first implementation handles simple POINTs directly.
     * MULTIPOINT is handled by the existing edge representation
     * in a later extension. */
    if (g1->type == POINTTYPE && g2->type == POINTTYPE)
    {
      relate_point_point(g1, g2, &m);
      de9im_to_string(&m, result);
      return true;
    }
  }

  /* Point / Linear */
  if (relate_is_point(g1) && relate_is_linear(g2))
  {
    if (g1->type == POINTTYPE)
    {
      relate_point_linear(g1, g2, &m);
      de9im_to_string(&m, result);
      return true;
    }
  }

  /* Linear / Point */
  if (relate_is_linear(g1) && relate_is_point(g2))
  {
    if (g2->type == POINTTYPE)
    {
      relate_linear_point(g1, g2, &m);
      de9im_to_string(&m, result);
      return true;
    }
  }

  /* Point / Area */
  if (relate_is_point(g1) && relate_is_areal(g2))
  {
    if (g1->type == POINTTYPE)
    {
      relate_point_area(g1, g2, &m);
      de9im_to_string(&m, result);
      return true;
    }
  }

  /* Area / Point */
  if (relate_is_areal(g1) && relate_is_point(g2))
  {
    if (g2->type == POINTTYPE)
    {
      relate_area_point(g1, g2, &m);
      de9im_to_string(&m, result);
      return true;
    }
  }

  /* Linear / Linear */
  if (relate_is_linear(g1) && relate_is_linear(g2))
  {
    relate_linear_linear(g1, g2, &m);
    de9im_to_string(&m, result);
    return true;
  }

  /* Linear / Area */
  if (relate_is_linear(g1) && relate_is_areal(g2))
  {
    relate_linear_area(g1, g2, &m);
    de9im_to_string(&m, result);
    return true;
  }

  /* Area / Linear */
  if (relate_is_areal(g1) && relate_is_linear(g2))
  {
    relate_area_linear(g1, g2, &m);
    de9im_to_string(&m, result);
    return true;
  }

  /* Area / Area */
  if (relate_is_areal(g1) && relate_is_areal(g2))
  {
    relate_area_area(g1, g2, &m);
    de9im_to_string(&m, result);
    return true;
  }

  return false;
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
