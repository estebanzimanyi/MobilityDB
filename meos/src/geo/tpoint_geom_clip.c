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
 * @brief Return true if a segment defined by two points intersects a polygon
 * edge, 0 otherwise
 * @param[in] a,b Points defining the segment
 * @param[in] e Polygon edge
 * @param[out] t Fraction in [0,1] determining the intersection point 
 */
static bool
linesegm_intersect(POINT4D a, POINT4D b, Edge e, double *t)
{
  double dx = b.x - a.x;
  double dy = b.y - a.y;

  double ex = e.x2 - e.x1;
  double ey = e.y2 - e.y1;

  /* Compute the determinant */
  double det = dx * (-ey) + dy * ex;
  if (fabs(det) < POSTGIS_FP_TOLERANCE)
    return false;

  double cx = e.x1 - a.x;
  double cy = e.y1 - a.y;

  double t1 = (cx * (-ey) + cy * ex) / det;
  if (t1 < 0 || t1 > 1)
    return false;

  *t = t1;
  return true;
}

/*****************************************************************************
 * Events
 *****************************************************************************/

/**
 * @brief Return the double obtained by clamping a double to [0,1]
 */
static inline double
clamp01(double t)
{
  return t < 0 ? 0 : (t > 1 ? 1 : t);
}

/**
 * @brief Build the sweep events obtained by collecting the intersections
 * between a segment and an array of geometry edges
 * @param[in] a,b Points defining the segment
 * @param[in] e Array of geometry edges
 * @param[in] n Number of elements in the array
 * @param[inout] Dynamic array of the collected events 
 */
static void
build_events(POINT4D a, POINT4D b, const Edge *edges, int n, MeosArray *events)
{
  for (int i = 0; i < n; i++)
  {
    double t;
    if (linesegm_intersect(a, b, edges[i], &t))
    {
      t = clamp01(t);
      /* Signed contribution */
      Event e = {t, edges[i].delta};
      meos_array_add(events, &e);
    }
  }
}

/**
 * @brief Comparator for sweep events
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
 * @brief Compute the intervals during which an intersecti
 */
static void
sweep_xy(MeosArray *events, MeosArray *intervals, int initial_coverage)
{
  Event *ev = events->elems;
  int n = events->count;

  qsort(ev, n, sizeof(Event), event_cmp);

  int coverage = initial_coverage;
  double start = 0.0;

  /* already inside at t=0 */
  if (coverage != 0)
    start = 0.0;

  for (int i = 0; i < n; i++)
  {
    int prev = coverage;
    coverage += ev[i].delta;

    if (prev == 0 && coverage != 0)
    {
      start = ev[i].t;
    }
    else if (prev != 0 && coverage == 0)
    {
      ClipInterval in = {start, ev[i].t};
      meos_array_add(intervals, &in);
    }
  }
}

/*****************************************************************************
 * Clip a temporal sequence
 *****************************************************************************/

/**
 * @brief Return the initial winding of a point wrt a set of polygon edges
 */
