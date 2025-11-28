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


#include "rgeo/trgeo.h"

/* C */
#include <stdio.h>
/* PostgreSQL */
#include <postgres.h>
#include "utils/array.h"
#include "utils/timestamp.h"
/* MEOS */
#include <meos.h>
#include <meos_rgeo.h>
#include "temporal/set.h"
#include "temporal/span.h"
#include "temporal/spanset.h"
#include "temporal/type_inout.h"
#include "temporal/type_util.h"
#include "geo/tgeo_spatialfuncs.h"
#include "pose/pose.h"
#include "rgeo/trgeo.h"
#include "rgeo/trgeo_parser.h"
#include "rgeo/trgeo_all.h"
/* MobilityDB */
#include "pg_temporal/meos_catalog.h"
#include "pg_temporal/temporal.h"
#include "pg_temporal/type_util.h"
#include "pg_geo/postgis.h"
#include "pg_geo/tspatial.h"

/*****************************************************************************
 * Input/output functions
 *****************************************************************************/

PGDLLEXPORT Datum Trgeo_in(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_in);
/**
 * @ingroup mobilitydb_rgeo_inout
 * @brief Generic input function for temporal rigid geometries
 * @details Examples of input for the various temporal types:
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
Trgeo_in(PG_FUNCTION_ARGS)
{
  const char *input = PG_GETARG_CSTRING(0);
  Oid temptypid = PG_GETARG_OID(1);
  Temporal *result = trgeo_parse(&input, oid_type(temptypid));
  PG_RETURN_POINTER(result);
}

PGDLLEXPORT Datum Trgeo_out(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_out);
/**
 * @ingroup mobilitydb_rgeo_inout
 * @brief Generic output function for temporal rigid geometries
 */
Datum
Trgeo_out(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  char *result = trgeo_out(temp);
  PG_FREE_IF_COPY(temp, 0);
  PG_RETURN_CSTRING(result);
}

PGDLLEXPORT Datum Trgeo_recv(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_recv);
/**
 * @ingroup mobilitydb_rgeo_inout
 * @brief Return a temporal rigid geometry from its Well-Known Binary (WKB)
 * representation
 * @sqlfn trgeometry_recv()
 */
Datum
Trgeo_recv(PG_FUNCTION_ARGS)
{
  StringInfo buf = (StringInfo) PG_GETARG_POINTER(0);
  Temporal *result = temporal_from_wkb((uint8_t *) buf->data, buf->len);
  /* Set cursor to the end of buffer (so the backend is happy) */
  buf->cursor = buf->len;
  PG_RETURN_TEMPORAL_P(result);
}

PGDLLEXPORT Datum Trgeo_send(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_send);
/**
 * @ingroup mobilitydb_rgeo_inout
 * @brief Return the Well-Known Binary (WKB) representation of a temporal rigid geometry
 * @sqlfn trgeometry_send()
 */
Datum
Trgeo_send(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  size_t wkb_size = VARSIZE_ANY_EXHDR(temp);
  /* A temporal geometry always outputs the SRID */
  uint8_t *wkb = temporal_as_wkb(temp, WKB_EXTENDED, &wkb_size);
  bytea *result = bstring2bytea(wkb, wkb_size);
  pfree(wkb);
  PG_RETURN_BYTEA_P(result);
}

PGDLLEXPORT Datum Trgeo_typmod_in(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_typmod_in);
/**
 * @brief Input typmod information for temporal rigid geometries
 */
Datum
Trgeo_typmod_in(PG_FUNCTION_ARGS)
{
  ArrayType *array = (ArrayType *) DatumGetPointer(PG_GETARG_DATUM(0));
  uint32 typmod = tspatial_typmod_in(array, true, false);
  PG_RETURN_INT32(typmod);
}

/*****************************************************************************
 * Input in EWKT representation
 *****************************************************************************/

PGDLLEXPORT Datum Trgeo_from_ewkt(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_from_ewkt);
/**
 * @ingroup mobilitydb_rgeo_inout
 * @brief Input a temporal rigid geometry from its Extended Well-Known Text
 * (EWKT) representation
 * @note This just does the same thing as the _in function, except it has to handle
 * a 'text' input. First, unwrap the text into a cstring, then do as tgeometry_in
 * @sqlfn trgeometryFromEWKT()
 */
PGDLLEXPORT Datum
Trgeo_from_ewkt(PG_FUNCTION_ARGS)
{
  text *wkt_text = PG_GETARG_TEXT_P(0);
  Oid temptypid = get_fn_expr_rettype(fcinfo->flinfo);
  char *wkt = text2cstring(wkt_text);
  /* Copy the pointer since it will be advanced during parsing */
  const char *wkt_ptr = wkt;
  Temporal *result = trgeo_parse(&wkt_ptr, oid_type(temptypid));
  pfree(wkt);
  PG_FREE_IF_COPY(wkt_text, 0);
  PG_RETURN_POINTER(result);
}

/*****************************************************************************
 * Output in (E)WKT representation
 *****************************************************************************/

/**
 * @brief Return the (Extended) Well-Known Text (WKT or EWKT) representation of
 * a temporal rigid geometry
 */
