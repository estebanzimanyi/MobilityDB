/*****************************************************************************
 * PostgreSQL Globals 
 *****************************************************************************/

/* File /meos/postgres/pg_config.h */

/* Define to 1 if you have the global variable 'int timezone'. */
#define HAVE_INT_TIMEZONE 1

/*****************************************************************************/

/* File /meos/postgres/timezone/findtimezone.c */

/*
 * Get GMT offset from a system struct tm
 */
static int
get_timezone_offset(struct tm *tm)
{
#if defined(HAVE_STRUCT_TM_TM_ZONE)
  return tm->tm_gmtoff;
#elif defined(HAVE_INT_TIMEZONE)
  return -TIMEZONE_GLOBAL;
#else
#error No way to determine TZ? Can this happen?
#endif
}

/*****************************************************************************/

/* File /meos/postgres/timezone/localtime.c */

/*
 * Section 4.12.3 of X3.159-1989 requires that
 *	Except for the strftime function, these functions [asctime,
 *	ctime, gmtime, localtime] return values in one of two static
 *	objects: a broken-down time structure and an array of char.
 * Thanks to Paul Eggert for noting this.
 */

static struct pg_tm tm;

/*****************************************************************************/

/* File /meos/postgres/utils/formatting.c */

typedef struct
{
  FormatNode  format[DCH_CACHE_SIZE + 1];
  char    str[DCH_CACHE_SIZE + 1];
  bool    std;
  bool    valid;
  int      age;
} DCHCacheEntry;


/* global cache for date/time format pictures */
static DCHCacheEntry *DCHCache[DCH_CACHE_ENTRIES];
static int  n_DCHCache = 0;    /* current number of entries */
static int  DCHCounter = 0;    /* aging-event counter */

/*****************************************************************************
 * PostGIS Globals 
 *****************************************************************************/

/* File /postgis/libpgcommon/lwgeom_transform.c */

/* Global to hold the Proj object cache */
PROJSRSCache *PROJ_CACHE = NULL;

/*****************************************************************************/

/* File /postgis/liblwgeom/lwgeodetic.c */

/**
* For testing geodetic bounding box, we have a magic global variable.
* When this is true (when the cunit tests set it), use the slow, but
* guaranteed correct, algorithm. Otherwise use the regular one.
*/
int gbox_geocentric_slow = LW_FALSE;

/*****************************************************************************/

/* File /postgis/liblwgeom/lwin_wkt_parse.c */

/* Declare the global parser variable */
LWGEOM_PARSER_RESULT global_parser_result;

/* Turn on/off verbose parsing (turn off for production) */
int wkt_yydebug = 0;

/*****************************************************************************/

/* File /postgis/liblwgeom/lwutils.c */

/* Default allocators */
lwallocator lwalloc_var = default_allocator;
lwreallocator lwrealloc_var = default_reallocator;
lwfreeor lwfree_var = default_freeor;

/* Default reporters */
lwreporter lwnotice_var = default_noticereporter;
lwreporter lwerror_var = default_errorreporter;

/* Default logger */

lwdebuglogger lwdebug_var = default_debuglogger;

/*****************************************************************************/

/* Global to hold all the run-time constants */
extern postgisConstants *POSTGIS_CONSTANTS;

/*****************************************************************************/

