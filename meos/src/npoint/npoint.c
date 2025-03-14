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
 * @brief Network-based static point and segment types
 *
 * Several functions are commented out since they are not currently used.
 * They are kept if needed in the future.
 */

#include "npoint/tnpoint.h"

/* C */
#include <assert.h>
#include <float.h>
/* PostgreSQL */
#include <postgres.h>
#if ! MEOS
  #include <libpq/pqformat.h>
  #include <executor/spi.h>
#endif /* ! MEOS */
/* PostGIS */
#include <liblwgeom.h>
/* MEOS */
#include <meos.h>
#include <meos_geo.h>
#include <meos_npoint.h>
#include <meos_internal.h>
#include "general/pg_types.h"
#include "general/tsequence.h"
#include "general/type_out.h"
#include "general/type_util.h"
#include "geo/pgis_types.h"
#include "geo/tgeo.h"
#include "geo/tgeo_out.h"
#include "geo/tgeo_spatialfuncs.h"
#include "npoint/tnpoint_parser.h"
#include "npoint/tnpoint.h"

/** Buffer size for input and output of npoint and nsegment values */
#define NPOINT_MAXLEN     128
#define NSEGMENT_MAXLEN   128

/* Global variable saving the SRID of the ways table */
static int32_t SRID_WAYS = SRID_INVALID;

/*****************************************************************************
 * Collinear function
 *****************************************************************************/

/**
 * @brief Return true if the three values are collinear
 * @param[in] np1,np2,np3 Input values
 * @param[in] ratio Value in [0,1] representing the duration of the
 * timestamps associated to `np1` and `np2` divided by the duration
 * of the timestamps associated to `np1` and `np3`
 */
bool
npoint_collinear(Npoint *np1, Npoint *np2, Npoint *np3, double ratio)
{
  return float_collinear(np1->pos, np2->pos, np3->pos, ratio);
}

/*****************************************************************************
 * Definitions for reading the ways.csv file
 *****************************************************************************/

#if MEOS
/* Maximum length in characters of a header record in the input CSV file */
#define MAX_LENGTH_HEADER 1024
/* Maximum length in characters of a geometry in the input data */
#define MAX_LENGTH_GEOM 100001
/* Location of the ways.csv file */
#define WAYS_CSV "/home/esteban/src/MobilityDB/meos/examples/data/ways.csv"

typedef struct
{
  long int gid;
  GSERIALIZED *the_geom;
} ways_record;
#endif /* MEOS */

/*****************************************************************************
 * General functions
 *****************************************************************************/

/**
 * @ingroup meos_temporal_accessor
 * @brief Return the SRID of the routes in the ways table
 * @return On error return SRID_INVALID
 */
#if MEOS
int32_t
get_srid_ways()
{
  /* Get the value from the global variable if it has been already set */
  if (SRID_WAYS != SRID_INVALID)
    return SRID_WAYS;

  /* Substitute the full file path in the first argument of fopen */
  FILE *file = fopen(WAYS_CSV, "r");

  if (! file)
  {
    meos_error(ERROR, MEOS_ERR_INTERNAL_TYPE_ERROR,
      "Cannot open the ways.csv file");
    return SRID_INVALID;
  }

  bool found = false;
  ways_record rec;
  char header_buffer[MAX_LENGTH_HEADER];
  char geo_buffer[MAX_LENGTH_GEOM];

  /* Read the first line of the file with the headers */
  fscanf(file, "%1023s\n", header_buffer);

  /* Continue reading the file */
  do
  {
    int read = fscanf(file, "%ld,%100000[^\n]\n",
      &rec.gid, geo_buffer);

    if (ferror(file))
    {
      meos_error(ERROR, MEOS_ERR_INTERNAL_TYPE_ERROR,
        "Error reading the ways.csv file");
      return SRID_INVALID;
    }

    /* Ignore the records with NULL values */
    if (read == 3)
    {
      /* Transform the string representing the geometry into a geometry value */
      rec.the_geom = geom_in(geo_buffer, -1);
      if (geo_is_empty(rec.the_geom))
      {
        free(rec.the_geom);
        continue;
      }
      found = true;
      break;
    }
  } while (!feof(file));

  /* Close the input file */
  fclose(file);
  
  if (! found)
    return SRID_INVALID;
  
  int32_t result = gserialized_get_srid(rec.the_geom);
  free(rec.the_geom);
  /* Save the SRID value in a global variable */
  SRID_WAYS = result;
  return result;  
}
#else
int32_t
get_srid_ways()
{
  /* Get the value from the global variable if it has been already set */
  if (SRID_WAYS != SRID_INVALID)
    return SRID_WAYS;
  
  /* Fetch the SRID value from the table */
  int32_t result = 0; /* make compiler quiet */
  bool isNull = true;
  SPI_connect();
  int ret = SPI_execute("SELECT ST_SRID(the_geom) FROM public.ways LIMIT 1;",
    true, 1);
  uint64 proc = SPI_processed;
  if (ret > 0 && proc > 0 && SPI_tuptable)
  {
    SPITupleTable *tuptable = SPI_tuptable;
    Datum value = SPI_getbinval(tuptable->vals[0], tuptable->tupdesc, 1, &isNull);
    if (isNull)
    {
      meos_error(ERROR, MEOS_ERR_INTERNAL_TYPE_ERROR,
        "Cannot determine SRID of the ways table");
      return SRID_INVALID;
    }
    result = DatumGetInt32(value);
  }
  else
  {
    meos_error(ERROR, MEOS_ERR_INTERNAL_TYPE_ERROR,
      "Cannot determine SRID of the ways table");
    return SRID_INVALID;
  }
  SPI_finish();
  /* Save the SRID value in a global variable */
  SRID_WAYS = result;
  return result;
}
#endif /* MEOS */

