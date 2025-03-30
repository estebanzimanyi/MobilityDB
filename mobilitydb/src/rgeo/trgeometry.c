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
 * @brief General functions for temporal rigid geometries
 */


#include "rgeo/trgeometry.h"

/* PostgreSQL */
#include <postgres.h>
#include "utils/array.h"
#include "utils/timestamp.h"
/* MEOS */
#include <meos.h>
#include <meos_rgeo.h>
#include <stdio.h>
#include "general/type_inout.h"
#include "general/type_util.h"
#include "rgeo/trgeometry_parser.h"
#include "rgeo/trgeometry_temporaltypes.h"
#include "geo/tgeo_spatialfuncs.h"
#include "pose/pose.h"
/* MobilityDB */
#include "pg_general/meos_catalog.h"
#include "pg_general/temporal.h"
#include "pg_general/type_util.h"
#include "pg_geo/postgis.h"

/*****************************************************************************
 * Input/output functions
 *****************************************************************************/

PGDLLEXPORT Datum Trgeometry_in(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeometry_in);
/**
 * @brief Generic input function for temporal rigid geometries
 *
 * @note Examples of input for the various temporal types:
 * - Instant
 * @code
 * Polygon((0 0, 1 0, 0 1, 0 0)); Pose(0, 0, 0) @ 2012-01-01 08:00:00
 * @endcode
 * - Instant set
 * @code
 * Polygon((0 0, 1 0, 0 1, 0 0));{ Pose(0, 0, 0) @ 2012-01-01 08:00:00 ,
 * Pose(1, 1, 0) @ 2012-01-01 08:10:00 }
 * @endcode
 * - Sequence
 * @code
 * Polygon((0 0, 1 0, 0 1, 0 0));[ Pose(0, 0, 0) @ 2012-01-01 08:00:00 ,
 * Pose(1, 1, 0) @ 2012-01-01 08:10:00 )
 * @endcode
 * - Sequence set
 * @code
 * Polygon((0 0, 1 0, 0 1, 0 0));{ [ Pose(0, 0, 0) @ 2012-01-01 08:00:00 ,
 * Pose(1, 1, 0) @ 2012-01-01 08:10:00 ) , [ Pose(1, 1, 0) @ 2012-01-01 08:20:00 ,
 * Pose(0, 0, 0) @ 2012-01-01 08:30:00 ] }
 * @endcode
 */
Datum
Trgeometry_in(PG_FUNCTION_ARGS)
{
  const char *input = PG_GETARG_CSTRING(0);
  Oid temptypid = PG_GETARG_OID(1);
  Temporal *result = trgeo_parse(&input, oid_type(temptypid));
  PG_RETURN_POINTER(result);
}

PGDLLEXPORT Datum Trgeometry_out(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeometry_out);
/**
 * @brief Generic output function for temporal rigid geometries
 */
Datum
Trgeometry_out(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  char *result = tspatial_as_text(temp, OUT_DEFAULT_DECIMAL_DIGITS);
  PG_FREE_IF_COPY(temp, 0);
  PG_RETURN_CSTRING(result);
}

PGDLLEXPORT Datum Trgeometry_recv(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeometry_recv);
/**
 * @ingroup mobilitydb_pose_base_inout
 * @brief Return a pose from its Well-Known Binary (WKB) representation
 * @sqlfn pose_recv()
 */
Datum
Trgeometry_recv(PG_FUNCTION_ARGS)
{
  StringInfo buf = (StringInfo) PG_GETARG_POINTER(0);
  Pose *result = pose_from_wkb((uint8_t *) buf->data, buf->len);
  /* Set cursor to the end of buffer (so the backend is happy) */
  buf->cursor = buf->len;
  PG_RETURN_POSE_P(result);
}

PGDLLEXPORT Datum Trgeometry_send(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeometry_send);
/**
 * @ingroup mobilitydb_pose_base_inout
 * @brief Return the Well-Known Binary (WKB) representation of a pose
 * @sqlfn pose_send()
 */
