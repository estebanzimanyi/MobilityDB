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
 * @brief Spatial functions for temporal rigid geometries
 */

/* PostgreSQL */
#include <postgres.h>
/* MEOS */
#include <meos.h>
#include <meos_internal.h>
#include <meos_rgeo.h>
#include "temporal/tsequence.h"
#include "geo/stbox.h"
#include "geo/tgeo_spatialfuncs.h"
#include "rgeo/trgeo.h"
#include "rgeo/trgeo_spatialfuncs.h"
#include "rgeo/trgeo_utils.h"
/* MobilityDB */
#include "pg_temporal/temporal.h"
#include "pg_geo/postgis.h"

/*****************************************************************************
 * SRID functions
 *****************************************************************************/

PGDLLEXPORT Datum Trgeo_set_srid(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_set_srid);
/**
 * @ingroup mobilitydb_geo_srid
 * @brief Return a temporal rigid_geometry with the coordinates set to an SRID
 * @sqlfn setSRID()
 */
Datum
Trgeo_set_srid(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  int32 srid = PG_GETARG_INT32(1);
  Temporal *result = trgeo_set_srid(temp, srid);
  PG_FREE_IF_COPY(temp, 0);
  PG_RETURN_TEMPORAL_P(result);
}

PGDLLEXPORT Datum Trgeo_transform(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_transform);
/**
 * @ingroup mobilitydb_geo_srid
 * @brief Return a temporal rigid geometry transformed to an SRID
 * @sqlfn transform()
 */
Datum
Trgeo_transform(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  int32_t srid = PG_GETARG_INT32(1);
  Temporal *result = trgeo_transform(temp, srid);
  PG_FREE_IF_COPY(temp, 0);
  PG_RETURN_TEMPORAL_P(result);
}

PGDLLEXPORT Datum Trgeo_transform_pipeline(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_transform_pipeline);
/**
 * @ingroup mobilitydb_geo_srid
 * @brief Return a temporal rigid geometry transformed to an SRID using a
 * pipeline
 * @sqlfn transformPipeline()
 */
Datum
Trgeo_transform_pipeline(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  text *pipelinetxt = PG_GETARG_TEXT_P(1);
  int32_t srid = PG_GETARG_INT32(2);
  bool is_forward = PG_GETARG_BOOL(3);
  char *pipelinestr = text2cstring(pipelinetxt);
  Temporal *result = trgeo_transform_pipeline(temp, pipelinestr, srid,
    is_forward);
  pfree(pipelinestr);
  PG_FREE_IF_COPY(temp, 0);
  PG_FREE_IF_COPY(pipelinetxt, 1);
  PG_RETURN_TEMPORAL_P(result);
}

/*****************************************************************************
 * Trajectory functions
 *****************************************************************************/

PGDLLEXPORT Datum Trgeo_trajectory_center(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_trajectory_center);
/**
 * @ingroup mobilitydb_rgeo_accessor
 * @brief Return the trajectory of the center of rotation a temporal rigid
 * geometry
 * @sqlfn trajectoryCenter()
 */
PGDLLEXPORT Datum
Trgeo_trajectory_center(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  Temporal *result = trgeo_trajectory_center(temp);
  PG_FREE_IF_COPY(temp, 0);
  PG_RETURN_POINTER(result);
}

/*****************************************************************************
 * Traversed area functions
 *****************************************************************************/

PGDLLEXPORT Datum Trgeo_traversed_area(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_traversed_area);
/**
 * @ingroup mobilitydb_rgeo_accessor
 * @brief Return a temporal rigid geometry restricted to a geometry
 * @sqlfn atGeometry()
 */
Datum
Trgeo_traversed_area(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  bool unary_union = false;
  if (PG_NARGS() > 1 && ! PG_ARGISNULL(1))
    unary_union = PG_GETARG_BOOL(1);
  GSERIALIZED *result = trgeo_traversed_area(temp, unary_union);
  PG_FREE_IF_COPY(temp, 0);
  if (! result)
    PG_RETURN_NULL();
  PG_RETURN_GSERIALIZED_P(result);
}

// PGDLLEXPORT Datum Trgeo_trajectory(PG_FUNCTION_ARGS);
// PG_FUNCTION_INFO_V1(Trgeo_trajectory);
// /**
 // * @ingroup mobilitydb_rgeo_accessor
 // * @brief Return the trajectory of a temporal rigid geometry
 // * @sqlfn trajectory()
 // */
