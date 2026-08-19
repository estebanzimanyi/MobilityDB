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
 * @brief Native implementation of PostGIS function @p ST_Buffer() that do not
 * polygonizes arc segments
 * @details This is not yet a complete implementation
 */

/* C */
#include <math.h>
#include <stdlib.h>
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
#include "geo/geo_funcs.h"
#include "geo/postgis_funcs.h"
#include "geo/tgeo_spatialfuncs.h"

/*****************************************************************************
 * Data structures
 *****************************************************************************/

/**
 * @brief Structure keeping an edge together with its source geometry.
 * @details The source geometry identifies the polygon from which the edge
 * originates. This will be used by the subsequent noding/classification
 * stages.
 */
typedef struct
{
  Edge edge;
  uint32_t source;
} UnionEdge;

/**
 * @brief Dynamic collection of polygon boundary edges for union.
 */
typedef struct
{
  UnionEdge *edges;
  uint32_t count;
  uint32_t capacity;
} UnionEdges;

/**
 * @brief Join style for line buffering
 */
typedef enum
{
  JOIN_ROUND = 1,
  JOIN_MITRE = 2,
  JOIN_BEVEL = 3
} JoinStyle;

/**
 * @brief End-cap style for line buffering
 */
typedef enum
{
  ENDCAP_ROUND = 1,
  ENDCAP_FLAT = 2,
  ENDCAP_SQUARE = 3
} EndCapStyle;

/**
 * @brief Type of a buffer boundary piece.
 */
typedef enum
{
  BUFFER_SEGMENT,
  BUFFER_ARC
} BufferPieceType;

/**
 * @brief A piece of a buffer boundary.
 */
typedef struct
{
  BufferPieceType type;
  double x1;
  double y1;
  double x2;
  double y2;
  /* Parameters used only for circular arcs */
  double cx;
  double cy;
  double radius;
  double theta1;
  double theta2;
  bool ccw;
} BufferPiece;

/**
 * @brief Add a point to a local parameterized node array.
 */
typedef struct
{
  POINT2D point;
  double parameter;
} BufferSplitPoint;

/**
 * @brief Classification of a split buffer boundary piece with respect to
 * another buffer.
 */
typedef enum
{
  BUFFER_PIECE_EXTERIOR = 0,
  BUFFER_PIECE_INTERIOR = 1,
  BUFFER_PIECE_BOUNDARY = 2
} BufferPieceLocation;

/**
 * @brief Topological classification of a closed buffer boundary ring.
 * @details
 * - ring Boundary ring
 * - pieces The ordered pieces used to construct the ring. The ring owns the
 *   geometric representation, while this array contains copies of the
 *   BufferPiece descriptors needed by later topology stages.
 * - parent is the index of the immediately containing ring, or -1 when
 *   the ring has no containing ring.
 * - depth is the number of containing rings between the ring and the
 *   exterior. Even depth means shell, odd depth means hole.
 * - shell identifies the shell which directly owns the ring when the
 *   ring is a hole. It is -1 for shells.
 */
typedef struct
{
  LWCOMPOUND *ring;
  MeosArray *pieces;
  double x;
  double y;
  int32_t parent;
  uint32_t depth;
  int32_t shell;
} BufferRingInfo;
/* Temporary forward declaration */
extern LWGEOM *
meos_buffer(const LWGEOM *geom, double radius, JoinStyle join_style,
  EndCapStyle cap_style, double mitre_limit);
  
/*****************************************************************************
 * Polygon union / dissolve
 *****************************************************************************/

/**
 * @brief Initialize a union edge collection.
 */
static void
union_edges_init(UnionEdges *edges, uint32_t capacity)
{
  assert(edges);
  if (capacity < 16)
    capacity = 16;
  edges->edges = palloc(sizeof(UnionEdge) * capacity);
  edges->count = 0;
  edges->capacity = capacity;
  return;
}

/**
 * @brief Free a union edge collection.
 */
static void
union_edges_free(UnionEdges *edges)
{
  assert(edges);
  if (edges->edges)
    pfree(edges->edges);
  edges->edges = NULL;
  edges->count = 0;
  edges->capacity = 0;
  return;
}

/**
 * @brief Append an edge to a union edge collection.
 */
static void
union_edges_add(UnionEdges *edges, const Edge *edge, uint32_t source)
{
  assert(edges); assert(edge);
  if (edges->count == edges->capacity)
  {
    edges->capacity *= 2;
    edges->edges = repalloc(edges->edges, 
      sizeof(UnionEdge) * edges->capacity);
  }
  edges->edges[edges->count].edge = *edge;
  edges->edges[edges->count].source = source;
  edges->count++;
  return;
}

/**
 * @brief Collect the boundary edges of one polygonal geometry.
 * @details Only region-boundary edges are retained:
 *   EDGE_POLYSEG straight boundary
 *   EDGE_POLYARC circular boundary
 * This excludes EDGE_LINE and EDGE_ARC because the union operation 
 * works on surfaces, not on standalone one-dimensional geometries.
 */
static void
union_collect_geometry(const LWGEOM *geom, uint32_t source, UnionEdges *result)
{
  assert(geom); assert(result);
  MeosArray *array = geom_extract_edges(geom);
  for (uint32_t i = 0; i < array->count; i++)
  {
    const Edge *edge = (const Edge *) meos_array_get(array, i);
    if (edge->etype != EDGE_POLYSEG && edge->etype != EDGE_POLYARC)
      continue;
    union_edges_add(result, edge, source);
  }
  meos_array_destroy(array);
  return;
}

/**
 * @brief Return true if an edge bounding box overlaps another edge box.
 */
static inline bool
union_edge_bbox_overlap(const Edge *a, const Edge *b)
{
  assert(a); assert(b);
  if (a->xmax < b->xmin - FP_TOLERANCE || b->xmax < a->xmin - FP_TOLERANCE)
    return false;
  if (a->ymax < b->ymin - FP_TOLERANCE || b->ymax < a->ymin - FP_TOLERANCE)
    return false;
  return true;
}

/**
 * @brief Return the number of polygon boundary edges in a union input.
 */
static uint32_t
union_count_edges(LWGEOM **geoms, uint32_t ngeoms)
{
  assert(geoms || ngeoms == 0);
  uint32_t count = 0;
  for (uint32_t i = 0; i < ngeoms; i++)
  {
    if (! geoms[i] || lwgeom_is_empty(geoms[i]))
      continue;
    MeosArray *array = geom_extract_edges(geoms[i]);
    for (uint32_t j = 0; j < array->count; j++)
    {
      const Edge *edge = (const Edge *) meos_array_get(array, j);
      if (edge->etype == EDGE_POLYSEG || edge->etype == EDGE_POLYARC)
        count++;
    }
    meos_array_destroy(array);
  }
  return count;
}

/**
 * @brief Collect all polygon boundary edges participating in a union.
 * @param[in] geoms Input polygonal geometries
 * @param[in] ngeoms Number of input geometries
 * @param[out] result Collected boundary edges
 */
static void
union_collect_edges(LWGEOM **geoms, uint32_t ngeoms, UnionEdges *result)
{
  assert(geoms); assert(result);
  for (uint32_t i = 0; i < ngeoms; i++)
  {
    if (! geoms[i] || lwgeom_is_empty(geoms[i]))
      continue;
    union_collect_geometry(geoms[i], i, result);
  }
  return;
}

/*****************************************************************************
 * Buffer utilities
 *****************************************************************************/

/**
 * @brief Return the 2D cross product of two vectors
 */
static inline double
buffer_cross(double ax, double ay, double bx, double by)
{
  return ax * by - ay * bx;
}

/**
 * @brief Return the dot product of two vectors
 */
static inline double
buffer_dot(double ax, double ay, double bx, double by)
{
  return ax * bx + ay * by;
}

/**
 * @brief Return the length of a vector
 */
static inline double
buffer_length(double x, double y)
{
  return hypot(x, y);
}

/**
 * @brief Normalize a vector
 */
static bool
buffer_normalize(double x, double y, double *nx, double *ny)
{
  assert(nx); assert(ny);
  double length = buffer_length(x, y);
  if (length <= FP_TOLERANCE)
    return false;
  *nx = x / length;
  *ny = y / length;
  return true;
}

/**
 * @brief Compute the left normal of a directed segment
 */
static bool
buffer_left_normal(double x1, double y1, double x2, double y2,
  double *nx, double *ny)
{
  assert(nx); assert(ny);
  double dx = x2 - x1;
  double dy = y2 - y1;
  double length = hypot(dx, dy);
  if (length <= FP_TOLERANCE)
    return false;
  *nx = -dy / length;
  *ny = dx / length;
  return true;
}

/**
 * @brief Return a point displaced from another point
 */
static inline POINT2D
buffer_point_offset(double x, double y, double nx, double ny, double distance)
{
  POINT2D result;
  result.x = x + nx * distance;
  result.y = y + ny * distance;
  return result;
}

/**
 * @brief Append a 2D point to a point array
 */
static void
buffer_append_point(POINTARRAY *pa, double x, double y)
{
  POINT4D point;
  point.x = x;
  point.y = y;
  point.z = 0.0;
  point.m = 0.0;
  ptarray_append_point(pa, &point, LW_TRUE);
}

/**
 * @brief Append a 2D CIRCSTRING arc to a point array
 */
static void
buffer_append_arc(POINTARRAY *pa, double cx, double cy, double radius,
  double start_angle, double end_angle, bool ccw)
{
  assert(pa); assert(radius >= 0);
  double sweep = ccw ? 
    angle_normalize(end_angle - start_angle) :
    angle_normalize(start_angle - end_angle);
  if (sweep <= FP_TOLERANCE)
    return;
  /* A CIRCSTRING arc is represented by three points.
   * Split arcs larger than PI into several pieces in the caller. */
  double middle_angle = ccw ?
    start_angle + sweep * 0.5 : start_angle - sweep * 0.5;
  buffer_append_point(pa, cx + radius * cos(start_angle),
    cy + radius * sin(start_angle));
  buffer_append_point(pa, cx + radius * cos(middle_angle),
    cy + radius * sin(middle_angle));
  buffer_append_point(pa, cx + radius * cos(end_angle),
    cy + radius * sin(end_angle));
};

/**
 * @brief Compute the intersection of two line segments.
 * @details Returns false when the segments do not have a unique point
 * intersection. Collinear overlap is deliberately handled separately by the
 * topology layer.
 */
static bool
buffer_segment_intersection(POINT2D a1, POINT2D a2, POINT2D b1, POINT2D b2,
  POINT2D *result)
{
  assert(result);
  double r_x = a2.x - a1.x;
  double r_y = a2.y - a1.y;
  double s_x = b2.x - b1.x;
  double s_y = b2.y - b1.y;
  double denominator = buffer_cross(r_x, r_y, s_x, s_y);
  if (fabs(denominator) <= FP_TOLERANCE)
    return false;
  double q_x = b1.x - a1.x;
  double q_y = b1.y - a1.y;
  double t = buffer_cross(q_x, q_y, s_x, s_y) / denominator;
  double u = buffer_cross(q_x, q_y, r_x, r_y) / denominator;
  if (t < -FP_TOLERANCE || t > 1.0 + FP_TOLERANCE)
    return false;
  if (u < -FP_TOLERANCE || u > 1.0 + FP_TOLERANCE)
    return false;
  result->x = a1.x + t * r_x;
  result->y = a1.y + t * r_y;
  return true;
}

/**
 * @brief Compute the intersection of two infinite lines
 * @details The first line is P + tR and the second line is Q + uS.
 */
static bool
buffer_line_intersection(POINT2D p, double rx, double ry, POINT2D q,
  double sx, double sy, POINT2D *result)
{
  assert(result);
  double denominator = buffer_cross(rx, ry, sx, sy);
  if (fabs(denominator) <= FP_TOLERANCE)
    return false;
  double qpx = q.x - p.x;
  double qpy = q.y - p.y;
  double t = buffer_cross(qpx, qpy, sx, sy) / denominator;
  result->x = p.x + t * rx;
  result->y = p.y + t * ry;
  return true;
}

/**
 * @brief Construct a 2D LINESTRING containing two points
 */
static LWLINE *
buffer_make_segment(int32_t srid, POINT2D p1, POINT2D p2)
{
  POINTARRAY *points = ptarray_construct_empty(LW_FALSE, LW_FALSE, 2);
  buffer_append_point(points, p1.x, p1.y);
  buffer_append_point(points, p2.x, p2.y);
  return lwline_construct(srid, NULL, points);
}

/**
 * @brief Construct a 3-point circular arc
 */
static LWCIRCSTRING *
buffer_make_arc(int32_t srid, double cx, double cy, double radius,
  double start_angle, double end_angle, bool ccw)
{
  double sweep, middle_angle;
  POINTARRAY *points;
  if (ccw)
    sweep = angle_normalize(end_angle - start_angle);
  else
    sweep = angle_normalize(start_angle - end_angle);
  if (sweep <= FP_TOLERANCE)
    return NULL;

  /* A CIRCSTRING arc is represented by three points.
   * Split arcs larger than PI into several pieces in the caller. */
  middle_angle = ccw ? start_angle + sweep * 0.5 : start_angle - sweep * 0.5;
  points = ptarray_construct_empty(LW_FALSE, LW_FALSE, 3);
  buffer_append_point(points, cx + radius * cos(start_angle),
    cy + radius * sin(start_angle));
  buffer_append_point(points, cx + radius * cos(middle_angle),
    cy + radius * sin(middle_angle));
  buffer_append_point(points, cx + radius * cos(end_angle),
    cy + radius * sin(end_angle));
  return lwcircstring_construct(srid, NULL, points);
}

/**
 * @brief Add a straight segment to a compound curve
 */
static void
buffer_add_segment(LWCOMPOUND *curve, int32_t srid, POINT2D p1, POINT2D p2)
{
  assert(curve);
  if (hypot(p2.x - p1.x, p2.y - p1.y) <= FP_TOLERANCE)
    return;
  LWLINE *line = buffer_make_segment(srid, p1, p2);
  lwcompound_add_lwgeom(curve, lwline_as_lwgeom(line));
}

/**
 * @brief Add circular arcs to a compound curve
 */
static void
buffer_add_arc(LWCOMPOUND *curve, int32_t srid, double cx, double cy,
  double radius, double start_angle, double end_angle, bool ccw)
{
  assert(curve);
  double sweep = ccw ? 
    angle_normalize(end_angle - start_angle) :
    angle_normalize(start_angle - end_angle);
  if (sweep <= FP_TOLERANCE)
    return;

  /* PostGIS circular strings use three points per arc and an individual
   * arc must not exceed 180 degrees */
  int count = (int) ceil(sweep / M_PI);
  if (count < 1)
    count = 1;
  double delta = sweep / (double) count;
  for (int i = 0; i < count; i++)
  {
    double a0 = ccw ? start_angle + delta * i : start_angle - delta * i;
    double a1 = ccw ? start_angle + delta * (i + 1) : 
      start_angle - delta * (i + 1);
    LWCIRCSTRING *arc = buffer_make_arc(srid, cx, cy, radius, a0, a1, ccw);
    if (arc)
      lwcompound_add_lwgeom(curve, lwcircstring_as_lwgeom(arc));
  }
}

/**
 * @brief Add a round join
 */
static void
buffer_add_round_join(LWCOMPOUND *curve, int32_t srid, POINT2D vertex,
  POINT2D p1, POINT2D p2, double radius, bool ccw)
{
  assert(curve);
  double start_angle = atan2(p1.y - vertex.y, p1.x - vertex.x);
  double end_angle = atan2(p2.y - vertex.y, p2.x - vertex.x);
  buffer_add_arc(curve, srid, vertex.x, vertex.y, radius, start_angle,
    end_angle, ccw);
}

/**
 * @brief Add a bevel join
 */
static void
buffer_add_bevel_join(LWCOMPOUND *curve, int32_t srid, POINT2D p1, POINT2D p2)
{
  assert(curve);
  buffer_add_segment(curve, srid, p1, p2);
}

/**
 * @brief Add a mitre join
 */
static bool
buffer_add_mitre_join(LWCOMPOUND *curve, int32_t srid, POINT2D vertex,
  POINT2D p1, POINT2D p2, double radius, double mitre_limit)
{
  assert(curve);
  double r1x = p1.x - vertex.x;
  double r1y = p1.y - vertex.y;
  double r2x = p2.x - vertex.x;
  double r2y = p2.y - vertex.y;
  double l1 = hypot(r1x, r1y);
  double l2 = hypot(r2x, r2y);
  if (l1 <= FP_TOLERANCE || l2 <= FP_TOLERANCE)
    return false;
  r1x /= l1;
  r1y /= l1;
  r2x /= l2;
  r2y /= l2;
  POINT2D intersection;
  if (! buffer_line_intersection(p1, r1x, r1y, p2, r2x, r2y, &intersection))
    return false;
  double length = hypot(intersection.x - vertex.x, intersection.y - vertex.y);
  if (length > radius * mitre_limit + FP_TOLERANCE)
    return false;
  buffer_add_segment(curve, srid, p1, intersection);
  buffer_add_segment(curve, srid, intersection, p2);
  return true;
}

/**
 * @brief Add a join between two offset segments
 */
static void
buffer_add_join(LWCOMPOUND *curve, int32_t srid, POINT2D vertex, POINT2D p1,
  POINT2D p2, double radius, JoinStyle join_style, double mitre_limit,
  bool outer)
{
  assert(curve);
  /* The inner side of a turn is always joined by the intersection
   * of the two offset lines */
  if (! outer)
  {
    double r1x = p1.x - vertex.x;
    double r1y = p1.y - vertex.y;
    double r2x = p2.x - vertex.x;
    double r2y = p2.y - vertex.y;
    POINT2D intersection;
    if (buffer_line_intersection(p1, r1x, r1y, p2, r2x, r2y, &intersection))
    {
      buffer_add_segment(curve, srid, p1, intersection);
      buffer_add_segment(curve, srid, intersection, p2);
    }
    else
      buffer_add_segment(curve, srid, p1, p2);
    return;
  }

  switch (join_style)
  {
    case JOIN_ROUND:
    {
      double cross = buffer_cross(p1.x - vertex.x, p1.y - vertex.y,
        p2.x - vertex.x, p2.y - vertex.y);
      buffer_add_round_join(curve, srid, vertex, p1, p2, radius, cross > 0.0);
      break;
    }
    case JOIN_MITRE:
      if (! buffer_add_mitre_join(curve, srid, vertex, p1, p2, radius,
          mitre_limit))
        buffer_add_bevel_join(curve, srid, p1, p2);
      break;
    case JOIN_BEVEL:
      buffer_add_bevel_join(curve, srid, p1, p2);
      break;
  }
}

/**
 * @brief Add a round cap
 */
static void
buffer_add_round_cap(LWCOMPOUND *curve, int32_t srid, POINT2D center,
  POINT2D p1, POINT2D p2, double radius, bool ccw)
{
  assert(curve);
  double start_angle = atan2(p1.y - center.y, p1.x - center.x);
  double end_angle = atan2( p2.y - center.y, p2.x - center.x);
  buffer_add_arc(curve, srid, center.x, center.y, radius, start_angle,
    end_angle, ccw);
}

/*****************************************************************************
 * Buffer intersection
 *****************************************************************************/

/**
 * @brief Return true if two buffer geometries have overlapping interiors.
 * @details This is used to determine whether individual line buffers can
 * be returned independently or whether an overlay/union operation is needed.
 *
 * This first implementation performs a boundary intersection test.
 */
static bool
buffer_geometries_intersect(const LWGEOM *geom1, const LWGEOM *geom2)
{
  assert(geom1); assert(geom2);
  MeosArray *a1 = geom_extract_edges(geom1);
  MeosArray *a2 = geom_extract_edges(geom2);
  int n1 = (int) a1->count;
  int n2 = (int) a2->count;
  Edge **e1 = palloc(sizeof(Edge *) * n1);
  Edge **e2 = palloc(sizeof(Edge *) * n2);
  for (int i = 0; i < n1; i++)
    e1[i] = (Edge *) meos_array_get(a1, i);
  for (int i = 0; i < n2; i++)
    e2[i] = (Edge *) meos_array_get(a2, i);
  bool result = false;
  for (int i = 0; i < n1 && ! result; i++)
  {
    const Edge *a = e1[i];
    if (a->etype != EDGE_POLYSEG && a->etype != EDGE_POLYARC &&
        a->etype != EDGE_ARC && a->etype != EDGE_LINE)
      continue;
    for (int j = 0; j < n2; j++)
    {
      const Edge *b = e2[j];
      if (b->etype != EDGE_POLYSEG && b->etype != EDGE_POLYARC &&
          b->etype != EDGE_ARC && b->etype != EDGE_LINE)
        continue;

      /* Line / line */
      if (a->etype == EDGE_LINE && b->etype == EDGE_LINE)
      {
        IntersectResult r = linesegm_intersect(a->x1, a->y1, a->dx, a->dy,
          b->x1, b->y1, b->x2, b->y2);
        if (r.type != INTERSECT_NONE)
        {
          result = true;
          break;
        }
      }

      /* Line / arc */
      else if (a->etype == EDGE_LINE && b->etype == EDGE_ARC)
      {
        double roots[2];
        int n = arcsegm_intersect(a->x1, a->y1, a->dx, a->dy, b, roots);
        if (n > 0)
        {
          result = true;
          break;
        }
      }

      /* Arc / line */
      else if (a->etype == EDGE_ARC && b->etype == EDGE_LINE)
      {
        double roots[2];
        int n = arcsegm_intersect(b->x1, b->y1, b->dx, b->dy, a, roots);
        if (n > 0)
        {
          result = true;
          break;
        }
      }

      /* Arc / arc */
      else if (a->etype == EDGE_ARC && b->etype == EDGE_ARC)
      {
        if (arcarc_intersect(a, b))
        {
          result = true;
          break;
        }
      }

      /* Polygon boundary / polygon boundary.
       * The existing edge engine represents polygon straight boundaries as
       * EDGE_POLYSEG and curved boundaries as EDGE_POLYARC. */
      else if (a->etype == EDGE_POLYSEG && b->etype == EDGE_POLYSEG)
      {
        IntersectResult r = linesegm_intersect(a->x1, a->y1, a->dx, a->dy,
          b->x1, b->y1, b->x2, b->y2);
        if (r.type != INTERSECT_NONE)
        {
          result = true;
          break;
        }
      }
      else if (a->etype == EDGE_POLYSEG && b->etype == EDGE_POLYARC)
      {
        double roots[2];
        int n = arcsegm_intersect(a->x1, a->y1, a->dx, a->dy, b, roots);
        if (n > 0)
        {
          result = true;
          break;
        }
      }
      else if (a->etype == EDGE_POLYARC && b->etype == EDGE_POLYSEG)
      {
        double roots[2];
        int n = arcsegm_intersect(b->x1, b->y1, b->dx, b->dy, a, roots);
        if (n > 0)
        {
          result = true;
          break;
        }
      }
      else if (a->etype == EDGE_POLYARC && b->etype == EDGE_POLYARC)
      {
        if (arcarc_intersect(a, b))
        {
          result = true;
          break;
        }
      }
    }
  }
  /* Clean up and return */
  pfree(e1); pfree(e2); meos_array_destroy(a1); meos_array_destroy(a2);
  return result;
}

/**
 * @brief Return true if a point is inside a buffered geometry.
 */
static bool
buffer_point_in_geometry(double x, double y, const LWGEOM *geom)
{
  assert(geom);
  MeosArray *arr = geom_extract_edges(geom);
  int nedges = (int) arr->count;
  Edge **edges = palloc(sizeof(Edge *) * nedges);
  for (int i = 0; i < nedges; i++)
    edges[i] = (Edge *) meos_array_get(arr, i);
  bool result = point_in_polygon(x, y, edges, nedges);
  /* Clean up and return */
  pfree(edges); meos_array_destroy(arr);
  return result;
}

/**
 * @brief Return true if two buffered components have overlapping interiors.
 * @details Boundary intersection alone is not sufficient: two buffers
 * touching at one point may legitimately remain separate components.
 * We therefore additionally test representative boundary points against
 * the other geometry.
 */
