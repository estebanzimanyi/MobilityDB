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

/*****************************************************************************
 * Points
 *****************************************************************************/

/**
 * @brief Are the points equal ?
 */
bool
point2d_eq(const POINT2D *p1, const POINT2D *p2)
{
  return float8_eq(p1->x, p2->x) && float8_eq(p1->y, p2->y);
}

/**
 * @brief Sign of the signed area of the triangle(p0, p1, p2)
 */
int
signedArea(const POINT2D *p0, const POINT2D *p1, const POINT2D *p2)
{
  int res = (p0->x - p2->x)*(p1->y - p2->y) - (p1->x - p2->x) *(p0->y - p2->y);
  if (res > 0) return -1;
  if (res < 0) return 1;
  return 0;
}

/*****************************************************************************
 * Contour
 *****************************************************************************/

/**
 * @brief Create a new countour
 */
Contour *
cntr_make(void)
{
  Contour *result = palloc0(sizeof(Contour));
  result->points = vector_byref_make();
  result->holeIds = vector_byvalue_make();
  result->holeOf = -1;
  result->depth = -1;
  return result;
}

/**
 * @brief Free a contour
 */
void
cntr_free(Contour *c)
{
  vector_free(c->points);
  vector_free(c->holeIds);
  pfree(c);
  return;
}

/*****************************************************************************
 * Polygon
 *****************************************************************************/

/**
 * @brief Create a new polygon
 */
Polygon *
poly_make(void)
{
  Polygon *result = palloc0(sizeof(Polygon));
  result->contours = vector_byref_make();
  return result;
}

/**
 * @brief Free a polygon
 */
void
poly_free(Polygon *p)
{
  vector_free(p->contours);
  pfree(p);
  return;
}

/*****************************************************************************
 * SweepEvent
 *****************************************************************************/

/**
 * @brief Create a new sweepline event
 */
