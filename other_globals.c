/*****************************************************************************
 * PostgreSQL Globals 
 *****************************************************************************/

/* File /meos/postgres/pg_config.h */

/* Define to 1 if you have the global variable 'int opterr'. */
#define HAVE_INT_OPTERR 1

/* Define to 1 if you have the global variable 'int optreset'. */
/* #undef HAVE_INT_OPTRESET */

/* Define to 1 if you have the global variable 'int timezone'. */
#define HAVE_INT_TIMEZONE 1

/* Define to 1 if you have the global variable
   'rl_completion_append_character'. */
#define HAVE_RL_COMPLETION_APPEND_CHARACTER 1

/* Define to 1 if you have the global variable 'rl_completion_suppress_quote'.
   */
#define HAVE_RL_COMPLETION_SUPPRESS_QUOTE 1

/* Define to 1 if you have the global variable 'rl_filename_quote_characters'.
   */
#define HAVE_RL_FILENAME_QUOTE_CHARACTERS 1

/* Define to 1 if you have the global variable 'rl_filename_quoting_function'.
   */
#define HAVE_RL_FILENAME_QUOTING_FUNCTION 1

/*****************************************************************************/

/* File /postgres/utils/datetime.h */

/* Definitions of the global variables taken from miscadmin.h */
extern int DateStyle;
extern int DateOrder;
extern int IntervalStyle;

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

/* File /postgis/liblwgeom/liblwgeom.h */

/**
 * Global functions for memory/logging handlers.
 */
typedef void* (*lwallocator)(size_t size);
typedef void* (*lwreallocator)(void *mem, size_t size);
typedef void (*lwfreeor)(void* mem);
typedef void (*lwreporter)(const char* fmt, va_list ap)
  __attribute__ (( format(printf, 1, 0) ));
typedef void (*lwdebuglogger)(int level, const char* fmt, va_list ap)
  __attribute__ (( format(printf, 2,0) ));

/*****************************************************************************/

/* File /postgis/liblwgeom/lwout_twkb.h */

typedef struct
{
	/* Options defined at start */
	uint8_t variant;
	int8_t prec_xy;
	int8_t prec_z;
	int8_t prec_m;
	float factor[4]; /*What factor to multiply the coordinates with to get the requested precision*/
} TWKB_GLOBALS;

/*****************************************************************************/

/* File /postgis/liblwgeom/lwgeom_transform.h */

/*
* Proj4 caching has it's own mechanism, and is
* stored globally as the cost of proj_create_crs_to_crs()
* is so high (20-40ms) that the lifetime of fcinfo->flinfo->fn_extra
* is too short to assist some work loads.
*/

/* An entry in the PROJ SRS cache */
typedef struct struct_PROJSRSCacheItem
{
	int32_t srid_from;
	int32_t srid_to;
	uint64_t hits;
	LWPROJ *projection;
}
PROJSRSCacheItem;

/* PROJ 4 lookup transaction cache methods */
#define PROJ_CACHE_ITEMS 128

/*
* The proj4 cache holds a fixed number of reprojection
* entries. In normal usage we don't expect it to have
* many entries, so we always linearly scan the list.
*/
typedef struct struct_PROJSRSCache
{
	PROJSRSCacheItem PROJSRSCache[PROJ_CACHE_ITEMS];
	uint32_t PROJSRSCacheCount;
	MemoryContext PROJSRSCacheContext;
}
PROJSRSCache;

/*****************************************************************************/

/* File /postgis/liblwgeom/lwin_wkt.h */

/*
* Global that holds the final output geometry for the WKT parser.
*/
extern LWGEOM_PARSER_RESULT global_parser_result;

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
static void * default_allocator(size_t size);
static void default_freeor(void *mem);
static void * default_reallocator(void *mem, size_t size);
lwallocator lwalloc_var = default_allocator;
lwreallocator lwrealloc_var = default_reallocator;
lwfreeor lwfree_var = default_freeor;

/* Default reporters */
static void default_noticereporter(const char *fmt, va_list ap) __attribute__ ((format (printf, 1, 0)));
static void default_errorreporter(const char *fmt, va_list ap) __attribute__ ((format (printf, 1, 0)));
lwreporter lwnotice_var = default_noticereporter;
lwreporter lwerror_var = default_errorreporter;

/* Default logger */
static void default_debuglogger(int level, const char *fmt, va_list ap) __attribute__ ((format (printf, 2, 0)));
lwdebuglogger lwdebug_var = default_debuglogger;

#define LW_MSG_MAXLEN 256

static char *lwgeomTypeName[] = {...}

/* Structure for the type array */
struct geomtype_struct
{
	char *typename;
	int type;
	int z;
	int m;
};

/* Type array. Note that the order of this array is important in
   that any typename in the list must *NOT* occur within an entry
   before it. Otherwise if we search for "POINT" at the top of the
   list we would also match MULTIPOINT, for example. */

struct geomtype_struct geomtype_struct_array[] = {...}

/*****************************************************************************/

/* File /postgis/libpgcommon/lwgeom_pg.h */

typedef enum
{
	GEOMETRYOID = 1,
	GEOGRAPHYOID,
	BOX3DOID,
	BOX2DFOID,
	GIDXOID,
	RASTEROID,
	POSTGISNSPOID
} postgisType;

typedef struct
{
	Oid geometry_oid;
	Oid geography_oid;
	Oid box2df_oid;
	Oid box3d_oid;
	Oid gidx_oid;
	Oid raster_oid;
	Oid install_nsp_oid;
	char *install_nsp;
	char *spatial_ref_sys;
} postgisConstants;

/* Global to hold all the run-time constants */
extern postgisConstants *POSTGIS_CONSTANTS;

/*****************************************************************************/

/* File /postgis/libpgcommon/lwgeom_transform.c */

/* Global to hold the Proj object cache */
PROJSRSCache *PROJ_CACHE = NULL;

/*****************************************************************************/

