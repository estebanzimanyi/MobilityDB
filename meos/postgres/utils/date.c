/*-------------------------------------------------------------------------
 *
 * date.c
 *	  implements DATE and TIME data types specified in SQL standard
 *
 * Portions Copyright (c) 1996-2025, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994-5, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/utils/adt/date.c
 *
 *-------------------------------------------------------------------------
 */

/* MEOS: This file only contains a few definitions from the original file */

#include "postgres.h"
#include "utils/date.h"
#include "utils/datetime.h"

/*****************************************************************************/

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

/*****************************************************************************/
