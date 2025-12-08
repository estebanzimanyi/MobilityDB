/*****************************************************************************
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
 * @brief Quaternion functions
 */

#ifndef __QUATERNION_H__
#define __QUATERNION_H__

#include <postgres.h>
#include "temporal/doublen.h"

/*****************************************************************************
 * Struct definitions
 *****************************************************************************/

/* Quaternion */

typedef struct
{
  double     W;
  double     X;
  double     Y;
  double     Z;
} Quaternion;

/*****************************************************************************/

/* Constructor functions */

extern Quaternion quaternion_from_axis_angle(double3 axis, double theta);

/* Math functions */

extern double quaternion_norm(Quaternion q);

extern Quaternion quaternion_normalize(Quaternion q);
extern Quaternion quaternion_negate(Quaternion q);
extern Quaternion quaternion_invert(Quaternion q);
extern double quaternion_dot(Quaternion q1, Quaternion q2);
extern bool quaternion_eq(Quaternion q1, Quaternion q2);
extern bool quaternion_same(Quaternion q1, Quaternion q2);

extern Quaternion quaternion_add(Quaternion q1, Quaternion q2);
extern Quaternion quaternion_diff(Quaternion q1, Quaternion q2);
extern Quaternion quaternion_multiply(Quaternion q1, Quaternion q2);
extern Quaternion quaternion_multiply_scalar(Quaternion q, double s);
extern double quaternion_distance(Quaternion q1, Quaternion q2);
extern Quaternion quaternion_slerp(Quaternion q1, Quaternion q2, double ratio);
extern Quaternion quaternion_lerp(Quaternion q1, Quaternion q2, double ratio);
extern Quaternion quaternion_pow(Quaternion q, double fraction);
extern double quaternion_locate(Quaternion q1, Quaternion q2, Quaternion q,
  bool geodetic);
extern double quaternion_intersection(Quaternion q1, Quaternion q2,
  Quaternion q3, Quaternion q4, bool geodetic);

/*****************************************************************************/

#endif /* __QUATERNION_H__ */
