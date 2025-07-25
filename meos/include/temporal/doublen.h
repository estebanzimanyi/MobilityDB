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
 * @brief Internal types used in particular for computing the average and
 * centroid temporal aggregates.
 */

#ifndef __DOUBLEN_H__
#define __DOUBLEN_H__

/* C */
#include <stdbool.h>

/*****************************************************************************/

/**
 * Structure to represent values of the internal type for computing aggregates
 * for temporal number types
 */
typedef struct
{
  double a;
  double b;
} double2;

/**
 * Structure to represent values of the internal type for computing aggregates
 * for 2D temporal points
 */
typedef struct
{
  double a;
  double b;
  double c;
} double3;

/**
 * Structure to represent values of the internal type for computing aggregates
 * for 3D temporal points
 */
typedef struct
{
  double a;
  double b;
  double c;
  double d;
} double4;

/*****************************************************************************/

extern char *double2_out(const double2 *d, int maxdd);
extern void double2_set(double a, double b, double2 *result);
extern double2 *double2_add(const double2 *d1, const double2 *d2);
extern bool double2_eq(const double2 *d1, const double2 *d2);

extern char *double3_out(const double3 *d, int maxdd);
extern void double3_set(double a, double b, double c, double3 *result);
extern double3 *double3_add(const double3 *d1, const double3 *d2);
extern bool double3_eq(const double3 *d1, const double3 *d2);

extern char *double4_out(const double4 *d, int maxdd);
extern void double4_set(double a, double b, double c, double d, double4 *result);
extern double4 *double4_add(const double4 *d1, const double4 *d2);
extern bool double4_eq(const double4 *d1, const double4 *d2);

extern bool double2_collinear(const double2 *x1, const double2 *x2,
  const double2 *x3, double ratio);
extern bool double3_collinear(const double3 *x1, const double3 *x2,
  const double3 *x3, double ratio);
extern bool double4_collinear(const double4 *x1, const double4 *x2,
  const double4 *x3, double ratio);

extern double2 *double2segm_interpolate(const double2 *start,
  const double2 *end, long double ratio);
extern double3 *double3segm_interpolate(const double3 *start,
  const double3 *end, long double ratio);
extern double4 *double4segm_interpolate(const double4 *start,
  const double4 *end, long double ratio);

/*****************************************************************************/

#endif /* __DOUBLEN_H__ */
