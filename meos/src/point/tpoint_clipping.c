/*****************************************************************************
 *
 * This MobilityDB code is provided under The PostgreSQL License.
 * Copyright (c) 2016-2023, Université libre de Bruxelles and MobilityDB
 * contributors
 *
 * MobilityDB includes portions of PostGIS version 3 source code released
 * under the GNU General Public License (GPLv2 or later).
 * Copyright (c) 2001-2023, PostGIS contributors
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
 * @brief Clipping functions for temporal points based on the Martinez-Rueda
 * clipping algorithm.
 * @note This implementation is based on the Javascript implementation in
 * https://github.com/w8r/martinez
 */

#include "point/tpoint_clipping.h"

/* C */
#include <assert.h>
/* PostgreSQL */
#include <postgres.h>
#include <utils/float.h>
#if ! MEOS
  #include <utils/memutils.h> /* for MaxAllocSize */
#endif /* ! MEOS */
/* PostGIS */
#include <liblwgeom.h>
/* MEOS */
#include <meos_internal.h>
#include "general/temporal.h"
#include "point/pqueue.h"
#include "point/splay_tree.h"
#include "point/stbox.h"
#include "point/tpoint.h"
#include "point/tpoint_spatialfuncs.h"

/*****************************************************************************/

const char* namesEventTypes[] =
{
  "Normal",
  "Non contributing",
  "Same Transition",
  "Different Transition"
};

/*****************************************************************************
 * Points
 *****************************************************************************/

/**
 * @brief Are the points equal?
 */
static bool
point2d_eq(const POINT2D *p1, const POINT2D *p2)
{
  return float8_eq(p1->x, p2->x) && float8_eq(p1->y, p2->y);
}

/**
 * @brief Set a POINT4D from a POINT2D
 */
static void
point2d_set_point4d(const POINT2D *p1, POINT4D *p2)
{
  memset(p2, 0, sizeof(POINT4D));
  p2->x = p1->x;
  p2->y = p1->y;
  return;
}

/*****************************************************************************
 * Contour
 *****************************************************************************/

/* Constants defining the initialization of POINTARRAY */
#define POINTARRAY_INITIAL_CAPACITY 1024
#define POINTARRAY_GROW 1   /**< double the capacity to expand the point array */

/**
 * @brief Create a new countour
 */
static Contour *
contour_make(void)
{
  Contour *result = palloc0(sizeof(Contour));
  result->points = ptarray_construct_empty(LW_FALSE, LW_FALSE,
    POINTARRAY_INITIAL_CAPACITY);
  result->holeIds = vector_make(TYPE_BY_VALUE);
  result->holeOf = -1;
  result->depth = -1;
  return result;
}

/**
 * @brief Free a contour
 */
static void
contour_free(Contour *c)
{
  /* We do not need to ptarray_free(c->points) since the point arrays are
   * passed to their polygons */
  // vector_free(c->holeIds);
  pfree(c);
  return;
}

/*****************************************************************************
 * SweepEvent
 *****************************************************************************/

/**
 * @brief Create a new sweepline event
 */
static SweepEvent *
swevent_make(POINT2D *point, bool left, SweepEvent *otherEvent, bool subject,
  EdgeType edgeType)
{
  SweepEvent *result = palloc0(sizeof(SweepEvent));
  result->left = left;
  result->point = *point;
  result->otherEvent = otherEvent;
  result->subject = subject;
  result->type = edgeType;
  /* Internal fields */
  result->inOut = false;
  result->otherInOut = false;
  result->prevInResult = NULL;
  /* Type of result transition (0 = not in result, +1 = out-in, -1, in-out) */
  result->resultTransition = 0;
  /* connection step */
  result->otherPos = -1;
  result->outputContourId = -1;
  result->isExteriorRing = true; // TODO: Looks unused, remove?
  return result;
}

/**
 * @brief Signed area of the triangle(p0, p1, p2)
 */
#define SIGNED_AREA(p0, p1, p2) ( \
  ((p0)->x - (p2)->x) * ((p1)->y - (p2)->y) - \
  ((p1)->x - (p2)->x) * ((p0)->y - (p2)->y) )

/**
 * @brief Return true if the sweepline event is below the point
 */
#define SWEVENT_IS_BELOW(e, p) ( \
  (e)->left ? SIGNED_AREA(&(e)->point, &(e)->otherEvent->point, (p)) > 0 : \
    SIGNED_AREA(&(e)->otherEvent->point, &(e)->point, (p)) > 0 )

/**
 * @brief Return true if the sweepline event is above the point
 */
#define SWEVENT_IS_ABOVE(e, p) ( \
  ! SWEVENT_IS_BELOW((e), (p)) )

/**
 * @brief Return true if the sweepline event is vertical
 */
#define SWEVENT_IS_VERTICAL(e) ( \
  e->point.x == e->otherEvent->point.x )

/**
 * @brief Does the event belong to result?
 */
#define SWEVENT_IN_RESULT(e) ( e->resultTransition != 0 )

/**
 * @brief Comparison of sweepline events
 * @note This comparison is used for the priority queue
 */
static int
swevent_cmp(const SweepEvent *e1, const SweepEvent *e2)
{
  const POINT2D *p1 = &e1->point;
  const POINT2D *p2 = &e2->point;

  /* Compare x-coordinate, event with lower x-coordinate is processed first */
  if (p1->x > p2->x) return 1;
  if (p1->x < p2->x) return -1;

  /* Same x-coordinate, event with lower y-coordinate is processed first */
  if (p1->y > p2->y) return 1;
  if (p1->y < p2->y) return -1;

  /* Same coordinates, but one is a left endpoint and the other is
   * a right endpoint. The right endpoint is processed first */
  if (e1->left != e2->left)
    return e1->left ? 1 : -1;

  /* Same coordinates, both events are left endpoints or right endpoints.
   * not collinear */
  if (SIGNED_AREA(p1, &e1->otherEvent->point, &e2->otherEvent->point) != 0)
    /* the event associate to the bottom segment is processed first */
    return (SWEVENT_IS_ABOVE(e1, &e2->otherEvent->point)) ? 1 : -1;

  return (! e1->subject && e2->subject) ? 1 : -1;
}

