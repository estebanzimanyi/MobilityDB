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
 * @brief Utilities for manipulating temporal rigid geometries
 */

#include "rgeo/trgeo_boxops.h"

/* C */
#include <assert.h>
/* PostgreSQL */
#include <postgres.h>
/* PostGIS */
#include <liblwgeom.h>
/* MEOS */
#include <meos.h>
#include <meos_rgeo.h>
#include "geo/postgis_funcs.h"
#include "pose/pose.h"
#include "pose/quaternion.h"
#include "rgeo/lwgeom_utils.h"
#include "rgeo/trgeo_inst.h"

/*****************************************************************************/

/**
 * @brief Ensure that two rings of areference polyhedral surfaces are the same
 * @param[in] poly1,poly2 Polygons
 */
static void
ensure_same_rings_lwpoly(const LWPOLY *poly1, const LWPOLY *poly2)
{
  if (poly1->nrings != poly2->nrings)
    meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
      "Operation on different reference geometries");
  for (int i = 0; i < (int) poly1->nrings; ++i)
    if (poly1->rings[i]->npoints != poly2->rings[i]->npoints)
      meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
        "Operation on different reference geometries");
}

/**
 * @brief Ensure that two reference polyhedral surfaces are the same
 * @param[in] psurf1,psurf2 Polyhedral surfaces
 */
static void
ensure_same_geoms_lwpsurface(const LWPSURFACE *psurf1,
  const LWPSURFACE *psurf2)
{
  if (psurf1->ngeoms != psurf2->ngeoms)
    meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
      "Operation on different reference geometries");
  for (int i = 0; i < (int) psurf1->ngeoms; ++i)
    ensure_same_rings_lwpoly(psurf1->geoms[i], psurf2->geoms[i]);
}

/**
 * @brief Ensure that two reference geometries are the same
 * @param[in] geom1,geom2 Geometries
 */
static bool
same_lwgeom(const LWGEOM *geom1, const LWGEOM *geom2)
{
  LWPOINTITERATOR *it1 = lwpointiterator_create(geom1);
  LWPOINTITERATOR *it2 = lwpointiterator_create(geom2);
  POINT4D p1;
  POINT4D p2;

  bool result = true;
  while (lwpointiterator_next(it1, &p1) && lwpointiterator_next(it2, &p2) &&
    result)
  {
    if (FLAGS_GET_Z(geom1->flags))
    {
      result = fabs(p1.x - p2.x) < MEOS_EPSILON &&
        fabs(p1.y - p2.y) < MEOS_EPSILON && fabs(p1.z - p2.z) < MEOS_EPSILON;
    }
    else
    {
      result = fabs(p1.x - p2.x) < MEOS_EPSILON &&
        fabs(p1.y - p2.y) < MEOS_EPSILON;
    }
  }
  lwpointiterator_destroy(it1);
  lwpointiterator_destroy(it2);
  return result;
}

/**
 * @brief Ensure that two reference geometries are the same
 * @param[in] gs1,gs2 Geometries
 */
bool
ensure_same_geom(const GSERIALIZED *gs1, const GSERIALIZED *gs2)
{
  if (gs1 == gs2)
    return true;

  if (gserialized_get_type(gs1) != gserialized_get_type(gs2))
  {
    meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
      "Operation on different reference geometries");
    return false;
  }

  LWGEOM *geom1 = lwgeom_from_gserialized(gs1);
  LWGEOM *geom2 = lwgeom_from_gserialized(gs2);
  if (gserialized_get_type(gs1) == POLYGONTYPE)
    ensure_same_rings_lwpoly((LWPOLY *) geom1, (LWPOLY *) geom2);
  else
    ensure_same_geoms_lwpsurface((LWPSURFACE *) geom1, (LWPSURFACE *) geom2);

  if (! same_lwgeom(geom1, geom2))
  {
    meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
      "Operation on different reference geometries");
    return false;
  }

  lwgeom_free(geom1);
  lwgeom_free(geom2);
  return true;
}

