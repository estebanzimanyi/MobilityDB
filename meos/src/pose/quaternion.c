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

#include "pose/quaternion.h"

#include <math.h>
#include <float.h>

#include "temporal/doublen.h"
#include "temporal/temporal.h" /* For MEOS_EPSILON */

/*****************************************************************************
 * Constructor functions
 *****************************************************************************/

/**
 * @brief Costruct a quaternion from an axis and an angle
 * @param[in] axis Axis
 * @param[in] theta Angle
 */
Quaternion
quaternion_from_axis_angle(double3 axis, double theta)
{
  axis = vec3_normalize(axis);
  double sin_theta_2 = sin(theta / 2);
  double W = cos(theta / 2);
  double X = axis.a * sin_theta_2;
  double Y = axis.b * sin_theta_2;
  double Z = axis.c * sin_theta_2;
  return (Quaternion) {W, X, Y, Z};
}

/*****************************************************************************
 * Math functions
 *****************************************************************************/

/**
 * @brief Return the norm a quaternion
 * @param[in] q Quaternion
 */
double
quaternion_norm(Quaternion q)
{
  return sqrt(q.W*q.W + q.X*q.X + q.Y*q.Y + q.Z*q.Z);
}

/**
 * @brief Return a quaternion normalized
 * @param[in] q Quaternion
 */
Quaternion
quaternion_normalize(Quaternion q)
{
  double norm = quaternion_norm(q);
  return (Quaternion) {q.W / norm, q.X / norm, q.Y / norm, q.Z / norm};
}

/**
 * @brief Return a quaternion negated
 * @param[in] q Quaternion
 */
Quaternion
quaternion_negate(Quaternion q)
{
  return (Quaternion) {-q.W, -q.X, -q.Y, -q.Z};
}

/**
 * @brief Return a quaternion inversed
 * @param[in] q Quaternion
 */
Quaternion
quaternion_invert(Quaternion q)
{
  return (Quaternion) {q.W, -q.X, -q.Y, -q.Z};
}

/**
 * @brief Return the dot product of two quaternions
 * @param[in] q1,q2 Quaternions
 */
double
quaternion_dot(Quaternion q1, Quaternion q2)
{
  return q1.W*q2.W + q1.X*q2.X + q1.Y*q2.Y + q1.Z*q2.Z;
}

/**
 * @brief Return true if the quaternions are equal
 * @param[in] q1,q2 Quaternions
 */
bool
quaternion_eq(Quaternion q1, Quaternion q2)
{
  return (q1.W == q2.W && q1.X == q2.X && q1.Y == q2.Y && q1.Z == q2.Z);
}

/**
 * @brief Return true if the quaternions are equal up to an epsilon value
 * @param[in] q1,q2 Quaternions
 */
bool
quaternion_same(Quaternion q1, Quaternion q2)
{
  return (fabs(q1.W - q2.W) < MEOS_EPSILON &&
    fabs(q1.X - q2.X) < MEOS_EPSILON && fabs(q1.Y - q2.Y) < MEOS_EPSILON &&
    fabs(q1.Z - q2.Z) < MEOS_EPSILON);
}

/**
 * @brief Return the addition of two quaternions
 * @param[in] q1,q2 Quaternions
 */
Quaternion
quaternion_add(Quaternion q1, Quaternion q2)
{
  return (Quaternion) {q1.W + q2.W, q1.X + q2.X, q1.Y +q2.Y, q1.Z + q2.Z};
}

/**
 * @brief Return the difference of two quaternions
 * @param[in] q1,q2 Quaternions
 */
Quaternion
quaternion_diff(Quaternion q1, Quaternion q2)
{
  return (Quaternion) {q1.W - q2.W, q1.X - q2.X, q1.Y - q2.Y, q1.Z - q2.Z};
}

/**
 * @brief Return the multiplication of two quaternions
 * @param[in] q1,q2 Quaternions
 */
Quaternion
quaternion_multiply(Quaternion q1, Quaternion q2)
{
  double W = q1.W * q2.W - q1.X * q2.X - q1.Y * q2.Y - q1.Z * q2.Z;
  double X = q1.W * q2.X + q1.X * q2.W + q1.Y * q2.Z - q1.Z * q2.Y;
  double Y = q1.W * q2.Y - q1.X * q2.Z + q1.Y * q2.W + q1.Z * q2.X;
  double Z = q1.W * q2.Z + q1.X * q2.Y - q1.Y * q2.X + q1.Z * q2.W;
  return (Quaternion) {W, X, Y, Z};
}

/**
 * @brief Return a quaternion multiplied by a scalar
 * @param[in] q Quaternion
 * @param[in] s Scalar
 */
Quaternion
quaternion_multiply_scalar(Quaternion q, double s)
{
  return (Quaternion) {q.W * s, q.X *s, q.Y * s, q.Z * s};
}

/**
 * @brief Return the distance between two quaternions (treated as 4D vectors)
 * @param[in] q1,q2 Quaternions
 */
double
quaternion_distance(Quaternion q1, Quaternion q2)
{
  return fabs(q1.W - q2.W) + fabs(q1.X - q2.X) + fabs(q1.Y - q2.Y) +
    fabs(q1.Z - q2.Z);
}

/**
 * @brief Return the SLERP (Spherical Linear Interpolation) of two quaternions,
 * giving the shortest, smoothest path between them on the unit hypersphere
 * @param[in] q1,q2 Quaternions
 * @param[in] ratio Ratio
 */
