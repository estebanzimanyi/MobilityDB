/*-------------------------------------------------------------------------
 *
 * date.c
 *    implements DATE and TIME data types specified in SQL standard
 *
 * Portions Copyright (c) 1996-2025, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994-5, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *    src/backend/utils/adt/date.c
 *
 *-------------------------------------------------------------------------
 */


#include <ctype.h>
#include <limits.h>
#include <float.h>
#include <math.h>
#include <time.h>

#include "postgres.h"
#include "postgres_types.h"
#include "miscadmin.h"
#include "catalog/pg_type.h"
#include "common/hashfn.h"
#include "common/int.h"
#include "parser/scansup.h"
#include "utils/builtins.h"
#include "utils/date.h"
#include "utils/datetime.h"
#include "utils/float.h"
#include "utils/numeric.h"

extern Numeric int64_div_fast_to_numeric(int64 val1, int log10val2);

// #include "access/xact.h"
// #include "catalog/pg_type.h"
// #include "common/hashfn.h"
// #include "common/int.h"
// #include "libpq/pqformat.h"
// #include "miscadmin.h"
// #include "nodes/supportnodes.h"
// #include "parser/scansup.h"
// #include "utils/array.h"
// #include "utils/builtins.h"
// #include "utils/date.h"
// #include "utils/datetime.h"
// #include "utils/numeric.h"
// #include "utils/skipsupport.h"
// #include "utils/sortsupport.h"

/*
 * gcc's -ffast-math switch breaks routines that expect exact results from
 * expressions like timeval / SECS_PER_HOUR, where timeval is double.
 */
#ifdef __FAST_MATH__
#error -ffast-math is known to break this code
#endif

/* Check the typmod of a time value */
int32
anytime_typmod_check(bool istz, int32 typmod)
{
  if (typmod < 0)
  {
    elog(ERROR, "TIME(%d)%s precision must not be negative",
      typmod, (istz ? " WITH TIME ZONE" : ""));
    return INT_MAX;
  }
  if (typmod > MAX_TIME_PRECISION)
  {
    elog(WARNING, "TIME(%d)%s precision reduced to maximum allowed, %d",
      typmod, (istz ? " WITH TIME ZONE" : ""));
    typmod = MAX_TIME_PRECISION;
  }

  return typmod;
}

/* common code for timetypmodout and timetztypmodout */
static char *
anytime_typmodout(bool istz, int32 typmod)
{
  const char *tz = istz ? " with time zone" : " without time zone";

  if (typmod >= 0)
    return psprintf("(%d)%s", (int) typmod, tz);
  else
    return pstrdup(tz);
}

/*****************************************************************************
 *   Date ADT
 *****************************************************************************/

/**
 * @ingroup meos_base_date
 * @brief Return a date from a string
 * @return On error return INT_MAX
 * @note Derived from PostgreSQL function @p date_in()
 */
#if MEOS
DateADT
date_in(const char *str)
{
  return pg_date_in(str);
}
#endif
DateADT
pg_date_in(const char *str)
{
  DateADT date;
  fsec_t    fsec;
  struct pg_tm tt,
         *tm = &tt;
  int      tzp;
  int      dtype;
  int      nf;
  int      dterr;
  char     *field[MAXDATEFIELDS];
  int      ftype[MAXDATEFIELDS];
  char    workbuf[MAXDATELEN + 1];
  DateTimeErrorExtra extra;

  dterr = ParseDateTime((char *) str, workbuf, sizeof(workbuf),
              field, ftype, MAXDATEFIELDS, &nf);
  if (dterr == 0)
    dterr = DecodeDateTime(field, ftype, nf,
                 &dtype, tm, &fsec, &tzp, &extra);
  if (dterr != 0)
  {
    DateTimeParseError(dterr, &extra, (char *) str, "date", NULL);
    return INT_MAX;
  }

  switch (dtype)
  {
    case DTK_DATE:
      break;

    case DTK_EPOCH:
      GetEpochTime(tm);
      break;

    case DTK_LATE:
      DATE_NOEND(date);
      return (date);

    case DTK_EARLY:
      DATE_NOBEGIN(date);
      return (date);

    default:
      DateTimeParseError(DTERR_BAD_FORMAT, &extra, (char *) str, "date", NULL);
      return INT_MAX;
  }

  /* Prevent overflow in Julian-day routines */
  if (!IS_VALID_JULIAN(tm->tm_year, tm->tm_mon, tm->tm_mday))
  {
    elog(ERROR, "date out of range: \"%s\"", (char *) str);
    return INT_MAX;
  }

  date = date2j(tm->tm_year, tm->tm_mon, tm->tm_mday) - POSTGRES_EPOCH_JDATE;

  /* Now check for just-out-of-range dates */
  if (!IS_VALID_DATE(date))
  {
    elog(ERROR, "date out of range: \"%s\"", (char *) str);
    return INT_MAX;
  }

  return date;
}

/**
 * @ingroup meos_base_date
 * @brief Return the string representation of a date
 * @return On error return INT_MAX
 * @note Derived from PostgreSQL function @p date_out()
 */
#if MEOS
char *
date_out(DateADT date)
{
  return pg_date_out(date);
}
#endif
char *
pg_date_out(DateADT date)
{
  char     *result;
  struct pg_tm tt, *tm = &tt;
  char    buf[MAXDATELEN + 1];

  if (DATE_NOT_FINITE(date))
    EncodeSpecialDate(date, buf);
  else
  {
    j2date(date + POSTGRES_EPOCH_JDATE,
         &(tm->tm_year), &(tm->tm_mon), &(tm->tm_mday));
    EncodeDateOnly(tm, DateStyle, buf);
  }

  result = pstrdup(buf);
  return result;
}

/**
 * @ingroup meos_base_date
 * @brief Return a date from a year, a month, and a day
 * @note Derived from PostgreSQL function @p date_make()
 */
#if MEOS
DateADT
date_make(int year, int mon, int mday)
{
  return pg_date_make(year, mon, mday);
}
#endif
DateADT
pg_date_make(int year, int mon, int mday)
{
  struct pg_tm tm;
  DateADT date;
  int      dterr;
  bool    bc = false;

  tm.tm_year = year;
  tm.tm_mon = mon;
  tm.tm_mday = mday;

  /* Handle negative years as BC */
  if (tm.tm_year < 0)
  {
    int      year = tm.tm_year;

    bc = true;
    if (pg_neg_s32_overflow(year, &year))
    {
      elog(ERROR, "date field value out of range: %d-%02d-%02d",
        tm.tm_year, tm.tm_mon, tm.tm_mday);
      return INT_MAX;
    }
    tm.tm_year = year;
  }

  dterr = ValidateDate(DTK_DATE_M, false, false, bc, &tm);

  if (dterr != 0)
  {
    elog(ERROR, "date field value out of range: %d-%02d-%02d",
      tm.tm_year, tm.tm_mon, tm.tm_mday);
    return INT_MAX;
  }

  /* Prevent overflow in Julian-day routines */
  if (!IS_VALID_JULIAN(tm.tm_year, tm.tm_mon, tm.tm_mday))
  {
    elog(ERROR, "date out of range: %d-%02d-%02d",
      tm.tm_year, tm.tm_mon, tm.tm_mday);
    return INT_MAX;
  }

  date = date2j(tm.tm_year, tm.tm_mon, tm.tm_mday) - POSTGRES_EPOCH_JDATE;

  /* Now check for just-out-of-range dates */
  if (!IS_VALID_DATE(date))
  {
    elog(ERROR, "date out of range: %d-%02d-%02d",
        tm.tm_year, tm.tm_mon, tm.tm_mday);
    return INT_MAX;
  }

  return date;
}

/*
 * Convert reserved dates to string.
 */
void
EncodeSpecialDate(DateADT dt, char *str)
{
  if (DATE_IS_NOBEGIN(dt))
    strcpy(str, EARLY);
  else if (DATE_IS_NOEND(dt))
    strcpy(str, LATE);
  else            /* shouldn't happen */
    elog(ERROR, "invalid argument for EncodeSpecialDate");
  return;
}

/**
 * @ingroup meos_base_date
 * @brief Return true if two dates are equal
 * @note Derived from PostgreSQL function @p date_eq()
 */
bool
eq_date_date(DateADT date1, DateADT date2)
{
  return (date1 == date2);
}

/**
 * @ingroup meos_base_date
 * @brief Return true if two dates are not equal
 * @note Derived from PostgreSQL function @p date_ne()
 */
bool
ne_date_date(DateADT date1, DateADT date2)
{
  return (date1 != date2);
}

/**
 * @ingroup meos_base_date
 * @brief Return true if the first date is less than the second cone
 * @note Derived from PostgreSQL function @p date_lt()
 */
bool
lt_date_date(DateADT date1, DateADT date2)
{
  return (date1 < date2);
}

/**
 * @ingroup meos_base_date
 * @brief Return true if the first date is less than or equal to second one
 * @note Derived from PostgreSQL function @p date_le()
 */
bool
le_date_date(DateADT date1, DateADT date2)
{
  return (date1 <= date2);
}

/**
 * @ingroup meos_base_date
 * @brief Return true if the first date is greater than the second one
 * @note Derived from PostgreSQL function @p date_gt()
 */
bool
gt_date_date(DateADT date1, DateADT date2)
{
  return (date1 > date2);
}

/**
 * @ingroup meos_base_date
 * @brief Return true if the first date is greater than or equal to second one
 * @note Derived from PostgreSQL function @p date_ge()
 */
bool
ge_date_date(DateADT date1, DateADT date2)
{
  return (date1 >= date2);
}

/**
 * @ingroup meos_base_date
 * @brief Return -1, 0, or 1 depending on whether the first date is less than,
 * equal to, or less than the second one
 * @note Derived from PostgreSQL function @p date_cmp()
 */
int
cmp_date_date(DateADT date1, DateADT date2)
{
  if (date1 < date2)
    return -1;
  else if (date1 > date2)
    return 1;
  return 0;
}

/**
 * @ingroup meos_base_date
 * @brief Return the 32-bit hash value of a span set
 * @note Derived from PostgreSQL function @p hashdate()
 */
uint32
date_hash(DateADT date)
{
  return hash_uint32(date);
}

/**
 * @ingroup meos_base_date
 * @brief Return the 64-bit hash value of a span set using a seed
 * @note Derived from PostgreSQL function @p hashdateextended()
 */
uint64
date_hash_extended(DateADT date, int64 seed)
{
  return hash_uint32_extended(date, seed);
}

/**
 * @ingroup meos_base_date
 * @brief Return true if a date is finite
 * @note Derived from PostgreSQL function @p date_finite()
 */