/*****************************************************************************/

/**
 * @brief Apply a pose to an LWGEOM
 * @param[in] pose Pose
 * @param[in] geom Geometry
 */
void
lwgeom_apply_pose(const Pose *pose, LWGEOM *geom)
{
  if (! MEOS_FLAGS_GET_Z(pose->flags))
  {
    double a = cos(pose->data[2]);
    double b = sin(pose->data[2]);

    lwgeom_affine_transform(geom,
      a, b, 0,
      b, -a, 0,
      0, 0, 1,
      pose->data[0], pose->data[1], 0);
  }
  else
  {
    double W = pose->data[3];
    double X = pose->data[4];
    double Y = pose->data[5];
    double Z = pose->data[6];

    double a = W*W + X*X - Y*Y - Z*Z;
    double b = 2*X*Y - 2*W*Z;
    double c = 2*X*Z + 2*W*Y;
    double d = 2*X*Y + 2*W*Z;
    double e = W*W - X*X + Y*Y - Z*Z;
    double f = 2*Y*Z - 2*W*X;
    double g = 2*X*Z - 2*W*Y;
    double h = 2*Y*Z + 2*W*X;
    double i = W*W - X*X - Y*Y + Z*Z;

    lwgeom_affine_transform(geom,
      a, b, c,
      d, e, f,
      g, h, i,
      pose->data[0], pose->data[1], pose->data[2]);
  }
  return;
}

/*****************************************************************************/

/**
 * @brief Return the radius of a geometry
 * @param[in] gs Geometry
 */
double
geom_radius(const GSERIALIZED *gs)
{
  LWGEOM *geom = lwgeom_from_gserialized(gs);
  LWPOINTITERATOR *it = lwpointiterator_create(geom);
  double r = 0;
  POINT4D p;
  while (lwpointiterator_next(it, &p))
  {
    r = FLAGS_GET_Z(geom->flags) ?
      fmax(r, sqrt(p.x * p.x + p.y * p.y + p.z * p.z)) :
      fmax(r, sqrt(p.x * p.x + p.y * p.y));
  }
  lwpointiterator_destroy(it);
  lwgeom_free(geom);
  return r;
}

/*****************************************************************************
 * Compute Functions
 *****************************************************************************/

/**
 * @brief Return the pose that transforms the first geometry to the second one
 * @param[in] poly1,poly2 Polygons
 */
static Pose *
lwpoly_compute_2d(const LWPOLY *poly1, const LWPOLY *poly2)
{
  LWPOINT *centroid1 = lwpoly_centroid(poly1);
  double cx = lwpoint_get_x(centroid1);
  double cy = lwpoint_get_y(centroid1);

  POINTARRAY *ptarr1 = poly1->rings[0];
  POINTARRAY *ptarr2 = poly2->rings[0];

  POINT2D p11 = getPoint2d(ptarr1, 0);
  POINT2D p12 = getPoint2d(ptarr1, 1);
  POINT2D p21 = getPoint2d(ptarr2, 0);
  POINT2D p22 = getPoint2d(ptarr2, 1);

  double x1 = p11.x - cx, y1 = p11.y - cy;
  double x2 = p12.x - cx, y2 = p12.y - cy;
  double x1_ = p21.x - cx, y1_ = p21.y - cy;
  double x2_ = p22.x - cx, y2_ = p22.y - cy;
  double a, b, c, d;

  /* Compute affine transformation from poly1 to poly2 */
  a = ((x1_ - x2_)*(x1 - x2) + (y1_ - y2_)*(y1 - y2))/
    ((x1 - x2)*(x1 - x2) + (y1 - y2)*(y1 - y2));
  b = ((y1_ - y2_)*(x1 - x2) - (x1_ - x2_)*(y1 - y2))/
    ((x1 - x2)*(x1 - x2) + (y1 - y2)*(y1 - y2));
  c = x1_ - a*x1 + b*y1;
  d = y1_ - a*y1 - b*x1;

  double theta = atan2(b, a);
  return pose_make_2d(c, d, theta, poly1->srid);
}

