/*****************************************************************************
 *
 * This MobilityDB code is provided under The PostgreSQL License.
 *
 * Copyright (c) 2016-2021, Université libre de Bruxelles and MobilityDB
 * contributors
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
 * @file postgis.h
 * PostGIS definitions that are needed in MobilityDB but are not exported
 * in PostGIS headers
 */

#ifndef __POSTGIS_H__
#define __POSTGIS_H__

#include <fmgr.h>

#define ACCEPT_USE_OF_DEPRECATED_PROJ_API_H 1

/*****************************************************************************/
// Definitions needed for developing geography_line_interpolate_point

/**
* Conversion functions
*/
#define deg2rad(d) (M_PI * (d) / 180.0)
#define rad2deg(r) (180.0 * (r) / M_PI)

/**
* Point in spherical coordinates on the world. Units of radians.
*/
typedef struct
{
  double lon;
  double lat;
} GEOGRAPHIC_POINT;

/**
* Two-point great circle segment from a to b.
*/
typedef struct
{
  GEOGRAPHIC_POINT start;
  GEOGRAPHIC_POINT end;
} GEOGRAPHIC_EDGE;

extern int spheroid_init_from_srid(FunctionCallInfo fcinfo, int srid, SPHEROID *s);
extern double ptarray_length_spheroid(const POINTARRAY *pa, const SPHEROID *s);
extern int lwline_is_empty(const LWLINE *line);
extern void geographic_point_init(double lon, double lat, GEOGRAPHIC_POINT *g);
extern double sphere_distance(const GEOGRAPHIC_POINT *s, const GEOGRAPHIC_POINT *e);
extern void geog2cart(const GEOGRAPHIC_POINT *g, POINT3D *p);
extern void cart2geog(const POINT3D *p, GEOGRAPHIC_POINT *g);
extern void normalize(POINT3D *p);
extern double edge_distance_to_point(const GEOGRAPHIC_EDGE *e,
  const GEOGRAPHIC_POINT *gp, GEOGRAPHIC_POINT *closest);
extern uint32_t edge_intersects(const POINT3D *A1, const POINT3D *A2,
  const POINT3D *B1, const POINT3D *B2);
extern int edge_intersection(const GEOGRAPHIC_EDGE *e1, const GEOGRAPHIC_EDGE *e2,
  GEOGRAPHIC_POINT *g);
extern double edge_distance_to_edge(const GEOGRAPHIC_EDGE *e1, const GEOGRAPHIC_EDGE *e2,
  GEOGRAPHIC_POINT *closest1, GEOGRAPHIC_POINT *closest2);


/**
* Macro for reading the size from the GSERIALIZED size attribute.
* Cribbed from PgSQL, top 30 bits are size. Use VARSIZE() when working
* internally with PgSQL.
*/
#define SIZE_GET(varsize) (((varsize) >> 2) & 0x3FFFFFFF)
#define SIZE_SET(varsize, size) (((varsize) & 0x00000003)|(((size) & 0x3FFFFFFF) << 2 ))

/*
 * This macro is based on PG_FREE_IF_COPY, except that it accepts two pointers.
 * See PG_FREE_IF_COPY comment in src/include/fmgr.h in postgres source code
 * for more details.
 */
#define POSTGIS_FREE_IF_COPY_P(ptrsrc, ptrori) \
  do { \
    if ((Pointer) (ptrsrc) != (Pointer) (ptrori)) \
      pfree(ptrsrc); \
  } while (0)

extern void srid_is_latlong(FunctionCallInfo fcinfo, int srid);
extern int clamp_srid(int srid);
extern int getSRIDbySRS(const char* srs);
extern char *getSRSbySRID(int32_t srid, bool short_crs);
extern int lwprint_double(double d, int maxdd, char* buf, size_t bufsize);
extern char getMachineEndian(void);

extern Datum transform(PG_FUNCTION_ARGS);
extern Datum buffer(PG_FUNCTION_ARGS);
extern Datum centroid(PG_FUNCTION_ARGS);
extern Datum geography_centroid(PG_FUNCTION_ARGS);
extern Datum ST_GeometricMedian(PG_FUNCTION_ARGS);

