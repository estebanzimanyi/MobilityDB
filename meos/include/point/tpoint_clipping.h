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

#ifndef __TPOINT_CLIPPING_H__
#define __TPOINT_CLIPPING_H__

/* PostgreSQL */
#include <postgres.h>
/* PostGIS */
#include <liblwgeom.h>
/* MEOS */
#include "general/temporal.h"
#include "point/tpoint.h"

/*****************************************************************************/

typedef enum {
  CL_INTERSECTION = 0,
  CL_UNION        = 1,
  CL_DIFFERENCE   = 2,
  CL_XOR          = 3,
} ClipOpType;

typedef enum {
  EDGE_NORMAL               = 0,
  EDGE_NON_CONTRIBUTING     = 1,
  EDGE_SAME_TRANSITION      = 2,
  EDGE_DIFFERENT_TRANSITION = 3,
} EdgeType;

/*****************************************************************************/

// typedef struct
// {
  // double x, y;
// } Point;

// typedef struct
// {
  // double xmin, ymin, xmax, ymax;
// } BBOX;

// typedef struct
// {
  // /** Segment endpoints */
  // Point p1, p2;
// } Segment;

/* Definition of vector of Points */

#define vec_POINT2D vec_pt
#define POD
#define NOT_INTEGRAL
#define T POINT2D
#include <ctl/vector.h>

/* Definition of vector of integers */

#define POD
#define T int
#include <ctl/vector.h>

typedef struct
{
  /** Set of points conforming the external contour */
  vec_pt points;
  /** Holes of the contour. They are stored as the indexes of the holes in a polygon class */
  vec_int holeIds;
  int holeOf;
  int depth;
  bool external; // is the contour an external contour? (i.e., is it not a hole?)
  bool precomputedCC;
  bool CC;
} Contour;

/* Definition of vector of Contour */

void cntr_free(Contour *);
Contour cntr_copy(Contour *);

#define Contour_free cntr_free
#define Contour_copy cntr_copy
#define vec_Contour vec_cntr
#define T Contour
#include <ctl/vector.h>

typedef struct
{
  vec_cntr contours;
} Polygon;

/* Function prototypes for Polygon */

void poly_free(Polygon *);
Polygon poly_copy(Polygon *);
int poly_cmp(Polygon *a, Polygon *b);

#define Polygon_free poly_free
#define Polygon_copy poly_copy
#define Polygon_cmp poly_cmp

/* Definition of vector of Polygon */

#define vec_Polygon vec_poly
#define T Polygon
#include <ctl/vector.h>

/*****************************************************************************/

typedef struct SweepEvent SweepEvent;

struct SweepEvent
{
  POINT2D point;  /**< Point associated with the event */
  bool left;    /**< Is left endpoint? */
  SweepEvent *otherEvent; /**< Event associated to the other endpoint of the segment */
  bool isSubject;  /**< Belongs to source or clipping polygon */
  EdgeType type;  /**< Edge contribution type */
  /* Internal fields */
  /** In-out transition for the sweepline crossing polygon */
  bool inOut;
  bool otherInOut;
  SweepEvent *prevInResult; /**< Previous event in result? */
  /** Type of result transition (0 = not in result, +1 = out-in, -1, in-out) */
  int resultTransition;
  int otherPos;
  int contourId; /**< Contour id */
  int outputContourId; /**< Output contour id */
  bool isExteriorRing; /**< Is the exterior ring ? */
};

extern GSERIALIZED *clip_poly_poly(const GSERIALIZED *subj,
  const GSERIALIZED *clip, ClipOpType operation);

/*****************************************************************************/

#endif


