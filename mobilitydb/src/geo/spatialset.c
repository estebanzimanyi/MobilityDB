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

PGDLLEXPORT Datum Spatialset_to_stbox(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Spatialset_to_stbox);
/**
 * @ingroup mobilitydb_geo_set
 * @brief Return a spatial set converted to a spatiotemporal box
 * @sqlfn stbox()
 * @sqlop @p ::
 */
Datum
Spatialset_to_stbox(PG_FUNCTION_ARGS)
{
  Set *set = PG_GETARG_SET_P(0);
  STBox *result = spatialset_stbox(set);
  PG_FREE_IF_COPY(set, 0);
  PG_RETURN_STBOX_P(result);
}

/*****************************************************************************
 * Spatial reference system functions for spatial sets
 *****************************************************************************/

PGDLLEXPORT Datum Spatialset_srid(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Spatialset_srid);
/**
 * @ingroup mobilitydb_geo_set_srid
 * @brief Return the SRID of a spatial set
 * @sqlfn SRID()
 */
Datum
Spatialset_srid(PG_FUNCTION_ARGS)
{
  Set *s = PG_GETARG_SET_P(0);
  int result = spatialset_srid(s);
  PG_FREE_IF_COPY(s, 0);
  PG_RETURN_INT32(result);
}

PGDLLEXPORT Datum Spatialset_set_srid(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Spatialset_set_srid);
/**
 * @ingroup mobilitydb_geo_set_srid
 * @brief Return a spatial set with the values set to an SRID
 * @sqlfn setSRID()
 */
Datum
Spatialset_set_srid(PG_FUNCTION_ARGS)
{
  Set *s = PG_GETARG_SET_P(0);
  int32 srid = PG_GETARG_INT32(1);
  Set *result = spatialset_set_srid(s, srid);
  PG_FREE_IF_COPY(s, 0);
  PG_RETURN_SET_P(result);
}

PGDLLEXPORT Datum Spatialset_transform(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Spatialset_transform);
/**
 * @ingroup mobilitydb_geo_set_srid
 * @brief Return a spatial set transformed to an SRID
 * @sqlfn transform()
 */
Datum
Spatialset_transform(PG_FUNCTION_ARGS)
{
  Set *s = PG_GETARG_SET_P(0);
  int32 srid = PG_GETARG_INT32(1);
  Set *result = spatialset_transform(s, srid);
  PG_FREE_IF_COPY(s, 0);
  if (! result)
    PG_RETURN_NULL();
  PG_RETURN_SET_P(result);
}

PGDLLEXPORT Datum Spatialset_transform_pipeline(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Spatialset_transform_pipeline);
/**
 * @ingroup mobilitydb_geo_set_srid
 * @brief Return a spatial set transformed to an SRID using a transformation
 * pipeline
 * @sqlfn transformPipeline()
 */
Datum
Spatialset_transform_pipeline(PG_FUNCTION_ARGS)
{
  Set *s = PG_GETARG_SET_P(0);
  text *pipelinetxt = PG_GETARG_TEXT_P(1);
  int32_t srid = PG_GETARG_INT32(2);
  bool is_forward = PG_GETARG_BOOL(3);
  char *pipelinestr = text2cstring(pipelinetxt);
  Set *result = spatialset_transform_pipeline(s, pipelinestr, srid, is_forward);
  pfree(pipelinestr);
  PG_FREE_IF_COPY(s, 0);
  PG_FREE_IF_COPY(pipelinetxt, 1);
  if (! result)
    PG_RETURN_NULL();
  PG_RETURN_SET_P(result);
}

/*****************************************************************************/
