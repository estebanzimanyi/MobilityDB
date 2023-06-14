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
 * @brief Clipping functions for temporal points.
 */

/* PostgreSQL */
#include <postgres.h>
#include <utils/float.h>
/* PostGIS */
#include <liblwgeom.h>
/* MEOS */
#include "general/temporal.h"
#include "point/splay_tree.h"
#include "point/stbox.h"
#include "point/tpoint.h"
#include "point/tpoint_clipping.h"

// #include <stdbool.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <math.h>
// #include <assert.h>
// #include <float.h>

/*****************************************************************************
 * Points
 *****************************************************************************/

/** Are the points equal ? */
bool
point_eq(const POINT2D *p1, const POINT2D *p2)
{
  return float8_eq(p1->x, p2->x) && float8_eq(p1->y, p2->y);
}


/** Sign of the signed area of the triangle(p0, p1, p2) */
int
signedArea(const POINT2D *p0, const POINT2D *p1, const POINT2D *p2)
{
  int res = (p0->x - p2->x)*(p1->y - p2->y) - (p1->x - p2->x) *(p0->y - p2->y);
  if (res > 0) return -1;
  if (res < 0) return 1;
  return 0;
}

/** Return -1 if the polygons are not empty, 0 if the result is empty,
 * 1 if the result is the subject, 2 if the result is the clipping */
// int
// trivialOperation(Polygon *subject, Polygon *clipping, ClipOpType operation)
// {
  // int ncontsubj = vec_cntr_size(&subject->contours);
  // int ncontclip = vec_cntr_size(&clipping->contours);

  // int result = -1;
  // if (ncontsubj * ncontclip == 0)
  // {
    // if (operation == CL_INTERSECTION)
      // result = 0;
    // else if (operation == CL_DIFFERENCE)
      // result = 1;
    // else if (operation == CL_UNION || operation == XOR)
      // result = (ncontsubj == 0) ? 2 : 1;
  // }
  // return result;
// }

/** Return -1 if the polygons are not empty, 0 if the result is empty,
 * 1 if the result is the subject, 2 if the result is the union of both */
// int
// compareBBoxes(Polygon *subject, Polygon *clipping, STBox *sbbox,
  // STBox *cbbox, ClipOpType operation)
// {
  // int result = -1;
  // if (sbbox->xmin > cbbox->xmax || cbbox->xmin > sbbox->xmax ||
      // sbbox->ymin > cbbox->ymax || cbbox->ymin > sbbox->ymax)
  // {
    // if (operation == INTERSECTION)
      // result = 0;
    // else if (operation == CL_DIFFERENCE)
      // result = 1;
    // else if (operation == CL_UNION || operation == XOR)
      // result = 2;
  // }
  // return result;
// }

/*****************************************************************************
 * SweepEvent
 *****************************************************************************/

SweepEvent *
swev_make(POINT2D *point, bool left, SweepEvent *otherEvent,
  bool isSubject, EdgeType edgeType)
{
  SweepEvent *result = malloc(sizeof(SweepEvent));
  result->left = left;
  result->point = *point;
  result->otherEvent = otherEvent;
  result->isSubject = isSubject;
  result->type = edgeType;
  /* Internal fields */
  result->inOut = false;
  result->otherInOut = false;
  result->prevInResult = NULL;
  result->resultTransition = 0;
  /* connection step */
  result->otherPos = -1;
  result->outputContourId = -1;
  result->isExteriorRing = true;
  return result;
}

SweepEvent
swev_copy(SweepEvent *e)
{
  SweepEvent *result = malloc(sizeof(SweepEvent));
  memcpy(result, e, sizeof(SweepEvent));
  return *result;
}

void
swev_free(SweepEvent *e)
{
  free(e);
  return;
}

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

bool
swev_isAbove(const SweepEvent *e, const POINT2D *p)
{
  return ! swev_isBelow(e, p);
}

bool
swev_isVertical(const SweepEvent *e)
{
  return e->point.x == e->otherEvent->point.x;
}

/**
 * Does event belong to result?
 */
bool
swev_inResult(const SweepEvent *e)
{
  return e->resultTransition != 0;
}