bool 
date_is_finite(DateADT date)
{
  return ! DATE_NOT_FINITE(date);
}

/**
 * @ingroup meos_base_date
 * @brief Return the larger of two dates
 * @note Derived from PostgreSQL function @p date_larger()
 */
#if MEOS
DateADT
date_larger(DateADT date1, DateADT date2)
{
  pg_date_larger(date1, date2);
}
#endif 
DateADT
pg_date_larger(DateADT date1, DateADT date2)
{
  return ((date1 > date2) ? date1 : date2);
}

/**
 * @ingroup meos_base_date
 * @brief Return the smaller of two dates
 * @note Derived from PostgreSQL function @p date_smaller()
 */
#if MEOS
DateADT
date_smaller(DateADT date1, DateADT date2)
{
  pg_date_smaller(date1, date2);
}
#endif
DateADT
pg_date_smaller(DateADT date1, DateADT date2)
{
  return ((date1 < date2) ? date1 : date2);
}

/**
 * @ingroup meos_base_date
 * @brief Return the subtraction of two dates
 * @param[in] date1,date2 Dates
 * @note Derived from PostgreSQL function @p date_mi()
 */
Interval *
minus_date_date(DateADT date1, DateADT date2)
{
  if (DATE_NOT_FINITE(date1) || DATE_NOT_FINITE(date2))
  {
    elog(ERROR, "cannot subtract infinite dates");
    return NULL;
  }
  return ((int32) (date1 - date2));
}

/**
 * @ingroup meos_base_date
 * @brief Add a number of days to a date, giving a new date
 * @details Must handle both positive and negative numbers of days
 * @note Derived from PostgreSQL function @p date_pli()
 */
DateADT
add_date_int(DateADT date, int32 days)
{
  if (DATE_NOT_FINITE(date))
    return date; /* can't change infinity */

  DateADT result = date + days;

  /* Check for integer overflow and out-of-allowed-range */
  if ((days >= 0 ? (result < date) : (result > date)) ||
    !IS_VALID_DATE(result))
  {
    elog(ERROR, "date out of range");
    return DATEVAL_NOEND;
  }

  return result;
}

/**
 * @ingroup meos_base_date
 * @brief Subtract a number of days from a date, giving a new date
 * @note Derived from PostgreSQL function @p date_mii()
 */
DateADT
minus_date_int(DateADT date, int32 days)
{
  if (DATE_NOT_FINITE(date))
    return date; /* can't change infinity */

  DateADT result = date - days;

  /* Check for integer overflow and out-of-allowed-range */
  if ((days >= 0 ? (result > date) : (result < date)) ||
    !IS_VALID_DATE(result))
  {
    elog(ERROR, "date out of range");
    return DATEVAL_NOEND;
  }

  return result;
}

/*
 * Promote date to timestamp.
 *
 * On successful conversion, *overflow is set to zero if it's not NULL.
 *
 * If the date is finite but out of the valid range for timestamp, then:
 * if overflow is NULL, we throw an out-of-range error.
 * if overflow is not NULL, we store +1 or -1 there to indicate the sign
 * of the overflow, and return the appropriate timestamp infinity.
 *
 * Note: *overflow = -1 is actually not possible currently, since both
 * datatypes have the same lower bound, Julian day zero.
 */
Timestamp
date2timestamp_opt_overflow(DateADT date, int *overflow)
{
  Timestamp  result;

  if (overflow)
    *overflow = 0;

  if (DATE_IS_NOBEGIN(date))
    return DT_NOBEGIN;
  else if (DATE_IS_NOEND(date))
    return DT_NOEND;
  else
  {
    /*
     * Since dates have the same minimum values as timestamps, only upper
     * boundary need be checked for overflow.
     */
    if (date >= (TIMESTAMP_END_JULIAN - POSTGRES_EPOCH_JDATE))
    {
      if (overflow)
      {
        *overflow = 1;
        return DT_NOEND;
      }
      else
      {
        elog(ERROR, "date out of range for timestamp");
        return DT_NOEND;
      }
    }

    /* date is days since 2000, timestamp is microseconds since same... */
    result = date * USECS_PER_DAY;
  }

  return result;
}

/*
 * Promote date to timestamp, throwing error for overflow.
 */
static TimestampTz
date2timestamp(DateADT date)
{
  return date2timestamp_opt_overflow(date, NULL);
}

/*
 * Promote date to timestamp with time zone.
 *
 * On successful conversion, *overflow is set to zero if it's not NULL.
 *
 * If the date is finite but out of the valid range for timestamptz, then:
 * if overflow is NULL, we throw an out-of-range error.
 * if overflow is not NULL, we store +1 or -1 there to indicate the sign
 * of the overflow, and return the appropriate timestamptz infinity.
 */
TimestampTz
date2timestamptz_opt_overflow(DateADT date, int *overflow)
{
  TimestampTz result;
  struct pg_tm tt,
         *tm = &tt;
  int      tz;

  if (overflow)
    *overflow = 0;

  if (DATE_IS_NOBEGIN(date))
    return DT_NOBEGIN;
  else if (DATE_IS_NOEND(date))
    return DT_NOEND;
  else
  {
    /*
     * Since dates have the same minimum values as timestamps, only upper
     * boundary need be checked for overflow.
     */
    if (date >= (TIMESTAMP_END_JULIAN - POSTGRES_EPOCH_JDATE))
    {
      if (overflow)
      {
        *overflow = 1;
        return DT_NOEND;
      }
      else
      {
        elog(ERROR, "date out of range for timestamp");
        return DT_NOEND;
      }
    }

    j2date(date + POSTGRES_EPOCH_JDATE,
         &(tm->tm_year), &(tm->tm_mon), &(tm->tm_mday));
    tm->tm_hour = 0;
    tm->tm_min = 0;
    tm->tm_sec = 0;
    tz = DetermineTimeZoneOffset(tm, session_timezone);

    result = date * USECS_PER_DAY + tz * USECS_PER_SEC;

    /*
     * Since it is possible to go beyond allowed timestamptz range because
     * of time zone, check for allowed timestamp range after adding tz.
     */
    if (!IS_VALID_TIMESTAMP(result))
    {
      if (overflow)
      {
        if (result < MIN_TIMESTAMP)
        {
          *overflow = -1;
          TIMESTAMP_NOBEGIN(result);
        }
        else
        {
          *overflow = 1;
          TIMESTAMP_NOEND(result);
        }
      }
      else
      {
        elog(ERROR, "date out of range for timestamp");
        return DT_NOEND;
      }
    }
  }

  return result;
}

/*
 * Promote date to timestamptz, throwing error for overflow.
 */
static TimestampTz
date2timestamptz(DateADT date)
{
  return date2timestamptz_opt_overflow(date, NULL);
}

/*
 * date2timestamp_no_overflow
 *
 * This is chartered to produce a double value that is numerically
 * equivalent to the corresponding Timestamp value, if the date is in the
 * valid range of Timestamps, but in any case not throw an overflow error.
 * We can do this since the numerical range of double is greater than
 * that of non-erroneous timestamps.  The results are currently only
 * used for statistical estimation purposes.
 */
double
date2timestamp_no_overflow(DateADT date)
{
  double    result;

  if (DATE_IS_NOBEGIN(date))
    // result = -DBL_MAX; // TODO
    result = -get_float8_infinity();
  else if (DATE_IS_NOEND(date))
    // result = DBL_MAX;  // TODO
    result = get_float8_infinity();
  else
  {
    /* date is days since 2000, timestamp is microseconds since same... */
    result = date * (double) USECS_PER_DAY;
  }

  return result;
}


/*
 * Crosstype comparison functions for dates
 */

int32
date_cmp_timestamp_internal(DateADT date, Timestamp dt2)
{
  Timestamp  dt1;
  int      overflow;

  dt1 = date2timestamp_opt_overflow(date, &overflow);
  if (overflow > 0)
  {
    /* dt1 is larger than any finite timestamp, but less than infinity */
    return TIMESTAMP_IS_NOEND(dt2) ? -1 : +1;
  }
  Assert(overflow == 0);    /* -1 case cannot occur */

  return timestamp_cmp_internal(dt1, dt2);
}

/**
 * @ingroup meos_base_date
 * @brief Return true if a date and a timestamp are equal
 * @note Derived from PostgreSQL function @p date_eq_timestamp()
 */
bool
eq_date_timestamp(DateADT date, Timestamp dt2)
{
  return (date_cmp_timestamp_internal(date, dt2) == 0);
}

/**
 * @ingroup meos_base_date
 * @brief Return true if a date and a timestamp are not equal
 * @note Derived from PostgreSQL function @p date_ne_timestamp()
 */
bool
ne_date_timestamp(DateADT date, Timestamp dt2)
{
  return (date_cmp_timestamp_internal(date, dt2) != 0);
}

/**
 * @ingroup meos_base_date
 * @brief Return true if a date is less than a timestamp
 * @note Derived from PostgreSQL function @p date_lt_timestamp()
 */
bool
lt_date_timestamp(DateADT date, Timestamp dt2)
{
  return (date_cmp_timestamp_internal(date, dt2) < 0);
}

/**
 * @ingroup meos_base_date
 * @brief Return true if a date is greater than a timestamp
 * @note Derived from PostgreSQL function @p date_gt_timestamp()
 */
bool
gt_date_timestamp(DateADT date, Timestamp dt2)
{
  return (date_cmp_timestamp_internal(date, dt2) > 0);
}

/**
 * @ingroup meos_base_date
 * @brief Return true if a date is less than or equal to a timestamp
 * @note Derived from PostgreSQL function @p date_le_timestamp()
 */
bool
le_date_timestamp(DateADT date, Timestamp dt2)
{
  return (date_cmp_timestamp_internal(date, dt2) <= 0);
}

/**
 * @ingroup meos_base_date
 * @brief Return true if a date is greater than or equal to a timestamp
 * @note Derived from PostgreSQL function @p date_ge_timestamp()
 */
bool
ge_date_timestamp(DateADT date, Timestamp dt2)
{
  return (date_cmp_timestamp_internal(date, dt2) >= 0);
}

/**
 * @ingroup meos_base_date
 * @brief Return -1, 0, or 1 depending on whether a date is less than, equal
 * to, or greater than a timestamp
 * @note Derived from PostgreSQL function @p date_cmp_timestamp()
 */
int32
cmp_date_timestamp(DateADT date, Timestamp dt2)
{
  return date_cmp_timestamp_internal(date, dt2);
}