static Datum
Trgeo_as_text_common(FunctionCallInfo fcinfo, bool extended)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  int dbl_dig_for_wkt = OUT_DEFAULT_DECIMAL_DIGITS;
  if (PG_NARGS() > 1 && ! PG_ARGISNULL(1))
    dbl_dig_for_wkt = PG_GETARG_INT32(1);
  char *str = trgeo_wkt_out(temp, dbl_dig_for_wkt, extended);
  text *result = cstring2text(str);
  pfree(str);
  PG_FREE_IF_COPY(temp, 0);
  PG_RETURN_TEXT_P(result);
}

PGDLLEXPORT Datum Trgeo_as_text(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_as_text);
/**
 * @ingroup mobilitydb_rgeo_inout
 * @brief Return the Well-Known Text (WKT) representation of a temporal rigid
 * geometry
 * @sqlfn asText()
 */
Datum
Trgeo_as_text(PG_FUNCTION_ARGS)
{
  return Trgeo_as_text_common(fcinfo, false);
}

PGDLLEXPORT Datum Trgeo_as_ewkt(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_as_ewkt);
/**
 * @ingroup mobilitydb_rgeo_inout
 * @brief Return the Extended Well-Known Text (EWKT) representation of a
 * temporal rigid geometry
 * @note It is the WKT representation prefixed with the SRID
 * @sqlfn asEWKT()
 */
Datum
Trgeo_as_ewkt(PG_FUNCTION_ARGS)
{
  return Trgeo_as_text_common(fcinfo, true);
}

/*****************************************************************************
 * Constructor functions
 *****************************************************************************/

PGDLLEXPORT Datum Trgeoinst_constructor(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeoinst_constructor);
/**
 * @ingroup mobilitydb_rgeo_constructor
 * @brief Construct a temporal rigid geometry instant from a geometry, a pose,
 * and a timestamptz
 * @sqlfn trgeometryInst()
 */
Datum
Trgeoinst_constructor(PG_FUNCTION_ARGS)
{
  GSERIALIZED *gs = PG_GETARG_GSERIALIZED_P(0);
  Pose *pose = PG_GETARG_POSE_P(1);
  ensure_not_empty(gs);
  ensure_has_not_M_geo(gs);
  TimestampTz t = PG_GETARG_TIMESTAMPTZ(2);
  TInstant *result = trgeoinst_make(gs, pose, t);
  PG_FREE_IF_COPY(gs, 0);
  PG_RETURN_POINTER(result);
}

PGDLLEXPORT Datum Trgeoseq_constructor(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeoseq_constructor);
/**
 * @ingroup mobilitydb_rgeo_constructor
 * @brief Construct a temporal rigid geometry sequence from an array of
 * temporal instants
 * @sqlfn trgeometrySeq()
 */
Datum
Trgeoseq_constructor(PG_FUNCTION_ARGS)
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
  Temporal *result = (Temporal *) trgeoseq_make(trgeoinst_geom_p(instants[0]),
    instants, count, lower_inc, upper_inc, interp, NORMALIZE);
  pfree(instants);
  PG_FREE_IF_COPY(array, 0);
  PG_RETURN_POINTER(result);
}

PGDLLEXPORT Datum Trgeoseqset_constructor(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeoseqset_constructor);
/**
 * @ingroup mobilitydb_rgeo_constructor
 * @brief Construct a temporal rigid geometry sequence set from an array of
 * temporal sequences
 * @sqlfn trgeometrySeqSet()
 */
Datum
Trgeoseqset_constructor(PG_FUNCTION_ARGS)
{
  ArrayType *array = PG_GETARG_ARRAYTYPE_P(0);
  ensure_not_empty_array(array);
  int count;
  TSequence **sequences = (TSequence **) temparr_extract(array, &count);
  Temporal *result = (Temporal *) trgeoseqset_make(
    trgeoseq_geom_p(sequences[0]), sequences, count, NORMALIZE);
  pfree(sequences);
  PG_FREE_IF_COPY(array, 0);
  PG_RETURN_POINTER(result);
}

PGDLLEXPORT Datum Trgeoseqset_constructor_gaps(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeoseqset_constructor_gaps);
/**
 * @ingroup mobilitydb_rgeo_constructor
 * @brief Construct a temporal rigid geometry sequence set from an array of
 * temporal instants accounting for potential gaps
 * @note The SQL function is not strict
 * @sqlfn trgeoSeqsetGaps()
 */
Datum
Trgeoseqset_constructor_gaps(PG_FUNCTION_ARGS)
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
  TSequenceSet *result = trgeoseqset_make_gaps(trgeoinst_geom_p(instants[0]),
    instants, count, interp, maxt, maxdist);
  pfree(instants);
  PG_FREE_IF_COPY(array, 0);
  PG_RETURN_POINTER(result);
}

/*****************************************************************************/

PGDLLEXPORT Datum Trgeo_constructor(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_constructor);
/**
 * @ingroup mobilitydb_rgeo_constructor
 * @brief Construct a temporal rigid geometry from a reference geometry and a
 * temporal pose
 * @sqlfn trgeometry()
 */
