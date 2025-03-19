/***********************************************************************
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
 * @brief Functions for spatial reference systems for spatial values, that is,
 * spatial sets, spatiotemporal boxes, temporal points, and temporal geos
 */

/* C */
#include <assert.h>
/* PostgreSQL */
#include <postgres.h>
#include <funcapi.h>
/* PostGIS */
#include <liblwgeom.h>
/* MEOS */
#include <meos.h>
#include <meos_internal.h>
#include "general/set.h"
#include "general/span.h"
#include "general/type_util.h"
#include "geo/tgeo_spatialfuncs.h"
#include "geo/stbox.h"
#include "geo/tpoint_restrfuncs.h"
/* MobilityDB */
#include "pg_general/temporal.h"
#include "pg_general/type_util.h"
#include "pg_geo/postgis.h"

/*****************************************************************************
 * Conversion functions
 *****************************************************************************/

PGDLLEXPORT Datum Tspatial_to_stbox(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tspatial_to_stbox);
/**
 * @ingroup mobilitydb_geo_conversion
 * @brief Return a temporal spatial value converted to a spatiotemporal box
 * @sqlfn stbox()
 * @sqlop @p ::
 */
Datum
Tspatial_to_stbox(PG_FUNCTION_ARGS)
{
  Datum tempdatum = PG_GETARG_DATUM(0);
  Temporal *temp = temporal_slice(tempdatum);
  STBox *result = palloc(sizeof(STBox));
  tspatial_set_stbox(temp, result);
  PG_RETURN_STBOX_P(result);
}

/*****************************************************************************
 * Transformation functions
 *****************************************************************************/

PGDLLEXPORT Datum Tspatial_expand_space(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tspatial_expand_space);
/**
 * @ingroup mobilitydb_geo_transf
 * @brief Return the bounding box of a temporal spatial value expanded on the
 * spatial dimension by a value
 * @sqlfn expandSpace()
 */
Datum
Tspatial_expand_space(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  double d = PG_GETARG_FLOAT8(1);
  STBox *result = tspatial_expand_space(temp, d);
  PG_FREE_IF_COPY(temp, 0);
  if (! result)
    PG_RETURN_NULL();
  PG_RETURN_STBOX_P(result);
}

/*****************************************************************************
 * Spatial reference system functions for temporal spatial types
 *****************************************************************************/

PGDLLEXPORT Datum Tspatial_srid(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tspatial_srid);
/**
 * @ingroup mobilitydb_geo_srid
 * @brief Return the SRID of a temporal spatial value
 * @sqlfn SRID()
 */
Datum
Tspatial_srid(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  int result = tspatial_srid(temp);
  PG_FREE_IF_COPY(temp, 0);
  PG_RETURN_INT32(result);
}

PGDLLEXPORT Datum Tspatial_set_srid(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tspatial_set_srid);
/**
 * @ingroup mobilitydb_geo_srid
 * @brief Return a temporal spatial value with the coordinates set to an SRID
 * @sqlfn setSRID()
 */
Datum
Tspatial_set_srid(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  int32 srid = PG_GETARG_INT32(1);
  Temporal *result = tspatial_set_srid(temp, srid);
  PG_FREE_IF_COPY(temp, 0);
  PG_RETURN_TEMPORAL_P(result);
}

PGDLLEXPORT Datum Tspatial_transform(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tspatial_transform);
/**
 * @ingroup mobilitydb_geo_srid
 * @brief Return a temporal circular buffer transformed to an SRID
 * @sqlfn transform()
 */
Datum
Tspatial_transform(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  int32_t srid = PG_GETARG_INT32(1);
  Temporal *result = tspatial_transform(temp, srid);
  PG_FREE_IF_COPY(temp, 0);
  PG_RETURN_TEMPORAL_P(result);
}

PGDLLEXPORT Datum Tspatial_transform_pipeline(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tspatial_transform_pipeline);
/**
 * @ingroup mobilitydb_geo_srid
 * @brief Return a temporal circular buffer transformed to an SRID using a
 * transformation pipeline
 * @sqlfn transformPipeline()
 */
Datum
Tspatial_transform_pipeline(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  text *pipelinetxt = PG_GETARG_TEXT_P(1);
  int32_t srid = PG_GETARG_INT32(2);
  bool is_forward = PG_GETARG_BOOL(3);
  char *pipelinestr = text2cstring(pipelinetxt);
  Temporal *result = tspatial_transform_pipeline(temp, pipelinestr, srid,
    is_forward);
  pfree(pipelinestr);
  PG_FREE_IF_COPY(temp, 0);
  PG_FREE_IF_COPY(pipelinetxt, 1);
  PG_RETURN_TEMPORAL_P(result);
}

/*****************************************************************************/