extern Datum geography_from_geometry(PG_FUNCTION_ARGS);
extern Datum geometry_from_geography(PG_FUNCTION_ARGS);

extern Datum contains(PG_FUNCTION_ARGS);
extern Datum containsproperly(PG_FUNCTION_ARGS);
extern Datum covers(PG_FUNCTION_ARGS);
extern Datum coveredby(PG_FUNCTION_ARGS);
extern Datum crosses(PG_FUNCTION_ARGS);
extern Datum disjoint(PG_FUNCTION_ARGS);
extern Datum ST_Equals(PG_FUNCTION_ARGS);
extern Datum intersects(PG_FUNCTION_ARGS); /* For 2D */
extern Datum intersects3d(PG_FUNCTION_ARGS); /* For 3D */
extern Datum overlaps(PG_FUNCTION_ARGS);
extern Datum touches(PG_FUNCTION_ARGS);
extern Datum within(PG_FUNCTION_ARGS);
extern Datum relate_full(PG_FUNCTION_ARGS);
extern Datum relate_pattern(PG_FUNCTION_ARGS);
extern Datum geomunion(PG_FUNCTION_ARGS);
extern Datum ST_Scale(PG_FUNCTION_ARGS);
extern Datum ST_Snap(PG_FUNCTION_ARGS);
extern Datum ST_UnaryUnion(PG_FUNCTION_ARGS);

extern Datum intersection(PG_FUNCTION_ARGS);
extern Datum difference(PG_FUNCTION_ARGS);
extern Datum distance(PG_FUNCTION_ARGS); /* For 2D */
extern Datum distance3d(PG_FUNCTION_ARGS); /* For 3D */
extern Datum issimple(PG_FUNCTION_ARGS);

extern Datum pgis_union_geometry_array(PG_FUNCTION_ARGS);
extern Datum linemerge(PG_FUNCTION_ARGS);

extern Datum LWGEOM_addpoint(PG_FUNCTION_ARGS);
extern Datum LWGEOM_asText(PG_FUNCTION_ARGS); /* also for geography */
extern Datum LWGEOM_asEWKT(PG_FUNCTION_ARGS); /* also for geography */
extern Datum LWGEOM_azimuth(PG_FUNCTION_ARGS);
extern Datum LWGEOM_closestpoint(PG_FUNCTION_ARGS); /* For 2D */
extern Datum LWGEOM_closestpoint3d(PG_FUNCTION_ARGS); /* For 3D */
extern Datum LWGEOM_collect(PG_FUNCTION_ARGS);
extern Datum LWGEOM_collect_garray(PG_FUNCTION_ARGS);
extern Datum LWGEOM_dfullywithin(PG_FUNCTION_ARGS);
extern Datum LWGEOM_dwithin(PG_FUNCTION_ARGS); /* For 2D */
extern Datum LWGEOM_dwithin3d(PG_FUNCTION_ARGS); /* For 3D */
extern Datum LWGEOM_expand(PG_FUNCTION_ARGS);
extern Datum LWGEOM_exteriorring_polygon(PG_FUNCTION_ARGS);
extern Datum LWGEOM_geometryn_collection(PG_FUNCTION_ARGS);
extern Datum LWGEOM_get_srid(PG_FUNCTION_ARGS);  /* also for geography */
extern Datum LWGEOM_set_srid(PG_FUNCTION_ARGS);  /* also for geography */
extern Datum LWGEOM_getTYPE(PG_FUNCTION_ARGS); /* also for geography */
extern Datum LWGEOM_isempty(PG_FUNCTION_ARGS);
extern Datum LWGEOM_length_linestring(PG_FUNCTION_ARGS);
extern Datum LWGEOM_line_locate_point(PG_FUNCTION_ARGS);
extern Datum LWGEOM_line_interpolate_point(PG_FUNCTION_ARGS);
extern Datum LWGEOM_line_substring(PG_FUNCTION_ARGS);
extern Datum LWGEOM_makepoint(PG_FUNCTION_ARGS);
extern Datum LWGEOM_npoints(PG_FUNCTION_ARGS);
extern Datum LWGEOM_numgeometries_collection(PG_FUNCTION_ARGS);
extern Datum LWGEOM_numpoints_linestring(PG_FUNCTION_ARGS);
extern Datum LWGEOM_pointn_linestring(PG_FUNCTION_ARGS);
extern Datum LWGEOM_setpoint_linestring(PG_FUNCTION_ARGS);
extern Datum LWGEOM_shortestline2d(PG_FUNCTION_ARGS); /* For 2D */
extern Datum LWGEOM_shortestline3d(PG_FUNCTION_ARGS); /* For 3D */
extern Datum LWGEOM_reverse(PG_FUNCTION_ARGS);

