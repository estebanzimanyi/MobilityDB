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
 * @brief Input and output of temporal poses in WKT and EWKT
 */

/* PostgreSQL */
#include <postgres.h>
#include <fmgr.h>
#include <utils/array.h>
/* PostGIS */
#include <liblwgeom_internal.h>
/* MEOS */
#include <meos.h>
#include <meos_internal.h>
#include "general/set.h"
#include "general/temporal.h"
#include "geo/tgeo_parser.h"
#include "pose/tpose.h"
#include "pose/tpose_parser.h"
#include "pose/tpose_spatialfuncs.h"
/* MobilityDB */
#include "pg_general/meos_catalog.h" /* For oid_type */
#include "pg_general/type_util.h"

/*****************************************************************************
 * Input in EWKT format
 *****************************************************************************/

PGDLLEXPORT Datum Tpose_from_ewkt(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tpose_from_ewkt);
/**
 * @ingroup mobilitydb_temporal_inout
 * @brief Return a temporal pose from its Extended Well-Known Text (EWKT)
 * representation
 * @note This just does the same thing as the _in function, except it has to handle
 * a 'text' input. First, unwrap the text into a cstring, then do as tpoint_in
 * @sqlfn tposeFromText(), tgeompointFromEWKT(), tposeFromEWKT()
 */
Datum
Tpose_from_ewkt(PG_FUNCTION_ARGS)
{
  text *wkt_text = PG_GETARG_TEXT_P(0);
  char *wkt = text2cstring(wkt_text);
  /* Copy the pointer since it will be advanced during parsing */
  const char *wkt_ptr = wkt;
  Temporal *result = tpose_parse(&wkt_ptr);
  pfree(wkt);
  PG_FREE_IF_COPY(wkt_text, 0);
  PG_RETURN_TEMPORAL_P(result);
}

/*****************************************************************************
 * Output in WKT and EWKT representation
 *****************************************************************************/

/**
 * @brief Return the (Extended) Well-Known Text (WKT or EWKT) representation of
 * a pose
 * @sqlfn asText()
 */
static Datum
Pose_as_text_ext(FunctionCallInfo fcinfo, bool extended)
{
  Pose *pose = PG_GETARG_POSE_P(0);
  int dbl_dig_for_wkt = OUT_DEFAULT_DECIMAL_DIGITS;
  if (PG_NARGS() > 1 && ! PG_ARGISNULL(1))
    dbl_dig_for_wkt = PG_GETARG_INT32(1);
  char *str = extended ? pose_as_ewkt(pose, dbl_dig_for_wkt) : 
    pose_as_text(pose, dbl_dig_for_wkt);
  text *result = cstring2text(str);
  pfree(str);
  PG_FREE_IF_COPY(pose, 0);
  PG_RETURN_TEXT_P(result);
}

PGDLLEXPORT Datum Pose_as_text(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Pose_as_text);
/**
 * @ingroup mobilitydb_temporal_inout
 * @brief Return the Well-Known Text (WKT) representation of a pose
 * @sqlfn asText()
 */
Datum
Pose_as_text(PG_FUNCTION_ARGS)
{
  return Pose_as_text_ext(fcinfo, false);
}

PGDLLEXPORT Datum Pose_as_ewkt(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Pose_as_ewkt);
/**
 * @ingroup mobilitydb_temporal_inout
 * @brief Return the Extended Well-Known Text (EWKT) representation of a
 * pose
 * @note It is the WKT representation prefixed with the SRID
 * @sqlfn asEWKT()
 */
Datum
Pose_as_ewkt(PG_FUNCTION_ARGS)
{
  return Pose_as_text_ext(fcinfo, true);
}

/*****************************************************************************/

PGDLLEXPORT Datum Poseset_as_text(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Poseset_as_text);
/**
 * @ingroup mobilitydb_setspan_inout
 * @brief Return the Well-Known Text (WKT) representation of a circular 
 * buffer set
 * @sqlfn asText()
 */
Datum
Poseset_as_text(PG_FUNCTION_ARGS)
{
  Set *s = PG_GETARG_SET_P(0);
  int dbl_dig_for_wkt = OUT_DEFAULT_DECIMAL_DIGITS;
  if (PG_NARGS() > 1 && ! PG_ARGISNULL(1))
    dbl_dig_for_wkt = PG_GETARG_INT32(1);
  char *str = poseset_as_text(s, dbl_dig_for_wkt);
  text *result = cstring2text(str);
  pfree(str);
  PG_FREE_IF_COPY(s, 0);
  PG_RETURN_TEXT_P(result);
}

PGDLLEXPORT Datum Poseset_as_ewkt(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Poseset_as_ewkt);
/**
 * @ingroup mobilitydb_setspan_inout
 * @brief Return the Extended Well-Known Text (EWKT) representation of a
 * pose set
 * @sqlfn asEWKT()
 */
Datum
Poseset_as_ewkt(PG_FUNCTION_ARGS)
{
  Set *s = PG_GETARG_SET_P(0);
  int dbl_dig_for_wkt = OUT_DEFAULT_DECIMAL_DIGITS;
  if (PG_NARGS() > 1 && ! PG_ARGISNULL(1))
    dbl_dig_for_wkt = PG_GETARG_INT32(1);
  char *str = poseset_as_ewkt(s, dbl_dig_for_wkt);
  text *result = cstring2text(str);
  pfree(str);
  PG_FREE_IF_COPY(s, 0);
  PG_RETURN_TEXT_P(result);
}

