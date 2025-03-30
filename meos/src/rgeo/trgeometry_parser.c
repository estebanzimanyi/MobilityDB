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

#include "rgeo/trgeometry_parser.h"

/* MEOS */
#include <meos.h>
#include <meos_internal.h>
#include "general/type_parser.h"
#include "general/type_util.h"
#include "geo/tgeo_spatialfuncs.h"
#include "geo/tspatial_parser.h"
#include "rgeo/trgeometry_temporaltypes.h"

/*****************************************************************************/

/**
 * @brief Parse a temporal rigid instant geometry from the buffer
 * @param[in] str Input string
 * @param[in] temptype Temporal type
 * @param[in] end Set to true when reading a single instant to ensure there is
 * no more input after the sequence
 * @param[in] make Set to false for the first pass to do not create the instant
 * @param[in] geom the reference geometry
 */
static TInstant *
trgeoinst_parse(const char **str, meosType temptype, bool end, bool make,
  Datum geom)
{
  p_whitespace(str);
  meosType basetype = temptype_basetype(temptype);
  /* The next two instructions will throw an exception if they fail */
  Datum value;
  if (! basetype_parse(str, basetype, '@', &value))
    return NULL;
  /* Consume the delimiter */
  // TODO
  TimestampTz t = timestamp_parse(str);
  if ((end && ! ensure_end_input(str, "trgeometry")) || ! make)
  {
    pfree(DatumGetPointer(value));
    return NULL;
  }
  TInstant *result = trgeoinst_make(geom, value, temptype, t);
  pfree(DatumGetPointer(value));
  return result;
}

/**
 * @brief Parse a temporal rigid discrete sequence geometry from the buffer
 * @param[in] str Input string
 * @param[in] temptype Temporal type
 * @param[in] geom the reference geometry
 */
static TSequence *
trgeo_discseq_parse(const char **str, meosType temptype, Datum geom)
{
  const char *type_str = "trgeometry";
  p_whitespace(str);
  /* We are sure to find an opening brace because that was the condition
   * to call this function in the dispatch function trgeo_parse */
  p_obrace(str);

  /* First parsing */
  const char *bak = *str;
  trgeoinst_parse(str, temptype, false, false, geom);
  int count = 1;
  while (p_comma(str))
  {
    count++;
    trgeoinst_parse(str, temptype, false, false, geom);
  }
  if (! ensure_cbrace(str, type_str) || ! ensure_end_input(str, type_str))
    return NULL;

  /* Second parsing */
  *str = bak;
  TInstant **instants = palloc(sizeof(TInstant *) * count);
  for (int i = 0; i < count; i++)
  {
    p_comma(str);
    instants[i] = trgeoinst_parse(str, temptype, false, true, geom);
  }
  p_cbrace(str);
  return trgeoseq_make_free(geom, instants, count, true, true,
    DISCRETE, NORMALIZE_NO);
}

/**
 * @brief Parse a temporal rigid sequence geometry from the buffer
 * @param[in] str Input string
 * @param[in] temptype Temporal type
 * @param[in] interp Interpolation
 * @param[in] end Set to true when reading a single instant to ensure there is
 * no more input after the sequence
 * @param[in] make Set to false for the first pass to do not create the instant
 * @param[in] geom the reference geometry
*/
static TSequence *
trgeo_contseq_parse(const char **str, meosType temptype, interpType interp,
  bool end, bool make, Datum geom)
{
  p_whitespace(str);
  bool lower_inc = false, upper_inc = false;
  /* We are sure to find an opening bracket or parenthesis because that was the
   * condition to call this function in the dispatch function trgeo_parse */
  if (p_obracket(str))
    lower_inc = true;
  else if (p_oparen(str))
    lower_inc = false;

  /* First parsing */
  const char *bak = *str;
  trgeoinst_parse(str, temptype, false, false, geom);
  int count = 1;
  while (p_comma(str))
  {
    count++;
    trgeoinst_parse(str, temptype, false, false, geom);
  }
  if (p_cbracket(str))
    upper_inc = true;
  else if (p_cparen(str))
    upper_inc = false;
  else
  {
    meos_error(ERROR, MEOS_ERR_TEXT_INPUT,
      "Could not parse temporal geometry value: Missing closing bracket/parenthesis");
    return NULL;
  }
  /* Ensure there is no more input */
  if ((end && ! ensure_end_input(str, "trgeometry")) || ! make)
    return NULL;

  /* Second parsing */
  *str = bak;
  TInstant **instants = palloc(sizeof(TInstant *) * count);
  for (int i = 0; i < count; i++)
  {
    p_comma(str);
    instants[i] = trgeoinst_parse(str, temptype, false, true, geom);
  }
  p_cbracket(str);
  p_cparen(str);
  return trgeoseq_make_free(geom, instants, count,
    lower_inc, upper_inc, interp, NORMALIZE);
}

