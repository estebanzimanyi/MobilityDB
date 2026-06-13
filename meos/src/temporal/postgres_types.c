/*****************************************************************************
 *
 * This MobilityDB code is provided under The PostgreSQL License.
 * Copyright (c) 2016-2026, Université libre de Bruxelles and MobilityDB
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
 * @brief Functions for base and time types corresponding to external
 * PostgreSQL functions in order to bypass the function manager @p fmgr.c.
 */

/* C */
#include <float.h>
#include <math.h>
#include <limits.h>
/* PostgreSQL */
#include <postgres.h>
#include <common/hashfn.h>
#include <common/int.h>
#include <common/int128.h>
#include <utils/datetime.h>
#include <utils/float.h>
#include "utils/formatting.h"
#include "utils/timestamp.h"
#if POSTGRESQL_VERSION_NUMBER >= 160000
  #include "varatt.h"
#endif
#include "utils/varlena.h"
/* PostGIS */
#include <liblwgeom_internal.h> /* for OUT_DOUBLE_BUFFER_SIZE */
/* MEOS */
#include <meos.h>
#include <meos_geo.h>
#include <meos_internal.h>
#include "temporal/temporal.h"

#include <utils/jsonb.h>
#include <utils/numeric.h>
#include <pgtypes.h>

#if ! MEOS
  extern Datum call_function1(PGFunction func, Datum arg1);
  extern Datum call_function3(PGFunction func, Datum arg1, Datum arg2, Datum arg3);
  extern Datum date_in(PG_FUNCTION_ARGS);
  extern Datum timestamp_in(PG_FUNCTION_ARGS);
  extern Datum timestamptz_in(PG_FUNCTION_ARGS);
  extern Datum date_out(PG_FUNCTION_ARGS);
  extern Datum timestamp_out(PG_FUNCTION_ARGS);
  extern Datum timestamptz_out(PG_FUNCTION_ARGS);
  extern Datum interval_out(PG_FUNCTION_ARGS);
#endif /* ! MEOS */

#if POSTGRESQL_VERSION_NUMBER >= 150000 || MEOS
  extern int64 pg_strtoint64(const char *s);
#else
  extern bool scanint8(const char *str, bool errorOK, int64 *result);
#endif

/*****************************************************************************
 * Locale-independent numeric parsing
 *
 * MEOS pins all textual double parsing to the C locale so that '.' is
 * always the decimal separator regardless of the process LC_NUMERIC. We
 * use strtod_l with a process-lifetime "C" locale handle where available
 * (glibc/macOS/FreeBSD) and otherwise fall back to a save/switch/restore
 * dance around plain strtod. We do NOT call setlocale() at MEOS init time
 * because downstream consumers (PyMEOS, JMEOS, ...) can legitimately want
 * a non-C locale for their own code.
 *
 * Issue #425 — see meos.h for the documented locale contract.
 *****************************************************************************/

#if defined(LC_NUMERIC_MASK) && defined(__GLIBC__)
  #define MEOS_HAVE_STRTOD_L 1
#elif defined(LC_NUMERIC_MASK) && (defined(__APPLE__) || defined(__FreeBSD__))
  #define MEOS_HAVE_STRTOD_L 1
#endif

#if MEOS_HAVE_STRTOD_L
/* Explicit forward declarations: strtod_l/newlocale require POSIX 2008
 * feature test macros to appear in <stdlib.h>/<locale.h>, and the MEOS
 * build doesn't set those globally (the postgres headers manage their
 * own feature macros). Without a visible prototype the compiler treats
 * strtod_l as returning int, which silently truncates the double — see
 * #425 for the bug story. */
extern double strtod_l(const char *str, char **endptr, locale_t loc);
extern float strtof_l(const char *str, char **endptr, locale_t loc);
extern locale_t newlocale(int category_mask, const char *locale,
  locale_t base);
/* Note: freelocale's return type differs across platforms (POSIX/macOS
 * declare it as int, glibc < 2.32 as void), so we don't forward-declare
 * it. We never call it anyway: the C-locale handle lives for the
 * process lifetime. */
static locale_t meos_c_locale = (locale_t) 0;
#endif

/**
 * @brief Parse a double from a string using the C locale (decimal point
 * '.'), independent of the process LC_NUMERIC setting
 */