extern Datum lwgeom_lt(PG_FUNCTION_ARGS);
extern Datum lwgeom_le(PG_FUNCTION_ARGS);
extern Datum lwgeom_gt(PG_FUNCTION_ARGS);
extern Datum lwgeom_ge(PG_FUNCTION_ARGS);
extern Datum lwgeom_eq(PG_FUNCTION_ARGS);
extern Datum lwgeom_cmp(PG_FUNCTION_ARGS);
extern Datum lwgeom_hash(PG_FUNCTION_ARGS);

extern Datum geography_covers(PG_FUNCTION_ARGS);
extern Datum geography_project(PG_FUNCTION_ARGS);
extern Datum geography_length(PG_FUNCTION_ARGS);
extern Datum geography_expand(PG_FUNCTION_ARGS);
extern Datum geography_dwithin(PG_FUNCTION_ARGS);
extern Datum geography_distance(PG_FUNCTION_ARGS);
extern Datum geography_azimuth(PG_FUNCTION_ARGS);
extern Datum geography_bestsrid(PG_FUNCTION_ARGS);

extern Datum geography_eq(PG_FUNCTION_ARGS);
extern Datum geography_lt(PG_FUNCTION_ARGS);
extern Datum geography_le(PG_FUNCTION_ARGS);
extern Datum geography_gt(PG_FUNCTION_ARGS);
extern Datum geography_ge(PG_FUNCTION_ARGS);
extern Datum geography_eq(PG_FUNCTION_ARGS);
extern Datum geography_cmp(PG_FUNCTION_ARGS);

extern Datum gserialized_same_2d(PG_FUNCTION_ARGS);
extern Datum gserialized_within_2d(PG_FUNCTION_ARGS);
extern Datum gserialized_contains_2d(PG_FUNCTION_ARGS);
extern Datum gserialized_overlaps_2d(PG_FUNCTION_ARGS);
extern Datum gserialized_left_2d(PG_FUNCTION_ARGS);
extern Datum gserialized_right_2d(PG_FUNCTION_ARGS);
extern Datum gserialized_above_2d(PG_FUNCTION_ARGS);
extern Datum gserialized_below_2d(PG_FUNCTION_ARGS);
extern Datum gserialized_overleft_2d(PG_FUNCTION_ARGS);
extern Datum gserialized_overright_2d(PG_FUNCTION_ARGS);
extern Datum gserialized_overabove_2d(PG_FUNCTION_ARGS);
extern Datum gserialized_overbelow_2d(PG_FUNCTION_ARGS);
extern Datum gserialized_distance_box_2d(PG_FUNCTION_ARGS);

extern Datum gserialized_analyze_nd(PG_FUNCTION_ARGS);

#define PG_GETARG_GSERIALIZED_P(varno) ((GSERIALIZED *)PG_DETOAST_DATUM(PG_GETARG_DATUM(varno)))

/*****************************************************************************/

/* Definitions from liblwgeom.h */

/**
* Return types for functions with status returns.
*/
#define LW_TRUE 1
#define LW_FALSE 0
#define LW_UNKNOWN 2
#define LW_FAILURE 0
#define LW_SUCCESS 1

/**
* LWTYPE numbers, used internally by PostGIS
*/
#define  POINTTYPE                1
#define  LINETYPE                 2
#define  MULTIPOINTTYPE           4
#define  MULTILINETYPE            5
#define  COLLECTIONTYPE           7