/**
 * @brief Parse a temporal rigid sequence set geometry from the buffer.
 * @param[in] str Input string
 * @param[in] temptype Temporal type
 * @param[in] interp Interpolation
 * @param[in] geom the reference geometry
 */
static TSequenceSet *
trgeoseqset_parse(const char **str, meosType temptype, interpType interp,
  Datum geom)
{
  const char *type_str = "trgeometry";
  p_whitespace(str);
  /* We are sure to find an opening brace because that was the condition
   * to call this function in the dispatch function tpoint_parse */
  p_obrace(str);

  /* First parsing */
  const char *bak = *str;
  trgeo_contseq_parse(str, temptype, interp, false, false, geom);
  int count = 1;
  while (p_comma(str))
  {
    count++;
    trgeo_contseq_parse(str, temptype, interp, false, false, geom);
  }
  if (! ensure_cbrace(str, type_str) || ! ensure_end_input(str, type_str))
    return NULL;

  /* Second parsing */
  *str = bak;
  TSequence **sequences = palloc(sizeof(TSequence *) * count);
  for (int i = 0; i < count; i++)
  {
    p_comma(str);
    sequences[i] = trgeo_contseq_parse(str, temptype, interp, false, true,
      geom);
  }
  p_cbrace(str);
  return trgeoseqset_make_free(geom, sequences, count, NORMALIZE);
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
  int trgeo_srid = SRID_UNKNOWN;
  srid_parse(str, &trgeo_srid);

  interpType interp = temptype_continuous(temptype) ? LINEAR : STEP;
  /* Starts with "Interp=Step" */
  if (strncasecmp(*str, "Interp=Step;", 12) == 0)
  {
    /* Move str after the semicolon */
    *str += 12;
    interp = STEP;
  }

  /* Parse de reference geometry */
  p_whitespace(str);
  int pos = 0;
  while ((*str)[pos] != ';' && (*str)[pos] != '\0')
    pos++;
  if ((*str)[pos] == '\0')
  {
    meos_error(ERROR, MEOS_ERR_TEXT_INPUT,
      "Could not parse element value");
    return NULL;
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
  int geo_srid = gserialized_get_srid(gs);
  if (trgeo_srid == SRID_UNKNOWN && geo_srid != SRID_UNKNOWN)
    trgeo_srid = geo_srid;
  else if (trgeo_srid != SRID_UNKNOWN && geo_srid == SRID_UNKNOWN)
    gserialized_set_srid(gs, trgeo_srid);
  /* If the SRID of the temporal rigid geometry and of the geometry do not match */
  else if (trgeo_srid != SRID_UNKNOWN && geo_srid != SRID_UNKNOWN &&
    trgeo_srid != geo_srid)
  {
    meos_error(ERROR, MEOS_ERR_TEXT_INPUT,
      "Geometry SRID (%d) does not match temporal type SRID (%d)",
      geo_srid, trgeo_srid);
    pfree(gs);
    return NULL;
  }

  p_whitespace(str);
  Temporal *result = NULL; /* keep compiler quiet */
  /* Determine the type of the temporal rigid geometry */
  if (**str != '{' && **str != '[' && **str != '(')
  {
    result = (Temporal *) trgeoinst_parse(str, temptype, true, true, geom);
  }
  else if (**str == '[' || **str == '(')
    result = (Temporal *) trgeo_contseq_parse(str, temptype, interp,
      true, true, geom);
  else if (**str == '{')
  {
    const char *bak = *str;
    p_obrace(str);
    p_whitespace(str);
    if (**str == '[' || **str == '(')
    {
      *str = bak;
      result = (Temporal *) trgeoseqset_parse(str, temptype, interp, geom);
    }
    else
    {
      *str = bak;
      result = (Temporal *) trgeo_discseq_parse(str, temptype, geom);
    }
  }
  pfree(gs);
  return result;
}

/*****************************************************************************/
