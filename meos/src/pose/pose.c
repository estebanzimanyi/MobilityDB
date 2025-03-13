/*****************************************************************************
 *
 * This MobilityDB code is provided under The PostgreSQL License.
 * Copyright (c) 2016-2024, Université libre de Bruxelles and MobilityDB
 * contributors
 *
 * MobilityDB includes portions of PostGIS version 3 source code released
 * under the GNU General Public License (GPLv2 or later).
 * Copyright (c) 2001-2024, PostGIS contributors
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
 * @brief Basic functions for static pose objects.
 */

/* C */
#include <math.h>
#include <limits.h>
/* Postgres */
#include <postgres.h>
#if POSTGRESQL_VERSION_NUMBER >= 160000
  #include "varatt.h"
#endif
#include <common/hashfn.h>
#include <utils/float.h>
/* MEOS */
#include <meos.h>
#include <meos_internal.h>
#include <meos_pose.h>
#include "general/pg_types.h"
#include "geo/tgeo_spatialfuncs.h"
#include "general/type_parser.h"
#include "geo/tspatial_parser.h"
#include "pose/pose.h"

/** Buffer size for input and output of pose values */
#define MAXPOSELEN    128

/*****************************************************************************
 * Interpolation function
 *****************************************************************************/

/**
 * @brief Return the pose value interpolated from the two poses and a ratio
 * @param[in] pose1,pose2 Poses
 * @param[in] ratio Value in [0,1] representing the duration of the
 * timestamps associated to `p1` and `p2` divided by the duration
 * of the timestamps associated to `p1` and `p3`
 */
Pose *
pose_interpolate(const Pose *pose1, const Pose *pose2, double ratio)
{
  assert(pose1); assert(pose2);
  Pose *result;
  if (!MEOS_FLAGS_GET_Z(pose1->flags))
  {
    double x = pose1->data[0] * (1 - ratio) + pose2->data[0] * ratio;
    double y = pose1->data[1] * (1 - ratio) + pose2->data[1] * ratio;
    double theta;
    double theta_delta = pose2->data[2] - pose1->data[2];
    /* If fabs(theta_delta) == M_PI: Always turn counter-clockwise */
    if (fabs(theta_delta) < MEOS_EPSILON)
        theta = pose1->data[2];
    else if (theta_delta > 0 && fabs(theta_delta) <= M_PI)
        theta = pose1->data[2] + theta_delta*ratio;
    else if (theta_delta > 0 && fabs(theta_delta) > M_PI)
        theta = pose2->data[2] + (2*M_PI - theta_delta)*(1 - ratio);
    else if (theta_delta < 0 && fabs(theta_delta) < M_PI)
        theta = pose1->data[2] + theta_delta*ratio;
    else /* (theta_delta < 0 && fabs(theta_delta) >= M_PI) */
        theta = pose1->data[2] + (2*M_PI + theta_delta)*ratio;
    if (theta > M_PI)
        theta = theta - 2*M_PI;
    result = pose_make_2d(x, y, theta);
  }
  else
  {
    double x = pose1->data[0] * (1 - ratio) + pose2->data[0] * ratio;
    double y = pose1->data[1] * (1 - ratio) + pose2->data[1] * ratio;
    double z = pose1->data[2] * (1 - ratio) + pose2->data[2] * ratio;
    double W, W1 = pose1->data[3], W2 = pose2->data[3];
    double X, X1 = pose1->data[4], X2 = pose2->data[4];
    double Y, Y1 = pose1->data[5], Y2 = pose2->data[5];
    double Z, Z1 = pose1->data[6], Z2 = pose2->data[6];
    double dot =  W1*W2 + X1*X2 + Y1*Y2 + Z1*Z2;
    if (dot < 0.0f)
    {
      W2 = -W2;
      X2 = -X2;
      Y2 = -Y2;
      Z2 = -Z2;
      dot = -dot;
    }
    const double DOT_THRESHOLD = 0.9995;
    if (dot > DOT_THRESHOLD)
    {
      W = W1 + (W2 - W1)*ratio;
      X = X1 + (X2 - X1)*ratio;
      Y = Y1 + (Y2 - Y1)*ratio;
      Z = Z1 + (Z2 - Z1)*ratio;
    }
    else
    {
      double theta_0 = acos(dot);
      double theta = theta_0*ratio;
      double sin_theta = sin(theta);
      double sin_theta_0 = sin(theta_0);
      double s1 = cos(theta) - dot * sin_theta / sin_theta_0;
      double s2 = sin_theta / sin_theta_0;
      W = W1*s1 + W2*s2;
      X = X1*s1 + X2*s2;
      Y = Y1*s1 + Y2*s2;
      Z = Z1*s1 + Z2*s2;
    }
    double norm = W*W + X*X + Y*Y + Z*Z;
    W /= norm;
    X /= norm;
    Y /= norm;
    Z /= norm;
    result = pose_make_3d(x, y, z, W, X, Y, Z);
  }
  return result;
}