Datum
Trgeo_constructor(PG_FUNCTION_ARGS)
{
  GSERIALIZED *gs = PG_GETARG_GSERIALIZED_P(0);
  Temporal *tpose = PG_GETARG_TEMPORAL_P(1);
  Temporal *result = geo_tpose_to_trgeo(gs, tpose);
  PG_FREE_IF_COPY(gs, 0);
  if (! result)
    PG_RETURN_NULL();
  PG_RETURN_POINTER(result);
}

/*****************************************************************************/

PGDLLEXPORT Datum Trgeoseq_from_base_tstzset(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeoseq_from_base_tstzset);
/**
 * @ingroup mobilitydb_rgeo_constructor
 * @brief Return a temporal rigid geometry discrete sequence from a geometry,
 * a pose, and a timestamptz set
 * @sqlfn trgeometry()
 */
Datum
Trgeoseq_from_base_tstzset(PG_FUNCTION_ARGS)
{
  GSERIALIZED *gs = PG_GETARG_GSERIALIZED_P(0);
  Datum pose = PG_GETARG_ANYDATUM(1);
  Set *s = PG_GETARG_SET_P(2);
  Temporal *tpose = (Temporal *) tsequence_from_base_tstzset(pose, T_TPOSE, s);
  TSequence *result = (TSequence *) geo_tpose_to_trgeo(gs, tpose);
  pfree(tpose);
  PG_FREE_IF_COPY(gs, 0); PG_FREE_IF_COPY(DatumGetPointer(pose), 0);
  PG_FREE_IF_COPY(s, 2);
  PG_RETURN_TSEQUENCE_P(result);
}

PGDLLEXPORT Datum Trgeoseq_from_base_tstzspan(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeoseq_from_base_tstzspan);
/**
 * @ingroup mobilitydb_rgeo_constructor
 * @brief Return a temporal rigid geometry continuous sequence from a geometry,
 * a pose, and a timestamptz set
 * @sqlfn trgeometry()
 */
Datum
Trgeoseq_from_base_tstzspan(PG_FUNCTION_ARGS)
{
  GSERIALIZED *gs = PG_GETARG_GSERIALIZED_P(0);
  Datum pose = PG_GETARG_ANYDATUM(1);
  Span *s = PG_GETARG_SPAN_P(2);
  interpType interp = LINEAR;
  if (PG_NARGS() > 3 && ! PG_ARGISNULL(3))
    interp = input_interp_string(fcinfo, 3);
  Temporal *tpose = (Temporal *) tsequence_from_base_tstzspan(pose, T_TPOSE,
    s, interp);
  TSequence *result = (TSequence *) geo_tpose_to_trgeo(gs, tpose);
  pfree(tpose);
  PG_FREE_IF_COPY(gs, 0); PG_FREE_IF_COPY(DatumGetPointer(pose), 0);
  PG_RETURN_TSEQUENCE_P(result);
}

PGDLLEXPORT Datum Trgeoseqset_from_base_tstzspanset(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeoseqset_from_base_tstzspanset);
/**
 * @ingroup mobilitydb_rgeo_constructor
 * @brief Return a temporal rigid geometry sequence set from a geometry,
 * a pose, and a timestamptz set
 * @sqlfn trgeometry()
 */
Datum
Trgeoseqset_from_base_tstzspanset(PG_FUNCTION_ARGS)
{
  GSERIALIZED *gs = PG_GETARG_GSERIALIZED_P(0);
  Datum pose = PG_GETARG_ANYDATUM(1);
  SpanSet *ss = PG_GETARG_SPANSET_P(2);
  interpType interp = LINEAR;
  if (PG_NARGS() > 3 && ! PG_ARGISNULL(3))
    interp = input_interp_string(fcinfo, 3);
  Temporal *tpose = (Temporal *) tsequenceset_from_base_tstzspanset(pose,
    T_TPOSE, ss, interp);
  TSequence *result = (TSequence *) geo_tpose_to_trgeo(gs, tpose);
  pfree(tpose);
  PG_FREE_IF_COPY(gs, 0); PG_FREE_IF_COPY(DatumGetPointer(pose), 0);
  PG_FREE_IF_COPY(ss, 2);
  PG_RETURN_TSEQUENCE_P(result);
}

/*****************************************************************************
 * Conversion functions
 *****************************************************************************/

PGDLLEXPORT Datum Trgeo_to_tpose(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_to_tpose);
/**
 * @ingroup mobilitydb_rgeo_conversion
 * @brief Convert a temporal rigid geometry into a temporal pose
 * @sqlfn tpose()
 */
Datum
Trgeo_to_tpose(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  Temporal *result = trgeo_to_tpose(temp);
  PG_FREE_IF_COPY(temp, 0);
  PG_RETURN_POINTER(result);
}

PGDLLEXPORT Datum Trgeo_to_tpoint(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_to_tpoint);
/**
 * @ingroup mobilitydb_rgeo_conversion
 * @brief Convert a temporal rigid geometry into a temporal point 
 * @sqlfn tgeompoint()
 */
Datum
Trgeo_to_tpoint(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  Temporal *result = trgeo_to_tpoint(temp);
  PG_FREE_IF_COPY(temp, 0);
  PG_RETURN_POINTER(result);
}