/**
* Macros for manipulating the 'flags' byte. A uint8_t used as follows:
* VVSRGBMZ
* Version bit, followed by
* Validty, Solid, ReadOnly, Geodetic, HasBBox, HasM and HasZ flags.
*/
#define FLAGS_GET_Z(flags) ((flags) & 0x01)
#define FLAGS_GET_M(flags) (((flags) & 0x02)>>1)
#define FLAGS_GET_BBOX(flags) (((flags) & 0x04)>>2)
#define FLAGS_GET_GEODETIC(flags) (((flags) & 0x08)>>3)
#define FLAGS_GET_READONLY(flags) (((flags) & 0x10)>>4)
#define FLAGS_GET_SOLID(flags) (((flags) & 0x20)>>5)
#define FLAGS_SET_Z(flags, value) ((flags) = (value) ? ((flags) | 0x01) : ((flags) & 0xFE))
#define FLAGS_SET_M(flags, value) ((flags) = (value) ? ((flags) | 0x02) : ((flags) & 0xFD))
#define FLAGS_SET_BBOX(flags, value) ((flags) = (value) ? ((flags) | 0x04) : ((flags) & 0xFB))
#define FLAGS_SET_GEODETIC(flags, value) ((flags) = (value) ? ((flags) | 0x08) : ((flags) & 0xF7))
#define FLAGS_SET_READONLY(flags, value) ((flags) = (value) ? ((flags) | 0x10) : ((flags) & 0xEF))
#define FLAGS_SET_SOLID(flags, value) ((flags) = (value) ? ((flags) | 0x20) : ((flags) & 0xDF))
#define FLAGS_NDIMS(flags) (2 + FLAGS_GET_Z(flags) + FLAGS_GET_M(flags))
#define FLAGS_GET_ZM(flags) (FLAGS_GET_M(flags) + FLAGS_GET_Z(flags) * 2)
#define FLAGS_NDIMS_BOX(flags) (FLAGS_GET_GEODETIC(flags) ? 3 : FLAGS_NDIMS(flags))

/**
* Macros for manipulating the 'typemod' int. An int32_t used as follows:
* Plus/minus = Top bit.
* Spare bits = Next 2 bits.
* SRID = Next 21 bits.
* TYPE = Next 6 bits.
* ZM Flags = Bottom 2 bits.
*/

#define TYPMOD_GET_SRID(typmod) ((((typmod) & 0x0FFFFF00) - ((typmod) & 0x10000000)) >> 8)
#define TYPMOD_SET_SRID(typmod, srid) ((typmod) = (((typmod) & 0xE00000FF) | ((srid & 0x001FFFFF)<<8)))
#define TYPMOD_GET_TYPE(typmod) ((typmod & 0x000000FC)>>2)
#define TYPMOD_SET_TYPE(typmod, type) ((typmod) = (typmod & 0xFFFFFF03) | ((type & 0x0000003F)<<2))
#define TYPMOD_GET_Z(typmod) ((typmod & 0x00000002)>>1)
#define TYPMOD_SET_Z(typmod) ((typmod) = typmod | 0x00000002)
#define TYPMOD_GET_M(typmod) (typmod & 0x00000001)
#define TYPMOD_SET_M(typmod) ((typmod) = typmod | 0x00000001)
#define TYPMOD_GET_NDIMS(typmod) (2+TYPMOD_GET_Z(typmod)+TYPMOD_GET_M(typmod))

/** Unknown SRID value */
#define SRID_UNKNOWN 0

/*
** EPSG WGS84 geographics, OGC standard default SRS, better be in
** the SPATIAL_REF_SYS table!
*/
#define SRID_DEFAULT 4326

/*
** Variants available for WKB and WKT output types
*/

#define WKT_ISO 0x01
#define WKT_EXTENDED 0x04

/**
 * Global functions for memory/logging handlers.
 */
typedef void* (*lwallocator)(size_t size);
typedef void* (*lwreallocator)(void *mem, size_t size);
typedef void (*lwfreeor)(void* mem);
typedef void (*lwreporter)(const char* fmt, va_list ap)
  __attribute__ (( format(printf, 1, 0) ));

