/*****************************************************************************
 *
 * trgeo_transform.h
 *    Transformation (encoder/decoder) functions for temporal geometries.
 *
 * Portions Copyright (c) 2019, Maxime Schoemans, Esteban Zimanyi,
 *    Universite Libre de Bruxelles
 * Portions Copyright (c) 1996-2019, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *****************************************************************************/

#ifndef __TRGEO_TRANSFORM_H__
#define __TRGEO_TRANSFORM_H__

#include <postgres.h>
#include <meos.h>

#include "rgeo/trgeo.h"

/*****************************************************************************
 * Encoding functions
 *****************************************************************************/

extern TInstant *trgeoinst_geom_to_tpose(const TInstant *inst, const GSERIALIZED *gs);

extern TInstant **trgeoinstarr_geom_to_tpose(TInstant **instants, int count, 
  const GSERIALIZED *gs);

/*****************************************************************************
 * Decoding functions
 *****************************************************************************/

extern TInstant *trgeoinst_pose_to_geometry(const TInstant *inst, const GSERIALIZED *gs);

/*****************************************************************************
 * Other transformation functions
 *****************************************************************************/

extern TInstant *trgeoinst_pose_zero(TimestampTz t, bool hasz, int32_t srid);
extern TInstant *trgeoinst_pose_combine(const TInstant *inst, const TInstant *ref_pose);
extern TInstant *trgeoinst_pose_apply_point(const TInstant *inst, const TInstant *point_inst, const TInstant *centroid_inst);
extern TInstant *trgeoinst_pose_revert_point(const TInstant *inst, const TInstant *point_inst, const TInstant *centroid_inst);

/*****************************************************************************/

#endif /* __TRGEO_TRANSFORM_H__ */