Quaternion
quaternion_slerp(Quaternion q1, Quaternion q2, double ratio)
{
  q1 = quaternion_normalize(q1);
  q2 = quaternion_normalize(q2);

  /* Hemisphere correction */
  double dot = quaternion_dot(q1, q2);
  if (dot < 0.0f)
  {
    q2 = quaternion_negate(q2);
    dot = -dot;
  }

  const double DOT_THRESHOLD = 0.9995;
  if (dot > DOT_THRESHOLD)
  {
    Quaternion result = quaternion_add(q1,
      quaternion_multiply_scalar(quaternion_diff(q2, q1), ratio));
    return quaternion_normalize(result);
  }

  double theta_0 = acos(dot);
  double theta = theta_0*ratio;
  double sin_theta = sin(theta);
  double sin_theta_0 = sin(theta_0);

  double s1 = cos(theta) - dot * sin_theta / sin_theta_0;
  double s2 = sin_theta / sin_theta_0;

  Quaternion result = quaternion_add(quaternion_multiply_scalar(q1, s1),
    quaternion_multiply_scalar(q2, s2));

  return quaternion_normalize(result);
}

/**
 * @brief Return a float in [0,1] representing the location of the given
 * quaternion on the quaternion segment, as a fraction of the segment length
 * using SLERP
 * @param[in] q1,q2 Quaternion defining the segment
 * @param[in] q Quaternion to locate
 * @param[in] geodetic True when using spherical interpolation (SLERP), 
 * false for linear interpolation (LERP)
 */
double
quaternion_locate(Quaternion q1, Quaternion q2, Quaternion q, bool geodetic)
{
  /* Normalize all three */
  q1 = quaternion_normalize(q1);
  q2 = quaternion_normalize(q2);
  q  = quaternion_normalize(q);

  /* Hemisphere correction */
  double dot12 = quaternion_dot(q1, q2);
  if (dot12 < 0.0)
  {
    q2 = quaternion_negate(q2);
    dot12 = -dot12;
  }

  double dot1q = quaternion_dot(q1, q);
  if (dot1q < 0.0)
  {
    q = quaternion_negate(q);
    dot1q = -dot1q;
  }

  /* If q1 and q2 are nearly identical, the only valid q is q1 */
  if (fabs(dot12 - 1.0) < MEOS_EPSILON)
  {
    if (fabs(quaternion_dot(q1, q) - 1.0) < MEOS_EPSILON)
      return 0.0;  /* only one point on the path */
    return -1.0;
  }


  /* Compute angles */
  double theta0 = acos(dot12);   /* full arc */
  double theta  = acos(dot1q);   /* arc from q1 -> q */

  /* Compute t */
  double t = theta / theta0;

  /* Must lie in [0,1] (with tolerance) */
  if (t < -MEOS_EPSILON || t > 1.0 + MEOS_EPSILON)
      return -1.0;

  /* Reconstruct expected quaternion and compare */
  Quaternion expected = quaternion_slerp(q1, q2, t);

  double diff = fabs(expected.W - q.W) + fabs(expected.X - q.X) +
    fabs(expected.Y - q.Y) + fabs(expected.Z - q.Z);
  if (diff < 1e-5)
    return t;

  return -1.0;  /* not on the curve */
}

/*****************************************************************************/

/**
 * @brief Return a fraction between [0,1] at which two unit quaternions q1 and
 * q2 intersect two unit quaternions q3 and q4 and returns -1 if they do not
 * intersect
 * @param[in] q1,q2 Quaternions defining the first segment
 * @param[in] q3,q4 Quaternions defining the second segment
 * @param[in] geodetic True when using spherical interpolation (SLERP), 
 * false for linear interpolation (LERP)
 */
double
quaternion_intersection(Quaternion q1, Quaternion q2, Quaternion q3,
  Quaternion q4, bool geodetic)
{
  const int MAX_ITERS = 80;
  double lo = 0.0;
  double hi = 1.0;

  /* Evaluate endpoints: if they match here, return immediately */
  Quaternion a0 = quaternion_slerp(q1, q2, lo);
  Quaternion b0 = quaternion_slerp(q3, q4, lo);
  if (quaternion_distance(a0, b0) < MEOS_EPSILON)
    return 0.0;
  Quaternion a1 = quaternion_slerp(q1, q2, hi);
  Quaternion b1 = quaternion_slerp(q3, q4, hi);
  if (quaternion_distance(a1, b1) < MEOS_EPSILON)
    return 1.0;

  /* Bisection search over t ∈ [0,1] */
  for (int i = 0; i < MAX_ITERS; i++)
  {
    double mid = (lo + hi) / 2.0;
    Quaternion qa = quaternion_slerp(q1, q2, mid);
    Quaternion qb = quaternion_slerp(q3, q4, mid);
    double d = quaternion_distance(qa, qb);
    if (d < MEOS_EPSILON)
      return mid;

    /* Evaluate sign-like criterion:
       Compare distances slightly left and right of mid */
    double tleft  = mid - 1e-4; if (tleft < 0) tleft = 0;
    double tright = mid + 1e-4; if (tright > 1) tright = 1;

    double dl = quaternion_distance(interp_func(q1, q2, tleft),
      interp_func(q3, q4, tleft));
    double dr = quaternion_distance(interp_func(q1, q2, tright),
      interp_func(q3, q4, tright));

    /* If the left distance is smaller, the root is left */
    if (dl < dr)
      hi = mid;
    else
      lo = mid;
  }
  /* No intersection found */
  return -1.0; 
}

/*****************************************************************************/