int32
date_cmp_timestamptz_internal(DateADT date, TimestampTz dt2)
{
  TimestampTz dt1;
  int      overflow;

  dt1 = date2timestamptz_opt_overflow(date, &overflow);
  if (overflow > 0)
  {
    /* dt1 is larger than any finite timestamp, but less than infinity */
    return TIMESTAMP_IS_NOEND(dt2) ? -1 : +1;
  }
  if (overflow < 0)
  {
    /* dt1 is less than any finite timestamp, but more than -infinity */
    return TIMESTAMP_IS_NOBEGIN(dt2) ? +1 : -1;
  }

  return timestamptz_cmp_internal(dt1, dt2);
}

/**
 * @ingroup meos_base_date
 * @brief Return true if a date is equal to a timestamptz
 * @note Derived from PostgreSQL function @p date_eq_timestamptz()
 */
bool
eq_date_timestamptz(DateADT date, TimestampTz dt2)
{
  return (date_cmp_timestamptz_internal(date, dt2) == 0);
}

/**
 * @ingroup meos_base_date
 * @brief Return true if a date is not equal to a timestamptz
 * @note Derived from PostgreSQL function @p date_ne_timestamptz()
 */
bool
ne_date_timestamptz(DateADT date, TimestampTz dt2)
{
  return (date_cmp_timestamptz_internal(date, dt2) != 0);
}

/**
 * @ingroup meos_base_date
 * @brief Return true if a date is less than a timestamptz
 * @note Derived from PostgreSQL function @p date_lt_timestamptz()
 */
bool
lt_date_timestamptz(DateADT date, TimestampTz dt2)
{
  return (date_cmp_timestamptz_internal(date, dt2) < 0);
}

/**
 * @ingroup meos_base_date
 * @brief Return true if a date is greater than a timestamptz
 * @note Derived from PostgreSQL function @p date_gt_timestamptz()
 */
bool
gt_date_timestamptz(DateADT date, TimestampTz dt2)
{
  return (date_cmp_timestamptz_internal(date, dt2) > 0);
}

/**
 * @ingroup meos_base_date
 * @brief Return true if a date is less than or equal to a timestamptz
 * @note Derived from PostgreSQL function @p date_le_timestamptz()
 */
bool
le_date_timestamptz(DateADT date, TimestampTz dt2)
{
  return (date_cmp_timestamptz_internal(date, dt2) <= 0);
}

/**
 * @ingroup meos_base_date
 * @brief Return true if a date is greater than or equal to a timestamptz
 * @note Derived from PostgreSQL function @p date_ge_timestamptz()
 */
bool
ge_date_timestamptz(DateADT date, TimestampTz dt2)
{
  return (date_cmp_timestamptz_internal(date, dt2) >= 0);
}

/**
 * @ingroup meos_base_date
 * @brief Return -1, 0, or 1 depending on whether a date is less than, equal
 * to, or greater than a timestamptz
 * @note Derived from PostgreSQL function @p date_cmp_timestamptz()
 */
int32
cmp_date_timestamptz(DateADT date, TimestampTz dt2)
{
  return (date_cmp_timestamptz_internal(date, dt2));
}

/**
 * @ingroup meos_base_date
 * @brief Return true if a timestamp is less than to a date
 * @note Derived from PostgreSQL function @p timestamp_eq_date()
 */
bool
eq_timestamp_date(Timestamp dt1, DateADT date)
{
  return (date_cmp_timestamp_internal(date, dt1) == 0);
}

/**
 * @ingroup meos_base_date
 * @brief Return true if a timestamp is not equal to a date
 * @note Derived from PostgreSQL function @p timestamp_ne_date()
 */
bool
ne_timestamp_date(Timestamp dt1, DateADT date)
{
  return (date_cmp_timestamp_internal(date, dt1) != 0);
}

/**
 * @ingroup meos_base_date
 * @brief Return true if a timestamp is less than a date
 * @note Derived from PostgreSQL function @p timestamp_lt_date()
 */
bool
lt_timestamp_date(Timestamp dt1, DateADT date)
{
  return (date_cmp_timestamp_internal(date, dt1) > 0);
}

/**
 * @ingroup meos_base_date
 * @brief Return true if a timestamp is greater than a date
 * @note Derived from PostgreSQL function @p timestamp_gt_date()
 */
bool
gt_timestamp_date(Timestamp dt1, DateADT date)
{
  return (date_cmp_timestamp_internal(date, dt1) < 0);
}

/**
 * @ingroup meos_base_date
 * @brief Return true if a timestamp is less than or equal to a date
 * @note Derived from PostgreSQL function @p timestamp_le_date()
 */
bool
le_timestamp_date(Timestamp dt1, DateADT date)
{
  return (date_cmp_timestamp_internal(date, dt1) >= 0);
}

/**
 * @ingroup meos_base_date
 * @brief Return true if a timestamp is greater than or equal to a date
 * @note Derived from PostgreSQL function @p timestamp_ge_date()
 */
bool
ge_timestamp_date(Timestamp dt1, DateADT date)
{
  return (date_cmp_timestamp_internal(date, dt1) <= 0);
}

/**
 * @ingroup meos_base_date
 * @brief Return -1, 0, or 1 depending on whether a timestamp is less than,
 * equal to, or greater than a date
 * @note Derived from PostgreSQL function @p timestamp_cmp_date()
 */
int32
cmp_timestamp_date(Timestamp dt1, DateADT date)
{
  return (-date_cmp_timestamp_internal(date, dt1));
}

/**
 * @ingroup meos_base_date
 * @brief Return true if a timestamptz is equal to a date
 * @note Derived from PostgreSQL function @p timestamptz_eq_date()
 */
bool
eq_timestamptz_date(TimestampTz dt1, DateADT date)
{
  return (date_cmp_timestamptz_internal(date, dt1) == 0);
}

/**
 * @ingroup meos_base_date
 * @brief Return true if a timestamptz is not equal to a date
 * @note Derived from PostgreSQL function @p timestamptz_ne_date()
 */
bool
ne_timestamptz_date(TimestampTz dt1, DateADT date)
{
  return (date_cmp_timestamptz_internal(date, dt1) != 0);
}

/**
 * @ingroup meos_base_date
 * @brief Return true if a timestamptz is less than a date
 * @note Derived from PostgreSQL function @p timestamptz_lt_date()
 */
bool
lt_timestamptz_date(TimestampTz dt1, DateADT date)
{
  return (date_cmp_timestamptz_internal(date, dt1) > 0);
}

/**
 * @ingroup meos_base_date
 * @brief Return true if a timestamptz is greater than a date
 * @note Derived from PostgreSQL function @p timestamptz_gt_date()
 */
bool
gt_timestamptz_date(TimestampTz dt1, DateADT date)
{
  return (date_cmp_timestamptz_internal(date, dt1) < 0);
}

/**
 * @ingroup meos_base_date
 * @brief Return true if a timestamptz is less than or equal to a date
 * @note Derived from PostgreSQL function @p timestamptz_le_date()
 */
bool
le_timestamptz_date(TimestampTz dt1, DateADT date)
{
  return (date_cmp_timestamptz_internal(date, dt1) >= 0);
}

/**
 * @ingroup meos_base_date
 * @brief Return true if a timestamptz is greater than or equal to a date
 * @note Derived from PostgreSQL function @p timestamptz_ge_date()
 */
bool
ge_timestamptz_date(TimestampTz dt1, DateADT date)
{
  return (date_cmp_timestamptz_internal(date, dt1) <= 0);
}

/**
 * @ingroup meos_base_date
 * @brief Return -1, 0, or 1 depending on whether a timestamptz is less than,
 * equal to, or greater than a date
 * @note Derived from PostgreSQL function @p timestamptz_cmp_date()
 */
int32
cmp_timestamptz_date(TimestampTz dt1, DateADT date)
{
  return (-date_cmp_timestamptz_internal(date, dt1));
}

/**
 * @ingroup meos_base_date
 * @brief Extract a field from a date
 * @note Derived from PostgreSQL function @p extract_date()
 */
Numeric
date_extract(DateADT date, text *units)
{
  int64 intresult;
  int type, val;
  int year, mon, mday;

  char *lowunits = downcase_truncate_identifier(VARDATA_ANY(units),
    VARSIZE_ANY_EXHDR(units), false);

  type = DecodeUnits(0, lowunits, &val);
  if (type == UNKNOWN_FIELD)
    type = DecodeSpecial(0, lowunits, &val);

  if (DATE_NOT_FINITE(date) && (type == UNITS || type == RESERV))
  {
    switch (val)
    {
        /* Oscillating units */
      case DTK_DAY:
      case DTK_MONTH:
      case DTK_QUARTER:
      case DTK_WEEK:
      case DTK_DOW:
      case DTK_ISODOW:
      case DTK_DOY:
        return NULL;

        /* Monotonically-increasing units */
      case DTK_YEAR:
      case DTK_DECADE:
      case DTK_CENTURY:
      case DTK_MILLENNIUM:
      case DTK_JULIAN:
      case DTK_ISOYEAR:
      case DTK_EPOCH:
        if (DATE_IS_NOBEGIN(date))
          return NumericGetDatum(pg_numeric_in("-Infinity", -1));
        else
          return NumericGetDatum(pg_numeric_in("Infinity", -1));
      default:
        elog(ERROR, "unit \"%s\" not supported for type %s",
          // lowunits, format_type_be(DATEOID));
          lowunits, "date");
        return NULL;
    }
  }
  else if (type == UNITS)
  {
    j2date(date + POSTGRES_EPOCH_JDATE, &year, &mon, &mday);

    switch (val)
    {
      case DTK_DAY:
        intresult = mday;
        break;

      case DTK_MONTH:
        intresult = mon;
        break;

      case DTK_QUARTER:
        intresult = (mon - 1) / 3 + 1;
        break;

      case DTK_WEEK:
        intresult = date2isoweek(year, mon, mday);
        break;

      case DTK_YEAR:
        if (year > 0)
          intresult = year;
        else
          /* there is no year 0, just 1 BC and 1 AD */
          intresult = year - 1;
        break;

      case DTK_DECADE:
        /* see comments in timestamp_part */
        if (year >= 0)
          intresult = year / 10;
        else
          intresult = -((8 - (year - 1)) / 10);
        break;

      case DTK_CENTURY:
        /* see comments in timestamp_part */
        if (year > 0)
          intresult = (year + 99) / 100;
        else
          intresult = -((99 - (year - 1)) / 100);
        break;

      case DTK_MILLENNIUM:
        /* see comments in timestamp_part */
        if (year > 0)
          intresult = (year + 999) / 1000;
        else
          intresult = -((999 - (year - 1)) / 1000);
        break;

      case DTK_JULIAN:
        intresult = date + POSTGRES_EPOCH_JDATE;
        break;

      case DTK_ISOYEAR:
        intresult = date2isoyear(year, mon, mday);
        /* Adjust BC years */
        if (intresult <= 0)
          intresult -= 1;
        break;

      case DTK_DOW:
      case DTK_ISODOW:
        intresult = j2day(date + POSTGRES_EPOCH_JDATE);
        if (val == DTK_ISODOW && intresult == 0)
          intresult = 7;
        break;

      case DTK_DOY:
        intresult = date2j(year, mon, mday) - date2j(year, 1, 1) + 1;
        break;

      default:
        elog(ERROR, "unit \"%s\" not supported for type %s",
          // lowunits, format_type_be(DATEOID));
          lowunits, "date");
        intresult = 0;
    }
  }
  else if (type == RESERV)
  {
    switch (val)
    {
      case DTK_EPOCH:
        intresult = ((int64) date + POSTGRES_EPOCH_JDATE - UNIX_EPOCH_JDATE) * SECS_PER_DAY;
        break;

      default:
        elog(ERROR, "unit \"%s\" not supported for type %s",
          // lowunits, format_type_be(DATEOID));
          lowunits, "date");
        intresult = 0;
    }
  }
  else
  {
    elog(ERROR, "unit \"%s\" not recognized for type %s",
        // lowunits, format_type_be(DATEOID));
        lowunits, "date");
    intresult = 0;
  }

  return int64_to_numeric(intresult);
}

