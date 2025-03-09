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
 * @file
 * @brief Basic functions for pose objects
 */

#include "pose/pose.h"

/* C */
#include <assert.h>
#include <math.h>
/* MobilityDB */
#include "general/pg_types.h"
#include "general/type_out.h"
#include "general/type_util.h"

/*****************************************************************************
 * Input/Output functions for pose
 *****************************************************************************/

PGDLLEXPORT Datum Pose_in(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Pose_in);
/**
 * @ingroup mobilitydb_base_inout
 * @brief Input function for pose values
 * @details Example of input:
 *    (1, 0.5)
 */
Datum
Pose_in(PG_FUNCTION_ARGS)
{
  const char *str = PG_GETARG_CSTRING(0);
  PG_RETURN_POINTER(pose_in(str, true));
}

PGDLLEXPORT Datum Pose_out(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Pose_out);
/**
 * @ingroup mobilitydb_base_inout
 * @brief Output function for pose values
 */
Datum
Pose_out(PG_FUNCTION_ARGS)
{
  Pose *pose = PG_GETARG_POSE_P(0);
  PG_RETURN_CSTRING(pose_out(pose, OUT_DEFAULT_DECIMAL_DIGITS));
}

/*****************************************************************************
 * Constructors
 *****************************************************************************/

PGDLLEXPORT Datum Pose_constructor(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Pose_constructor);
/**
 * @ingroup mobilitydb_base_constructor
 * @brief Construct a pose value from the arguments
 * @sqlfn pose()
 */
Datum
Pose_constructor(PG_FUNCTION_ARGS)
{
  double x = PG_GETARG_FLOAT8(0);
  double y = PG_GETARG_FLOAT8(1);
  double z = PG_GETARG_FLOAT8(2);
  double theta = z;

  Pose *result;
  if (PG_NARGS() == 3)
    result = pose_make_2d(x, y, theta);
  else /* PG_NARGS() == 7 */
  {
    double W = PG_GETARG_FLOAT8(3);
    double X = PG_GETARG_FLOAT8(4);
    double Y = PG_GETARG_FLOAT8(5);
    double Z = PG_GETARG_FLOAT8(6);
    result = pose_make_3d(x, y, z, W, X, Y, Z);
  }

  PG_RETURN_POINTER(result);
}

/*****************************************************************************
 * Casting to Point
 *****************************************************************************/

PGDLLEXPORT Datum Pose_to_geom(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Pose_to_geom);
/**
 * @ingroup mobilitydb_base_conversion
 * @brief Transforms the pose into a geometry point
 * @sqlfn geometry()
 */
Datum
Pose_to_geom(PG_FUNCTION_ARGS)
{
  Pose *pose = PG_GETARG_POSE_P(0);
  GSERIALIZED *result = pose_geom(pose);
  PG_RETURN_POINTER(result);
}

/*****************************************************************************
 * Approximate equality for poses
 *****************************************************************************/

PGDLLEXPORT Datum Pose_same(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Pose_same);
/**
 * @ingroup mobilitydb_base_spatial
 * @brief Return true if two poses are approximately equal with respect to an
 * epsilon value
 * @sqlfn same()
 */
Datum
Pose_same(PG_FUNCTION_ARGS)
{
  Pose *pose1 = PG_GETARG_POSE_P(0);
  Pose *pose2 = PG_GETARG_POSE_P(1);
  PG_RETURN_BOOL(pose_same(pose1, pose2));
}

/*****************************************************************************
 * Comparison functions
 *****************************************************************************/

PGDLLEXPORT Datum Pose_eq(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Pose_eq);
/**
 * @ingroup mobilitydb_base_comp
 * @brief Return true if the first pose is equal to the second one
 * @sqlfn pose_eq()
 * @sqlop @p =
 */
Datum
Pose_eq(PG_FUNCTION_ARGS)
{
  Pose *pose1 = PG_GETARG_POSE_P(0);
  Pose *pose2 = PG_GETARG_POSE_P(1);
  PG_RETURN_BOOL(pose_eq(pose1, pose2));
}

PGDLLEXPORT Datum Pose_ne(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Pose_ne);
/**
 * @ingroup mobilitydb_base_comp
 * @brief Return true if the first pose is not equal to the second one
 * @sqlfn pose_ne()
 * @sqlop @p <>
 */