double
meos_strtod(const char *str, char **endptr)
{
#if MEOS_HAVE_STRTOD_L
  if (meos_c_locale == (locale_t) 0)
    meos_c_locale = newlocale(LC_NUMERIC_MASK, "C", (locale_t) 0);
  if (meos_c_locale != (locale_t) 0)
    return strtod_l(str, endptr, meos_c_locale);
#endif
  /* Portable fallback: save the current LC_NUMERIC, switch to "C" for
   * the parse, restore it. Not thread-safe (other threads could see the
   * "C" value during the parse), but always correct.
   *
   * Skip the dance if we are already in a C-equivalent locale, which
   * avoids two strdup/free pairs on the hot path. */
  const char *cur = setlocale(LC_NUMERIC, NULL);
  if (cur == NULL || strcmp(cur, "C") == 0 || strcmp(cur, "POSIX") == 0)
    return strtod(str, endptr);
  char *saved = strdup(cur);
  setlocale(LC_NUMERIC, "C");
  double val = strtod(str, endptr);
  if (saved != NULL)
  {
    setlocale(LC_NUMERIC, saved);
    free(saved);
  }
  return val;
}

/**
 * @brief Parse a float from a string using the C locale (decimal point '.'),
 * independent of the process LC_NUMERIC setting. The float4 analogue of
 * meos_strtod; using strtof (not strtod+cast) preserves the single-rounding
 * behaviour pg_float4in_internal relies on.
 */
float
meos_strtof(const char *str, char **endptr)
{
#if MEOS_HAVE_STRTOD_L
  if (meos_c_locale == (locale_t) 0)
    meos_c_locale = newlocale(LC_NUMERIC_MASK, "C", (locale_t) 0);
  if (meos_c_locale != (locale_t) 0)
    return strtof_l(str, endptr, meos_c_locale);
#endif
  const char *cur = setlocale(LC_NUMERIC, NULL);
  if (cur == NULL || strcmp(cur, "C") == 0 || strcmp(cur, "POSIX") == 0)
    return strtof(str, endptr);
  char *saved = strdup(cur);
  setlocale(LC_NUMERIC, "C");
  float val = strtof(str, endptr);
  if (saved != NULL)
  {
    setlocale(LC_NUMERIC, saved);
    free(saved);
  }
  return val;
}

/*****************************************************************************
 * Text and binary string functions
 *****************************************************************************/

/**
 * @brief Convert a C binary string into a bytea
 */
bytea *
bstring2bytea(const uint8_t *wkb, size_t size)
{
  bytea *result = palloc(size + VARHDRSZ);
  memcpy(VARDATA(result), wkb, size);
  SET_VARSIZE(result, size + VARHDRSZ);
  return result;
}

/**
 * @ingroup meos_base_text
 * @brief Return the concatenation of the two text values
 * @param[in] txt1,txt2 Text values
 * @note Function adapted from the external function @p text_catenate in file
 * @p varlena.c
 */
text *
textcat_text_text(const text *txt1, const text *txt2)
{
  size_t len1 = VARSIZE_ANY_EXHDR(txt1);
  size_t len2 = VARSIZE_ANY_EXHDR(txt2);
  size_t len = len1 + len2 + VARHDRSZ;
  text *result = palloc(len);

  /* Set size of result string... */
  SET_VARSIZE(result, len);

  /* Fill data field of result string... */
  char *ptr = VARDATA(result);
  if (len1 > 0)
    memcpy(ptr, VARDATA_ANY(txt1), len1);
  if (len2 > 0)
    memcpy(ptr + len1, VARDATA_ANY(txt2), len2);

  return result;
}

/**
 * @brief Return the concatenation of the two text values
 */
Datum
datum_textcat(Datum l, Datum r)
{
  return PointerGetDatum(textcat_text_text(DatumGetTextP(l), DatumGetTextP(r)));
}

/**
 * @ingroup meos_base_text
 * @brief Return the text value transformed to lowercase
 * @param[in] txt Text value
 * @note PostgreSQL function: @p lower() in file @p varlena.c.
 * Notice that @p DEFAULT_COLLATION_OID is used instead of
 * @p PG_GET_COLLATION()
 */
text *
text_lower(const text *txt)
{
#if MEOS
  VALIDATE_NOT_NULL(txt, NULL);
  char *out_string = asc_tolower(VARDATA_ANY(txt), VARSIZE_ANY_EXHDR(txt));
#else /* ! MEOS */
  char *out_string = str_tolower(VARDATA_ANY(txt), VARSIZE_ANY_EXHDR(txt),
    DEFAULT_COLLATION_OID);
#endif /* MEOS */
  text *result = cstring_to_text(out_string);
  pfree(out_string);
  return result;
}

/**
 * @brief Return the text value transformed to lowercase
 */
Datum
datum_lower(Datum value)
{
  return PointerGetDatum(text_lower(DatumGetTextP(value)));
}

/**
 * @ingroup meos_base_text
 * @brief Return the text value transformed to uppercase
 * @param[in] txt Text value
 * @note PostgreSQL function: @p upper() in file @p varlena.c.
 * Notice that @p DEFAULT_COLLATION_OID is used instead of
 * @p PG_GET_COLLATION()
 */