SweepEvent *
swev_make(POINT2D *point, bool left, SweepEvent *otherEvent, bool isSubject,
  EdgeType edgeType)
{
  SweepEvent *result = palloc0(sizeof(SweepEvent));
  result->left = left;
  result->point = *point;
  result->otherEvent = otherEvent;
  result->isSubject = isSubject;
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
 * @brief Return true if the sweepline event is below the point
 */
bool
swev_isBelow(const SweepEvent *e, const POINT2D *p)
{
  const POINT2D *p0 = &e->point, *p1 = &e->otherEvent->point;
  return e->left ?
    (p0->x - p->x) * (p1->y - p->y) - (p1->x - p->x) * (p0->y - p->y) > 0 :
    // signedArea(e->point, e->otherEvent.point, p) > 0 :
    (p1->x - p->x) * (p0->y - p->y) - (p0->x - p->x) * (p1->y - p->y) > 0;
    //signedArea(e->otherEvent.point, e->point, p) > 0;
}

/**
 * @brief Return true if the sweepline event is above the point
 */
bool
swev_isAbove(const SweepEvent *e, const POINT2D *p)
{
  return ! swev_isBelow(e, p);
}

/**
 * @brief Return true if the sweepline event is vertical
 */
bool
swev_isVertical(const SweepEvent *e)
{
  return e->point.x == e->otherEvent->point.x;
}

/**
 * @brief Does event belong to result?
 */
bool
swev_inResult(const SweepEvent *e)
{
  return e->resultTransition != 0;
}

/**
 * @brief
 */
int
specialCases(const SweepEvent *e1, const SweepEvent *e2, const POINT2D *p1,
  const POINT2D *p2 __attribute__((unused)))
{
  /* Same coordinates, but one is a left endpoint and the other is
   * a right endpoint. The right endpoint is processed first */
  if (e1->left != e2->left)
    return e1->left ? 1 : -1;

  // const p2 = e1->otherEvent.point, p3 = e2->otherEvent.point;
  // const sa = (p1->x - p3->x) * (p2->y - p3->y) - (p2->x - p3->x) * (p1->y - p3->y)
  /* Same coordinates, both events are left endpoints or right endpoints.
   * not collinear */
  if (signedArea(p1, &e1->otherEvent->point, &e2->otherEvent->point) != 0)
  {
    /* the event associate to the bottom segment is processed first */
    return (! swev_isBelow(e1, &e2->otherEvent->point)) ? 1 : -1;
  }

  return (! e1->isSubject && e2->isSubject) ? 1 : -1;
}

/**
 * @brief Comparison of sweepline events
 */
int
swev_compare(const SweepEvent *e1, const SweepEvent *e2)
{
  const POINT2D *p1 = &e1->point;
  const POINT2D *p2 = &e2->point;

  /* Different x-coordinate */
  if (p1->x > p2->x) return 1;
  if (p1->x < p2->x) return -1;

  /* Different points, but same x-coordinate
   * Event with lower y-coordinate is processed first */
  if (p1->y != p2->y) return p1->y > p2->y ? 1 : -1;

  return specialCases(e1, e2, p1, p2);
}

/**
 * @brief Comparison of segments associated to two sweepline events
 */
int
compareSegments(SweepEvent *e1, SweepEvent *e2)
{
  if (e1 == e2) return 0;

  /* Segments are not collinear */
  if (signedArea(&e1->point, &e1->otherEvent->point, &e2->point) != 0 ||
    signedArea(&e1->point, &e1->otherEvent->point, &e2->otherEvent->point) != 0)
  {
    /* If they share their left endpoint use the right endpoint to sort */
    if (point2d_eq(&e1->point, &e2->point))
      return swev_isBelow(e1, &e2->otherEvent->point) ? -1 : 1;

    /* Different left endpoint: use the left endpoint to sort */
    if (e1->point.x == e2->point.x)
      return e1->point.y < e2->point.y ? -1 : 1;

    /* has the line segment associated to e1 been inserted
     * into S after the line segment associated to e2 ? */
    if (swev_compare(e1, e2) == 1)
      return swev_isAbove(e2, &e1->point) ? -1 : 1;

    /* The line segment associated to e2 has been inserted
     * into S after the line segment associated to e1 */
    return swev_isBelow(e1, &e2->point) ? -1 : 1;
  }

  if (e1->isSubject == e2->isSubject)
  {
    /* same polygon */
    const POINT2D *p1 = &e1->point;
    const POINT2D *p2 = &e2->point;
    if (p1->x == p2->x && p1->y == p2->y/*point2d_eq(e1->point, e2->point)*/)
    {
      p1 = &e1->otherEvent->point;
      p2 = &e2->otherEvent->point;
      if (p1->x == p2->x && p1->y == p2->y)
        return 0;
      else
       return e1->contourId > e2->contourId ? 1 : -1;
    }
  }
  else
  {
    /* Segments are collinear, but belong to separate polygons */
    return e1->isSubject ? -1 : 1;
  }

  return swev_compare(e1, e2) == 1 ? 1 : -1;
}

/*****************************************************************************
 * Queue
 * -----
 * To explain the various steps of the algorith we use the following example
 * polygons P and Q
 *
 *                         3-----------------4
 *    3-----------8        |                 |
 *    |           |        |                 |
 *    |  6-----7  |        |                 |
 *    |  |     |  |        |                 |
 *    |  |     |  |        |                 |
 *    |  4-----5  |        |                 |
 *    |           |        |                 |
 *    1-----------2        |                 |
 *                         1-----------------2
 *         P                       Q
 *
 *  3*****************4      3-----------------4
 *  *  3-----------8  *      |  3***********8  |
 *  *  |           |  *      |  *           *  |
 *  *  |  6*****7  |  *      |  *  6*****7  *  |
 *  *  |  *     *  |  *      |  *  |     |  *  |
 *  *  |  *     *  |  *      |  *  |     |  *  |
 *  *  |  4*****5  |  *      |  *  4*****5  *  |
 *  *  |           |  *      |  *           *  |
 *  *  1-----------2  *      |  1***********2  |
 *  1*****************2      1-----------------2
 *       Union               Intersection
 *
 * The first step of the algorithm create sweepline events for the left and
 * right point of each segment and introduces these events into a priority
 * queue using a lexicographic order, i.e., based on the order of x and
 * then y, and for edges sharing the same initial point, a vertical segment is
 * after the other segments. Each left/rigth event is connected to the other
 * event of the segment.
 *
 * The resulting queue after inserting the three contours is as follows, where
 * the number between brackets is the position on the queue, the top point is
 * the one of the event, and the bottom point is the one of the other event of
 * the segment
 * @code
 *  [0]   [1]   [2]   [3]   [4]   [5]   [6]   [7]   [8]   [9]   [10]  [11] ...
 * (0,0) (0,0) (0,6) (0,6) (1,1) (1,1) (5,5) (1,5) (2,3) (1,5) (3,4) (2,3) ...
 *   |     |     |     |     |     |     |     |     |     |     |     |   ...
 *   v     v     v     v     v     v     v     v     v     v     v     v   ...
 * (6,0) (0,6) (6,6) (0,0) (1,5) (5,1) (1,5) (1,1) (3,2) (5,5) (4,3) (3,4) ...
 *
 *  [12]  [13]  [14]  [15]  [16]  [17]  [18]  [19]  [20]  [21]  [22]  [23]
 * (6,6) (6,0) (6,0) (5,1) (5,5) (3,2) (3,2) (4,3) (5,1) (4,3) (6,6) (3,4)
 *   |     |     |     |     |     |     |     |     |     |     |     |
 *   v     v     v     v     v     v     v     v     v     v     v     v
 * (6,0) (6,6) (0,0) (1,1) (5,1) (2,3) (4,3) (3,2) (5,5) (3,4) (0,6) (2,3)
 * @endcode
 *****************************************************************************/

/**
 * @brief Insert the points of a linear ring into the priority queue
 */
void
processRing(POINTARRAY *contourOrHole, int depth, bool isSubject,
  bool isExteriorRing, PQueue *eventQueue)
{
  POINT2D *s1, *s2;
  SweepEvent *e1, *e2;
  for (uint32_t i = 0; i < contourOrHole->npoints - 1; i++)
  {
    s1 = (POINT2D *) getPoint_internal(contourOrHole, i);
    s2 = (POINT2D *) getPoint_internal(contourOrHole, i + 1);
    e1 = swev_make(s1, false, NULL, isSubject, EDGE_NORMAL);
    e2 = swev_make(s2, false, e1,   isSubject, EDGE_NORMAL);
    e1->otherEvent = e2;

    if (s1->x == s2->x && s1->y == s2->y)
      continue; /* skip collapsed edges, or it breaks */

    e1->contourId = e2->contourId = depth;
    if (! isExteriorRing)
    {
      e1->isExteriorRing = false;
      e2->isExteriorRing = false;
    }
    if (swev_compare(e1, e2) > 0)
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
 * @brief Insert the points of a (multi)polygon into the priority queue
 */
void
fill_queue(LWGEOM *geom, bool isSubject, PQueue *eventQueue)
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
      processRing(poly->rings[j], contourId, isSubject, isExteriorRing,
        eventQueue);
    }
  }
  return;
}