PGDLLEXPORT Datum Trgeo_to_geom(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_to_geom);
/**
 * @ingroup mobilitydb_rgeo_conversion
 * @brief Return the reference geometry of a temporal rigid geometry
 * @sqlfn geometry()
 */
Datum
Trgeo_to_geom(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  GSERIALIZED *result = trgeo_to_geom(temp);
  PG_FREE_IF_COPY(temp, 0);
  PG_RETURN_POINTER(result);
}

/*****************************************************************************
 * Accessor functions
 *****************************************************************************/

PGDLLEXPORT Datum Trgeo_start_geom(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_start_geom);
/**
 * @ingroup mobilitydb_rgeo_accessor
 * @brief Return the start value of a temporal rigid geometry
 * @sqlfn startValue()
 */
Datum
Trgeo_start_geom(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  GSERIALIZED *result = trgeo_start_geom(temp);
  PG_FREE_IF_COPY(temp, 0);
  PG_RETURN_POINTER(result);
}

PGDLLEXPORT Datum Trgeo_end_geom(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_end_geom);
/**
 * @ingroup mobilitydb_rgeo_accessor
 * @brief Return the end value of a temporal rigid geometry
 * @sqlfn endValue()
 */
Datum
Trgeo_end_geom(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  GSERIALIZED *result = trgeo_end_geom(temp);
  PG_FREE_IF_COPY(temp, 0);
  PG_RETURN_POINTER(result);
}

PGDLLEXPORT Datum Trgeo_geom_n(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_geom_n);
/**
 * @ingroup mobilitydb_rgeo_accessor
 * @brief Return the n-th value of a temporal rigid geometry
 * @sqlfn valueN()
 */
Datum
Trgeo_geom_n(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  int n = PG_GETARG_INT32(1); /* Assume 1-based */
  GSERIALIZED *result;
  bool found = trgeo_geom_n(temp, n, &result);
  PG_FREE_IF_COPY(temp, 0);
  if (! found)
    PG_RETURN_NULL();
  PG_RETURN_POINTER(result);
}

PGDLLEXPORT Datum Trgeo_geoms(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_geoms);
/**
 * @ingroup mobilitydb_rgeo_accessor
 * @brief Return the geometries of a temporal rigid geometry as a set
 * @sqlfn geometries()
 */
Datum
Trgeo_geoms(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  int count;
  GSERIALIZED **geoms = trgeo_geoms(temp, &count);
  Datum *values = palloc(sizeof(Datum) * count);
  for (int i = 0; i < count; i++)
    values[i] = PointerGetDatum(geoms[i]);
  Set *result = set_make_free(values, count, T_GEOMETRY, ORDER_NO);
  for (int i = 0; i < count; i++)
    pfree(geoms[i]);
  PG_FREE_IF_COPY(temp, 0);
  PG_RETURN_SET_P(result);
}

PGDLLEXPORT Datum Trgeo_start_instant(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_start_instant);
/**
 * @ingroup mobilitydb_rgeo_accessor
 * @brief Return the start instant of a temporal rigid geometry
 * @sqlfn startInstant()
 */
Datum
Trgeo_start_instant(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  TInstant *result = trgeo_start_instant(temp);
  PG_FREE_IF_COPY(temp, 0);
  PG_RETURN_TINSTANT_P(result);
}

PGDLLEXPORT Datum Trgeo_end_instant(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_end_instant);
/**
 * @ingroup mobilitydb_rgeo_accessor
 * @brief Return the end instant of a temporal rigid geometry
 * @sqlfn endInstant()
 */
Datum
Trgeo_end_instant(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  TInstant *result = trgeo_end_instant(temp);
  PG_FREE_IF_COPY(temp, 0);
  PG_RETURN_TINSTANT_P(result);
}

PGDLLEXPORT Datum Trgeo_instant_n(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_instant_n);
/**
 * @ingroup mobilitydb_rgeo_accessor
 * @brief Return the n-th instant of a temporal rigid geometry
 * @sqlfn instantN()
 */
Datum
Trgeo_instant_n(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  int n = PG_GETARG_INT32(1); /* Assume 1-based */
  TInstant *result = trgeo_instant_n(temp, n);
  PG_FREE_IF_COPY(temp, 0);
  if (! result)
    PG_RETURN_NULL();
  PG_RETURN_TINSTANT_P(result);
}

PGDLLEXPORT Datum Trgeo_instants(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_instants);
/**
 * @ingroup mobilitydb_rgeo_accessor
 * @brief Return the array of distinct instants of a temporal rigid geometry
 * @sqlfn instants()
 */
Datum
Trgeo_instants(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  int count;
  TInstant **instants =  trgeo_instants(temp, &count);
  ArrayType *result = temparr_to_array((Temporal **) instants, count, FREE);
  PG_FREE_IF_COPY(temp, 0);
  PG_RETURN_ARRAYTYPE_P(result);
}

PGDLLEXPORT Datum Trgeo_start_sequence(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_start_sequence);
/**
 * @ingroup mobilitydb_rgeo_accessor
 * @brief Return the start sequence of a temporal sequence (set)
 * @sqlfn startSequence()
 */
Datum
Trgeo_start_sequence(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  TSequence *result = trgeo_start_sequence(temp);
  PG_FREE_IF_COPY(temp, 0);
  PG_RETURN_TSEQUENCE_P(result);
}

