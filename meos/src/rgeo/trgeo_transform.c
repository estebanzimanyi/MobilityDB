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

#include "rgeo/trgeo_transform.h"

/* C */
#include <math.h>
#include <float.h>
/* PostGIS */
#include <liblwgeom.h>
/* MEOS */
#include <meos.h>
#include <meos_pose.h>
#include <meos_rgeo.h>

#include "pose/pose.h"
#include "rgeo/trgeo.h"
#include "rgeo/trgeo_parser.h"
#include "rgeo/trgeo_spatialfuncs.h"
#include "rgeo/trgeo_utils.h"

/*****************************************************************************
 * Encoding functions
 *****************************************************************************/

/**
 * @brief Return the transformation for an instant with respect to the
 * reference geometry 
 */
TInstant *
trgeoinst_geo_to_tpose(const TInstant *inst, const GSERIALIZED *gs)
{
  const GSERIALIZED *geom = DatumGetGserializedP(tinstant_value_p(inst));
  Datum pose = PointerGetDatum(geom_compute_pose(gs, geom));
  return tinstant_make_free(pose, inst->t, T_TPOSE);
}

/**
 * @brief Return the transformations for all instants with respect to the
 * reference geometry 
 * @details Raises an error if the geometries are not colinear enough
 * @note Creates a new array of instants, does not free old array
 */
TInstant **
trgeoinstarr_geo_to_tpose(TInstant **instants, int count, 
  const GSERIALIZED *gs)
{
  TInstant **newinsts = palloc(sizeof(TInstant *) * count);;
  for (int i = 0; i < count; ++i)
    newinsts[i] = trgeoinst_geo_to_tpose(instants[i], gs);
  return newinsts;
}

/*****************************************************************************
 * Decoding functions
 *****************************************************************************/

/**
 * @brief 
 */
TInstant *
trgeoinst_pose_to_tgeom(const TInstant *inst, const GSERIALIZED *gs)
{
  const Pose *pose = DatumGetPoseP(tinstant_value_p(inst));
  Datum res = PointerGetDatum(geom_apply_pose(gs, pose));
  return tinstant_make_free(res, inst->t, T_TGEOMETRY);
}

/*****************************************************************************
 * Other transformation functions
 *****************************************************************************/

/**
 * @brief 
 */
static Pose *
pose_make_zero(bool hasz, int32_t srid, bool geodetic)
{
  return hasz ?
    pose_make_3d(0, 0, 0, 1, 0, 0, 0, srid, geodetic) : 
    pose_make_2d(0, 0, 0, srid, geodetic);
}

/**
 * @brief 
 */
TInstant *
trgeoinst_pose_zero(TimestampTz t, bool hasz, int32_t srid, bool geodetic)
{
  Datum pose = PointerGetDatum(pose_make_zero(hasz, srid, geodetic));
  return tinstant_make_free(pose, t, T_TPOSE);
}

/**
 * @brief 
 */
TInstant *
trgeoinst_pose_combine(const TInstant *inst, const TInstant *ref_inst)
{
  const Pose *pose1 = DatumGetPoseP(tinstant_value_p(inst));
  const Pose *pose2 = DatumGetPoseP(tinstant_value_p(ref_inst));
  Datum pose = PointerGetDatum(pose_combine(pose2, pose1));
  return tinstant_make_free(pose, inst->t, inst->temptype);
}

/**
 * @brief 
 */
// TInstant *
// trgeoinst_pose_apply_point(const TInstant *inst,
  // const TInstant *point_inst, const TInstant *centroid_inst)
// {
  // Datum pose = tinstant_value_p(inst);
  // Datum point = tinstant_value_p(point_inst);
  // Datum centroid = PointerGetDatum(NULL);
  // if (centroid_inst)
    // centroid = tinstant_value_p(centroid_inst);
  // Datum result_datum = geom_apply_pose(point, pose, centroid);
  // TInstant *result = tinstant_make(result_datum, inst->t, point_inst->basetypid);
  // pfree((void *) result_datum);
  // return result;
// }

/**
 * @brief 
 */
// TInstant *
// trgeoinst_pose_revert_point(const TInstant *inst,
  // const TInstant *point_inst, const TInstant *centroid_inst)
// {
  // Datum pose = tinstant_value(inst);
  // Datum point = tinstant_value(point_inst);
  // Datum centroid = tinstant_value(centroid_inst);
  // Datum new_centroid = geom_apply_pose(pose, centroid,
    // PointerGetDatum(NULL));
  // Datum rt_inverse = pose_invert_datum(pose, inst->basetypid);
  // Datum old_point = geom_apply_pose(rt_inverse, point,
    // new_centroid, inst->basetypid);
  // TInstant *result = tinstant_make(old_point, centroid_inst->t,
    // point_inst->basetypid);
  // pfree((void *) new_centroid);
  // pfree((void *) rt_inverse);
  // pfree((void *) old_point);
  // return result;
// }

/*****************************************************************************/
