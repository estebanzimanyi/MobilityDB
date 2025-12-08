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
 * @brief Ever and always spatial relationships for temporal poses
 * @details These relationships compute the ever/always spatial relationship
 * between the arguments and return a Boolean. These functions may be used for
 * filtering purposes before applying the corresponding spatiotemporal
 * relationship.
 *
 * The following relationships are supported: `eContains`, `aContains`,
 * `eCovers`, `aCovers`, `eDisjoint`, `aDisjoint`, `eIntersects`, 
 * `aIntersects`, `eTouches`, `aTouches`, `eDwithin`, and `aDwithin`.
 *
 * Most of these relationships support the following combination of arguments
 * `({geo, pose, tpose}, {geo, pose, tpose})`.
 * One exception is for the non-symmetric relationships `eContains` and 
 * `eCovers` since there is no efficient algorithm for enabling the
 * combination of arguments `(geo, tpose)`.
 * Another exception is that only `eDisjoint`, `aDisjoint`, `eIntersects`,
 * `aIntersects`, `eDwithin`, and `aDwithin` support the following arguments
 * `(tpose, tpose)`.
 */

#include "pose/tpose_spatialrels.h"

/* MEOS */
#include <meos.h>
#include <meos_internal.h>
#include "temporal/temporal.h" /* For varfunc */
#include "geo/tgeo_spatialfuncs.h"
#include "geo/tgeo_spatialrels.h"
#include "pose/pose.h"
#include "pose/tpose_spatialfuncs.h"
/* MobilityDB */
#include "pg_geo/postgis.h"
#include "pg_geo/tspatial.h"

/*****************************************************************************
 * Generic ever/always spatial relationship functions
 *****************************************************************************/

/**
 * @brief Return true if a pose and a temporal pose ever/always satisfy a
 * spatial relationship
 */
Datum
EA_spatialrel_pose_tpose(FunctionCallInfo fcinfo,
  int (*func)(const Pose *, const Temporal *, bool), bool ever)
{
  Pose *pose = PG_GETARG_POSE_P(0);
  Temporal *temp = PG_GETARG_TEMPORAL_P(1);
  int result = func(pose, temp, ever);
  PG_FREE_IF_COPY(temp, 1);
  if (result < 0)
    PG_RETURN_NULL();
  PG_RETURN_INT32(result);
}

/**
 * @brief Return true if a geometry and a spatiotemporal value ever/always
 * satisfy a spatial relationship
 */
Datum
EA_spatialrel_tpose_pose(FunctionCallInfo fcinfo,
  int (*func)(const Temporal *, const Pose *, bool), bool ever)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  Pose *pose = PG_GETARG_POSE_P(1);
  int result = func(temp, pose, ever);
  PG_FREE_IF_COPY(temp, 0);
  if (result < 0)
    PG_RETURN_NULL();
  PG_RETURN_INT32(result);
}

/*****************************************************************************
 * Ever/always contains
 *****************************************************************************/

/* Econtains_geo_tpose is not supported */

PGDLLEXPORT Datum Acontains_geo_tpose(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Acontains_geo_tpose);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if a geometry always contains a temporal pose
 * @sqlfn aContains()
 */
inline Datum
Acontains_geo_tpose(PG_FUNCTION_ARGS)
{
  return EA_spatialrel_geo_tspatial(fcinfo, &ea_contains_geo_tpose, ALWAYS);
}

/*****************************************************************************
 * Ever/always covers
 *****************************************************************************/

/* Ecovers_geo_tpose is not supported */

PGDLLEXPORT Datum Acovers_geo_tpose(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Acovers_geo_tpose);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if a geometry always covers a temporal pose
 * @sqlfn aCovers()
 */
inline Datum
Acovers_geo_tpose(PG_FUNCTION_ARGS)
{
  return EA_spatialrel_geo_tspatial(fcinfo, &ea_covers_geo_tpose, ALWAYS);
}

PGDLLEXPORT Datum Ecovers_tpose_geo(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Ecovers_tpose_geo);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if a temporal pose ever covers a geometry
 * @sqlfn eCovers()
 */
inline Datum
Ecovers_tpose_geo(PG_FUNCTION_ARGS)
{
  return EA_spatialrel_tspatial_geo(fcinfo, &ea_covers_tpose_geo, EVER);
}