#if DEBUG_BUILD
/**
 * @brief Print sweepline events for debugging purpose
 */
static void
swevent_print(const SweepEvent *e)
{
  elog(WARNING, "Point: (%lg,%lg), Other point: (%lg,%lg), "
    "%s, %s, %s, %s, %s", // left, inside, inOut, type, subject
    e->point.x, e->point.y, e->otherEvent->point.x, e->otherEvent->point.y,
    e->left ? "Left" : "Right", e->inside ? "Inside" : "Outside",
    e->inOut ? "In-Out" : "Out-In", namesEventTypes[e->type],
    e->subject ? "Subject" : "Clipping");
  return;
}
#endif

/**
 * @brief Comparison of segments associated to two sweepline events
 * @note This comparison is used for the sweepline
 */
static int
segment_cmp(SweepEvent *e1, SweepEvent *e2)
{
  if (e1 == e2) return 0;

  /* Segments are not collinear */
  if (SIGNED_AREA(&e1->point, &e1->otherEvent->point, &e2->point) != 0 ||
    SIGNED_AREA(&e1->point, &e1->otherEvent->point, &e2->otherEvent->point) != 0)
  {
    /* If they share their left endpoint use the right endpoint to sort */
    if (point2d_eq(&e1->point, &e2->point))
      return SWEVENT_IS_BELOW(e1, &e2->otherEvent->point) ? -1 : 1;

    /* Different left endpoint: use the left endpoint to sort */
    if (e1->point.x == e2->point.x)
      return e1->point.y < e2->point.y ? -1 : 1;

    /* has the line segment associated to e1 been inserted
     * into S after the line segment associated to e2 ? */
    if (swevent_cmp(e1, e2) == 1)
      return SWEVENT_IS_ABOVE(e2, &e1->point) ? -1 : 1;

    /* The line segment associated to e2 has been inserted
     * into S after the line segment associated to e1 */
    return SWEVENT_IS_BELOW(e1, &e2->point) ? -1 : 1;
  }

  if (e1->subject == e2->subject)
  {
    /* same polygon */
    if (point2d_eq(&e1->point, &e2->point))
    {
      if (point2d_eq(&e1->otherEvent->point, &e2->otherEvent->point))
        return 0;
      else
       return e1->contourId > e2->contourId ? 1 : -1;
    }
  }
  else
  {
    /* Segments are collinear, but belong to separate polygons */
    return e1->subject ? -1 : 1;
  }

  return swevent_cmp(e1, e2) == 1 ? 1 : -1;
}

/*****************************************************************************
 * Queue
 * -----
 * To explain the various steps of the algorith we use the following example
 * polygons P and Q
 *
 *                     12------------11
 *    4---------3       |            |
 *    | 6-----7 |       |            |
 *    | |     | |       |            |
 *    | |     | |       |            |
 *    | 5-----8 |       |            |
 *    1---------2       |            |
 *                      9------------10
 *         P                    Q
 *
 * where the vertices are as follows
 *   p1 = (1,1)  p2  = (5,1)  p3  = (5,5)  p4  = (1,5)
 *   p5 = (2,2)  p6  = (2,4)  p7  = (4,4)  p8  = (4,2)
 *   p9 = (0,0)  p10 = (6,0)  p11 = (6,6)  p12 = (0,6)
 * and the result (marked with '*') is as follows
 *
 * 12************11    12------------11
 *  * 4--------3 *      | 4********3 |
 *  * | 6----7 | *      | * 6****7 * |
 *  * | |    | | *      | * *    * * |
 *  * | 5----8 | *      | * 5****8 * |
 *  * 1--------2 *      | 1********2 |
 *  9************10     9------------10
 *       Union           Intersection
 *
 * The first step of the algorithm create events for the left and right point
 * of each segment and introduces these events into a priority queue using
 * a lexicographic order, i.e., based on the order of x and then y, and for
 * edges sharing the same point, a vertical segment is after the other
 * segments. Each left/rigth event is connected to the other event of the
 * segment.
 *
 * The resulting queue after inserting the three contours is as follows, where
 * the for an event ei, i is the position on the queue, the first point is
 * the one of the event, and the second point is the one of the other event of
 * the segment
 * @code
 * e1 = (0,0) -> (6,0)
 * e2 = (0,0) -> (0,6)
 * e3 = (0,6) -> (0,0)
 * e4 = (0,6) -> (6,6)
 * e5 = (1,1) -> (5,1)
 * e6 = (1,1) -> (1,5)
 * e7 = (1,5) -> (1,1)
 * e8 = (1,5) -> (5,5)
 * e9 = (2,2) -> (4,2)
 * e10 = (2,2) -> (2,4)
 * e11 = (2,4) -> (2,2)
 * e12 = (2,4) -> (4,4)
 * e13 = (4,2) -> (2,2)
 * e14 = (4,2) -> (4,4)
 * e15 = (4,4) -> (4,2)
 * e16 = (4,4) -> (2,4)
 * e17 = (5,1) -> (1,1)
 * e18 = (5,1) -> (5,5)
 * e19 = (5,5) -> (5,1)
 * e20 = (5,5) -> (1,5)
 * e21 = (6,0) -> (0,0)
 * e22 = (6,0) -> (6,6)
 * e23 = (6,6) -> (6,0)
 * e24 = (6,6) -> (0,6)
 * @endcode
 *
 *****************************************************************************/

/**
 * @brief Insert the events associated to the points of a linear ring into the
 * priority queue
 */