#define SQL_ROUTE_MAXLEN 64

/**
 * @ingroup meos_temporal_accessor
 * @brief Return true if the edge table contains a route with the route
 * identifier
 * @param[in] rid Route identifier
 */
#if MEOS
bool
route_exists(int64 rid)
{
  /* Substitute the full file path in the first argument of fopen */
  FILE *file = fopen(WAYS_CSV, "r");

  if (! file)
  {
    meos_error(ERROR, MEOS_ERR_INTERNAL_TYPE_ERROR,
      "Cannot open the ways.csv file");
    return false;
  }

  bool result = false;
  ways_record rec;
  char header_buffer[MAX_LENGTH_HEADER];
  char geo_buffer[MAX_LENGTH_GEOM];

  /* Read the first line of the file with the headers */
  fscanf(file, "%1023s\n", header_buffer);

  /* Continue reading the file */
  do
  {
    int read = fscanf(file, "%ld,%100000[^\n]\n",
      &rec.gid, geo_buffer);

    if (ferror(file))
    {
      meos_error(ERROR, MEOS_ERR_INTERNAL_TYPE_ERROR,
        "Error reading the ways.csv file");
      return false;
    }

    /* Ignore the records with NULL values */
    if (read == 2)
    {
      /* Transform the string representing the geometry into a geometry value */
      rec.the_geom = geom_in(geo_buffer, -1);
      if (geo_is_empty(rec.the_geom))
      {
        free(rec.the_geom);
        continue;
      }
      if (rec.gid == rid)
      {
        result = true;
        break;
      }
    }
  } while (!feof(file));

  /* Close the input file */
  fclose(file);
  
  return result; 
}
#else
bool
route_exists(int64 rid)
{
  char sql[SQL_ROUTE_MAXLEN];
  snprintf(sql, sizeof(sql),
    "SELECT true FROM public.ways WHERE gid = %ld", rid);
  bool isNull = true;
  bool result = false;
  SPI_connect();
  int ret = SPI_execute(sql, true, 1);
  uint64 proc = SPI_processed;
  if (ret > 0 && proc > 0 && SPI_tuptable)
  {
    SPITupleTable *tuptable = SPI_tuptable;
    result = DatumGetBool(SPI_getbinval(tuptable->vals[0],
      tuptable->tupdesc, 1, &isNull));
  }
  SPI_finish();
  return result;
}
#endif /* MEOS */

/**
 * @ingroup meos_temporal_accessor
 * @brief Access the edge table to return the route length from the
 * corresponding route identifier
 * @param[in] rid Route identifier
 * @return On error return -1.0
 */
#if MEOS
double
route_length(int64 rid)
{
  /* Substitute the full file path in the first argument of fopen */
  FILE *file = fopen(WAYS_CSV, "r");

  if (! file)
  {
    meos_error(ERROR, MEOS_ERR_INTERNAL_TYPE_ERROR,
      "Cannot open the ways.csv file");
    return -1.0;
  }

  bool found = false;
  ways_record rec;
  char header_buffer[MAX_LENGTH_HEADER];
  char geo_buffer[MAX_LENGTH_GEOM];

  /* Read the first line of the file with the headers */
  fscanf(file, "%1023s\n", header_buffer);

  /* Continue reading the file */
  do
  {
    int read = fscanf(file, "%ld,%100000[^\n]\n",
      &rec.gid, geo_buffer);

    if (ferror(file))
    {
      meos_error(ERROR, MEOS_ERR_INTERNAL_TYPE_ERROR,
        "Error reading the ways.csv file");
      return -1.0;
    }

    /* Ignore the records with NULL values */
    if (read == 2)
    {
      /* Transform the string representing the geometry into a geometry value */
      rec.the_geom = geom_in(geo_buffer, -1);
      if (geo_is_empty(rec.the_geom))
      {
        free(rec.the_geom);
        continue;
      }
      if (rec.gid == rid)
      {
        found = true;
        break;
      }
    }
  } while (!feof(file));

  /* Close the input file */
  fclose(file);
  
  if (! found)
    return -1.0;
  
  double result = geom_length(rec.the_geom);
  free(rec.the_geom);
  return result; 
}