static bool
buffer_components_overlap(const LWGEOM *geom1, const LWGEOM *geom2)
{
  assert(geom1); assert(geom2);
  MeosArray *a1 = geom_extract_edges(geom1);
  MeosArray *a2 = geom_extract_edges(geom2);
  int n1 = (int) a1->count;
  int n2 = (int) a2->count;
  Edge **e1 = palloc(sizeof(Edge *) * n1);
  Edge **e2 = palloc(sizeof(Edge *) * n2);
  for (int i = 0; i < n1; i++)
    e1[i] = (Edge *) meos_array_get(a1, i);
  for (int i = 0; i < n2; i++)
    e2[i] = (Edge *) meos_array_get(a2, i);
  bool result = false;

  /* First check proper boundary intersections */
  if (buffer_geometries_intersect(geom1, geom2))
  {
    /*  A boundary intersection can mean either:
     *    1. the interiors overlap, or
     *    2. the two buffers merely touch.
     * Test the midpoint of every straight edge and the midpoint of every
     * circular edge. */
    for (int i = 0; i < n1 && ! result; i++)
    {
      const Edge *e = e1[i];
      double x, y;
      if (e->etype == EDGE_LINE || e->etype == EDGE_POLYSEG)
      {
        x = (e->x1 + e->x2) * 0.5;
        y = (e->y1 + e->y2) * 0.5;
      }
      else if (e->etype == EDGE_ARC || e->etype == EDGE_POLYARC)
      {
        double sweep = e->ccw ? angle_normalize(e->theta1 - e->theta0) :
          angle_normalize(e->theta0 - e->theta1);
        double theta = e->ccw ? e->theta0 + sweep * 0.5 :
          e->theta0 - sweep * 0.5;
        x = e->cx + e->radius * cos(theta);
        y = e->cy + e->radius * sin(theta);
      }
      else
        continue;
      if (buffer_point_in_geometry(x, y, geom2))
      {
        result = true;
        break;
      }
    }

    /* Symmetric test.
     * This is important when geom1 is completely contained inside geom2. */
    for (int i = 0; i < n2 && ! result; i++)
    {
      const Edge *e = e2[i];
      double x, y;
      if (e->etype == EDGE_LINE || e->etype == EDGE_POLYSEG)
      {
        x = (e->x1 + e->x2) * 0.5;
        y = (e->y1 + e->y2) * 0.5;
      }
      else if (e->etype == EDGE_ARC || e->etype == EDGE_POLYARC)
      {
        double sweep = e->ccw ? angle_normalize(e->theta1 - e->theta0) :
          angle_normalize(e->theta0 - e->theta1);
        double theta = e->ccw ? e->theta0 + sweep * 0.5 :
          e->theta0 - sweep * 0.5;
        x = e->cx + e->radius * cos(theta);
        y = e->cy + e->radius * sin(theta);
      }
      else
        continue;
      if (buffer_point_in_geometry(x, y, geom1))
      {
        result = true;
        break;
      }
    }
  }
  /* Clean up and merge */
  pfree(e1); pfree(e2); meos_array_destroy(a1); meos_array_destroy(a2);
  return result;
}

/*****************************************************************************
 * Buffer boundary intersection
 *****************************************************************************/

/**
 * @brief Return true if two buffer boundary edges intersect.
 * @details Uses the line and circular-arc intersection engines.
 */
static bool
buffer_edges_intersect(const Edge *e1, const Edge *e2)
{
  if (! e1 || ! e2)
    return false;

  /* Line / Line */
  if (e1->etype == EDGE_POLYSEG && e2->etype == EDGE_POLYSEG)
  {
    IntersectResult r = linesegm_intersect(e1->x1, e1->y1, e1->dx, e1->dy,
      e2->x1, e2->y1, e2->x2, e2->y2);
    return r.type != INTERSECT_NONE;
  }

  /* Line / Arc */
  if (e1->etype == EDGE_POLYSEG && e2->etype == EDGE_POLYARC)
  {
    double roots[2];
    int n = arcsegm_intersect(e1->x1, e1->y1, e1->dx, e1->dy, e2, roots);
    return n > 0;
  }

  /* Arc / Line */
  if (e1->etype == EDGE_POLYARC && e2->etype == EDGE_POLYSEG)
  {
    double roots[2];
    int n = arcsegm_intersect(e2->x1, e2->y1, e2->dx, e2->dy, e1, roots);
    return n > 0;
  }

  /* Arc / Arc */
  if (e1->etype == EDGE_POLYARC && e2->etype == EDGE_POLYARC)
    return arcarc_intersect(e1, e2);

  return false;
}

/**
 * @brief Return true if two sets of buffer edges intersect.
 * @details The edges are extracted from the two geometries and tested pairwise
 * using the line/arc intersection functions.
 */
static bool
buffer_boundaries_intersect(const LWGEOM *geom1, const LWGEOM *geom2)
{
  assert(geom1); assert(geom2);
  MeosArray *a1 = geom_extract_edges(geom1);
  MeosArray *a2 = geom_extract_edges(geom2);
  uint32_t n1 = a1->count;
  uint32_t n2 = a2->count;
  for (uint32_t i = 0; i < n1; i++)
  {
    const Edge *e1 = (const Edge *) meos_array_get(a1, i);
    if (! e1)
      continue;
    for (uint32_t j = 0; j < n2; j++)
    {
      const Edge *e2 = (const Edge *) meos_array_get(a2, j);
      if (! e2)
        continue;
      if (buffer_edges_intersect(e1, e2))
      {
        meos_array_destroy(a1); meos_array_destroy(a2);
        return true;
      }
    }
  }
  meos_array_destroy(a1); meos_array_destroy(a2);
  return false;
}

/*****************************************************************************
 * Buffer areal union
 *****************************************************************************/

/**
 * @brief Return a representative point of an areal geometry.
 * @details The returned point is taken from the first boundary edge.
 * It is only used for containment tests and is therefore subsequently
 * verified against the complete geometry.
 */
static bool
buffer_areal_representative_point(const LWGEOM *geom, double *x, double *y)
{
  assert(geom); assert(x); assert(y);
  MeosArray *arr = geom_extract_edges(geom);
  if (arr->count == 0)
  {
    meos_array_destroy(arr);
    return false;
  }
  const Edge *edge = (const Edge *) meos_array_get(arr, 0);
  if (edge->etype == EDGE_POLYSEG || edge->etype == EDGE_LINE)
  {
    *x = (edge->x1 + edge->x2) * 0.5;
    *y = (edge->y1 + edge->y2) * 0.5;
  }
  else if (edge->etype == EDGE_POLYARC || edge->etype == EDGE_ARC)
  {
    double sweep = edge->ccw ? angle_normalize(edge->theta1 - edge->theta0) :
      angle_normalize(edge->theta0 - edge->theta1);
    double theta = edge->ccw ? edge->theta0 + sweep * 0.5 :
      edge->theta0 - sweep * 0.5;
    *x = edge->cx + edge->radius * cos(theta);
    *y = edge->cy + edge->radius * sin(theta);
  }
  else
  {
    meos_array_destroy(arr);
    return false;
  }
  meos_array_destroy(arr);
  return true;
}

/**
 * @brief Return true if an areal geometry contains a point in its interior.
 * @details This function deliberately treats boundary points as not being
 * interior. It is therefore suitable for determining strict containment.
 */
static bool
buffer_areal_contains_point(const LWGEOM *geom, double x, double y)
{
  assert(geom);
  MeosArray *arr = geom_extract_edges(geom);
  int nedges = (int) arr->count;
  if (nedges == 0)
  {
    meos_array_destroy(arr);
    return false;
  }
  Edge **edges = palloc(sizeof(Edge *) * nedges);
  for (int i = 0; i < nedges; i++)
    edges[i] = (Edge *) meos_array_get(arr, i);
  if (relate_point_on_boundary(x, y, edges, nedges))
  {
    pfree(edges);
    meos_array_destroy(arr);
    return false;
  }
  bool result = point_in_polygon(x, y, edges, nedges);
  pfree(edges); meos_array_destroy(arr);
  return result;
}

/**
 * @brief Return true if an areal geometry completely contains another
 * areal geometry.
 * @details This is an exact containment test for the supported geometry
 * representation. The boundary intersection test is performed first so
 * that touching geometries are not classified as containment.
 */
static bool
buffer_areal_contains(const LWGEOM *outer, const LWGEOM *inner)
{
  assert(outer); assert(inner);
  double x, y;
  if (! buffer_areal_representative_point(inner, &x, &y))
    return false;
  /* If the representative point is not in the interior, the inner
   * geometry cannot be strictly contained */
  if (! buffer_areal_contains_point(outer, x, y))
    return false;
  /* Verify that no boundary of the inner geometry intersects the
   * boundary of the outer geometry */
  if (buffer_geometries_intersect(outer, inner))
    return false;
  return true;
}

/**
 * @brief Construct a MULTISURFACE from two disjoint areal geometries.
 */
static LWGEOM *
buffer_areal_collection(const LWGEOM *geom1, const LWGEOM *geom2)
{
  assert(geom1); assert(geom2);
  int32_t srid = lwgeom_get_srid(geom1);
  LWGEOM **geoms = palloc(sizeof(LWGEOM *) * 2);
  geoms[0] = lwgeom_clone(geom1);
  geoms[1] = lwgeom_clone(geom2);
  LWCOLLECTION *result = lwcollection_construct(MULTISURFACETYPE, srid,
    NULL, 2, geoms);
  return lwcollection_as_lwgeom(result);
}

/**
 * @brief Union two crossing buffer geometries while preserving circular arcs.
 * @details The implementation is defined later in this file.
 */
static LWGEOM *
buffer_union_crossing(const LWGEOM *geom1, const LWGEOM *geom2);

/**
 * @brief Return the union of two areal geometries for the
 * disjoint/containment cases.
 * @details The following cases are handled exactly:
 *   A disjoint B: MULTISURFACE(A, B)
 *   A contains B: A
 *   B contains A: B
 * Boundary-touching and boundary-crossing cases are deliberately not
 * handled by this slice because they require boundary noding and face
 * extraction.
 */
static LWGEOM *
buffer_areal_union_simple(const LWGEOM *geom1, const LWGEOM *geom2)
{
  assert(geom1); assert(geom2);

  /* A contains B */
  if (buffer_areal_contains(geom1, geom2))
    return lwgeom_clone(geom1);
  /* B contains A */
  if (buffer_areal_contains(geom2, geom1))
    return lwgeom_clone(geom2);

  /* If the boundaries do not intersect, the geometries are disjoint */
  if (! buffer_geometries_intersect(geom1, geom2))
    return buffer_areal_collection(geom1, geom2);

  /* The boundaries intersect. Try the curved-boundary union.
   * At this stage #buffer_union_crossing() handles the case where
   * the boundaries cross at discrete intersection points and the
   * resulting exterior boundary consists of one connected component.
   * More difficult cases, such as coincident boundaries, touching
   * boundaries, and multiple resulting rings, return @p NULL
   * and will be handled by subsequent overlay slices. */
  LWGEOM *result = buffer_union_crossing(geom1, geom2);
  if (result)
    return result;
  return NULL;
}

/*****************************************************************************
 * Buffer overlay - boundary intersection detection
 *****************************************************************************/

/**
 * @brief Return true if a geometry is a circular buffer ring.
 * @details Line buffers produce CURVEPOLYGONs whose exterior ring is a
 * CIRCULARSTRING.
 */
static bool
buffer_is_circular_ring(const LWGEOM *geom)
{
  assert(geom);
  if (! geom || geom->type != CURVEPOLYTYPE)
    return false;
  const LWCURVEPOLY *curvepoly = (const LWCURVEPOLY *) geom;
  if (curvepoly->nrings != 1)
    return false;
  if (! curvepoly->rings[0])
    return false;
  return curvepoly->rings[0]->type == CIRCSTRINGTYPE;
}

/**
 * @brief Return true if an edge belongs to a buffer boundary.
 * @details Buffer boundaries can contain both straight segments and exact
 * circular arcs. The existing buffer implementation represents round joins
 * and caps using CircularStrings, which are exposed by geom_extract_edges()
 * as EDGE_POLYSEG and EDGE_POLYARC edges.
 */
static bool
buffer_is_boundary_edge(const Edge *edge)
{
  assert(edge);
  return edge->etype == EDGE_POLYSEG || edge->etype == EDGE_POLYARC;
}

/**
 * @brief Return the dimension of an intersection between two buffer edges.
 * @return Return 0 if the intersection is a point, 1 if the intersection is a
 * curve, -1 if there is no intersection.
 */
static int
buffer_boundary_intersection(const Edge *e1, const Edge *e2)
{
  assert(e1); assert(e2);

  /* Straight segment / straight segment */
  if (e1->etype == EDGE_POLYSEG && e2->etype == EDGE_POLYSEG)
  {
    IntersectResult result = linesegm_intersect(e1->x1, e1->y1, e1->dx, e1->dy,
      e2->x1, e2->y1, e2->x2, e2->y2);
    if (result.type == INTERSECT_OVERLAP)
      return 1;
    if (result.type == INTERSECT_POINT)
      return 0;
    return -1;
  }

  /* Straight segment / circular arc */
  if (e1->etype == EDGE_POLYSEG && e2->etype == EDGE_POLYARC)
  {
    double roots[2];
    int n = arcsegm_intersect(e1->x1, e1->y1, e1->dx, e1->dy, e2, roots);
    return n > 0 ? 0 : -1;
  }

  /* Circular arc / straight segment */
  if (e1->etype == EDGE_POLYARC && e2->etype == EDGE_POLYSEG)
  {
    double roots[2];
    int n = arcsegm_intersect(e2->x1, e2->y1, e2->dx, e2->dy, e1, roots);
    return n > 0 ? 0 : -1;
  }

  /* Circular arc / circular arc */
  if (e1->etype == EDGE_POLYARC && e2->etype == EDGE_POLYARC)
  {
    return arcarc_intersect(e1, e2) ? 0 : -1;
  }

  return -1;
}

/*****************************************************************************
 * Buffer overlay - buffer piece geometry
 *****************************************************************************/

/**
 * @brief Return the parametric position of a point on a straight piece.
 * @details The returned parameter is approximately in [0,1].
 */
static double
buffer_segment_parameter(const BufferPiece *piece, double x, double y)
{
  assert(piece); assert(piece->type == BUFFER_SEGMENT);
  double dx = piece->x2 - piece->x1;
  double dy = piece->y2 - piece->y1;
  if (fabs(dx) >= fabs(dy))
  {
    if (fabs(dx) <= FP_TOLERANCE)
      return 0.0;
    return (x - piece->x1) / dx;
  }
  if (fabs(dy) <= FP_TOLERANCE)
    return 0.0;
  return (y - piece->y1) / dy;
}

/**
 * @brief Return true if a point lies on a straight buffer piece.
 */
static bool
buffer_point_on_segment(const BufferPiece *piece, double x, double y)
{
  assert(piece); assert(piece->type == BUFFER_SEGMENT);
  double t = buffer_segment_parameter(piece, x, y);
  if (t < -FP_TOLERANCE || t > 1.0 + FP_TOLERANCE)
    return false;
  double px = piece->x1 + t * (piece->x2 - piece->x1);
  double py = piece->y1 + t * (piece->y2 - piece->y1);
  return hypot(px - x, py - y) <= FP_TOLERANCE;
}

/*****************************************************************************
 * Buffer overlay - containment detection
 *****************************************************************************/

/**
 * @brief Return a representative point on a buffer boundary.
 * @details The returned point is guaranteed to lie on the first usable
 * boundary edge. For a circular arc, the midpoint of the arc is used.
 * For a straight segment, its midpoint is used.
 * @note The point is intentionally obtained from the edge
 * representation.
 */
static bool
buffer_boundary_representative_point(const LWGEOM *geom, double *x, double *y)
{
  assert(geom); assert(x); assert(y);
  MeosArray *edges = geom_extract_edges(geom);
  for (uint32_t i = 0; i < edges->count; i++)
  {
    const Edge *edge = (const Edge *) meos_array_get(edges, i);
    if (! buffer_is_boundary_edge(edge))
      continue;

    if (edge->etype == EDGE_POLYSEG)
    {
      *x = (edge->x1 + edge->x2) * 0.5;
      *y = (edge->y1 + edge->y2) * 0.5;
      meos_array_destroy(edges);
      return true;
    }

    if (edge->etype == EDGE_POLYARC)
    {
      double sweep;
      if (edge->ccw)
        sweep = angle_normalize(edge->theta1 - edge->theta0);
      else
        sweep = angle_normalize(edge->theta0 - edge->theta1);
      if (sweep <= FP_TOLERANCE)
        continue;
      double theta;
      if (edge->ccw)
        theta = edge->theta0 + sweep * 0.5;
      else
        theta = edge->theta0 - sweep * 0.5;
      *x = edge->cx + edge->radius * cos(theta);
      *y = edge->cy + edge->radius * sin(theta);
      meos_array_destroy(edges);
      return true;
    }
  }

  meos_array_destroy(edges);
  return false;
}

/**
 * @brief Return a small displacement suitable for probing around a boundary.
 */
static double
buffer_containment_epsilon(const LWGEOM *geom)
{
  assert(geom);
  const GBOX *box = lwgeom_get_bbox(geom);
  if (box)
  {
    double dx = box->xmax - box->xmin;
    double dy = box->ymax - box->ymin;
    double scale = fmax(dx, dy);
    if (scale > FP_TOLERANCE)
      return fmax(scale * MEOS_EPSILON, FP_TOLERANCE * 10.0);
  }
  return FP_TOLERANCE * 10.0;
}

/**
 * @brief Return the normal direction of a buffer boundary edge.
 * @details The function returns both possible normals because the
 * orientation of an arbitrary buffer boundary cannot be assumed here.
 */
static bool
buffer_edge_normals(const Edge *edge, double *nx, double *ny)
{
  assert(edge); assert(nx); assert(ny);

  if (edge->etype == EDGE_POLYSEG)
  {
    double dx = edge->x2 - edge->x1;
    double dy = edge->y2 - edge->y1;
    double length = hypot(dx, dy);
    if (length <= FP_TOLERANCE)
      return false;
    *nx = -dy / length;
    *ny = dx / length;
    return true;
  }

  if (edge->etype == EDGE_POLYARC)
  {
    /* Use the radial direction at the middle of the arc */
    double sweep;
    if (edge->ccw)
      sweep = angle_normalize(edge->theta1 - edge->theta0);
    else
      sweep = angle_normalize(edge->theta0 - edge->theta1);
    if (sweep <= FP_TOLERANCE)
      return false;
    double theta;
    if (edge->ccw)
      theta = edge->theta0 + sweep * 0.5;
    else
      theta = edge->theta0 - sweep * 0.5;
    double rx = cos(theta);
    double ry = sin(theta);
    /* Radial direction is normal to the circular arc */
    *nx = rx;
    *ny = ry;
    return true;
  }
  return false;
}

/**
 * @brief Return the location of a point with respect to a buffer.
 * @return
 *   0 = interior
 *   1 = boundary
 *   2 = exterior
 */
static int
buffer_point_location(const LWGEOM *geom, double x, double y)
{
  assert(geom);
  MeosArray *arr = geom_extract_edges(geom);
  int nedges = (int) arr->count;
  if (nedges == 0)
  {
    meos_array_destroy(arr);
    return 2;
  }
  Edge **edges = palloc(sizeof(Edge *) * nedges);
  for (int i = 0; i < nedges; i++)
    edges[i] = (Edge *) meos_array_get(arr, i);
  int result = relate_point_in_area(x, y, edges, nedges);
  /* Clean up and return */
  pfree(edges); meos_array_destroy(arr);
  return result;
}

/**
 * @brief Determine whether one buffer is completely contained in another.
 * @details This function assumes that the two buffer boundaries do not
 * intersect.
 * Since the boundary of a connected buffer is a closed curve, if its
 * boundary does not intersect the boundary of the other buffer, testing
 * one point immediately inside the boundary is sufficient to determine
 * whether the complete buffer lies inside the other buffer.
 */
static bool
buffer_is_contained(const LWGEOM *inner, const LWGEOM *outer)
{
  assert(inner); assert(outer);
  /* First obtain a point on the boundary of the candidate inner buffer */
  double x, y;
  if (! buffer_boundary_representative_point(inner, &x, &y))
    return false;
  /* Locate the corresponding edge again so that we can determine
   * its local normal */
  MeosArray *edges = geom_extract_edges(inner);
  const Edge *representative = NULL;
  for (uint32_t i = 0; i < edges->count; i++)
  {
    const Edge *edge = (const Edge *) meos_array_get(edges, i);
    if (! buffer_is_boundary_edge(edge))
      continue;
    representative = edge;
    break;
  }
  if (! representative)
  {
    meos_array_destroy(edges);
    return false;
  }
  double nx, ny;
  if (! buffer_edge_normals(representative, &nx, &ny))
  {
    meos_array_destroy(edges);
    return false;
  }
  meos_array_destroy(edges);

  /* Move slightly to either side of the boundary. We test both sides because
   * we do not want to depend on the orientation of the curve ring. */
  double epsilon = buffer_containment_epsilon(inner);
  double x1 = x + nx * epsilon;
  double y1 = y + ny * epsilon;
  double x2 = x - nx * epsilon;
  double y2 = y - ny * epsilon;
  int loc1 = buffer_point_location(outer, x1, y1);
  int loc2 = buffer_point_location(outer, x2, y2);

  /* If either side of the boundary is inside the outer buffer, the candidate
   * buffer is contained in it, provided the boundaries are known not to
   * intersect */
  if (loc1 == 0 || loc2 == 0)
    return true;
  return false;
}

/**
 * @brief Classify the relationship between two buffer surfaces.
 * @return
 *   0 = disjoint
 *   1 = boundary intersection
 *   2 = first buffer contained in second
 *   3 = second buffer contained in first
 *   4 = coincident/degenerate relationship
 */
static int
buffer_components_relation(const LWGEOM *geom1, const LWGEOM *geom2)
{
  assert(geom1); assert(geom2);
  /* Boundary intersection has priority */
  if (buffer_boundaries_intersect(geom1, geom2))
    return 1;
  /* Boundaries are disjoint. Therefore containment can be tested */
  if (buffer_is_contained(geom1, geom2))
    return 2;
  if (buffer_is_contained(geom2, geom1))
    return 3;
  return 0;
}

/*****************************************************************************
 * Buffer overlay - containment detection
 *****************************************************************************/

/**
 * @brief Add a linear edge to an edge array.
 */
static void
buffer_add_linear_edge(Edge *edges, uint32_t *count, double x1, double y1,
  double x2, double y2)
{
  assert(edges); assert(count);
  Edge *edge = &edges[*count];
  edge->x1 = x1;
  edge->y1 = y1;
  edge->x2 = x2;
  edge->y2 = y2;
  edge->xmin = fmin(x1, x2);
  edge->ymin = fmin(y1, y2);
  edge->xmax = fmax(x1, x2);
  edge->ymax = fmax(y1, y2);
  edge->dx = x2 - x1;
  edge->dy = y2 - y1;
  edge->length = hypot(edge->dx, edge->dy);
  edge->cx = 0.0;
  edge->cy = 0.0;
  edge->radius = 0.0;
  edge->theta0 = 0.0;
  edge->theta1 = 0.0;
  edge->ccw = false;
  edge->etype = EDGE_LINE;
  (*count)++;
}

/**
 * @brief Add a circular arc edge to an edge array.
 */
static void
buffer_add_arc_edge(Edge *edges, uint32_t *count, double x1, double y1,
  double x2, double y2, double cx, double cy, double radius, double theta0,
  double theta1, bool ccw)
{
  assert(edges); assert(count);
  Edge *edge = &edges[*count];
  edge->x1 = x1;
  edge->y1 = y1;
  edge->x2 = x2;
  edge->y2 = y2;
  edge->cx = cx;
  edge->cy = cy;
  edge->radius = radius;
  edge->theta0 = theta0;
  edge->theta1 = theta1;
  edge->ccw = ccw;
  edge->dx = x2 - x1;
  edge->dy = y2 - y1;
  edge->length = radius * fabs(ccw ? angle_normalize(theta1 - theta0) :
    angle_normalize(theta0 - theta1));
  /* Initialize the bounding box with the endpoints */
  edge->xmin = fmin(x1, x2);
  edge->ymin = fmin(y1, y2);
  edge->xmax = fmax(x1, x2);
  edge->ymax = fmax(y1, y2);

  /* An arc can reach one or more of the four cardinal points of its circle.
   * Add those points to the bounding box whenever they lie on the arc. */
  const double cardinal[4] = { 0.0, M_PI_2, M_PI, 3.0 * M_PI_2 };
  double sweep = ccw ? angle_normalize(theta1 - theta0) :
    angle_normalize(theta0 - theta1);
  for (int i = 0; i < 4; i++)
  {
    double angle = cardinal[i];
    double offset = ccw ? angle_normalize(angle - theta0) :
      angle_normalize(theta0 - angle);
    if (offset <= sweep + FP_TOLERANCE)
    {
      double x = cx + radius * cos(angle);
      double y = cy + radius * sin(angle);
      edge->xmin = fmin(edge->xmin, x);
      edge->ymin = fmin(edge->ymin, y);
      edge->xmax = fmax(edge->xmax, x);
      edge->ymax = fmax(edge->ymax, y);
    }
  }
  edge->etype = EDGE_ARC;
  (*count)++;
}

/**
 * @brief Extract the edges of a buffer boundary.
 * @details LINESTRING boundaries are decomposed into linear edges.
 * CIRCULARSTRING boundaries are decomposed into circular arc edges.
 * A CIRCULARSTRING is encoded using overlapping triples:
 *   P0 P1 P2 P3 P4 ...
 * giving arcs:
 *   P0 P1 P2
 *   P2 P3 P4
 *   ...
 */