PGDLLEXPORT Datum Acovers_tpose_geo(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Acovers_tpose_geo);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if a temporal pose always covers a geometry
 * @sqlfn aCovers()
 */
inline Datum
Acovers_tpose_geo(PG_FUNCTION_ARGS)
{
  return EA_spatialrel_tspatial_geo(fcinfo, &ea_covers_tpose_geo, ALWAYS);
}

/*****************************************************************************/

PGDLLEXPORT Datum Ecovers_pose_tpose(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Ecovers_pose_tpose);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if a geometry ever covers a temporal pose
 * @sqlfn eCovers()
 */
inline Datum
Ecovers_pose_tpose(PG_FUNCTION_ARGS)
{
  return EA_spatialrel_pose_tpose(fcinfo, &ea_covers_pose_tpose, EVER);
}

PGDLLEXPORT Datum Acovers_pose_tpose(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Acovers_pose_tpose);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if a geometry always covers a temporal pose
 * @sqlfn aCovers()
 */
inline Datum
Acovers_pose_tpose(PG_FUNCTION_ARGS)
{
  return EA_spatialrel_pose_tpose(fcinfo, &ea_covers_pose_tpose, ALWAYS);
}

PGDLLEXPORT Datum Ecovers_tpose_pose(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Ecovers_tpose_pose);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if a temporal pose ever covers a geometry
 * @sqlfn eCovers()
 */
inline Datum
Ecovers_tpose_pose(PG_FUNCTION_ARGS)
{
  return EA_spatialrel_tpose_pose(fcinfo, &ea_covers_tpose_pose, EVER);
}

PGDLLEXPORT Datum Acovers_tpose_pose(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Acovers_tpose_pose);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if a temporal pose always covers a geometry
 * @sqlfn aCovers()
 */
inline Datum
Acovers_tpose_pose(PG_FUNCTION_ARGS)
{
  return EA_spatialrel_tpose_pose(fcinfo, &ea_covers_tpose_pose, ALWAYS);
}

/*****************************************************************************/

PGDLLEXPORT Datum Ecovers_tpose_tpose(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Ecovers_tpose_tpose);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if a temporal pose and a pose 
 * ever touch
 * @sqlfn eTouches()
 */
inline Datum
Ecovers_tpose_tpose(PG_FUNCTION_ARGS)
{
  return EA_spatialrel_tspatial_tspatial(fcinfo, &ea_covers_tpose_tpose, EVER);
}

PGDLLEXPORT Datum Acovers_tpose_tpose(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Acovers_tpose_tpose);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if a temporal pose and a pose 
 * always touch
 * @sqlfn aTouches()
 */
inline Datum
Acovers_tpose_tpose(PG_FUNCTION_ARGS)
{
  return EA_spatialrel_tspatial_tspatial(fcinfo, &ea_covers_tpose_tpose,
    ALWAYS);
}
/*****************************************************************************
 * Ever/always disjoint
 *****************************************************************************/

PGDLLEXPORT Datum Edisjoint_geo_tpose(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Edisjoint_geo_tpose);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if a geometry and a temporal pose are ever
 * disjoint
 * @sqlfn eDisjoint()
 */
inline Datum
Edisjoint_geo_tpose(PG_FUNCTION_ARGS)
{
  return EA_spatialrel_geo_tspatial(fcinfo, &ea_disjoint_geo_tpose, EVER);
}

PGDLLEXPORT Datum Adisjoint_geo_tpose(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Adisjoint_geo_tpose);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if a geometry and a temporal pose are always
 * disjoint
 * @sqlfn aDisjoint()
 */
inline Datum
Adisjoint_geo_tpose(PG_FUNCTION_ARGS)
{
  return EA_spatialrel_geo_tspatial(fcinfo, &ea_disjoint_geo_tpose, ALWAYS);
}

PGDLLEXPORT Datum Edisjoint_tpose_geo(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Edisjoint_tpose_geo);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if a temporal pose and a geometry are ever
 * disjoint
 * @sqlfn eDisjoint()
 */
inline Datum
Edisjoint_tpose_geo(PG_FUNCTION_ARGS)
{
  return EA_spatialrel_tspatial_geo(fcinfo, &ea_disjoint_tpose_geo, EVER);
}