#else
double
route_length(int64 rid)
{
  char sql[SQL_ROUTE_MAXLEN];
  snprintf(sql, sizeof(sql),
    "SELECT length FROM public.ways WHERE gid = %ld", rid);
  bool isNull = true;
  double result = 0.0;
  SPI_connect();
  int ret = SPI_execute(sql, true, 1);
  uint64 proc = SPI_processed;
  if (ret > 0 && proc > 0 && SPI_tuptable)
  {
    SPITupleTable *tuptable = SPI_tuptable;
    result = DatumGetFloat8(SPI_getbinval(tuptable->vals[0],
      tuptable->tupdesc, 1, &isNull));
  }
  SPI_finish();

  if (isNull)
  {
    meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
      "Cannot get the length for route %ld", rid);
    return -1.0;
  }
  return result;
}
#endif /* MEOS */

/**
 * @ingroup meos_temporal_accessor
 * @brief Access the edge table to get the route geometry from corresponding
 * route identifier
 * @param[in] rid Route identifier
 * @return On error return @p NULL
 */
#if MEOS
GSERIALIZED *
route_geom(int64 rid)
{
  /* Substitute the full file path in the first argument of fopen */
  FILE *file = fopen(WAYS_CSV, "r");

  if (! file)
  {
    meos_error(ERROR, MEOS_ERR_INTERNAL_TYPE_ERROR,
      "Cannot open the ways.csv file");
    return NULL;
  }

  bool found = false;
  ways_record rec;
  char header_buffer[MAX_LENGTH_HEADER];
  char geo_buffer[MAX_LENGTH_GEOM];

  /* Read the first line of the file with the headers */
  fscanf(file, "%1023s\n", header_buffer);

  /* Continue reading the file */
  do
  {
    int read = fscanf(file, "%ld,%100000[^\n]\n",
      &rec.gid, geo_buffer);

    if (ferror(file))
    {
      meos_error(ERROR, MEOS_ERR_INTERNAL_TYPE_ERROR,
        "Error reading the ways.csv file");
      return NULL;
    }

    /* Ignore the records with NULL values */
    if (read == 3)
    {
      /* Transform the string representing the geometry into a geometry value */
      rec.the_geom = geom_in(geo_buffer, -1);
      if (geo_is_empty(rec.the_geom))
      {
        free(rec.the_geom);
        continue;
      }
      if (rec.gid == rid)
      {
        found = true;
        break;
      }
    }
  } while (!feof(file));

  /* Close the input file */
  fclose(file);
  
  if (! found)
    return NULL;
  
  GSERIALIZED *result = geo_copy(rec.the_geom);
  free(rec.the_geom);
  return result; 
}
#else
GSERIALIZED *
route_geom(int64 rid)
{
  char sql[SQL_ROUTE_MAXLEN];
  snprintf(sql, sizeof(sql),
    "SELECT the_geom FROM public.ways WHERE gid = %ld", rid);
  bool isNull = true;
  GSERIALIZED *result = NULL;
  SPI_connect();
  int ret = SPI_execute(sql, true, 1);
  uint64 proc = SPI_processed;
  if (ret > 0 && proc > 0 && SPI_tuptable)
  {
    SPITupleTable *tuptable = SPI_tuptable;
    Datum line = SPI_getbinval(tuptable->vals[0], tuptable->tupdesc, 1, &isNull);
    if (! isNull)
    {
      /* Must allocate this in upper executor context to keep it alive after SPI_finish() */
      GSERIALIZED *gs = (GSERIALIZED *) PG_DETOAST_DATUM(line);
      result = (GSERIALIZED *) SPI_palloc(gs->size);
      memcpy(result, gs, gs->size);
    }
  }
  SPI_finish();

  if (isNull)
  {
    meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
      "Cannot get the geometry for route %ld", rid);
    return NULL;
  }

  if (! ensure_not_empty(result))
  {
    pfree(result);
    return NULL;
  }

  return result;
}
#endif /* MEOS */

#define SQL_MAXLEN 1024

/**
 * @ingroup meos_temporal_conversion
 * @brief Transform a geometry into a network point
 * @param[in] gs Geometry
 * @csqlfn #Geom_to_npoint()
 */