/**
 * @ingroup meos_base_date
 * @brief Add an interval to a date, giving a new date
 * @details Must handle both positive and negative intervals.
 * We implement this by promoting the date to timestamp (without time zone)
 * and then using the timestamp plus interval function.
 * @note Derived from PostgreSQL function @p date_pl_interval()
 */
DateADT
add_date_interval(DateADT date, Interval *span)
{
  Timestamp dateStamp = date2timestamp(date);
  return add_timestamp_interval(dateStamp, span);
}

/**
 * @ingroup meos_base_date
 * @brief Subtract an interval from a date, giving a new date
 * @details Must handle both positive and negative intervals.
 * We implement this by promoting the date to timestamp (without time zone)
 * and then using the timestamp minus interval function.
 * @note Derived from PostgreSQL function @p date_mi_interval()
 */
DateADT
minus_date_interval(DateADT date, Interval *span)
{
  Timestamp dateStamp = date2timestamp(date);
  return minus_timestamp_interval(dateStamp, span);
}

/**
 * @ingroup meos_base_date
 * @brief Convert date to timestamp
 * @note Derived from PostgreSQL function @p date_timestamp()
 */
Timestamp
date_to_timestamp(DateADT date)
{
  return date2timestamp(date);
}

/**
 * @ingroup meos_base_date
 * @brief Convert a timestamp to a date
 * @note Derived from PostgreSQL function @p timestamp_date()
 */
DateADT
timestamp_to_date(Timestamp timestamp)
{
  DateADT result;
  struct pg_tm tt, *tm = &tt;
  fsec_t fsec;

  if (TIMESTAMP_IS_NOBEGIN(timestamp))
    DATE_NOBEGIN(result);
  else if (TIMESTAMP_IS_NOEND(timestamp))
    DATE_NOEND(result);
  else
  {
    if (timestamp2tm(timestamp, NULL, tm, &fsec, NULL, NULL) != 0)
    {
      elog(ERROR, "timestamp out of range");
      return DATEVAL_NOEND;
    }
    result = date2j(tm->tm_year, tm->tm_mon, tm->tm_mday) - POSTGRES_EPOCH_JDATE;
  }
  return result;
}

/**
 * @ingroup meos_base_date
 * @brief Convert a date to a timestamptz
 * @note Derived from PostgreSQL function @p date_timestamptz()
 */
Timestamp
date_to_timestamptz(DateADT date)
{
  return date2timestamptz(date);
}

/**
 * @ingroup meos_base_date
 * @brief Convert a timestamptz to date
 * @note Derived from PostgreSQL function @p timestamptz_date()
 */
DateADT
timestamptz_to_date(TimestampTz timestamp)
{
  DateADT    result;
  struct pg_tm tt,
         *tm = &tt;
  fsec_t    fsec;
  int      tz;

  if (TIMESTAMP_IS_NOBEGIN(timestamp))
    DATE_NOBEGIN(result);
  else if (TIMESTAMP_IS_NOEND(timestamp))
    DATE_NOEND(result);
  else
  {
    if (timestamp2tm(timestamp, &tz, tm, &fsec, NULL, NULL) != 0)
    {
      elog(ERROR, "timestamp out of range");
      return DATEVAL_NOEND;
    }

    result = date2j(tm->tm_year, tm->tm_mon, tm->tm_mday) - POSTGRES_EPOCH_JDATE;
  }

  return result;
}

/*****************************************************************************
 *   Time ADT
 *****************************************************************************/

/**
 * @ingroup meos_base_time
 * @brief Return a time from its string representation
 * @note Derived from PostgreSQL function @p time_in()
 */
#if MEOS
TimeADT
time_in(const char *str, int32 typmod)
{
  return pg_time_in(str, typmod);
}
#endif
TimeADT
pg_time_in(const char *str, int32 typmod)
{
  TimeADT    result;
  fsec_t    fsec;
  struct pg_tm tt,
         *tm = &tt;
  int      tz;
  int      nf;
  int      dterr;
  char    workbuf[MAXDATELEN + 1];
  char     *field[MAXDATEFIELDS];
  int      dtype;
  int      ftype[MAXDATEFIELDS];
  DateTimeErrorExtra extra;

  dterr = ParseDateTime(str, workbuf, sizeof(workbuf),
              field, ftype, MAXDATEFIELDS, &nf);
  if (dterr == 0)
    dterr = DecodeTimeOnly(field, ftype, nf,
                 &dtype, tm, &fsec, &tz, &extra);
  if (dterr != 0)
  {
    DateTimeParseError(dterr, &extra, str, "time", NULL);
    return PG_INT64_MAX;
  }

  tm2time(tm, fsec, &result);
  AdjustTimeForTypmod(&result, typmod);

 return result;
}

/* tm2time()
 * Convert a tm structure to a time data type.
 */
int
tm2time(struct pg_tm *tm, fsec_t fsec, TimeADT *result)
{
  *result = ((((tm->tm_hour * MINS_PER_HOUR + tm->tm_min) * SECS_PER_MINUTE) + tm->tm_sec)
         * USECS_PER_SEC) + fsec;
  return 0;
}

/* time_overflows()
 * Check to see if a broken-down time-of-day is out of range.
 */
bool
time_overflows(int hour, int min, int sec, fsec_t fsec)
{
  /* Range-check the fields individually. */
  if (hour < 0 || hour > HOURS_PER_DAY ||
    min < 0 || min >= MINS_PER_HOUR ||
    sec < 0 || sec > SECS_PER_MINUTE ||
    fsec < 0 || fsec > USECS_PER_SEC)
    return true;

  /*
   * Because we allow, eg, hour = 24 or sec = 60, we must check separately
   * that the total time value doesn't exceed 24:00:00.
   */
  if ((((((hour * MINS_PER_HOUR + min) * SECS_PER_MINUTE)
       + sec) * USECS_PER_SEC) + fsec) > USECS_PER_DAY)
    return true;

  return false;
}

/* float_time_overflows()
 * Same, when we have seconds + fractional seconds as one "double" value.
 */
bool
float_time_overflows(int hour, int min, double sec)
{
  /* Range-check the fields individually. */
  if (hour < 0 || hour > HOURS_PER_DAY ||
    min < 0 || min >= MINS_PER_HOUR)
    return true;

  /*
   * "sec", being double, requires extra care.  Cope with NaN, and round off
   * before applying the range check to avoid unexpected errors due to
   * imprecise input.  (We assume rint() behaves sanely with infinities.)
   */
  if (isnan(sec))
    return true;
  sec = rint(sec * USECS_PER_SEC);
  if (sec < 0 || sec > SECS_PER_MINUTE * USECS_PER_SEC)
    return true;

  /*
   * Because we allow, eg, hour = 24 or sec = 60, we must check separately
   * that the total time value doesn't exceed 24:00:00.  This must match the
   * way that callers will convert the fields to a time.
   */
  if (((((hour * MINS_PER_HOUR + min) * SECS_PER_MINUTE)
      * USECS_PER_SEC) + (int64) sec) > USECS_PER_DAY)
    return true;

  return false;
}


/* time2tm()
 * Convert time data type to POSIX time structure.
 *
 * Note that only the hour/min/sec/fractional-sec fields are filled in.
 */
int
time2tm(TimeADT time, struct pg_tm *tm, fsec_t *fsec)
{
  tm->tm_hour = time / USECS_PER_HOUR;
  time -= tm->tm_hour * USECS_PER_HOUR;
  tm->tm_min = time / USECS_PER_MINUTE;
  time -= tm->tm_min * USECS_PER_MINUTE;
  tm->tm_sec = time / USECS_PER_SEC;
  time -= tm->tm_sec * USECS_PER_SEC;
  *fsec = time;
  return 0;
}

/**
 * @ingroup meos_base_time
 * @brief Return the string representation of a time
 * @note Derived from PostgreSQL function @p time_out()
 */
#if MEOS
char *
time_out(TimeADT time)
{
  return pg_time_out(time);
}
#endif
char *
pg_time_out(TimeADT time)
{
  char     *result;
  struct pg_tm tt,
         *tm = &tt;
  fsec_t    fsec;
  char    buf[MAXDATELEN + 1];

  time2tm(time, tm, &fsec);
  EncodeTimeOnly(tm, fsec, false, 0, DateStyle, buf);

  result = pstrdup(buf);
  return result;
}

char *
time_typmodout(int32 typmod)
{
  return anytime_typmodout(false, typmod);
}

/**
 * @ingroup meos_base_time
 * @brief Construct a time from the arguments
 * @return On error return PG_INT64_MAX
 * @note Derived from PostgreSQL function @p make_time()
 */
TimeADT
time_make(int tm_hour, int tm_min, double sec)
{
  /* Check for time overflow */
  if (float_time_overflows(tm_hour, tm_min, sec))
  {
    elog(ERROR, "time field value out of range: %d:%02d:%02g",
      tm_hour, tm_min, sec);
    return PG_INT64_MAX;
  }

  /* This should match tm2time */
  TimeADT time = (((tm_hour * MINS_PER_HOUR + tm_min) * SECS_PER_MINUTE)
      * USECS_PER_SEC) + (int64) rint(sec * USECS_PER_SEC);
 return time;
}