PGDLLEXPORT Datum Adisjoint_tpose_geo(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Adisjoint_tpose_geo);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if a temporal pose and a geometry are always
 * disjoint
 * @sqlfn aDisjoint()
 */
inline Datum
Adisjoint_tpose_geo(PG_FUNCTION_ARGS)
{
  return EA_spatialrel_tspatial_geo(fcinfo, &ea_disjoint_tpose_geo, ALWAYS);
}

/*****************************************************************************/

PGDLLEXPORT Datum Edisjoint_pose_tpose(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Edisjoint_pose_tpose);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if a pose and a temporal pose are ever
 * disjoint
 * @sqlfn eDisjoint()
 */
inline Datum
Edisjoint_pose_tpose(PG_FUNCTION_ARGS)
{
  return EA_spatialrel_pose_tpose(fcinfo, &ea_disjoint_pose_tpose, EVER);
}

PGDLLEXPORT Datum Adisjoint_pose_tpose(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Adisjoint_pose_tpose);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if a pose and a temporal pose are
 * always disjoint
 * @sqlfn aDisjoint()
 */
inline Datum
Adisjoint_pose_tpose(PG_FUNCTION_ARGS)
{
  return EA_spatialrel_pose_tpose(fcinfo, &ea_disjoint_pose_tpose, ALWAYS);
}

PGDLLEXPORT Datum Edisjoint_tpose_pose(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Edisjoint_tpose_pose);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if a temporal pose and a pose are
 * ever disjoint
 * @sqlfn eDisjoint()
 */
inline Datum
Edisjoint_tpose_pose(PG_FUNCTION_ARGS)
{
  return EA_spatialrel_tpose_pose(fcinfo, &ea_disjoint_tpose_pose, EVER);
}

PGDLLEXPORT Datum Adisjoint_tpose_pose(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Adisjoint_tpose_pose);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if a temporal pose and a pose are
 * always disjoint
 * @sqlfn aDisjoint()
 */
inline Datum
Adisjoint_tpose_pose(PG_FUNCTION_ARGS)
{
  return EA_spatialrel_tpose_pose(fcinfo, &ea_disjoint_tpose_pose, ALWAYS);
}

/*****************************************************************************/

PGDLLEXPORT Datum Edisjoint_tpose_tpose(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Edisjoint_tpose_tpose);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if two temporal poses are ever disjoint
 * @sqlfn eDisjoint()
 */
inline Datum
Edisjoint_tpose_tpose(PG_FUNCTION_ARGS)
{
  return EA_spatialrel_tspatial_tspatial(fcinfo, &ea_disjoint_tpose_tpose,
    EVER);
}

PGDLLEXPORT Datum Adisjoint_tpose_tpose(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Adisjoint_tpose_tpose);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if two temporal poses are ever disjoint
 * @sqlfn aDisjoint()
 */
inline Datum
Adisjoint_tpose_tpose(PG_FUNCTION_ARGS)
{
  return EA_spatialrel_tspatial_tspatial(fcinfo, &ea_disjoint_tpose_tpose,
    ALWAYS);
}

/*****************************************************************************
 * Ever/always intersects
 *****************************************************************************/

PGDLLEXPORT Datum Eintersects_geo_tpose(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Eintersects_geo_tpose);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if a geometry and a temporal pose ever
 * intersect
 * @sqlfn eIntersects()
 */
inline Datum
Eintersects_geo_tpose(PG_FUNCTION_ARGS)
{
  return EA_spatialrel_geo_tspatial(fcinfo, &ea_intersects_geo_tpose, EVER);
}

PGDLLEXPORT Datum Aintersects_geo_tpose(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Aintersects_geo_tpose);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if a geometry and a temporal pose ever
 * intersect
 * @sqlfn aIntersects()
 */
inline Datum
Aintersects_geo_tpose(PG_FUNCTION_ARGS)
{
  return EA_spatialrel_geo_tspatial(fcinfo, &ea_intersects_geo_tpose, ALWAYS);
}

PGDLLEXPORT Datum Eintersects_tpose_geo(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Eintersects_tpose_geo);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if a temporal pose and a geometry ever
 * intersect
 * @sqlfn eIntersects()
 */