Datum
Pose_ne(PG_FUNCTION_ARGS)
{
  Pose *pose1 = PG_GETARG_POSE_P(0);
  Pose *pose2 = PG_GETARG_POSE_P(1);
  PG_RETURN_BOOL(pose_ne(pose1, pose2));
}

PGDLLEXPORT Datum Pose_cmp(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Pose_cmp);
/**
 * @ingroup mobilitydb_base_comp
 * @brief Return -1, 0, or 1 depending on whether the first pose is less than,
 * equal to, or greater than the second one
 * @note Function used for B-tree comparison
 * @sqlfn pose_cmp()
 */
Datum
Pose_cmp(PG_FUNCTION_ARGS)
{
  Pose *pose1 = PG_GETARG_POSE_P(0);
  Pose *pose2 = PG_GETARG_POSE_P(1);
  PG_RETURN_INT32(pose_cmp(pose1, pose2));
}

PGDLLEXPORT Datum Pose_lt(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Pose_lt);
/**
 * @ingroup mobilitydb_base_comp
 * @brief Return true if the first pose is less than the second one
 * @sqlfn pose_lt()
 * @sqlop @p <
 */
Datum
Pose_lt(PG_FUNCTION_ARGS)
{
  Pose *pose1 = PG_GETARG_POSE_P(0);
  Pose *pose2 = PG_GETARG_POSE_P(1);
  PG_RETURN_BOOL(pose_lt(pose1, pose2));
}

PGDLLEXPORT Datum Pose_le(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Pose_le);
/**
 * @ingroup mobilitydb_base_comp
 * @brief Return true if the first pose is less than or equal to the second one
 * @sqlfn pose_le()
 * @sqlop @p <=
 */
Datum
Pose_le(PG_FUNCTION_ARGS)
{
  Pose *pose1 = PG_GETARG_POSE_P(0);
  Pose *pose2 = PG_GETARG_POSE_P(1);
  PG_RETURN_BOOL(pose_le(pose1, pose2));
}

PGDLLEXPORT Datum Pose_ge(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Pose_ge);
/**
 * @ingroup mobilitydb_base_comp
 * @brief Return true if the first pose is greater than or equal to the second
 * one
 * @sqlfn pose_ge()
 * @sqlop @p >=
 */
Datum
Pose_ge(PG_FUNCTION_ARGS)
{
  Pose *pose1 = PG_GETARG_POSE_P(0);
  Pose *pose2 = PG_GETARG_POSE_P(1);
  PG_RETURN_BOOL(pose_ge(pose1, pose2));
}

PGDLLEXPORT Datum Pose_gt(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Pose_gt);
/**
 * @ingroup mobilitydb_base_comp
 * @brief Return true if the first pose is greater than the second one
 * @sqlfn pose_gt()
 * @sqlop @p >
 */
Datum
Pose_gt(PG_FUNCTION_ARGS)
{
  Pose *pose1 = PG_GETARG_POSE_P(0);
  Pose *pose2 = PG_GETARG_POSE_P(1);
  PG_RETURN_BOOL(pose_gt(pose1, pose2));
}

/*****************************************************************************
 * Functions for defining hash indexes
 *****************************************************************************/

PGDLLEXPORT Datum Pose_hash(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Pose_hash);
/**
 * @ingroup mobilitydb_base_accessor
 * @brief Return the 32-bit hash value of a pose
 * @sqlfn hash()
 */
Datum
Pose_hash(PG_FUNCTION_ARGS)
{
  Pose *pose = PG_GETARG_POSE_P(0);
  uint32 result = pose_hash(pose);
  PG_FREE_IF_COPY(pose, 0);
  PG_RETURN_UINT32(result);
}

PGDLLEXPORT Datum Pose_hash_extended(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Pose_hash_extended);
/**
 * @ingroup mobilitydb_base_accessor
 * @brief Return the 64-bit hash value of a pose using a seed
 * @sqlfn hash_extended()
 */
Datum
Pose_hash_extended(PG_FUNCTION_ARGS)
{
  Pose *pose = PG_GETARG_POSE_P(0);
  uint64 seed = PG_GETARG_INT64(1);
  uint64 result = pose_hash_extended(pose, seed);
  PG_FREE_IF_COPY(pose, 0);
  PG_RETURN_UINT64(result);
}


/*****************************************************************************/
