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
 * @brief LWGEOM functions that are not handled by PostGIS yet.
 */

#ifndef __LWGEOM_UTILS_H__
#define __LWGEOM_UTILS_H__

#include <postgres.h>
#include <liblwgeom.h>

/*****************************************************************************/

/* Affine Transformations */

extern void lwgeom_affine_transform(LWGEOM *geom, double a, double b, double c,
  double d, double e, double f, double g, double h, double i, double xoff,
  double yoff, double zoff);
extern void lwgeom_rotate_2d(LWGEOM *geom, double a, double b, double c,
  double d);
extern void lwgeom_rotate_3d(LWGEOM *geom, double a, double b, double c,
  double d, double e, double f, double g, double h, double i);
extern void lwgeom_translate_2d(LWGEOM *geom, double x, double y);
extern void lwgeom_translate_3d(LWGEOM *geom, double x, double y, double z);

/* Centroid Functions */

extern LWPOINT *lwpoly_centroid(const LWPOLY *poly);
extern LWPOINT *lwpsurface_centroid(const LWPSURFACE *psurface);

/* Traversed Area Function */

extern LWGEOM *lwgeom_traversed_area(const LWGEOM *geom1, const LWGEOM *geom2);

/* Distance Functions */

extern double lwpoly_max_vertex_distance(const LWPOLY *poly, const LWPOINT *point);
extern double lwpsurface_max_vertex_distance(const LWPSURFACE *psurface, const LWPOINT *point);

/* Rigidity Testing */

extern bool lwgeom_is_rigid(const LWGEOM *geom1, const LWGEOM *geom2);

/*****************************************************************************/

#endif /* __LWGEOM_UTILS_H__ */