static void
process_ring(POINTARRAY *contourOrHole, int depth, bool subject,
  bool isExteriorRing, PQueue *eventQueue)
{
  POINT2D *s1, *s2;
  SweepEvent *e1, *e2;
  for (uint32_t i = 0; i < contourOrHole->npoints - 1; i++)
  {
    s1 = (POINT2D *) getPoint_internal(contourOrHole, i);
    s2 = (POINT2D *) getPoint_internal(contourOrHole, i + 1);
    e1 = swevent_make(s1, false, NULL, subject, EDGE_NORMAL);
    e2 = swevent_make(s2, false, e1,   subject, EDGE_NORMAL);
    e1->otherEvent = e2;

    if (s1->x == s2->x && s1->y == s2->y)
      continue; /* skip collapsed edges, or it breaks */

    e1->contourId = e2->contourId = depth;
    if (! isExteriorRing)
    {
      e1->isExteriorRing = false;
      e2->isExteriorRing = false;
    }
    if (swevent_cmp(e1, e2) > 0)
      e2->left = true;
    else
      e1->left = true;

    /* Pushing it so the queue is sorted from left to right,
     * with object on the left having the highest priority. */
    pqueue_enqueue(eventQueue, e1);
    pqueue_enqueue(eventQueue, e2);
  }
}

/**
 * @brief Insert the events associated to the points of a (multi)polygon into
 * the priority queue
 */
static void
fill_queue(LWGEOM *geom, bool subject, PQueue *eventQueue)
{
  uint32_t type = lwgeom_get_type(geom);
  int npoly;
  LWPOLY *poly = NULL; /* make compiler quiet */
  LWMPOLY *multi = NULL; /* make compiler quiet */
  if (type == POLYGONTYPE)
  {
    npoly = 1;
    poly = lwgeom_as_lwpoly(geom);
  }
  else /* lwgeom_get_type(geom) == MULTIPOLYGONTYPE */
  {
    multi = lwgeom_as_lwmpoly(geom);
    npoly = multi->ngeoms;
  }
  /* Loop for every polygon */
  for (int i = 0; i < npoly; i++)
  {
    /* Find the i-th polygon if argument is a multipolygon */
    if (npoly > 1)
      poly = multi->geoms[i];
    int contourId = 0;
    /* Loop for every ring */
    for (uint32_t j = 0; j < poly->nrings; j++)
    {
      bool isExteriorRing = (j == 0);
      if (isExteriorRing) contourId++;
      process_ring(poly->rings[j], contourId, subject, isExteriorRing,
        eventQueue);
    }
  }
  return;
}

/*****************************************************************************
 * SweepLine
 * ---------
 * This step of the algorithm uses a sweepline and
 * - Filters events that are outside the bounding box of the result. In our
 *   example above, eigth events are removed leaving the following events
 *   @code
 *    [0]   [1]   [2]   [3]   [4]   [5]   [6]   [7]   [8]   [9]   [10]
 *   NULL  NULL  (0,6) (0,6) (1,1) (1,1) (1,5) (1,5) (2,3) (2,3) (3,2)
 *                 |     |     |     |     |     |     |     |     |
 *                 v     v     v     v     v     v     v     v     v
 *               (0,0) (6,6) (5,1) (1,5) (1,1) (5,5) (3,2) (3,4) (2,3)
 *
 *    [11]  [12]  [13]  [14]  [15]  [16]  [17]
 *   (3,2) (3,4) (3,4) (4,3) (4,3) (5,1) (5,1)
 *     |     |     |     |     |     |     |
 *     v     v     v     v     v     v     v
 *   (4,3) (2,3) (4,3) (3,2) (3,4) (1,1) (5,5)
 *   @endcode
 * - Divides segments that intersect or overlap. In our example above there are
 *   no intersections no events are subdivided.
 * - Computes internal fields of the event that are neede for the second step
 *   of the algorithm to construct the rings.
 *****************************************************************************/

/**
 * @brief Divide the event of a segment into two events and push them into the
 * queue
 */
static PQueue *
divide_segment(SweepEvent *e, POINT2D *p, PQueue *queue)
{
  assert(e->left);

  if (point2d_eq(&e->point, &e->otherEvent->point))
    elog(ERROR, "A collapsed segment cannot be in the result");

  /* Create e's right event */
  SweepEvent *r = swevent_make(p, false, e, e->subject, e->type);
  /* Create e->otherEvent's left event */
  SweepEvent *l = swevent_make(p, true,  e->otherEvent, e->subject,
    e->otherEvent->type);
  /* The new events belong to e's countour */
  r->contourId = l->contourId = e->contourId;

  /* Left event would be processed after the right event */
  if (swevent_cmp(l, e->otherEvent) > 0)
  {
    e->otherEvent->left = true;
      l->left = false;
  }

  /* Left event would be processed after the right event */
  // if (swevent_cmp(e, r) > 0) {}

  e->otherEvent->otherEvent = l;
  e->otherEvent = r;

  pqueue_enqueue(queue, l);
  pqueue_enqueue(queue, r);

  return queue;
}

/**
 * @brief Return the magnitude of the cross product of two vectors, pretending
 * they are in three dimensions
 * @param a,b Vectors
 */
static double crossProduct(POINT2D *a, POINT2D *b)
{
  return (a->x * b->y) - (a->y * b->x);
}

/**
 * @brief Return the dot product of two vectors.
 * @param a,b Vectors
 */
static double dotProduct(POINT2D *a, POINT2D *b)
{
  return (a->x * b->x) + (a->y * b->y);
}

/**
 * @brief Set a point from a parametric equation
 * @param[in] p Initial point
 * @param[in] s Value in [0,1]
 * @param[in] d Vector
 * @param[out] result Resulting point
 */
static inline void
setPoint(const POINT2D *p, double s, const POINT2D *d, POINT2D *result)
{
  result->x = p->x + s * d->x;
  result->y = p->y + s * d->y;
}