static Edge *
buffer_extract_edges(const LWGEOM *geom, uint32_t *nedges)
{
  assert(geom); assert(nedges);
  *nedges = 0;

  /* LINESTRING */
  if (geom->type == LINETYPE)
  {
    const LWLINE *line = (const LWLINE *) geom;
    uint32_t npoints = line->points->npoints;
    if (npoints < 2)
      return NULL;
    Edge *edges = palloc(sizeof(Edge) * (npoints - 1));
    uint32_t count = 0;
    for (uint32_t i = 0; i < npoints - 1; i++)
    {
      POINT4D p1;
      POINT4D p2;
      getPoint4d_p(line->points, i, &p1);
      getPoint4d_p(line->points, i + 1, &p2);
      buffer_add_linear_edge(edges, &count, p1.x, p1.y, p2.x, p2.y);
    }
    *nedges = count;
    return edges;
  }

  /* CIRCULARSTRING */
  if (geom->type == CIRCSTRINGTYPE)
  {
    const LWCIRCSTRING *circ = (const LWCIRCSTRING *) geom;
    uint32_t npoints = circ->points->npoints;
    /* A circular string must contain an odd number of points and at least
     * three points */
    if (npoints < 3 || ((npoints - 1) % 2) != 0)
      return NULL;
    uint32_t narcs = (npoints - 1) / 2;
    Edge *edges = palloc(sizeof(Edge) * narcs);
    uint32_t count = 0;
    for (uint32_t i = 0; i < narcs; i++)
    {
      uint32_t first = 2 * i;
      POINT4D p0;
      POINT4D p1;
      POINT4D p2;
      getPoint4d_p(circ->points, first, &p0);
      getPoint4d_p(circ->points, first + 1, &p1);
      getPoint4d_p(circ->points, first + 2, &p2);

      /* Compute the circumcenter of the three points */
      double ax = p0.x;
      double ay = p0.y;
      double bx = p1.x;
      double by = p1.y;
      double cx = p2.x;
      double cy = p2.y;
      double d = 2.0 * (ax * (by - cy) + bx * (cy - ay) + cx * (ay - by));

      /* Collinear points do not define a circular arc.
       * Represent the degenerate case as a linear edge. */
      if (fabs(d) <= FP_TOLERANCE)
      {
        buffer_add_linear_edge(edges, &count, p0.x, p0.y, p2.x, p2.y);
        continue;
      }

      double a2 = ax * ax + ay * ay;
      double b2 = bx * bx + by * by;
      double c2 = cx * cx + cy * cy;
      double center_x = (a2 * (by - cy) + b2 * (cy - ay) + c2 * (ay - by)) / d;
      double center_y = (a2 * (cx - bx) +b2 * (ax - cx) + c2 * (bx - ax)) / d;
      double radius = hypot(p0.x - center_x, p0.y - center_y);
      if (radius <= FP_TOLERANCE)
      {
        buffer_add_linear_edge(edges, &count, p0.x, p0.y, p2.x, p2.y);
        continue;
      }
      double theta0 = atan2(p0.y - center_y, p0.x - center_x);
      double theta1 = atan2( p2.y - center_y, p2.x - center_x);
      double thetam = atan2(p1.y - center_y, p1.x - center_x);

      /* Determine the orientation of the arc from the three defining points */
      double cross = buffer_cross(p1.x - p0.x, p1.y - p0.y, p2.x - p1.x,
        p2.y - p1.y);
      bool ccw = cross > 0.0;
      /* Verify that the midpoint lies on the selected arc */
      double sweep = ccw ? angle_normalize(theta1 - theta0) :
        angle_normalize(theta0 - theta1);
      double offset = ccw ? angle_normalize(thetam - theta0) :
        angle_normalize(theta0 - thetam);

      /* If the midpoint is not on the selected arc, use the opposite
       * orientation */
      if (offset > sweep + FP_TOLERANCE)
        ccw = ! ccw;
      buffer_add_arc_edge(edges, &count, p0.x, p0.y, p2.x, p2.y, center_x,
        center_y, radius, theta0, theta1, ccw);
    }
    *nedges = count;
    return edges;
  }
  return NULL;
}

/*****************************************************************************
 * Buffer overlay - intersection point collection
 *****************************************************************************/

/**
 * @brief Add an intersection point to an array.
 * @details Duplicate points are ignored. This is important because
 * adjacent buffer segments may report the same topological node.
 */
static void
buffer_intersections_add(MeosArray *array, double x, double y)
{
  assert(array);
  /* Avoid inserting the same node more than once */
  for (uint32_t i = 0; i < array->count; i++)
  {
    POINT2D *point = (POINT2D *) meos_array_get(array, i);
    if (fabs(point->x - x) <= FP_TOLERANCE && 
        fabs(point->y - y) <= FP_TOLERANCE)
      return;
  }
  POINT2D new;
  new.x = x;
  new.y = y;
  meos_array_add(array, &new);
}

/**
 * @brief Add an intersection point to an array if it is not already present.
 */
static void
buffer_add_intersection_point(MeosArray *points, double x, double y)
{
  assert(points);
  for (uint32_t i = 0; i < points->count; i++)
  {
    const POINT2D *p = (const POINT2D *) meos_array_get(points, i);
    if (fabs(p->x - x) <= FP_TOLERANCE && fabs(p->y - y) <= FP_TOLERANCE)
      return;
  }
  POINT2D point;
  point.x = x;
  point.y = y;
  meos_array_add(points, &point);
}

/**
 * @brief Collect intersections between two straight buffer edges.
 */
static void
buffer_collect_line_line_intersections(const Edge *e1, const Edge *e2,
  MeosArray *points)
{
  assert(e1); assert(e2); assert(points);
  IntersectResult result = linesegm_intersect(e1->x1, e1->y1, e1->dx, e1->dy,
    e2->x1, e2->y1, e2->x2, e2->y2);
  if (result.type == INTERSECT_POINT)
  {
    double x = e1->x1 + result.t0 * e1->dx;
    double y = e1->y1 + result.t0 * e1->dy;
    buffer_add_intersection_point(points, x, y);
  }
  else if (result.type == INTERSECT_OVERLAP)
  {
    /* For an overlapping segment there are two nodes */
    double x0 = e1->x1 + result.t0 * e1->dx;
    double y0 = e1->y1 + result.t0 * e1->dy;
    double x1 = e1->x1 + result.t1 * e1->dx;
    double y1 = e1->y1 + result.t1 * e1->dy;
    buffer_add_intersection_point(points, x0, y0);
    buffer_add_intersection_point(points, x1, y1);
  }
}

/**
 * @brief Collect intersections between a straight edge and an arc.
 * @details arcsegm_intersect() returns parameters along the straight segment.
 * Therefore the exact intersection coordinates can be reconstructed
 * directly without approximating the circular arc.
 */
static void
buffer_collect_line_arc_intersections(const Edge *line, const Edge *arc,
  MeosArray *points)
{
  assert(line); assert(arc); assert(points);
  double roots[2];
  int n = arcsegm_intersect(line->x1, line->y1, line->dx, line->dy, arc,
    roots);
  for (int i = 0; i < n; i++)
  {
    double x = line->x1 + roots[i] * line->dx;
    double y = line->y1 + roots[i] * line->dy;
    buffer_add_intersection_point(points, x, y);
  }
}

/**
 * @brief Collect intersections between two circular arcs.
 * @details This is the same geometric construction already used by
 * #arcarc_intersect(), but instead of returning only a Boolean,
 * this function records the actual intersection coordinates.
 */
static void
buffer_collect_arc_arc_intersections(const Edge *e1, const Edge *e2,
  MeosArray *points)
{
  assert(e1); assert(e2); assert(points);
  double dx = e2->cx - e1->cx;
  double dy = e2->cy - e1->cy;
  double d = hypot(dx, dy);
  double r1 = e1->radius;
  double r2 = e2->radius;

  /* Concentric circles.
   * Coincident arcs are not split here. Their common endpoints are
   * already existing boundary nodes and will be handled separately. */
  if (d < FP_TOLERANCE)
  {
    if (fabs(r1 - r2) > FP_TOLERANCE)
      return;

    /* Same supporting circle. Add common endpoints. */
    const double angles1[2] = {e1->theta0, e1->theta1};
    const double angles2[2] = {e2->theta0, e2->theta1};
    for (int i = 0; i < 2; i++)
    {
      double x = e1->cx + r1 * cos(angles1[i]);
      double y = e1->cy + r1 * sin(angles1[i]);
      if (point_on_arc(x, y, e2))
        buffer_add_intersection_point(points, x, y);
    }
    for (int i = 0; i < 2; i++)
    {
      double x = e2->cx + r2 * cos(angles2[i]);
      double y = e2->cy + r2 * sin(angles2[i]);
      if (point_on_arc(x, y, e1))
        buffer_add_intersection_point(points, x, y);
    }
    return;
  }

  /* Circles that cannot intersect */
  if (d > r1 + r2 + FP_TOLERANCE || d < fabs(r1 - r2) - FP_TOLERANCE)
    return;

  /* Distance from the first centre to the radical-line foot */
  double a = (d * d + r1 * r1 - r2 * r2) / (2.0 * d);
  double h2 = r1 * r1 - a * a;
  if (h2 < 0.0)
    h2 = 0.0;
  double h = sqrt(h2);
  /* Unit vector from centre 1 to centre 2 */
  double ux = dx / d;
  double uy = dy / d;

  /* Foot of the perpendicular on the radical line */
  double mx = e1->cx + a * ux;
  double my = e1->cy + a * uy;

  /* At most two circle intersection points */
  for (int k = 0; k < 2; k++)
  {
    double sign = k == 0 ? -1.0 : 1.0;
    double x = mx + sign * h * (-uy);
    double y = my + sign * h * ux;
    double phi1 = atan2(y - e1->cy, x - e1->cx);
    double phi2 = atan2(y - e2->cy, x - e2->cx);
    if (arc_contains_angle(e1, phi1) && arc_contains_angle(e2, phi2))
      buffer_add_intersection_point(points, x, y);
    /* Tangency gives only one point */
    if (h <= FP_TOLERANCE)
      break;
  }
}

/**
 * @brief Collect all intersection points between two buffer boundaries.
 * @details The result contains exact Cartesian intersection coordinates.
 */
static MeosArray *
buffer_collect_intersections(const LWGEOM *geom1, const LWGEOM *geom2)
{
  assert(geom1); assert(geom2);
  MeosArray *result = meos_array_create(sizeof(POINT2D));
  MeosArray *a1 = geom_extract_edges(geom1);
  MeosArray *a2 = geom_extract_edges(geom2);
  for (uint32_t i = 0; i < a1->count; i++)
  {
    const Edge *e1 = (const Edge *) meos_array_get(a1, i);
    if (! buffer_is_boundary_edge(e1))
      continue;
    for (uint32_t j = 0; j < a2->count; j++)
    {
      const Edge *e2 = (const Edge *) meos_array_get(a2, j);
      if (! buffer_is_boundary_edge(e2))
        continue;
      if (e1->etype == EDGE_POLYSEG && e2->etype == EDGE_POLYSEG)
        buffer_collect_line_line_intersections(e1, e2, result);
      else if (e1->etype == EDGE_POLYSEG && e2->etype == EDGE_POLYARC)
        buffer_collect_line_arc_intersections(e1, e2, result);
      else if (e1->etype == EDGE_POLYARC && e2->etype == EDGE_POLYSEG)
        buffer_collect_line_arc_intersections(e2, e1, result);
      else if (e1->etype == EDGE_POLYARC && e2->etype == EDGE_POLYARC)
        buffer_collect_arc_arc_intersections(e1, e2, result);
    }
  }
  meos_array_destroy(a1); meos_array_destroy(a2);
  return result;
}

/**
 * @brief Return the number of distinct intersection points between two
 * buffer boundaries.
 */
static uint32_t
buffer_intersection_count(const LWGEOM *geom1, const LWGEOM *geom2)
{
  assert(geom1); assert(geom2);
  MeosArray *points = buffer_collect_intersections(geom1, geom2);
  uint32_t result = points->count;
  meos_array_destroy(points);
  return result;
}

/**
 * @brief Extract the points of a buffer exterior ring.
 */
static bool
buffer_ring_points(const LWGEOM *geom, POINT2D **points, uint32_t *count)
{
  assert(geom); assert(points); assert(count);
  *points = NULL;
  *count = 0;
  if (! geom || geom->type != CURVEPOLYTYPE)
    return false;
  const LWCURVEPOLY *curvepoly = (const LWCURVEPOLY *) geom;
  if (curvepoly->nrings != 1 || ! curvepoly->rings[0])
    return false;
  if (curvepoly->rings[0]->type != CIRCSTRINGTYPE)
    return false;
  const LWCIRCSTRING *ring = (const LWCIRCSTRING *) curvepoly->rings[0];
  if (! ring->points || ring->points->npoints < 2)
    return false;
  uint32_t npoints = ring->points->npoints;
  POINT2D *result = palloc(sizeof(POINT2D) * npoints);
  for (uint32_t i = 0; i < npoints; i++)
  {
    POINT4D p;
    getPoint4d_p(ring->points, i, &p);
    result[i].x = p.x;
    result[i].y = p.y;
  }
  *points = result;
  *count = npoints;
  return true;
}

/**
 * @brief Compute the circle passing through three points.
 * @return true if the three points define a non-degenerate circle.
 */
static bool
buffer_circle_from_points(POINT2D p1, POINT2D p2, POINT2D p3, double *cx,
  double *cy, double *radius)
{
  assert(cx); assert(cy); assert(radius);
  double ax = p1.x;
  double ay = p1.y;
  double bx = p2.x;
  double by = p2.y;
  double cx0 = p3.x;
  double cy0 = p3.y;
  double denominator = 2.0 * (ax * (by - cy0) + bx * (cy0 - ay) + 
    cx0 * (ay - by));
  if (fabs(denominator) <= FP_TOLERANCE)
    return false;
  double a2 = ax * ax + ay * ay;
  double b2 = bx * bx + by * by;
  double c2 = cx0 * cx0 + cy0 * cy0;
  *cx = (a2 * (by - cy0) + b2 * (cy0 - ay) + c2 * (ay - by)) / denominator;
  *cy = (a2 * (cx0 - bx) + b2 * (ax - cx0) + c2 * (bx - ax)) / denominator;
  *radius = hypot(ax - *cx, ay - *cy);
  return *radius > FP_TOLERANCE;
}

/**
 * @brief Construct a circular buffer boundary arc from three points.
 */
static bool
buffer_piece_from_arc(POINT2D p1, POINT2D p2, POINT2D p3, BufferPiece *piece)
{
  assert(piece);
  double cx, cy, radius;
  if (! buffer_circle_from_points(p1, p2, p3, &cx, &cy, &radius))
    return false;
  double theta1 = atan2(p1.y - cy, p1.x - cx);
  double theta2 = atan2(p3.y - cy, p3.x - cx);
  double thetam = atan2(p2.y - cy, p2.x - cx);

  /* Determine which direction from theta1 to theta2 passes through the
   * middle point */
  double ccw_sweep = angle_normalize(theta2 - theta1);
  double ccw_middle = angle_normalize(thetam - theta1);
  bool ccw = ccw_middle <= ccw_sweep + FP_TOLERANCE;
  piece->type = BUFFER_ARC;
  piece->x1 = p1.x;
  piece->y1 = p1.y;
  piece->x2 = p3.x;
  piece->y2 = p3.y;
  piece->cx = cx;
  piece->cy = cy;
  piece->radius = radius;
  piece->theta1 = theta1;
  piece->theta2 = theta2;
  piece->ccw = ccw;
  return true;
}

/**
 * @brief Extract the pieces of a buffer boundary.
 * @details Circular-string triples are interpreted as circular arcs.
 * Degenerate triples are interpreted as linear pieces.
 */
static bool
buffer_ring_pieces(const LWGEOM *geom, BufferPiece **pieces, uint32_t *count)
{
  assert(geom); assert(pieces); assert(count);
  *pieces = NULL;
  *count = 0;
  POINT2D *points = NULL;
  uint32_t npoints = 0;
  if (! buffer_ring_points(geom, &points, &npoints))
    return false;
  if (npoints < 2)
  {
    pfree(points);
    return false;
  }

  /* A CircularString normally contains one start point followed by
   * pairs of points. Allocate one piece per possible interval. */
  uint32_t capacity = npoints;
  BufferPiece *result = palloc(sizeof(BufferPiece) * capacity);
  uint32_t npieces = 0;

  /* A buffer constructed by meos_buffer_line() uses triples for
   * circular arcs. Linear pieces are represented by three collinear points. */
  uint32_t i = 0;
  while (i + 2 < npoints)
  {
    POINT2D p1 = points[i];
    POINT2D p2 = points[i + 1];
    POINT2D p3 = points[i + 2];
    BufferPiece piece;
    /* Try to interpret the triple as a circular arc */
    if (buffer_piece_from_arc(p1, p2, p3, &piece))
    {
      result[npieces++] = piece;
      i += 2;
      continue;
    }
    /* Collinear points represent a straight portion */
    piece.type = BUFFER_SEGMENT;
    piece.x1 = p1.x;
    piece.y1 = p1.y;
    piece.x2 = p2.x;
    piece.y2 = p2.y;
    result[npieces++] = piece;
    i++;
  }

  /* Handle a final segment if the point representation does not
   * terminate exactly on a circular-string triple */
  if (i + 1 < npoints)
  {
    BufferPiece piece;
    piece.type = BUFFER_SEGMENT;
    piece.x1 = points[i].x;
    piece.y1 = points[i].y;
    piece.x2 = points[i + 1].x;
    piece.y2 = points[i + 1].y;
    result[npieces++] = piece;
  }
  pfree(points);
  if (npieces == 0)
  {
    pfree(result);
    return false;
  }
  *pieces = result;
  *count = npieces;
  return true;
}

/**
 * @brief Extract the chord segments of a circular-string buffer ring.
 * @details The current buffer representation stores circular arcs
 * as three-point circular-string arcs. Each consecutive pair of points
 * is therefore used as a candidate boundary segment.
 */
static bool
buffer_ring_segments(const LWGEOM *geom, POINT2D **points, uint32_t *count)
{
  assert(geom); assert(points); assert(count);
  *points = NULL;
  *count = 0;
  if (! buffer_is_circular_ring(geom))
    return false;
  const LWCURVEPOLY *curvepoly = (const LWCURVEPOLY *) geom;
  const LWGEOM *ringgeom = curvepoly->rings[0];
  const LWCIRCSTRING *ring = (const LWCIRCSTRING *) ringgeom;
  if (! ring->points || ring->points->npoints < 2)
    return false;
  uint32_t npoints = ring->points->npoints;
  POINT2D *result = palloc(sizeof(POINT2D) * npoints);
  for (uint32_t i = 0; i < npoints; i++)
  {
    POINT4D p;
    getPoint4d_p(ring->points, i, &p);
    result[i].x = p.x;
    result[i].y = p.y;
  }
  *points = result;
  *count = npoints;
  return true;
}

/**
 * @brief Add intersections between two linear buffer pieces.
 * @details Both point intersections and linear overlaps are handled.
 * For an overlap, the two endpoints of the common interval become
 * topological nodes.
 */
static void
buffer_intersect_segments(const BufferPiece *a, const BufferPiece *b,
  MeosArray *intersections)
{
  assert(a); assert(b); assert(intersections);
  double adx = a->x2 - a->x1;
  double ady = a->y2 - a->y1;
  IntersectResult result = linesegm_intersect(a->x1, a->y1, adx, ady,
    b->x1, b->y1, b->x2, b->y2);
  if (result.type == INTERSECT_POINT)
  {
    double x = a->x1 + result.t0 * adx;
    double y = a->y1 + result.t0 * ady;
    buffer_intersections_add(intersections, x, y);
  }
  else if (result.type == INTERSECT_OVERLAP)
  {
    /* An overlap has two boundary nodes */
    double x0 = a->x1 + result.t0 * adx;
    double y0 = a->y1 + result.t0 * ady;
    double x1 = a->x1 + result.t1 * adx;
    double y1 = a->y1 + result.t1 * ady;
    buffer_intersections_add(intersections, x0, y0);
    buffer_intersections_add(intersections, x1, y1);
  }
}

/**
 * @brief Add the endpoints of a linear overlap to the intersection array.
 * @details Unlike a point intersection, an overlapping pair of segments has
 * two topological nodes: the beginning and end of the common interval.
 * These nodes are subsequently used to split both boundary pieces.
 */
static bool
buffer_add_segment_overlap_nodes(const BufferPiece *a, const BufferPiece *b,
  MeosArray *intersections)
{
  assert(a); assert(b); assert(intersections);
  double adx = a->x2 - a->x1;
  double ady = a->y2 - a->y1;
  IntersectResult result = linesegm_intersect(a->x1, a->y1, adx, ady, b->x1,
    b->y1, b->x2, b->y2);
  if (result.type != INTERSECT_OVERLAP)
    return false;
  /* t0 and t1 are the parameters along segment A delimiting the
   * overlapping interval */
  double x0 = a->x1 + result.t0 * adx;
  double y0 = a->y1 + result.t0 * ady;
  double x1 = a->x1 + result.t1 * adx;
  double y1 = a->y1 + result.t1 * ady;
  buffer_intersections_add(intersections, x0, y0);
  buffer_intersections_add(intersections, x1, y1);
  return true;
}

/**
 * @brief Test whether an angle lies on a circular arc.
 * @details The arc is directed from theta0 to theta1 according to ccw.
 */
static bool
buffer_angle_on_arc(double theta, double theta0, double theta1, bool ccw)
{
  double sweep, delta;
  if (ccw)
  {
    sweep = angle_normalize(theta1 - theta0);
    delta = angle_normalize(theta - theta0);
  }
  else
  {
    sweep = angle_normalize(theta0 - theta1);
    delta = angle_normalize(theta0 - theta);
  }
  return delta <= sweep + FP_TOLERANCE;
}

/**
 * @brief Test whether a point lies on a circular buffer arc.
 */
static bool
buffer_point_on_arc(const BufferPiece *arc, double x, double y)
{
  assert(arc); assert(arc->type == BUFFER_ARC);
  double theta = atan2(y - arc->cy, x - arc->cx);
  return buffer_angle_on_arc(theta, arc->theta1, arc->theta2, arc->ccw);
}

/**
 * @brief Add intersections between two circular arcs.
 * @details The circle-circle intersection is computed analytically. Candidate
 * intersection points are retained only when they lie on both finite
 * circular arcs.
 */
static void
buffer_intersect_arcs(const BufferPiece *a, const BufferPiece *b,
  MeosArray *intersections)
{
  assert(a); assert(b); assert(intersections);
  assert(a->type == BUFFER_ARC); assert(b->type == BUFFER_ARC);

  /* Build Edge representations for the existing arc intersection
   * infrastructure.
   * BufferPiece:
   *   theta1 = start angle
   *   theta2 = end angle
   * Edge:
   *   theta0 = start angle
   *   theta1 = end angle
   */
  Edge edge_a, edge_b;
  memset(&edge_a, 0, sizeof(Edge));
  memset(&edge_b, 0, sizeof(Edge));

  /* Edge a */
  edge_a.etype = EDGE_ARC;
  edge_a.x1 = a->x1;
  edge_a.y1 = a->y1;
  edge_a.x2 = a->x2;
  edge_a.y2 = a->y2;
  edge_a.cx = a->cx;
  edge_a.cy = a->cy;
  edge_a.radius = a->radius;
  edge_a.theta0 = a->theta1;
  edge_a.theta1 = a->theta2;
  edge_a.ccw = a->ccw;
  /* Edge b */
  edge_b.etype = EDGE_ARC;
  edge_b.x1 = b->x1;
  edge_b.y1 = b->y1;
  edge_b.x2 = b->x2;
  edge_b.y2 = b->y2;
  edge_b.cx = b->cx;
  edge_b.cy = b->cy;
  edge_b.radius = b->radius;
  edge_b.theta0 = b->theta1;
  edge_b.theta1 = b->theta2;
  edge_b.ccw = b->ccw;

  /* First use the existing arc/arc intersection predicate. This also
   * rejects arcs whose finite angular ranges do not intersect. */
  if (! arcarc_intersect(&edge_a, &edge_b))
    return;

  double dx = b->cx - a->cx;
  double dy = b->cy - a->cy;
  double d = hypot(dx, dy);

  /* Concentric circles have either no intersections or infinitely many
   * intersections. Neither case produces a finite node here. */
  if (d <= FP_TOLERANCE)
    return;

  double r1 = a->radius;
  double r2 = b->radius;

  /* Reject circles which are too far apart or where one circle is
   * completely inside the other. */
  if (d > r1 + r2 + FP_TOLERANCE)
    return;

  if (d < fabs(r1 - r2) - FP_TOLERANCE)
    return;

  /* Circle-circle intersection.
   * x is the distance from the centre of circle a to the radical
   * chord along the centre-to-centre direction. */
  double x = (r1 * r1 - r2 * r2 + d * d) / (2.0 * d);
  double h2 = r1 * r1 - x * x;
  if (h2 < -FP_TOLERANCE)
    return;
  if (h2 < 0.0)
    h2 = 0.0;
  double h = sqrt(h2);
  double ux = dx / d;
  double uy = dy / d;
  /* Base point on the radical chord */
  double px = a->cx + x * ux;
  double py = a->cy + x * uy;

  /* First candidate */
  double ix = px - h * uy;
  double iy = py + h * ux;
  if (buffer_point_on_arc(a, ix, iy) && buffer_point_on_arc(b, ix, iy))
    buffer_intersections_add(intersections, ix, iy);

  /* Second candidate.
   * For tangent circles h == 0, this is the same point as above and must
   * not be inserted twice. */
  if (h > FP_TOLERANCE)
  {
    ix = px + h * uy;
    iy = py - h * ux;
    if (buffer_point_on_arc(a, ix, iy) &&  buffer_point_on_arc(b, ix, iy))
      buffer_intersections_add(intersections, ix, iy);
  }
}