/*****************************************************************************
 * Divide segments
 * ---------------
 * The second step of the algorithm divides segments that cross or overlap.
 * In our example polygons above there are no intersections and no events are
 * subdivided.
 * In addition to this, this step computes internal fields of the event that
 * are necessary for the second step of the algorithm to construct the rings.
 * In our example above, four events are removed because they going beyond
 * the rightbound of the intersection of the bounding boxes leavind the
 * following events
 * @code
 [0]   [1]   [2]   [3]   [4]   [5]   [6]   [7]   [8]   [9]   [10]
(0,0) (0,0) (0,6) (0,6) (1,1) (1,1) (1,5) (1,5) (2,3) (2,3) (3,2)
  |     |     |     |     |     |     |     |     |     |     |
  v     v     v     v     v     v     v     v     v     v     v
(6,0) (0,6) (0,0) (6,6) (5,1) (1,5) (1,1) (5,5) (3,2) (3,4) (2,3)

 [11]  [12]  [13]  [14]  [15]  [16]  [17]  [18]  [19]
(3,2) (3,4) (3,4) (4,3) (4,3) (5,1) (5,1) (5,5) (5,5)
  |     |     |     |     |     |     |     |     |
  v     v     v     v     v     v     v     v     v
(4,3) (2,3) (4,3) (3,2) (3,4) (1,1) (5,5) (5,1) (1,5)
 * @endcode
 *****************************************************************************/

/**
 * @brief Divide the event of a segment into two events and push them into the
 * queue
 */