/**
 * @ingroup meos_base_time
 * @brief Adjust a time to a scale factor
 * @note Derived from PostgreSQL function @p time_scale()
 */
#if MEOS
TimeADT
time_scale(TimeADT date, int32 typmod)
{
  return pg_time_scale(date, typmod);
}
#endif
TimeADT
pg_time_scale(TimeADT date, int32 typmod)
{
  TimeADT result = time;
  AdjustTimeForTypmod(&result, typmod);
  return result;
}

/* AdjustTimeForTypmod()
 * Force the precision of the time value to a specified value.
 * Uses *exactly* the same code as in AdjustTimestampForTypmod()
 * but we make a separate copy because those types do not
 * have a fundamental tie together but rather a coincidence of
 * implementation. - thomas
 */
void
AdjustTimeForTypmod(TimeADT *time, int32 typmod)
{
  static const int64 TimeScales[MAX_TIME_PRECISION + 1] = {
    INT64CONST(1000000),
    INT64CONST(100000),
    INT64CONST(10000),
    INT64CONST(1000),
    INT64CONST(100),
    INT64CONST(10),
    INT64CONST(1)
  };

  static const int64 TimeOffsets[MAX_TIME_PRECISION + 1] = {
    INT64CONST(500000),
    INT64CONST(50000),
    INT64CONST(5000),
    INT64CONST(500),
    INT64CONST(50),
    INT64CONST(5),
    INT64CONST(0)
  };

  if (typmod >= 0 && typmod <= MAX_TIME_PRECISION)
  {
    if (*time >= INT64CONST(0))
      *time = ((*time + TimeOffsets[typmod]) / TimeScales[typmod]) *
        TimeScales[typmod];
    else
      *time = -((((-*time) + TimeOffsets[typmod]) / TimeScales[typmod]) *
            TimeScales[typmod]);
  }
}

/**
 * @ingroup meos_base_time
 * @brief Return true if two times are equal
 * @note Derived from PostgreSQL function @p time_eq()
 */
#if MEOS
bool
time_eq(TimeADT time1, TimeADT time2)
{
  return (time1 == time2);
}
#endif
bool
pg_time_eq(TimeADT time1, TimeADT time2)
{
  return (time1 == time2);
}

/**
 * @ingroup meos_base_time
 * @brief Return true if two times are not equal
 * @note Derived from PostgreSQL function @p time_ne()
 */
#if MEOS
bool
time_ne(TimeADT time1, TimeADT time2)
{
  return (time1 != time2);
}
#endif
bool
pg_time_ne(TimeADT time1, TimeADT time2)
{
  return (time1 != time2);
}

/**
 * @ingroup meos_base_time
 * @brief Return true if a time is less than another one
 * @note Derived from PostgreSQL function @p time_lt()
 */
#if MEOS
bool
time_lt(TimeADT time1, TimeADT time2)
{
  return (time1 < time2);
}
#endif
bool
pg_time_lt(TimeADT time1, TimeADT time2)
{
  return (time1 < time2);
}

/**
 * @ingroup meos_base_time
 * @brief Return true if a time is less than or equal to another one
 * @note Derived from PostgreSQL function @p time_le()
 */
#if MEOS
bool
time_le(TimeADT time1, TimeADT time2)
{
  return (time1 <= time2);
}
#endif
bool
pg_time_le(TimeADT time1, TimeADT time2)
{
  return (time1 <= time2);
}

/**
 * @ingroup meos_base_time
 * @brief Return true if a time is greater than another one
 * @note Derived from PostgreSQL function @p time_gt()
 */
#if MEOS
bool
time_gt(TimeADT time1, TimeADT time2)
{
  return (time1 > time2);
}
#endif
bool
pg_time_gt(TimeADT time1, TimeADT time2)
{
  return (time1 > time2);
}

/**
 * @ingroup meos_base_time
 * @brief Return true if a time is greater than or equal to another one
 * @note Derived from PostgreSQL function @p time_ge()
 */
#if MEOS
bool
time_ge(TimeADT time1, TimeADT time2)
{
  return (time1 >= time2);
}
#endif
bool
pg_time_ge(TimeADT time1, TimeADT time2)
{
  return (time1 >= time2);
}

/**
 * @ingroup meos_base_time
 * @brief Return -1, 0, or 1 depending on whether the first time is less than,
 * equal to, or less than the second one
 * @note Derived from PostgreSQL function @p time_cmp()
 */
#if MEOS
int
time_cmp(TimeADT time1, TimeADT time2)
{
  return pg_time_cmp(time1, time2);
}
#endif
int
pg_time_cmp(TimeADT time1, TimeADT time2)
{
  if (time1 < time2)
    return -1;
  if (time1 > time2)
    return 1;
  return 0;
}

/**
 * @ingroup meos_base_time
 * @brief Return the 32-bit hash value of a time
 * @note Derived from PostgreSQL function @p time_hash()
 */
#if MEOS
uint32
time_hash(TimeADT time)
{
  return int64_hash(time);
}
#endif
uint32
pg_time_hash(TimeADT time)
{
  return int64_hash(time);
}

/**
 * @ingroup meos_base_time
 * @brief Return the 64-bit hash value of a time using a seed
 * @note Derived from PostgreSQL function @p time_hash_extended()
 */
#if MEOS
uint64
time_hash_extended(TimeADT time, int32 seed)
{
  return int64_hash_extended(time, seed);
}
#endif
uint64
pg_time_hash_extended(TimeADT time, int32 seed)
{
  return int64_hash_extended(time, seed);
}

/**
 * @ingroup meos_base_time
 * @brief Return the larger of two times
 * @note Derived from PostgreSQL function @p time_larger()
 */
#if MEOS
TimeADT
time_larger(TimeADT time1, TimeADT time2)
{
 return ((time1 > time2) ? time1 : time2);
}
#endif
TimeADT
pg_time_larger(TimeADT time1, TimeADT time2)
{
 return ((time1 > time2) ? time1 : time2);
}

/**
 * @ingroup meos_base_time
 * @brief Return the smaller of two times
 * @note Derived from PostgreSQL function @p time_smaller()
 */
#if MEOS
TimeADT
time_smaller(TimeADT time1, TimeADT time2)
{
 return ((time1 < time2) ? time1 : time2);
}
#endif
TimeADT
pg_time_smaller(TimeADT time1, TimeADT time2)
{
 return ((time1 < time2) ? time1 : time2);
}

/**
 * @ingroup meos_base_date
 * @brief Return true if two times overalp
 * @details Implements the SQL OVERLAPS operator, although in this case none
 * of the inputs is null
 * @note Derived from PostgreSQL function @p overlaps_time()
 */
bool
time_overlaps(TimeADT *ts1, TimeADT *te1, TimeADT *ts2, TimeADT *te2)
{
#define TIMEADT_GT(t1,t2) \
  (DatumGetTimeADT(t1) > DatumGetTimeADT(t2))
#define TIMEADT_LT(t1,t2) \
  (DatumGetTimeADT(t1) < DatumGetTimeADT(t2))

  /*
   * We can consider three cases: ts1 > ts2, ts1 < ts2, ts1 = ts2
   */
  if (TIMEADT_GT(ts1, ts2))
  {
    /*
     * This case is ts1 < te2 OR te1 < te2, which may look redundant but
     * in the presence of nulls it's not quite completely so.
     */
    if (TIMEADT_LT(ts1, te2))
      return true;

    /*
     * We had ts1 <= te1 above, and we just found ts1 >= te2, hence te1 >= te2
     */
    return false;
  }
  else if (TIMEADT_LT(ts1, ts2))
  {
    /* This case is ts2 < te1 OR te2 < te1 */
    if (TIMEADT_LT(ts2, te1))
      return true;

    /*
     * We had ts2 <= te2 above, and we just found ts2 >= te1, hence te2 >= te1
     */
    return false;
  }
  else
  {
    /*
     * For ts1 = ts2 the spec says te1 <> te2 OR te1 = te2, which is a
     * rather silly way of saying "true if both are nonnull, else null".
     */
    return true;
  }

#undef TIMEADT_GT
#undef TIMEADT_LT
}

/**
 * @ingroup meos_base_time
 * @brief Convert a timestamp to a time
 * @note Derived from PostgreSQL function @p timestamp_time()
 */
TimeADT
timestamp_to_time(Timestamp timestamp)
{
  TimeADT    result;
  struct pg_tm tt,
         *tm = &tt;
  fsec_t    fsec;

  if (TIMESTAMP_NOT_FINITE(timestamp))
    return PG_INT64_MAX;

  if (timestamp2tm(timestamp, NULL, tm, &fsec, NULL, NULL) != 0)
  {
    elog(ERROR, "timestamp out of range");
    return PG_INT64_MAX;
  }

  /*
   * Could also do this with time = (timestamp / USECS_PER_DAY *
   * USECS_PER_DAY) - timestamp;
   */
  result = ((((tm->tm_hour * MINS_PER_HOUR + tm->tm_min) * SECS_PER_MINUTE) + tm->tm_sec) *
        USECS_PER_SEC) + fsec;

 return result;
}

/**
 * @ingroup meos_base_time
 * @brief Convert a timestamptz to a time
 * @note Derived from PostgreSQL function @p timestamptz_time()
 */
TimeADT
timestamptz_to_time(TimestampTz timestamp)
{
  TimeADT    result;
  struct pg_tm tt,
         *tm = &tt;
  int      tz;
  fsec_t    fsec;

  if (TIMESTAMP_NOT_FINITE(timestamp))
    return PG_INT64_MAX;

  if (timestamp2tm(timestamp, &tz, tm, &fsec, NULL, NULL) != 0)
  {
    elog(ERROR, "timestamp out of range");
    return PG_INT64_MAX;
  }

  /*
   * Could also do this with time = (timestamp / USECS_PER_DAY *
   * USECS_PER_DAY) - timestamp;
   */
  result = ((((tm->tm_hour * MINS_PER_HOUR + tm->tm_min) * 
    SECS_PER_MINUTE) + tm->tm_sec) * USECS_PER_SEC) + fsec;

 return result;
}

/**
 * @ingroup meos_base_timestamp
 * @brief Convert a date and a time to a timestamp
 * @note Derived from PostgreSQL function @p datetime_timestamp()
 */
Timestamp
date_time_to_timestamp(DateADT date, TimeADT time)
{
  Timestamp result = date2timestamp(date);

  if (!TIMESTAMP_NOT_FINITE(result))
  {
    result += time;
    if (!IS_VALID_TIMESTAMP(result))
    {
      elog(ERROR, "timestamp out of range");
      return PG_INT64_MAX;
    }
  }

  return result;
}

