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
 * @brief Spatial functions for temporal geometries/geographies.
 */

/* PostgreSQL */
#include <postgres.h>
/* MEOS */
#include <meos.h>
#include <meos_geo.h>
#include <meos_internal.h>
#include "general/span.h"
#include "point/stbox.h"
#include "point/tpoint_restrfuncs.h"
// #include "geo/tgeo_spatialfuncs.h"
/* MobilityDB */
#include "pg_general/temporal.h"
#include "pg_point/postgis.h"

/*****************************************************************************
 * Geometric positions (Trajectory) functions
 * Return the geometric positions covered by a temporal geometry/geography
 *****************************************************************************/

PGDLLEXPORT Datum Tgeo_traversed_area(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tgeo_traversed_area);
/**
 * @ingroup mobilitydb_temporal_spatial_accessor
 * @brief Return the geometry covered by a temporal geometry/geography
 * @sqlfn traversedArea()
 */
Datum
Tgeo_traversed_area(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  GSERIALIZED *result = tgeo_traversed_area(temp);
  PG_FREE_IF_COPY(temp, 0);
  PG_RETURN_TEMPORAL_P(result);
}

/*****************************************************************************
 * Length functions
 *****************************************************************************/

// PGDLLEXPORT Datum Tgeo_length(PG_FUNCTION_ARGS);
// PG_FUNCTION_INFO_V1(Tgeo_length);
// /**
 // * @ingroup mobilitydb_temporal_spatial_accessor
 // * @brief Return the length traversed by a temporal geometry/geography
 // * @sqlfn length()
 // */
// Datum
// Tgeo_length(PG_FUNCTION_ARGS)
// {
  // Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  // double result = tgeo_length(temp);
  // PG_FREE_IF_COPY(temp, 0);
  // PG_RETURN_FLOAT8(result);
// }

// PGDLLEXPORT Datum Tgeo_cumulative_length(PG_FUNCTION_ARGS);
// PG_FUNCTION_INFO_V1(Tgeo_cumulative_length);
// /**
 // * @ingroup mobilitydb_temporal_spatial_accessor
 // * @brief Return the cumulative length traversed by a temporal geometry/geography
 // * @sqlfn cumulativeLength()
 // */
// Datum
// Tgeo_cumulative_length(PG_FUNCTION_ARGS)
// {
  // Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  // Temporal *result = tgeo_cumulative_length(temp);
  // PG_FREE_IF_COPY(temp, 0);
  // PG_RETURN_TEMPORAL_P(result);
// }

/*****************************************************************************
 * Speed functions
 *****************************************************************************/

// PGDLLEXPORT Datum Tgeo_speed(PG_FUNCTION_ARGS);
// PG_FUNCTION_INFO_V1(Tgeo_speed);
// /**
 // * @ingroup mobilitydb_temporal_spatial_accessor
 // * @brief Return the speed of a temporal geometry/geography
 // * @sqlfn speed()
 // */
// Datum
// Tgeo_speed(PG_FUNCTION_ARGS)
// {
  // Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  // Temporal *result = tgeo_speed(temp);
  // PG_FREE_IF_COPY(temp, 0);
  // if (! result)
    // PG_RETURN_NULL();
  // PG_RETURN_TEMPORAL_P(result);
// }

/*****************************************************************************
 * Time-weighed centroid for temporal geometries/geographies
 *****************************************************************************/

// PGDLLEXPORT Datum Tgeo_twcentroid(PG_FUNCTION_ARGS);
// PG_FUNCTION_INFO_V1(Tgeo_twcentroid);
// /**
 // * @ingroup mobilitydb_temporal_agg
 // * @brief Return the time-weighed centroid of a temporal geometry/geography
 // * @sqlfn twCentroid()
 // */
// Datum
// Tgeo_twcentroid(PG_FUNCTION_ARGS)
// {
  // Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  // GSERIALIZED *result = tgeo_twcentroid(temp);
  // PG_FREE_IF_COPY(temp, 0);
  // PG_RETURN_GSERIALIZED_P(result);
// }

/*****************************************************************************
 * Temporal azimuth
 *****************************************************************************/

