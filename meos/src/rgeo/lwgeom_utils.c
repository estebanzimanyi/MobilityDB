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
 * @brief LWGEOM functions that are not provided by PostGIS yet
 */

#include "rgeo/lwgeom_utils.h"

/* C */
#include <assert.h>
#include <math.h>
#include <float.h>
/* PostGIS */
#include <liblwgeom_internal.h>
/* MEOS */
#include "temporal/doublen.h"
#include "temporal/temporal.h"
#include "rgeo/planar_graph.h"

/*****************************************************************************
 * Affine Transformations
 *****************************************************************************/

/**
 * @brief Apply an affine transform to an LWGEOM
 * @param[in] geom Geometry
 * @param[in] a,b,c,d,e,f,g,h,i Values defining the rotation
 * @param[in] xoff,yoff,zoff Values defining the translation
 */
void
lwgeom_affine_transform(LWGEOM *geom, double a, double b, double c, double d,
  double e, double f, double g, double h, double i, double xoff, double yoff,
  double zoff)
{
  AFFINE affine;
  affine.afac =  a;
  affine.bfac =  b;
  affine.cfac =  c;
  affine.dfac =  d;
  affine.efac =  e;
  affine.ffac =  f;
  affine.gfac =  g;
  affine.hfac =  h;
  affine.ifac =  i;
  affine.xoff =  xoff;
  affine.yoff =  yoff;
  affine.zoff =  zoff;
  lwgeom_affine(geom, &affine);
  return;
}

/**
 * @brief Apply an rotation to a 2D LWGEOM
 * @param[in] geom Geometry
 * @param[in] a,b,c,d Values defining the rotation
 */
void
lwgeom_rotate_2d(LWGEOM *geom, double a, double b, double c, double d)
{
  lwgeom_affine_transform(geom,
    a, b, 0,
    c, d, 0,
    0, 0, 1,
    0, 0, 0);
  return;
}

/**
 * @brief Apply an rotation to a 3D LWGEOM
 * @param[in] geom Geometry
 * @param[in] a,b,c,d,e,f,g,h,i Values defining the rotation
 */
void
lwgeom_rotate_3d(LWGEOM *geom, double a, double b, double c, double d,
  double e, double f, double g, double h, double i)
{
  lwgeom_affine_transform(geom,
    a, b, c,
    d, e, f,
    g, h, i,
    0, 0, 0);
  return;
}

/**
 * @brief Apply a translation to a 2D LWGEOM
 * @param[in] geom Geometry
 * @param[in] x,y Values defining the translation
 */
void
lwgeom_translate_2d(LWGEOM *geom, double x, double y)
{
  lwgeom_affine_transform(geom,
    1, 0, 0,
    0, 1, 0,
    0, 0, 1,
    x, y, 0);
  return;
}

/**
 * @brief Apply a translation to a 3D LWGEOM
 * @param[in] geom Geometry
 * @param[in] x,y,z Values defining the translation
 */
void
lwgeom_translate_3d(LWGEOM *geom, double x, double y, double z)
{
  lwgeom_affine_transform(geom,
    1, 0, 0,
    0, 1, 0,
    0, 0, 1,
    x, y, z);
  return;
}

/*****************************************************************************
 * Traversed Area Function
 *****************************************************************************/

/**
 * @brief Return the traversed area of two geometries 
 * @param[in] geom1,geom2 Geometries
 */
LWGEOM *
lwgeom_traversed_area(const LWGEOM *geom1, const LWGEOM *geom2)
{
  const LWPOLY *poly1 = (const LWPOLY *) geom1;
  const LWPOLY *poly2 = (const LWPOLY *) geom2;
  uint32_t n = poly1->rings[0]->npoints - 1;

  /* Create array of segments */
  Segment *segments = palloc(sizeof(Segment) * n * 3);
  for (uint32_t i = 0; i < n; ++i)
  {
    POINT4D start1_p = getPoint4d(poly1->rings[0], i);
    POINT4D end1_p = getPoint4d(poly1->rings[0], i + 1);
    POINT4D start2_p = getPoint4d(poly2->rings[0], i);
    POINT4D end2_p = getPoint4d(poly2->rings[0], i + 1);
    double2 start1 = (double2) {start1_p.x, start1_p.y};
    double2 end1 = (double2) {end1_p.x, end1_p.y};
    double2 start2 = (double2) {start2_p.x, start2_p.y};
    double2 end2 = (double2) {end2_p.x, end2_p.y};
    segments[3 * i] = make_segment(start1, end1);
    segments[3 * i + 1] = make_segment(start2, end2);
    segments[3 * i + 2] = make_segment(start1, start2);
  }

  /* Create graph from segments and compute the result */
  Graph g;
  init_graph(&g, 3 * n);
  for (uint32_t i = 0; i < 3 * n; ++i)
    add_segment_to_graph(&g, segments[i]);
  POINTARRAY *poly_point_arr = get_cycle_from_graph(&g);
  free_graph(&g);
  LWPOLY *result = lwpoly_construct_empty(poly1->srid, false, false);
  lwpoly_add_ring(result, poly_point_arr);
  return (LWGEOM *) result;
}