/**
 * @brief Return the angle that transforms the first geometry to the second one
 * @param[in] e xxx
 * @param[in] p1,p2 xxx
 */
static double
pose_compute_angle_3d(double3 e, double3 p1, double3 p2)
{
  double3 p1_e = vec3_normalize(vec3_diff(p1, vec3_mult(e, vec3_dot(p1, e))));
  double3 p2_e = vec3_normalize(vec3_diff(p2, vec3_mult(e, vec3_dot(p2, e))));
  /* clip to [-1, 1] for acos */
  double dot = fmin(fmax(vec3_dot(p1_e, p2_e), -1.0), 1.0); 
  double theta = acos(dot);
  if (vec3_dot(e, vec3_cross(p1_e, p2_e)) < 0.0)
    theta = -theta;
  return theta;
}

/**
 * @brief Return the pose that transforms the first geometry to the second one
 * @param[in] psurf1,psurf2 Polyhedral surfaces
 */
static Pose *
lwpsurf_compute_3d(const LWPSURFACE *psurf1, const LWPSURFACE *psurf2)
{
  LWPOINT *centroid1 = lwpsurface_centroid(psurf1);
  LWPOINT *centroid2 = lwpsurface_centroid(psurf2);
  double cx1 = lwpoint_get_x(centroid1); double cx2 = lwpoint_get_x(centroid2);
  double cy1 = lwpoint_get_y(centroid1); double cy2 = lwpoint_get_y(centroid2);
  double cz1 = lwpoint_get_z(centroid1); double cz2 = lwpoint_get_z(centroid2);

  double dx = cx2 - cx1;
  double dy = cy2 - cy1;
  double dz = cz2 - cz1;

  POINTARRAY *ptarr1 = psurf1->geoms[0]->rings[0];
  POINTARRAY *ptarr2 = psurf2->geoms[0]->rings[0];

  POINT3DZ p11 = getPoint3dz(ptarr1, 0);
  POINT3DZ p12 = getPoint3dz(ptarr1, 1);
  POINT3DZ p21 = getPoint3dz(ptarr2, 0);
  POINT3DZ p22 = getPoint3dz(ptarr2, 1);
  if (fabs(p11.x) < MEOS_EPSILON && fabs(p11.y) < MEOS_EPSILON &&
      fabs(p11.z) < MEOS_EPSILON)
  {
    p11 = getPoint3dz(ptarr1, 2);
    p21 = getPoint3dz(ptarr2, 2);
  }
  int i = 2;
  while ((fabs(p12.x) < MEOS_EPSILON && fabs(p12.y) < MEOS_EPSILON &&
          fabs(p12.z) < MEOS_EPSILON) ||
        (fabs(p12.x - p11.x) < MEOS_EPSILON &&
          fabs(p12.y - p11.y) < MEOS_EPSILON &&
          fabs(p12.z - p11.z) < MEOS_EPSILON))
  {
    p12 = getPoint3dz(ptarr1, i);
    p22 = getPoint3dz(ptarr2, i);
    i++;
  }

  double3 P = (double3) {p11.x - cx1, p11.y - cy1, p11.z - cz1};
  double3 R = (double3) {p12.x - cx1, p12.y - cy1, p12.z - cz1};
  double3 P_ = (double3) {p21.x - cx2, p21.y - cy2, p21.z - cz2};
  double3 R_ = (double3) {p22.x - cx2, p22.y - cy2, p22.z - cz2};

  double3 PP_ = vec3_diff(P_, P);
  double3 RR_ = vec3_diff(R_, R);
  double3 PR = vec3_diff(R, P);

  double Pnorm = vec3_norm(PP_);
  double Rnorm = vec3_norm(RR_);

  double3 e;
  double theta;
  if (Pnorm < MEOS_EPSILON && Rnorm < MEOS_EPSILON) // No rotation
    return pose_make_3d(dx, dy, dz, 1, 0, 0, 0, psurf1->srid);
  else if (Pnorm < MEOS_EPSILON) // Rotation around P
  {
    e = vec3_normalize(P);
    theta = pose_compute_angle_3d(e, R, R_);
  }
  else if (Rnorm < MEOS_EPSILON) // Rotation around R
  {
    e = vec3_normalize(R);
    theta = pose_compute_angle_3d(e, P, P_);
  }
  else
  {
    double dot = vec3_dot(PP_, RR_);
    if (fabs(dot - Pnorm*Rnorm) < MEOS_EPSILON) // Same direction
      e = vec3_normalize(vec3_cross(PP_, vec3_cross(PR, PP_)));
    else if (fabs(dot + Pnorm*Rnorm) < MEOS_EPSILON) // Opposite direction
      e = vec3_normalize(vec3_cross(PR, PP_));
    else // General case
      e = vec3_normalize(vec3_cross(PP_, RR_));
    theta = pose_compute_angle_3d(e, P, P_);
  }
  Quaternion q = quaternion_from_axis_angle(e, theta);
  return pose_make_3d(dx, dy, dz, q.X, q.Y, q.Z, q.W, psurf1->srid);
}