#if MEOS
Npoint *
geom_npoint(const GSERIALIZED *gs)
{
  /* Ensure validity of the arguments */
  if (! ensure_not_null((void *) gs) || ! ensure_not_empty(gs) ||
      ! ensure_point_type(gs))
    return NULL;
  int32_t srid_geom = gserialized_get_srid(gs);
  int32_t srid_ways = get_srid_ways();
  if (srid_ways == SRID_INVALID || ! ensure_same_srid(srid_geom, srid_ways))
    return NULL;

  /* Substitute the full file path in the first argument of fopen */
  FILE *file = fopen(WAYS_CSV, "r");

  if (! file)
  {
    meos_error(ERROR, MEOS_ERR_INTERNAL_TYPE_ERROR,
      "Cannot open the ways.csv file");
    return NULL;
  }

  /* Record holding one line of the file */
  ways_record rec;
  /* Buffers for reading one line of the file */
  char header_buffer[MAX_LENGTH_HEADER];
  char geo_buffer[MAX_LENGTH_GEOM];
  /* Distances */
  double dist, min_dist = DBL_MAX;
  /* Position in the geometry with the shortest distance */
  double pos;

  /* Read the first line of the file with the headers */
  fscanf(file, "%1023s\n", header_buffer);

  /* Continue reading the file */
  do
  {
    int read = fscanf(file, "%ld,%100000[^\n]\n",
      &rec.gid, geo_buffer);

    if (ferror(file))
    {
      meos_error(ERROR, MEOS_ERR_INTERNAL_TYPE_ERROR,
        "Error reading the ways.csv file");
      return NULL;
    }

    /* Ignore the records with NULL values */
    if (read == 3)
    {
      /* Transform the string representing the geometry into a geometry value */
      rec.the_geom = geom_in(geo_buffer, -1);
      if (geo_is_empty(rec.the_geom))
      {
        free(rec.the_geom);
        continue;
      }

      /* We need to implement the following SQL query for a given geo
       *   SELECT npoint(gid, ST_LineLocatePoint(the_geom, geo))
       *   FROM public.ways WHERE ST_DWithin(the_geom, geo, DIST_EPSILON)
       *   ORDER BY ST_Distance(the_geom, geo) LIMIT 1;
       */
      
      pos = line_locate_point(rec.the_geom, gs);
      if (pos < 0)
      {
        free(rec.the_geom);
        continue;
      }

      dist = geom_distance2d(rec.the_geom, gs);
      if (dist < min_dist)
        min_dist = dist;

    }    
  } while (!feof(file));

  /* Close the input file */
  fclose(file);
  
  /* If the point was not found */
  if (! gs)
    return NULL;
  
  Npoint *result = npoint_make(rec.gid, pos);
  free(rec.the_geom);
  return result; 
}
#else
Npoint *
geom_npoint(const GSERIALIZED *gs)
{
  /* Ensure validity of the arguments */
  if (! ensure_not_null((void *) gs) || ! ensure_not_empty(gs) ||
      ! ensure_point_type(gs))
    return NULL;
  int32_t srid_geom = gserialized_get_srid(gs);
  int32_t srid_ways = get_srid_ways();
  if (srid_ways == SRID_INVALID || ! ensure_same_srid(srid_geom, srid_ways))
    return NULL;

  char *geomstr = geo_ewkt_out(PointerGetDatum(gs), 0, OUT_DEFAULT_DECIMAL_DIGITS);
  char sql[SQL_MAXLEN];
  snprintf(sql, sizeof(sql),
    "SELECT npoint(gid, ST_LineLocatePoint(the_geom, '%s')) "
    "FROM public.ways WHERE ST_DWithin(the_geom, '%s', %lf) "
    "ORDER BY ST_Distance(the_geom, '%s') LIMIT 1", geomstr, geomstr,
    DIST_EPSILON, geomstr);
  pfree(geomstr);
  Npoint *result = palloc(sizeof(Npoint));
  bool isNull = true;
  SPI_connect();
  int ret = SPI_execute(sql, true, 1);
  uint64 proc = SPI_processed;
  if (ret > 0 && proc > 0 && SPI_tuptable)
  {
    SPITupleTable *tuptable = SPI_tuptable;
    Datum value = SPI_getbinval(tuptable->vals[0], tuptable->tupdesc, 1, &isNull);
    if (! isNull)
    {
      /* Must allocate this in upper executor context to keep it alive after SPI_finish() */
      Npoint *np = DatumGetNpointP(value);
      memcpy(result, np, sizeof(Npoint));
    }
  }
  SPI_finish();
  if (isNull)
  {
    pfree(result);
    return NULL;
  }
  return result;
}
#endif /* MEOS */

/*****************************************************************************
 * Transformation functions
 *****************************************************************************/

/**
 * @brief Return an array of network points converted into a geometry
 * @param[in] points Array of network points
 * @param[in] nelems Number of elements in the input array
 * @pre The argument @p nelems is greater than 1, all points have the same SRID
 */
GSERIALIZED *
npointarr_geom(Npoint **points, int nelems)
{
  assert(nelems > 1);
  LWGEOM **geoms = palloc(sizeof(LWGEOM *) * nelems);
  int32_t srid = npoint_srid(points[0]);
  for (int i = 0; i < nelems; i++)
  {
    GSERIALIZED *gsline = route_geom(points[i]->rid);
    assert(gserialized_get_srid(gsline) == srid);
    LWGEOM *line = lwgeom_from_gserialized(gsline);
    geoms[i] = lwgeom_line_interpolate_point(line, points[i]->pos, srid, 0);
    pfree(gsline); pfree(line);
  }
  int newcount;
  LWGEOM **newgeoms = lwpointarr_remove_duplicates(geoms, nelems, &newcount);
  LWGEOM *geom = lwpointarr_make_trajectory(newgeoms, newcount, STEP);
  GSERIALIZED *result = geo_serialize(geom);
  pfree(newgeoms); pfree(geom);
  pfree_array((void **) geoms, nelems);
  return result;
}

/**
 * @brief Return an array of network segments converted into a geometry
 * @param[in] segments Array of network segments
 * @param[in] nelems Number of elements in the input array
 * @pre The argument @p nelems is greater than 1
 */