PGDLLEXPORT Datum Trgeo_end_sequence(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_end_sequence);
/**
 * @ingroup mobilitydb_rgeo_accessor
 * @brief Return the end sequence of a temporal sequence (set)
 * @sqlfn endSequence()
 */
Datum
Trgeo_end_sequence(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  TSequence *result = trgeo_end_sequence(temp);
  PG_FREE_IF_COPY(temp, 0);
  PG_RETURN_TSEQUENCE_P(result);
}

PGDLLEXPORT Datum Trgeo_sequence_n(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_sequence_n);
/**
 * @ingroup mobilitydb_rgeo_accessor
 * @brief Return the n-th sequence of a temporal sequence (set)
 * @sqlfn sequenceN()
 */
Datum
Trgeo_sequence_n(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  int n = PG_GETARG_INT32(1); /* Assume 1-based */
  TSequence *result = trgeo_sequence_n(temp, n);
  PG_FREE_IF_COPY(temp, 0);
  if (! result)
    PG_RETURN_NULL();
  PG_RETURN_TSEQUENCE_P(result);
}

PGDLLEXPORT Datum Trgeo_sequences(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_sequences);
/**
 * @ingroup mobilitydb_rgeo_accessor
 * @brief Return the array of sequences of a temporal sequence (set)
 * @sqlfn sequences()
 */
Datum
Trgeo_sequences(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  int count;
  TSequence **sequences = trgeo_sequences(temp, &count);
  ArrayType *result = temparr_to_array((Temporal **) sequences, count, FREE);
  PG_FREE_IF_COPY(temp, 0);
  PG_RETURN_ARRAYTYPE_P(result);
}

PGDLLEXPORT Datum Trgeo_segments(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_segments);
/**
 * @ingroup mobilitydb_rgeo_accessor
 * @brief Return the array of sequences of a temporal sequence (set)
 * @sqlfn sequences()
 */
Datum
Trgeo_segments(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  int count;
  TSequence **segments = trgeo_segments(temp, &count);
  ArrayType *result = temparr_to_array((Temporal **) segments, count, FREE);
  PG_FREE_IF_COPY(temp, 0);
  PG_RETURN_ARRAYTYPE_P(result);
}

/*****************************************************************************/

PGDLLEXPORT Datum Trgeo_geom_at_timestamptz(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_geom_at_timestamptz);
/**
 * @ingroup mobilitydb_rgeo_accessor
 * @brief Return the value of a temporal rigid geometry at a timestamptz
 * @sqlfn valueAtTimestamp()
 */
Datum
Trgeo_geom_at_timestamptz(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  TimestampTz t = PG_GETARG_TIMESTAMPTZ(1);
  GSERIALIZED *result;
  bool found = trgeo_geom_at_timestamptz(temp, t, true, &result);
  PG_FREE_IF_COPY(temp, 0);
  if (! found)
    PG_RETURN_NULL();
  PG_RETURN_POINTER(result);
}

/*****************************************************************************
 * Transformation Functions
 *****************************************************************************/

PGDLLEXPORT Datum Trgeo_round(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_round);
/**
 * @ingroup mobilitydb_rgeo_transf
 * @brief Return a temporal rigid geometry with the component values set to a
 * number of decimal places
 * @sqlfn round()
 */
Datum
Trgeo_round(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  int size = PG_GETARG_INT32(1);
  Temporal *result = trgeo_round(temp, size);
  PG_FREE_IF_COPY(temp, 0);
  PG_RETURN_TEMPORAL_P(result);
}

PGDLLEXPORT Datum Trgeoarr_round(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeoarr_round);
/**
 * @ingroup mobilitydb_rgeo_transf
 * @brief Return an array of temporal rigid geometries with the precision of
 * the values set to a number of decimal places
 * @sqlfn round()
 */
Datum
Trgeoarr_round(PG_FUNCTION_ARGS)
{
  ArrayType *array = PG_GETARG_ARRAYTYPE_P(0);
  /* Return NULL on empty array */
  int count = ArrayGetNItems(ARR_NDIM(array), ARR_DIMS(array));
  if (count == 0)
  {
    PG_FREE_IF_COPY(array, 0);
    PG_RETURN_NULL();
  }
  int maxdd = PG_GETARG_INT32(1);

  Temporal **temparr = temparr_extract(array, &count);
  Temporal **resarr = trgeoarr_round(temparr, count, maxdd);
  ArrayType *result = temparr_to_array(resarr, count, FREE_ALL);
  pfree(temparr);
  PG_FREE_IF_COPY(array, 0);
  PG_RETURN_ARRAYTYPE_P(result);
}

/**
 * @ingroup meos_temporal_transf
 * @brief Return an array of temporal rigid geometries with the precision of
 * the coordinates set to a number of decimal places
 * @param[in] temparr Array of temporal rigid geometries
 * @param[in] count Number of values in the input array
 * @param[in] maxdd Maximum number of decimal digits
 */