/**
 * @brief Add intersections between a linear piece and a circular arc.
 * @details The existing arc/segment intersection routine returns parameters
 * along the segment. The resulting points are additionally checked against the
 * finite angular extent of the circular arc.
 */
static void
buffer_intersect_segment_arc(const BufferPiece *segment,
  const BufferPiece *arc, MeosArray *intersections)
{
  assert(segment); assert(arc); assert(intersections);
  assert(segment->type == BUFFER_SEGMENT); assert(arc->type == BUFFER_ARC);

  Edge edge_segment, edge_arc;
  memset(&edge_segment, 0, sizeof(Edge));
  memset(&edge_arc, 0, sizeof(Edge));

  /* Segment */
  edge_segment.etype = EDGE_LINE;
  edge_segment.x1 = segment->x1;
  edge_segment.y1 = segment->y1;
  edge_segment.x2 = segment->x2;
  edge_segment.y2 = segment->y2;
  edge_segment.dx = segment->x2 - segment->x1;
  edge_segment.dy = segment->y2 - segment->y1;

  /* Arc */
  edge_arc.etype = EDGE_ARC;
  edge_arc.x1 = arc->x1;
  edge_arc.y1 = arc->y1;
  edge_arc.x2 = arc->x2;
  edge_arc.y2 = arc->y2;
  edge_arc.cx = arc->cx;
  edge_arc.cy = arc->cy;
  edge_arc.radius = arc->radius;
  edge_arc.theta0 = arc->theta1;
  edge_arc.theta1 = arc->theta2;
  edge_arc.ccw = arc->ccw;

  double roots[2];
  int n = arcsegm_intersect(edge_segment.x1, edge_segment.y1, edge_segment.dx,
    edge_segment.dy, &edge_arc, roots);

  /* arcsegm_intersect() returns the parameter(s) along the segment.
   * Recover the coordinates and verify that the point belongs to the
   * finite circular arc. */
  for (int i = 0; i < n; i++)
  {
    double x = edge_segment.x1 + roots[i] * edge_segment.dx;
    double y = edge_segment.y1 + roots[i] * edge_segment.dy;
    if (buffer_point_on_arc(arc, x, y))
      buffer_intersections_add(intersections, x, y);
  }
}

/**
 * @brief Find exact boundary intersections between two buffers.
 * @details Linear/linear, linear/arc and arc/arc intersections are handled
 * separately. The resulting points are the nodes that will subsequently be
 * used to split the buffer boundaries.
 */
static void
buffer_find_boundary_intersections(const LWGEOM *geom1, const LWGEOM *geom2,
  MeosArray *intersections)
{
  assert(geom1); assert(geom2); assert(intersections);
  BufferPiece *pieces1 = NULL, *pieces2 = NULL;
  uint32_t count1 = 0, count2 = 0;
  if (! buffer_ring_pieces(geom1, &pieces1, &count1))
    return;
  if (! buffer_ring_pieces(geom2, &pieces2, &count2))
  {
    pfree(pieces1);
    return;
  }
  for (uint32_t i = 0; i < count1; i++)
  {
    const BufferPiece *a = &pieces1[i];
    for (uint32_t j = 0; j < count2; j++)
    {
      const BufferPiece *b = &pieces2[j];
      if (a->type == BUFFER_SEGMENT && b->type == BUFFER_SEGMENT)
        buffer_intersect_segments(a, b, intersections);
      else if (a->type == BUFFER_SEGMENT && b->type == BUFFER_ARC)
        buffer_intersect_segment_arc(a, b, intersections);
      else if (a->type == BUFFER_ARC && b->type == BUFFER_SEGMENT)
        buffer_intersect_segment_arc(b, a, intersections);
      else
        buffer_intersect_arcs(a, b, intersections);
    }
  }
  pfree(pieces1); pfree(pieces2);
}

/**
 * @brief Compute all boundary intersection nodes between two buffers.
 * @return Number of unique intersection nodes.
 */
static uint32_t
buffer_compute_intersections(const LWGEOM *geom1, const LWGEOM *geom2,
  MeosArray *intersections)
{
  assert(geom1); assert(geom2); assert(intersections);
  intersections = meos_array_create(sizeof(POINT2D));
  buffer_find_boundary_intersections(geom1, geom2, intersections);
  return intersections->count;
}

/*****************************************************************************
 * Buffer overlay - coincident piece equivalence
 *****************************************************************************/

/**
 * @brief Return true if two scalar values are equal within the geometric
 * tolerance used by the buffer overlay.
 */
static bool
buffer_values_equal(double a, double b)
{
  return fabs(a - b) <= FP_TOLERANCE;
}

/**
 * @brief Return true if two points are equal within the geometric tolerance.
 */
static bool
buffer_piece_points_equal(double x1, double y1, double x2, double y2)
{
  return buffer_values_equal(x1, x2) && buffer_values_equal(y1, y2);
}

/**
 * @brief Normalize an angle to [0, 2*pi).
 */
static double
buffer_normalize_angle(double theta)
{
  theta = fmod(theta, 2.0 * M_PI);
  if (theta < 0.0)
    theta += 2.0 * M_PI;
  return theta;
}

/**
 * @brief Return true if two angles are equal modulo 2*pi.
 */
static bool
buffer_angles_equal(double a, double b)
{
  double d = buffer_normalize_angle(a) - buffer_normalize_angle(b);
  d = fabs(d);
  if (d > M_PI)
    d = 2.0 * M_PI - d;
  return d <= FP_TOLERANCE;
}

/**
 * @brief Return true if two straight buffer pieces represent the same
 * geometric segment, independently of orientation.
 */
static bool
buffer_segments_equal(const BufferPiece *a, const BufferPiece *b)
{
  assert(a); assert(b);
  if (a->type != BUFFER_SEGMENT || b->type != BUFFER_SEGMENT)
    return false;
  if (buffer_piece_points_equal(a->x1, a->y1, b->x1, b->y1) &&
      buffer_piece_points_equal(a->x2, a->y2, b->x2, b->y2))
    return true;
  return
    buffer_piece_points_equal(a->x1, a->y1, b->x2, b->y2) &&
    buffer_piece_points_equal(a->x2, a->y2, b->x1, b->y1);
}

/**
 * @brief Return true if two circular buffer pieces lie on the same circle.
 */
static bool
buffer_arcs_same_circle(const BufferPiece *a, const BufferPiece *b)
{
  assert(a); assert(b);
  if (a->type != BUFFER_ARC || b->type != BUFFER_ARC)
    return false;
  return
    buffer_values_equal(a->cx, b->cx) &&
    buffer_values_equal(a->cy, b->cy) &&
    buffer_values_equal(a->radius, b->radius);
}

/**
 * @brief Return true if a circular arc contains an angular position.
 * @details The test is orientation independent: it only tests whether theta
 * lies on the geometric arc.
 */
static bool
buffer_arc_contains_angle(const BufferPiece *arc, double theta)
{
  assert(arc); assert(arc->type == BUFFER_ARC);
  double start = buffer_normalize_angle(arc->theta1);
  double end = buffer_normalize_angle(arc->theta2);
  theta = buffer_normalize_angle(theta);
  double sweep;
  if (arc->ccw)
    sweep = buffer_normalize_angle(end - start);
  else
    sweep = buffer_normalize_angle(start - end);
  double position;
  if (arc->ccw)
    position = buffer_normalize_angle(theta - start);
  else
    position = buffer_normalize_angle(start - theta);
  return position <= sweep + FP_TOLERANCE;
}

/**
 * @brief Return true if two circular arcs represent the same geometric
 * arc, independently of traversal direction.
 */
static bool
buffer_arcs_equal(const BufferPiece *a, const BufferPiece *b)
{
  assert(a); assert(b);
  if (! buffer_arcs_same_circle(a, b))
    return false;

  /* The two endpoints must be the same geometric points. 
   * Orientation is deliberately ignored. */
  bool same_endpoints =
    buffer_piece_points_equal(a->x1, a->y1, b->x1, b->y1) &&
    buffer_piece_points_equal(a->x2, a->y2, b->x2, b->y2);
  bool reversed_endpoints =
    buffer_piece_points_equal(a->x1, a->y1, b->x2, b->y2) &&
    buffer_piece_points_equal(a->x2, a->y2, b->x1, b->y1);
  if (! same_endpoints && ! reversed_endpoints)
    return false;

  /* Equal endpoints alone are not enough for circles because there are two
   * possible arcs between two points. Check the midpoint of A  and verify
   * that it lies on B. */
  double sweep;
  if (a->ccw)
    sweep = buffer_normalize_angle(a->theta2 - a->theta1);
  else
    sweep = buffer_normalize_angle(a->theta1 - a->theta2);
  double mid;
  if (a->ccw)
    mid = a->theta1 + sweep * 0.5;
  else
    mid = a->theta1 - sweep * 0.5;
  return buffer_arc_contains_angle(b, mid);
}

/**
 * @brief Return true if two buffer pieces represent the same
 * geometric locus.
 * @details The orientation of the pieces is ignored.
 */
static bool
buffer_pieces_equal(const BufferPiece *a, const BufferPiece *b)
{
  assert(a); assert(b);
  if (a->type != b->type)
    return false;
  if (a->type == BUFFER_SEGMENT)
    return buffer_segments_equal(a, b);
  if (a->type == BUFFER_ARC)
    return buffer_arcs_equal(a, b);
  return false;
}

/**
 * @brief Return true if a piece is already present in an array.
 */
static bool
buffer_piece_array_contains(const MeosArray *pieces, const BufferPiece *piece)
{
  assert(pieces); assert(piece);
  for (uint32_t i = 0; i < pieces->count; i++)
  {
    BufferPiece *piece_i = (BufferPiece *) meos_array_get(pieces, i);
    if (buffer_pieces_equal(piece_i, piece))
      return true;
  }
  return false;
}

/*****************************************************************************
 * Native buffer boundary splitting
 *****************************************************************************/

/**
 * @brief Add a straight subpiece to a piece array.
 * @details Degenerate pieces are ignored.
 */
static void
buffer_segment_piece_add(MeosArray *pieces, double x1, double y1,
  double x2, double y2)
{
  assert(pieces);
  if (buffer_piece_points_equal(x1, y1, x2, y2))
    return;
  BufferPiece piece = {0};
  piece.type = BUFFER_SEGMENT;
  piece.x1 = x1;
  piece.y1 = y1;
  piece.x2 = x2;
  piece.y2 = y2;
  meos_array_add(pieces, &piece);
}

/**
 * @brief Add a piece to an array unless an equivalent geometric piece is
 * already present.
 */
static void
buffer_pieces_add_unique(MeosArray *pieces, BufferPiece *piece)
{
  assert(pieces); assert(piece);
  if (! buffer_piece_array_contains(pieces, piece))
    meos_array_add(pieces, piece);
}

/**
 * @brief Return the directed angular parameter of a point on an arc.
 * @details The returned value is an angular distance from the start of the 
 * arc, measured in the direction of travel.
 * - For a CCW arc: parameter = angle - theta1
 * - For a CW arc:  parameter = theta1 - angle
 * The result is normalized to [0, 2*pi).
 */
static double
buffer_arc_parameter(const BufferPiece *piece, POINT2D *point)
{
  assert(piece); assert(point); assert(piece->type == BUFFER_ARC);
  double theta = atan2(point->y - piece->cy, point->x - piece->cx);
  if (piece->ccw)
    return angle_normalize(theta - piece->theta1);
  return angle_normalize(piece->theta1 - theta);
}

/**
 * @brief Return the total angular sweep of an arc.
 */
static double
buffer_arc_sweep(const BufferPiece *piece)
{
  assert(piece); assert(piece->type == BUFFER_ARC);
  if (piece->ccw)
    return angle_normalize(piece->theta2 - piece->theta1);
  return angle_normalize(piece->theta1 - piece->theta2);
}

/**
 * @brief Return true if a point belongs to a buffer piece.
 * @details This is a geometric test rather than an intersection test.
 * It is used to determine which collected nodes belong to a particular
 * boundary piece before splitting it.
 */
static bool
buffer_piece_contains_point(const BufferPiece *piece, POINT2D *point)
{
  assert(piece); assert(point);
  if (piece->type == BUFFER_SEGMENT)
  {
    double dx = piece->x2 - piece->x1;
    double dy = piece->y2 - piece->y1;
    double px = point->x - piece->x1;
    double py = point->y - piece->y1;
    /* Collinearity test */
    double cross = buffer_cross(dx, dy, px, py);
    double scale = fmax(1.0, hypot(dx, dy) * hypot(px, py));
    if (fabs(cross) > FP_TOLERANCE * scale)
      return false;
    /* Bounding-box test */
    if (point->x < fmin(piece->x1, piece->x2) - FP_TOLERANCE ||
        point->x > fmax(piece->x1, piece->x2) + FP_TOLERANCE ||
        point->y < fmin(piece->y1, piece->y2) - FP_TOLERANCE ||
        point->y > fmax(piece->y1, piece->y2) + FP_TOLERANCE)
      return false;
    return true;
  }

  if (piece->type == BUFFER_ARC)
  {
    double dx = point->x - piece->cx;
    double dy = point->y - piece->cy;
    double distance = hypot(dx, dy);
    /* First check that the point is on the supporting circle */
    if (fabs(distance - piece->radius) >
        FP_TOLERANCE * fmax(1.0, piece->radius))
      return false;
    /* Then check that its angle lies within the finite arc */
    return buffer_point_on_arc(piece, point->x, point->y);
  }
  return false;
}

/**
 * @brief Sort split points according to their position on a boundary piece.
 */
static int
buffer_split_point_cmp(const void *a, const void *b)
{
  assert(a); assert(b);
  const BufferSplitPoint *p1 = (const BufferSplitPoint *) a;
  const BufferSplitPoint *p2 = (const BufferSplitPoint *) b;
  if (p1->parameter < p2->parameter)
    return -1;
  if (p1->parameter > p2->parameter)
    return 1;
  return 0;
}

/**
 * @brief Add a split point if it is not already present.
 */
static void
buffer_split_point_add(BufferSplitPoint *points, uint32_t *count,
  uint32_t capacity, POINT2D *point, double parameter)
{
  assert(points); assert(count); assert(point);
  /* Duplicate parameters correspond either to duplicate intersection
   * nodes or to an intersection occurring at an existing endpoint */
  for (uint32_t i = 0; i < *count; i++)
  {
    if (fabs(points[i].parameter - parameter) <= FP_TOLERANCE)
      return;
  }
  if (*count >= capacity)
    return;
  points[*count].point = *point;
  points[*count].parameter = parameter;
  (*count)++;
}

/**
 * @brief Split a linear buffer piece at the supplied intersection nodes.
 */
static void
buffer_split_segment(const BufferPiece *piece, const MeosArray *intersections,
  MeosArray *result)
{
  assert(piece); assert(intersections); assert(result);
  assert(piece->type == BUFFER_SEGMENT);
  /* At most all intersection points plus the two endpoints */
  uint32_t capacity = intersections->count + 2;
  BufferSplitPoint *points = palloc(sizeof(BufferSplitPoint) * capacity);
  uint32_t count = 0;
  POINT2D start = {piece->x1, piece->y1};
  POINT2D end = {piece->x2, piece->y2};
  buffer_split_point_add(points, &count, capacity, &start, 0.0);
  buffer_split_point_add(points, &count, capacity, &end, 1.0);
  for (uint32_t i = 0; i < intersections->count; i++)
  {
    POINT2D *point = (POINT2D *) meos_array_get(intersections, i);
    if (! buffer_piece_contains_point(piece, point))
      continue;
    double parameter = buffer_segment_parameter(piece, point->x, point->y);
    /* Ignore nodes outside the segment due to numerical noise */
    if (parameter < -FP_TOLERANCE || parameter > 1.0 + FP_TOLERANCE)
      continue;
    if (parameter < 0.0)
      parameter = 0.0;
    else if (parameter > 1.0)
      parameter = 1.0;
    buffer_split_point_add(points, &count, capacity, point, parameter);
  }
  qsort(points, count, sizeof(BufferSplitPoint), buffer_split_point_cmp);

  /* Generate one segment between every pair of consecutive nodes */
  for (uint32_t i = 0; i + 1 < count; i++)
  {
    POINT2D p1 = points[i].point;
    POINT2D p2 = points[i + 1].point;
    if (hypot(p2.x - p1.x, p2.y - p1.y) <= FP_TOLERANCE)
      continue;
    BufferPiece split;
    memset(&split, 0, sizeof(BufferPiece));
    split.type = BUFFER_SEGMENT;
    split.x1 = p1.x;
    split.y1 = p1.y;
    split.x2 = p2.x;
    split.y2 = p2.y;
    meos_array_add(result, &split);
  }
  pfree(points);
}

/**
 * @brief Split all linear pieces of a buffer boundary at the supplied
 * intersection nodes.
 * @details Circular arcs are preserved unchanged for now. They will be handled
 * separately once the linear splitting path is validated.
 */
static bool
buffer_split_boundary_pieces(BufferPiece *pieces, uint32_t count,
  const MeosArray *intersections, MeosArray *result)
{
  assert(pieces); assert(intersections); assert(result);
  for (uint32_t i = 0; i < count; i++)
  {
    BufferPiece *piece = &pieces[i];
    if (piece->type == BUFFER_SEGMENT)
      buffer_split_segment(piece, intersections, result);
    else
      /* Circular-arc splitting will be added in a later slice.
       * Keeping the original arc here is important: we do not polygonize
       * it and we do not approximate it by chords. */
      meos_array_add(result, piece);
  }
  return true;
}

/**
 * @brief Extract and split the boundary pieces of a buffer.
 * @details The returned array contains linear pieces split at all supplied
 * intersection nodes, while circular pieces are currently preserved.
 */
static bool
buffer_ring_pieces_split(const LWGEOM *geom, const MeosArray *intersections,
  MeosArray *result)
{
  assert(geom); assert(intersections); assert(result);
  BufferPiece *pieces = NULL;
  uint32_t count = 0;
  if (! buffer_ring_pieces(geom, &pieces, &count))
    return false;
  bool success = buffer_split_boundary_pieces(pieces, count, intersections,
    result);
  pfree(pieces);
  return success;
}

/**
 * @brief Build the split boundary pieces of two buffer geometries.
 * @details The same global set of boundary intersection nodes is applied to
 * both boundaries. Linear pieces are split at those nodes. Circular arcs are
 * currently preserved unchanged and will be split in a later slice.
 */
static bool
buffer_build_split_boundary_pieces(const LWGEOM *geom1, const LWGEOM *geom2,
  const MeosArray *intersections, MeosArray *pieces1, MeosArray *pieces2)
{
  assert(geom1); assert(geom2); assert(intersections);
  assert(pieces1); assert(pieces2);
  if (! buffer_ring_pieces_split(geom1, intersections, pieces1))
    return false;
  if (! buffer_ring_pieces_split(geom2, intersections, pieces2))
    return false;
  return true;
}

/**
 * @brief Return true if two piece arrays contain at least one
 * geometrically coincident piece.
 * @details This is only a diagnostic/topology primitive at this stage.
 * It does not modify either array.
 */
static bool
buffer_piece_arrays_have_coincident_piece(const MeosArray *pieces1,
  const MeosArray *pieces2)
{
  assert(pieces1); assert(pieces2);
  for (uint32_t i = 0; i < pieces1->count; i++)
  {
    for (uint32_t j = 0; j < pieces2->count; j++)
    {
      BufferPiece *piece_i = (BufferPiece *) meos_array_get(pieces1, i);
      BufferPiece *piece_j = (BufferPiece *) meos_array_get(pieces2, j);
      if (buffer_pieces_equal(piece_i, piece_j))
        return true;
    }
  }
  return false;
}

/**
 * @brief Split a circular buffer piece at supplied intersection nodes.
 */
static void
buffer_split_arc(const BufferPiece *piece, const MeosArray *intersections,
  MeosArray *result)
{
  assert(piece); assert(intersections); assert(result);
  assert(piece->type == BUFFER_ARC);
  uint32_t capacity = intersections->count + 2;
  BufferSplitPoint *points = palloc(sizeof(BufferSplitPoint) * capacity);
  uint32_t count = 0;
  POINT2D start = {piece->x1, piece->y1};
  POINT2D end = {piece->x2, piece->y2};
  double sweep = buffer_arc_sweep(piece);
  buffer_split_point_add(points, &count, capacity, &start, 0.0);
  buffer_split_point_add(points, &count, capacity, &end, sweep);
  for (uint32_t i = 0; i < intersections->count; i++)
  {
    POINT2D *point = (POINT2D *) meos_array_get(intersections, i);
    if (! buffer_piece_contains_point(piece, point))
      continue;
    double parameter = buffer_arc_parameter(piece, point);
    /* Ignore nodes outside the finite arc */
    if (parameter < -FP_TOLERANCE || parameter > sweep + FP_TOLERANCE)
      continue;
    if (parameter < 0.0)
      parameter = 0.0;
    else if (parameter > sweep)
      parameter = sweep;
    buffer_split_point_add(points, &count, capacity, point, parameter);
  }
  qsort(points, count, sizeof(BufferSplitPoint), buffer_split_point_cmp);
  /* Generate one circular arc between every consecutive pair of nodes */
  for (uint32_t i = 0; i + 1 < count; i++)
  {
    double theta_start, theta_end;
    if (piece->ccw)
    {
      theta_start = piece->theta1 + points[i].parameter;
      theta_end = piece->theta1 + points[i + 1].parameter;
    }
    else
    {
      theta_start = piece->theta1 - points[i].parameter;
      theta_end = piece->theta1 - points[i + 1].parameter;
    }
    double sub_sweep = points[i + 1].parameter - points[i].parameter;
    if (sub_sweep <= FP_TOLERANCE)
      continue;

    /* Use the actual sorted intersection coordinates as endpoints.
     * This avoids unnecessarily recomputing the coordinates and preserves
     * the exact node shared with another boundary piece. */
    POINT2D p1 = points[i].point;
    POINT2D p2 = points[i + 1].point;
    BufferPiece split;
    memset(&split, 0, sizeof(BufferPiece));
    split.type = BUFFER_ARC;
    split.x1 = p1.x;
    split.y1 = p1.y;
    split.x2 = p2.x;
    split.y2 = p2.y;
    split.cx = piece->cx;
    split.cy = piece->cy;
    split.radius = piece->radius;
    split.theta1 = theta_start;
    split.theta2 = theta_end;
    split.ccw = piece->ccw;
    meos_array_add(result, &split);
  }
  pfree(points);
}

/**
 * @brief Split all pieces of a buffer boundary at intersection nodes.
 * @details Every resulting piece is either a straight segment or an exact
 * circular arc. Circular arcs are never replaced by chords.
 */
static void
buffer_split_pieces(const BufferPiece *pieces, uint32_t count,
  const MeosArray *intersections, MeosArray *result)
{
  assert(pieces || count == 0); assert(intersections); assert(result);
  for (uint32_t i = 0; i < count; i++)
  {
    if (pieces[i].type == BUFFER_SEGMENT)
      buffer_split_segment(&pieces[i], intersections, result);
    else if (pieces[i].type == BUFFER_ARC)
      buffer_split_arc(&pieces[i], intersections, result);
  }
}

/*****************************************************************************
 * Native buffer boundary classification
 *****************************************************************************/

/**
 * @brief Compute a representative point in the interior of a buffer piece.
 * @details For a segment the midpoint is used. For a circular arc the angular
 * midpoint is used. The returned point is therefore exactly on the supporting
 * circle and does not approximate the arc by a chord.
 */
static bool
buffer_piece_midpoint(const BufferPiece *piece, POINT2D *point)
{
  assert(piece); assert(point);
  if (piece->type == BUFFER_SEGMENT)
  {
    point->x = (piece->x1 + piece->x2) * 0.5;
    point->y = (piece->y1 + piece->y2) * 0.5;
    return true;
  }
  if (piece->type == BUFFER_ARC)
  {
    double sweep = buffer_arc_sweep(piece);
    if (sweep <= FP_TOLERANCE)
      return false;
    double theta;
    if (piece->ccw)
      theta = piece->theta1 + sweep * 0.5;
    else
      theta = piece->theta1 - sweep * 0.5;
    point->x = piece->cx + piece->radius * cos(theta);
    point->y = piece->cy + piece->radius * sin(theta);
    return true;
  }
  return false;
}