text *
text_upper(const text *txt)
{
#if MEOS
  VALIDATE_NOT_NULL(txt, NULL);
  char *out_string = asc_toupper(VARDATA_ANY(txt), VARSIZE_ANY_EXHDR(txt));
#else /* ! MEOS */
  char *out_string = str_toupper(VARDATA_ANY(txt), VARSIZE_ANY_EXHDR(txt),
    DEFAULT_COLLATION_OID);
#endif /* MEOS */
  text *result = cstring_to_text(out_string);
  pfree(out_string);
  return result;
}

/**
 * @brief Return the text value transformed to uppercase
 */
Datum
datum_upper(Datum value)
{
  return PointerGetDatum(text_upper(DatumGetTextP(value)));
}

/**
 * @ingroup meos_base_text
 * @brief Convert the text value to initcap
 * @param[in] txt Text value
 * @note PostgreSQL function: @p initcap() in file @p varlena.c.
 * Notice that @p DEFAULT_COLLATION_OID is used instead of
 * @p PG_GET_COLLATION()
 */
text *
text_initcap(const text *txt)
{
#if MEOS
  VALIDATE_NOT_NULL(txt, NULL);
  char *out_string = asc_initcap(VARDATA_ANY(txt), VARSIZE_ANY_EXHDR(txt));
#else /* ! MEOS */
  char *out_string = str_initcap(VARDATA_ANY(txt), VARSIZE_ANY_EXHDR(txt),
    DEFAULT_COLLATION_OID);
#endif /* MEOS */
  text *result = cstring_to_text(out_string);
  pfree(out_string);
  return result;
}

/**
 * @brief Convert the text value to uppercase
 */
Datum
datum_initcap(Datum value)
{
  return PointerGetDatum(text_initcap(DatumGetTextP(value)));
}

/*****************************************************************************/


/**
 * @ingroup meos_base_types
 * @brief Return the multiplication of an interval and a factor
 * @param[in] interv Interval
 * @param[in] factor Factor
 * @note PostgreSQL function: @p interval_mul(PG_FUNCTION_ARGS) taken from
 * PG version 17.2
 */
Interval *
mul_interval_double(const Interval *interv, double factor)
{
  /* Ensure the validity of the arguments */
  VALIDATE_NOT_NULL(interv, NULL);

  double month_remainder_days, sec_remainder, result_double;
  int32 orig_month = interv->month,
    orig_day = interv->day;
  Interval *result;

  result = palloc(sizeof(Interval));

  result_double = interv->month * factor;
  if (isnan(result_double) ||
    result_double > INT_MAX || result_double < INT_MIN)
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "interval out of range");
    return NULL;
  }
  result->month = (int32) result_double;

  result_double = interv->day * factor;
  if (isnan(result_double) ||
    result_double > INT_MAX || result_double < INT_MIN)
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "interval out of range");
    return NULL;
  }
  result->day = (int32) result_double;

  /*
   * The above correctly handles the whole-number part of the month and day
   * products, but we have to do something with any fractional part
   * resulting when the factor is non-integral.  We cascade the fractions
   * down to lower units using the conversion factors DAYS_PER_MONTH and
   * SECS_PER_DAY.  Note we do NOT cascade up, since we are not forced to do
   * so by the representation.  The user can choose to cascade up later,
   * using justify_hours and/or justify_days.
   */

  /*
   * Fractional months full days into days.
   *
   * Floating point calculation are inherently imprecise, so these
   * calculations are crafted to produce the most reliable result possible.
   * TSROUND() is needed to more accurately produce whole numbers where
   * appropriate.
   */
  month_remainder_days = (orig_month * factor - result->month) * DAYS_PER_MONTH;
  month_remainder_days = TSROUND(month_remainder_days);
  sec_remainder = (orig_day * factor - result->day +
           month_remainder_days - (int) month_remainder_days) * SECS_PER_DAY;
  sec_remainder = TSROUND(sec_remainder);

  /*
   * Might have 24:00:00 hours due to rounding, or >24 hours because of time
   * cascade from months and days.  It might still be >24 if the combination
   * of cascade and the seconds factor operation itself.
   */
  if (fabs(sec_remainder) >= SECS_PER_DAY)
  {
    result->day += (int) (sec_remainder / SECS_PER_DAY);
    sec_remainder -= (int) (sec_remainder / SECS_PER_DAY) * SECS_PER_DAY;
  }

  /* cascade units down */
  result->day += (int32) month_remainder_days;
  result_double = rint(interv->time * factor + sec_remainder * USECS_PER_SEC);
  if (isnan(result_double) || !FLOAT8_FITS_IN_INT64(result_double))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "interval out of range");
    return NULL;
  }
  result->time = (int64) result_double;

  return result;
}