Datum
Trgeometry_send(PG_FUNCTION_ARGS)
{
  Pose *pose = PG_GETARG_POSE_P(0);
  size_t wkb_size = VARSIZE_ANY_EXHDR(pose);
  /* A pose always outputs the SRID */
  uint8_t *wkb = pose_as_wkb(pose, WKB_EXTENDED, &wkb_size);
  bytea *result = bstring2bytea(wkb, wkb_size);
  pfree(wkb);
  PG_RETURN_BYTEA_P(result);
}

/*****************************************************************************
 * Constructor functions
 *****************************************************************************/

PGDLLEXPORT Datum Trgeometry_inst_constructor(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeometry_inst_constructor);
/**
 * @brief Construct a temporal instant value from the arguments
 */
Datum
Trgeometry_inst_constructor(PG_FUNCTION_ARGS)
{
  GSERIALIZED *gs = PG_GETARG_GSERIALIZED_P(0);
  Pose *pose = PG_GETARG_POSE_P(1);
  ensure_not_empty(gs);
  ensure_has_not_M_geo(gs);
  TimestampTz t = PG_GETARG_TIMESTAMPTZ(2);
  meosType temptype = oid_type(get_fn_expr_rettype(fcinfo->flinfo));
  Temporal *result = (Temporal *) trgeoinst_make(PointerGetDatum(gs),
    PointerGetDatum(pose), temptype, t);
  PG_FREE_IF_COPY(gs, 0);
  PG_RETURN_POINTER(result);
}

PGDLLEXPORT Datum Trgeometry_seq_constructor(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeometry_seq_constructor);
/**
 * @ingroup mobilitydb_rgeo_constructor
 * @brief Construct a temporal sequence from an array of temporal instants
 * @sqlfn tbool_seq(), tint_seq(), tfloat_seq(), ttext_seq(), ...
 */
Datum
Trgeometry_seq_constructor(PG_FUNCTION_ARGS)
{
  ArrayType *array = PG_GETARG_ARRAYTYPE_P(0);
  meosType temptype = oid_type(get_fn_expr_rettype(fcinfo->flinfo));
  interpType interp = temptype_continuous(temptype) ? LINEAR : STEP;
  if (PG_NARGS() > 1 && ! PG_ARGISNULL(1))
  {
    text *interp_txt = PG_GETARG_TEXT_P(1);
    char *interp_str = text2cstring(interp_txt);
    interp = interptype_from_string(interp_str);
    pfree(interp_str);
  }
  bool lower_inc = true, upper_inc = true;
  if (PG_NARGS() > 2 && ! PG_ARGISNULL(2))
    lower_inc = PG_GETARG_BOOL(2);
  if (PG_NARGS() > 3 && ! PG_ARGISNULL(3))
    upper_inc = PG_GETARG_BOOL(3);
  ensure_not_empty_array(array);
  int count;
  TInstant **instants = (TInstant **) temparr_extract(array, &count);
  Temporal *result = (Temporal *) trgeoseq_make(trgeoinst_geom(instants[0]),
    (const TInstant **) instants, count, lower_inc, upper_inc, interp,
    NORMALIZE);
  pfree(instants);
  PG_FREE_IF_COPY(array, 0);
  PG_RETURN_POINTER(result);
}

PGDLLEXPORT Datum Trgeometry_seqset_constructor(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeometry_seqset_constructor);
/**
 * @brief Construct a temporal sequence set value from the array of temporal
 * sequence values
 */
Datum
Trgeometry_seqset_constructor(PG_FUNCTION_ARGS)
{
  ArrayType *array = PG_GETARG_ARRAYTYPE_P(0);
  ensure_not_empty_array(array);
  int count;
  TSequence **sequences = (TSequence **) temparr_extract(array, &count);
  Temporal *result = (Temporal *) trgeoseqset_make(
    trgeoseq_geom(sequences[0]), (const TSequence **) sequences, count,
    NORMALIZE);
  pfree(sequences);
  PG_FREE_IF_COPY(array, 0);
  PG_RETURN_POINTER(result);
}

PGDLLEXPORT Datum Trgeometry_seqset_constructor_gaps(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeometry_seqset_constructor_gaps);
/**
 * @ingroup mobilitydb_rgeo_constructor
 * @brief Construct a temporal sequence set from an array of temporal instants
 * accounting for potential gaps
 * @note The SQL function is not strict
 * @sqlfn trgeo_seqset_gaps()
 */
