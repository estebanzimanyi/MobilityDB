/***********************************************************************
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
 * @brief The liblwgeom entry points reaching GEOS that the rest of liblwgeom
 * calls, for a build carrying no GEOS
 * @details The files of liblwgeom that reach GEOS are left out of a build
 * carrying none, and three of the files that remain still call into them:
 * @p lwkmeans.c seeds its clusters from a centroid, @p lwgeom.c reduces a
 * geometry to a grid through an intersection, and @p lwlinearreferencing.c
 * offsets a curve. Each of those calls would leave a symbol undefined, which a
 * shared library accepts at link time and fails on the first call to, so they
 * are answered here by the error saying the operation needs GEOS.
 *
 * The difference and the unary union complete the set. MEOS answers both from
 * Clipper2 for the polygonal geometries that library covers, and reports the
 * others as not supported before reaching liblwgeom at all, so nothing calls
 * the two definitions below; they are kept so that the set of entry points the
 * left-out files carry is answered as a whole.
 */

/* PostGIS */
#include "liblwgeom.h"
#include "lwgeom_log.h"

/**
 * @brief Report that an operation needs the GEOS library
 */
static LWGEOM *
lwgeom_needs_geos(const char *name)
{
  lwerror("%s: the operation needs the GEOS library, which this build carries "
    "none of", name);
  return NULL;
}

LWGEOM *
lwgeom_centroid(const LWGEOM *geom)
{
  (void) geom;
  return lwgeom_needs_geos(__func__);
}

LWGEOM *
lwgeom_make_valid(LWGEOM *geom)
{
  (void) geom;
  return lwgeom_needs_geos(__func__);
}

LWGEOM *
lwgeom_intersection_prec(const LWGEOM *geom1, const LWGEOM *geom2,
  double gridSize)
{
  (void) geom1; (void) geom2; (void) gridSize;
  return lwgeom_needs_geos(__func__);
}

LWGEOM *
lwgeom_difference_prec(const LWGEOM *geom1, const LWGEOM *geom2,
  double gridSize)
{
  (void) geom1; (void) geom2; (void) gridSize;
  return lwgeom_needs_geos(__func__);
}

LWGEOM *
lwgeom_unaryunion_prec(const LWGEOM *geom1, double gridSize)
{
  (void) geom1; (void) gridSize;
  return lwgeom_needs_geos(__func__);
}

LWGEOM *
lwgeom_offsetcurve(const LWGEOM *geom, double size, int quadsegs, int joinStyle,
  double mitreLimit)
{
  (void) geom; (void) size; (void) quadsegs; (void) joinStyle;
  (void) mitreLimit;
  return lwgeom_needs_geos(__func__);
}

/*****************************************************************************/