/**
 * @ingroup meos_base_time
 * @brief Convert a time to an interval
 * @note Derived from PostgreSQL function @p time_interval()
 */
Interval *
time_to_interval(TimeADT date)
{
  Interval *result = (Interval *) palloc(sizeof(Interval));
  result->time = time;
  result->day = 0;
  result->month = 0;
  return result;
}

/**
 * @ingroup meos_base_time
 * @brief Convert an interval to a time
 * @details This is defined as producing the fractional-day portion of the
 * interval. Therefore, we can just ignore the months field.  It is not real
 * clear what to do with negative intervals, but we choose to subtract the
 * floor, so that, say, '-2 hours' becomes '22:00:00'.
 * @note Derived from PostgreSQL function @p interval_time()
 */
TimeADT
interval_to_time(const Interval *span)
{
  if (INTERVAL_NOT_FINITE(span))
  {
    elog(ERROR, "cannot convert infinite interval to time");
    return PG_INT64_MAX;
  }

  TimeADT result = span->time % USECS_PER_DAY;
  if (result < 0)
    result += USECS_PER_DAY;
 return result;
}

/**
 * @ingroup meos_base_time
 * @brief Subtract two times to produce an interval
 * @note Derived from PostgreSQL function @p time_mi_time()
 */
Interval *
minus_time_time(TimeADT time1, TimeADT time2)
{
  Interval *result = (Interval *) palloc(sizeof(Interval));
  result->month = 0;
  result->day = 0;
  result->time = time1 - time2;
  return result;
}

/**
 * @ingroup meos_base_time
 * @brief Add an interval to a time
 * @note Derived from PostgreSQL function @p time_pl_interval()
 */
TimeADT
plus_time_interval(TimeADT date, Interval *span)
{
  TimeADT    result;

  if (INTERVAL_NOT_FINITE(span))
  {
    elog(ERROR, "cannot add infinite interval to time");
    return PG_INT64_MAX;
  }

  result = time + span->time;
  result -= result / USECS_PER_DAY * USECS_PER_DAY;
  if (result < INT64CONST(0))
    result += USECS_PER_DAY;

 return result;
}

/**
 * @ingroup meos_base_time
 * @brief Subtract an interval from a time
 * @note Derived from PostgreSQL function @p time_mi_interval()
 */
TimeADT
minus_time_interval(TimeADT date, Interval *span)
{
  if (INTERVAL_NOT_FINITE(span))
  {
    elog(ERROR, "cannot subtract infinite interval from time");
    return PG_INT64_MAX;
  }

  TimeADT result = time - span->time;
  result -= result / USECS_PER_DAY * USECS_PER_DAY;
  if (result < INT64CONST(0))
    result += USECS_PER_DAY;
 return result;
}

/* time_part() and extract_time()
 * Extract specified field from time type.
 */
static Datum
time_part_common(text *units, TimeADT time, bool retnumeric)
{
  int64    intresult;
  int      type,
        val;
  char     *lowunits;

  lowunits = downcase_truncate_identifier(VARDATA_ANY(units),
                      VARSIZE_ANY_EXHDR(units),
                      false);

  type = DecodeUnits(0, lowunits, &val);
  if (type == UNKNOWN_FIELD)
    type = DecodeSpecial(0, lowunits, &val);

  if (type == UNITS)
  {
    fsec_t    fsec;
    struct pg_tm tt,
           *tm = &tt;

    time2tm(time, tm, &fsec);

    switch (val)
    {
      case DTK_MICROSEC:
        intresult = tm->tm_sec * INT64CONST(1000000) + fsec;
        break;

      case DTK_MILLISEC:
        if (retnumeric)
          /*---
           * tm->tm_sec * 1000 + fsec / 1000
           * = (tm->tm_sec * 1'000'000 + fsec) / 1000
           */
          NumericGetDatum(int64_div_fast_to_numeric(tm->tm_sec * INT64CONST(1000000) + fsec, 3));
        else
          Float8GetDatum(tm->tm_sec * 1000.0 + fsec / 1000.0);
        break;

      case DTK_SECOND:
        if (retnumeric)
          /*---
           * tm->tm_sec + fsec / 1'000'000
           * = (tm->tm_sec * 1'000'000 + fsec) / 1'000'000
           */
          NumericGetDatum(int64_div_fast_to_numeric(tm->tm_sec * INT64CONST(1000000) + fsec, 6));
        else
          Float8GetDatum(tm->tm_sec + fsec / 1000000.0);
        break;

      case DTK_MINUTE:
        intresult = tm->tm_min;
        break;

      case DTK_HOUR:
        intresult = tm->tm_hour;
        break;

      case DTK_TZ:
      case DTK_TZ_MINUTE:
      case DTK_TZ_HOUR:
      case DTK_DAY:
      case DTK_MONTH:
      case DTK_QUARTER:
      case DTK_YEAR:
      case DTK_DECADE:
      case DTK_CENTURY:
      case DTK_MILLENNIUM:
      case DTK_ISOYEAR:
      default:
        elog(ERROR, "unit \"%s\" not supported for type %s",
          // lowunits, format_type_be(TIMEOID));
          lowunits, "time without time zone");
        intresult = 0;
    }
  }
  else if (type == RESERV && val == DTK_EPOCH)
  {
    if (retnumeric)
      return NumericGetDatum(int64_div_fast_to_numeric(time, 6));
    else
      return Float8GetDatum(time / 1000000.0);
  }
  else
  {
    elog(ERROR, "unit \"%s\" not recognized for type %s",
      // lowunits, format_type_be(TIMEOID));
      lowunits, "time without time zone");
    intresult = 0;
  }

  if (retnumeric)
    return NumericGetDatum(int64_to_numeric(intresult));
  else
    return Float8GetDatum(intresult);
}

/**
 * @ingroup meos_base_time
 * @brief Extract a field from a time
 * @note Derived from PostgreSQL function @p time_part()
 */
#if MEOS
float8
time_part(TimeADT time, const text *units)
{
  return DatumGetFloat8(time_part_common(units, time, false));
}
#endif
float8
pg_time_part(TimeADT time, const text *units)
{
  return DatumGetFloat8(time_part_common(units, time, false));
}

/**
 * @ingroup meos_base_time
 * @brief Extract a field from a time
 * @note Derived from PostgreSQL function @p extract_time()
 */
Numeric
time_extract(TimeADT time, const text *units)
{
  return DatumGetNumeric(time_part_common(units, time, true));
}

/*****************************************************************************
 *   Time With Time Zone ADT
 *****************************************************************************/

/* tm2timetz()
 * Convert a tm structure to a time data type.
 */
int
tm2timetz(struct pg_tm *tm, fsec_t fsec, int tz, TimeTzADT *result)
{
  result->time = ((((tm->tm_hour * MINS_PER_HOUR + tm->tm_min) * SECS_PER_MINUTE) + tm->tm_sec) *
          USECS_PER_SEC) + fsec;
  result->zone = tz;

  return 0;
}

/**
 * @ingroup meos_base_time
 * @brief Return true if two dates are equal
 * @note Derived from PostgreSQL function @p timetz_in()
 */
#if MEOS
TimeTzADT *
timetz_in(const char *str, int32 typmod)
{
  return pg_timetz_in(str, typmod);
}
#endif
TimeTzADT *
pg_timetz_in(const char *str, int32 typmod)
{
  TimeTzADT  *result;
  fsec_t    fsec;
  struct pg_tm tt,
         *tm = &tt;
  int      tz;
  int      nf;
  int      dterr;
  char    workbuf[MAXDATELEN + 1];
  char     *field[MAXDATEFIELDS];
  int      dtype;
  int      ftype[MAXDATEFIELDS];
  DateTimeErrorExtra extra;

  dterr = ParseDateTime(str, workbuf, sizeof(workbuf),
              field, ftype, MAXDATEFIELDS, &nf);
  if (dterr == 0)
    dterr = DecodeTimeOnly(field, ftype, nf,
                 &dtype, tm, &fsec, &tz, &extra);
  if (dterr != 0)
  {
    DateTimeParseError(dterr, &extra, str, "time with time zone", NULL);
    return NULL;
  }

  result = (TimeTzADT *) palloc(sizeof(TimeTzADT));
  tm2timetz(tm, fsec, tz, result);
  AdjustTimeForTypmod(&(result->time), typmod);

  return result;
}

/**
 * @ingroup meos_base_time
 * @brief Return true if two dates are equal
 * @note Derived from PostgreSQL function @p timetz_out()
 */
#if MEOS
char *
timetz_out(const TimeTzADT *time)
{
  return pg_timetz_out(time);
}
#endif
char *
pg_timetz_out(const TimeTzADT *time)
{
  char *result;
  struct pg_tm tt,
         *tm = &tt;
  fsec_t    fsec;
  int      tz;
  char    buf[MAXDATELEN + 1];

  timetz2tm(time, tm, &fsec, &tz);
  EncodeTimeOnly(tm, fsec, true, tz, DateStyle, buf);

  result = pstrdup(buf);
  return result;
}

/* Output the typmod of a timetz */
char *
timetz_typmodout(int32 typmod)
{
  return anytime_typmodout(true, typmod);
}

/* timetz2tm()
 * Convert TIME WITH TIME ZONE data type to POSIX time structure.
 */
int
timetz2tm(TimeTzADT *time, struct pg_tm *tm, fsec_t *fsec, int *tzp)
{
  TimeOffset  trem = time->time;

  tm->tm_hour = trem / USECS_PER_HOUR;
  trem -= tm->tm_hour * USECS_PER_HOUR;
  tm->tm_min = trem / USECS_PER_MINUTE;
  trem -= tm->tm_min * USECS_PER_MINUTE;
  tm->tm_sec = trem / USECS_PER_SEC;
  *fsec = trem - tm->tm_sec * USECS_PER_SEC;

  if (tzp != NULL)
    *tzp = time->zone;

  return 0;
}

/**
 * @ingroup meos_base_time
 * @brief Adjust time type for specified scale factor
 * @note Derived from PostgreSQL function @p timetz_scale()
 */
#if MEOS
TimeTzADT *
timetz_scale(const TimeTzADT *time, int32 typmod)
{
  return pg_timetz_scale(time, typmod);
}
#endif
TimeTzADT *
pg_timetz_scale(const TimeTzADT *time, int32 typmod)
{
  TimeTzADT *result = (TimeTzADT *) palloc(sizeof(TimeTzADT));
  result->time = time->time;
  result->zone = time->zone;
  AdjustTimeForTypmod(&(result->time), typmod);
  return result;
}