// PGDLLEXPORT Datum
// Trgeo_trajectory(PG_FUNCTION_ARGS)
// {
  // Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  // Temporal *tpoint = PG_GETARG_TEMPORAL_P(1);
  // int32 n = PG_GETARG_INT32(2);
  // Temporal *result = trgeo_trajectory(temp, (TInstant *) tpoint, n);
  // PG_FREE_IF_COPY(temp, 0);
  // PG_FREE_IF_COPY(tpoint, 1);
  // PG_RETURN_POINTER(result);
// }

// PGDLLEXPORT Datum Trgeo_trajectory_center(PG_FUNCTION_ARGS);
// PG_FUNCTION_INFO_V1(Trgeo_trajectory_center);
// /**
 // * @ingroup mobilitydb_rgeo_accessor
 // * @brief Return the center of the trajectory of a temporal rigid geometry
 // * @sqlfn atGeometry()
 // */
// PGDLLEXPORT Datum
// Trgeo_trajectory_center(PG_FUNCTION_ARGS)
// {
  // Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  // Temporal *result = trgeo_trajectory_center(temp);
  // PG_FREE_IF_COPY(temp, 0);
  // PG_RETURN_POINTER(result);
// }

/*****************************************************************************
 * Restriction functions
 *****************************************************************************/

/**
 * @brief Return a temporal rigid geometry restricted to (the complement of) a
 * geometry
 */
static Datum
Trgeo_restrict_geom(FunctionCallInfo fcinfo, bool atfunc)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  GSERIALIZED *gs = PG_GETARG_GSERIALIZED_P(1);
  Temporal *result = trgeo_restrict_geom(temp, gs, atfunc);
  PG_FREE_IF_COPY(temp, 0);
  PG_FREE_IF_COPY(gs, 1);
  if (! result)
    PG_RETURN_NULL();
  PG_RETURN_TEMPORAL_P(result);
}

PGDLLEXPORT Datum Trgeo_at_geom(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_at_geom);
/**
 * @ingroup mobilitydb_rgeo_restrict
 * @brief Return a temporal rigid geometry restricted to a geometry
 * @sqlfn atGeometry()
 */
inline Datum
Trgeo_at_geom(PG_FUNCTION_ARGS)
{
  return Trgeo_restrict_geom(fcinfo, REST_AT);
}

PGDLLEXPORT Datum Trgeo_minus_geom(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_minus_geom);
/**
 * @ingroup mobilitydb_rgeo_restrict
 * @brief Return a temporal rigid geometry restricted to the complement of a
 * geometry
 * @sqlfn minusGeometry()
 */
inline Datum
Trgeo_minus_geom(PG_FUNCTION_ARGS)
{
  return Trgeo_restrict_geom(fcinfo, REST_MINUS);
}

/*****************************************************************************/

/**
 * @brief Return a temporal rigid geometry restricted to a spatiotemporal box
 */
static Datum
Trgeo_restrict_stbox(FunctionCallInfo fcinfo, bool atfunc)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  STBox *box = PG_GETARG_STBOX_P(1);
  bool border_inc = PG_GETARG_BOOL(2);
  Temporal *result = trgeo_restrict_stbox(temp, box, border_inc, atfunc);
  PG_FREE_IF_COPY(temp, 0);
  if (! result)
    PG_RETURN_NULL();
  PG_RETURN_TEMPORAL_P(result);
}

PGDLLEXPORT Datum Trgeo_at_stbox(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_at_stbox);
/**
 * @ingroup mobilitydb_rgeo_restrict
 * @brief Return a temporal rigid geometry restricted to a spatiotemporal box
 * @sqlfn atStbox()
 */
inline Datum
Trgeo_at_stbox(PG_FUNCTION_ARGS)
{
  return Trgeo_restrict_stbox(fcinfo, REST_AT);
}

PGDLLEXPORT Datum Trgeo_minus_stbox(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_minus_stbox);
/**
 * @ingroup mobilitydb_rgeo_restrict
 * @brief Return a temporal rigid geometry restricted to the complement of a
 * spatiotemporal box
 * @sqlfn minusStbox()
 */
inline Datum
Trgeo_minus_stbox(PG_FUNCTION_ARGS)
{
  return Trgeo_restrict_stbox(fcinfo, REST_MINUS);
}

/*****************************************************************************/