/**
 * @brief Return true if the three values are collinear
 * @param[in] p1,p2,p3 Poses
 * @param[in] ratio Value in [0,1] representing the duration of the
 * timestamps associated to `p1` and `p2` divided by the duration
 * of the timestamps associated to `p1` and `p3`
 */
bool
pose_collinear(const Pose *p1, const Pose *p2, const Pose *p3, double ratio)
{
  assert(p1); assert(p2); assert(p3); 
  Pose *p2_interpolated = pose_interpolate(p1, p3, ratio);
  bool result = pose_same(p2, p2_interpolated);
  pfree(p2_interpolated);
  return result;
}

/*****************************************************************************
 * Input/output functions
 *****************************************************************************/

/**
 * @brief Parse a pose value from the buffer
 */
Pose *
pose_parse(const char **str, bool end)
{
  Pose *result;
  bool hasZ = false;
  const char *type_str = meostype_name(T_POSE);

  /* Determine whether the box has an SRID */
  int32_t srid;
  srid_parse(str, &srid);

  if (strncasecmp(*str,"POSE",4) == 0)
  {
    *str += 4;
    p_whitespace(str);
  }
  else
  {
    meos_error(ERROR, MEOS_ERR_TEXT_INPUT,
      "Could not parse pose value");
    return NULL;
  }

  /* Determine whether the pose is 3D */
  if (strncasecmp(*str,"Z",1) == 0)
  {
    hasZ = true;
    *str += 1;
    p_whitespace(str);
  }

  /* Parse opening parenthesis */
  if (! ensure_oparen(str, type_str))
    return NULL;

  /* Parse first 3 values: (x, y, theta) in 2D or (x, y, z, ...) in 3D */
  double x, y, z;
  p_whitespace(str);
  if (! double_parse(str, &x)) return NULL;
  p_whitespace(str); p_comma(str); p_whitespace(str);
  if (! double_parse(str, &y)) return NULL;
  p_whitespace(str); p_comma(str); p_whitespace(str);
  if (! double_parse(str, &z)) return NULL;

  if (!hasZ)
  {
    /* use z as theta in 2D */
    if (z < -M_PI || z > M_PI)
    {
      meos_error(ERROR, MEOS_ERR_TEXT_INPUT,
        "Could not parse 2D pose: Rotation angle must be in ]-pi, pi]. Recieved: %f", z);
      return NULL;
    }
    result = pose_make_2d(x, y, z);
  }
  else
  {
    double W, X, Y, Z;
    p_whitespace(str); p_comma(str); p_whitespace(str);
    if (! double_parse(str, &W)) return NULL;
    p_whitespace(str); p_comma(str); p_whitespace(str);
    if (! double_parse(str, &X)) return NULL;
    p_whitespace(str); p_comma(str); p_whitespace(str);
    if (! double_parse(str, &Y)) return NULL;
    p_whitespace(str); p_comma(str); p_whitespace(str);
    if (! double_parse(str, &Z)) return NULL;
    if (fabs(sqrt(W*W + X*X + Y*Y + Z*Z) - 1)  > MEOS_EPSILON)
    {
      meos_error(ERROR, MEOS_ERR_TEXT_INPUT,
        "Could not parse 3D pose: Rotation quaternion must be of unit norm. Recieved: %f",
        sqrt(W*W + X*X + Y*Y + Z*Z));
      return NULL;
    }
    result = pose_make_3d(x, y, z, W, X, Y, Z);
  }

  /* Parse closing parenthesis */
  p_whitespace(str);
  if (! ensure_cparen(str, type_str) ||
        (end && ! ensure_end_input(str, type_str)))
    return NULL;

  pose_set_srid(result, srid);
  return result;
}