Temporal **
trgeoarr_round(Temporal **temparr, int count, int maxdd)
{
  /* Ensure the validity of the arguments */
  VALIDATE_NOT_NULL(temparr, NULL);
  if (! ensure_positive(count) || ! ensure_not_negative(maxdd))
    return NULL;

  Temporal **result = palloc(sizeof(Temporal *) * count);
  for (int i = 0; i < count; i++)
    result[i] = trgeo_round(temparr[i], maxdd);
  return result;
}

PGDLLEXPORT Datum Trgeo_set_interp(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_set_interp);
/**
 * @ingroup mobilitydb_rgeo_transf
 * @brief Return a temporal rigid geometry transformed to an interpolation
 * @sqlfn setInterp()
 */
Datum
Trgeo_set_interp(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  interpType interp = input_interp_string(fcinfo, 1);
  Temporal *result = trgeo_set_interp(temp, interp);
  PG_FREE_IF_COPY(temp, 0);
  PG_RETURN_TEMPORAL_P(result);
}

PGDLLEXPORT Datum Trgeo_to_tinstant(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_to_tinstant);
/**
 * @ingroup mobilitydb_rgeo_transf
 * @brief Return a temporal rigid geometry transformed into a temporal instant
 * @sqlfn trgeometryInst()
 */
Datum
Trgeo_to_tinstant(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  TInstant *result = trgeo_to_tinstant(temp);
  PG_FREE_IF_COPY(temp, 0);
  PG_RETURN_TINSTANT_P(result);
}

PGDLLEXPORT Datum Trgeo_to_tsequence(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_to_tsequence);
/**
 * @ingroup mobilitydb_rgeo_transf
 * @brief Return a temporal rigid geometry transformed into a temporal sequence
 * @note The SQL function is not strict
 * @sqlfn trgeometrySeq()
 */
Datum
Trgeo_to_tsequence(PG_FUNCTION_ARGS)
{
  if (PG_ARGISNULL(0))
    PG_RETURN_NULL();

  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  char *interp_str = NULL;
  if (PG_NARGS() > 1 && ! PG_ARGISNULL(1))
  {
    text *interp_txt = PG_GETARG_TEXT_P(1);
    interp_str = text2cstring(interp_txt);
  }
  TSequence *result = trgeo_to_tsequence(temp, interp_str);
  PG_FREE_IF_COPY(temp, 0);
  PG_RETURN_TSEQUENCE_P(result);
}

PGDLLEXPORT Datum Trgeo_to_tsequenceset(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_to_tsequenceset);
/**
 * @ingroup mobilitydb_rgeo_transf
 * @brief Return a temporal rigid geometry transformed into a temporal sequence
 * set
 * @note The SQL function is not strict
 * @sqlfn trgeoSeqSet()
 */
Datum
Trgeo_to_tsequenceset(PG_FUNCTION_ARGS)
{
  if (PG_ARGISNULL(0))
    PG_RETURN_NULL();

  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  char *interp_str = NULL;
  if (PG_NARGS() > 1 && ! PG_ARGISNULL(1))
  {
    text *interp_txt = PG_GETARG_TEXT_P(1);
    interp_str = text2cstring(interp_txt);
  }
  TSequenceSet *result = trgeo_to_tsequenceset(temp, interp_str);
  PG_FREE_IF_COPY(temp, 0);
  PG_RETURN_TSEQUENCESET_P(result);
}

/*****************************************************************************
 * Modification Functions
 *****************************************************************************/


PGDLLEXPORT Datum Trgeo_append_tinstant(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_append_tinstant);
/**
 * @ingroup mobilitydb_rgeo_modif
 * @brief Append an instant to a temporal rigid geometry
 * @sqlfn appendInstant()
 */
Datum
Trgeo_append_tinstant(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  TInstant *inst = PG_GETARG_TINSTANT_P(1);
  /* Get interpolation */
  interpType interp;
  if (PG_NARGS() == 2 || PG_ARGISNULL(2))
  {
    /* Set default interpolation according to the base type */
    meosType temptype = oid_type(get_fn_expr_argtype(fcinfo->flinfo, 1));
    interp = temptype_continuous(temptype) ? LINEAR : STEP;
  }
  else
    interp = input_interp_string(fcinfo, 2);
  Temporal *result = trgeo_append_tinstant(temp, inst, interp, 0.0, NULL,
    false);
  PG_FREE_IF_COPY(temp, 0);
  PG_FREE_IF_COPY(inst, 1);
  PG_RETURN_TEMPORAL_P(result);
}

PGDLLEXPORT Datum Trgeo_append_tsequence(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_append_tsequence);
/**
 * @ingroup mobilitydb_rgeo_modif
 * @brief Append a sequence to a temporal rigid geometry
 * @sqlfn appendSequence()
 */
Datum
Trgeo_append_tsequence(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  TSequence *seq = PG_GETARG_TSEQUENCE_P(1);
  Temporal *result = trgeo_append_tsequence(temp, seq, false);
  PG_FREE_IF_COPY(temp, 0);
  PG_FREE_IF_COPY(seq, 1);
  PG_RETURN_TEMPORAL_P(result);
}

PGDLLEXPORT Datum Trgeo_merge(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_merge);
/**
 * @ingroup mobilitydb_rgeo_modif
 * @brief Merge two temporal rigid geometries
 * @sqlfn merge()
 */