/**
 * @brief Find the intersection (if any) between two line segments a and b,
 * given the line segments' end points a1, a2 and b1, b2.
 * @details This algorithm is based on Schneider and Eberly, page 244
 * http://www.cimec.org.ar/~ncalvo/Schneider_Eberly.pdf
 * @param a1,a2 points of the first segment
 * @param b1,b2 points of the second segment
 * @param noEndpointTouch whether to skip single touchpoints (meaning
 * connected segments) as intersections
 * @param[out] p1,p2 Intersection points, if any
 * @returns If the segments intersect, return 1 and the point of intersection
 * in p1. If they overlap, return 2 and the two end points of the overlapping
 * segment in p1 and p2. Otherwise, return 0.
 */
static int
segment_intersection(POINT2D *a1, POINT2D *a2, POINT2D *b1, POINT2D *b2,
  bool noEndpointTouch, POINT2D *p1, POINT2D *p2)
{
  /* The algorithm expects our lines in the form P + sd, where P is a point,
   * s is on the interval [0, 1], and d is a vector.
   * We are passed two points. P can be the first point of each pair. The
   * vector, then, could be thought of as the distance (in x and y components)
   * from the first point to the second point.
   * So first, let's make our vectors:  */
  POINT2D va, vb;
  va.x = a2->x - a1->x;
  va.y = a2->y - a1->y;
  vb.x = b2->x - b1->x;
  vb.y = b2->y - b1->y;
  /* We also define a function to convert back to regular point form: */

  /* The rest is pretty much a straight port of the algorithm. */
  POINT2D e;
  e.x = b1->x - a1->x;
  e.y = b1->y - a1->y;
  double kross    = crossProduct(&va, &vb);
  double sqrKross = kross * kross;
  double sqrLenA  = dotProduct(&va, &va);
  //const sqrLenB  = dotProduct(vb, vb);

  /* Check for line intersection. This works because of the properties of the
   * cross product -- specifically, two vectors are parallel if and only if the
   * cross product is the 0 vector. The full calculation involves relative error
   * to account for possible very small line segments. See Schneider & Eberly
   * for details. */
  if (sqrKross > 0/* EPS * sqrLenB * sqLenA */)
  {
    /* If they're not parallel, then (because these are line segments) they
     * still might not actually intersect. This code checks that the
     * intersection point of the lines is actually on both line segments. */
    double s = crossProduct(&e, &vb) / kross;
    if (s < 0 || s > 1) {
      /* not on line segment a */
      return 0;
    }
    double t = crossProduct(&e, &va) / kross;
    if (t < 0 || t > 1) {
      /* not on line segment b */
      return 0;
    }
    if (s == 0 || s == 1)
    {
      /* on an endpoint of line segment a */
      if (noEndpointTouch) return 0;
      setPoint(a1, s, &va, p1);
      return 1;
    }
    if (t == 0 || t == 1)
    {
      /* on an endpoint of line segment b */
      if (noEndpointTouch) return 0;
      setPoint(b1, t, &vb, p1);
    return 1;
    }
    setPoint(a1, s, &va, p1);
    return 1;
  }

  /* If we've reached this point, then the lines are either parallel or the
   * same, but the segments could overlap partially or fully, or not at all.
   * So we need to find the overlap, if any. To do that, we can use e, which is
   * the (vector) difference between the two initial points. If this is parallel
   * with the line itself, then the two lines are the same line, and there will
   * be overlap.  */
  //const sqrLenE = dotProduct(e, e);
  kross = crossProduct(&e, &va);
  sqrKross = kross * kross;

  if (sqrKross > 0 /* EPS * sqLenB * sqLenE */) {
    /* Lines are just parallel, not the same. No overlap. */
    return 0;
  }

  double sa = dotProduct(&va, &e) / sqrLenA;
  double sb = sa + dotProduct(&va, &vb) / sqrLenA;
  double smin = fmin(sa, sb);
  double smax = fmax(sa, sb);

  /* this is, essentially, the FindIntersection acting on floats from
   * Schneider & Eberly, just inlined into this function.  */
  if (smin <= 1 && smax >= 0)
  {
    // overlap on an end point
    if (smin == 1)
    {
      if (noEndpointTouch) return 0;
      setPoint(a1, smin > 0 ? smin : 0, &va, p1);
      return 1;
    }

    if (smax == 0)
    {
      if (noEndpointTouch) return 0;
      setPoint(a1, smax < 1 ? smax : 1, &va, p1);
      return 1;
    }

    if (noEndpointTouch && smin == 0 && smax == 1)
      return 0;

    /* Segments overlap: two points of intersection. Return both */
    setPoint(a1, smin > 0 ? smin : 0, &va, p1);
    setPoint(a1, smax < 1 ? smax : 1, &va, p2);
    return 2;
  }

  return 0;
}

/**
 * @brief Divide the events of the two segments if they intersect
 * @details The function divides segments that cross or overlap and thus,
 * - replaces the 4 events of two segments a--x--b and c--x--d that intersect
 *   at a point x in the middle by 8 events a--x,  x--b, a--x, and x--b
 * - replaces the 2 events of a segment a--x--b that intersect at an endpoint
 *   of one segment c--d (say x = c) by a--c and c--b
 * - replaces the 4 events of two segments a--b and c--d that overlap as
 *   follows a--c==b--d by a--c, c--b, and b--d
 * - replaces the 4 events of two segments a--b and c--d that overlap as
 *   follows a/c====b--d by a--b, c--b, and b--d
 * @param e1,e2 Sweepline events
 * @param queue
 * @result Number of intersections of the segments associated to the events
 */