/**
 * @ingroup meos_base_inout
 * @brief Return a pose from its string representation.
 * @param[in] str String
 * @csqlfn #Pose_in()
 */
Pose *
pose_in(const char *str)
{
  /* Ensure validity of the arguments */
#if MEOS
  if (! ensure_not_null((void *) str))
    return NULL;
#else
  assert(str);
#endif /* MEOS */
  return pose_parse(&str, true);
}

/**
 * @ingroup meos_base_inout
 * @brief Return the string representation of a pose
 * @param[in] pose Pose
 * @param[in] maxdd Maximum number of decimal digits
 * @csqlfn #Pose_out()
 */
char *
pose_out(const Pose *pose, int maxdd)
{
  /* Ensure validity of the arguments */
#if MEOS
  if (! ensure_not_null((void *) pose))
    return NULL;
#else
  assert(pose);
#endif /* MEOS */
  if (! ensure_not_negative(maxdd))
    return NULL;

  char *result = palloc(MAXPOSELEN);
  char *x = float8_out(pose->data[0], maxdd);
  char *y = float8_out(pose->data[1], maxdd);
  char *z = float8_out(pose->data[2], maxdd); /* theta if 2D*/
  if (!MEOS_FLAGS_GET_Z(pose->flags))
  {
    snprintf(result, MAXPOSELEN - 1, "POSE (%s, %s, %s)", x, y, z);
  }
  else
  {
    char *W = float8_out(pose->data[3], maxdd);
    char *X = float8_out(pose->data[4], maxdd);
    char *Y = float8_out(pose->data[5], maxdd);
    char *Z = float8_out(pose->data[6], maxdd);
    snprintf(result, MAXPOSELEN - 1, "POSE Z (%s, %s, %s, %s, %s, %s, %s)",
      x, y, z, W, X, Y, Z);
    pfree(W); pfree(X); pfree(Y); pfree(Z);
  }
  pfree(x); pfree(y); pfree(z);
  return result;
}

/*****************************************************************************
 * Constructors
 *****************************************************************************/

/**
 * @ingroup meos_base_constructor
 * @brief Construct a 2D pose value from the arguments
 * @param[in] x,y Position
 * @param[in] theta Orientation
 */
Pose *
pose_make_2d(double x, double y, double theta)
{
  if (theta < -M_PI || theta > M_PI)
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE,
      "Rotation angle must be in ]-pi, pi]. Received: %f", theta);

  /* We want a unique representation for theta */
  if (theta == -M_PI)
    theta = M_PI;

  size_t memsize = DOUBLE_PAD(sizeof(Pose)) + 3 * sizeof(double);
  Pose *result = palloc0(memsize);
  SET_VARSIZE(result, memsize);
  MEOS_FLAGS_SET_X(result->flags, true);
  MEOS_FLAGS_SET_Z(result->flags, false);
  result->data[0] = x;
  result->data[1] = y;
  result->data[2] = theta;
  return result;
}

/**
 * @ingroup meos_base_constructor
 * @brief Construct a 3D pose value from the arguments
 * @param[in] x,y,z Position
 * @param[in] W,X,Y,Z Orientation
 */
Pose *
pose_make_3d(double x, double y, double z,
  double W, double X, double Y, double Z)
{
  if (fabs(sqrt(W*W + X*X + Y*Y + Z*Z) - 1)  > MEOS_EPSILON)
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE,
      "Rotation quaternion must be of unit norm. Received: %f",
      sqrt(W*W + X*X + Y*Y + Z*Z));

  /* If we want a unique representation for the quaternion */
  if (W < 0.0)
  {
    W = -W;
    X = -X;
    Y = -Y;
    Z = -Z;
  }

  size_t memsize = DOUBLE_PAD(sizeof(Pose)) + 7 * sizeof(double);
  Pose *result = palloc0(memsize);
  SET_VARSIZE(result, memsize);
  MEOS_FLAGS_SET_X(result->flags, true);
  MEOS_FLAGS_SET_Z(result->flags, true);
  result->data[0] = x;
  result->data[1] = y;
  result->data[2] = z;
  result->data[3] = W;
  result->data[4] = X;
  result->data[5] = Y;
  result->data[6] = Z;
  return result;
}