GSERIALIZED *
nsegmentarr_geom(Nsegment **segments, int nelems)
{
  assert(nelems > 1);
  GSERIALIZED **geoms = palloc(sizeof(GSERIALIZED *) * nelems);
  for (int i = 0; i < nelems; i++)
  {
    GSERIALIZED *line = route_geom(segments[i]->rid);
    if (segments[i]->pos1 == 0 && segments[i]->pos2 == 1)
      geoms[i] = geo_copy(line);
    else if (segments[i]->pos1 == segments[i]->pos2)
      geoms[i] = line_interpolate_point(line, segments[i]->pos1, 0);
    else
      geoms[i] = line_substring(line, segments[i]->pos1, segments[i]->pos2);
    pfree(line);
  }
  GSERIALIZED *result = geom_array_union(geoms, nelems);
  pfree_array((void **) geoms, nelems);
  return result;
}

/*****************************************************************************
 * General functions
 *****************************************************************************/

/**
 * @brief Comparator function for network segments
 */
static int
nsegment_sort_cmp(Nsegment **l, Nsegment **r)
{
  return nsegment_cmp(*l, *r);
}

/**
 * @brief Sort function for network segments
 */
static void
nsegmentarr_sort(Nsegment **segments, int nelems)
{
  qsort(segments, (size_t) nelems, sizeof(Nsegment *),
      (qsort_comparator) &nsegment_sort_cmp);
  return;
}

/**
 * @brief Normalize an array of temporal segments
 */
Nsegment **
nsegmentarr_normalize(Nsegment **segments, int *nelems)
{
  assert(*nelems != 0);
  nsegmentarr_sort(segments, *nelems);
  int newcount = 0;
  Nsegment **result = palloc(sizeof(Nsegment *) * *nelems);
  Nsegment *current = segments[0];
  for (int i = 1; i < *nelems; i++)
  {
    Nsegment *seg = segments[i];
    if (current->rid == seg->rid)
    {
      current->pos1 = Min(current->pos1, seg->pos1);
      current->pos2 = Max(current->pos2, seg->pos2);
      pfree(seg);
    }
    else
    {
      result[newcount++] = current;
      current = seg;
    }
  }
  result[newcount++] = current;
  *nelems = newcount;
  return result;
}

/*****************************************************************************
 * Input/output functions
 *****************************************************************************/

/**
 * @ingroup meos_temporal_inout
 * @brief Return a network point from its string representation
 * @param[in] str String
 * @csqlfn #Npoint_in()
 */
Npoint *
npoint_in(const char *str)
{
  return npoint_parse(&str, true);
}

/**
 * @ingroup meos_temporal_inout
 * @brief Return the string representation of a network point
 * @param[in] np Network point
 * @param[in] maxdd Maximum number of decimal digits
 * @csqlfn #Npoint_out()
 */
char *
npoint_out(const Npoint *np, int maxdd)
{
  /* Ensure validity of the arguments */
  if (! ensure_not_null((void *) np) || ! ensure_not_negative(maxdd))
    return NULL;

  char *result = palloc(NPOINT_MAXLEN);
  char *rid = int8_out(np->rid);
  char *pos = float8_out(np->pos, maxdd);
  snprintf(result, NPOINT_MAXLEN, "NPoint(%s,%s)", rid, pos);
  pfree(rid); pfree(pos);
  return result;
}

/*****************************************************************************/

/**
 * @ingroup meos_temporal_inout
 * @brief Return a network point from its string representation
 * @param[in] str String
 * @csqlfn #Nsegment_in()
 */
Nsegment *
nsegment_in(const char *str)
{
  return nsegment_parse(&str);
}

/**
 * @ingroup meos_temporal_inout
 * @brief Return the string representation of a network segment
 * @param[in] ns Network segment
 * @param[in] maxdd Maximum number of decimal digits
 * @csqlfn #Nsegment_out()
 */
char *
nsegment_out(const Nsegment *ns, int maxdd)
{
  /* Ensure validity of the arguments */
  if (! ensure_not_null((void *) ns) || ! ensure_not_negative(maxdd))
    return NULL;

  char *result = palloc(NSEGMENT_MAXLEN);
  char *rid = int8_out(ns->rid);
  char *pos1 = float8_out(ns->pos1, maxdd);
  char *pos2 = float8_out(ns->pos2, maxdd);
  snprintf(result, NSEGMENT_MAXLEN, "NSegment(%s,%s,%s)", rid, pos1, pos2);
  pfree(rid); pfree(pos1); pfree(pos2);
  return result;
}

/*****************************************************************************
 * Constructor functions
 *****************************************************************************/

/**
 * @ingroup meos_temporal_constructor
 * @brief Return a network point from a route identifier and a position
 * @param[in] rid Route identifier
 * @param[in] pos Position
 * @csqlfn #Npoint_constructor()
 */
Npoint *
npoint_make(int64 rid, double pos)
{
  /* Note: zero-fill is done in the npoint_set function */
  Npoint *result = palloc(sizeof(Npoint));
  npoint_set(rid, pos, result);
  return result;
}

/**
 * @brief Return in the last argument a network point constructed from a route
 * identifier and a position
 */