/**
* Install custom memory management and error handling functions you want your
* application to use.
* @ingroup system
* @todo take a structure ?
*/
extern void lwgeom_set_handlers(lwallocator allocator,
        lwreallocator reallocator, lwfreeor freeor, lwreporter errorreporter,
        lwreporter noticereporter);

/******************************************************************/

typedef struct
{
  double xmin, ymin, zmin;
  double xmax, ymax, zmax;
  int32_t srid;
}
  BOX3D;

/******************************************************************
* GBOX structure.
* We include the flags (information about dimensionality),
* so we don't have to constantly pass them
* into functions that use the GBOX.
*/
typedef struct
{
  uint8_t flags;
  double xmin;
  double xmax;
  double ymin;
  double ymax;
  double zmin;
  double zmax;
  double mmin;
  double mmax;
} GBOX;


/******************************************************************
* POINT2D, POINT3D, POINT3DM, POINT4D
*/
typedef struct
{
  double x, y;
}
POINT2D;

typedef struct
{
  double x, y, z;
}
POINT3DZ;

typedef struct
{
  double x, y, z;
}
POINT3D;

typedef struct
{
  double x, y, m;
}
POINT3DM;

typedef struct
{
  double x, y, z, m;
}
POINT4D;

/******************************************************************
*  POINTARRAY
*  Point array abstracts a lot of the complexity of points and point lists.
*  It handles 2d/3d translation
*    (2d points converted to 3d will have z=0 or NaN)
*  DO NOT MIX 2D and 3D POINTS! EVERYTHING* is either one or the other
*/
typedef struct
{
  /* Array of POINT 2D, 3D or 4D, possibly misaligned. */
  uint8_t *serialized_pointlist;

  /* Use FLAGS_* macros to handle */
  uint8_t  flags;

  uint32_t npoints;   /* how many points we are currently storing */
  uint32_t maxpoints; /* how many points we have space for in serialized_pointlist */
}
POINTARRAY;

/******************************************************************
* GSERIALIZED
*/
typedef struct
{
  uint32_t size; /* For PgSQL use only, use VAR* macros to manipulate. */
  uint8_t srid[3]; /* 24 bits of SRID */
  uint8_t flags; /* HasZ, HasM, HasBBox, IsGeodetic, IsReadOnly */
  uint8_t data[1]; /* See gserialized.txt */
} GSERIALIZED;

/******************************************************************
* LWGEOM (any geometry type)
*
* Abstract type, note that 'type', 'bbox' and 'srid' are available in
* all geometry variants.
*/
typedef struct
{
  uint8_t type;
  uint8_t flags;
  GBOX *bbox;
  int32_t srid;
  void *data;
}
LWGEOM;

/* POINTYPE */
typedef struct
{
  uint8_t type; /* POINTTYPE */
  uint8_t flags;
  GBOX *bbox;
  int32_t srid;
  POINTARRAY *point;  /* hide 2d/3d (this will be an array of 1 point) */
}
LWPOINT; /* "light-weight point" */

/* LINETYPE */
typedef struct
{
  uint8_t type; /* LINETYPE */
  uint8_t flags;
  GBOX *bbox;
  int32_t srid;
  POINTARRAY *points; /* array of POINT3D */
}
LWLINE; /* "light-weight line" */

/* COLLECTIONTYPE */
typedef struct
{
  uint8_t type;
  uint8_t flags;
  GBOX *bbox;
  int32_t srid;
  uint32_t ngeoms;   /* how many geometries we are currently storing */
  uint32_t maxgeoms; /* how many geometries we have space for in **geoms */
  LWGEOM **geoms;
}
LWCOLLECTION;

/**********************************************************************
** Spherical radius.
** Moritz, H. (1980). Geodetic Reference System 1980, by resolution of
** the XVII General Assembly of the IUGG in Canberra.
** http://en.wikipedia.org/wiki/Earth_radius
** http://en.wikipedia.org/wiki/World_Geodetic_System
*/

#define WGS84_MAJOR_AXIS 6378137.0
#define WGS84_INVERSE_FLATTENING 298.257223563
#define WGS84_MINOR_AXIS (WGS84_MAJOR_AXIS - WGS84_MAJOR_AXIS / WGS84_INVERSE_FLATTENING)
#define WGS84_RADIUS ((2.0 * WGS84_MAJOR_AXIS + WGS84_MINOR_AXIS ) / 3.0)
#define WGS84_SRID 4326