inline Datum
Eintersects_tpose_geo(PG_FUNCTION_ARGS)
{
  return EA_spatialrel_tspatial_geo(fcinfo, &ea_intersects_tpose_geo, EVER);
}

PGDLLEXPORT Datum Aintersects_tpose_geo(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Aintersects_tpose_geo);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if a temporal pose and a geometry always
 * intersect
 * @sqlfn aIntersects()
 */
inline Datum
Aintersects_tpose_geo(PG_FUNCTION_ARGS)
{
  return EA_spatialrel_tspatial_geo(fcinfo, &ea_intersects_tpose_geo, ALWAYS);
}

PGDLLEXPORT Datum Eintersects_pose_tpose(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Eintersects_pose_tpose);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if a pose and a temporal
 * pose ever intersect
 * @sqlfn eIntersects()
 */
inline Datum
Eintersects_pose_tpose(PG_FUNCTION_ARGS)
{
  return EA_spatialrel_pose_tpose(fcinfo, &ea_intersects_pose_tpose, EVER);
}

PGDLLEXPORT Datum Aintersects_pose_tpose(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Aintersects_pose_tpose);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if a pose and a temporal
 * pose always intersect
 * @sqlfn aIntersects()
 */
inline Datum
Aintersects_pose_tpose(PG_FUNCTION_ARGS)
{
  return EA_spatialrel_pose_tpose(fcinfo, &ea_intersects_pose_tpose, ALWAYS);
}

PGDLLEXPORT Datum Eintersects_tpose_pose(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Eintersects_tpose_pose);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if a temporal pose and a pose 
 * ever intersect
 * @sqlfn eIntersects()
 */
inline Datum
Eintersects_tpose_pose(PG_FUNCTION_ARGS)
{
  return EA_spatialrel_tpose_pose(fcinfo, &ea_intersects_tpose_pose, EVER);
}

PGDLLEXPORT Datum Aintersects_tpose_pose(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Aintersects_tpose_pose);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if a temporal pose and a pose 
 * always intersect
 * @sqlfn aIntersects()
 */
inline Datum
Aintersects_tpose_pose(PG_FUNCTION_ARGS)
{
  return EA_spatialrel_tpose_pose(fcinfo, &ea_intersects_tpose_pose, ALWAYS);
}

/*****************************************************************************/

PGDLLEXPORT Datum Eintersects_tpose_tpose(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Eintersects_tpose_tpose);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if two temporal poses ever intersect
 * @sqlfn eIntersects()
 */
inline Datum
Eintersects_tpose_tpose(PG_FUNCTION_ARGS)
{
  return EA_spatialrel_tspatial_tspatial(fcinfo, &ea_intersects_tpose_tpose,
    EVER);
}

PGDLLEXPORT Datum Aintersects_tpose_tpose(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Aintersects_tpose_tpose);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if two temporal poses ever intersect
 * @sqlfn aIntersects()
 */
inline Datum
Aintersects_tpose_tpose(PG_FUNCTION_ARGS)
{
  return EA_spatialrel_tspatial_tspatial(fcinfo, &ea_intersects_tpose_tpose,
    ALWAYS);
}

/*****************************************************************************
 * Ever/always touches
 *****************************************************************************/

PGDLLEXPORT Datum Etouches_geo_tpose(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Etouches_geo_tpose);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if a geometry and a temporal pose ever touch
 * @sqlfn eTouches()
 */
inline Datum
Etouches_geo_tpose(PG_FUNCTION_ARGS)
{
  return EA_spatialrel_geo_tspatial(fcinfo, &ea_touches_geo_tpose, EVER);
}

PGDLLEXPORT Datum Atouches_geo_tpose(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Atouches_geo_tpose);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if a geometry and a temporal pose ever touch
 * @sqlfn aTouches()
 */
inline Datum
Atouches_geo_tpose(PG_FUNCTION_ARGS)
{
  return EA_spatialrel_geo_tspatial(fcinfo, &ea_touches_geo_tpose, EVER);
}

PGDLLEXPORT Datum Etouches_tpose_geo(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Etouches_tpose_geo);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if a temporal pose and a geometry ever touch
 * @sqlfn eTouches()
 */