static int
possible_intersection(SweepEvent *e1, SweepEvent *e2, PQueue *queue)
{
  // The following disallows self-intersecting polygons,
  // did cost us half a day, so I'll leave it out of respect
  // if (e1->subject == e2->subject) return;
  POINT2D p1, p2;
  int nintersections = segment_intersection(&e1->point, &e1->otherEvent->point,
    &e2->point, &e2->otherEvent->point, false, &p1, &p2);

  if (nintersections == 0)
    return 0; /* no intersection */

  /* The line segments intersect at an endpoint of both line segments */
  if ((nintersections == 1) &&
      (point2d_eq(&e1->point, &e2->point) ||
       point2d_eq(&e1->otherEvent->point, &e2->otherEvent->point)))
    return 0;

  if (nintersections == 2 && e1->subject == e2->subject)
  {
    // if(e1->contourId == e2->contourId)
    //   elog(WARNING, 'Edges of the same polygon overlap',
    //     e1->point, e1->otherEvent->point, e2->point, e2->otherEvent->point);
    return 0;
  }

  /* The line segments associated to e1 and e2 intersect */
  if (nintersections == 1)
  {
    /* If the intersection point is not an endpoint of e1 */
    if (! point2d_eq(&e1->point, &p1) &&
        ! point2d_eq(&e1->otherEvent->point, &p1))
      divide_segment(e1, &p1, queue);

    /* If the intersection point is not an endpoint of e2 */
    if (! point2d_eq(&e2->point, &p1) &&
        ! point2d_eq(&e2->otherEvent->point, &p1))
      divide_segment(e2, &p1, queue);
    return 1;
  }

  /* The line segments associated to e1 and e2 overlap */
  SweepEvent *events[4] = {0};
  bool leftCoincide  = false;
  bool rightCoincide = false;
  int nevents = 0;

  if (point2d_eq(&e1->point, &e2->point))
    leftCoincide = true; // linked
  else if (swevent_cmp(e1, e2) == 1)
  {
    events[nevents++] = e2;
    events[nevents++] = e1;
  }
  else
  {
    events[nevents++] = e1;
    events[nevents++] = e2;
  }

  if (point2d_eq(&e1->otherEvent->point, &e2->otherEvent->point))
    rightCoincide = true;
  else if (swevent_cmp(e1->otherEvent, e2->otherEvent) == 1)
  {
    events[nevents++] = e2->otherEvent;
    events[nevents++] = e1->otherEvent;
  }
  else
  {
    events[nevents++] = e1->otherEvent;
    events[nevents++] = e2->otherEvent;
  }

  if ((leftCoincide && rightCoincide) || leftCoincide)
  {
    /* Both line segments are equal or share the left endpoint */
    e2->type = EDGE_NON_CONTRIBUTING;
    e1->type = (e2->inOut == e1->inOut) ?
      EDGE_SAME_TRANSITION : EDGE_DIFFERENT_TRANSITION;

    if (leftCoincide && ! rightCoincide)
    {
      /* honestly no idea, but changing events selection from [2, 1]
       * to [0, 1] fixes the overlapping self-intersecting polygons issue */
      divide_segment(events[1]->otherEvent, &events[0]->point, queue);
    }
    return 2;
  }

  /* The line segments share the right endpoint */
  if (rightCoincide)
  {
    divide_segment(events[0], &events[1]->point, queue);
    return 3;
  }

  /* No line segment includes totally the other one */
  if (events[0] != events[3]->otherEvent)
  {
    divide_segment(events[0], &events[1]->point, queue);
    divide_segment(events[1], &events[2]->point, queue);
    return 3;
  }

  /* One line segment includes the other one */
  divide_segment(events[0], &events[1]->point, queue);
  divide_segment(events[3]->otherEvent, &events[2]->point, queue);
  return 3;
}

/**
 * @brief Return true if the event is in the result of the operation
 */
static bool
in_result(SweepEvent *event, ClipOper oper)
{
  switch (event->type)
  {
    case EDGE_NORMAL:
      switch (oper)
      {
        case CL_INTERSECTION:
          return ! event->otherInOut;
        case CL_UNION:
          return event->otherInOut;
        case CL_DIFFERENCE:
          // return (event->subject && !event->otherInOut) ||
          //         (!event->subject && event->otherInOut);
          return (event->subject && event->otherInOut) ||
                  (! event->subject && ! event->otherInOut);
        case CL_XOR:
          return true;
      }
      break;
    case EDGE_SAME_TRANSITION:
      return oper == CL_INTERSECTION || oper == CL_UNION;
    case EDGE_DIFFERENT_TRANSITION:
      return oper == CL_DIFFERENCE;
    case EDGE_NON_CONTRIBUTING:
      return false;
  }
  return false;
}

/**
 * @brief
 */
static int
get_result_transition(SweepEvent *event, ClipOper oper)
{
  bool thisIn = ! event->inOut;
  bool thatIn = ! event->otherInOut;
  bool inside = false; /* make compiler quiet */
  switch (oper)
  {
    case CL_INTERSECTION:
      inside = thisIn && thatIn;
      break;
    case CL_UNION:
      inside = thisIn || thatIn;
      break;
    case CL_XOR:
      inside = thisIn ^ thatIn;
      break;
    case CL_DIFFERENCE:
      if (event->subject)
        inside = thisIn && ! thatIn;
      else
        inside = thatIn && ! thisIn;
      break;
  }
  return inside ? +1 : -1;
}

/**
 * @brief Compute the internal fields of the events preparing for the second
 * phase of the algorithm, that is, connecting the segments into contours
 * @param event,prev Sweepline events
 * @param oper Clipping operation
 */