static int
initial_winding(double x, double y, const Edge *edges, int nedges)
{
  int winding = 0;
  for (int i = 0; i < nedges; i++)
  {
    const Edge *e = &edges[i];

    /* ignore horizontal edges (PostGIS standard rule) */
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
 * @brief Return the POINT4D obtained by interpolating two points with respect
 * to a fraction in [0,1]
 */
static inline POINT4D
point4d_interp(POINT4D a, POINT4D b, double t)
{
  POINT4D p;
  p.x = a.x + t * (b.x - a.x);
  p.y = a.y + t * (b.y - a.y);
  p.z = a.z + t * (b.z - a.z);
  p.m = a.m + t * (b.m - a.m);
  return p;
}

/**
 * @brief Return in the dynamic array passed as last argument the clip events
 * found when looking for intersections between the segment defined by two
 * trajectory points with respect to the edges of a polygon 
 */
int
linesegm_clip_xy(POINT4D a, POINT4D b, const Edge *edges, int nedges,
  MeosArray *events, MeosArray *intervals, MeosArray *result_points)
{
  events->count = 0;
  intervals->count = 0;

  /* XY clipping */
  build_events(a, b, edges, nedges, events);

  int init = initial_winding(a.x, a.y, edges, nedges);
  sweep_xy(events, intervals, init);

  if (intervals->count == 0)
    return 0;

  ClipInterval *arr = intervals->elems;
  int out_n = 0;

  for (int i = 0; i < (int) intervals->count; i++)
  {
    POINT4D p0 = point4d_interp(a, b, arr[i].t0);
    POINT4D p1 = point4d_interp(a, b, arr[i].t1);
    meos_array_add(result_points, &p0);
    meos_array_add(result_points, &p1);
    out_n += 2;
  }
  return out_n;
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
 * @brief Clip a 2D/3D trajectory with linear interpolation with respect to a
 * geometry
 */
static TSequenceSet *
tpointseq_clip(const TSequence *seq, const Edge *edges, int nedges)
{
  assert(MEOS_FLAGS_GET_INTERP(seq->flags) == LINEAR);
  assert(! MEOS_FLAGS_GET_GEODETIC(seq->flags));

  int32_t srid = tspatial_srid((Temporal *) seq);
  bool hasz = MEOS_FLAGS_GET_Z(seq->flags);

  MeosArray *sequences = meos_array_init(sizeof(TSequence *));
  MeosArray *curr = meos_array_init(sizeof(POINT4D));
  MeosArray *segpts = meos_array_init(sizeof(POINT4D));

  /* reusable buffers */
  MeosArray *events = meos_array_init(sizeof(Event));
  MeosArray *intervals = meos_array_init(sizeof(ClipInterval));

  POINT4D a, b;
  datum_point4d(tinstant_value_p(TSEQUENCE_INST_N(seq, 0)), &a);
  for (int i = 1; i < seq->count; i++)
  {
    datum_point4d(tinstant_value_p(TSEQUENCE_INST_N(seq, i)), &b);

    segpts->count = 0;
    linesegm_clip_xy(a, b, edges, nedges, events, intervals, segpts);

    if (segpts->count == 0)
    {
      if (curr->count > 0)
      {
        TSequence *s = tsequence_from_trajpoints(
          curr->elems, curr->count,
          srid, hasz, seq->temptype);

        meos_array_add(sequences, &s);
        curr->count = 0;
      }
      a = b;
      continue;
    }

    POINT4D *pts = segpts->elems;

    for (int j = 0; j < (int) segpts->count; j += 2)
    {
      POINT4D p0 = pts[j];
      POINT4D p1 = pts[j + 1];

      if (curr->count == 0)
      {
        meos_array_add(curr, &p0);
      }
      else
      {
        POINT4D *last = &((POINT4D *) curr->elems)[curr->count - 1];

        if (fabs(last->x - p0.x) > 1e-9 ||
            fabs(last->y - p0.y) > 1e-9 ||
            fabs(last->z - p0.z) > 1e-9)
        {
          TSequence *s = tsequence_from_trajpoints(curr->elems, curr->count,
            srid, hasz, seq->temptype);
          meos_array_add(sequences, &s);
          curr->count = 0;
          meos_array_add(curr, &p0);
        }
      }
      meos_array_add(curr, &p1);
    }
    a = b;
  }

  /* final flush */
  if (curr->count > 0)
  {
    TSequence *s = tsequence_from_trajpoints(
      curr->elems, curr->count,
      srid, hasz, seq->temptype);

    meos_array_add(sequences, &s);
  }

  if (sequences->count == 0)
  {
    meos_array_destroy(curr, true);
    meos_array_destroy(segpts, true);
    meos_array_destroy(events, true);
    meos_array_destroy(intervals, true);
    meos_array_destroy(sequences, true);
    return NULL;
  }

  TSequenceSet *result =
    tsequenceset_make_free(sequences->elems,
      sequences->count, NORMALIZE);

  meos_array_destroy(curr, true);
  meos_array_destroy(segpts, true);
  meos_array_destroy(events, true);
  meos_array_destroy(intervals, true);
  meos_array_destroy(sequences, true);

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