/**
 * @ingroup meos_base_constructor
 * @brief Copy a pose value
 * @param[in] pose Pose
 */
Pose *
pose_copy(const Pose *pose)
{
  /* Ensure validity of the arguments */
#if MEOS
  if (! ensure_not_null((void *) pose))
    return NULL;
#else
  assert(pose);
#endif /* MEOS */
  Pose *result = palloc(VARSIZE(pose));
  memcpy(result, pose, VARSIZE(pose));
  return result;
}

/*****************************************************************************
 * SRID functions
 *****************************************************************************/

/**
 * @ingroup meos_base_spatial
 * @brief Return the SRID
 * @param[in] pose Pose
 */
int32
pose_srid(const Pose *pose)
{
  /* Ensure validity of the arguments */
#if MEOS
  if (! ensure_not_null((void *) pose))
    return SRID_INVALID;
#else
  assert(pose);
#endif /* MEOS */

  int32 srid = 0;
  srid = srid | (pose->srid[0] << 16);
  srid = srid | (pose->srid[1] << 8);
  srid = srid | (pose->srid[2]);
  /* Only the first 21 bits are set. Slide up and back to pull
     the negative bits down, if we need them. */
  srid = (srid<<11)>>11;

  /* 0 is our internal unknown value. We'll map back and forth here for now */
  if (srid == 0)
    return SRID_UNKNOWN;
  else
    return srid;
}

/**
 * @ingroup meos_base_spatial
 * @brief Set the SRID
 * @param[in] pose Pose
 * @param[in] srid SRID
 */
void
pose_set_srid(Pose *pose, int32 srid)
{
  /* Ensure validity of the arguments */
#if MEOS
  if (! ensure_not_null((void *) pose))
  {
    ;
  }
#else
  assert(pose);
#endif /* MEOS */

  srid = clamp_srid(srid);

  /* 0 is our internal unknown value.
   * We'll map back and forth here for now */
  if (srid == SRID_UNKNOWN)
    srid = 0;

  pose->srid[0] = (srid & 0x001F0000) >> 16;
  pose->srid[1] = (srid & 0x0000FF00) >> 8;
  pose->srid[2] = (srid & 0x000000FF);
}

/*****************************************************************************
 * Conversion functions
 *****************************************************************************/

/**
 * @ingroup meos_base_conversion
 * @brief Transforms the pose into a geometry point
 * @param[in] pose Pose
 */
GSERIALIZED *
pose_point(const Pose *pose)
{
  /* Ensure validity of the arguments */
#if MEOS
  if (! ensure_not_null((void *) pose))
    return NULL;
#else
  assert(pose);
#endif /* MEOS */

  LWPOINT *point;
  if (MEOS_FLAGS_GET_Z(pose->flags))
    point = lwpoint_make3dz(pose_srid(pose),
      pose->data[0], pose->data[1], pose->data[2]);
  else
    point = lwpoint_make2d(pose_srid(pose),
      pose->data[0], pose->data[1]);
  GSERIALIZED *gs = geo_serialize((LWGEOM *)point);
  lwpoint_free(point);
  return gs;
}

/**
 * @brief Transforms the pose into a geometry point
 */
Datum
datum_pose_point(Datum pose)
{
  return PosePGetDatum(pose_point(DatumGetPoseP(pose)));
}

/*****************************************************************************
 * Distance function
 *****************************************************************************/

/**
 * @brief Return the distance between the two poses
 */
Datum
pose_distance(Datum pose1, Datum pose2)
{
  Datum geom1 = PosePGetDatum(pose_point(DatumGetPoseP(pose1)));
  Datum geom2 = PosePGetDatum(pose_point(DatumGetPoseP(pose2)));
  return datum_pt_distance2d(geom1, geom2);
}

/*****************************************************************************
 * Comparison functions for defining B-tree indexes
 *****************************************************************************/