PQueue *
divideSegment(SweepEvent *e, POINT2D *p, PQueue *queue)
{
  SweepEvent *r = swev_make(p, false, e, e->isSubject, EDGE_NORMAL);
  SweepEvent *l = swev_make(p, true,  e->otherEvent, e->isSubject, EDGE_NORMAL);

  if (point2d_eq(&e->point, &e->otherEvent->point))
    elog(ERROR, "A collapsed segment cannot be in the result");

  r->contourId = l->contourId = e->contourId;

  /* Avoid a rounding error: left event would be processed after the right event */
  if (swev_compare(l, e->otherEvent) > 0)
  {
    e->otherEvent->left = true;
    l->left = false;
  }

  /* Avoid a rounding error: left event would be processed after the right event */
  // if (swev_compare(e, r) > 0) {}

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
double crossProduct(POINT2D *a, POINT2D *b)
{
  return (a->x * b->y) - (a->y * b->x);
}

/**
 * @brief Return the dot product of two vectors.
 * @param a,b Vectors
 */
double dotProduct(POINT2D *a, POINT2D *b)
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
void setPoint(const POINT2D *p, double s, const POINT2D *d, POINT2D *result)
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
int
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
int
possibleIntersection(SweepEvent *e1, SweepEvent *e2, PQueue *queue)
{
  // The following disallows self-intersecting polygons,
  // did cost us half a day, so I'll leave it out of respect
  // if (e1->isSubject == e2->isSubject) return;
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

  if (nintersections == 2 && e1->isSubject == e2->isSubject)
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
    if (! point2d_eq(&e1->point, &p1) && !point2d_eq(&e1->otherEvent->point, &p1))
      divideSegment(e1, &p1, queue);

    /* If the intersection point is not an endpoint of e2 */
    if (!point2d_eq(&e2->point, &p1) && !point2d_eq(&e2->otherEvent->point, &p1))
      divideSegment(e2, &p1, queue);
    return 1;
  }

  /* The line segments associated to e1 and e2 overlap */
  SweepEvent *events[4] = {0};
  bool leftCoincide  = false;
  bool rightCoincide = false;
  int nevents = 0;

  if (point2d_eq(&e1->point, &e2->point))
    leftCoincide = true; // linked
  else if (swev_compare(e1, e2) == 1)
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
  else if (swev_compare(e1->otherEvent, e2->otherEvent) == 1)
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
      divideSegment(events[1]->otherEvent, &events[0]->point, queue);
    }
    return 2;
  }

  /* The line segments share the right endpoint */
  if (rightCoincide)
  {
    divideSegment(events[0], &events[1]->point, queue);
    return 3;
  }

  /* No line segment includes totally the other one */
  if (events[0] != events[3]->otherEvent)
  {
    divideSegment(events[0], &events[1]->point, queue);
    divideSegment(events[1], &events[2]->point, queue);
    return 3;
  }

  /* One line segment includes the other one */
  divideSegment(events[0], &events[1]->point, queue);
  divideSegment(events[3]->otherEvent, &events[2]->point, queue);
  return 3;
}

/*****************************************************************************
 * SweepLine
 *****************************************************************************/

/**
 * @brief Return true if the event is in the result of the operation
 */
bool
inResult(SweepEvent *event, ClipOper oper)
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
          // return (event->isSubject && !event->otherInOut) ||
          //         (!event->isSubject && event->otherInOut);
          return (event->isSubject && event->otherInOut) ||
                  (! event->isSubject && ! event->otherInOut);
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
int
determineResultTransition(SweepEvent *event, ClipOper oper)
{
  bool thisIn = ! event->inOut;
  bool thatIn = ! event->otherInOut;
  bool isIn = false; /* make compiler quiet */
  switch (oper)
  {
    case CL_INTERSECTION:
      isIn = thisIn && thatIn;
      break;
    case CL_UNION:
      isIn = thisIn || thatIn;
      break;
    case CL_XOR:
      isIn = thisIn ^ thatIn;
      break;
    case CL_DIFFERENCE:
      if (event->isSubject)
        isIn = thisIn && ! thatIn;
      else
        isIn = thatIn && ! thisIn;
      break;
  }
  return isIn ? +1 : -1;
}