inline Datum
Etouches_tpose_geo(PG_FUNCTION_ARGS)
{
  return EA_spatialrel_tspatial_geo(fcinfo, &ea_touches_tpose_geo, EVER);
}

PGDLLEXPORT Datum Atouches_tpose_geo(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Atouches_tpose_geo);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if a temporal pose and a geometry always touch
 * @sqlfn aTouches()
 */
inline Datum
Atouches_tpose_geo(PG_FUNCTION_ARGS)
{
  return EA_spatialrel_tspatial_geo(fcinfo, &ea_touches_tpose_geo, ALWAYS);
}

/*****************************************************************************
 * Ever/always dwithin
 * The function only accepts points and not arbitrary geometries
 *****************************************************************************/

/**
 * @brief Return true if a geometry and a temporal pose are
 * ever/always within a distance
 * @sqlfn eDwithin()
 */
static Datum
EA_dwithin_geo_tpose(FunctionCallInfo fcinfo, bool ever)
{
  GSERIALIZED *gs = PG_GETARG_GSERIALIZED_P(0);
  Temporal *temp = PG_GETARG_TEMPORAL_P(1);
  double dist = PG_GETARG_FLOAT8(2);
  int result = ever ?
    edwithin_tpose_geo(temp, gs, dist) :
    adwithin_tpose_geo(temp, gs, dist);
  PG_FREE_IF_COPY(gs, 0);
  PG_FREE_IF_COPY(temp, 1);
  if (result < 0)
    PG_RETURN_NULL();
  PG_RETURN_BOOL(result);
}

PGDLLEXPORT Datum Edwithin_geo_tpose(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Edwithin_geo_tpose);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if a geometry and a temporal pose are ever
 * within a distance
 * @sqlfn eDwithin()
 */
inline Datum
Edwithin_geo_tpose(PG_FUNCTION_ARGS)
{
  return EA_dwithin_geo_tpose(fcinfo, EVER);
}

PGDLLEXPORT Datum Adwithin_geo_tpose(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Adwithin_geo_tpose);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if a geometry and a temporal pose are always
 * within a distance
 * @sqlfn aDwithin()
 */
inline Datum
Adwithin_geo_tpose(PG_FUNCTION_ARGS)
{
  return EA_dwithin_geo_tpose(fcinfo, ALWAYS);
}

/**
 * @brief Return true if a temporal pose and a geometry are
 * ever/always within a distance
 * @sqlfn eDwithin()
 */
static Datum
EA_dwithin_tpose_geo(FunctionCallInfo fcinfo, bool ever)
{
  GSERIALIZED *gs = PG_GETARG_GSERIALIZED_P(1);
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  double dist = PG_GETARG_FLOAT8(2);
  int result = ever ? edwithin_tpose_geo(temp, gs, dist) :
    adwithin_tpose_geo(temp, gs, dist);
  PG_FREE_IF_COPY(temp, 0);
  PG_FREE_IF_COPY(gs, 1);
  if (result < 0)
    PG_RETURN_NULL();
  PG_RETURN_BOOL(result);
}

PGDLLEXPORT Datum Edwithin_tpose_geo(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Edwithin_tpose_geo);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if a temporal pose and a geometry are ever
 * within a distance
 * @sqlfn eDwithin()
 */
inline Datum
Edwithin_tpose_geo(PG_FUNCTION_ARGS)
{
  return EA_dwithin_tpose_geo(fcinfo, EVER);
}

PGDLLEXPORT Datum Adwithin_tpose_geo(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Adwithin_tpose_geo);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if a temporal pose and a geometry are always
 * within a distance
 * @sqlfn aDwithin()
 */
inline Datum
Adwithin_tpose_geo(PG_FUNCTION_ARGS)
{
  return EA_dwithin_tpose_geo(fcinfo, ALWAYS);
}

/**
 * @brief Return true if two temporal poses are even/always within a
 * distance
 * @sqlfn eDwithin(), aDwithin()
 */
static Datum
EA_dwithin_tpose_tpose(FunctionCallInfo fcinfo, bool ever)
{
  Temporal *temp1 = PG_GETARG_TEMPORAL_P(0);
  Temporal *temp2 = PG_GETARG_TEMPORAL_P(1);
  double dist = PG_GETARG_FLOAT8(2);
  int result = ever ? edwithin_tpose_tpose(temp1, temp2, dist) :
    adwithin_tpose_tpose(temp1, temp2, dist);
  PG_FREE_IF_COPY(temp1, 0);
  PG_FREE_IF_COPY(temp2, 1);
  if (result < 0)
    PG_RETURN_NULL();
  PG_RETURN_BOOL(result);
}

