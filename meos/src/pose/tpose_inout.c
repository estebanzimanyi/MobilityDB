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
 * @brief Output of temporal poses in WKT, EWKT, and MF-JSON format
 */

// #include "pose/tpose_out.h"

/* PostGIS */
#include <liblwgeom_internal.h>
/* MEOS */
#include <meos.h>
#include <meos_internal.h>
#include "general/pg_types.h"
#include "general/tinstant.h"
#include "general/tsequence.h"
#include "general/tsequenceset.h"
#include "geo/tgeo_spatialfuncs.h"
#include "pose/pose.h"
#include "pose/tpose_parser.h"

/*****************************************************************************
 * Input in WKT and EWKT format
 *****************************************************************************/

#if MEOS
/**
 * @ingroup meos_temporal_inout
 * @brief Return a temporal pose from its Well-Known Text (WKT) representation
 * @param[in] str String
 */
Temporal *
tpose_in(const char *str)
{
  /* Ensure validity of the arguments */
  if (! ensure_not_null((void *) str))
    return NULL;
  return tpose_parse(&str);
}
#endif /* MEOS */

/*****************************************************************************
 * Output in WKT and EWKT format
 *****************************************************************************/

/**
 * @brief Output a pose in the Well-Known Text (WKT) representation (internal
 * function.
 */
char *
pose_wkt_out_int(Datum value, bool extended, int maxdd)
{
  Pose *pose = DatumGetPoseP(value);
  bool hasz = MEOS_FLAGS_GET_Z(pose->flags);
  int32_t srid = pose_srid(pose);
  GSERIALIZED *gs = hasz ?
    geopoint_make(pose->data[0], pose->data[1], pose->data[2], true, false,
      srid) :
    geopoint_make(pose->data[0], pose->data[1], 0.0, false, false, srid);
  LWGEOM *geom = lwgeom_from_gserialized(gs);
  size_t len;
  char *wkt = lwgeom_to_wkt(geom, extended ? WKT_EXTENDED : WKT_ISO, maxdd, 
    &len);
  char *W, *X, *Y, *Z, *theta;
  if (hasz)
  {
    W = float8_out(pose->data[3], maxdd);
    X = float8_out(pose->data[4], maxdd);
    Y = float8_out(pose->data[5], maxdd);
    Z = float8_out(pose->data[6], maxdd);
    len += strlen(W) + strlen(X) + strlen(Y) + strlen(Z);
  }
  else
  {
    theta = float8_out(pose->data[2], maxdd);
    len += strlen(theta);
  }
  len += 8; // Pose(,) + end NULL
  char *result = palloc(len);
  if (hasz)
  {
    snprintf(result, len, "Pose(%s,%s,%s,%s,%s)", wkt, W, X, Y, Z);
    pfree(W); pfree(X); pfree(Y); pfree(Z); 
  }
  else
  {
    snprintf(result, len, "Pose(%s,%s)", wkt, theta);
    pfree(theta);
  }
  lwgeom_free(geom); pfree(wkt);
  return result;
}

/**
 * @brief Output a pose in the Well-Known Text (WKT)
 * representation
 * @note The parameter @p type is not needed for poses
 */
char *
pose_wkt_out(Datum value, meosType type __attribute__((unused)), int maxdd)
{
  return pose_wkt_out_int(value, false, maxdd);
}

/**
 * @brief Output a pose in the Extended Well-Known Text (EWKT)
 * representation, that is, in WKT representation prefixed with the SRID
 * @note The parameter @p type is not needed for temporal points
 */
char *
pose_ewkt_out(Datum value, meosType type __attribute__((unused)), int maxdd)
{
  return pose_wkt_out_int(value, true, maxdd);
}

/*****************************************************************************/

/**
 * @ingroup meos_base_inout
 * @brief Return the Well-Known Text (WKT) representation of a pose
 * @param[in] pose Pose
 * @param[in] maxdd Maximum number of decimal digits
 * @csqlfn #Pose_as_text()
 */
char *
pose_as_text(const Pose *pose, int maxdd)
{
  /* Ensure validity of the arguments */
  if (! ensure_not_null((void *) pose) || ! ensure_not_negative(maxdd))
    return NULL;

  return pose_wkt_out(PointerGetDatum(pose), 0, maxdd);
}

/**
 * @ingroup meos_base_inout
 * @brief Return the Extended Well-Known Text (EWKT) representation of a
 * pose
 * @param[in] pose Pose
 * @param[in] maxdd Maximum number of decimal digits
 * @csqlfn #Tpose_as_ewkt()
 */
char *
pose_as_ewkt(const Pose *pose, int maxdd)
{
  /* Ensure validity of the arguments */
  if (! ensure_not_null((void *) pose) || ! ensure_not_negative(maxdd))
    return NULL;

  int32_t srid = pose_srid(pose);
  char str1[18];
  if (srid > 0)
    /* SRID_MAXIMUM is defined by PostGIS as 999999 */
    snprintf(str1, sizeof(str1), "SRID=%d;", srid);
  else
    str1[0] = '\0';
  char *str2 = pose_wkt_out(PointerGetDatum(pose), 0, maxdd);
  char *result = palloc(strlen(str1) + strlen(str2) + 1);
  strcpy(result, str1);
  strcat(result, str2);
  pfree(str2);
  return result;
}