// PGDLLEXPORT Datum Tgeo_azimuth(PG_FUNCTION_ARGS);
// PG_FUNCTION_INFO_V1(Tgeo_azimuth);
// /**
 // * @ingroup mobilitydb_temporal_spatial_accessor
 // * @brief Return the temporal azimuth of a temporal geometry/geography
 // * @sqlfn azimuth()
 // */
// Datum
// Tgeo_azimuth(PG_FUNCTION_ARGS)
// {
  // Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  // Temporal *result = tgeo_azimuth(temp);
  // PG_FREE_IF_COPY(temp, 0);
  // if (! result)
    // PG_RETURN_NULL();
  // PG_RETURN_TEMPORAL_P(result);
// }

/*****************************************************************************
 * Restriction functions
 *****************************************************************************/

// /**
 // * @brief Return a temporal geometry/geography restricted to (the complement of) a
 // * geometry
 // */
// static Datum
// Tgeo_restrict_geom(FunctionCallInfo fcinfo, bool atfunc)
// {
  // Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  // GSERIALIZED *gs = PG_GETARG_GSERIALIZED_P(1);
  // Temporal *result = tgeo_restrict_geom(temp, gs, NULL, atfunc);
  // PG_FREE_IF_COPY(temp, 0);
  // PG_FREE_IF_COPY(gs, 1);
  // if (! result)
    // PG_RETURN_NULL();
  // PG_RETURN_TEMPORAL_P(result);
// }

// PGDLLEXPORT Datum Tgeo_at_geom(PG_FUNCTION_ARGS);
// PG_FUNCTION_INFO_V1(Tgeo_at_geom);
// /**
 // * @ingroup mobilitydb_temporal_restrict
 // * @brief Return a temporal geometry/geography restricted to a geometry
 // * @sqlfn atGeometry()
 // */
// Datum
// Tgeo_at_geom(PG_FUNCTION_ARGS)
// {
  // return Tgeo_restrict_geom(fcinfo, REST_AT);
// }

// PGDLLEXPORT Datum Tgeo_minus_geom(PG_FUNCTION_ARGS);
// PG_FUNCTION_INFO_V1(Tgeo_minus_geom);
// /**
 // * @ingroup mobilitydb_temporal_restrict
 // * @brief Return a temporal geometry/geography restricted to the complement of a
 // * geometry
 // * @sqlfn minusGeometry()
 // */
// Datum
// Tgeo_minus_geom(PG_FUNCTION_ARGS)
// {
  // return Tgeo_restrict_geom(fcinfo, REST_MINUS);
// }

/*****************************************************************************/

// PGDLLEXPORT Datum Tgeo_at_stbox(PG_FUNCTION_ARGS);
// PG_FUNCTION_INFO_V1(Tgeo_at_stbox);
// /**
 // * @ingroup mobilitydb_temporal_restrict
 // * @brief Return a temporal geometry/geography restricted to a spatiotemporal box
 // * @sqlfn atStbox()
 // */
// Datum
// Tgeo_at_stbox(PG_FUNCTION_ARGS)
// {
  // Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  // STBox *box = PG_GETARG_STBOX_P(1);
  // bool border_inc = PG_GETARG_BOOL(2);
  // Temporal *result = tgeo_restrict_stbox(temp, box, border_inc, REST_AT);
  // PG_FREE_IF_COPY(temp, 0);
  // if (! result)
    // PG_RETURN_NULL();
  // PG_RETURN_TEMPORAL_P(result);
// }

// PGDLLEXPORT Datum Tgeo_minus_stbox(PG_FUNCTION_ARGS);
// PG_FUNCTION_INFO_V1(Tgeo_minus_stbox);
// /**
 // * @ingroup mobilitydb_temporal_restrict
 // * @brief Return a temporal geometry/geography restricted to the complement of a
 // * spatiotemporal box
 // * @sqlfn minusStbox()
 // */
// Datum
// Tgeo_minus_stbox(PG_FUNCTION_ARGS)
// {
  // Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  // STBox *box = PG_GETARG_STBOX_P(1);
  // bool border_inc = PG_GETARG_BOOL(2);
  // Temporal *result = tgeo_restrict_stbox(temp, box, border_inc, REST_MINUS);
  // PG_FREE_IF_COPY(temp, 0);
  // if (! result)
    // PG_RETURN_NULL();
  // PG_RETURN_TEMPORAL_P(result);
// }

/*****************************************************************************/