/**
 * @brief Classify one split boundary piece with respect to a geometry.
 * @details The representative point is located against the complete other
 * buffer. This is a classification of the boundary itself:
 * - EXTERIOR  -> the piece lies outside the other buffer and may belong
 *   to the exterior union boundary.
 * - INTERIOR  -> the piece lies inside the other buffer and must not
 *   belong to the exterior union boundary.
 * - BOUNDARY  -> the piece coincides with, or touches, the other boundary.
 *   This case is retained for the later coincident-boundary handling.
 */
static BufferPieceLocation
buffer_classify_piece(const BufferPiece *piece, const LWGEOM *other)
{
  assert(piece); assert(other);
  POINT2D midpoint;
  if (! buffer_piece_midpoint(piece, &midpoint))
    return BUFFER_PIECE_BOUNDARY;
  int location = buffer_point_location(other, midpoint.x, midpoint.y);
  switch (location)
  {
    case 0:
      return BUFFER_PIECE_INTERIOR;
    case 1:
      return BUFFER_PIECE_BOUNDARY;
    case 2:
      return BUFFER_PIECE_EXTERIOR;
    default:
      return BUFFER_PIECE_BOUNDARY;
  }
}

/**
 * @brief Classify all split pieces with respect to another buffer.
 * @details The resulting arrays contain copies of the pieces and are
 * independent of the input array.
 */
static void
buffer_classify_pieces(const MeosArray *pieces, const LWGEOM *other,
  MeosArray *exterior, MeosArray *interior, MeosArray *boundary)
{
  assert(pieces); assert(other); assert(exterior); assert(interior);
  assert(boundary);
  for (uint32_t i = 0; i < pieces->count; i++)
  {
    BufferPiece *piece = (BufferPiece *) meos_array_get(pieces, i);
    BufferPieceLocation location = buffer_classify_piece(piece, other);
    switch (location)
    {
      case BUFFER_PIECE_EXTERIOR:
        meos_array_add(exterior, &piece);
        break;
      case BUFFER_PIECE_INTERIOR:
        meos_array_add(interior, &piece);
        break;
      case BUFFER_PIECE_BOUNDARY:
        meos_array_add(boundary, &piece);
        break;
      default:
        break;
    }
  }
}

/**
 * @brief Split a buffer boundary and classify its pieces against another
 * buffer.
 * @details This is the first complete boundary-overlay primitive:
 *   boundary
 *   -> intersection nodes
 *   -> exact segment/arc splitting
 *   -> inside/outside classification
 */
static bool
buffer_split_and_classify(const LWGEOM *geom, const LWGEOM *other,
  const MeosArray *intersections, MeosArray *exterior, MeosArray *interior,
  MeosArray *boundary)
{
  assert(geom); assert(other); assert(intersections);
  assert(exterior); assert(interior); assert(boundary);
  BufferPiece *pieces = NULL;
  uint32_t count = 0;
  if (! buffer_ring_pieces(geom, &pieces, &count))
    return false;
  MeosArray *split = meos_array_create(sizeof(BufferPiece));
  buffer_split_pieces(pieces, count, intersections, split);
  buffer_classify_pieces(split, other, exterior, interior, boundary);
  meos_array_destroy(split); pfree(pieces);
  return true;
}

/*****************************************************************************
 * Buffer overlay - coincident boundary classification
 *****************************************************************************/

/**
 * @brief Compute a point on the left/right side of a buffer piece.
 * @details The piece orientation determines the tangent direction.
 * For a circular arc, the tangent is evaluated at the angular midpoint.
 * The returned points are very close to the boundary. They are used only to
 * determine which side of a coincident boundary is occupied by the other buffer.
 */