Datum
Trgeometry_seqset_constructor_gaps(PG_FUNCTION_ARGS)
{
  if (PG_ARGISNULL(0))
    PG_RETURN_NULL();

  ArrayType *array = PG_GETARG_ARRAYTYPE_P(0);
  ensure_not_empty_array(array);
  double maxdist = -1.0;
  Interval *maxt = NULL;
  meosType temptype = oid_type(get_fn_expr_rettype(fcinfo->flinfo));
  interpType interp = temptype_continuous(temptype) ? LINEAR : STEP;
  if (PG_NARGS() > 1 && ! PG_ARGISNULL(1))
    maxt = PG_GETARG_INTERVAL_P(1);
  if (PG_NARGS() > 2 && ! PG_ARGISNULL(2))
    maxdist = PG_GETARG_FLOAT8(2);
  if (PG_NARGS() > 3 && ! PG_ARGISNULL(3))
  {
    text *interp_txt = PG_GETARG_TEXT_P(3);
    char *interp_str = text2cstring(interp_txt);
    interp = interptype_from_string(interp_str);
    pfree(interp_str);
  }
  /* Store fcinfo into a global variable */
  /* Needed for the distance function for temporal geographic points */
  store_fcinfo(fcinfo);
  /* Extract the array of instants */
  int count;
  TInstant **instants = (TInstant **) temparr_extract(array, &count);
  TSequenceSet *result = trgeoseqset_make_gaps(trgeoinst_geom(instants[0]),
    (const TInstant **) instants, count, interp, maxt, maxdist);
  pfree(instants);
  PG_FREE_IF_COPY(array, 0);
  PG_RETURN_POINTER(result);
}

/*****************************************************************************/

PGDLLEXPORT Datum Trgeometry_constructor(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeometry_constructor);
/**
 * @brief Construct a temporal instant value from the arguments
 */
Datum
Trgeometry_constructor(PG_FUNCTION_ARGS)
{
  GSERIALIZED *gs = PG_GETARG_GSERIALIZED_P(0);
  Temporal *tpose = PG_GETARG_TEMPORAL_P(1);
  Temporal *result = (Temporal *) geo_tpose_to_trgeo(gs, tpose);
  PG_FREE_IF_COPY(gs, 0);
  if (! result)
    PG_RETURN_NULL();
  PG_RETURN_POINTER(result);
}

/*****************************************************************************
 * Conversion functions
 *****************************************************************************/

PGDLLEXPORT Datum Trgeometry_to_tpose(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeometry_to_tpose);
/**
 * @ingroup mobilitydb_rgeo_accessor
 * @brief Return the end instant of a temporal value
 * @sqlfn endInstant()
 */
Datum
Trgeometry_to_tpose(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  Temporal *result = trgeo_tpose(temp);
  PG_FREE_IF_COPY(temp, 0);
  PG_RETURN_POINTER(result);
}

PGDLLEXPORT Datum Trgeometry_to_tpoint(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeometry_to_tpoint);
/**
 * @ingroup mobilitydb_rgeo_accessor
 * @brief Return the end instant of a temporal value
 * @sqlfn endInstant()
 */
Datum
Trgeometry_to_tpoint(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  Temporal *result = trgeo_tpoint(temp);
  PG_FREE_IF_COPY(temp, 0);
  PG_RETURN_POINTER(result);
}

/*****************************************************************************
 * Accessor functions
 *****************************************************************************/


/*****************************************************************************
 * Restriction Functions
 *****************************************************************************/

PGDLLEXPORT Datum Trgeometry_value_at_timestamp(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeometry_value_at_timestamp);
/**
 * @ingroup mobilitydb_rgeo_accessor
 * @brief Return the base value of a temporal value at the timestamp
 * @sqlfn valueAtTimestamp()
 */
Datum
Trgeometry_value_at_timestamp(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  TimestampTz t = PG_GETARG_TIMESTAMPTZ(1);
  Datum result;
  bool found = trgeo_value_at_timestamptz(temp, t, true, &result);
  PG_FREE_IF_COPY(temp, 0);
  if (! found)
    PG_RETURN_NULL();
  PG_RETURN_DATUM(result);
}

/*****************************************************************************/