static void
compute_fields(SweepEvent *event, SweepEvent *prev, ClipOper oper)
{
  /* compute inOut and otherInOut fields */
  if (prev == NULL)
  {
    event->inOut      = false;
    event->otherInOut = true;
  }
  else
  {
    /* Previous line segment in sweepline belongs to the same polygon */
    if (event->subject == prev->subject)
    {
      event->inOut      = ! prev->inOut;
      event->otherInOut = prev->otherInOut;
    }
    else
    {
      /* Previous line segment in sweepline belongs to the clipping polygon */
      event->inOut      = ! prev->otherInOut;
      event->otherInOut = SWEVENT_IS_VERTICAL(prev) ? ! prev->inOut : prev->inOut;
    }

    /* Compute prevInResult field */
    if (prev)
    {
      event->prevInResult =
        (! in_result(prev, oper) || SWEVENT_IS_VERTICAL(prev)) ?
        prev->prevInResult : prev;
    }
  }

  /* Check if the line segment belongs to the Boolean operation */
  bool inResult = in_result(event, oper);
  if (inResult)
    event->resultTransition = get_result_transition(event, oper);
  else
    event->resultTransition = 0;
  return;
}

/**
 * @brief Subdivide the segments of the event queue precomputing internal
 * fields needed for the second phase of the computation
 */
static Vector *
subdivide_segments(PQueue *eventQueue, GBOX *sbbox, GBOX *clbox, ClipOper oper)
{
  SplayTree sweepLine = splay_new(&segment_cmp);
  Vector *sortedEvents = vector_make(TYPE_BY_REF);
  double leftbound = fmax(sbbox->xmin, clbox->xmin);
  double rightbound = fmin(sbbox->xmax, clbox->xmax);
  SweepEvent *event, *prev, *next, *begin = NULL;
  /* loop for every event in the queue */
  while (eventQueue->length != 0)
  {
    /* Remove the event from the queue and insert it into the sweepline */
    event = pqueue_dequeue(eventQueue);
#if DEBUG_BUILD
    elog(WARNING, "Event:");
    swevent_print(event);
#endif

    /* Filter by leftbound of bounding boxes for intersection and difference */
    if ((oper == CL_INTERSECTION && event->point.x < leftbound) ||
        (oper == CL_DIFFERENCE   && event->point.x < sbbox->xmin))
    {
      if (event->left)
      {
        SweepEvent *other = event->otherEvent;
        if ((oper == CL_INTERSECTION && other->point.x < leftbound) ||
            (oper == CL_DIFFERENCE   && other->point.x < sbbox->xmin))
        {
          /* Mark the right event as deleted to do not add it to sortedEvents */
          other->deleted = true;
          pfree(event);
#if DEBUG_BUILD
          elog(WARNING, "Event deleted due to left bounding box test");
#endif
          continue;
        }
      }
      else
      {
        /* If the event was marked as deleted when processing its left event */
        if (event->deleted == true)
        {
          pfree(event);
#if DEBUG_BUILD
          elog(WARNING, "Right event deleted since its left event was previously deleted");
#endif
          continue;
        }
      }
    }

    /* Filter by rightbound of bounding boxes for intersection and difference */
    if ((oper == CL_INTERSECTION && event->point.x > rightbound) ||
        (oper == CL_DIFFERENCE   && event->point.x > sbbox->xmax))
    {
      if (! event->left && event->otherPos != - 1)
      {
        /* Remove the other event from sortedEvents */
        vector_delete(sortedEvents, event->otherPos);
        SweepEvent *other = event->otherEvent;
        pfree(other);
#if DEBUG_BUILD
          elog(WARNING, "Other event of current event deleted due to right bounding box test");
#endif
      }
      pfree(event);
#if DEBUG_BUILD
          elog(WARNING, "Event deleted due to right bounding box test");
          elog(WARNING, "Removing remaining events in the status line due to right bounding box test");
#endif
      /* Remove the remaining points in the sweepline */
      while (eventQueue->length > 0)
      {
        event = pqueue_dequeue(eventQueue);
        if (! event->left && event->otherPos != - 1)
        {
          /* Remove the other event from sortedEvents */
          vector_delete(sortedEvents, event->otherPos);
          SweepEvent *other = event->otherEvent;
          pfree(other);
        }
        pfree(event);
      }
      break;
    }

    /* Insert the event into the output events */
    int pos = vector_append(sortedEvents, PointerGetDatum(event));
    /* Mark the position where this event is stored in the other event */
    event->otherEvent->otherPos = pos;

    if (event->left)
    {
      next = prev = event;
      /* Insert the event into the sweepline */
      splay_insert(sweepLine, event);
      begin = splay_min(sweepLine);

      if (prev != begin)
        prev = splay_prev(sweepLine, prev);
      else
        prev = NULL;

      next = splay_next(sweepLine, next);
      compute_fields(event, prev, oper);

      #if DEBUG_BUILD
      elog(WARNING, "Status line after insertion:");
      SweepEvent *e = splay_root(sweepLine);
      while (e)
      {
        swevent_print(e);
        e = splay_next(sweepLine, e);
      }
      #endif

      if (next)
      {
        if (possible_intersection(event, next, eventQueue) == 2)
        {
          compute_fields(event, prev, oper);
          compute_fields(next, event, oper);
        }
      }

      if (prev)
      {
        if (possible_intersection(prev, event, eventQueue) == 2)
        {
          SweepEvent *prevprev = prev;
          if (prevprev != begin)
            prevprev = splay_prev(sweepLine, prevprev);
          else
            prevprev = NULL;

          compute_fields(prev,  prevprev, oper);
          compute_fields(event, prev,     oper);
        }
      }
    }
    else
    {
      event = event->otherEvent;
      next = prev = splay_search(sweepLine, event);

      if (prev && next)
      {
        if (prev != begin)
          prev = splay_prev(sweepLine, prev);
        else
          prev = NULL;

        next = splay_next(sweepLine, next);
        splay_delete(sweepLine, event);

        if (next && prev)
          possible_intersection(prev, next, eventQueue);
      }
    }
  }
  /* Free the sweepline
   * We arrive may here on a break after the bounding box test and thus before
   * all events of the sweepline have been shifted to sortedEvents.
   * We do not free events in the sweepline since they are freed either in
   * the eventQueue or in sortedEvents */
  splay_free(sweepLine);
  return sortedEvents;
}