static int
timetz_cmp_internal(TimeTzADT *time1, TimeTzADT *time2)
{
  TimeOffset t1, t2;

  /* Primary sort is by true (GMT-equivalent) time */
  t1 = time1->time + (time1->zone * USECS_PER_SEC);
  t2 = time2->time + (time2->zone * USECS_PER_SEC);

  if (t1 > t2)
    return 1;
  if (t1 < t2)
    return -1;

  /*
   * If same GMT time, sort by timezone; we only want to say that two
   * timetz's are equal if both the time and zone parts are equal.
   */
  if (time1->zone > time2->zone)
    return 1;
  if (time1->zone < time2->zone)
    return -1;

  return 0;
}

/**
 * @ingroup meos_base_time
 * @brief Return true if two dates are equal
 * @note Derived from PostgreSQL function @p timetz_eq()
 */
#if MEOS
bool
timetz_eq(const TimeTzADT *time1, const TimeTzADT *time2)
{
  return (timetz_cmp_internal(time1, time2) == 0);
}
#endif
bool
pg_timetz_eq(const TimeTzADT *time1, const TimeTzADT *time2)
{
  return (timetz_cmp_internal(time1, time2) == 0);
}

/**
 * @ingroup meos_base_time
 * @brief Return true if two dates are equal
 * @note Derived from PostgreSQL function @p timetz_ne()
 */
#if MEOS
bool
timetz_ne(const TimeTzADT *time1, const TimeTzADT *time2)
{
  return (timetz_cmp_internal(time1, time2) != 0);
}
#endif
bool
pg_timetz_ne(const TimeTzADT *time1, const TimeTzADT *time2)
{
  return (timetz_cmp_internal(time1, time2) != 0);
}

/**
 * @ingroup meos_base_time
 * @brief Return true if two dates are equal
 * @note Derived from PostgreSQL function @p timetz_lt()
 */
#if MEOS
bool
timetz_lt(const TimeTzADT *time1, const TimeTzADT *time2)
{
  return (timetz_cmp_internal(time1, time2) < 0);
}
#endif
bool
pg_timetz_lt(const TimeTzADT *time1, const TimeTzADT *time2)
{
  return (timetz_cmp_internal(time1, time2) < 0);
}

/**
 * @ingroup meos_base_time
 * @brief Return true if two dates are equal
 * @note Derived from PostgreSQL function @p timetz_le()
 */
#if MEOS
bool
timetz_le(const TimeTzADT *time1, const TimeTzADT *time2)
{
  return (timetz_cmp_internal(time1, time2) <= 0);
}
#endif
bool
pg_timetz_le(const TimeTzADT *time1, const TimeTzADT *time2)
{
  return (timetz_cmp_internal(time1, time2) <= 0);
}

/**
 * @ingroup meos_base_time
 * @brief Return true if two dates are equal
 * @note Derived from PostgreSQL function @p timetz_gt()
 */
#if MEOS
bool
timetz_gt(const TimeTzADT *time1, const TimeTzADT *time2)
{
  return (timetz_cmp_internal(time1, time2) > 0);
}
#endif
bool
pg_timetz_gt(const TimeTzADT *time1, const TimeTzADT *time2)
{
  return (timetz_cmp_internal(time1, time2) > 0);
}

/**
 * @ingroup meos_base_time
 * @brief Return true if two dates are equal
 * @note Derived from PostgreSQL function @p timetz_ge()
 */
#if MEOS
bool
timetz_ge(const TimeTzADT *time1, const TimeTzADT *time2)
{
  return (timetz_cmp_internal(time1, time2) >= 0);
}
#endif
bool
pg_timetz_ge(const TimeTzADT *time1, const TimeTzADT *time2)
{
  return (timetz_cmp_internal(time1, time2) >= 0);
}

/**
 * @ingroup meos_base_time
 * @brief Return -1, 0, or 1 depending on whether the first timetz is less than,
 * equal to, or less than the second one
 * @note Derived from PostgreSQL function @p timetz_cmp()
 */
#if MEOS
int32
timetz_cmp(const TimeTzADT *time1, const TimeTzADT *time2)
{
  return (timetz_cmp_internal(time1, time2));
}
#endif
int32
pg_timetz_cmp(const TimeTzADT *time1, const TimeTzADT *time2)
{
  return (timetz_cmp_internal(time1, time2));
}

/**
 * @ingroup meos_base_time
 * @brief Return true if two dates are equal
 * @note Derived from PostgreSQL function @p timetz_hash()
 */
#if MEOS
uint32
timetz_hash(const TimeTzADT *key)
{
  return pg_timetz_hash(key);
}
#endif
uint32
pg_timetz_hash(const TimeTzADT *key)
{
  /*
   * To avoid any problems with padding bytes in the struct, we figure the
   * field hashes separately and XOR them.
   */
  uint32 thash = int64_hash(key->time);
  thash ^= DatumGetUInt32(hash_uint32(key->zone));
  return thash;
}

/**
 * @ingroup meos_base_time
 * @brief Return true if two dates are equal
 * @note Derived from PostgreSQL function @p timetz_hash_extended()
 */
#if MEOS
uint64
timetz_hash_extended(const TimeTzADT *key, int64 seed)
{
  return pg_timetz_hash_extended(key, seed);
}
#endif
uint64
pg_timetz_hash_extended(const TimeTzADT *key, int64 seed)
{
  /* Same approach as timetz_hash */
  uint64 thash = int64_hash_extended(key->time, seed);
  thash ^= DatumGetUInt64(hash_uint32_extended(key->zone,
    DatumGetInt64(seed)));
  return thash;
}

/**
 * @ingroup meos_base_time
 * @brief Return true if two dates are equal
 * @note Derived from PostgreSQL function @p timetz_larger()
 */
#if MEOS
TimeTzADT * 
timetz_larger(const TimeTzADT *time1, const TimeTzADT *time2)
{
  return pg_timetz_larger(time1, time2);
}
#endif
TimeTzADT *
pg_timetz_larger(const TimeTzADT *time1, const TimeTzADT *time2)
{
  TimeTzADT *result;
  if (timetz_cmp_internal(time1, time2) > 0)
    result = time1;
  else
    result = time2;
  return result;
}

/**
 * @ingroup meos_base_time
 * @brief Return true if two dates are equal
 * @note Derived from PostgreSQL function @p timetz_smaller()
 */
#if MEOS
TimeTzADT * 
timetz_smaller(const TimeTzADT *time1, const TimeTzADT *time2)
{
  return pg_timetz_smaller(time1, time2);
}
#endif
TimeTzADT * 
pg_timetz_smaller(const TimeTzADT *time1, const TimeTzADT *time2)
{
  TimeTzADT *result;
  if (timetz_cmp_internal(time1, time2) < 0)
    result = time1;
  else
    result = time2;
  return result;
}

/**
 * @ingroup meos_base_time
 * @brief Add an interval to a timetz
 * @note Derived from PostgreSQL function @p timetz_pl_interval()
 */
TimeTzADT *
plus_timetz_interval(const TimeTzADT *time, const Interval *span)
{
  if (INTERVAL_NOT_FINITE(span))
  {
    elog(ERROR, "cannot add infinite interval to time");
    return PG_INT64_MAX;
  }

  TimeTzADT *result = (TimeTzADT *) palloc(sizeof(TimeTzADT));
  result->time = time->time + span->time;
  result->time -= result->time / USECS_PER_DAY * USECS_PER_DAY;
  if (result->time < INT64CONST(0))
    result->time += USECS_PER_DAY;
  result->zone = time->zone;
  return result;
}

/**
 * @ingroup meos_base_time
 * @brief Subtract an interval from a timetz
 * @note Derived from PostgreSQL function @p timetz_mi_interval()
 */
TimeTzADT *
minus_timetz_interval(const TimeTzADT *time, const Interval *span)
{
  if (INTERVAL_NOT_FINITE(span))
  {
    elog(ERROR, "cannot subtract infinite interval from time");
    return PG_INT64_MAX;
  }

  TimeTzADT *result = (TimeTzADT *) palloc(sizeof(TimeTzADT));
  result->time = time->time - span->time;
  result->time -= result->time / USECS_PER_DAY * USECS_PER_DAY;
  if (result->time < INT64CONST(0))
    result->time += USECS_PER_DAY;
  result->zone = time->zone;
  return result;
}

/**
 * @ingroup meos_base_time
 * @brief Return true if two timetz values overlap
 * @details Implements the SQL OVERLAPS operator, although in this case none
 * of the inputs is null
 * @note Derived from PostgreSQL function @p overlaps_timetz()
 */
bool
timetz_overlaps(const TimeTzADT *ts1, const TimeTzADT *te1,
  const TimeTzADT *ts2, const TimeTzADT *te2)
{
  /*
   * We can consider three cases: ts1 > ts2, ts1 < ts2, ts1 = ts2
   */
  if (pg_timetz_gt(ts1, ts2))
  {
    /*
     * This case is ts1 < te2 OR te1 < te2, which may look redundant but
     * in the presence of nulls it's not quite completely so.
     */
    if (pg_timetz_lt(ts1, te2))
      return true;

    /*
     * We had ts1 <= te1 above, and we just found ts1 >= te2, hence te1 >= te2
     */
    return false;
  }
  else if (pg_timetz_lt(ts1, ts2))
  {
    /* This case is ts2 < te1 OR te2 < te1 */
    if (pg_timetz_lt(ts2, te1))
      return true;

    /*
     * We had ts2 <= te2 above, and we just found ts2 >= te1, hence te2 >= te1
     */
    return false;
  }
  else
  {
    /*
     * For ts1 = ts2 the spec says te1 <> te2 OR te1 = te2, which is a
     * rather silly way of saying "true if both are nonnull, else null".
     */
    return true;
  }
}

/**
 * @ingroup meos_base_time
 * @brief Return true if two dates are equal
 * @note Derived from PostgreSQL function @p timetz_time()
 */
TimeADT
timetz_to_time(const TimeTzADT *timetz)
{
  /* swallow the time zone and just return the time */
  return timetz->time;
}

/**
 * @ingroup meos_base_time
 * @brief Return true if two dates are equal
 * @note Derived from PostgreSQL function @p time_timetz()
 */
TimeTzADT *
time_to_timetz(TimeADT date)
{
  TimeTzADT *result;
  struct pg_tm tt, *tm = &tt;
  fsec_t fsec;
  int tz;

  GetCurrentDateTime(tm);
  time2tm(time, tm, &fsec);
  tz = DetermineTimeZoneOffset(tm, session_timezone);

  result = (TimeTzADT *) palloc(sizeof(TimeTzADT));

  result->time = time;
  result->zone = tz;

  return result;
}