void
npoint_set(int64 rid, double pos, Npoint *np)
{
  if (! route_exists(rid))
  {
    meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
      "There is no route with gid value %ld in table ways", rid);
    return;
  }
  if (pos < 0 || pos > 1)
  {
    meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
      "The relative position must be a real number between 0 and 1");
    return;
  }
  /* Note: zero-fill is required here, just as in heap tuples */
  memset(np, 0, sizeof(Npoint));
  /* Fill in the network point */
  np->rid = rid;
  np->pos = pos;
  return;
}

/**
 * @ingroup meos_temporal_constructor
 * @brief Return a network segment from a route identifier and two positions
 * @param[in] rid Route identifier
 * @param[in] pos1, pos2 Positions
 * @csqlfn #Nsegment_constructor()
 */
Nsegment *
nsegment_make(int64 rid, double pos1, double pos2)
{
  /* Note: zero-fill is done in the nsegment_set function */
  Nsegment *result = palloc(sizeof(Nsegment));
  nsegment_set(rid, pos1, pos2, result);
  return result;
}

/**
 * @brief Return in the last argument a network segment constructed from a
 * route identifier and two positions
 */
void
nsegment_set(int64 rid, double pos1, double pos2, Nsegment *ns)
{
  if (! route_exists(rid))
  {
    meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
      "There is no route with gid value %ld in table ways", rid);
    return;
  }
  if (pos1 < 0 || pos1 > 1 || pos2 < 0 || pos2 > 1)
  {
    meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
      "The relative position of a network segment must be a real number between 0 and 1");
    return;
  }
  ns->rid = rid;
  ns->pos1 = Min(pos1, pos2);
  ns->pos2 = Max(pos1, pos2);
  return;
}

/*****************************************************************************
 * Conversion functions
 *****************************************************************************/

/**
 * @ingroup meos_temporal_conversion
 * @brief Return a network point converted to a network segment
 * @param[in] np Network point
 * @csqlfn #Npoint_to_nsegment()
 */
Nsegment *
npoint_nsegment(const Npoint *np)
{
  return nsegment_make(np->rid, np->pos, np->pos);
}

/*****************************************************************************
 * Accessor functions
 *****************************************************************************/

/**
 * @ingroup meos_temporal_accessor
 * @brief Return the route of a network point
 * @param[in] np Network point
 * @csqlfn #Npoint_route()
 */
int64
npoint_route(const Npoint *np)
{
  return np->rid;
}

/**
 * @ingroup meos_temporal_accessor
 * @brief Return the position of a network point
 * @param[in] np Network point
 * @csqlfn #Npoint_position()
 */
double
npoint_position(const Npoint *np)
{
  return np->pos;
}

/**
 * @ingroup meos_temporal_accessor
 * @brief Return the route of a network segment
 * @param[in] ns Network segment
 * @csqlfn #Nsegment_route()
 */
int64
nsegment_route(const Nsegment *ns)
{
  return ns->rid;
}

/**
 * @ingroup meos_temporal_accessor
 * @brief Return the start position of a network segment
 * @param[in] ns Network segment
 * @csqlfn #Nsegment_start_position()
 */
double
nsegment_start_position(const Nsegment *ns)
{
  return ns->pos1;
}

/**
 * @ingroup meos_temporal_accessor
 * @brief Return the end position of a network segment
 * @param[in] ns Network segment
 * @csqlfn #Nsegment_end_position()
 */
double
nsegment_end_position(const Nsegment *ns)
{
  return ns->pos2;
}

/*****************************************************************************
 * Conversions between network and Euclidean space
 *****************************************************************************/

/**
 * @ingroup meos_temporal_conversion
 * @brief Transform a network point into a geometry
 * @param[in] np Network point
 * @csqlfn #Npoint_to_geom()
 */
GSERIALIZED *
npoint_geom(const Npoint *np)
{
  GSERIALIZED *line = route_geom(np->rid);
  GSERIALIZED *result = line_interpolate_point(line, np->pos, 0);
  pfree(line);
  return result;
}

/**
 * @ingroup meos_temporal_conversion
 * @brief Transform a network segment into a geometry
 * @param[in] ns Network segment
 * @csqlfn #Nsegment_to_geom()
 */
GSERIALIZED *
nsegment_geom(const Nsegment *ns)
{
  GSERIALIZED *line = route_geom(ns->rid);
  GSERIALIZED *result;
  if (fabs(ns->pos1 - ns->pos2) < MEOS_EPSILON)
    result = line_interpolate_point(line, ns->pos1, 0);
  else
    result = line_substring(line, ns->pos1, ns->pos2);
  pfree(line);
  return result;
}

/**
 * @ingroup meos_temporal_conversion
 * @brief Transform a geometry into a network segment
 * @return On error return @p NULL
 * @param[in] gs Geometry
 * @csqlfn #Geom_to_nsegment()
 */