int
specialCases(const SweepEvent *e1, const SweepEvent *e2, const POINT2D *p1,
  const POINT2D *p2 __attribute__((unused)))
{
  // Same coordinates, but one is a left endpoint and the other is
  // a right endpoint. The right endpoint is processed first
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

int
compareSegments(const SweepEvent *e1, const SweepEvent *e2)
{
  if (e1 == e2) return 0;

  /* Segments are not collinear */
  if (signedArea(&e1->point, &e1->otherEvent->point, &e2->point) != 0 ||
    signedArea(&e1->point, &e1->otherEvent->point, &e2->otherEvent->point) != 0)
  {
    /* If they share their left endpoint use the right endpoint to sort */
    if (point_eq(&e1->point, &e2->point))
      return swev_isBelow(e1, &e2->otherEvent->point) ? -1 : 1;

    // Different left endpoint: use the left endpoint to sort
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
    if (p1->x == p2->x && p1->y == p2->y/*point_eq(e1->point, e2->point)*/)
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

pqu_SweepEvent *
divideSegment(SweepEvent *e, POINT2D *p, pqu_SweepEvent *queue)
{
  SweepEvent *r = swev_make(p, false, e, e->isSubject, EDGE_NORMAL);
  SweepEvent *l = swev_make(p, true,  e->otherEvent, e->isSubject, EDGE_NORMAL);

  if (point_eq(&e->point, &e->otherEvent->point))
    elog(WARNING, "what is that, a collapsed segment?");

  r->contourId = l->contourId = e->contourId;

  // avoid a rounding error. The left event would be processed after the right event
  if (swev_compare(l, e->otherEvent) > 0)
  {
    e->otherEvent->left = true;
    l->left = false;
  }

  // avoid a rounding error. The left event would be processed after the right event
  // if (compareEvents(e, r) > 0) {}

  e->otherEvent->otherEvent = l;
  e->otherEvent = r;

  pqu_swev_push(queue, *l);
  pqu_swev_push(queue, *r);

  return queue;
}


bool
inResult(SweepEvent *event, ClipOpType operation)
{
  switch (event->type)
  {
    case EDGE_NORMAL:
      switch (operation)
      {
        case CL_INTERSECTION:
          return !event->otherInOut;
        case CL_UNION:
          return event->otherInOut;
        case CL_DIFFERENCE:
          // return (event->isSubject && !event->otherInOut) ||
          //         (!event->isSubject && event->otherInOut);
          return (event->isSubject && event->otherInOut) ||
                  (!event->isSubject && !event->otherInOut);
        case CL_XOR:
          return true;
      }
      break;
    case EDGE_SAME_TRANSITION:
      return operation == CL_INTERSECTION || operation == CL_UNION;
    case EDGE_DIFFERENT_TRANSITION:
      return operation == CL_DIFFERENCE;
    case EDGE_NON_CONTRIBUTING:
      return false;
  }
  return false;
}

int
determineResultTransition(SweepEvent *event, ClipOpType operation)
{
  bool thisIn = ! event->inOut;
  bool thatIn = ! event->otherInOut;
  bool isIn = false; /* make compiler quiet */
  switch (operation)
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
 * @param  {SweepEvent} event
 * @param  {SweepEvent} prev
 * @param  {Operation} operation
 */
void
computeFields(SweepEvent *event, SweepEvent *prev, ClipOpType operation)
{
  // compute inOut and otherInOut fields
  if (prev == NULL)
  {
    event->inOut      = false;
    event->otherInOut = true;
  /* previous line segment in sweepline belongs to the same polygon */
  }
  else
  {
    if (event->isSubject == prev->isSubject)
    {
      event->inOut      = !prev->inOut;
      event->otherInOut = prev->otherInOut;

    /* previous line segment in sweepline belongs to the clipping polygon */
    }
    else
    {
      event->inOut      = !prev->otherInOut;
      event->otherInOut = swev_isVertical(prev) ?
        ! prev->inOut : prev->inOut;
    }

    /* compute prevInResult field */
    if (prev)
    {
      event->prevInResult =
        (! inResult(prev, operation) || swev_isVertical(prev)) ?
        prev->prevInResult : prev;
    }
  }

  // check if the line segment belongs to the Boolean operation
  bool isInResult = inResult(event, operation);
  if (isInResult)
    event->resultTransition = determineResultTransition(event, operation);
  else
    event->resultTransition = 0;
  return;
}

//const EPS = 1e-9;

/**
 * @brief Return the magnitude of the cross product of two vectors (if we
 * pretend they're in three dimensions)
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

void setPoint(const POINT2D *p, double s, const POINT2D *d, POINT2D *result)
{
  result->x = p->x + s * d->x;
  result->y = p->y + s * d->y;
}

/**
 * Finds the intersection (if any) between two line segments a and b, given the
 * line segments' end points a1, a2 and b1, b2.
 *
 * This algorithm is based on Schneider and Eberly.
 * http://www.cimec.org.ar/~ncalvo/Schneider_Eberly.pdf
 * Page 244.
 *
 * @param a1 point of first line
 * @param a2 point of first line
 * @param b1 point of second line
 * @param b2 point of second line
 * @param noEndpointTouch whether to skip single touchpoints (meaning
 * connected segments) as intersections
 * @returns If the lines intersect, return 1 and the point of intersection
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
 * @param  e1, e2 SweepEvent
 * @param  {Queue}      queue
 * @return {Number}
 */
int
possibleIntersection(SweepEvent *e1, SweepEvent *e2, pqu_SweepEvent *queue)
{
  // that disallows self-intersecting polygons,
  // did cost us half a day, so I'll leave it out of respect
  // if (e1->isSubject == e2->isSubject) return;
  POINT2D p1, p2;
  int nintersections = segment_intersection(&e1->point, &e1->otherEvent->point,
    &e2->point, &e2->otherEvent->point, false, &p1, &p2);

  if (nintersections == 0) return 0; /* no intersection */

  /* the line segments intersect at an endpoint of both line segments */
  if ((nintersections == 1) &&
      (point_eq(&e1->point, &e2->point) ||
       point_eq(&e1->otherEvent->point, &e2->otherEvent->point)))
    return 0;

  if (nintersections == 2 && e1->isSubject == e2->isSubject)
  {
    // if(e1->contourId == e2->contourId){
    // console.warn('Edges of the same polygon overlap',
    //   e1->point, e1->otherEvent->point, e2->point, e2->otherEvent->point);
    // }
    //throw new Error('Edges of the same polygon overlap');
    return 0;
  }

  /* The line segments associated to e1 and e2 intersect */
  if (nintersections == 1)
  {
    /* if the intersection point is not an endpoint of e1 */
    if (! point_eq(&e1->point, &p1) && !point_eq(&e1->otherEvent->point, &p1))
      divideSegment(e1, &p1, queue);

    /* if the intersection point is not an endpoint of e2 */
    if (!point_eq(&e2->point, &p1) && !point_eq(&e2->otherEvent->point, &p1))
      divideSegment(e2, &p1, queue);
    return 1;
  }

  /* The line segments associated to e1 and e2 overlap */
  SweepEvent *events[4] = {0};
  bool leftCoincide  = false;
  bool rightCoincide = false;

  if (point_eq(&e1->point, &e2->point))
    leftCoincide = true; // linked
  else if (swev_compare(e1, e2) == 1)
  {
    events[0] = e2;
    events[1] = e1;
  }
  else
  {
    events[0] = e1;
    events[1] = e2;
  }
  int nevents = 2;

  if (point_eq(&e1->otherEvent->point, &e2->otherEvent->point))
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
    /* both line segments are equal or share the left endpoint */
    e2->type = EDGE_NON_CONTRIBUTING;
    e1->type = (e2->inOut == e1->inOut) ?
      EDGE_SAME_TRANSITION : EDGE_DIFFERENT_TRANSITION;

    if (leftCoincide && !rightCoincide)
    {
      /* honestly no idea, but changing events selection from [2, 1]
       * to [0, 1] fixes the overlapping self-intersecting polygons issue */
      divideSegment(events[1]->otherEvent, &events[0]->point, queue);
    }
    return 2;
  }

  // the line segments share the right endpoint
  if (rightCoincide)
  {
    divideSegment(events[0], &events[1]->point, queue);
    return 3;
  }

  // no line segment includes totally the other one
  if (events[0] != events[3]->otherEvent)
  {
    divideSegment(events[0], &events[1]->point, queue);
    divideSegment(events[1], &events[2]->point, queue);
    return 3;
  }

  // one line segment includes the other one
  divideSegment(events[0], &events[1]->point, queue);
  divideSegment(events[3]->otherEvent, &events[2]->point, queue);
  return 3;
}

/*****************************************************************************
 * SweepLine
 *****************************************************************************/

// vec_swev
// subdivide(pqu_swev *eventQueue, vec_poly subject, vec_poly clipping,
  // STBox *sbbox, STBox *cbbox, ClipOpType operation)
 // {
  // SplayTree sweepLine = splay_new(&swev_compare);
  // vec_swev sortedEvents;

  // double rightbound = fmin(sbbox->xmax, cbbox->xmax);

  // SweepEvent *event, *prev, *next, *begin, *prevEvent, *prevprevEvent;

  // while (pqu_swev_size(eventQueue) != 0)
  // {
    // event = pqu_swev_pop(eventQueue);
    // sortedEvents.push(event);

    // /* optimization by bboxes for intersection and difference goes here */
    // if ((operation == CL_INTERSECTION && event->point.xmin > rightbound) ||
        // (operation == CL_DIFFERENCE   && event->point.xmin > sbbox->xmax))
      // break;

    // if (event->left)
    // {
      // next  = prev = splay_insert_value(sweepLine, event);
      // begin = splay_tree_minimum(sweepLine);

      // if (prev != begin) prev = sweepLine.prev(prev);
      // else                prev = NULL;

      // next = sweepLine.next(next);

      // prevEvent = prev ? prev->key : NULL;
      // computeFields(event, prevEvent, operation);
      // if (next)
      // {
        // if (possibleIntersection(event, next.key, eventQueue) == 2) {
          // computeFields(event, prevEvent, operation);
          // computeFields(next.key, event, operation);
        // }
      // }

      // if (prev)
      // {
        // if (possibleIntersection(prev.key, event, eventQueue) == 2) {
          // let prevprev = prev;
          // if (prevprev != begin) prevprev = sweepLine.prev(prevprev);
          // else                    prevprev = NULL;

          // prevprevEvent = prevprev ? prevprev.key : NULL;
          // computeFields(prevEvent, prevprevEvent, operation);
          // computeFields(event,     prevEvent,     operation);
        // }
      // }
    // }
    // else
    // {
      // event = event->otherEvent;
      // next = prev = sweepLine.find(event);

      // if (prev && next)
      // {

        // if (prev != begin) prev = sweepLine.prev(prev);
        // else                prev = null;

        // next = sweepLine.next(next);
        // sweepLine.remove(event);

        // if (next && prev)
          // possibleIntersection(prev.key, next.key, eventQueue);
      // }
    // }
  // }
  // return sortedEvents;
// }

// void processPolygon(vec_pt contourOrHole, bool isSubject, int depth,
  // pqu_swev eventQueue, STBox *bbox, bool isExteriorRing)
// {
  // int i, len;
  // POINT2D *s1, *s2;
  // SweepEvent *e1, *e2;
  // for (i = 0, len = contourOrHole.length - 1; i < len; i++)
  // {
    // s1 = contourOrHole[i];
    // s2 = contourOrHole[i + 1];
    // e1 = new SweepEvent(s1, false, undefined, isSubject);
    // e2 = new SweepEvent(s2, false, e1,        isSubject);
    // e1->otherEvent = e2;

    // if (s1[0] == s2[0] && s1[1] == s2[1]) {
      // continue; // skip collapsed edges, or it breaks
    // }

    // e1->contourId = e2->contourId = depth;
    // if (!isExteriorRing)
    // {
      // e1->isExteriorRing = false;
      // e2->isExteriorRing = false;
    // }
    // if (compareEvents(e1, e2) > 0)
      // e2->left = true;
    // else
      // e1->left = true;

    // double x = s1->x, y = s1->y;
    // bbox->xmin = Min(bbox->xmin, x);
    // bbox->ymax = Min(bbox->ymin, y);
    // bbox->xmax = Max(bbox->xmax, x);
    // bbox->ymax = Max(bbox->ymax, y);

    // /* Pushing it so the queue is sorted from left to right,
     // * with object on the left having the highest priority. */
    // eventQueue.push(e1);
    // eventQueue.push(e2);
  // }
// }

// pqu_swev
// fillQueue(vec_poly subject, vec_polyclipping, STBox *sbbox, STBox *cbbox,
  // ClipOpType operation)
// {
  // pqu_swev eventQueue = pqu_swev_init();
  // let polygonSet, isExteriorRing,
  // int i, ii, j, jj; //, k, kk;

  // for (i = 0, ii = subject.length; i < ii; i++)
  // {
    // polygonSet = subject[i];
    // for (j = 0, jj = polygonSet.length; j < jj; j++)
    // {
      // isExteriorRing = j == 0;
      // if (isExteriorRing) contourId++;
      // processPolygon(polygonSet[j], true, contourId, eventQueue, sbbox,
        // isExteriorRing);
    // }
  // }

  // for (i = 0, ii = clipping.length; i < ii; i++)
  // {
    // polygonSet = clipping[i];
    // for (j = 0, jj = polygonSet.length; j < jj; j++)
    // {
      // isExteriorRing = j == 0;
      // if (operation == CL_DIFFERENCE) isExteriorRing = false;
      // if (isExteriorRing) contourId++;
      // processPolygon(polygonSet[j], false, contourId, eventQueue, cbbox,
        // isExteriorRing);
    // }
  // }

  // return eventQueue;
// }

/*****************************************************************************/