/******************************************************************
* SPHEROID
*
*  Standard definition of an ellipsoid (what wkt calls a spheroid)
*    f = (a-b)/a
*    e_sq = (a*a - b*b)/(a*a)
*    b = a - fa
*/
typedef struct
{
    double  a;  /* semimajor axis */
    double  b;  /* semiminor axis b = (a - fa) */
    double  f;  /* flattening f = (a-b)/a */
    double  e;  /* eccentricity (first) */
    double  e_sq;   /* eccentricity squared (first) e_sq = (a*a-b*b)/(a*a) */
    double  radius;  /* spherical average radius = (2*a+b)/3 */
    char    name[20];  /* name of ellipse */
}
SPHEROID;


/* Casts LWGEOM->LW* (return NULL if cast is illegal) */

extern LWLINE *lwgeom_as_lwline(const LWGEOM *lwgeom);
extern LWPOINT *lwgeom_as_lwpoint(const LWGEOM *lwgeom);
extern LWCOLLECTION *lwgeom_as_lwcollection(const LWGEOM *lwgeom);

/* Casts LW*->LWGEOM (always cast) */
extern LWGEOM *lwline_as_lwgeom(const LWLINE *obj);
extern LWGEOM *lwpoint_as_lwgeom(const LWPOINT *obj);

/**
* @param lwgeom geometry to convert to WKT
* @param variant output format to use (WKT_ISO, WKT_SFSQL, WKT_EXTENDED)
*/
extern char *lwgeom_to_wkt(const LWGEOM *geom, uint8_t variant, int precision, size_t *size_out);

/**
* Return the type name string associated with a type number
* (e.g. Point, LineString, Polygon)
*/
extern const char *lwtype_name(uint8_t type);

/******************************************************************/

/*
 * copies a point from the point array into the parameter point
 * will set point's z=0 (or NaN) if pa is 2d
 * will set point's m=0 (or NaN) if pa is 3d or 2d
 * NOTE: point is a real POINT3D *not* a pointer
 */
extern POINT4D getPoint4d(const POINTARRAY *pa, uint32_t n);

/*
 * copies a point from the point array into the parameter point
 * will set point's z=0 (or NaN) if pa is 2d
 * NOTE: point is a real POINT3D *not* a pointer
 */
extern POINT3DZ getPoint3dz(const POINTARRAY *pa, uint32_t n);
extern POINT3DM getPoint3dm(const POINTARRAY *pa, uint32_t n);

extern LWPOINT* lwline_get_lwpoint(const LWLINE *line, uint32_t where);

extern int lwpoint_getPoint4d_p(const LWPOINT *point, POINT4D *out);

/*
* Geometry constructors. These constructors to not copy the point arrays
* passed to them, they just take references, so do not free them out
* from underneath the geometries.
*/

extern LWCOLLECTION* lwcollection_construct(uint8_t type, int srid, GBOX *bbox, uint32_t ngeoms, LWGEOM **geoms);

/* Other constructors */
extern LWPOINT *lwpoint_construct_empty(int srid, char hasz, char hasm);
extern LWPOINT *lwpoint_make2d(int srid, double x, double y);
extern LWPOINT *lwpoint_make3dz(int srid, double x, double y, double z);
extern LWPOINT *lwpoint_make3dm(int srid, double x, double y, double m);
extern LWPOINT *lwpoint_make4d(int srid, double x, double y, double z, double m);
extern LWLINE *lwline_construct_empty(int srid, char hasz, char hasm);
extern LWLINE *lwline_from_lwgeom_array(int srid, uint32_t ngeoms, LWGEOM **geoms);
extern LWLINE *lwline_from_ptarray(int srid, uint32_t npoints, LWPOINT **points); /* TODO: deprecate */

/**
* Return LW_TRUE or LW_FALSE depending on whether or not a geometry is
* a linestring with measure value growing from start to end vertex
*/
extern int lwgeom_is_trajectory(const LWGEOM *geom);