/*****************************************************************************/

#if MEOS
/**
 * @ingroup meos_temporal_inout
 * @brief Return the Well-Known Text (WKT) representation of a temporal pose
 * @param[in] temp Temporal pose
 * @param[in] maxdd Maximum number of decimal digits
 */
char *
tpose_out(const Temporal *temp, int maxdd)
{
  /* Ensure validity of the arguments */
  if (! ensure_not_null((void *) temp) || 
      ! ensure_temporal_isof_type(temp, T_TPOSE))
    return NULL;
  return temporal_out(temp, maxdd);
}
#endif /* MEOS */

/**
 * @ingroup meos_temporal_inout
 * @brief Return the Well-Known Text (WKT) representation of a temporal pose
 * @param[in] temp Temporal pose
 * @param[in] maxdd Maximum number of decimal digits
 * @csqlfn #Tpose_as_text()
 */
char *
tpose_as_text(const Temporal *temp, int maxdd)
{
  /* Ensure validity of the arguments */
  if (! ensure_not_null((void *) temp) || 
      ! ensure_temporal_isof_type(temp, T_TPOSE) ||
      ! ensure_not_negative(maxdd))
    return NULL;

  assert(temptype_subtype(temp->subtype));
  switch (temp->subtype)
  {
    case TINSTANT:
      return tinstant_to_string((TInstant *) temp, maxdd, &pose_wkt_out);
    case TSEQUENCE:
      return tsequence_to_string((TSequence *) temp, maxdd, false, &pose_wkt_out);
    default: /* TSEQUENCESET */
      return tsequenceset_to_string((TSequenceSet *) temp, maxdd, &pose_wkt_out);
  }
}

/**
 * @ingroup meos_temporal_inout
 * @brief Return the Extended Well-Known Text (EWKT) representation of a
 * temporal pose
 * @param[in] temp Temporal pose
 * @param[in] maxdd Maximum number of decimal digits
 * @csqlfn #Tpose_as_ewkt()
 */
char *
tpose_as_ewkt(const Temporal *temp, int maxdd)
{
  /* Ensure validity of the arguments */
  if (! ensure_not_null((void *) temp) || 
      ! ensure_temporal_isof_type(temp, T_TPOSE) ||
      ! ensure_not_negative(maxdd))
    return NULL;

  int32_t srid = tspatial_srid(temp);
  char str1[18];
  if (srid > 0)
    /* SRID_MAXIMUM is defined by PostGIS as 999999 */
    snprintf(str1, sizeof(str1), "SRID=%d%c", srid,
      (MEOS_FLAGS_GET_INTERP(temp->flags) == STEP) ? ',' : ';');
  else
    str1[0] = '\0';
  char *str2 = tpose_as_text(temp, maxdd);
  char *result = palloc(strlen(str1) + strlen(str2) + 1);
  strcpy(result, str1);
  strcat(result, str2);
  pfree(str2);
  return result;
}

/*****************************************************************************/

/**
 * @ingroup meos_internal_base_inout
 * @brief Return the (Extended) Well-Known Text (WKT or EWKT) representation
 * of a pose array
 * @param[in] posearr Array of poses
 * @param[in] count Number of elements in the input array
 * @param[in] maxdd Maximum number of decimal digits to output
 * @param[in] extended True if the output is in EWKT
 * @csqlfn #Posearr_as_text()
 */
char **
posearr_as_text(const Datum *posearr, int count, int maxdd, bool extended)
{
  /* Ensure validity of the arguments */
  if (! ensure_not_null((void *) posearr) || ! ensure_positive(count) ||
      ! ensure_not_negative(maxdd))
    return NULL;

  char **result = palloc(sizeof(char *) * count);
  for (int i = 0; i < count; i++)
    /* The pose_wkt_out and pose_ewkt_out functions do not use the second
     * argument */
    result[i] = extended ? pose_ewkt_out(posearr[i], 0, maxdd) : 
      pose_wkt_out(posearr[i], 0, maxdd);
  return result;
}

/**
 * @ingroup meos_internal_temporal_inout
 * @brief Return the (Extended) Well-Known Text (WKT or EWKT) representation
 * of an array of temporal poses
 * @param[in] temparr Array of temporal poses
 * @param[in] count Number of elements in the input array
 * @param[in] maxdd Maximum number of decimal digits to output
 * @param[in] extended True if the output is in EWKT
 * @csqlfn #Tposearr_as_text(), #Tposearr_as_ewkt()
 */
char **
tposearr_as_text(const Temporal **temparr, int count, int maxdd,
  bool extended)
{
  /* Ensure validity of the arguments */
  if (! ensure_not_null((void *) temparr) || ! ensure_positive(count) ||
      ! ensure_not_negative(maxdd))
    return NULL;

  char **result = palloc(sizeof(text *) * count);
  for (int i = 0; i < count; i++)
    result[i] = extended ? tpose_as_ewkt(temparr[i], maxdd) :
      tpose_as_text(temparr[i], maxdd);
  return result;
}

/*****************************************************************************/
