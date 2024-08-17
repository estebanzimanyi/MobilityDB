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
 * @brief Functions for spatiotemporal bounding boxes.
 */

#ifndef __TPOINT_TILE_H__
#define __TPOINT_TILE_H__

/* MEOS */
#include <meos.h>
#include "general/temporal.h"

/*****************************************************************************/

extern BitMatrix *bitmatrix_make(int *count, int ndims);
extern int tpoint_set_tiles(const Temporal *temp, const STboxGridState *state,
  BitMatrix *bm);
extern Temporal *tpoint_at_tile(const Temporal *temp, const STBox *box);

extern void stbox_tile_set(double x, double y, double z, TimestampTz t,
  double xsize, double ysize, double zsize, int64 tunits, bool hasz, bool hast,
  int32 srid, STBox *result);
extern STboxGridState *stbox_tile_state_make(const Temporal *temp,
  const STBox *box, double xsize, double ysize, double zsize, 
  const Interval *duration, POINT3DZ sorigin, TimestampTz torigin, 
  bool border_inc);
extern void stbox_tile_state_next(STboxGridState *state);
extern bool stbox_tile_state_get(STboxGridState *state, STBox *box);

extern STboxGridState *tpoint_space_time_tile_init(const Temporal *temp,
  double xsize, double ysize, double zsize, const Interval *duration,
  const GSERIALIZED *sorigin, TimestampTz torigin, bool bitmatrix, 
  bool border_inc, int *ntiles);

extern STBox *stbox_space_time_tile_common(const GSERIALIZED *point,
  TimestampTz t, double xsize, double ysize, double zsize, 
  const Interval *duration, const GSERIALIZED *sorigin, TimestampTz torigin,
  bool hast);

/*****************************************************************************/

#endif