static bool
buffer_piece_side_points(const BufferPiece *piece, double epsilon,
  POINT2D *left, POINT2D *right)
{
  assert(piece); assert(left); assert(right);
  POINT2D midpoint;
  if (! buffer_piece_midpoint(piece, &midpoint))
    return false;
  double tx, ty;
  if (piece->type == BUFFER_SEGMENT)
  {
    tx = piece->x2 - piece->x1;
    ty = piece->y2 - piece->y1;
  }
  else if (piece->type == BUFFER_ARC)
  {
    double sweep = buffer_arc_sweep(piece);
    if (sweep <= FP_TOLERANCE)
      return false;
    double theta;
    if (piece->ccw)
      theta = piece->theta1 + sweep * 0.5;
    else
      theta = piece->theta1 - sweep * 0.5;
    /* Tangent to a circle */
    if (piece->ccw)
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
  else
  {
    return false;
  }
  double length = hypot(tx, ty);
  if (length <= FP_TOLERANCE)
    return false;
  tx /= length;
  ty /= length;
  /* Left-hand normal */
  double nx = -ty;
  double ny =  tx;
  left->x  = midpoint.x + epsilon * nx;
  left->y  = midpoint.y + epsilon * ny;
  right->x = midpoint.x - epsilon * nx;
  right->y = midpoint.y - epsilon * ny;
  return true;
}

/**
 * @brief Determine which side of a boundary piece is inside a geometry.
 * @return Return values:
 *   -1 : neither side is interior
 *    0 : left side is interior
 *    1 : right side is interior
 *    2 : both sides are interior
 * Boundary classification is treated conservatively: if an offset point
 * still falls on the boundary, the offset distance is increased.
 */
static int
buffer_piece_interior_side(const BufferPiece *piece, const LWGEOM *geom)
{
  assert(piece); assert(geom);
  /* Start with a small displacement relative to the piece itself */
  double scale = 1.0;
  double dx = piece->x2 - piece->x1;
  double dy = piece->y2 - piece->y1;
  if (piece->type == BUFFER_ARC)
    scale = piece->radius;
  else
    scale = hypot(dx, dy);
  if (scale <= FP_TOLERANCE)
    scale = 1.0;
  double epsilon = fmax(FP_TOLERANCE * 100.0, scale * 1.0e-8);
  for (int attempt = 0; attempt < 4; attempt++)
  {
    POINT2D left, right;
    if (! buffer_piece_side_points(piece, epsilon, &left, &right))
      return -1;
    int left_loc = buffer_point_location(geom, left.x, left.y);
    int right_loc = buffer_point_location(geom, right.x, right.y);

    /* buffer_point_location():
     *   0 = interior
     *   1 = boundary
     *   2 = exterior */
    if (left_loc != 1 && right_loc != 1)
    {
      bool left_inside = left_loc == 0;
      bool right_inside = right_loc == 0;
      if (left_inside && right_inside)
        return 2;
      if (left_inside)
        return 0;
      if (right_inside)
        return 1;
      return -1;
    }
    epsilon *= 10.0;
  }
  return -1;
}

/**
 * @brief Determine whether a coincident boundary piece is an exterior
 * boundary of the union.
 * @details This is based on the side occupied by the other geometry.
 * - If the other geometry occupies the same side as this buffer, the common
 *   boundary is external to the union.
 * - If the other geometry occupies the opposite side, the common boundary is
 *   internal to the union.
 */
static int
buffer_classify_coincident_piece(const BufferPiece *piece, const LWGEOM *owner,
  const LWGEOM *other)
{
  assert(piece); assert(owner); assert(other);
  int owner_side = buffer_piece_interior_side(piece, owner);
  int other_side = buffer_piece_interior_side(piece, other);
  /* We need to know the side occupied by both geometries. */
  if (owner_side < 0 || other_side < 0)
    return -1;
  /* Both geometries occupy the same side of the boundary.
   * The boundary is external to their union. */
  if (owner_side == other_side)
    return 1;
  /* The geometries occupy opposite sides.
   * The coincident boundary lies inside the union. */
  if ((owner_side == 0 && other_side == 1) ||
      (owner_side == 1 && other_side == 0))
    return 0;
  /* Degenerate/tangential situation. Defer it. */
  return -1;
}

/**
 * @brief Resolve coincident pieces belonging to one buffer.
 * @details If the coincident piece is external to the union, it is retained.
 * If it is internal, it is discarded.
 * @note The caller is responsible for preventing the same geometric piece
 * from being inserted twice when processing the second buffer.
 */
static bool
buffer_resolve_coincident_piece(BufferPiece *piece, const LWGEOM *owner,
  const LWGEOM *other, MeosArray *result)
{
  assert(piece); assert(owner); assert(other); assert(result);
  int classification = buffer_classify_coincident_piece(piece, owner, other);
  /* The coincident boundary is external to the union */
  if (classification == 1)
  {
    buffer_pieces_add_unique(result, piece);
    return true;
  }
  /* The coincident boundary is internal to the union */
  if (classification == 0)
    return true;

  /* Unknown / degenerate topology */
  return false;
}

/**
 * @brief Find a geometrically equivalent piece in a piece array.
 * @details Returns the index of the first equivalent piece, or @p UINT32_MAX 
 * when none exists.
 */
static uint32_t
buffer_find_equal_piece(const MeosArray *pieces, const BufferPiece *piece)
{
  assert(pieces); assert(piece);
  for (uint32_t i = 0; i < pieces->count; i++)
  {
    BufferPiece *piece_i = (BufferPiece *) meos_array_get(pieces, i);
    if (buffer_pieces_equal(piece_i, piece))
      return i;
  }
  return UINT32_MAX;
}

/**
 * @brief Resolve all coincident pieces between two split buffer boundaries.
 * @details Pieces which are exterior to the union are copied once to result.
 * Pieces which are internal to the union are discarded.
 * @note The two input arrays are assumed to have already been split at their
 * boundary intersection nodes.
 */
static bool
buffer_resolve_coincident_pieces(const MeosArray *pieces1,
  const MeosArray *pieces2, const LWGEOM *geom1, const LWGEOM *geom2,
  MeosArray *result)
{
  assert(pieces1); assert(pieces2); assert(geom1); assert(geom2);
  assert(result);
  /* First process pieces belonging to geometry 1 */
  for (uint32_t i = 0; i < pieces1->count; i++)
  {
    BufferPiece *piece = (BufferPiece *) meos_array_get(pieces1, i);
    uint32_t j = buffer_find_equal_piece(pieces2, piece);
    if (j == UINT32_MAX)
      continue;
    BufferPiece *other = (BufferPiece *) meos_array_get(pieces2, j);
    /* The piece is shared by both boundaries */
    if (! buffer_resolve_coincident_piece(piece, geom1, geom2, result))
    {
      /* Unknown topology. Leave the piece for the normal
       * boundary-selection stage */
      buffer_pieces_add_unique(result, piece);
    }
  }
  /* We do not process pieces2 here. Otherwise an external coincident
   * piece would be considered twice. */
  return true;
}

/**
 * @brief Select the non-coincident pieces which belong to the union boundary.
 * @details The input pieces have already been split at all boundary
 * intersection nodes. Coincident pieces are handled separately.
 * A non-coincident piece is part of the union boundary when its midpoint
 * is not in the interior of the other geometry.
 */
static void
buffer_select_split_noncoincident_pieces(const MeosArray *pieces,
  const LWGEOM *other, const MeosArray *other_pieces, MeosArray *result)
{
  assert(pieces); assert(other); assert(other_pieces); assert(result);
  for (uint32_t i = 0; i < pieces->count; i++)
  {
    BufferPiece *piece = (BufferPiece *) meos_array_get(pieces, i);
    /* A piece which is geometrically coincident with a piece from the
     * other boundary is handled by the coincident-piece resolver */
    if (buffer_find_equal_piece(other_pieces, piece) != UINT32_MAX)
      continue;
    POINT2D midpoint;
    if (! buffer_piece_midpoint(
          piece, &midpoint))
      continue;
    int location = buffer_point_location(other, midpoint.x, midpoint.y);
    /* The piece is exterior to the other geometry.
     * Consequently it is exposed on the union boundary. */
    if (location != 0)
    {
      buffer_pieces_add_unique(result, piece);
    }
  }
}

/**
 * @brief Construct the union boundary pieces from two split buffer boundaries.
 * @details This function performs only piece classification.
 * Ring construction is left to the existing chaining code.
 */
static bool
buffer_select_split_union_pieces(const MeosArray *pieces1,
  const MeosArray *pieces2, const LWGEOM *geom1, const LWGEOM *geom2,
  MeosArray *result)
{
  assert(pieces1); assert(pieces2); assert(geom1); assert(geom2);
  assert(result);
  /* First resolve pieces which are coincident on both boundaries */
  if (! buffer_resolve_coincident_pieces(pieces1, pieces2, geom1, geom2,
      result))
    return false;
  /* Now select the exposed non-coincident pieces from geometry 1 */
  buffer_select_split_noncoincident_pieces(pieces1, geom2, pieces2, result);
  /* And from geometry 2 */
  buffer_select_split_noncoincident_pieces(pieces2, geom1, pieces1, result);
  return true;
}

/**
 * @brief Collect all exact boundary intersection nodes into the intersection
 * array.
 * @details The existing low-level intersection routines operate on MeosArray.
 * We therefore use a temporary MeosArray for each edge pair and transfer
 * the resulting points to MeosArray.
 */
static bool
buffer_collect_boundary_intersections(const LWGEOM *geom1, const LWGEOM *geom2,
  MeosArray *intersections)
{
  assert(geom1); assert(geom2); assert(intersections);
  MeosArray *a1 = geom_extract_edges(geom1);
  MeosArray *a2 = geom_extract_edges(geom2);
  if (! a1 || ! a2)
  {
    if (a1)
      meos_array_destroy(a1);
    if (a2)
      meos_array_destroy(a2);
    return false;
  }
  for (uint32_t i = 0; i < a1->count; i++)
  {
    const Edge *e1 = (const Edge *) meos_array_get(a1, i);
    if (! e1 || ! buffer_is_boundary_edge(e1))
      continue;
    for (uint32_t j = 0; j < a2->count; j++)
    {
      const Edge *e2 = (const Edge *) meos_array_get(a2, j);
      if (! e2 || ! buffer_is_boundary_edge(e2))
        continue;
      /* The existing intersection collectors expect MeosArray. */
      MeosArray *points = meos_array_create(sizeof(POINT2D));
      if (! points)
      {
        meos_array_destroy(a1); meos_array_destroy(a2);
        return false;
      }

      if (e1->etype == EDGE_POLYSEG && e2->etype == EDGE_POLYSEG)
        buffer_collect_line_line_intersections(e1, e2, points);
      else if (e1->etype == EDGE_POLYSEG && e2->etype == EDGE_POLYARC)
        buffer_collect_line_arc_intersections(e1, e2, points);
      else if (e1->etype == EDGE_POLYARC && e2->etype == EDGE_POLYSEG)
        buffer_collect_line_arc_intersections(e2, e1, points);
      else if (e1->etype == EDGE_POLYARC && e2->etype == EDGE_POLYARC)
        buffer_collect_arc_arc_intersections(e1, e2, points);

      /* Transfer the points to the intersection array */
      for (uint32_t k = 0; k < points->count; k++)
      {
        const POINT2D *point = (const POINT2D *) meos_array_get(points, k);
        if (point)
          buffer_intersections_add(intersections, point->x, point->y);
      }
      meos_array_destroy(points);
    }
  }
  meos_array_destroy(a1); meos_array_destroy(a2);
  return true;
}

/**
 * @brief Build the boundary pieces which belong to the union of
 * two buffer geometries.
 * @details Boundary intersections are first collected, then both boundaries
 * are split at the resulting nodes. The split pieces are subsequently
 * classified according to the union topology.
 */
static bool
buffer_build_union_pieces(const LWGEOM *geom1, const LWGEOM *geom2,
  MeosArray *result)
{
  assert(geom1); assert(geom2); assert(result);
  MeosArray *intersections = meos_array_create(sizeof(POINT2D));
  if (! buffer_collect_boundary_intersections(geom1, geom2, intersections))
  {
    meos_array_destroy(intersections);
    return false;
  }
  MeosArray *pieces1 = meos_array_create(sizeof(BufferPiece));
  MeosArray *pieces2 = meos_array_create(sizeof(BufferPiece));
  if (! buffer_build_split_boundary_pieces(geom1, geom2, intersections,
      pieces1, pieces2))
  {
    meos_array_destroy(pieces1); meos_array_destroy(pieces2);
    meos_array_destroy(intersections);
    return false;
  }
  bool success = buffer_select_split_union_pieces(pieces1, pieces2, geom1,
    geom2, result);
  meos_array_destroy(pieces1); meos_array_destroy(pieces2);
  meos_array_destroy(intersections);
  return success;
}

/*****************************************************************************
 * Buffer overlay - union relation dispatch
 *****************************************************************************/

/**
 * @brief Determine whether two buffer geometries can be handled without
 * boundary overlay.
 * @details The simple cases are:
 * - relation 0: disjoint
 * - relation 2: geom1 contained in geom2
 * - relation 3: geom2 contained in geom1
 * Only relation 1 requires boundary noding and union-piece construction.
 */
static bool
buffer_union_relation_simple(const LWGEOM *geom1, const LWGEOM *geom2,
  int *relation)
{
  assert(geom1); assert(geom2); assert(relation);
  *relation = buffer_components_relation(geom1, geom2);
  return *relation != 1;
}

/**
 * @brief Handle the union cases which do not require boundary overlay.
 * @details Return @p NULL when the two boundaries intersect and the
 * boundary overlay must be performed.
 */
static LWGEOM *
buffer_union_simple_result(const LWGEOM *geom1, const LWGEOM *geom2,
  int relation)
{
  assert(geom1); assert(geom2);
  switch (relation)
  {
    /* Disjoint geometries */
    case 0:
      return buffer_areal_collection(geom1, geom2);
    /* geom1 is contained in geom2 */
    case 2:
      return lwgeom_clone(geom2);
    /* geom2 is contained in geom1 */
    case 3:
      return lwgeom_clone(geom1);
    /* Boundary intersection requires the overlay path */
    case 1:
      return NULL;
    default:
      /* The current relation classifier does not otherwise return a
       * relationship requiring a special result */
      return NULL;
  }
}

/**
 * @brief Dispatch the areal union between two buffer geometries.
 * @details Simple disjoint/containment cases are returned immediately.
 * Intersecting boundaries are left for the boundary-piece overlay path.
 * The returned geometry is @p NULL when the caller must continue with the
 * boundary overlay.
 */
static LWGEOM *
buffer_union_dispatch(const LWGEOM *geom1, const LWGEOM *geom2, int *relation)
{
  assert(geom1); assert(geom2); assert(relation);
  if (buffer_union_relation_simple(geom1, geom2, relation))
    return buffer_union_simple_result(geom1, geom2, *relation);

  /* relation == 1: The boundaries intersect. 
   * The boundary overlay must handle this case. */
  return NULL;
}

/*****************************************************************************
 * Buffer overlay - union boundary selection
 *****************************************************************************/

/**
 * @brief Return true if a buffer piece is a usable exterior-boundary piece
 * for a union operation.
 * @details A piece belongs to the exterior boundary of A UNION B when its
 * interior side is not covered by the other geometry. The classification
 * performed by #buffer_classify_piece() is based on the midpoint of the piece:
 * - EXTERIOR -> retain
 * - INTERIOR -> discard
 * - BOUNDARY -> defer to coincident-boundary handling
 */
static bool
buffer_piece_is_union_exterior(const BufferPiece *piece, const LWGEOM *other)
{
  assert(piece); assert(other);
  return buffer_classify_piece(piece, other) == BUFFER_PIECE_EXTERIOR;
}

/**
 * @brief Select the pieces of one boundary which belong to the exterior
 * boundary of a union.
 */
static void
buffer_select_union_exterior(const MeosArray *pieces, const LWGEOM *other,
  MeosArray *result)
{
  assert(pieces); assert(other); assert(result);
  for (uint32_t i = 0; i < pieces->count; i++)
  {
    BufferPiece *piece = (BufferPiece *) meos_array_get(pieces, i);
    if (buffer_piece_is_union_exterior(piece, other))
      meos_array_add(result, piece);
  }
}

/**
 * @brief Select all non-interior pieces from two boundaries for a union.
 * @details This function does not resolve coincident boundary pieces.
 * Those pieces are returned in @p boundary and are handled by the next
 * overlay stage. The result is therefore:
 *   exterior(A relative to B) + exterior(B relative to A)
 * while pieces lying strictly inside the other geometry are discarded.
 */
static void
buffer_select_union_boundary(const MeosArray *pieces_a, const LWGEOM *geom_b,
  const MeosArray *pieces_b, const LWGEOM *geom_a, MeosArray *result,
  MeosArray *boundary)
{
  assert(pieces_a); assert(geom_b); assert(pieces_b); assert(geom_a);
  assert(result); assert(boundary);
  /* Pieces belonging to A */
  for (uint32_t i = 0; i < pieces_a->count; i++)
  {
    BufferPiece *piece = (BufferPiece *) meos_array_get(pieces_a, i);
    BufferPieceLocation location = buffer_classify_piece(piece, geom_b);
    if (location == BUFFER_PIECE_EXTERIOR)
      meos_array_add(result, piece);
    else if (location == BUFFER_PIECE_BOUNDARY)
    {
      if (! buffer_resolve_coincident_piece(piece, geom_a, geom_b, result))
        meos_array_add(boundary, piece);
    }
  }
  /* Pieces belonging to B */
  for (uint32_t i = 0; i < pieces_b->count; i++)
  {
    BufferPiece *piece = (BufferPiece *) meos_array_get(pieces_b, i);
    BufferPieceLocation location = buffer_classify_piece(piece, geom_a);
    if (location == BUFFER_PIECE_EXTERIOR)
      meos_array_add(result, piece);
    else if (location == BUFFER_PIECE_BOUNDARY)
    {
      if (! buffer_resolve_coincident_piece(piece, geom_b, geom_a, result))
        meos_array_add(boundary, piece);
    }
  }
}

/**
 * @brief Prepare the split boundary pieces of two geometries for union.
 * @details This function performs the complete operation:
 *   A boundary -> split at all intersection nodes
 *   B boundary -> split at all intersection nodes
 * then
 *  A pieces inside B -> discard
 *  B pieces inside A -> discard
 *  A pieces outside B -> retain
 *  B pieces outside A -> retain
 *  boundary pieces -> defer
 * The function is intentionally independent of the final ring/face
 * reconstruction.
 */
static bool
buffer_prepare_union_pieces(const LWGEOM *geom_a, const LWGEOM *geom_b,
  const MeosArray *intersections, MeosArray *result, MeosArray *boundary)
{
  assert(geom_a); assert(geom_b); assert(intersections); assert(result);
  assert(boundary);

  /* Extract the original boundary pieces */
  BufferPiece *pieces_a = NULL, *pieces_b = NULL;
  uint32_t count_a = 0, count_b = 0;
  if (! buffer_ring_pieces(geom_a, &pieces_a, &count_a))
    return false;
  if (! buffer_ring_pieces(geom_b, &pieces_b, &count_b))
  {
    pfree(pieces_a);
    return false;
  }

  /* Split A */
  MeosArray *split_a = meos_array_create(sizeof(BufferPiece));
  buffer_split_pieces(pieces_a, count_a, intersections, split_a);
  /* Split B */
  MeosArray *split_b = meos_array_create(sizeof(BufferPiece));
  buffer_split_pieces(pieces_b, count_b, intersections, split_b);
  /* Select the pieces which can contribute to the union boundary */
  buffer_select_union_boundary(split_a, geom_b, split_b, geom_a, result,
    boundary);

  meos_array_destroy(split_a); meos_array_destroy(split_b);
  pfree(pieces_a); pfree(pieces_b);
  return true;
}

/*****************************************************************************
 * Buffer overlay - boundary piece chaining
 *****************************************************************************/

/**
 * @brief Test whether two points represent the same topological node.
 */
static bool
buffer_points_equal(POINT2D p1, POINT2D p2)
{
  return fabs(p1.x - p2.x) <= FP_TOLERANCE &&
    fabs(p1.y - p2.y) <= FP_TOLERANCE;
}

/**
 * @brief Return the start point of a buffer piece.
 */
static POINT2D
buffer_piece_start(const BufferPiece *piece)
{
  assert(piece);
  POINT2D result;
  result.x = piece->x1;
  result.y = piece->y1;
  return result;
}

/**
 * @brief Return the end point of a buffer piece.
 */
static POINT2D
buffer_piece_end(const BufferPiece *piece)
{
  assert(piece);
  POINT2D result;
  result.x = piece->x2;
  result.y = piece->y2;
  return result;
}

/**
 * @brief Reverse the orientation of a buffer piece.
 * @details Reversing an arc also reverses its direction of traversal.
 * Therefore theta1/theta2 are exchanged and ccw is inverted.
 */
static void
buffer_piece_reverse(BufferPiece *piece)
{
  assert(piece);
  double tmp;
  tmp = piece->x1;
  piece->x1 = piece->x2;
  piece->x2 = tmp;
  tmp = piece->y1;
  piece->y1 = piece->y2;
  piece->y2 = tmp;
  if (piece->type == BUFFER_ARC)
  {
    tmp = piece->theta1;
    piece->theta1 = piece->theta2;
    piece->theta2 = tmp;
    piece->ccw = ! piece->ccw;
  }
}

/**
 * @brief Append one buffer piece to a compound curve.
 * @details The orientation of the piece is assumed to be the desired traversal
 * direction.
 */
static void
buffer_append_piece_to_curve(LWCOMPOUND *curve, int32_t srid,
  const BufferPiece *piece)
{
  assert(curve); assert(piece);
  POINT2D p1 = buffer_piece_start(piece);
  POINT2D p2 = buffer_piece_end(piece);
  if (piece->type == BUFFER_SEGMENT)
    buffer_add_segment(curve, srid, p1, p2);
  else if (piece->type == BUFFER_ARC)
    buffer_add_arc(curve, srid, piece->cx, piece->cy, piece->radius,
      piece->theta1, piece->theta2, piece->ccw);
}

/**
 * @brief Find an unused piece whose endpoint is connected to point.
 * @details If reverse is returned true, the piece must be reversed before it
 * is appended to the boundary.
 */
static int
buffer_find_connected_piece(const MeosArray *pieces, const bool *used,
  POINT2D point, bool *reverse)
{
  assert(pieces); assert(used); assert(reverse);
  for (uint32_t i = 0; i < pieces->count; i++)
  {
    if (used[i])
      continue;
    BufferPiece *piece = (BufferPiece *) meos_array_get(pieces, i);
    POINT2D start = buffer_piece_start(piece);
    POINT2D end = buffer_piece_end(piece);
    /* Prefer the natural orientation */
    if (buffer_points_equal(start, point))
    {
      *reverse = false;
      return (int) i;
    }
    /* Otherwise the piece can be traversed in reverse */
    if (buffer_points_equal(end, point))
    {
      *reverse = true;
      return (int) i;
    }
  }
  return -1;
}

/**
 * @brief Chain connected buffer pieces into one closed compound curve.
 * @details The pieces are assumed to form one connected boundary.
 * Each piece may be either a straight segment or an exact circular arc.
 * @return A closed compound curve, or @p NULL if the pieces cannot be chained
 * into a closed boundary.
 */
static LWCOMPOUND *
buffer_chain_pieces(const MeosArray *pieces, int32_t srid)
{
  assert(pieces);
  if (pieces->count == 0)
    return NULL;

  /* One piece is sufficient only if it is itself closed. Normally buffer
   * pieces have distinct endpoints, so this case will fail below unless
   * the input really represents a closed piece. */
  bool *used = palloc0(sizeof(bool) * pieces->count);
  LWCOMPOUND *curve = lwcompound_construct_empty(srid, 0, 0);

  /* Start with the first unused piece. Its orientation establishes the
   * orientation of the resulting ring. */
  uint32_t first_index = 0;
  BufferPiece *first = (BufferPiece *) meos_array_get(pieces, first_index);
  POINT2D first_start = buffer_piece_start(first);
  POINT2D current = buffer_piece_end(first);
  buffer_append_piece_to_curve(curve, srid, first);
  used[first_index] = true;
  uint32_t used_count = 1;

  /* Add pieces until the current endpoint returns to the initial point */
  while (used_count < pieces->count)
  {
    /* We have already closed the ring before consuming all pieces.
     * This means that the selected pieces contain another disconnected
     * component. That will be handled by the multi-ring layer later. */
    if (buffer_points_equal(current, first_start))
      break;
    bool reverse = false;
    int index = buffer_find_connected_piece(pieces, used, current, &reverse);
    if (index < 0)
    {
      /* The selected boundary is not currently a closed chain.
       * Do not silently construct an invalid CURVEPOLYGON. */
      lwgeom_free(lwcompound_as_lwgeom(curve));
      pfree(used);
      return NULL;
    }

    BufferPiece *piece = (BufferPiece *) meos_array_get(pieces, index);
    if (reverse)
      buffer_piece_reverse(piece);
    buffer_append_piece_to_curve(curve, srid, piece);
    current = buffer_piece_end(piece);
    used[index] = true;
    used_count++;
  }

  /* Verify that the chain actually closes */
  if (! buffer_points_equal(current, first_start))
  {
    lwgeom_free(lwcompound_as_lwgeom(curve));
    pfree(used);
    return NULL;
  }

  /* There may be unused pieces if the input contains multiple connected
   * components. Do not silently merge them into the first ring. */
  if (used_count != pieces->count)
  {
    lwgeom_free(lwcompound_as_lwgeom(curve));
    pfree(used);
    return NULL;
  }
  pfree(used);
  return curve;
}

/**
 * @brief Find an unused boundary piece connected to a point.
 * @details The piece is returned in its natural orientation when its start
 * point matches the current point. If its end point matches, the caller must
 * reverse the piece before appending it.
 * @param[in] pieces Boundary pieces
 * @param[in] used Flags indicating which pieces have already been consumed
 * @param[in] point Current endpoint of the ring being constructed
 * @param[out] reverse True when the returned piece must be reversed
 * @return Index of the connected piece, or -1 if none exists
 */
static int
buffer_find_unused_connected_piece(const MeosArray *pieces, const bool *used,
  POINT2D point, bool *reverse)
{
  assert(pieces); assert(used); assert(reverse);
  for (uint32_t i = 0; i < pieces->count; i++)
  {
    if (used[i])
      continue;
    const BufferPiece *piece = (const BufferPiece *) meos_array_get(pieces, i);
    POINT2D start = buffer_piece_start(piece);
    POINT2D end = buffer_piece_end(piece);
    if (buffer_points_equal(start, point))
    {
      *reverse = false;
      return (int) i;
    }
    if (buffer_points_equal(end, point))
    {
      *reverse = true;
      return (int) i;
    }
  }
  return -1;
}

/**
 * @brief Chain one connected boundary component and retain its ordered pieces.
 * @details The returned piece array contains copies of the pieces in exactly
 * the traversal order used to construct the compound curve.
 * The input array is never modified.
 */
static LWCOMPOUND *
buffer_chain_ring_with_pieces(const MeosArray *pieces, bool *used,
  uint32_t start_index, int32_t srid, MeosArray *ordered)
{
  assert(pieces); assert(used); assert(ordered);
  assert(start_index < pieces->count);
  const BufferPiece *first = (const BufferPiece *) meos_array_get(pieces,
    start_index);
  if (! first)
    return NULL;
  LWCOMPOUND *curve = lwcompound_construct_empty(srid, 0, 0);
  if (! curve)
    return NULL;
  BufferPiece oriented = *first;
  buffer_append_piece_to_curve(curve, srid, &oriented);
  meos_array_add(ordered, &oriented);
  used[start_index] = true;
  POINT2D start = buffer_piece_start(&oriented);
  POINT2D current = buffer_piece_end(&oriented);
  while (! buffer_points_equal(current, start))
  {
    bool reverse = false;
    int index = buffer_find_connected_piece(pieces, used, current, &reverse);
    if (index < 0)
    {
      lwgeom_free(lwcompound_as_lwgeom(curve));
      return NULL;
    }
    const BufferPiece *piece = (const BufferPiece *) meos_array_get(pieces,
      (uint32_t) index);
    if (! piece)
    {
      lwgeom_free(lwcompound_as_lwgeom(curve));
      return NULL;
    }
    oriented = *piece;
    /* Reverse only the local copy. The input pieces must remain unchanged. */
    if (reverse)
      buffer_piece_reverse(&oriented);
    buffer_append_piece_to_curve(curve, srid, &oriented);
    meos_array_add(ordered, &oriented);
    used[(uint32_t) index] = true;
    current = buffer_piece_end(&oriented);
  }
  return curve;
}

/**
 * @brief Chain one connected boundary component into a closed ring.
 */
static LWCOMPOUND *
buffer_chain_ring(const MeosArray *pieces, bool *used, uint32_t start_index,
  int32_t srid)
{
  assert(pieces); assert(used);
  MeosArray *ordered = meos_array_create(sizeof(BufferPiece));
  if (! ordered)
    return NULL;
  LWCOMPOUND *ring = buffer_chain_ring_with_pieces(pieces, used, start_index,
    srid, ordered);
  meos_array_destroy(ordered);
  return ring;
}

/**
 * @brief Chain all selected boundary pieces into closed rings.
 * @details The resulting BufferRingInfo objects retain both the geometric
 * ring and the ordered boundary pieces used to construct it.
 */
static bool
buffer_chain_ring_infos(const MeosArray *pieces, int32_t srid,
  MeosArray *rings)
{
  assert(pieces); assert(rings);
  if (pieces->count == 0)
    return true;
  bool *used = palloc0(sizeof(bool) * pieces->count);
  uint32_t used_count = 0;
  while (used_count < pieces->count)
  {
    uint32_t start_index = UINT32_MAX;
    for (uint32_t i = 0; i < pieces->count; i++)
    {
      if (! used[i])
      {
        start_index = i;
        break;
      }
    }
    if (start_index == UINT32_MAX)
      break;
    MeosArray *ordered = meos_array_create(sizeof(BufferPiece));
    if (! ordered)
    {
      pfree(used);
      return false;
    }
    LWCOMPOUND *ring = buffer_chain_ring_with_pieces(pieces, used, start_index,
      srid, ordered);
    if (! ring)
    {
      meos_array_destroy(ordered); pfree(used);
      return false;
    }
    /* Count the pieces consumed by this ring */
    uint32_t new_used_count = 0;
    for (uint32_t i = 0; i < pieces->count; i++)
    {
      if (used[i])
        new_used_count++;
    }
    if (new_used_count == used_count)
    {
      lwgeom_free(lwcompound_as_lwgeom(ring));
      meos_array_destroy(ordered); pfree(used);
      return false;
    }
    used_count = new_used_count;
    BufferRingInfo info;
    memset(&info, 0, sizeof(BufferRingInfo));
    info.ring = ring;
    info.pieces = ordered;
    info.parent = -1;
    info.depth = 0;
    info.shell = -1;
    meos_array_add(rings, &info);
  }
  pfree(used);
  return used_count == pieces->count;
}

/**
 * @brief Construct all closed boundary rings from selected pieces.
 * @details The input pieces may form several disconnected components.
 * Each connected component is independently chained into one closed
 * compound curve.
 * @param[in] pieces Selected boundary pieces
 * @param[in] srid Spatial reference identifier
 * @param[out] rings Array of LWCOMPOUND pointers
 * @return True if every piece belongs to a closed ring
 */
static bool
buffer_chain_rings(const MeosArray *pieces, int32_t srid, MeosArray *rings)
{
  assert(pieces); assert(rings);
  if (pieces->count == 0)
    return true;
  bool *used = palloc0(sizeof(bool) * pieces->count);
  uint32_t used_count = 0;
  while (used_count < pieces->count)
  {
    uint32_t start_index = UINT32_MAX;
    /* Find the first unused piece and use it to start a new connected
     * boundary component. */
    for (uint32_t i = 0; i < pieces->count; i++)
    {
      if (! used[i])
      {
        start_index = i;
        break;
      }
    }
    if (start_index == UINT32_MAX)
      break;
    uint32_t before = used_count;
    LWCOMPOUND *ring = buffer_chain_ring(pieces, used, start_index, srid);
    if (! ring)
    {
      pfree(used);
      return false;
    }
    /* Count how many pieces were consumed by this ring */
    for (uint32_t i = 0; i < pieces->count; i++)
    {
      if (used[i])
        used_count++;
    }
    /* A successful chain must consume at least one previously unused
     * piece. This protects against an unexpected topology loop. */
    if (used_count == before)
    {
      lwgeom_free(lwcompound_as_lwgeom(ring));
      pfree(used);
      return false;
    }
    meos_array_add(rings, &ring);
  }
  pfree(used);
  return used_count == pieces->count;
}

/**
 * @brief Construct a temporary CURVEPOLYGON containing one ring.
 * @details The returned geometry owns the supplied ring.
 */
static LWGEOM *
buffer_make_single_ring_polygon(LWCOMPOUND *ring, int32_t srid)
{
  assert(ring);
  LWCURVEPOLY *polygon = lwcurvepoly_construct_empty(srid, 0, 0);
  if (! polygon)
    return NULL;
  lwcurvepoly_add_ring(polygon, lwcompound_as_lwgeom(ring));
  return lwcurvepoly_as_lwgeom(polygon);
}

/**
 * @brief Find a point strictly inside a closed boundary ring.
 * @details A candidate point is generated close to the midpoint of a
 * boundary edge and displaced toward the interior. The candidate is
 * verified with the existing strict point-in-areal test.
 *
 * The displacement is progressively reduced if the initial candidate
 * is not suitable. This is important for narrow buffer regions where a
 * fixed displacement could cross the opposite boundary.
 */
static bool
buffer_ring_find_interior_point(const LWCOMPOUND *ring, int32_t srid,
  double *x, double *y)
{
  assert(ring); assert(x); assert(y);
  MeosArray *arr = geom_extract_edges(lwcompound_as_lwgeom(ring));
  if (! arr || arr->count == 0)
  {
    if (arr)
      meos_array_destroy(arr);
    return false;
  }
  for (uint32_t i = 0; i < arr->count; i++)
  {
    Edge *edge = (Edge *) meos_array_get(arr, i);
    if (! edge)
      continue;

    double mx, my;
    double nx, ny;
    /* Straight edges */
    if (edge->etype == EDGE_POLYARC)
    {
      mx = (edge->x1 + edge->x2) * 0.5;
      my = (edge->y1 + edge->y2) * 0.5;
      double length = hypot(edge->x2 - edge->x1, edge->y2 - edge->y1);
      if (length == 0.0)
        continue;
      /* Unit normal to the left side of the directed edge */
      nx = -(edge->y2 - edge->y1) / length;
      ny =  (edge->x2 - edge->x1) / length;
    }
    /* Circular arcs */
    else if (edge->etype == EDGE_POLYARC || edge->etype == EDGE_ARC)
    {
      double sweep = edge->ccw ?
        angle_normalize(edge->theta1 - edge->theta0) :
        angle_normalize(edge->theta0 - edge->theta1);
      if (sweep == 0.0 || edge->radius <= 0.0)
        continue;
      double theta = edge->ccw ?
        edge->theta0 + sweep * 0.5 : edge->theta0 - sweep * 0.5;
      mx = edge->cx + edge->radius * cos(theta);
      my = edge->cy + edge->radius * sin(theta);

      /* The radial direction points away from the circle centre.
       * For a CCW arc, the left-hand side is toward the centre;
       * for a clockwise arc it is away from the centre. */
      double rx = mx - edge->cx;
      double ry = my - edge->cy;
      double radius = hypot(rx, ry);
      if (radius == 0.0)
        continue;
      if (edge->ccw)
      {
        nx = -rx / radius;
        ny = -ry / radius;
      }
      else
      {
        nx = rx / radius;
        ny = ry / radius;
      }
    }
    else
      continue;

    /* Estimate a suitable displacement from the edge length/radius.
     * The progressively smaller offsets make the test robust near
     * narrow regions. */
    double scale;
    if (edge->etype == EDGE_POLYARC)
      scale = hypot(edge->x2 - edge->x1, edge->y2 - edge->y1);
    else
      scale = edge->radius;
    if (scale <= 0.0)
      continue;
    double offsets[] = {
      scale * 1.0e-3,
      scale * 1.0e-4,
      scale * 1.0e-5,
      scale * 1.0e-6,
      scale * 1.0e-7
    };
    for (size_t k = 0; k < sizeof(offsets) / sizeof(offsets[0]); k++)
    {
      double cx = mx + nx * offsets[k];
      double cy = my + ny * offsets[k];
      /* Construct the temporary areal geometry only for the
       * containment test */
      LWCOMPOUND *copy = (LWCOMPOUND *) lwgeom_clone(
          lwcompound_as_lwgeom(ring));
      LWGEOM *polygon = buffer_make_single_ring_polygon(copy, srid);
      if (! polygon)
      {
        meos_array_destroy(arr);
        return false;
      }
      bool inside = buffer_areal_contains_point(polygon, cx, cy);
      lwgeom_free(polygon);
      if (inside)
      {
        *x = cx;
        *y = cy;
        meos_array_destroy(arr);
        return true;
      }
    }
  }
  meos_array_destroy(arr);
  return false;
}

/**
 * @brief Compute a representative point strictly inside a boundary ring.
 * @details The point is obtained from a boundary edge and verified using
 * strict interior containment.
 */
static bool
buffer_ring_representative_point(LWCOMPOUND *ring, int32_t srid,
  double *x, double *y)
{
  assert(ring); assert(x); assert(y);
  return buffer_ring_find_interior_point(ring, srid, x, y);
}

/**
 * @brief Test whether one boundary ring contains another ring.
 * @details The representative point of the inner ring is tested against
 * the areal region bounded by the outer ring. The boundary is excluded
 * from the interior test.
 */
static bool
buffer_ring_contains_ring(const BufferRingInfo *outer,
  const BufferRingInfo *inner, int32_t srid)
{
  assert(outer); assert(inner);
  LWCOMPOUND *ring_copy = (LWCOMPOUND *) lwgeom_clone(
    lwcompound_as_lwgeom(outer->ring));
  if (! ring_copy)
    return false;
  LWGEOM *polygon = buffer_make_single_ring_polygon(ring_copy, srid);
  if (! polygon)
    return false;
  bool result = buffer_areal_contains_point(polygon, inner->x, inner->y);
  lwgeom_free(polygon);
  return result;
}

/**
 * @brief Build the containment hierarchy of closed boundary rings.
 * @details For every ring, the immediate containing ring is identified
 * using the containment relation between rings. No ring orientation or
 * area calculation is required.
 * If A contains B and B contains C, then B is the parent of C even
 * though A also contains C. The depth is the number of ancestors:
 *   depth 0 -> shell
 *   depth 1 -> hole
 *   depth 2 -> shell
 *   depth 3 -> hole
 *   ...
 */
static bool
buffer_classify_rings(const MeosArray *rings, int32_t srid,
  MeosArray *classified)
{
  assert(rings); assert(classified);
  uint32_t count = rings->count;
  if (count == 0)
    return true;
  BufferRingInfo *info = palloc0(sizeof(BufferRingInfo) * count);

  /* Initialize the ring information and compute one representative
   * point strictly inside every ring */
  for (uint32_t i = 0; i < count; i++)
  {
    BufferRingInfo *ring_info = (BufferRingInfo *) meos_array_get(rings, i);
    if (! ring_info || ! ring_info->ring || ! ring_info->pieces)
    {
      pfree(info);
      return false;
    }
    info[i] = *ring_info;
    info[i].parent = -1;
    info[i].depth = 0;
    info[i].shell = -1;
    if (! buffer_ring_representative_point(info[i].ring, srid,
        &info[i].x, &info[i].y))
    {
      pfree(info);
      return false;
    }
  }

  /* First determine all pairwise containment relationships.
   * contains[i * count + j] means that ring i strictly contains
   * the representative point of ring j. */
  bool *contains = palloc0(sizeof(bool) * count * count);
  for (uint32_t i = 0; i < count; i++)
  {
    for (uint32_t j = 0; j < count; j++)
    {
      if (i == j)
        continue;
      contains[i * count + j] = buffer_ring_contains_ring(&info[i], &info[j],
        srid);
    }
  }

  /* Determine the immediate parent.
   * Candidate j contains ring i. It is the immediate parent if there
   * is no third ring k such that:
   *   j contains k
   *   k contains i
   * In other words, no containing ring may lie strictly between j and i
   * in the containment hierarchy. */
  for (uint32_t i = 0; i < count; i++)
  {
    int32_t parent = -1;
    for (uint32_t j = 0; j < count; j++)
    {
      if (i == j)
        continue;
      if (! contains[j * count + i])
        continue;
      bool is_immediate = true;
      for (uint32_t k = 0; k < count; k++)
      {
        if (k == i || k == j)
          continue;

        /* k must be between j and i:
         *     j contains k
         *     k contains i */
        if (contains[j * count + k] && contains[k * count + i])
        {
          is_immediate = false;
          break;
        }
      }
      if (is_immediate)
      {
        /* There must be at most one immediate parent in a valid
         * non-intersecting ring hierarchy */
        if (parent >= 0)
        {
          pfree(contains); pfree(info);
          return false;
        }
        parent = (int32_t) j;
      }
    }
    info[i].parent = parent;
  }

  /* Derive the depth from the parent hierarchy */
  for (uint32_t i = 0; i < count; i++)
  {
    uint32_t depth = 0;
    int32_t current = info[i].parent;
    uint32_t steps = 0;
    while (current >= 0)
    {
      if ((uint32_t) current >= count)
      {
        pfree(contains); pfree(info);
        return false;
      }
      depth++;
      current = info[current].parent;
      /* A valid containment hierarchy is acyclic */
      if (++steps > count)
      {
        pfree(contains); pfree(info);
        return false;
      }
    }
    info[i].depth = depth;
  }

  /* Assign every hole to its immediate containing shell.
   * Because depth alternates between shells and holes, the parent
   * of every odd-depth ring must be an even-depth ring. */
  for (uint32_t i = 0; i < count; i++)
  {
    if ((info[i].depth & 1) == 0)
      continue;
    int32_t parent = info[i].parent;
    if (parent < 0 || (uint32_t) parent >= count)
    {
      pfree(contains); pfree(info);
      return false;
    }
    if ((info[parent].depth & 1) != 0)
    {
      pfree(contains); pfree(info);
      return false;
    }
    info[i].shell = parent;
  }
  /* Transfer the classification information into the generic array */
  for (uint32_t i = 0; i < count; i++)
    meos_array_add(classified, &info[i]);
  pfree(contains); pfree(info);
  return true;
}

/**
 * @brief Compute the signed area contribution of a straight buffer piece.
 * @details The contribution is one half of the line integral
 *   x dy - y dx
 * along the segment.
 */
static double
buffer_segment_signed_area(const BufferPiece *piece)
{
  assert(piece);
  assert(piece->type == BUFFER_SEGMENT);
  return 0.5 * (piece->x1 * piece->y2 - piece->x2 * piece->y1);
}

/**
 * @brief Compute the signed area contribution of a circular buffer arc.
 * @details The arc contribution is obtained from the line integral
 *   1/2 * integral(x dy - y dx)
 * along the directed circular arc. The sign of the angular sweep follows
 * the traversal direction: positive for CCW and negative for CW.
 */
static double
buffer_arc_signed_area(const BufferPiece *piece)
{
  assert(piece); assert(piece->type == BUFFER_ARC);
  double theta1 = piece->theta1;
  double theta2 = piece->theta2;
  /* Directed angular sweep.
   * Unlike #buffer_arc_sweep(), the sign is retained because it
   * determines the orientation of the complete ring. */
  double delta = piece->ccw ?
    angle_normalize(theta2 - theta1) : -angle_normalize(theta1 - theta2);
  /* Integral of x dy - y dx for
   *   x = cx + r cos(theta)
   *   y = cy + r sin(theta)
   * is
   *   r*cx*sin(theta)
   * - r*cy*cos(theta)
   * + r^2*theta */
  double contribution = piece->radius * piece->cx * 
      (sin(theta2) - sin(theta1)) +
    piece->radius * piece->cy * (cos(theta1) - cos(theta2)) +
    piece->radius * piece->radius * delta;
  return 0.5 * contribution;
}

/**
 * @brief Compute the signed area of an ordered buffer ring.
 * @details The ring may contain both straight segments and exact circular
 * arcs. A positive value means counter-clockwise traversal and a negative
 * value means clockwise traversal.
 */
static double
buffer_ring_signed_area(const MeosArray *pieces)
{
  assert(pieces);
  double area = 0.0;
  for (uint32_t i = 0; i < pieces->count; i++)
  {
    const BufferPiece *piece = (const BufferPiece *) meos_array_get(pieces, i);
    if (! piece)
      continue;
    if (piece->type == BUFFER_SEGMENT)
      area += buffer_segment_signed_area(piece);
    else if (piece->type == BUFFER_ARC)
      area += buffer_arc_signed_area(piece);
  }
  return area;
}

/**
 * @brief Reverse the traversal direction of a complete boundary ring.
 * @details The piece order is reversed and every individual piece is
 * reversed. The resulting sequence represents exactly the same geometric
 * ring with the opposite orientation.
 */
static MeosArray *
buffer_reverse_ring_pieces(const MeosArray *pieces)
{
  assert(pieces);
  MeosArray *reversed = meos_array_create(sizeof(BufferPiece));
  if (! reversed)
    return NULL;
  for (uint32_t i = pieces->count; i > 0; i--)
  {
    const BufferPiece *source = 
      (const BufferPiece *) meos_array_get(pieces, i - 1);
    if (! source)
    {
      meos_array_destroy(reversed);
      return NULL;
    }
    BufferPiece piece = *source;
    buffer_piece_reverse(&piece);
    meos_array_add(reversed, &piece);
  }
  return reversed;
}

/**
 * @brief Construct a compound curve from an ordered buffer-piece sequence.
 */
static LWCOMPOUND *
buffer_build_ring_from_pieces(const MeosArray *pieces, int32_t srid)
{
  assert(pieces);
  if (pieces->count == 0)
    return NULL;
  LWCOMPOUND *ring = lwcompound_construct_empty(srid, 0, 0);
  if (! ring)
    return NULL;
  for (uint32_t i = 0; i < pieces->count; i++)
  {
    const BufferPiece *piece = (const BufferPiece *) meos_array_get(pieces, i);
    if (! piece)
    {
      lwgeom_free(lwcompound_as_lwgeom(ring));
      return NULL;
    }
    buffer_append_piece_to_curve(ring, srid, piece);
  }
  return ring;
}

/**
 * @brief Normalize the orientation of one classified boundary ring.
 * @details Shells are normalized to counter-clockwise traversal and holes
 * to clockwise traversal.
 * If the current orientation already matches the desired orientation,
 * the existing ring and piece sequence are retained.
 */
static bool
buffer_normalize_ring_orientation(BufferRingInfo *info, int32_t srid)
{
  assert(info); assert(info->ring); assert(info->pieces);
  double area =
    buffer_ring_signed_area(info->pieces);
  if (fabs(area) <= FP_TOLERANCE)
    return false;
  /* Even depth = shell = CCW
   * Odd depth  = hole  = CW */
  bool want_ccw = (info->depth & 1) == 0;
  bool is_ccw = area > 0.0;
  if (is_ccw == want_ccw)
    return true;
  /* Reverse both the ordered pieces and the compound curve */
  MeosArray *reversed = buffer_reverse_ring_pieces(info->pieces);
  if (! reversed)
    return false;
  LWCOMPOUND *ring = buffer_build_ring_from_pieces(reversed, srid);
  if (! ring)
  {
    meos_array_destroy(reversed);
    return false;
  }
  lwgeom_free(lwcompound_as_lwgeom(info->ring));
  meos_array_destroy(info->pieces);
  info->ring = ring;
  info->pieces = reversed;
  /* Reversing a closed ring does not change its interior or its
   * containment relationship, so x/y, parent, depth and shell remain
   * unchanged. */
  return true;
}

/**
 * @brief Normalize the orientation of all classified boundary rings.
 */
static bool
buffer_normalize_ring_orientations(MeosArray *classified, int32_t srid)
{
  assert(classified);
  for (uint32_t i = 0; i < classified->count; i++)
  {
    BufferRingInfo *info = (BufferRingInfo *) meos_array_get(classified, i);
    if (! info)
      return false; 
    if (! buffer_normalize_ring_orientation(info, srid))
      return false;
  }
  return true;
}

/**
 * @brief Construct polygonal surfaces from classified boundary rings.
 * @details The topology stage has already classified every ring as either
 * a shell or a hole and assigned every hole to its immediate shell.
 * This function therefore performs no further geometric reasoning.
 * It simply constructs one CURVEPOLYGON for every shell and attaches the
 * holes belonging directly to that shell.
 * Shells and holes are already oriented by the preceding topology stage.
 */
static LWGEOM *
buffer_build_surfaces_from_classified_rings(const MeosArray *classified,
  int32_t srid)
{
  assert(classified);
  uint32_t nshells = 0;
  /* Count the shells */
  for (uint32_t i = 0; i < classified->count; i++)
  {
    const BufferRingInfo *info =
      (const BufferRingInfo *) meos_array_get(classified, i);
    if (! info)
      return NULL;
    if ((info->depth & 1) == 0)
      nshells++;
  }
  /* No shells means that there is no areal result */
  if (nshells == 0)
    return NULL;
  LWGEOM **surfaces = palloc0(sizeof(LWGEOM *) * nshells);
  uint32_t surface_count = 0;
  /* Construct one CURVEPOLYGON for every shell */
  for (uint32_t i = 0; i < classified->count; i++)
  {
    const BufferRingInfo *shell =
      (const BufferRingInfo *) meos_array_get(classified, i);
    if (! shell)
      goto fail;
    /* Odd depth means hole */
    if ((shell->depth & 1) != 0)
      continue;
    LWCURVEPOLY *polygon = lwcurvepoly_construct_empty(srid, 0, 0);
    if (! polygon)
      goto fail;
    /* Add the shell.
     * The output geometry receives its own copy because the classified
     * ring remains owned by BufferRingInfo until the classification
     * array is destroyed. */
    LWCOMPOUND *shell_ring = (LWCOMPOUND *) lwgeom_clone(
        lwcompound_as_lwgeom(shell->ring));
    if (! shell_ring)
    {
      lwgeom_free(lwcurvepoly_as_lwgeom(polygon));
      goto fail;
    }
    lwcurvepoly_add_ring(polygon, lwcompound_as_lwgeom(shell_ring));
    /* Attach only the holes whose immediate shell is this shell */
    for (uint32_t j = 0; j < classified->count; j++)
    {
      const BufferRingInfo *hole =
        (const BufferRingInfo *) meos_array_get(classified, j);
      if (! hole)
      {
        lwgeom_free(lwcurvepoly_as_lwgeom(polygon));
        goto fail;
      }
      if ((hole->depth & 1) == 0)
        continue;
      if (hole->shell != (int32_t) i)
        continue;
      LWCOMPOUND *hole_ring = (LWCOMPOUND *) lwgeom_clone(
          lwcompound_as_lwgeom(hole->ring));
      if (! hole_ring)
      {
        lwgeom_free(lwcurvepoly_as_lwgeom(polygon));
        goto fail;
      }
      lwcurvepoly_add_ring(polygon, lwcompound_as_lwgeom(hole_ring));
    }
    surfaces[surface_count++] = lwcurvepoly_as_lwgeom(polygon);
  }

  /* Exactly one shell gives a single CURVEPOLYGON */
  if (surface_count == 1)
  {
    LWGEOM *result = surfaces[0];
    pfree(surfaces);
    return result;
  }
  /* Several independent shells give a MULTISURFACE */
  LWCOLLECTION *collection = lwcollection_construct(MULTISURFACETYPE, srid,
    NULL, surface_count, surfaces);
  if (! collection)
    goto fail;
  pfree(surfaces);
  return lwcollection_as_lwgeom(collection);

fail:
  for (uint32_t i = 0; i < surface_count; i++)
    lwgeom_free(surfaces[i]);
  pfree(surfaces);
  return NULL;
}

/**
 * @brief Construct polygonal surfaces from closed boundary rings.
 * @details Rings are classified according to their containment depth.
 * Even-depth rings are shells and odd-depth rings are holes.
 * Each hole is assigned to its immediate containing shell.
 */
static LWGEOM *
buffer_build_surfaces_from_rings(const MeosArray *rings, int32_t srid)
{
  assert(rings);
  if (rings->count == 0)
  {
    LWCURVEPOLY *empty = lwcurvepoly_construct_empty(srid, 0, 0);
    return lwcurvepoly_as_lwgeom(empty);
  }
  /* The classification array contains one BufferRingInfo for each
   * closed boundary ring */
  MeosArray *classified = meos_array_create(sizeof(BufferRingInfo));
  if (! classified)
    return NULL;
  if (! buffer_classify_rings(rings, srid, classified))
  {
    meos_array_destroy(classified);
    return NULL;
  }
  /* Normalize shell/hole orientation only after the containment
   * hierarchy has been established */
  if (! buffer_normalize_ring_orientations(classified, srid))
  {
    meos_array_destroy(classified);
    return NULL;
  }
  LWGEOM *result = buffer_build_surfaces_from_classified_rings(classified,
    srid);
  meos_array_destroy(classified);
  return result;
}

/**
 * @brief Construct a CURVEPOLYGON from selected boundary pieces.
 * @details The selected pieces may form several disconnected boundary
 * components. Each component is first chained into a closed ring.
 * Shell/hole classification is handled by a later topology stage.
 */
static LWGEOM *
buffer_make_curvepolygon_from_pieces(const MeosArray *pieces, int32_t srid)
{
  assert(pieces);
  MeosArray *rings = meos_array_create(sizeof(BufferRingInfo));
  if (! rings)
    return NULL;
  if (! buffer_chain_ring_infos(pieces, srid, rings))
  {
    meos_array_destroy(rings);
    return NULL;
  }
  LWGEOM *result = buffer_build_surfaces_from_rings(rings, srid);
  /* The CURVEPOLYGON now owns the ring geometries. 
   * Only the array of pointers has to be destroyed here. */
  meos_array_destroy(rings);
  return result;
}

/*****************************************************************************
 * Buffer overlay - topology classification
 *****************************************************************************/

/**
 * @brief Determine whether two buffer boundaries have a proper crossing.
 * @details A proper crossing means that at least one boundary intersection
 * is an isolated point and the two surfaces overlap in their interiors.
 * A point-touch without interior overlap is NOT a crossing.
 */
static bool
buffer_boundaries_cross(const LWGEOM *geom1, const LWGEOM *geom2)
{
  assert(geom1); assert(geom2);
  MeosArray *a1 = geom_extract_edges(geom1);
  MeosArray *a2 = geom_extract_edges(geom2);
  if (! a1 || ! a2)
  {
    if (a1)
      meos_array_destroy(a1);
    if (a2)
      meos_array_destroy(a2);
    return false;
  }

  bool point_intersection = false;
  for (uint32_t i = 0; i < a1->count && ! point_intersection; i++)
  {
    const Edge *e1 = (const Edge *) meos_array_get(a1, i);
    if (! e1 || ! buffer_is_boundary_edge(e1))
      continue;
    for (uint32_t j = 0; j < a2->count; j++)
    {
      const Edge *e2 = (const Edge *) meos_array_get(a2, j);
      if (! e2 || ! buffer_is_boundary_edge(e2))
        continue;
      int dimension = buffer_boundary_intersection(e1, e2);

      /* Dimension 1 means that the boundaries overlap over a curve.
       * That is not a proper crossing and must be handled separately. */
      if (dimension == 1)
      {
        meos_array_destroy(a1); meos_array_destroy(a2);
        return false;
      }
      if (dimension == 0)
        point_intersection = true;
    }
  }

  meos_array_destroy(a1); meos_array_destroy(a2);
  return point_intersection;
}

/*****************************************************************************
 * Buffer overlay - first complete two-buffer union
 *****************************************************************************/

/**
 * @brief Union two crossing buffer surfaces while preserving circular arcs.
 * @details This is the first end-to-end overlay path.
 * It is intentionally restricted to the crossing case:
 * - boundaries intersect at one or more discrete nodes;
 * - neither geometry contains the other;
 * - coincident boundary portions are deferred;
 * - the selected exterior pieces form one connected boundary.
 */
static LWGEOM *
buffer_union_crossing(const LWGEOM *geom1, const LWGEOM *geom2)
{
  assert(geom1); assert(geom2);
  int relation = buffer_components_relation(geom1, geom2);
  /* This routine is only for the proper crossing/partial-overlap case */
  if (relation != 1)
    return NULL;

  /* A point intersection by itself does not imply an overlapping union
   * boundary. In particular, two buffers may merely touch at one point.
   * Such components should remain separate surfaces. */
  if (! buffer_boundaries_cross(geom1, geom2))
    return NULL;

  /* Collect the exact intersection nodes */
  MeosArray *intersections = meos_array_create(sizeof(POINT2D));
  if (! buffer_collect_boundary_intersections(geom1, geom2, intersections))
  {
    meos_array_destroy(intersections);
    return NULL;
  }

  /* A boundary intersection was reported, but there are no discrete
   * nodes. This indicates a coincident/overlapping-boundary case.
   * Defer it to the next topology layer. */
  if (meos_array_count(intersections) == 0)
  {
    meos_array_destroy(intersections);
    return NULL;
  }

  /* Extract and split both complete boundaries */
  BufferPiece *raw_a = NULL, *raw_b = NULL;
  uint32_t raw_count_a = 0, raw_count_b = 0;
  if (! buffer_ring_pieces(geom1, &raw_a, &raw_count_a))
  {
    meos_array_destroy(intersections);
    return NULL;
  }
  if (! buffer_ring_pieces(geom2, &raw_b, &raw_count_b))
  {
    pfree(raw_a);
    meos_array_destroy(intersections);
    return NULL;
  }

  MeosArray *split_a = meos_array_create(sizeof(BufferPiece));
  MeosArray *split_b = meos_array_create(sizeof(BufferPiece));
  buffer_split_pieces(raw_a, raw_count_a, intersections, split_a);
  buffer_split_pieces(raw_b, raw_count_b, intersections, split_b);

  /* Select the exterior portions of both boundaries */
  MeosArray *selected = meos_array_create(sizeof(BufferPiece));
  MeosArray *boundary = meos_array_create(sizeof(BufferPiece));
  buffer_select_union_boundary(split_a, geom2, split_b, geom1, selected, boundary);
  /* We reject unresolved coincident pieces here */
  if (meos_array_count(boundary) > 0)
  {
    meos_array_destroy(selected); meos_array_destroy(boundary);
    meos_array_destroy(split_a); meos_array_destroy(split_b);
    pfree(raw_a); pfree(raw_b); meos_array_destroy(intersections);
    return NULL;
  }

  /* Chain the selected exterior pieces into one closed curve */
  int32_t srid = lwgeom_get_srid(geom1);
  LWGEOM *result = buffer_make_curvepolygon_from_pieces(selected, srid);
  /* Clean up and return */
  meos_array_destroy(selected); meos_array_destroy(boundary);
  meos_array_destroy(split_a); meos_array_destroy(split_b);
  pfree(raw_a); pfree(raw_b); meos_array_destroy(intersections);
  return result;
}

/*****************************************************************************
 * Polygon buffer
 *****************************************************************************/

/**
 * @brief Return the signed area of a ring.
 * @details A positive value means counter-clockwise orientation and a
 * negative value means clockwise orientation.
 */
static double
buffer_ring_area(const POINTARRAY *pa)
{
  assert(pa);
  if (pa->npoints < 3)
    return 0.0;
  double area = 0.0;
  for (uint32_t i = 0; i < pa->npoints - 1; i++)
  {
    POINT4D p1, p2;
    getPoint4d_p(pa, i, &p1);
    getPoint4d_p(pa, i + 1, &p2);
    area += p1.x * p2.y - p2.x * p1.y;
  }
  return area * 0.5;
}

/**
 * @brief Return the effective outward side of a polygon ring.
 * @details For a counter-clockwise ring the interior is on the left and
 * therefore the exterior is on the right. For a clockwise ring the interior
 * is on the right and therefore the exterior is on the left.
 */
static bool
buffer_ring_outward_left(const POINTARRAY *pa)
{
  assert(pa);
  return buffer_ring_area(pa) < 0.0;
}

/**
 * @brief Compute the offset point of a ring vertex.
 * @details The point returned is the intersection of the two offset segments.
 * This is the proper miter point and is also used as the starting point for
 * the round/bevel join construction.
 */
static bool
buffer_ring_vertex(const POINT2D *p0, const POINT2D *p1, const POINT2D *p2,
  double radius, bool outward_left, POINT2D *result)
{
  assert(p0); assert(p1); assert(p2); assert(result);
  double dx1 = p1->x - p0->x;
  double dy1 = p1->y - p0->y;
  double dx2 = p2->x - p1->x;
  double dy2 = p2->y - p1->y;
  double l1 = hypot(dx1, dy1);
  double l2 = hypot(dx2, dy2);
  if (l1 <= FP_TOLERANCE || l2 <= FP_TOLERANCE)
    return false;
  dx1 /= l1;
  dy1 /= l1;
  dx2 /= l2;
  dy2 /= l2;
  double n1x, n1y, n2x, n2y;
  if (outward_left)
  {
    n1x = -dy1;
    n1y = dx1;
    n2x = -dy2;
    n2y = dx2;
  }
  else
  {
    n1x = dy1;
    n1y = -dx1;
    n2x = dy2;
    n2y = -dx2;
  }
  POINT2D a, b;
  a.x = p1->x + radius * n1x;
  a.y = p1->y + radius * n1y;
  b.x = p1->x + radius * n2x;
  b.y = p1->y + radius * n2y;
  /* Offset lines can become parallel at a 180-degree vertex */
  if (! buffer_line_intersection(a, dx1, dy1, b, dx2, dy2, result))
    return false;
  return true;
}

/**
 * @brief Construct an offset ring.
 * @details The ring is constructed from the offset segments of the input ring.
 * Convex vertices on the buffered side are connected using the requested
 * join style. Concave vertices are connected by the intersection of the
 * two offset lines. Round joins are represented by exact circular arcs.
 * @param[in] source Source polygon ring
 * @param[in] radius Buffer distance
 * @param[in] outward_left True if the buffered side is left of the ring
 * traversal direction
 * @param[in] join_style Join style
 * @param[in] mitre_limit Maximum mitre ratio
 * @param[in] srid Spatial reference identifier
 * @return The buffered ring as a CircularString, or @p NULL if the offset
 * cannot be constructed
 */
static LWCIRCSTRING *
buffer_ring(const POINTARRAY *source, double radius, bool outward_left,
  JoinStyle join_style, double mitre_limit, int32_t srid)
{
  assert(source); assert(radius > 0.0);
  if (source->npoints < 4)
    return NULL;
  /* The input ring is explicitly closed. The last point is therefore
   * identical to the first point and is not treated as a separate vertex. */
  uint32_t n = source->npoints - 1;
  if (n < 3)
    return NULL;
  POINT2D *points = palloc(sizeof(POINT2D) * n);
  for (uint32_t i = 0; i < n; i++)
  {
    POINT4D p;
    getPoint4d_p(source, i, &p);
    points[i].x = p.x;
    points[i].y = p.y;
  }
  /* For every input segment we compute its unit normal on the buffered side */
  double *nx = palloc(sizeof(double) * n);
  double *ny = palloc(sizeof(double) * n);
  for (uint32_t i = 0; i < n; i++)
  {
    uint32_t next = (i + 1) % n;
    double dx = points[next].x - points[i].x;
    double dy = points[next].y - points[i].y;
    double len = hypot(dx, dy);
    if (len <= FP_TOLERANCE)
    {
      pfree(points); pfree(nx); pfree(ny);
      return NULL;
    }
    dx /= len;
    dy /= len;
    if (outward_left)
    {
      nx[i] = -dy;
      ny[i] = dx;
    }
    else
    {
      nx[i] = dy;
      ny[i] = -dx;
    }
  }

  /* Allocate enough room for the CircularString. Round joins may be split
   * into several arcs. The allocation is deliberately generous. */
  POINTARRAY *ring = ptarray_construct_empty(LW_FALSE, LW_FALSE, 8 * n + 8);
  /* At vertex i:
   * - incoming = offset endpoint of segment (i-1) -> i
   * - outgoing = offset start point of segment i -> (i+1)
   * These are the two tangent points that must be connected by the
   * appropriate join. */
  for (uint32_t i = 0; i < n; i++)
  {
    uint32_t prev = (i + n - 1) % n;
    uint32_t next = (i + 1) % n;
    POINT2D incoming, outgoing;
    incoming.x = points[i].x + radius * nx[prev];
    incoming.y = points[i].y + radius * ny[prev];
    outgoing.x = points[i].x + radius * nx[i];
    outgoing.y = points[i].y + radius * ny[i];
    /* For the first vertex we start at the incoming tangent point */
    if (i == 0)
      buffer_append_point(ring, incoming.x, incoming.y);
    else
      /* The previous join ended at the outgoing tangent point of the previous
       * vertex. Connect that point to the incoming tangent point of this
       * vertex. This is an offset straight segment. */
      buffer_append_point(ring, incoming.x, incoming.y);

    /* Determine the turn of the original ring */
    double dx1 = points[i].x - points[prev].x;
    double dy1 = points[i].y - points[prev].y;
    double dx2 = points[next].x - points[i].x;
    double dy2 = points[next].y - points[i].y;
    double turn = buffer_cross(dx1, dy1, dx2, dy2);
    /* Determine whether the vertex is convex on the buffered side.
     * For a left-side offset: positive turn -> convex
     * For a right-side offset: negative turn -> convex */
    bool convex;
    if (outward_left)
      convex = turn > FP_TOLERANCE;
    else
      convex = turn < -FP_TOLERANCE;

    /* A concave vertex does not receive a round/bevel/miter lobe.
     * Instead the two offset segments meet at their intersection. */
    if (! convex)
    {
      POINT2D intersection;
      /* Incoming offset line */
      double in_dx = points[i].x - points[prev].x;
      double in_dy = points[i].y - points[prev].y;
      /* Outgoing offset line */
      double out_dx = points[next].x - points[i].x;
      double out_dy = points[next].y - points[i].y;
      if (! buffer_line_intersection(incoming, in_dx, in_dy, outgoing,
          out_dx, out_dy, &intersection))
      {
        pfree(points); pfree(nx); pfree(ny); ptarray_free(ring);
        return NULL;
      }
      buffer_append_point(ring, intersection.x, intersection.y);
      continue;
    }

    /* Convex vertex */
    switch (join_style)
    {
      case JOIN_ROUND:
      {
        double a1 = atan2(incoming.y - points[i].y, incoming.x - points[i].x);
        double a2 = atan2(outgoing.y - points[i].y, outgoing.x - points[i].x);
        /* For a convex corner, the arc must travel through the exterior of the
         * original polygon */
        bool ccw = outward_left;
        buffer_append_arc(ring, points[i].x, points[i].y, radius, a1, a2, ccw);
        break;
      }
      case JOIN_BEVEL:
      {
        /* A bevel join is simply the straight segment joining the two tangent
         * points */
        buffer_append_point(ring, outgoing.x, outgoing.y);
        break;
      }
      case JOIN_MITRE:
      {
        POINT2D intersection;
        double in_dx = points[i].x - points[prev].x;
        double in_dy = points[i].y - points[prev].y;
        double out_dx = points[next].x - points[i].x;
        double out_dy = points[next].y - points[i].y;
        bool ok = buffer_line_intersection(incoming, in_dx, in_dy, outgoing,
          out_dx, out_dy, &intersection);
        if (! ok)
        {
          /* Parallel offset lines. Fall back to bevel. */
          buffer_append_point(ring, outgoing.x, outgoing.y);
          break;
        }
        double miter_length = hypot(intersection.x - points[i].x,
          intersection.y - points[i].y);
        if (miter_length > radius * mitre_limit + FP_TOLERANCE)
        {
          /* PostGIS-style miter limit fallback */
          buffer_append_point(ring, outgoing.x, outgoing.y);
        }
        else
        {
          buffer_append_point(ring, intersection.x, intersection.y);
          buffer_append_point(ring, outgoing.x, outgoing.y);
        }
        break;
      }
      default:
        pfree(points); pfree(nx); pfree(ny); ptarray_free(ring);
        return NULL;
    }
  }

  /* Close the ring by returning to the first point. The first input vertex
   * was processed last through the cyclic construction, so the final point
   * is the initial incoming tangent point. */
  POINT2D first;
  first.x = points[0].x + radius * nx[n - 1];
  first.y = points[0].y + radius * ny[n - 1];
  buffer_append_point(ring, first.x, first.y);
  /* Construct the exact circular string */
  LWCIRCSTRING *result = lwcircstring_construct(srid, NULL, ring);
  pfree(points); pfree(nx); pfree(ny);
  return result;
}

/*****************************************************************************
 * Buffer - POINT
 *****************************************************************************/

/**
 * @brief Construct a circle buffer around a POINT
 */
static LWGEOM *
meos_buffer_point(const LWPOINT *point, double radius)
{
  assert(point); assert(radius > 0.0);
  int32_t srid = lwgeom_get_srid((const LWGEOM *) point);
  POINT4D pt;
  lwpoint_getPoint4d_p(point, &pt);
  return lwcircle_make(pt.x, pt.y, radius, srid);
}

/*****************************************************************************
 * Buffer - LINESTRING
 *****************************************************************************/

/**
 * @brief Construct a curved buffer around a LINESTRING
 * @details The boundary consists of LINESTRING and CIRCULARSTRING components.
 * Supported:
 *   - round joins
 *   - mitre joins
 *   - bevel joins
 *   - round caps
 *   - flat caps
 *   - square caps
 */
static LWGEOM *
meos_buffer_line(const LWLINE *line, double radius, JoinStyle join_style,
  EndCapStyle cap_style, double mitre_limit)
{
  assert(line); assert(radius > 0.0);
  const int32_t srid = lwgeom_get_srid((const LWGEOM *) line);
  if (! line->points || line->points->npoints < 2)
  {
    LWPOLY *empty = lwpoly_construct_empty(srid, 0, 0);
    return lwpoly_as_lwgeom(empty);
  }
  uint32_t npoints = line->points->npoints;
  POINT2D *points = palloc(sizeof(POINT2D) * npoints);
  for (uint32_t i = 0; i < npoints; i++)
  {
    POINT4D point;
    getPoint4d_p(line->points, i, &point);
    points[i].x = point.x;
    points[i].y = point.y;
  }

  /* Remove duplicate consecutive points */
  uint32_t nvalid = 0;
  for (uint32_t i = 0; i < npoints; i++)
  {
    if (nvalid == 0 ||
        hypot(points[i].x - points[nvalid - 1].x,
              points[i].y - points[nvalid - 1].y) >  FP_TOLERANCE)
    {
      points[nvalid++] = points[i];
    }
  }
  if (nvalid < 2)
  {
    LWGEOM *result = lwcircle_make(points[0].x, points[0].y, radius, srid);
    pfree(points);
    return result;
  }
  npoints = nvalid;

  /* Compute the direction and normal of every segment */
  double *dx = palloc(sizeof(double) * (npoints - 1));
  double *dy = palloc(sizeof(double) * (npoints - 1));
  double *nx = palloc(sizeof(double) * (npoints - 1));
  double *ny = palloc(sizeof(double) * (npoints - 1));
  for (uint32_t i = 0; i < npoints - 1; i++)
  {
    double length;
    dx[i] = points[i + 1].x - points[i].x;
    dy[i] = points[i + 1].y - points[i].y;
    length = hypot(dx[i], dy[i]);
    dx[i] /= length;
    dy[i] /= length;
    nx[i] = -dy[i];
    ny[i] = dx[i];
  }

  /* Offset points on the left and right side.
   * For vertices the intersection of the two offset lines is used.
   * This is important: simply averaging normals produces incorrect
   * buffer distances at sharp angles. */
  POINT2D *left = palloc(sizeof(POINT2D) * npoints);
  POINT2D *right = palloc(sizeof(POINT2D) * npoints);
  left[0] = buffer_point_offset(points[0].x, points[0].y, nx[0], ny[0],
    radius);
  right[0] = buffer_point_offset(points[0].x, points[0].y, -nx[0], -ny[0],
    radius);
  left[npoints - 1] = buffer_point_offset(points[npoints - 1].x,
    points[npoints - 1].y, nx[npoints - 2], ny[npoints - 2], radius);
  right[npoints - 1] = buffer_point_offset(points[npoints - 1].x,
    points[npoints - 1].y, -nx[npoints - 2], -ny[npoints - 2], radius);

  for (uint32_t i = 1; i < npoints - 1; i++)
  {
    POINT2D p1 = buffer_point_offset(points[i].x, points[i].y,
      nx[i - 1], ny[i - 1], radius);
    POINT2D p2 = buffer_point_offset(points[i].x, points[i].y,
      nx[i], ny[i], radius);
    if (! buffer_line_intersection(p1, dx[i - 1], dy[i - 1], p2, dx[i], dy[i],
      &left[i]))
    {
      /* Parallel segments. Use the second offset point */
      left[i] = p2;
    }

    POINT2D r1 = buffer_point_offset(points[i].x, points[i].y,
      -nx[i - 1], -ny[i - 1], radius);
    POINT2D r2 = buffer_point_offset(points[i].x, points[i].y,
      -nx[i], -ny[i], radius);
    if (! buffer_line_intersection(r1, dx[i - 1], dy[i - 1], r2, dx[i], dy[i],
      &right[i]))
    {
      right[i] = r2;
    }
  }

  /* Construct the outer boundary as a compound curve */
  LWCOMPOUND *ring = lwcompound_construct_empty(srid, 0, 0);

  /* Left side */
  buffer_add_segment(ring, srid, left[0], left[1]);
  for (uint32_t i = 1; i < npoints - 1; i++)
  {
    double turn = buffer_cross(dx[i - 1], dy[i - 1], dx[i], dy[i]);
    bool outer = turn > FP_TOLERANCE;
    buffer_add_join(ring, srid, points[i], left[i], left[i], radius,
      join_style, mitre_limit, outer);
    buffer_add_segment(ring, srid, left[i], left[i + 1]);
  }

  /* End cap */
  if (cap_style == ENDCAP_ROUND)
  {
    buffer_add_round_cap(ring, srid, points[npoints - 1], left[npoints - 1],
      right[npoints - 1], radius, true);
  }
  else
  {
    POINT2D l = left[npoints - 1];
    POINT2D r = right[npoints - 1];
    if (cap_style == ENDCAP_SQUARE)
    {
      l.x += dx[npoints - 2] * radius;
      l.y += dy[npoints - 2] * radius;
      r.x += dx[npoints - 2] * radius;
      r.y += dy[npoints - 2] * radius;
    }
    buffer_add_segment(ring, srid, l, r);
  }

  /* Rigth side, in reverse direction */
  buffer_add_segment(ring, srid, right[npoints - 1], right[npoints - 2]);
  for (int i = (int) npoints - 2; i > 0; i--)
  {
    double turn = buffer_cross(dx[i - 1], dy[i - 1], dx[i], dy[i]);
    bool outer = turn < -FP_TOLERANCE;
    buffer_add_join(ring, srid, points[i], right[i], right[i], radius,
      join_style, mitre_limit, outer);
    buffer_add_segment(ring, srid, right[i], right[i - 1]);
  }

  /* Start cap */
  if (cap_style == ENDCAP_ROUND)
  {
    buffer_add_round_cap(ring, srid, points[0], right[0], left[0], radius,
      true);
  }
  else
  {
    POINT2D r = right[0];
    POINT2D l = left[0];
    if (cap_style == ENDCAP_SQUARE)
    {
      r.x -= dx[0] * radius;
      r.y -= dy[0] * radius;
      l.x -= dx[0] * radius;
      l.y -= dy[0] * radius;
    }
    buffer_add_segment(ring, srid, r, l);
  }

  /* Close the compound curve */
  if (ring->ngeoms > 0)
  {
    LWGEOM *first = ring->geoms[0];
    LWGEOM *last = ring->geoms[ring->ngeoms - 1];
    (void) first;
    (void) last;
  }

  /* A CURVEPOLYGON can directly contain the compound curve */
  LWCURVEPOLY *result = lwcurvepoly_construct_empty(srid, 0, 0);
  lwcurvepoly_add_ring(result, lwcompound_as_lwgeom(ring));

  pfree(points); pfree(dx); pfree(dy); pfree(nx); pfree(ny); pfree(left);
  pfree(right);
  return lwcurvepoly_as_lwgeom(result);
}

/*****************************************************************************
 * Buffer - MULTILINE
 *****************************************************************************/

/**
 * @brief Buffer a MULTILINESTRING.
 * @details Each LINESTRING component is buffered independently.
 * Disjoint component buffers are returned as a MULTISURFACE.
 * Overlapping component buffers are also preserved as a MULTISURFACE
 * at this stage. The subsequent polygon-union layer is responsible for
 * dissolving overlapping components.
 * @param[in] mline MULTILINESTRING to buffer
 * @param[in] radius Buffer radius
 * @param[in] join_style Join style
 * @param[in] cap_style End-cap style
 * @param[in] mitre_limit Mitre limit
 * @return Buffered geometry, or an empty MULTISURFACE
 */
static LWGEOM *
meos_buffer_mline(const LWMLINE *mline, double radius, JoinStyle join_style,
  EndCapStyle cap_style, double mitre_limit)
{
  assert(mline); assert(radius > 0.0);
  int32_t srid = lwgeom_get_srid((const LWGEOM *) mline);
  uint32_t ngeoms = mline->ngeoms;
  /* Empty MULTILINESTRING. */
  if (ngeoms == 0)
  {
    LWCOLLECTION *result = lwcollection_construct_empty(MULTISURFACETYPE, srid,
      0, 0);
    return lwcollection_as_lwgeom(result);
  }

  /* Allocate space for the buffered components */
  LWGEOM **buffers = palloc(sizeof(LWGEOM *) * ngeoms);
  uint32_t count = 0;

  /* Buffer every LINESTRING component independently */
  for (uint32_t i = 0; i < ngeoms; i++)
  {
    const LWGEOM *component = (const LWGEOM *) mline->geoms[i];
    if (! component || lwgeom_is_empty(component))
      continue;
    /* LWMLINE components should be LINESTRINGs */
    if (component->type != LINETYPE)
      continue;

    LWGEOM *buffer = meos_buffer_line((const LWLINE *) component, radius,
      join_style, cap_style, mitre_limit);
    if (buffer)
      buffers[count++] = buffer;
  }

  /* All components were empty */
  if (count == 0)
  {
    pfree(buffers);
    LWCOLLECTION *result = lwcollection_construct_empty(MULTISURFACETYPE, srid,
      0, 0);
    return lwcollection_as_lwgeom(result);
  }

  /* A single component does not need a MULTISURFACE wrapper. */
  if (count == 1)
  {
    LWGEOM *result = buffers[0];
    pfree(buffers);
    return result;
  }

  /* Determine whether component buffers intersect. We only detect the
   * relation here. We do not perform the union yet. The intersection
   * information will be consumed by the polygon overlay layer.
   */
  bool overlap = false;
  for (uint32_t i = 0; i < count && ! overlap; i++)
  {
    for (uint32_t j = i + 1; j < count; j++)
    {
      int relation = buffer_components_relation(buffers[i], buffers[j]);
      if (relation != 0)
      {
        overlap = true;
        break;
      }
    }
  }

  /* For now both cases are represented by the component surfaces.
   * If the components are disjoint, this is already a correct
   * MULTISURFACE representation.
   * If they overlap, this is an intermediate representation only.
   * The polygon-union layer will subsequently dissolve the overlap.
   */
  (void) overlap;

  /* lwcollection_construct() takes ownership of the geometry array.
   * Therefore buffers must NOT be freed after this call. */
  LWCOLLECTION *result = lwcollection_construct(MULTISURFACETYPE, srid, NULL,
    count, buffers);
  return lwcollection_as_lwgeom(result);
}

/*****************************************************************************
 * Buffer - POLYGON
 *****************************************************************************/

/**
 * @brief Buffer a POLYGON.
 * @details The exterior ring is expanded and interior rings are contracted.
 * Round joins are represented using exact circular arcs.
 *
 * This implementation deliberately does not perform polygon overlay.
 * Consequently, cases where an offset ring collapses or self-intersects
 * are rejected and return NULL.
 */
static LWGEOM *
meos_buffer_poly(const LWPOLY *poly, double radius, JoinStyle join_style,
  EndCapStyle cap_style, double mitre_limit)
{
  assert(poly); assert(radius > 0.0);
  (void) cap_style;
  int32_t srid = lwgeom_get_srid((const LWGEOM *) poly);
  if (poly->nrings == 0)
  {
    LWCOLLECTION *result = lwcollection_construct_empty(MULTISURFACETYPE, srid,
      0, 0);
    return lwcollection_as_lwgeom(result);
  }

  /* The first ring is the exterior ring */
  bool exterior_left = buffer_ring_outward_left(poly->rings[0]);
  LWCIRCSTRING *exterior = buffer_ring(poly->rings[0], radius, exterior_left,
    join_style, mitre_limit, srid);
  if (! exterior)
    return NULL;

  /* Construct the curved polygon */
  LWCURVEPOLY *result = lwcurvepoly_construct_empty(srid, 0, 0);
  lwcurvepoly_add_ring(result, lwcircstring_as_lwgeom(exterior));

  /* Holes.
   * A positive polygon buffer contracts the holes. Therefore the
   * buffering side is the opposite of the exterior side.
   */
  for (uint32_t i = 1; i < poly->nrings; i++)
  {
    LWCIRCSTRING *hole = buffer_ring(poly->rings[i], radius,
      ! buffer_ring_outward_left(poly->rings[i]), join_style, mitre_limit,
      srid);
    if (! hole)
    {
      lwgeom_free(lwcurvepoly_as_lwgeom(result));
      return NULL;
    }
    lwcurvepoly_add_ring(result, lwcircstring_as_lwgeom(hole));
  }
  return lwcurvepoly_as_lwgeom(result);
}

/*****************************************************************************
 * Buffer - MULTIPOLYGON
 *****************************************************************************/

/**
 * @brief Buffer a MULTIPOLYGON.
 * @details Each polygon component is buffered independently.
 * If the resulting components are disjoint, the result is returned as a
 * MULTISURFACE. If two component buffers overlap, they must be unioned
 * before returning the result. The polygon overlay/union layer is
 * responsible for that operation and is not performed here.
 * @param[in] mpoly MULTIPOLYGON
 * @param[in] radius Buffer radius
 * @param[in] join_style Join style
 * @param[in] cap_style Cap style
 * @param[in] mitre_limit Mitre limit
 * @return Buffered geometry, or @p NULL if a polygon union is required
 */
static LWGEOM *
meos_buffer_mpoly(const LWMPOLY *mpoly, double radius, JoinStyle join_style,
  EndCapStyle cap_style, double mitre_limit)
{
  assert(mpoly); assert(radius > 0.0);
  uint32_t ngeoms = mpoly->ngeoms;
  int32_t srid = lwgeom_get_srid((const LWGEOM *) mpoly);
  if (ngeoms == 0)
  {
    LWCOLLECTION *result = lwcollection_construct_empty(MULTISURFACETYPE, srid,
      0, 0);
    return lwcollection_as_lwgeom(result);
  }

  LWGEOM **buffers = palloc(sizeof(LWGEOM *) * ngeoms);
  uint32_t count = 0;
  /* Buffer every polygon independently */
  for (uint32_t i = 0; i < ngeoms; i++)
  {
    const LWGEOM *component = (const LWGEOM *) mpoly->geoms[i];
    if (! component || lwgeom_is_empty(component))
      continue;
    if (component->type != POLYGONTYPE)
      continue;
    LWGEOM *buffer = meos_buffer_poly((const LWPOLY *) component, radius,
      join_style, cap_style, mitre_limit);
    if (buffer)
      buffers[count++] = buffer;
  }

  /* All components were empty */
  if (count == 0)
  {
    pfree(buffers);
    LWCOLLECTION *result = lwcollection_construct_empty(MULTISURFACETYPE, srid,
      0, 0);
    return lwcollection_as_lwgeom(result);
  }

  /* If there are exactly two buffered components, try the
   * curved-boundary union.
   * At this stage the union implementation supports the crossing case
   * with discrete boundary intersections and one connected resulting
   * boundary. Unsupported topology returns NULL. */
  if (count == 2)
  {
    if (buffer_components_overlap(buffers[0], buffers[1]))
    {
      LWGEOM *union_result = buffer_areal_union_simple(buffers[0], buffers[1]);
      if (union_result)
      {
        lwgeom_free(buffers[0]); lwgeom_free(buffers[1]); pfree(buffers);
        return union_result;
      }

      /* The two components overlap, but the current overlay
       * layer cannot handle their topology yet */
      lwgeom_free(buffers[0]); lwgeom_free(buffers[1]); pfree(buffers);
      return NULL;
    }
  }

  /* The components are disjoint and can therefore safely be represented
   * as a MULTISURFACE. */
  LWGEOM **geoms = palloc(sizeof(LWGEOM *) * count);
  for (uint32_t i = 0; i < count; i++)
    geoms[i] = buffers[i];
  LWCOLLECTION *result = lwcollection_construct(MULTISURFACETYPE, srid,
    NULL, count, geoms);
  pfree(buffers);
  return lwcollection_as_lwgeom(result);
}

/*****************************************************************************
 * Buffer - GEOMETRYCOLLECTION
 *****************************************************************************/

/**
 * @brief Buffer a GEOMETRYCOLLECTION.
 * @details Each component is buffered recursively. Nested geometry
 * collections are therefore handled transparently.
 *
 * The component buffers are not unioned at this stage. If two component
 * buffers overlap, the function returns NULL because returning the
 * overlapping surfaces would not be equivalent to ST_Buffer.
 *
 * If all component buffers are disjoint, they are returned as a
 * MULTISURFACE.
 */
static LWGEOM *
meos_buffer_collection(const LWCOLLECTION *collection, double radius,
  JoinStyle join_style, EndCapStyle cap_style, double mitre_limit)
{
  assert(collection); assert(radius > 0.0);
  int32_t srid = lwgeom_get_srid((const LWGEOM *) collection);

  /* First count the maximum number of output components.
   * A nested GEOMETRYCOLLECTION may produce more than one component,
   * therefore we allocate dynamically below rather than relying on
   * collection->ngeoms as the final count. */
  uint32_t capacity = collection->ngeoms > 0 ? collection->ngeoms : 1;
  uint32_t count = 0;
  LWGEOM **buffers = palloc(sizeof(LWGEOM *) * capacity);

  /* Buffer every component */
  for (uint32_t i = 0; i < collection->ngeoms; i++)
  {
    const LWGEOM *component = collection->geoms[i];
    if (! component || lwgeom_is_empty(component))
      continue;
    LWGEOM *buffer = NULL;
    /* A nested GEOMETRYCOLLECTION may itself produce a MULTISURFACE.
     * We keep this as one component for now. The polygon-union layer
     * will later be responsible for dissolving all overlapping pieces. */
    if (component->type == COLLECTIONTYPE)
      buffer = meos_buffer_collection((const LWCOLLECTION *) component, radius,
        join_style, cap_style, mitre_limit);
    else
      buffer = meos_buffer(component, radius, join_style, cap_style,
        mitre_limit);
    if (! buffer)
      continue;
    /* Grow the output array if necessary */
    if (count == capacity)
    {
      capacity *= 2;
      buffers = repalloc(buffers, sizeof(LWGEOM *) * capacity);
    }
    buffers[count++] = buffer;
  }

  /* No non-empty component produced a buffer */
  if (count == 0)
  {
    pfree(buffers);
    LWCOLLECTION *result = lwcollection_construct_empty(MULTISURFACETYPE, srid,
      0, 0);
    return lwcollection_as_lwgeom(result);
  }

  /* A single component needs no collection wrapper */
  if (count == 1)
  {
    LWGEOM *result = buffers[0];
    pfree(buffers);
    return result;
  }

  /* Check whether any two component buffers overlap.
   * We do not union them yet. Returning the individual overlapping
   * surfaces would produce a geometrically incorrect result. */
  for (uint32_t i = 0; i < count; i++)
  {
    for (uint32_t j = i + 1; j < count; j++)
    {
      if (! buffer_components_overlap(buffers[i], buffers[j]))
        continue;
      /* The polygon overlay/union layer will handle this case in
       * the next implementation slice */
      for (uint32_t k = 0; k < count; k++)
        lwgeom_free(buffers[k]);
      pfree(buffers);
      return NULL;
    }
  }

  /* All component buffers are disjoint. They can therefore safely
   * be represented as a MULTISURFACE without performing a union. */
  LWCOLLECTION *result = lwcollection_construct(MULTISURFACETYPE, srid, NULL,
    count, buffers);
  pfree(buffers);
  return lwcollection_as_lwgeom(result);
}

/*****************************************************************************
 * Buffer - Dispatcher
 *****************************************************************************/

/**
 * @brief Native MEOS implementation of ST_Buffer.
 * @details Currently supported:
 * - POINT
 * - LINESTRING
 * - MULTILINESTRING
 * - POLYGONTYPE
 * - MULTIPOLYGONTYPE
 * - GEOMETRYCOLLECTION
 * Polygon buffering is handled by the polygon buffering layer.
 * Component buffers are not unioned yet. If buffering a collection
 * produces overlapping components, NULL is returned.
 */
LWGEOM *
meos_buffer(const LWGEOM *geom, double radius, JoinStyle join_style,
  EndCapStyle cap_style, double mitre_limit)
{
  assert(geom);
  int32_t srid = lwgeom_get_srid(geom);
  /* ST_Buffer with a non-positive distance produces an empty polygon
   * for the geometry types handled here */
  if (radius <= 0.0)
  {
    LWPOLY *empty = lwpoly_construct_empty(srid, 0, 0);
    return lwpoly_as_lwgeom(empty);
  }
  if (mitre_limit <= 0.0)
    mitre_limit = 5.0;
  switch (geom->type)
  {
    case POINTTYPE:
      return meos_buffer_point((const LWPOINT *) geom, radius);
    case LINETYPE:
      return meos_buffer_line((const LWLINE *) geom, radius, join_style,
        cap_style, mitre_limit);
    case MULTILINETYPE:
      return meos_buffer_mline((const LWMLINE *) geom, radius, join_style,
        cap_style, mitre_limit);
    case POLYGONTYPE:
      return meos_buffer_poly((const LWPOLY *) geom, radius, join_style,
        cap_style, mitre_limit);
    case MULTIPOLYGONTYPE:
      return meos_buffer_mpoly((const LWMPOLY *) geom, radius, join_style,
        cap_style, mitre_limit);
    case COLLECTIONTYPE:
      return meos_buffer_collection((const LWCOLLECTION *) geom, radius,
        join_style, cap_style, mitre_limit);
    default:
      return NULL;
  }
}

/**
 * @ingroup meos_geo_base_spatial
 * @brief Return a @p POLYGON or a @p MULTIPOLYGON that represents all points
 * whose distance from a geometry/geography is less than or equal to a given
 * distance
 * @param[in] gs Geometry
 * @param[in] size Distance
 * @param[in] params Buffer style parameters
 * @note PostGIS function: @p ST_Buffer(PG_FUNCTION_ARGS)
 */
GSERIALIZED *
geom_buffer_meos(const GSERIALIZED *gs, double size, const char *params)
{
  /* Ensure the validity of the arguments */
  VALIDATE_NOT_NULL(gs, NULL); VALIDATE_NOT_NULL(params, NULL);
  if (! ensure_not_geodetic_geo(gs))
    return NULL;

  int quadsegs = 8; /* the default */
  int singleside = 0; /* the default */
  const double DEFAULT_MITRE_LIMIT = 5.0;
  const int DEFAULT_ENDCAP_STYLE = ENDCAP_ROUND;
  const int DEFAULT_JOIN_STYLE = JOIN_ROUND;
  double mitre_limit = DEFAULT_MITRE_LIMIT;
  int cap_style = DEFAULT_ENDCAP_STYLE;
  int join_style  = DEFAULT_JOIN_STYLE;

  /* In the for loop below the params parameter is modified.
   * Therefore we need to take a copy of it */
  size_t params_size = strlen(params) + 1;
  char *params1 = palloc(params_size);
  memcpy(params1, params, params_size);
  char *param;
  for (param = params1; ; param = NULL)
  {
    char *key, *val;
    param = strtok(param, " ");
    if (! param)
      break;

    key = param;
    val = strchr(key, '=');
    if (! val || *(val + 1) == '\0')
    {
      meos_error(ERROR, MEOS_ERR_INTERNAL_TYPE_ERROR,
        "Missing value for buffer parameter %s", key);
      return NULL;
    }
    *val = '\0';
    ++val;

    if (! strcmp(key, "endcap"))
    {
      /* Supported end cap styles: "round", "flat", "square" */
      if (! strcmp(val, "round"))
        cap_style = ENDCAP_ROUND;
      else if (! strcmp(val, "flat") || ! strcmp(val, "butt"))
        cap_style = ENDCAP_FLAT;
      else if (! strcmp(val, "square"))
        cap_style = ENDCAP_SQUARE;
      else
      {
        meos_error(ERROR, MEOS_ERR_INTERNAL_TYPE_ERROR,
          "Invalid buffer end cap style: %s (accept: 'round', 'flat', "
          "'butt' or 'square')", val);
        return NULL;
      }
    }
    else if (! strcmp(key, "join"))
    {
      if (! strcmp(val, "round"))
        join_style = JOIN_ROUND;
      else if (! strcmp(val, "mitre") || ! strcmp(val, "miter"))
        join_style = JOIN_MITRE;
      else if (! strcmp(val, "bevel"))
        join_style = JOIN_BEVEL;
      else
      {
        meos_error(ERROR, MEOS_ERR_INTERNAL_TYPE_ERROR,
          "Invalid buffer end cap style: %s (accept: 'round', 'mitre', "
          "'miter'  or 'bevel')", val);
        return NULL;
      }
    }
    else if (! strcmp(key, "mitre_limit") || ! strcmp(key, "miter_limit"))
      /* mitre_limit is a float */
      mitre_limit = atof(val);
    else if (! strcmp(key, "quad_segs"))
      /* quadrant segments is an int */
      quadsegs = atoi(val);
    else if (! strcmp(key, "side"))
    {
      if (! strcmp(val, "both"))
        singleside = 0;
      else if (! strcmp(val, "left"))
        singleside = 1;
      else if (! strcmp(val, "right"))
      {
        singleside = 1;
        size *= -1;
      }
      else
      {
        meos_error(ERROR, MEOS_ERR_INTERNAL_TYPE_ERROR,
          "Invalid side parameter: %s (accept: 'right', 'left', 'both')",
          val);
        return NULL;
      }
    }
    else
    {
      meos_error(ERROR, MEOS_ERR_INTERNAL_TYPE_ERROR,
        "Invalid buffer parameter: %s (accept: 'endcap', 'join', "
        "'mitre_limit', 'miter_limit', 'quad_segs' and 'side')", key);
      return NULL;
    }
  }
  pfree(params1);

  LWGEOM *lwg;

  /* Empty.Buffer() == Empty[polygon] */
  if (gserialized_is_empty(gs))
  {
    lwg = lwpoly_as_lwgeom(lwpoly_construct_empty(gserialized_get_srid(gs),
      0, 0)); // buffer wouldn't give back z or m anyway
    GSERIALIZED *result = geo_serialize(lwg);
    lwgeom_free(lwg);
    return result;
  }

  lwg = lwgeom_from_gserialized(gs);
  if (! lwgeom_isfinite(lwg))
  {
    meos_error(ERROR, MEOS_ERR_INTERNAL_ERROR,
      "Geometry contains invalid coordinates");
    lwgeom_free(lwg);
    return NULL;
  }

  LWGEOM *res = meos_buffer(lwg, size, join_style, cap_style, mitre_limit);
  GSERIALIZED *result = geo_serialize(res);
  lwgeom_free(lwg); lwgeom_free(res);
  return result;
}


/*****************************************************************************/