/**
 * @ingroup meos_base_comp
 * @brief Return true if the first pose is equal to the second one
 * @param[in] pose1,pose2 Poses
 */
bool
pose_eq(const Pose *pose1, const Pose *pose2)
{
  /* Ensure validity of the arguments */
#if MEOS
  if (! ensure_not_null((void *) pose1) || ! ensure_not_null((void *) pose2))
    return NULL;
#else
  assert(pose1); assert(pose2);
#endif /* MEOS */

  if (MEOS_FLAGS_GET_Z(pose1->flags) != MEOS_FLAGS_GET_Z(pose2->flags) ||
      pose_srid(pose1) != pose_srid(pose2))
    return false;
  bool result = (
    float8_eq(pose1->data[0], pose2->data[0]) &&
    float8_eq(pose1->data[1], pose2->data[1]) &&
    float8_eq(pose1->data[2], pose2->data[2])
  );
  if (MEOS_FLAGS_GET_Z(pose1->flags))
    result &= (
      float8_eq(pose1->data[3], pose2->data[3]) &&
      float8_eq(pose1->data[4], pose2->data[4]) &&
      float8_eq(pose1->data[5], pose2->data[5]) &&
      float8_eq(pose1->data[6], pose2->data[6])
    );
  return result;
}

/**
 * @ingroup meos_base_comp
 * @brief Return true if the first pose is not equal to the second one
 * @param[in] pose1,pose2 Poses
 */
bool
pose_ne(const Pose *pose1, const Pose *pose2)
{
  return (! pose_eq(pose1, pose2));
}

/**
 * @ingroup meos_base_comp
 * @brief Return true if the first pose is equal to the second one
 * @param[in] pose1,pose2 Poses
 */
bool
pose_same(const Pose *pose1, const Pose *pose2)
{
  /* Ensure validity of the arguments */
#if MEOS
  if (! ensure_not_null((void *) pose1) || ! ensure_not_null((void *) pose2))
    return NULL;
#else
  assert(pose1); assert(pose2);
#endif /* MEOS */

  if (MEOS_FLAGS_GET_Z(pose1->flags) != MEOS_FLAGS_GET_Z(pose2->flags) ||
      pose_srid(pose1) != pose_srid(pose2))
    return false;
  bool result = (
    MEOS_FP_EQ(pose1->data[0], pose2->data[0]) &&
    MEOS_FP_EQ(pose1->data[1], pose2->data[1]) &&
    MEOS_FP_EQ(pose1->data[2], pose2->data[2])
  );
  if (MEOS_FLAGS_GET_Z(pose1->flags))
    result &= (
      MEOS_FP_EQ(pose1->data[3], pose2->data[3]) &&
      MEOS_FP_EQ(pose1->data[4], pose2->data[4]) &&
      MEOS_FP_EQ(pose1->data[5], pose2->data[5]) &&
      MEOS_FP_EQ(pose1->data[6], pose2->data[6])
    );
  return result;
}

/**
 * @ingroup meos_base_comp
 * @brief Return true if the first pose is not equal to the second one
 * @param[in] pose1,pose2 Poses
 */
bool
pose_nsame(const Pose *pose1, const Pose *pose2)
{
  return (! pose_same(pose1, pose2));
}

/**
 * @ingroup meos_base_comp
 * @brief Return -1, 0, or 1 depending on whether the first pose
 * is less than, equal to, or greater than the second one
 * @param[in] pose1,pose2 Poses
 */
int
pose_cmp(const Pose *pose1, const Pose *pose2)
{
  /* Ensure validity of the arguments */
#if MEOS
  if (! ensure_not_null((void *) pose1) || ! ensure_not_null((void *) pose2))
    return INT_MAX;
#else
  assert(pose1); assert(pose2);
#endif /* MEOS */

  /* Compare first the dimension, then the SRID,
     then the position, then the orientation */
  bool hasz1 = MEOS_FLAGS_GET_Z(pose1->flags),
       hasz2 = MEOS_FLAGS_GET_Z(pose2->flags);
  if (hasz1 != hasz2)
    return (hasz1 ? 1 : -1);

  int32 srid1 = pose_srid(pose1),
        srid2 = pose_srid(pose2);
  if (srid1 < srid2)
    return -1;
  else if (srid1 > srid2)
    return 1;

  if (hasz1)
    return memcmp(pose1->data, pose2->data, sizeof(double) * 7);
  else
    return memcmp(pose1->data, pose2->data, sizeof(double) * 3);
}