Nsegment *
geom_nsegment(const GSERIALIZED *gs)
{
  /* Ensure validity of the arguments */
  if (! ensure_not_null((void *) gs) || ! ensure_not_empty(gs))
    return NULL;
  int geomtype = gserialized_get_type(gs);
  if (geomtype != POINTTYPE && geomtype != LINETYPE)
  {
    meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
      "Only point or line geometries accepted");
    return NULL;
  }

  Npoint **points;
  Npoint *np;
  int npoints = 0;
  if (geomtype == POINTTYPE)
  {
    points = palloc0(sizeof(Npoint *));
    np = geom_npoint(gs);
    if (np)
      points[npoints++] = np;
  }
  else /* geomtype == LINETYPE */
  {
    int numpoints = line_numpoints(gs);
    points = palloc0(sizeof(Npoint *) * numpoints);
    for (int i = 0; i < numpoints; i++)
    {
      /* The composing points are from 1 to numcount */
      GSERIALIZED *point = line_point_n(gs, i + 1);
      np = geom_npoint(point);
      if (np)
        points[npoints++] = np;
      /* Cannot pfree(point); */
    }
  }

  if (npoints == 0)
  {
    pfree(points);
    return NULL;
  }
  int64 rid = points[0]->rid;
  double minPos, maxPos;
  minPos = maxPos = points[0]->pos;
  for (int i = 1; i < npoints; i++)
  {
    if (points[i]->rid != rid)
    {
      pfree_array((void **) points, npoints);
      return NULL;
    }
    minPos = Min(minPos, points[i]->pos);
    maxPos = Max(maxPos, points[i]->pos);
  }
  Nsegment *result = nsegment_make(rid, minPos, maxPos);
  pfree_array((void **) points, npoints);
  return result;
}

/*****************************************************************************
 * SRID functions
 *****************************************************************************/

/**
 * @ingroup meos_temporal_accessor
 * @brief Return the SRID of a network point
 * @param[in] np Network point
 * @csqlfn #Npoint_srid()
 * @note Since it is assumed that all network points share the same SRID which
 * is the one from the @p ways table, for performance reasons we simply get
 * the SRID of the table
 */
int32_t
npoint_srid(const Npoint *np __attribute__((unused)))
{
  return get_srid_ways();
}

/**
 * @ingroup meos_temporal_accessor
 * @brief Return the SRID of a network segment
 * @param[in] ns Network segment
 * @csqlfn #Nsegment_srid()
 * @note Since it is assumed that all network points share the same SRID which
 * is the one from the @p ways table, for performance reasons we simply get
 * the SRID of the table
 */
int32_t
nsegment_srid(const Nsegment *ns __attribute__((unused)))
{
  return get_srid_ways();
}

/*****************************************************************************
 * Comparison functions for defining B-tree indexes
 *****************************************************************************/

/**
 * @ingroup meos_temporal_comp_trad
 * @brief Return true if the first network point is equal to the second one
 * @param[in] np1,np2 Network points
 * @csqlfn #Npoint_eq()
 */
bool
npoint_eq(const Npoint *np1, const Npoint *np2)
{
  return np1->rid == np2->rid && fabs(np1->pos - np2->pos) < MEOS_EPSILON;
}

/**
 * @ingroup meos_temporal_comp_trad
 * @brief Return true if the first network point is not equal to the second one
 * @param[in] np1,np2 Network points
 * @csqlfn #Npoint_ne()
 */
bool
npoint_ne(const Npoint *np1, const Npoint *np2)
{
  return (!npoint_eq(np1, np2));
}

/**
 * @ingroup meos_temporal_comp_trad
 * @brief Return -1, 0, or 1 depending on whether the first network point
 * is less than, equal to, or greater than the second one
 * @param[in] np1,np2 Network points
 * @csqlfn #Npoint_cmp()
 */
int
npoint_cmp(const Npoint *np1, const Npoint *np2)
{
  if (np1->rid < np2->rid)
    return -1;
  else if (np1->rid > np2->rid)
    return 1;
  /* Both rid are equal */
  else if(np1->pos < np2->pos)
    return -1;
  else if (np1->pos > np2->pos)
    return 1;
  return 0;
}

/**
 * @ingroup meos_temporal_comp_trad
 * @brief Return true if the first network point is less than the second one
 * @param[in] np1,np2 Network points
 * @csqlfn #Npoint_lt()
 */
bool
npoint_lt(const Npoint *np1, const Npoint *np2)
{
  int cmp = npoint_cmp(np1, np2);
  return (cmp < 0);
}

/**
 * @ingroup meos_temporal_comp_trad
 * @brief Return true if the first network point is less than or equal to the
 * second one
 * @param[in] np1,np2 Network points
 * @csqlfn #Npoint_le()
 */
bool
npoint_le(const Npoint *np1, const Npoint *np2)
{
  int cmp = npoint_cmp(np1, np2);
  return (cmp <= 0);
}

/**
 * @ingroup meos_temporal_comp_trad
 * @brief Return true if the first network point is greater than the second one
 * @param[in] np1,np2 Network points
 * @csqlfn #Npoint_gt()
 */
bool
npoint_gt(const Npoint *np1, const Npoint *np2)
{
  int cmp = npoint_cmp(np1, np2);
  return (cmp > 0);
}

