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
 * @brief Transformation (encoder/decoder) functions for temporal geometries
 */

#ifndef __TRGEO_TRANSFORM_H__
#define __TRGEO_TRANSFORM_H__

#include <postgres.h>
#include <meos.h>

#include "rgeo/trgeo.h"

/*****************************************************************************
 * Encoding functions
 *****************************************************************************/

extern TInstant *trgeoinst_geo_to_tpose(const TInstant *inst, const GSERIALIZED *gs);

extern TInstant **trgeoinstarr_geo_to_tpose(TInstant **instants, int count, 
  const GSERIALIZED *gs);

/*****************************************************************************
 * Decoding functions
 *****************************************************************************/

extern TInstant *trgeoinst_pose_to_point(const TInstant *inst, const GSERIALIZED *gs);

/*****************************************************************************
 * Other transformation functions
 *****************************************************************************/

extern TInstant *trgeoinst_pose_zero(TimestampTz t, bool hasz, int32_t srid, bool geodetic);
extern TInstant *trgeoinst_pose_combine(const TInstant *inst, const TInstant *ref_pose);
extern TInstant *trgeoinst_pose_apply_point(const TInstant *inst, const TInstant *point_inst, const TInstant *centroid_inst);
extern TInstant *trgeoinst_pose_revert_point(const TInstant *inst, const TInstant *point_inst, const TInstant *centroid_inst);

/*****************************************************************************/

#endif /* __TRGEO_TRANSFORM_H__ */