/**
 * @ingroup meos_base_comp
 * @brief Return true if the first pose is less than the second one
 * @param[in] pose1,pose2 Poses
 */
bool
pose_lt(const Pose *pose1, const Pose *pose2)
{
  int cmp = pose_cmp(pose1, pose2);
  return (cmp < 0);
}

/**
 * @ingroup meos_base_comp
 * @brief Return true if the first pose is less than or equal to the second one
 * @param[in] pose1,pose2 Poses
 */
bool
pose_le(const Pose *pose1, const Pose *pose2)
{
  int cmp = pose_cmp(pose1, pose2);
  return (cmp <= 0);
}

/**
 * @ingroup meos_base_comp
 * @brief Return true if the first pose is greater than the second one
 * @param[in] pose1,pose2 Poses
 */
bool
pose_gt(const Pose *pose1, const Pose *pose2)
{
  int cmp = pose_cmp(pose1, pose2);
  return (cmp > 0);
}

/**
 * @ingroup meos_base_comp
 * @brief Return true if the first pose is greater than or equal to the second
 * one
 * @param[in] pose1,pose2 Poses
 */
bool
pose_ge(const Pose *pose1, const Pose *pose2)
{
  int cmp = pose_cmp(pose1, pose2);
  return (cmp >= 0);
}

/*****************************************************************************
 * Function for defining hash indexes
 * The function reuses the approach for span types for combining the hash of
 * the lower and upper bounds.
 *****************************************************************************/

/* Prototype for liblwgeom/lookup3.c */
/* key = the key to hash */
/* length = length of the key */
/* pc = IN: primary initval, OUT: primary hash */
/* pb = IN: secondary initval, OUT: secondary hash */
void hashlittle2(const void *key, size_t length, uint32_t *pc, uint32_t *pb);

/**
 * @ingroup meos_base_accessor
 * @brief Return the 32-bit hash value of a pose
 * @param[in] pose Pose
 */
uint32
pose_hash(const Pose *pose)
{
  /* Ensure validity of the arguments */
#if MEOS
  if (! ensure_not_null((void *) pose))
    return INT_MAX;
#else
  assert(pose);
#endif /* MEOS */

  /* Use same code as gserialized2_hash */
  int32_t hval;
  int32_t pb = 0, pc = 0;
  /* Point to just the type/coordinate part of buffer */
  size_t hsz1 = 8; /* varsize (4) + flags (1) + srid(3) */
  uint8_t *b1 = (uint8_t *)pose + hsz1;
  /* Calculate size of type/coordinate buffer */
  size_t sz1 = VARSIZE(pose);
  size_t bsz1 = sz1 - hsz1;
  /* Calculate size of srid/type/coordinate buffer */
  int32_t srid = pose_srid(pose);
  size_t bsz2 = bsz1 + sizeof(int);
  uint8_t *b2 = palloc(bsz2);
  /* Copy srid into front of combined buffer */
  memcpy(b2, &srid, sizeof(int));
  /* Copy type/coordinates into rest of combined buffer */
  memcpy(b2+sizeof(int), b1, bsz1);
  /* Hash combined buffer */
  hashlittle2(b2, bsz2, (uint32_t *)&pb, (uint32_t *)&pc);
  pfree(b2);
  hval = pb ^ pc;
  return hval;
}

/**
 * @ingroup meos_base_accessor
 * @brief Return the 64-bit hash value of a point using a seed
 * @param[in] pose Pose
 * @param[in] seed Seed
 * csqlfn hash_extended
 */
uint64
pose_hash_extended(const Pose *pose, uint64 seed)
{
  /* PostGIS currently does not provide an extended hash function, */
  return DatumGetUInt64(hash_any_extended(
    (unsigned char *) VARDATA_ANY(pose), VARSIZE_ANY_EXHDR(pose), seed));
}

/*****************************************************************************/