/*
* The *_free family of functions frees *all* memory associated
* with the pointer. When the recursion gets to the level of the
* POINTARRAY, the POINTARRAY is only freed if it is not flagged
* as "read only". LWGEOMs constructed on top of GSERIALIZED
* from PgSQL use read only point arrays.
*/

extern void lwgeom_free(LWGEOM *geom);
extern void lwpoint_free(LWPOINT *pt);
extern void lwline_free(LWLINE *line);

/***********************************************************************
** Utility functions for flag byte and srid_flag integer.
*/

/**
* Construct a new flags char.
*/
extern uint8_t gflags(int hasz, int hasm, int geodetic);

/**
* Extract the geometry type from the serialized form (it hides in
* the anonymous data area, so this is a handy function).
*/
extern uint32_t gserialized_get_type(const GSERIALIZED *g);

/**
* Allocate a new #GSERIALIZED from an #LWGEOM. For all non-point types, a bounding
* box will be calculated and embedded in the serialization. The geodetic flag is used
* to control the box calculation (cartesian or geocentric). If set, the size pointer
* will contain the size of the final output, which is useful for setting the PgSQL
* VARSIZE information.
*/
extern GSERIALIZED* gserialized_from_lwgeom(LWGEOM *geom, size_t *size);

/**
* Allocate a new #LWGEOM from a #GSERIALIZED. The resulting #LWGEOM will have coordinates
* that are double aligned and suitable for direct reading using getPoint2d_p_ro
*/
extern LWGEOM* lwgeom_from_gserialized(const GSERIALIZED *g);

/**
* Pull a #GBOX from the header of a #GSERIALIZED, if one is available. If
* it is not, calculate it from the geometry. If that doesn't work (null
* or empty) return LW_FAILURE.
*/
extern int gserialized_get_gbox_p(const GSERIALIZED *g, GBOX *gbox);

/**
* Check if a #GSERIALIZED is empty without deserializing first.
* Only checks if the number of elements of the parent geometry
* is zero, will not catch collections of empty, eg:
* GEOMETRYCOLLECTION(POINT EMPTY)
*/
extern int gserialized_is_empty(const GSERIALIZED *g);

/**
* Extract the SRID from the serialized form (it is packed into
* three bytes so this is a handy function).
*/
extern int32_t gserialized_get_srid(const GSERIALIZED *g);

/**
* Write the SRID into the serialized form (it is packed into
* three bytes so this is a handy function).
*/
extern void gserialized_set_srid(GSERIALIZED *g, int32_t srid);

/**
* Return a copy of the input serialized geometry.
*/
extern GSERIALIZED* gserialized_copy(const GSERIALIZED *g);

/**
* Create a new gbox with the dimensionality indicated by the flags. Caller
* is responsible for freeing.
*/
extern GBOX* gbox_new(uint8_t flags);

/**
* Return a copy of the #GBOX, based on dimensionality of flags.
*/
extern GBOX* gbox_copy(const GBOX *gbox);

/**
* Return false if any of the dimensions is NaN or infinite
*/
extern int gbox_is_valid(const GBOX *gbox);

/**
* Update the merged #GBOX to be large enough to include itself and the new box.
*/
extern int gbox_merge(const GBOX *new_box, GBOX *merged_box);

/**
* Utility function to get type number from string. For example, a string 'POINTZ'
* would return type of 1 and z of 1 and m of 0. Valid
*/
extern int geometry_type_from_string(const char *str, uint8_t *type, int *z, int *m);

/*
 * copies a point from the point array into the parameter point
 * z value (if present is not returned)
 * NOTE: point is a real POINT3D *not* a pointer
 */
extern POINT2D getPoint2d(const POINTARRAY *pa, uint32_t n);

extern int azimuth_pt_pt(const POINT2D *p1, const POINT2D *p2, double *ret);

extern POINTARRAY* ptarray_construct(char hasz, char hasm, uint32_t npoints);
extern void gserialized_error_if_srid_mismatch(const GSERIALIZED *g1, const GSERIALIZED *g2, const char *funcname);

#endif /* __TEMPORAL_POSTGIS_H__ */

/*****************************************************************************/