Datum
Trgeo_merge(PG_FUNCTION_ARGS)
{
  Temporal *temp1 = PG_ARGISNULL(0) ? NULL : PG_GETARG_TEMPORAL_P(0);
  Temporal *temp2 = PG_ARGISNULL(1) ? NULL : PG_GETARG_TEMPORAL_P(1);
  Temporal *result = trgeo_merge(temp1, temp2);
  if (temp1)
    PG_FREE_IF_COPY(temp1, 0);
  if (temp2)
    PG_FREE_IF_COPY(temp2, 1);
  if (! result)
    PG_RETURN_NULL();
  PG_RETURN_TEMPORAL_P(result);
}

PGDLLEXPORT Datum Trgeo_merge_array(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_merge_array);
/**
 * @ingroup mobilitydb_trgeo_modif
 * @brief Merge an array of temporal rigid geometries
 * @sqlfn merge()
 */
Datum
Trgeo_merge_array(PG_FUNCTION_ARGS)
{
  ArrayType *array = PG_GETARG_ARRAYTYPE_P(0);
  ensure_not_empty_array(array);
  int count;
  Temporal **temparr = temparr_extract(array, &count);
  Temporal *result = trgeo_merge_array(temparr, count);
  pfree(temparr);
  PG_FREE_IF_COPY(array, 0);
  PG_RETURN_TEMPORAL_P(result);
}

/*****************************************************************************
 * Restriction Functions
 *****************************************************************************/

/**
 * @brief Return a temporal rigid geometry restricted to (the complement of) a
 * base value
 */
static Datum
Trgeo_restrict_value(FunctionCallInfo fcinfo, bool atfunc)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  Datum value = PG_GETARG_ANYDATUM(1);
  meosType basetype = oid_type(get_fn_expr_argtype(fcinfo->flinfo, 1));
  Temporal *result = trgeo_restrict_value(temp, value, atfunc);
  PG_FREE_IF_COPY(temp, 0);
  DATUM_FREE_IF_COPY(value, basetype, 1);
  if (! result)
    PG_RETURN_NULL();
  PG_RETURN_TEMPORAL_P(result);
}

PGDLLEXPORT Datum Trgeo_at_value(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_at_value);
/**
 * @ingroup mobilitydb_rgeo_restrict
 * @brief Return a temporal rigid geometry restricted to a base value
 * @sqlfn atValue()
 */
Datum
Trgeo_at_value(PG_FUNCTION_ARGS)
{
  return Trgeo_restrict_value(fcinfo, REST_AT);
}

PGDLLEXPORT Datum Trgeo_minus_value(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_minus_value);
/**
 * @ingroup mobilitydb_rgeo_restrict
 * @brief Return a temporal rigid geometry restricted to the complement of a base value
 * @sqlfn minusValue()
 */
Datum
Trgeo_minus_value(PG_FUNCTION_ARGS)
{
  return Trgeo_restrict_value(fcinfo, REST_MINUS);
}

/*****************************************************************************/

/**
 * @brief Return a temporal rigid geometry restricted to (the complement of) a
 * set of base values
 */
static Datum
Trgeo_restrict_values(FunctionCallInfo fcinfo, bool atfunc)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  Set *s = PG_GETARG_SET_P(1);
  Temporal *result = trgeo_restrict_values(temp, s, atfunc);
  PG_FREE_IF_COPY(temp, 0);
  PG_FREE_IF_COPY(s, 1);
  if (! result)
    PG_RETURN_NULL();
  PG_RETURN_TEMPORAL_P(result);
}

PGDLLEXPORT Datum Trgeo_at_values(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_at_values);
/**
 * @ingroup mobilitydb_rgeo_restrict
 * @brief Return a temporal rigid geometry restricted to a set of base values
 * @sqlfn atValues()
 */
Datum
Trgeo_at_values(PG_FUNCTION_ARGS)
{
  return Trgeo_restrict_values(fcinfo, REST_AT);
}

PGDLLEXPORT Datum Trgeo_minus_values(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_minus_values);
/**
 * @ingroup mobilitydb_rgeo_restrict
 * @brief Return a temporal rigid geometry restricted to the complement of a
 * set of base values
 * @sqlfn minusValues()
 */
Datum
Trgeo_minus_values(PG_FUNCTION_ARGS)
{
  return Trgeo_restrict_values(fcinfo, REST_MINUS);
}

/**
 * @brief Return a temporal rigid geometry restricted to (the complement of) a
 * timestamptz
 */
static Datum
Trgeo_restrict_timestamptz(FunctionCallInfo fcinfo, bool atfunc)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  TimestampTz t = PG_GETARG_TIMESTAMPTZ(1);
  Temporal *result = trgeo_restrict_timestamptz(temp, t, atfunc);
  PG_FREE_IF_COPY(temp, 0);
  if (! result)
    PG_RETURN_NULL();
  PG_RETURN_TEMPORAL_P(result);
}

PGDLLEXPORT Datum Trgeo_at_timestamptz(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_at_timestamptz);
/**
 * @ingroup mobilitydb_rgeo_restrict
 * @brief Return a temporal rigid geometry restricted to a timestamptz
 * @sqlfn atTime()
 */
Datum
Trgeo_at_timestamptz(PG_FUNCTION_ARGS)
{
  return Trgeo_restrict_timestamptz(fcinfo, REST_AT);
}

