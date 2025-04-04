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
 * @brief Functions for parsing temporal rigid geometries
 */

#include "rgeo/trgeo_parser.h"

/* MEOS */
#include <meos.h>
#include <meos_internal.h>
#include "general/type_parser.h"
#include "general/type_util.h"
#include "geo/tgeo_spatialfuncs.h"
#include "geo/tspatial_parser.h"
#include "rgeo/trgeo_temporaltypes.h"

/*****************************************************************************/

/**
 * @brief Parse the reference geometry of a temporal rigid geometry value
 * @param[in] str Input string
 * @param[in] temptype Temporal type
 */
bool
trgeo_parse_geom(const char **str, int32_t temp_srid, Datum *result)
{
  p_whitespace(str);
  int pos = 0;
  while ((*str)[pos] != ';' && (*str)[pos] != '\0')
    pos++;
  if ((*str)[pos] == '\0')
  {
    meos_error(ERROR, MEOS_ERR_TEXT_INPUT,
      "Could not parse element value");
    return false;
  }
  char *str1 = palloc(sizeof(char) * (pos + 1));
  strncpy(str1, *str, pos);
  str1[pos] = '\0';
  Datum geom;
  bool success = basetype_in(str1, T_GEOMETRY, false, &geom);
  pfree(str1);
  if (! success)
    return false;
  /* The delimeter is consumed */
  *str += pos + 1;

  GSERIALIZED *gs = DatumGetGserializedP(geom);
  ensure_not_empty(gs);
  ensure_has_not_M_geo(gs);
  /* If one of the SRID of the temporal rigid geometry and of the geometry
   * is SRID_UNKNOWN and the other not, copy the SRID */
  int32_t geo_srid = gserialized_get_srid(gs);
  if (temp_srid == SRID_UNKNOWN && geo_srid != SRID_UNKNOWN)
    temp_srid = geo_srid;
  else if (temp_srid != SRID_UNKNOWN && geo_srid == SRID_UNKNOWN)
    gserialized_set_srid(gs, temp_srid);
  /* If the SRID of the temporal rigid geometry and of the geometry do not match */
  else if (temp_srid != SRID_UNKNOWN && geo_srid != SRID_UNKNOWN &&
    temp_srid != geo_srid)
  {
    meos_error(ERROR, MEOS_ERR_TEXT_INPUT,
      "Geometry SRID (%d) does not match temporal type SRID (%d)",
      geo_srid, temp_srid);
    pfree(gs);
    return false;
  }
  *result = geom;
  return true;
}

/**
 * @brief Parse a temporal rigid geometry value from the buffer.
 * @param[in] str Input string
 * @param[in] temptype Temporal type
 */
Temporal *
trgeo_parse(const char **str, meosType temptype)
{
  p_whitespace(str);

  /* Get the SRID if it is given */
  int temp_srid = SRID_UNKNOWN;
  srid_parse(str, &temp_srid);

  interpType interp = temptype_continuous(temptype) ? LINEAR : STEP;
  /* Starts with "Interp=Step" */
  if (strncasecmp(*str, "Interp=Step;", 12) == 0)
  {
    /* Move str after the semicolon */
    *str += 12;
    interp = STEP;
  }

  /* Parse the reference geometry */
  Datum geom;
  if (temptype == T_TRGEOMETRY)
    if (! trgeo_parse_geom(str, temp_srid, &geom))
      return NULL;

  p_whitespace(str);

  const char *bak = *str;
  Temporal *result = NULL; /* keep compiler quiet */
  /* Determine the subtype of the temporal spatial value and call the
   * function corresponding to the subtype passing the SRID */
  if (**str != '{' && **str != '[' && **str != '(')
  {
    TInstant *inst;
    if (! tspatialinst_parse(str, temptype, true, &temp_srid, &inst))
      return NULL;
    result = (Temporal *) inst;
  }
  else if (**str == '[' || **str == '(')
  {
    TSequence *seq;
    if (! tspatialseq_cont_parse(str, temptype, interp, true, &temp_srid, &seq))
      return NULL;
    result = (Temporal *) seq;
  }
  else if (**str == '{')
  {
    bak = *str;
    p_obrace(str);
    p_whitespace(str);
    if (**str == '[' || **str == '(')
    {
      *str = bak;
      result = (Temporal *) tspatialseqset_parse(str, temptype, interp,
        &temp_srid);
    }
    else
    {
      *str = bak;
      result = (Temporal *) tspatialseq_disc_parse(str, temptype, &temp_srid);
    }
  }
  return result;
}

/*****************************************************************************/