/*****************************************************************************
 * Connect Edges
 *****************************************************************************/

/**
 * @brief Select the events that are in the result and order them
 * @param[in] sortedEvents Vector of sorted sweepline events
 * @param[out] count Number of elements in the output array
 * @return Array of events
 */
static SweepEvent **
order_events(Vector *sortedEvents, int *count)
{
  SweepEvent **resultEvents = palloc(sizeof(SweepEvent *) *
    sortedEvents->length);
  int nevents = 0;
  SweepEvent *event, *next;
  int i;
  for (i = 0; i < sortedEvents->length; i++)
  {
    event = DatumGetSweepEventP(vector_at(sortedEvents, i));
    /* N.B. The event may have been deleted due to the bounding box test */
    if (! event)
      continue;
    if ((event->left && SWEVENT_IN_RESULT(event)) ||
        (! event->left && SWEVENT_IN_RESULT(event->otherEvent)))
      resultEvents[nevents++] = event;
  }
  /* Due to overlapping edges the resultEvents array may be not wholly sorted */
  bool sorted = false;
  while (! sorted)
  {
    sorted = true;
    for (i = 0; i < nevents - 1; i++)
    {
      event = resultEvents[i];
      next = resultEvents[i + 1];
      if (swevent_cmp(event, next) == 1)
      {
        resultEvents[i] = next;
        resultEvents[i + 1] = event;
        sorted = false;
      }
    }
  }

  for (i = 0; i < nevents; i++)
  {
    event = resultEvents[i];
    event->otherPos = i;
  }

  /* imagine, the right event is found in the beginning of the queue,
   * when his left counterpart is not marked yet */
  for (i = 0; i < nevents; i++)
  {
    event = resultEvents[i];
    if (! event->left)
    {
      int tmp = event->otherPos;
      event->otherPos = event->otherEvent->otherPos;
      event->otherEvent->otherPos = tmp;
    }
  }
  *count = nevents;
  return resultEvents;
}

/**
 * @brief Return the number of the next position
 * @param resultEvents Array of sweepline events
 * @param count Number of elements in the input array
 * @param processed Array of Boolean values stating whether the event has been
 * already processed
 * @param pos Position in the event vector
 * @param origPos Original position in the event vector
 */
static int
next_position(SweepEvent **resultEvents, int count, bool *processed, int pos,
  int origPos)
{
  int newPos = pos + 1;
  SweepEvent *event = resultEvents[pos];
  POINT2D *p = &event->point, *p1;

  if (newPos < count)
  {
    event = resultEvents[newPos];
    p1 = &event->point;
  }

  while (newPos < count && point2d_eq(p1, p))
  {
    if (! processed[newPos])
      return newPos;
    else
      newPos++;
    if (newPos < count)
    {
      event = resultEvents[newPos];
      p1 = &event->point;
    }
  }

  newPos = pos - 1;
  while (processed[newPos] && newPos > origPos)
    newPos--;
  return newPos;
}

/**
 * @brief Create a contour from the argument contours
 */
static Contour *
initialize_contour(SweepEvent *event, Vector *contours, int contourId)
{
  Contour *c;
  Contour *contour = contour_make();
  if (event->prevInResult != NULL)
  {
    SweepEvent *prevInResult = event->prevInResult;
    /* Note that it is valid to query the "previous in result" for its output
     * contour id, because we must have already processed it (i.e., assigned an
     * output contour id) in an earlier iteration, otherwise it wouldn't be
     * possible that it is "previous in result". */
    int lowerContourId = prevInResult->outputContourId;
    int lowerResultTransition = prevInResult->resultTransition;
    if (lowerResultTransition > 0)
    {
      /* We are inside. Now we have to check if the thing below us is another
       * hole or an exterior contour. */
      Contour *lowerContour = DatumGetContourP(vector_at(contours, lowerContourId));
      if (lowerContour->holeOf != -1)
      {
        /* The lower contour is a hole => Connect the new contour as a hole
         * to its parent, and use same depth. */
        int parentContourId = lowerContour->holeOf;
        c = DatumGetContourP(vector_at(contours, parentContourId));
        vector_append(c->holeIds, Int32GetDatum(contourId));
        contour->holeOf = parentContourId;
        contour->depth = DatumGetContourP(vector_at(contours, lowerContourId))->depth;
      }
      else
      {
        /* The lower contour is an exterior contour => Connect the new contour
         * as a hole,and increment depth. */
        c = DatumGetContourP(vector_at(contours, lowerContourId));
        vector_append(c->holeIds, Int32GetDatum(contourId));
        contour->holeOf = lowerContourId;
        contour->depth = DatumGetContourP(vector_at(contours, lowerContourId))->depth + 1;
      }
    }
    else
    {
      /* We are outside => this contour is an exterior contour of same depth. */
      contour->holeOf = -1;
      contour->depth = DatumGetContourP(vector_at(contours, lowerContourId))->depth;
    }
  }
  else
  {
    /* There is no lower/previous contour => this contour is an exterior
     * contour of depth 0. */
    contour->holeOf = -1;
    contour->depth = 0;
  }
  return contour;
}

/**
 * @brief Connect the edges from the events into an vector of polygons
 * @param resultEvents Array of result events
 * @param count Number of elements in the array
 * @return Vector of contours
 */