PGDLLEXPORT Datum Trgeo_minus_timestamptz(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_minus_timestamptz);
/**
 * @ingroup mobilitydb_rgeo_restrict
 * @brief Return a temporal rigid geometry restricted to the complement of a
 * timestamptz
 * @sqlfn minusTime()
 */
Datum
Trgeo_minus_timestamptz(PG_FUNCTION_ARGS)
{
  return Trgeo_restrict_timestamptz(fcinfo, REST_MINUS);
}

/*****************************************************************************/

/**
 * @brief Return a temporal rigid geometry restricted to a timestamptz set
 * @sqlfn atTime()
 */
Datum
Trgeo_restrict_tstzset(FunctionCallInfo fcinfo, bool atfunc)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  Set *s = PG_GETARG_SET_P(1);
  Temporal *result = trgeo_restrict_tstzset(temp, s, atfunc);
  PG_FREE_IF_COPY(temp, 0);
  PG_FREE_IF_COPY(s, 1);
  if (! result)
    PG_RETURN_NULL();
  PG_RETURN_TEMPORAL_P(result);
}

PGDLLEXPORT Datum Trgeo_at_tstzset(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_at_tstzset);
/**
 * @ingroup mobilitydb_rgeo_restrict
 * @brief Return a temporal rigid geometry restricted to a timestamptz set
 * @sqlfn atTime()
 */
Datum
Trgeo_at_tstzset(PG_FUNCTION_ARGS)
{
  return Trgeo_restrict_tstzset(fcinfo, REST_AT);
}

PGDLLEXPORT Datum Trgeo_minus_tstzset(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_minus_tstzset);
/**
 * @ingroup mobilitydb_rgeo_restrict
 * @brief Return a temporal rigid geometry restricted to the complement of a
 * timestamptz
 * set
 * @sqlfn minusTime()
 */
Datum
Trgeo_minus_tstzset(PG_FUNCTION_ARGS)
{
  return Trgeo_restrict_tstzset(fcinfo, REST_MINUS);
}

/*****************************************************************************/

/**
 * @brief Return a temporal rigid geometry restricted to (the complement of) a
 * timestamptz span
 */
static Datum
Trgeo_restrict_tstzspan(FunctionCallInfo fcinfo, bool atfunc)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  Span *s = PG_GETARG_SPAN_P(1);
  Temporal *result = trgeo_restrict_tstzspan(temp, s, atfunc);
  PG_FREE_IF_COPY(temp, 0);
  if (! result)
    PG_RETURN_NULL();
  PG_RETURN_TEMPORAL_P(result);
}

PGDLLEXPORT Datum Trgeo_at_tstzspan(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_at_tstzspan);
/**
 * @ingroup mobilitydb_rgeo_restrict
 * @brief Return a temporal rigid geometry restricted to a timestamptz span
 * @sqlfn atTime()
 */
Datum
Trgeo_at_tstzspan(PG_FUNCTION_ARGS)
{
  return Trgeo_restrict_tstzspan(fcinfo, REST_AT);
}

PGDLLEXPORT Datum Trgeo_minus_tstzspan(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_minus_tstzspan);
/**
 * @ingroup mobilitydb_rgeo_restrict
 * @brief Return a temporal rigid geometry restricted to the complement of a
 * timestamptz
 * span
 * @sqlfn minusTime()
 */
Datum
Trgeo_minus_tstzspan(PG_FUNCTION_ARGS)
{
  return Trgeo_restrict_tstzspan(fcinfo, REST_MINUS);
}

/*****************************************************************************/

/**
 * @brief Return a temporal rigid geometry restricted to a timestamptz span set
 * @sqlfn atTime()
 */
Datum
Trgeo_restrict_tstzspanset(FunctionCallInfo fcinfo, bool atfunc)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  SpanSet *ss = PG_GETARG_SPANSET_P(1);
  Temporal *result = trgeo_restrict_tstzspanset(temp, ss, atfunc);
  PG_FREE_IF_COPY(temp, 0);
  PG_FREE_IF_COPY(ss, 1);
  if (! result)
    PG_RETURN_NULL();
  PG_RETURN_TEMPORAL_P(result);
}

PGDLLEXPORT Datum Trgeo_at_tstzspanset(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_at_tstzspanset);
/**
 * @ingroup mobilitydb_rgeo_restrict
 * @brief Return a temporal rigid geometry restricted to a timestamptz span set
 * @sqlfn atTime()
 */
Datum
Trgeo_at_tstzspanset(PG_FUNCTION_ARGS)
{
  return Trgeo_restrict_tstzspanset(fcinfo, REST_AT);
}

PGDLLEXPORT Datum Trgeo_minus_tstzspanset(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(Trgeo_minus_tstzspanset);
/**
 * @ingroup mobilitydb_rgeo_restrict
 * @brief Return a temporal rigid geometry restricted to the complement of a
 * span set
 * @sqlfn minusTime()
 */
Datum
Trgeo_minus_tstzspanset(PG_FUNCTION_ARGS)
{
  return Trgeo_restrict_tstzspanset(fcinfo, REST_MINUS);
}

/*****************************************************************************/