/*****************************************************************************/

/**
 * @brief Return true if a pose and a temporal pose are
 * ever/always within a distance
 * @sqlfn eDwithin()
 */
static Datum
EA_dwithin_pose_tpose(FunctionCallInfo fcinfo, bool ever)
{
  Pose *pose = PG_GETARG_POSE_P(0);
  Temporal *temp = PG_GETARG_TEMPORAL_P(1);
  double dist = PG_GETARG_FLOAT8(2);
  int result = ever ?
    edwithin_tpose_pose(temp, pose, dist) :
    adwithin_tpose_pose(temp, pose, dist);
  PG_FREE_IF_COPY(pose, 0);
  PG_FREE_IF_COPY(temp, 1);
  if (result < 0)
    PG_RETURN_NULL();
  PG_RETURN_BOOL(result);
}

PGDLLEXPORT Datum Edwithin_pose_tpose(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Edwithin_pose_tpose);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if a pose and a temporal pose are
 * ever within a distance
 * @sqlfn eDwithin()
 */
inline Datum
Edwithin_pose_tpose(PG_FUNCTION_ARGS)
{
  return EA_dwithin_pose_tpose(fcinfo, EVER);
}

PGDLLEXPORT Datum Adwithin_pose_tpose(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Adwithin_pose_tpose);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if a pose and a temporal pose are
 * always within a distance
 * @sqlfn aDwithin()
 */
inline Datum
Adwithin_pose_tpose(PG_FUNCTION_ARGS)
{
  return EA_dwithin_pose_tpose(fcinfo, ALWAYS);
}

/**
 * @brief Return true if a temporal pose and a pose are
 * ever/always within a distance
 * @sqlfn eDwithin()
 */
Datum
EA_dwithin_tpose_pose(FunctionCallInfo fcinfo, bool ever)
{
  Pose *pose = PG_GETARG_POSE_P(1);
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  double dist = PG_GETARG_FLOAT8(2);
  int result = ever ? edwithin_tpose_pose(temp, pose, dist) :
    adwithin_tpose_pose(temp, pose, dist);
  PG_FREE_IF_COPY(temp, 0);
  PG_FREE_IF_COPY(pose, 1);
  if (result < 0)
    PG_RETURN_NULL();
  PG_RETURN_BOOL(result);
}

PGDLLEXPORT Datum Edwithin_tpose_pose(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Edwithin_tpose_pose);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if a temporal pose and a pose are
 * ever within a distance
 * @sqlfn eDwithin()
 */
inline Datum
Edwithin_tpose_pose(PG_FUNCTION_ARGS)
{
  return EA_dwithin_tpose_pose(fcinfo, EVER);
}

PGDLLEXPORT Datum Adwithin_tpose_pose(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Adwithin_tpose_pose);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if a temporal pose and a pose are
 * always within a distance
 * @sqlfn aDwithin()
 */
inline Datum
Adwithin_tpose_pose(PG_FUNCTION_ARGS)
{
  return EA_dwithin_tpose_pose(fcinfo, ALWAYS);
}

/*****************************************************************************/

PGDLLEXPORT Datum Edwithin_tpose_tpose(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Edwithin_tpose_tpose);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if two temporal poses are ever within a distance
 * @sqlfn eDwithin()
 */
inline Datum
Edwithin_tpose_tpose(PG_FUNCTION_ARGS)
{
  return EA_dwithin_tpose_tpose(fcinfo, EVER);
}

PGDLLEXPORT Datum Adwithin_tpose_tpose(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Adwithin_tpose_tpose);
/**
 * @ingroup mobilitydb_pose_rel_ever
 * @brief Return true if two temporal poses are always within a distance
 * @sqlfn aDwithin()
 */
inline Datum
Adwithin_tpose_tpose(PG_FUNCTION_ARGS)
{
  return EA_dwithin_tpose_tpose(fcinfo, ALWAYS);
}

/*****************************************************************************/