/*****************************************************************************
 * Distance Functions
 *****************************************************************************/

/**
 * @brief Return the maximum vertex distance between a polygon and a point
 * @param[in] poly Polygon
 * @param[in] point Point
 */
double
lwpoly_max_vertex_distance(const LWPOLY *poly, const LWPOINT *point)
{
  POINT4D p;
  double d = 0;
  double x = lwpoint_get_x(point);
  double y = lwpoint_get_y(point);
  for (uint32_t i = 0; i < poly->rings[0]->npoints; ++i)
  {
    getPoint4d_p(poly->rings[0], i, &p);
    d = Max(d, sqrt(pow(x - p.x, 2) + pow(y - p.y, 2)));
  }
  return d;
}

/**
 * @brief Return the maximum vertex distance between a polyhedral surface and a
 * point
 * @param[in] psurface Polyhedral surface
 * @param[in] point Point
 */
double
lwpsurface_max_vertex_distance(const LWPSURFACE *psurface,
  const LWPOINT *point)
{
  POINT4D p;
  double d = 0;
  double x = lwpoint_get_x(point);
  double y = lwpoint_get_y(point);
  double z = lwpoint_get_z(point);
  for (uint32_t i = 0; i < psurface->ngeoms; ++i)
  {
    for (uint32_t j = 0; j < psurface->geoms[i]->rings[0]->npoints - 1; ++j)
    {
      getPoint4d_p(psurface->geoms[i]->rings[0], j, &p);
      d = Max(d, sqrt(pow(x - p.x, 2) + pow(y - p.y, 2) + pow(z - p.z, 2)));
    }
  }
  return d;
}

/*****************************************************************************
 * Rigidity Testing
 *****************************************************************************/

/**
 * @brief Return true if two geometries are the same wrt to an epsilon value 
 * @param[in] geom1,geom2 Geometries
 */
bool
lwgeom_is_rigid(const LWGEOM *geom1, const LWGEOM *geom2)
{
  LWPOINTITERATOR *it1 = lwpointiterator_create(geom1);
  LWPOINTITERATOR *it2 = lwpointiterator_create(geom2);
  POINT4D p1;
  POINT4D p2;

  bool result = true;
  while (lwpointiterator_next(it1, &p1) && lwpointiterator_next(it2, &p2) &&
          result)
  {
    /* TODO: make sure this works for large point values too */
    if (FLAGS_GET_Z(geom1->flags))
    {
      result = fabs(p1.x - p2.x) < MEOS_EPSILON &&
        fabs(p1.y - p2.y) < MEOS_EPSILON && fabs(p1.z - p2.z) < MEOS_EPSILON;
    }
    else
    {
      result = fabs(p1.x - p2.x) < MEOS_EPSILON &&
        fabs(p1.y - p2.y) < MEOS_EPSILON;
    }
  }
  lwpointiterator_destroy(it1);
  lwpointiterator_destroy(it2);
  return result;
}

/*****************************************************************************
 * Centroid Functions
 *****************************************************************************/

/**
 * @brief Return the centroid of a polygon
 * @param[in] poly Polygon
 */
LWPOINT *
lwpoly_centroid(const LWPOLY *poly)
{
  return lwgeom_as_lwpoint(lwgeom_centroid(lwpoly_as_lwgeom(poly)));
}

/**
 * @brief Return the centroid of a polyhedral surface
 * @param[in] psurf Polyhedral surface
 */
/* TODO: Maybe define it better and ask for support from postgis */
LWPOINT *
lwpsurface_centroid(const LWPSURFACE *psurf)
{
  double x = 0, y = 0, z = 0;
  double tot = 0;
  for (uint32_t i = 0; i < psurf->ngeoms; ++i)
  {
    for (uint32_t j = 0; j < psurf->geoms[i]->nrings; ++j)
    {
      for (uint32_t k = 0; k < psurf->geoms[i]->rings[j]->npoints - 1; ++k)
      {
        POINT4D p = getPoint4d(psurf->geoms[i]->rings[j], k);
        x += p.x;
        y += p.y;
        z += p.z;
        ++tot;
      }
    }
  }
  return lwpoint_make3dz(psurf->srid, x / tot, y / tot, z / tot);
}

/*****************************************************************************/
