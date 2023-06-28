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

#ifndef __TPOINT_CLIPPING_H__
#define __TPOINT_CLIPPING_H__

/* PostgreSQL */
#include <postgres.h>
/* PostGIS */
#include <liblwgeom.h>
/* MEOS */
#include "general/temporal.h"
#include "point/tpoint.h"
#include "point/vector.h"

/*****************************************************************************/

typedef enum
{
  CL_INTERSECTION = 0,
  CL_UNION        = 1,
  CL_DIFFERENCE   = 2,
  CL_XOR          = 3,
} ClipOper;

typedef enum
{
  EDGE_NORMAL               = 0,
  EDGE_NON_CONTRIBUTING     = 1,
  EDGE_SAME_TRANSITION      = 2,
  EDGE_DIFFERENT_TRANSITION = 3,
} EdgeType;

/*****************************************************************************
 * Vector data structure
 *****************************************************************************/

typedef struct
{
  POINTARRAY *points;  /**< Points conforming the contour */
  /** Holes of the contour, stored as indexes of a vector of contours */
  Vector *holeIds;
  int holeOf;
  int depth;     /**< Depth, i.e., number of enclosing contours */
  bool external; /**< is the contour an external contour (i.e., not a hole)? */
} Contour;

/*****************************************************************************/

typedef struct SweepEvent SweepEvent;

struct SweepEvent
{
  POINT2D point;   /**< Point of the event */
  bool left;       /**< Is the event a left event? */
  SweepEvent *otherEvent; /**< Event of the other point of the segment */
  bool subject;  /**< Does the event belong to the subject polygon? */
  bool inside;   /**< Is the segment inside the polygon? */
  bool deleted;    /**< Has the event been deleted? */
  EdgeType type;   /**< Edge contribution type */
  bool inOut;  /** Is the segment an inside-outside transition into polygon? */
  bool otherInOut; /** Is the closest edge downwards in the sweepline that
                  belongs to the other polygon an inside-outside transition? */
  SweepEvent *prevInResult; /**< Previous event in result */
  int resultTransition; /**< Type of result transition (0 = not in result,
                             +1 = out-in, -1 = in-out) */
  int otherPos;        /**< Position of the other event in the vector */
  int contourId;       /**< Contour id */
  int outputContourId; /**< Output contour id */
  bool isExteriorRing; /**< Does the event belongs to the exterior ring? */
};

/*****************************************************************************/

/* Vector store Datums in order to differentiate elements passed by value and
 * by reference */
#define DatumGetContourP(X)    ((Contour *) DatumGetPointer(X))
#define DatumGetSweepEventP(X) ((SweepEvent *) DatumGetPointer(X))

extern GSERIALIZED *clip_poly_poly(const GSERIALIZED *subj,
  const GSERIALIZED *clip, ClipOper operation);

/*****************************************************************************/

#endif