/**
 * @brief Return true if the two geometries are congruent
 * @details This means that the geometries are identical in shape and size,
 * even if one is rotated, flipped, or translated, that is, all corresponding
 * sides and angles are equal.
 * @param[in] gs1,gs2 Geometries
 */
bool
ensure_rigid_body(const GSERIALIZED *gs1, const GSERIALIZED *gs2)
{
  LWGEOM *geom1 = lwgeom_from_gserialized(gs1);
  LWGEOM *geom2 = lwgeom_from_gserialized(gs2);
  bool rigid = lwgeom_is_rigid(geom1, geom2);
  lwgeom_free(geom1); lwgeom_free(geom2);
  if (! rigid)
  {
    meos_error(ERROR, MEOS_ERR_RESTRICT_VIOLATION,
      "All geometries must be congruent");
    return false;
  }
  return true;
}

/**
 * @brief Return true if the geometries is convex
 * @param[in] gs Geometries
 */
bool
ensure_is_convex(const GSERIALIZED *gs)
{
  if (! poly_is_convex(gs))
  {
    meos_error(ERROR, MEOS_ERR_INTERNAL_ERROR,
      "The geometry must be convext");
    return false;
  }
  return true;
}

/**
 * @brief Return the pose that transforms the first geometry to the second one
 * @param[in] gs1,gs2 Geometries
 */
Pose *
geom_compute_pose(const GSERIALIZED *gs1, const GSERIALIZED *gs2)
{
  /* Ensure the validity of arguments */
  VALIDATE_NOT_NULL(gs1, NULL); VALIDATE_NOT_NULL(gs2, NULL);

  Pose *result;
  if (! FLAGS_GET_Z(gs1->gflags))
  {
    LWPOLY *poly1 = (LWPOLY *) lwgeom_from_gserialized(gs1);
    LWPOLY *poly2 = (LWPOLY *) lwgeom_from_gserialized(gs2);
    result = lwpoly_compute_2d(poly1, poly2);
    lwpoly_free(poly1); lwpoly_free(poly2);
  }
  else
  {
    LWPSURFACE *psurf1 = (LWPSURFACE *) lwgeom_from_gserialized(gs1);
    LWPSURFACE *psurf2 = (LWPSURFACE *) lwgeom_from_gserialized(gs2);
    result = lwpsurf_compute_3d(psurf1, psurf2);
    lwpsurface_free(psurf1); lwpsurface_free(psurf2);
  }
  GSERIALIZED *gs2_computed = geom_apply_pose(gs1, result);
  bool rigid = ensure_rigid_body(gs2, gs2_computed);
  pfree(gs2_computed);
  if (! rigid)
  {
    pfree(result);
    return NULL;
  }
  return result;
}

/*****************************************************************************/