/**
 * @ingroup meos_temporal_comp_trad
 * @brief Return true if the first network point is greater than or equal to
 * the second one
 * @param[in] np1,np2 Network points
 * @csqlfn #Npoint_ge()
 */
bool
npoint_ge(const Npoint *np1, const Npoint *np2)
{
  int cmp = npoint_cmp(np1, np2);
  return (cmp >= 0);
}

/*****************************************************************************/

/**
 * @ingroup meos_temporal_comp_trad
 * @brief Return true if the first network segment is equal to the second one
 * @param[in] ns1,ns2 Network segments
 * @csqlfn #Nsegment_eq()
 */
bool
nsegment_eq(const Nsegment *ns1, const Nsegment *ns2)
{
  return ns1->rid == ns2->rid && fabs(ns1->pos1 - ns2->pos1) < MEOS_EPSILON &&
    fabs(ns1->pos2 - ns2->pos2) < MEOS_EPSILON;
}

/**
 * @ingroup meos_temporal_comp_trad
 * @brief Return true if the first network segment is not equal to the second
 * one
 * @param[in] ns1,ns2 Network segments
 * @csqlfn #Nsegment_ne()
 */
bool
nsegment_ne(const Nsegment *ns1, const Nsegment *ns2)
{
  return (! nsegment_eq(ns1, ns2));
}

/**
 * @ingroup meos_temporal_comp_trad
 * @brief Return -1, 0, or 1 depending on whether the first network segment
 * is less than, equal to, or greater than the second one
 * @param[in] ns1,ns2 Network segments
 * @csqlfn #Nsegment_cmp()
 */
int
nsegment_cmp(const Nsegment *ns1, const Nsegment *ns2)
{
  if (ns1->rid < ns2->rid)
    return -1;
  else if (ns1->rid > ns2->rid)
    return 1;
  /* Both rid are equal */
  else if(ns1->pos1 < ns2->pos1)
    return -1;
  else if (ns1->pos1 > ns2->pos1)
    return 1;
  /* Both pos1 are equal */
  else if(ns1->pos2 < ns2->pos2)
    return -1;
  else if (ns1->pos2 > ns2->pos2)
    return 1;
  return 0;
}

/**
 * @ingroup meos_temporal_comp_trad
 * @brief Return true if the first network segment is less than the second one
 * @param[in] ns1,ns2 Network segments
 * @csqlfn #Nsegment_lt()
 */
bool
nsegment_lt(const Nsegment *ns1, const Nsegment *ns2)
{
  int cmp = nsegment_cmp(ns1, ns2);
  return (cmp < 0);
}

/**
 * @ingroup meos_temporal_comp_trad
 * @brief Return true if the first network segment is less than or equal to the
 * second one
 * @param[in] ns1,ns2 Network segments
 * @csqlfn #Nsegment_le()
 */
bool
nsegment_le(const Nsegment *ns1, const Nsegment *ns2)
{
  int cmp = nsegment_cmp(ns1, ns2);
  return (cmp <= 0);
}

/**
 * @ingroup meos_temporal_comp_trad
 * @brief Return true if the first network segment is greater than the second
 * one
 * @param[in] ns1,ns2 Network segments
 * @csqlfn #Nsegment_gt()
 */
bool
nsegment_gt(const Nsegment *ns1, const Nsegment *ns2)
{
  int cmp = nsegment_cmp(ns1, ns2);
  return (cmp > 0);
}

/**
 * @ingroup meos_temporal_comp_trad
 * @brief Return true if the first network segment is greater than or equal to
 * the second one
 * @param[in] ns1,ns2 Network segments
 * @csqlfn #Nsegment_ge()
 */
bool
nsegment_ge(const Nsegment *ns1, const Nsegment *ns2)
{
  int cmp = nsegment_cmp(ns1, ns2);
  return (cmp >= 0);
}

/*****************************************************************************
 * Function for defining hash index
 * The function reuses the approach for span types for combining the hash of
 * the lower and upper bounds.
 *****************************************************************************/

/**
 * @ingroup meos_temporal_accessor
 * @brief Return the 32-bit hash value of a network point
 * @param[in] np Network point
 */
uint32
npoint_hash(const Npoint *np)
{
  /* Compute hashes of value and position */
  uint32 rid_hash = pg_hashint8(np->rid);
  uint32 pos_hash = pg_hashfloat8(np->pos);

  /* Merge hashes of value and position */
  uint32 result = rid_hash;
  result = (result << 1) | (result >> 31);
  result ^= pos_hash;
  return result;
}

/**
 * @ingroup meos_temporal_accessor
 * @brief Return the 32-bit hash value of a network point
 * @param[in] np Network point
 * @param[in] seed Seed
 */
uint64
npoint_hash_extended(const Npoint *np, uint64 seed)
{
  /* Compute hashes of value and position */
  uint64 rid_hash = pg_hashint8extended(np->rid, seed);
  uint64 pos_hash = pg_hashfloat8extended(np->pos, seed);

  /* Merge hashes of value and position */
  uint64 result = rid_hash;
  result = (result << 1) | (result >> 31);
  result ^= pos_hash;
  return result;
}

/*****************************************************************************/