static Vector *
connect_edges(SweepEvent **resultEvents, int count)
{
  /* false-filled vector */
  bool *processed = palloc(sizeof(bool) * count);
  for (int i = 0; i < count; i++)
    processed[i] = false;

  Vector *contours = vector_make(TYPE_BY_REF);
  for (int i = 0; i < count; i++)
  {
    if (processed[i])
      continue;

    int contourId = contours->length;
    SweepEvent *event = resultEvents[i];
    Contour *contour = initialize_contour(event, contours, contourId);

    int pos, origPos;
    pos = origPos = i;
    POINT4D p4d;
    point2d_set_point4d(&event->point, &p4d);
    ptarray_append_point(contour->points, &p4d, LW_TRUE);
    while (true)
    {
      /* Mark the event as processed */
      processed[pos] = true;
      SweepEvent *event = resultEvents[pos];
      if (pos < count && event)
        event->outputContourId = contourId;
      /* Mark the other event as processed */
      pos = event->otherPos;
      processed[pos] = true;
      event = resultEvents[pos];
      if (pos < count && event)
        event->outputContourId = contourId;
      /* Add the point to the point array */
      point2d_set_point4d(&event->point, &p4d);
      ptarray_append_point(contour->points, &p4d, LW_TRUE);
      pos = next_position(resultEvents, count, processed, pos, origPos);
      if (pos == origPos || pos >= count || ! event)
        break;
    }
    vector_append(contours, PointerGetDatum(contour));
  }
  pfree(processed);
  return contours;
}

/**
 * @brief Convert contours to polygons
 */
static GSERIALIZED *
contours_to_geom(Vector *contours, int32_t srid)
{
  int ncontours = contours->length;
  Contour *contour;
  LWGEOM **polygons = palloc(sizeof(LWGEOM *) * ncontours);
  int npoly = 0;
  for (int i = 0; i < ncontours; i++)
  {
    contour = DatumGetContourP(vector_at(contours, i));
    if (contour->holeOf == -1)
    {
      /* Create a new polygon */
      LWPOLY *poly = lwpoly_construct_empty(SRID_UNKNOWN, LW_FALSE, LW_FALSE);
      /* The exterior ring goes first */
      lwpoly_add_ring(poly, contour->points);
      /* Followed by holes if any */
      int nholes = contour->holeIds->length;
      for (int j = 0; j < nholes; j++)
      {
        int holeId = DatumGetInt32(vector_at(contour->holeIds, j));
        contour = DatumGetContourP(vector_at(contours, holeId));
        lwpoly_add_ring(poly, contour->points);
      }
      polygons[npoly++] = lwpoly_as_lwgeom(poly);
    }
  }

  /* Convert polygons to LWGEOM */
  LWGEOM *lwresult;
  if (npoly == 1)
  {
    /* Result is a LWPOLY */
    lwresult = polygons[0];
    lwgeom_set_srid(lwresult, srid);
    lwgeom_add_bbox(lwresult);
    pfree(polygons);
  }
  else
  {
    /* Result is a LWMPOLY */
    for (int i = 0; i < npoly; i++)
    {
      lwgeom_set_srid(polygons[i], srid);
      lwgeom_add_bbox(polygons[i]);
    }
    LWCOLLECTION *coll = lwcollection_construct(MULTIPOLYGONTYPE, srid, NULL,
      npoly, polygons);
    lwresult = lwcollection_as_lwgeom(coll);
    /* We cannot pfree(polygons) */
  }
  GSERIALIZED *result = geo_serialize(lwresult);
  lwgeom_free(lwresult);
  return result;
}

/*****************************************************************************/

/**
 * @brief Clip the two polygons using the operation
 */
GSERIALIZED *
clip_poly_poly(const GSERIALIZED *subj, const GSERIALIZED *clip, ClipOper oper)
{
  /* Trivial operation if at least one geometry is empty */
  bool empty_subj = gserialized_is_empty(subj);
  bool empty_clip = gserialized_is_empty(clip);
  if (empty_subj || empty_clip)
  {
    if (oper == CL_INTERSECTION)
      return NULL;
    else if (oper == CL_DIFFERENCE)
      return gserialized_copy(subj);
    else if (oper == CL_UNION || oper == CL_XOR)
      return empty_subj ? gserialized_copy(clip) : gserialized_copy(subj);
  }

  /* Trivial operation when the bounding boxes do not overlap */
  GBOX sbbox, clbox;
  memset(&sbbox, 0, sizeof(GBOX));
  memset(&clbox, 0, sizeof(GBOX));
  if (gserialized_get_gbox_p(subj, &sbbox) &&
      gserialized_get_gbox_p(clip, &clbox) &&
      gbox_overlaps_2d(&sbbox, &clbox) == LW_FALSE)
  {
    if (oper == CL_INTERSECTION)
      return NULL;
    else if (oper == CL_DIFFERENCE)
      return gserialized_copy(subj);
    else if (oper == CL_UNION || oper == CL_XOR)
      return gserialized_copy(clip);
  }

  /* Fill the event queue with the segments of both polygons */
  LWGEOM *subject = lwgeom_from_gserialized(subj);
  LWGEOM *clipping = lwgeom_from_gserialized(clip);
  PQueue *eventQueue = pqueue_make((qsort_comparator) &swevent_cmp);
  fill_queue(subject, true, eventQueue);
  fill_queue(clipping, false, eventQueue);

  /* Subdivide edges */
  Vector *sortedEvents = subdivide_segments(eventQueue, &sbbox, &clbox, oper);
  /* Free the priority queue */
  // TODO pfree(event) inside pqueue_free;
  pqueue_free(eventQueue);

  /* Select the result events */
  int nevents;
  SweepEvent **resultEvents = order_events(sortedEvents, &nevents);
  /* Free the sorted events */
  // TODO pfree(event) inside vector_free;
  vector_free(sortedEvents);

  /* Connect vertices */
  Vector *contours = connect_edges(resultEvents, nevents);
  int ncontours = contours->length;

  /* Convert contours to polygons */
  int32_t srid = gserialized_get_srid(subj);
  GSERIALIZED *result = contours_to_geom(contours, srid);

  for (int i = 0; i < ncontours; i++)
  {
    Contour *contour = DatumGetContourP(vector_at(contours, i));
    contour_free(contour);
  }
  vector_free(contours);
  lwgeom_free(subject);
  lwgeom_free(clipping);
  return result;
}

/*****************************************************************************/