/**
 * @ingroup meos_base_time
 * @brief Convert timestamp to timetz data type
 * @note Derived from PostgreSQL function @p timestamptz_timetz()
 */
TimeTzADT *
timestamptz_to_timetz(TimestampTz timestamp)
{
  struct pg_tm tt, *tm = &tt;
  int tz;
  fsec_t fsec;

  if (TIMESTAMP_NOT_FINITE(timestamp))
    return PG_INT64_MAX;

  if (timestamp2tm(timestamp, &tz, tm, &fsec, NULL, NULL) != 0)
  {
    elog(ERROR, "timestamp out of range");
    return PG_INT64_MAX;
  }

  TimeTzADT *result = (TimeTzADT *) palloc(sizeof(TimeTzADT));
  tm2timetz(tm, fsec, tz, result);
  return result;
}

/**
 * @ingroup meos_base_time
 * @brief Convert date and timetz to timestamp with time zone data type
 * @details Timestamp is stored in GMT, so add the time zone stored with the
 * timetz to the result.
 * @note Derived from PostgreSQL function @p datetimetz_timestamptz()
 */
TimestampTz
date_timetz_to_timestamptz(DateADT date, const TimeTzADT *time)
{
  if (DATE_IS_NOBEGIN(date))
    return DT_NOBEGIN;
  else if (DATE_IS_NOEND(date))
    return DT_NOEND;
  else
  {
    /*
     * Date's range is wider than timestamp's, so check for boundaries.
     * Since dates have the same minimum values as timestamps, only upper
     * boundary need be checked for overflow.
     */
    if (date >= (TIMESTAMP_END_JULIAN - POSTGRES_EPOCH_JDATE))
    {
      elog(ERROR, "date out of range for timestamp");
      return DT_NOEND;
    }

    TimestampTz result = date * USECS_PER_DAY + time->time + time->zone * 
      USECS_PER_SEC;

    /*
     * Since it is possible to go beyond allowed timestamptz range because
     * of time zone, check for allowed timestamp range after adding tz.
     */
    if (!IS_VALID_TIMESTAMP(result))
    {
      elog(ERROR, "date out of range for timestamp");
      return DT_NOEND;
    }

    return result;
  }
}


/* timetz_part() and extract_timetz()
 * Extract specified field from time type.
 */
static Datum
timetz_part_common(text *units, TimeTzADT *time, bool retnumeric)
{
  int64    intresult;
  int      type,
        val;
  char     *lowunits;

  lowunits = downcase_truncate_identifier(VARDATA_ANY(units),
                      VARSIZE_ANY_EXHDR(units),
                      false);

  type = DecodeUnits(0, lowunits, &val);
  if (type == UNKNOWN_FIELD)
    type = DecodeSpecial(0, lowunits, &val);

  if (type == UNITS)
  {
    int      tz;
    fsec_t    fsec;
    struct pg_tm tt,
           *tm = &tt;

    timetz2tm(time, tm, &fsec, &tz);

    switch (val)
    {
      case DTK_TZ:
        intresult = -tz;
        break;

      case DTK_TZ_MINUTE:
        intresult = (-tz / SECS_PER_MINUTE) % MINS_PER_HOUR;
        break;

      case DTK_TZ_HOUR:
        intresult = -tz / SECS_PER_HOUR;
        break;

      case DTK_MICROSEC:
        intresult = tm->tm_sec * INT64CONST(1000000) + fsec;
        break;

      case DTK_MILLISEC:
        if (retnumeric)
          /*---
           * tm->tm_sec * 1000 + fsec / 1000
           * = (tm->tm_sec * 1'000'000 + fsec) / 1000
           */
          return NumericGetDatum(int64_div_fast_to_numeric(tm->tm_sec * INT64CONST(1000000) + fsec, 3));
        else
          return Float8GetDatum(tm->tm_sec * 1000.0 + fsec / 1000.0);
        break;

      case DTK_SECOND:
        if (retnumeric)
          /*---
           * tm->tm_sec + fsec / 1'000'000
           * = (tm->tm_sec * 1'000'000 + fsec) / 1'000'000
           */
          return NumericGetDatum(int64_div_fast_to_numeric(tm->tm_sec * INT64CONST(1000000) + fsec, 6));
        else
          return Float8GetDatum(tm->tm_sec + fsec / 1000000.0);
        break;

      case DTK_MINUTE:
        intresult = tm->tm_min;
        break;

      case DTK_HOUR:
        intresult = tm->tm_hour;
        break;

      case DTK_DAY:
      case DTK_MONTH:
      case DTK_QUARTER:
      case DTK_YEAR:
      case DTK_DECADE:
      case DTK_CENTURY:
      case DTK_MILLENNIUM:
      default:
        elog(ERROR, "unit \"%s\" not supported for type %s",
          // lowunits, format_type_be(TIMETZOID));
          lowunits, "time with time zone");
        intresult = 0;
    }
  }
  else if (type == RESERV && val == DTK_EPOCH)
  {
    if (retnumeric)
      /*---
       * time->time / 1'000'000 + time->zone
       * = (time->time + time->zone * 1'000'000) / 1'000'000
       */
      return NumericGetDatum(int64_div_fast_to_numeric(time->time + time->zone * INT64CONST(1000000), 6));
    else
      return Float8GetDatum(time->time / 1000000.0 + time->zone);
  }
  else
  {
    elog(ERROR, "unit \"%s\" not recognized for type %s",
      // lowunits, format_type_be(TIMETZOID));
      lowunits, "time with time zone");
    intresult = 0;
  }

  if (retnumeric)
    return NumericGetDatum(int64_to_numeric(intresult));
  else
    return Float8GetDatum(intresult);
}

/**
 * @ingroup meos_base_time
 * @brief Return true if two dates are equal
 * @note Derived from PostgreSQL function @p timetz_part()
 */
#if MEOS
float8
timetz_part(const TimeTzADT *time, const text *units)
{
  return pg_timetz_part(time, units);
}
#endif
float8
pg_timetz_part(const TimeTzADT *time, const text *units)
{
  return DatumGetFloat8(timetz_part_common(units, time, false));
}

/**
 * @ingroup meos_base_time
 * @brief Extract a field from a timetz
 * @note Derived from PostgreSQL function @p extract_timetz()
 */
Numeric
timetz_extract(const TimeTzADT *time, const text *units)
{
  return DatumGetNumeric(timetz_part_common(units, time, true));
}

/**
 * @ingroup meos_base_time
 * @brief Encode a time with time zone with a specified time zone
 * @details Applies DST rules as of the transaction start time
 * @note Derived from PostgreSQL function @p timetz_zone()
 */
#if MEOS
TimeTzADT *
timetz_zone(const TimeTzADT *t, const text *zone)
{
  return pg_timetz_zone(t, zone);
}
#endif
TimeTzADT *
pg_timetz_zone(const TimeTzADT *t, const text *zone)
{
  TimeTzADT *result;
  int tz;
  char tzname[TZ_STRLEN_MAX + 1];
  int type, val;
  pg_tz *tzp;

  /*
   * Look up the requested timezone.
   */
  text_to_cstring_buffer(zone, tzname, sizeof(tzname));

  type = DecodeTimezoneName(tzname, &val, &tzp);

  if (type == TZNAME_FIXED_OFFSET)
  {
    /* fixed-offset abbreviation */
    tz = -val;
  }
  else if (type == TZNAME_DYNTZ)
  {
    /* dynamic-offset abbreviation, resolve using transaction start time */
    TimestampTz now = GetCurrentTimestamp();
    int isdst;
    tz = DetermineTimeZoneAbbrevOffsetTS(now, tzname, tzp, &isdst);
  }
  else
  {
    /* Get the offset-from-GMT that is valid now for the zone name */
    TimestampTz now = GetCurrentTimestamp();
    struct pg_tm tm;
    fsec_t fsec;
    if (timestamp2tm(now, &tz, &tm, &fsec, NULL, tzp) != 0)
    {
      elog(ERROR, "timestamp out of range");
      return DT_NOEND;
    }
  }

  result = (TimeTzADT *) palloc(sizeof(TimeTzADT));
  result->time = t->time + (t->zone - tz) * USECS_PER_SEC;
  /* C99 modulo has the wrong sign convention for negative input */
  while (result->time < INT64CONST(0))
    result->time += USECS_PER_DAY;
  if (result->time >= USECS_PER_DAY)
    result->time %= USECS_PER_DAY;
  result->zone = tz;

  return result;
}

/**
 * @ingroup meos_base_time
 * @brief Encode a time with time zone with a time interval as time zone
 * @return On error return NULL
 * @note Derived from PostgreSQL function @p timetz_izone()
 */
#if MEOS
TimeTzADT *
timetz_izone(const TimeTzADT *time, const Interval *zone)
{
  return pg_timetz_izone(time, zone);
}
#endif
TimeTzADT *
pg_timetz_izone(const TimeTzADT *time, const Interval *zone)
{
  TimeTzADT *result;
  int tz;

  if (INTERVAL_NOT_FINITE(zone))
  {
    elog(ERROR, "interval time zone \"%s\" must be finite", 
      pg_interval_out(zone));
    return NULL;
  }

  if (zone->month != 0 || zone->day != 0)
  {
    elog(ERROR, "interval time zone \"%s\" must not include months or days",
      pg_interval_out(zone));
    return NULL;
  }

  tz = -(zone->time / USECS_PER_SEC);

  result = (TimeTzADT *) palloc(sizeof(TimeTzADT));

  result->time = time->time + (time->zone - tz) * USECS_PER_SEC;
  /* C99 modulo has the wrong sign convention for negative input */
  while (result->time < INT64CONST(0))
    result->time += USECS_PER_DAY;
  if (result->time >= USECS_PER_DAY)
    result->time %= USECS_PER_DAY;

  result->zone = tz;

  return result;
}

/**
 * @ingroup meos_base_time
 * @brief Encode a time with time zone with the local time zone
 * @details Unlike for timestamp[tz]_at_local, the type for timetz does not
 * flip between time with/without time zone, so we cannot just call the
 * conversion function
 * @note Derived from PostgreSQL function @p timetz_at_local()
 */
#if MEOS
TimeTzADT *
timetz_at_local(const TimeTzADT *time)
{
  return pg_timetz_at_local(time);
}
#endif
TimeTzADT *
pg_timetz_at_local(TimeTzADT *time)
{
  const char *tzn = pg_get_timezone_name(session_timezone);
  char *zone = cstring_to_text(tzn);
  return pg_timetz_zone(time, zone);
}

/*****************************************************************************/