/**
 * @brief Compute the internal fields of the events preparing for the second
 * phase of the algorithm, that is, connecting the segments into contours
 * @param event,prev Sweepline events
 * @param oper Clipping operation
 */
void
computeFields(SweepEvent *event, SweepEvent *prev, ClipOper oper)
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
    if (event->isSubject == prev->isSubject)
    {
      event->inOut      = !prev->inOut;
      event->otherInOut = prev->otherInOut;
    }
    else
    {
      /* Previous line segment in sweepline belongs to the clipping polygon */
      event->inOut      = ! prev->otherInOut;
      event->otherInOut = swev_isVertical(prev) ? ! prev->inOut : prev->inOut;
    }

    /* Compute prevInResult field */
    if (prev)
    {
      event->prevInResult =
        (! inResult(prev, oper) || swev_isVertical(prev)) ?
        prev->prevInResult : prev;
    }
  }

  /* Check if the line segment belongs to the Boolean operation */
  bool isInResult = inResult(event, oper);
  if (isInResult)
    event->resultTransition = determineResultTransition(event, oper);
  else
    event->resultTransition = 0;
  return;
}

/**
 * @brief Subdivide the segments of the event queue precomputing internal
 * fields needed for the second phase of the computation
 */
Vector *
subdivideSegments(PQueue *eventQueue, GBOX *sbbox, GBOX *clbox, ClipOper oper)
{
  SplayTree sweepLine = splay_new(&compareSegments);
  Vector *sortedEvents = vector_byref_make();
  double rightbound = fmin(sbbox->xmax, clbox->xmax);
  SweepEvent *event, *prev, *next, *begin = NULL, *prevEvent, *prevprevEvent;
  /* loop for every event in the queue */
  while (eventQueue->length != 0)
  {
    /* Remove the event from the queue and insert it into the sweepline */
    event = pqueue_dequeue(eventQueue);

    /* Optimization by bounding boxes for intersection and difference */
    if ((oper == CL_INTERSECTION && event->point.x > rightbound) ||
        (oper == CL_DIFFERENCE   && event->point.x > sbbox->xmax))
    {
      // TODO pfree(event);
      break;
    }

    /* Insert the event into the sweepline */
    vector_append(sortedEvents, PointerGetDatum(event));

    if (event->left)
    {
      next = prev = event;
      splay_insert(sweepLine, event);
      begin = splay_min(sweepLine);

      if (prev != begin)
        prev = splay_prev(sweepLine, prev);
      else
        prev = NULL;

      next = splay_next(sweepLine, next);

      prevEvent = prev ? prev : NULL;
      computeFields(event, prevEvent, oper);
      if (next)
      {
        if (possibleIntersection(event, next, eventQueue) == 2)
        {
          computeFields(event, prevEvent, oper);
          computeFields(next, event, oper);
        }
      }

      if (prev)
      {
        if (possibleIntersection(prev, event, eventQueue) == 2)
        {
          SweepEvent *prevprev = prev;
          if (prevprev != begin)
            prevprev = splay_prev(sweepLine, prevprev);
          else
            prevprev = NULL;

          prevprevEvent = prevprev ? prevprev : NULL;
          computeFields(prevEvent, prevprevEvent, oper);
          computeFields(event,     prevEvent,     oper);
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
          possibleIntersection(prev, next, eventQueue);
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
 * @param sortedEvents Vector of sorted sweepline events
 * @return Vector of events
 */
Vector *
orderEvents(Vector *sortedEvents)
{
  Vector *resultEvents = vector_byref_make();
  SweepEvent *event, *next;
  int i;
  for (i = 0; i < sortedEvents->length; i++)
  {
    event = DatumGetSweepEventP(vector_at(sortedEvents, i));
    /* N.B. event->resultTransition != 0 is the definition of event.inResult */
    if ((event->left && event->resultTransition != 0) ||
        (! event->left && event->otherEvent->resultTransition != 0))
      vector_append(resultEvents, PointerGetDatum(event));
  }
  /* Due to overlapping edges the resultEvents array may be not wholly sorted */
  bool sorted = false;
  while (! sorted)
  {
    sorted = true;
    for (i = 0; i < resultEvents->length - 1; i++)
    {
      event = DatumGetSweepEventP(vector_at(resultEvents, i));
      next = DatumGetSweepEventP(vector_at(resultEvents, i + 1));
      if (swev_compare(event, next) == 1)
      {
        vector_set(resultEvents, i, PointerGetDatum(next));
        vector_set(resultEvents, i + 1, PointerGetDatum(event));
        sorted = false;
      }
    }
  }

  for (i = 0; i < resultEvents->length; i++)
  {
    event = DatumGetSweepEventP(vector_at(resultEvents, i));
    event->otherPos = i;
  }

  /* imagine, the right event is found in the beginning of the queue,
   * when his left counterpart is not marked yet */
  for (i = 0; i < resultEvents->length; i++)
  {
    event = DatumGetSweepEventP(vector_at(resultEvents, i));
    if (! event->left)
    {
      int tmp = event->otherPos;
      event->otherPos = event->otherEvent->otherPos;
      event->otherEvent->otherPos = tmp;
    }
  }
  return resultEvents;
}

/**
 * @brief Return the number of the next position
 * @param resultEvents Vector of sweepline events
 * @param processed Vector of Boolean values stating whether the event has been
 * already processed
 * @param pos Position in the event vector
 * @param origPos Original position in the event vector
 */
int nextPos(Vector *resultEvents, Vector *processed, int pos, int origPos)
{
  int newPos = pos + 1;
  SweepEvent *event = DatumGetSweepEventP(vector_at(resultEvents, pos));
  POINT2D *p = &event->point, *p1;

  if (newPos < resultEvents->length)
  {
    event = DatumGetSweepEventP(vector_at(resultEvents, newPos));
    p1 = &event->point;
  }

  while (newPos < resultEvents->length && p1->x == p->x && p1->y == p->y)
  {
    if (! DatumGetBool(vector_at(processed, newPos)))
      return newPos;
    else
      newPos++;
    if (newPos < resultEvents->length)
    {
      event = DatumGetSweepEventP(vector_at(resultEvents, newPos));
      p1 = &event->point;
    }
  }

  newPos = pos - 1;
  while (DatumGetBool(vector_at(processed, newPos)) && newPos > origPos)
    newPos--;
  return newPos;
}

/**
 * @brief Create a contour from the argument contours
 */
Contour *
initializeContourFromContext(SweepEvent *event, Vector *contours,
  int contourId)
{
  Contour *c;
  Contour *contour = cntr_make();
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
 * @brief Helper macro that combines marking an event as processed with
 * assigning its output contour ID
 */
#define MARK_AS_PROCESSED(pos) \
  do { \
    vector_set(processed, (pos), BoolGetDatum(true)); \
    SweepEvent *event = DatumGetSweepEventP(vector_at(resultEvents, (pos))); \
    if ((pos) < resultEvents->length && event) \
      event->outputContourId = contourId; \
  } while(0)

/**
 * @brief Connect the edges from the events into an vector of polygons
 * @param resultEvents Vector of result events
 * @return Vector of contours
 */
Vector *
connectEdges(Vector *resultEvents)
{
  /* "false"-filled vector */
  Vector *processed = vector_byvalue_make();
  for (int i = 0; i < resultEvents->length; i++)
    vector_append(processed, BoolGetDatum(false));

  Vector *contours = vector_byref_make();
  for (int i = 0; i < resultEvents->length; i++)
  {
    if (DatumGetBool(vector_at(processed, i)))
      continue;

    int contourId = contours->length;
    SweepEvent *event = DatumGetSweepEventP(vector_at(resultEvents, i));
    Contour *contour = initializeContourFromContext(event, contours, contourId);

    int pos = i;
    int origPos = i;
    POINT2D *initial = &event->point;
    vector_append(contour->points, PointerGetDatum(initial));
    while (true)
    {
      MARK_AS_PROCESSED(pos);
      event = DatumGetSweepEventP(vector_at(resultEvents, pos));
      pos = event->otherPos;
      MARK_AS_PROCESSED(pos);
      vector_append(contour->points, PointerGetDatum(&event->point));
      pos = nextPos(resultEvents, processed, pos, origPos);
      if (pos == origPos || pos >= resultEvents->length || ! event)
        break;
    }
    vector_append(contours, PointerGetDatum(contour));
  }
  return contours;
}

/*****************************************************************************/

/**
 * @brief Contruct a POINTARRAY from a vector of POINT2D
 */
POINTARRAY *
ptvec_to_ptarray(Vector *points)
{
  int len = points->length;
  POINTARRAY *result = ptarray_construct(0, 0, len);
  /* The two first points are equal, the first point should be the last one */
  for (int i = 0; i < len; i++)
  {
    POINT2D *pt = DatumGetPoint2DP(vector_at(points, (i + 1) % len));
    uint8_t *ptr = getPoint_internal(result, i);
    memcpy(ptr, pt, sizeof(POINT2D));
  }
  return result;
}

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
  PQueue *eventQueue = pqueue_make((qsort_comparator) &swev_compare);
  fill_queue(subject, true, eventQueue);
  fill_queue(clipping, false, eventQueue);

  /* Subdivide edges */
  Vector *sortedEvents = subdivideSegments(eventQueue, &sbbox, &clbox, oper);
  /* Free the priority queue */
  // TODO pfree(event) inside pqueue_free;
  pqueue_free(eventQueue);

  /* Select the result events */
  Vector *resultEvents = orderEvents(sortedEvents);
  /* Free the sorted events */
  // TODO pfree(event) inside vector_free;
  vector_free(sortedEvents);

  /* Connect vertices */
  Vector *contours = connectEdges(resultEvents);
  int len = contours->length;

  /* Convert contours to polygons */
  Vector *polygons = vector_byref_make();
  for (int i = 0; i < len; i++)
  {
    Contour *contour = DatumGetContourP(vector_at(contours, i));
    if (contour->holeOf == -1)
    {
      /* Create a new polygon */
      Polygon *poly = poly_make();
      /* The exterior ring goes first */
      vector_append(poly->contours, PointerGetDatum(contour));
      /* Followed by holes if any */
      int nholes = contour->holeIds->length;
      for (int j = 0; j < nholes; j++)
      {
        int holeId = DatumGetInt32(vector_at(contour->holeIds, j));
        contour = DatumGetContourP(vector_at(contours, holeId));
        vector_append(poly->contours, PointerGetDatum(contour));
      }
      vector_append(polygons, PointerGetDatum(poly));
    }
  }

  /* Convert polygons to LWGEOM */
  uint32_t npoly = polygons->length;
  int32_t srid = gserialized_get_srid(subj);
  LWGEOM *lwresult;
  LWPOLY *poly;
  if (npoly == 1)
  {
    /* Result is a LWPOLY */
    Polygon *p = DatumGetPolygonP(vector_at(polygons, 0));
    int nrings = p->contours->length;
    POINTARRAY **ptarrs = palloc(sizeof(POINTARRAY *) * nrings);
    for (int j = 0; j < nrings; j++)
    {
      Contour *c = DatumGetContourP(vector_at(p->contours, j));
      ptarrs[j] = ptvec_to_ptarray(c->points);
    }
    poly = lwpoly_construct(srid, NULL, nrings, ptarrs);
    lwresult = lwpoly_as_lwgeom(poly);
  }
  else
  {
    /* Result is a LWMPOLY */
    LWGEOM **geoms = palloc(sizeof(LWGEOM *) * npoly);
    for (uint32_t i = 0; i < npoly; i++)
    {
      Polygon *p = DatumGetPolygonP(vector_at(polygons, i));
      uint32_t nrings = p->contours->length;
      POINTARRAY **ptarrs = palloc(sizeof(POINTARRAY *) * nrings);
      for (uint32_t j = 0; j < nrings; j++)
      {
        Contour *c = DatumGetContourP(vector_at(p->contours, j));
        ptarrs[j] = ptvec_to_ptarray(c->points);
      }
      poly = lwpoly_construct(srid, NULL, nrings, ptarrs);
      geoms[i] = lwpoly_as_lwgeom(poly);
    }
    LWCOLLECTION *coll = lwcollection_construct(MULTIPOLYGONTYPE, srid, NULL,
      npoly, geoms);
    lwresult = lwcollection_as_lwgeom(coll);
  }

  GSERIALIZED *result = geo_serialize(lwresult);
  lwgeom_free(subject);
  lwgeom_free(clipping);
  lwgeom_free(lwresult);
  return result;
}

/*****************************************************************************/