/*****************************************************************************/

/**
 * @brief Return the (Extended) Well-Known Text (WKT or EWKT) representation of
 * a temporal pose
 * @sqlfn asText()
 */
static Datum
Tpose_as_text_ext(FunctionCallInfo fcinfo, bool extended)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  int dbl_dig_for_wkt = OUT_DEFAULT_DECIMAL_DIGITS;
  if (PG_NARGS() > 1 && ! PG_ARGISNULL(1))
    dbl_dig_for_wkt = PG_GETARG_INT32(1);
  char *str = extended ? tpose_as_ewkt(temp, dbl_dig_for_wkt) :
    tpose_as_text(temp, dbl_dig_for_wkt);
  text *result = cstring2text(str);
  pfree(str);
  PG_FREE_IF_COPY(temp, 0);
  PG_RETURN_TEXT_P(result);
}

PGDLLEXPORT Datum Tpose_as_text(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tpose_as_text);
/**
 * @ingroup mobilitydb_temporal_inout
 * @brief Return the Well-Known Text (WKT) representation of a temporal pose
 * @sqlfn asText()
 */
Datum
Tpose_as_text(PG_FUNCTION_ARGS)
{
  return Tpose_as_text_ext(fcinfo, false);
}

PGDLLEXPORT Datum Tpose_as_ewkt(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tpose_as_ewkt);
/**
 * @ingroup mobilitydb_temporal_inout
 * @brief Return the Extended Well-Known Text (EWKT) representation of a
 * temporal pose
 * @note It is the WKT representation prefixed with the SRID
 * @sqlfn asEWKT()
 */
Datum
Tpose_as_ewkt(PG_FUNCTION_ARGS)
{
  return Tpose_as_text_ext(fcinfo, true);
}

/*****************************************************************************/

/**
 * @brief Return the Well-Known Text (WKT) representation of an array of
 * geometry/geography
 */
static Datum
Tposearr_as_text_ext(FunctionCallInfo fcinfo, bool temporal, bool extended)
{
  ArrayType *array = PG_GETARG_ARRAYTYPE_P(0);
  /* Return NULL on empty array */
  int count = ArrayGetNItems(ARR_NDIM(array), ARR_DIMS(array));
  if (count == 0)
  {
    PG_FREE_IF_COPY(array, 0);
    PG_RETURN_NULL();
  }
  int dbl_dig_for_wkt = OUT_DEFAULT_DECIMAL_DIGITS;
  if (PG_NARGS() > 1 && ! PG_ARGISNULL(1))
    dbl_dig_for_wkt = PG_GETARG_INT32(1);

  char **strarr;
  if (temporal)
  {
    Temporal **temparr = temparr_extract(array, &count);
    strarr = tposearr_as_text((const Temporal **) temparr, count,
      dbl_dig_for_wkt, extended);
    /* We cannot use pfree_array */
    pfree(temparr);
  }
  else
  {
    Datum *cbufarr = datumarr_extract(array, &count);
    strarr = posearr_as_text(cbufarr, count, dbl_dig_for_wkt, extended);
    /* We cannot use pfree_array */
    pfree(cbufarr);
  }
  ArrayType *result = strarr_to_textarray(strarr, count);
  PG_FREE_IF_COPY(array, 0);
  PG_RETURN_ARRAYTYPE_P(result);
}

PGDLLEXPORT Datum Posearr_as_text(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Posearr_as_text);
/**
 * @ingroup mobilitydb_temporal_inout
 * @brief Return the Well-Known Text (WKT) representation of an array of
 * geometry/geography
 * @sqlfn asText()
 */
Datum
Posearr_as_text(PG_FUNCTION_ARGS)
{
  return Tposearr_as_text_ext(fcinfo, false, false);
}

PGDLLEXPORT Datum Posearr_as_ewkt(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Posearr_as_ewkt);
/**
 * @ingroup mobilitydb_temporal_inout
 * @brief Return the Extended Well-Known Text (EWKT) representation
 * of a geometry/geography array
 * @note It is the WKT representation prefixed with the SRID
 * @sqlfn asEWKT()
 */
Datum
Posearr_as_ewkt(PG_FUNCTION_ARGS)
{
  return Tposearr_as_text_ext(fcinfo, false, true);
}

PGDLLEXPORT Datum Tposearr_as_text(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tposearr_as_text);
/**
 * @ingroup mobilitydb_temporal_inout
 * @brief Return the Well-Known Text (WKT) representation of a
 * geometry/geography array
 * @sqlfn asText()
 */
Datum
Tposearr_as_text(PG_FUNCTION_ARGS)
{
  return Tposearr_as_text_ext(fcinfo, true, false);
}

PGDLLEXPORT Datum Tposearr_as_ewkt(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Tposearr_as_ewkt);
/**
 * @ingroup mobilitydb_temporal_inout
 * @brief Return the Extended Well-Known Text (EWKT) representation an array of
 * temporal poses
 * @note The output is the WKT representation prefixed with the SRID
 * @sqlfn asEWKT()
 */
Datum
Tposearr_as_ewkt(PG_FUNCTION_ARGS)
{
  return Tposearr_as_text_ext(fcinfo, true, true);
}

/*****************************************************************************/
