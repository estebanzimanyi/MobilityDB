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
 * @brief Ever/always spatial relationships for temporal poses
 * @details These relationships compute the ever/always spatial relationship
 * between the arguments and return a Boolean. These functions may be used for
 * filtering purposes before applying the corresponding spatiotemporal
 * relationship.
 *
 * The following relationships are supported: `econtains`, `acontains`, 
 * `ecovers`, `acovers`, `edisjoint`, `adisjoint`, `eintersects`, 
 * `aintersects`, `etouches`, atouches`,  `edwithin`, and `adwithin`.
 */

#include "pose/tpose_spatialrels.h"

/* C */
#include <assert.h>
/* MEOS */
#include <meos.h>
#include <meos_internal_geo.h>
#include "temporal/lifting.h"
#include "temporal/type_util.h"
#include "geo/tgeo_spatialfuncs.h"
#include "geo/tgeo_spatialrels.h"
#include "pose/pose.h"
#include "pose/tpose.h"
#include "pose/tpose_spatialfuncs.h"

/*****************************************************************************
 * Some GEOS versions cannot handle spatial relationships where one of the
 * arguments is a collection. For example, the calls to the `eIntersects`
 * function below translate to calls to the GEOSIntersects function with the
 * traversed areas and GEOS returns an error when the traversed area is a
 * collection.
 * @code
  SELECT eIntersects(pose 'Pose(Point(1 1),0.5)', tpose
    '[Pose(Point(1 1),0.5)@2000-01-01, Pose(Point(2 2),0.5)@2000-01-02,
      Pose(Point(1 1),0.5)@2000-01-03]');
  ERROR:  GEOS returned error
  SELECT eIntersects(pose 'Pose(Point(1 1),0.5)', tpose
    '[Pose(Point(1 1),0.5)@2000-01-01, Pose(Point(2 2),0.5)@2000-01-02]');
  -- t
  SELECT eIntersects(pose 'Pose(Point(1 1),0.5)', tpose
    '[Pose(Point(2 2),0.5)@2000-01-02, Pose(Point(1 1),0.5)@2000-01-03]');
  -- t
 * @endcode
 * Therefore, we need to iterate and apply the spatial relationship to each
 * element of the collection(s)
 *****************************************************************************/

/**
 * @brief Return 1 if two geometries satisfy a spatial relationship
 * @param[in] gs1,gs2 Geometry
 * @param[in] param Parameter
 * @param[in] func PostGIS function to be called
 * @param[in] numparam Number of parameters of the function
 * @param[in] invert True if the arguments should be inverted
 * @return On error return -1
 * @pre None of the two geometries is a geometry collection
 */
static int
spatialrel_geo_geo_simple(const GSERIALIZED *gs1, const GSERIALIZED *gs2,
  Datum param, varfunc func, int numparam, bool invert)
{  /* Call the GEOS function when the geometries are not collections */
  assert(geo_is_unitary(gs1)); assert(geo_is_unitary(gs2));
  Datum geo1 = PointerGetDatum(gs1);
  Datum geo2 = PointerGetDatum(gs2);
  bool res;
  if (numparam == 2)
  {
    datum_func2 func2 = (datum_func2) func;
    res = invert ? func2(geo2, geo1) : func2(geo1, geo2);
  }
  else /* numparam == 3 */
  {
    datum_func3 func3 = (datum_func3) func;
    res = invert ? func3(geo2, geo1, param) : func3(geo1, geo2, param);
  }
  return res ? 1 : 0;
}

/**
 * @brief Return 1 if two geometries satisfy a spatial relationship
 * @details The function iterates for every member of a collection if one or
 * the two geometries are collections
 * @param[in] gs1,gs2 Geometry
 * @param[in] param Parameter
 * @param[in] func PostGIS function to be called
 * @param[in] numparam Number of parameters of the functions
 * @param[in] invert True if the arguments should be inverted
 * @return On error return -1
 */
int
spatialrel_geo_geo(const GSERIALIZED *gs1, const GSERIALIZED *gs2,
  Datum param, varfunc func, int numparam, bool invert)
{
  /* Extract the elements of the arguments, if they are collections */
  int count1, count2;
  GSERIALIZED **elems1 = geo_extract_elements(gs1, &count1);
  GSERIALIZED **elems2 = geo_extract_elements(gs2, &count2);
  /* Perform the iterations for the elements in the collections if any */
  int result = 0;
  for (int i = 0; i < count1; i++)
  {
    for (int j = 0; j < count2; j++)
    {
      result = spatialrel_geo_geo_simple(elems1[i], elems2[j], param, func,
        numparam, invert);
      if (result)
        break;
    }
    if (result)
      break;
  }
  /* Clean up and return */
  pfree_array((void *) elems1, count1);
  pfree_array((void *) elems2, count2);
  return result;
}

/*****************************************************************************
 * Generic ever/always spatial relationship functions
 * Functions that verify the relationship with the traversed area of EACH
 * SEGMENT of the temporal pose
 *****************************************************************************/

/**
 * @brief Return 1 if a temporal pose instant and a geometry 
 * ever/always satisfy a spatial relationship
 * @param[in] inst Temporal instant
 * @param[in] gs Geometry
 * @param[in] param Optional parameter
 * @param[in] func Spatial relationship function to be applied
 * @param[in] numparam Number of parameters of the function
 * @param[in] invert True when the arguments of the function must be inverted
 * @note The `ever` parameter is not used since the result is the same for the
 * `ever` and the `always` semantics
 */
int
ea_spatialrel_tposeinst_geo(const TInstant *inst, const GSERIALIZED *gs,
  Datum param, varfunc func, int numparam, bool invert)
{
  assert(inst); assert(gs); assert(inst->temptype == T_TCBUFFER);
  const Pose *pose = DatumGetPoseP(tinstant_value_p(inst));
  GSERIALIZED *trav = pose_to_point(pose);
  int result = spatialrel_geo_geo(trav, gs, param, func, numparam, invert);
  pfree(trav);
  return result ? 1 : 0;
}

/**
 * @brief Return 1 if a temporal pose sequence with discrete or 
 * step interpolation and a geometry ever/always satisfy a spatial relationship
 * @param[in] seq Temporal sequence
 * @param[in] gs Geometry
 * @param[in] param Optional parameter
 * @param[in] func Spatial relationship function to be applied
 * @param[in] numparam Number of parameters of the function
 * @param[in] ever True for the ever semantics, false for the always semantics
 * @param[in] invert True when the arguments of the function must be inverted
 */
int
ea_spatialrel_tposeseq_discstep_geo(const TSequence *seq,
  const GSERIALIZED *gs, Datum param, varfunc func, int numparam,
  bool ever, bool invert)
{
  assert(seq); assert(gs); assert(seq->temptype == T_TCBUFFER);
  interpType interp = MEOS_FLAGS_GET_INTERP(seq->flags);
  assert(interp == DISCRETE || interp == STEP);
  bool result;
  for (int i = 0; i < seq->count; i++)
  {
    const TInstant *inst = TSEQUENCE_INST_N(seq, i);
    const Pose *pose = DatumGetPoseP(tinstant_value_p(inst));
    GSERIALIZED *trav = pose_to_point(pose);
    result = spatialrel_geo_geo(trav, gs, param, func, numparam, invert);
    pfree(trav);
    if (result && ever)
      return 1;
    else if (! result && ! ever)
      return 0;
  }
  return ever ? 0 : 1;
}

/**
 * @brief Return 1 if a temporal pose sequence with linear interpolation and a
 * geometry ever/always satisfy a spatial relationship
 * @param[in] seq Temporal sequence
 * @param[in] gs Geometry
 * @param[in] param Optional parameter
 * @param[in] func Spatial relationship function to be applied
 * @param[in] numparam Number of parameters of the function
 * @param[in] ever True for the ever semantics, false for the always semantics
 * @param[in] invert True when the arguments of the function must be inverted
 */
int
ea_spatialrel_tposeseq_linear_geo(const TSequence *seq, const GSERIALIZED *gs,
  Datum param, varfunc func, int numparam, bool ever, bool invert)
{
  assert(seq); assert(gs); assert(seq->temptype == T_TCBUFFER);
  assert(MEOS_FLAGS_LINEAR_INTERP(seq->flags));

  /* Instantaneous sequence */
  if (seq->count == 1)
    return ea_spatialrel_tposeinst_geo(TSEQUENCE_INST_N(seq, 0), gs,
      param, func, numparam, invert);

  /* General case */
  const TInstant *inst1 = TSEQUENCE_INST_N(seq, 0);
  int result;
  for (int i = 1; i < seq->count; i++)
  {
    const TInstant *inst2 = TSEQUENCE_INST_N(seq, i);
    GSERIALIZED *trav = tposesegm_traversed_area(inst1, inst2);
    result = spatialrel_geo_geo(trav, gs, param, func, numparam, invert);
    pfree(trav);
    if (result == 1 && ever)
      return 1;
    else if (result != 1 && ! ever)
      return 0;
  }
  return ever ? 0 : 1;
}

/**
 * @brief Return 1 if a temporal pose sequence and a geometry
 * ever/always satisfy a spatial relationship (dispatch function)
 * @param[in] seq Temporal sequence
 * @param[in] gs Geometry
 * @param[in] param Optional parameter
 * @param[in] func Spatial relationship function to be applied
 * @param[in] numparam Number of parameters of the function
 * @param[in] ever True for the ever semantics, false for the always semantics
 * @param[in] invert True when the arguments of the function must be inverted
 */
int
ea_spatialrel_tposeseq_geo(const TSequence *seq, const GSERIALIZED *gs,
  Datum param, varfunc func, int numparam, bool ever, bool invert)
{
  assert(seq); assert(gs); assert(seq->temptype == T_TCBUFFER);
  return MEOS_FLAGS_LINEAR_INTERP(seq->flags) ?
    ea_spatialrel_tposeseq_linear_geo(seq, gs, param, func, numparam,
      ever, invert) :
    ea_spatialrel_tposeseq_discstep_geo(seq, gs, param, func, numparam,
      ever, invert);
}

/**
 * @brief Return 1 if a temporal pose sequence set and a geometry
 * ever/always satisfy a spatial relationship
 * @param[in] ss Temporal sequence set
 * @param[in] gs Geometry
 * @param[in] param Optional parameter
 * @param[in] func Spatial relationship function to be applied
 * @param[in] numparam Number of parameters of the function
 * @param[in] ever True for the ever semantics, false for the always semantics
 * @param[in] invert True when the arguments of the function must be inverted
 */
int
ea_spatialrel_tposeseqset_geo(const TSequenceSet *ss, const GSERIALIZED *gs,
  Datum param, varfunc func, int numparam, bool ever, bool invert)
{
  /* Singleton sequence set */
  if (ss->count == 1)
    return ea_spatialrel_tposeseq_geo(TSEQUENCESET_SEQ_N(ss, 0), gs, 
      param, func, numparam, ever, invert);

  int result;
  for (int i = 0; i < ss->count; i++)
  {
    result = ea_spatialrel_tposeseq_geo(TSEQUENCESET_SEQ_N(ss, i), gs,
      param, func, numparam, ever, invert);
    if (result == 1 && ever)
      return 1;
    else if (result != 1 && ! ever)
      return 0;
}
  return ever ? 0 : 1;
}

/**
 * @brief Return true if a temporal pose and a geometry ever/always
 * satisfy a spatial relationship
 * @details The function computes the traversed area of each segment and
 * verifies that the traversed area and the geometry satisfy the relationship
 * @param[in] temp Temporal geo
 * @param[in] gs Geometry
 * @param[in] param Optional parameter
 * @param[in] func Spatial relationship function to be applied
 * @param[in] numparam Number of parameters of the function
 * @param[in] ever True for the ever semantics, false for the always semantics
 * @param[in] invert True when the arguments of the function must be inverted
 */
int
ea_spatialrel_tpose_geo(const Temporal *temp, const GSERIALIZED *gs,
  Datum param, varfunc func, int numparam, bool ever, bool invert)
{
  VALIDATE_TPOSE(temp, -1); VALIDATE_NOT_NULL(gs, -1);
  /* Ensure the validity of the arguments */
  if (! ensure_valid_tpose_geo(temp, gs) || gserialized_is_empty(gs))
    return -1;

  /* Bounding box test */ 
  if (func != (varfunc) &datum_geom_disjoint2d)
  {
    STBox box1, box2;
    tspatial_set_stbox(temp, &box1);
    /* Non-empty geometries have a bounding box */
    geo_set_stbox(gs, &box2);
    if (! overlaps_stbox_stbox(&box1, &box2))
      return 0;
  }

  assert(temptype_subtype(temp->subtype));
  switch (temp->subtype)
  {
    case TINSTANT:
      /* The result is the same for the `ever` and the `always` semantics */
      return ea_spatialrel_tposeinst_geo((TInstant *) temp, gs, param,
        func, numparam, invert);
    case TSEQUENCE:
      return ea_spatialrel_tposeseq_geo((TSequence *) temp, gs, param,
        func, numparam, ever, invert);
    default: /* TSEQUENCESET */
      return ea_spatialrel_tposeseqset_geo((TSequenceSet *) temp, gs,
        param, func, numparam, ever, invert);
  }
}

/*****************************************************************************/

/**
 * @brief Return true if a temporal pose and a pose
 * ever/always satisfy a spatial relationship
 * @param[in] temp Temporal geo
 * @param[in] pose Pose
 * @param[in] param Optional parameter
 * @param[in] func Spatial relationship function to be applied
 * @param[in] numparam Number of parameters of the function
 * @param[in] ever True for the ever semantics, false for the always semantics
 * @param[in] invert True when the arguments of the function must be inverted
 */
int
ea_spatialrel_tpose_pose(const Temporal *temp, const Pose *pose,
  Datum param, varfunc func, int numparam, bool ever, bool invert)
{
  VALIDATE_TPOSE(temp, -1); VALIDATE_NOT_NULL(pose, -1);
  /* Ensure the validity of the arguments */
  if (! ensure_valid_tpose_pose(temp, pose))
    return -1;
  GSERIALIZED *gs = pose_to_point(pose);
  int result = ea_spatialrel_tpose_geo(temp, gs, param, func, numparam,
    ever, invert);
  pfree(gs);
  return result;
}
  
/*****************************************************************************
 * Generic spatial relationship
 *****************************************************************************/

/**
 * @ingroup meos_internal_pose_rel_ever
 * @brief Return 1 if two temporal poses ever/always satisfy a
 * spatial relationship, 0 if not, and -1 on error
 * @param[in] temp1,temp2 Temporal poses
 * @param[in] func Spatial relationship function to be called
 * @param[in] ever True for the ever semantics, false for the always semantics
 * @param[in] bbox_test True if a bounding text can be used for filtering
 * @csqlfn #Aintersects_tpose_tpose(), #Ecovers_tpose_tpose(), ...
 */
int
ea_spatialrel_tpose_tpose(const Temporal *temp1, const Temporal *temp2,
  datum_func2 func, bool ever, bool bbox_test)
{
  VALIDATE_TPOSE(temp1, -1); VALIDATE_TPOSE(temp2, -1);
  /* Ensure the validity of the arguments */
  if (! ensure_valid_tpose_tpose(temp1, temp2))
    return -1;

  /* Bounding box test */
  if (bbox_test)
  {
    STBox box1, box2;
    tspatial_set_stbox(temp1, &box1);
    tspatial_set_stbox(temp2, &box2);
    if (! overlaps_stbox_stbox(&box1, &box2))
      return 0;
  }

  return ea_spatialrel_tspatial_tspatial(temp1, temp2, func, ever);
}

/*****************************************************************************
 * Ever/always contains
 *****************************************************************************/

/**
 * @brief Return 1 if a geometry ever/always contains a temporal circular
 * buffer, 0 if not, and -1 on error or if the geometry is empty
 * @param[in] gs Geometry
 * @param[in] temp Temporal pose
 * @param[in] ever True for the ever semantics, false for the always semantics
 * @csqlfn #Acontains_geo_tpose()
 * @note The function is not supported for the `ever` semantics
 */
int
ea_contains_geo_tpose(const GSERIALIZED *gs, const Temporal *temp,
  bool ever)
{
  /* This function is not provided for the ever semantics */
  assert(! ever);
  return ea_spatialrel_tpose_geo(temp, gs, (Datum) NULL,
      (varfunc) &datum_geom_contains, 2, ever, INVERT);
}

/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if a geometry always contains a temporal pose,
 * 0 if not, and -1 on error or if the geometry is empty
 * @param[in] gs Geometry
 * @param[in] temp Temporal pose
 * @note The function tests whether the traversed area is contained in the
 * geometry
 * @csqlfn #Acontains_geo_tpose()
 */
inline int
acontains_geo_tpose(const GSERIALIZED *gs, const Temporal *temp)
{
  return ea_contains_geo_tpose(gs, temp, ALWAYS);
}

/*****************************************************************************/

/**
 * @brief Return 1 if a pose ever contains a temporal circular
 * buffer, 0 if not, and -1 on error
 * @param[in] pose Pose
 * @param[in] temp Temporal pose
 * @param[in] ever True for the ever semantics, false for the always semantics
 */
int
ea_contains_pose_tpose(const Pose *pose, const Temporal *temp,
  bool ever)
{
  const char p[] = "T********";
  int result = ever ?
    ea_spatialrel_tpose_pose(temp, pose, PointerGetDatum(p),
      (varfunc) &datum_geom_relate_pattern, 3, ever, INVERT) :
    ea_spatialrel_tpose_pose(temp, pose, (Datum) NULL,
      (varfunc) &datum_geom_contains, 2, ever, INVERT);
  return result;
}

/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if a pose ever contains a temporal circular
 * buffer, 0 if not, and -1 on error
 * @param[in] pose Pose
 * @param[in] temp Temporal pose
 * @csqlfn #Econtains_pose_tpose()
 */
inline int
econtains_pose_tpose(const Pose *pose, const Temporal *temp)
{
  return ea_contains_pose_tpose(pose, temp, EVER);
}

/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if a pose always contains a temporal circular
 * buffer, 0 if not, and -1 on error
 * @param[in] pose Pose
 * @param[in] temp Temporal pose
 * @csqlfn #Acontains_pose_tpose()
 */
inline int
acontains_pose_tpose(const Pose *pose, const Temporal *temp)
{
  return ea_contains_pose_tpose(pose, temp, ALWAYS);
}

/*****************************************************************************/

/**
 * @brief Return 1 if a temporal pose ever/always contains a
 * pose, 0 if not, and -1 on error
 * @param[in] temp Temporal pose
 * @param[in] pose Pose
 * @param[in] ever True for the ever semantics, false for the always semantics
 */
int
ea_contains_tpose_pose(const Temporal *temp, const Pose *pose,
  bool ever)
{
  const char p[] = "T********";
  int result = ever ?
    ea_spatialrel_tpose_pose(temp, pose, PointerGetDatum(p),
      (varfunc) &datum_geom_relate_pattern, 3, ever, INVERT_NO) :
    ea_spatialrel_tpose_pose(temp, pose, (Datum) NULL,
      (varfunc) &datum_geom_contains, 2, ever, INVERT_NO);
  return result;
}

/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if a temporal pose ever contains a circular
 * buffer, 0 if not, and -1 on error
 * @param[in] temp Temporal pose
 * @param[in] pose Pose
 * @csqlfn #Econtains_tpose_pose()
 */
inline int
econtains_tpose_pose(const Temporal *temp, const Pose *pose)
{
  return ea_contains_tpose_pose(temp, pose, EVER);
}

/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if a temporal pose always contains a circular
 * buffer, 0 if not, and -1 on error
 * @param[in] temp Temporal pose
 * @param[in] pose Pose
 * @csqlfn #Acontains_tpose_pose()
 */
inline int
acontains_tpose_pose(const Temporal *temp, const Pose *pose)
{
  return ea_contains_tpose_pose(temp, pose, ALWAYS);
}

/*****************************************************************************
 * Ever/always covers
 *****************************************************************************/

/**
 * @brief Return 1 if a geometry ever/always covers a temporal pose
 * 0 if not, and -1 on error or if the geometry is empty
 * @param[in] gs Geometry
 * @param[in] temp Temporal pose
 * @param[in] ever True for the ever semantics, false for the always semantics
 * @note The function is not supported for the `ever` semantics
 * @csqlfn #Acovers_geo_tpose()
 */
int
ea_covers_geo_tpose(const GSERIALIZED *gs, const Temporal *temp, bool ever)
{
  /* This function is not provided for the ever semantics */
  assert(! ever);
  return ea_spatialrel_tpose_geo(temp, gs, (Datum) NULL,
      (varfunc) &datum_geom_covers, 2, ever, INVERT);
}

/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if a geometry always covers a temporal pose,
 * 0 if not, and -1 on error or if the geometry is empty
 * @param[in] gs Geometry
 * @param[in] temp Temporal pose
 * @note The function tests whether the traversed area is covered in the
 * geometry
 * @csqlfn #Acovers_geo_tpose()
 */
inline int
acovers_geo_tpose(const GSERIALIZED *gs, const Temporal *temp)
{
  return ea_covers_geo_tpose(gs, temp, ALWAYS);
}

/*****************************************************************************/

/**
 * @brief Return 1 if a temporal pose ever/always covers a
 * geometry, 0 if not, and -1 on error or if the geometry is empty
 * @param[in] temp Temporal pose
 * @param[in] gs Geometry
 * @param[in] ever True for the ever semantics, false for the always semantics
 * @note The function tests whether the traversed area intersects the interior
 * of the geometry.
 * @csqlfn #Ecovers_tpose_geo(), #Acovers_tpose_geo()
 */
int
ea_covers_tpose_geo(const Temporal *temp, const GSERIALIZED *gs, bool ever)
{
  return ea_spatialrel_tpose_geo(temp, gs, (Datum) NULL, 
    (varfunc) &datum_geom_covers, 2, ever, INVERT_NO);
}

/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if a temporal pose ever covers a geometry,
 * 0 if not, and -1 on error or if the geometry is empty
 * @param[in] temp Temporal pose
 * @param[in] gs Geometry
 * @note The function tests whether the traversed area is covered in the
 * geometry
 * @csqlfn #Ecovers_tpose_geo()
 */
inline int
ecovers_tpose_geo(const Temporal *temp, const GSERIALIZED *gs)
{
  return ea_covers_tpose_geo(temp, gs, EVER);
}

/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if a temporal pose always covers a geometry,
 * 0 if not, and -1 on error or if the geometry is empty
 * @param[in] temp Temporal pose
 * @param[in] gs Geometry
 * @note The function tests whether the traversed area is covered in the
 * geometry
 * @csqlfn #Acovers_tpose_geo()
 */
inline int
acovers_tpose_geo(const Temporal *temp, const GSERIALIZED *gs)
{
  return ea_covers_tpose_geo(temp, gs, ALWAYS);
}

/*****************************************************************************/

/**
 * @brief Return 1 if a pose ever covers a temporal circular
 * buffer, 0 if not, and -1 on error
 * @param[in] pose Pose
 * @param[in] temp Temporal pose
 * @param[in] ever True for the ever semantics, false for the always semantics
 */
int
ea_covers_pose_tpose(const Pose *pose, const Temporal *temp,
  bool ever)
{
  return ea_spatialrel_tpose_pose(temp, pose, (Datum) NULL,
    (varfunc) &datum_geom_covers, 2, ever, INVERT);
}

/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if a pose ever covers a temporal circular
 * buffer, 0 if not, and -1 on error
 * @param[in] pose Pose
 * @param[in] temp Temporal pose
 * @csqlfn #Ecovers_pose_tpose()
 */
inline int
ecovers_pose_tpose(const Pose *pose, const Temporal *temp)
{
  return ea_covers_pose_tpose(pose, temp, EVER);
}

/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if a pose always covers a temporal circular
 * buffer, 0 if not, and -1 on error
 * @param[in] pose Pose
 * @param[in] temp Temporal pose
 * @csqlfn #Acovers_pose_tpose()
 */
inline int
acovers_pose_tpose(const Pose *pose, const Temporal *temp)
{
  return ea_covers_pose_tpose(pose, temp, ALWAYS);
}

/*****************************************************************************/

/**
 * @brief Return 1 if a temporal pose ever/always covers a
 * pose, 0 if not, and -1 on error
 * @param[in] temp Temporal pose
 * @param[in] pose Pose
 * @param[in] ever True for the ever semantics, false for the always semantics
 */
int
ea_covers_tpose_pose(const Temporal *temp, const Pose *pose,
  bool ever)
{
  return ea_spatialrel_tpose_pose(temp, pose, (Datum) NULL,
    (varfunc) &datum_geom_covers, 2, ever, INVERT_NO);
}

/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if a temporal pose ever covers a circular
 * buffer, 0 if not, and -1 on error
 * @param[in] temp Temporal pose
 * @param[in] pose Pose
 * @csqlfn #Ecovers_tpose_pose()
 */
inline int
ecovers_tpose_pose(const Temporal *temp, const Pose *pose)
{
  return ea_covers_tpose_pose(temp, pose, EVER);
}

/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if a temporal pose always covers a circular
 * buffer, 0 if not, and -1 on error
 * @param[in] temp Temporal pose
 * @param[in] pose Pose
 * @csqlfn #Acovers_tpose_pose()
 */
inline int
acovers_tpose_pose(const Temporal *temp, const Pose *pose)
{
  return ea_covers_tpose_pose(temp, pose, ALWAYS);
}

/*****************************************************************************/

/**
 * @ingroup meos_internal_pose_rel_ever
 * @brief Return 1 if a temporal pose ever/always covers another
 * one, 0 if not, and -1 on error
 * @param[in] temp1,temp2 Temporal poses
 * @param[in] ever True for the ever semantics, false for the always semantics
 * @csqlfn #Ecovers_tpose_tpose(), #Acovers_tpose_tpose()
 */
int
ea_covers_tpose_tpose(const Temporal *temp1, const Temporal *temp2,
  bool ever)
{
  return ea_spatialrel_tpose_tpose(temp1, temp2, &datum_pose_covers,
    ever, true);
}

/**
 * @ingroup meos_geo_rel_ever
 * @brief Return 1 if a temporal pose ever covers another one, 0 if not,
 * and -1 on error
 * @param[in] temp1,temp2 Temporal geos
 * @csqlfn #Ecovers_tpose_tpose()
 */
int
ecovers_tpose_tpose(const Temporal *temp1, const Temporal *temp2)
{
  return ea_covers_tpose_tpose(temp1, temp2, EVER);
}

/**
 * @ingroup meos_geo_rel_ever
 * @brief Return 1 if a temporal pose always covers another one, 0 if not,
 * and -1 on error
 * @param[in] temp1,temp2 Temporal geos
 * @csqlfn #Acovers_tpose_tpose()
 */
int
acovers_tpose_tpose(const Temporal *temp1, const Temporal *temp2)
{
  return ea_covers_tpose_tpose(temp1, temp2, EVER);
}

/*****************************************************************************
 * Ever/always disjoint
 *****************************************************************************/

/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if a temporal pose and a geometry are ever
 * disjoint,0 if not, and -1 on error or if the geometry is empty
 * @param[in] temp Temporal pose
 * @param[in] gs Geometry
 * @param[in] ever True for the ever semantics, false for the always semantics
 * @csqlfn #Edisjoint_tpose_geo()
 */
int
ea_disjoint_tpose_geo(const Temporal *temp, const GSERIALIZED *gs,
  bool ever)
{
  return ea_spatialrel_tpose_geo(temp, gs, (Datum) NULL, 
    (varfunc) &datum_geom_disjoint2d, 2, ever, INVERT_NO);
}

/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if a temporal pose and a geometry are ever
 * disjoint,0 if not, and -1 on error or if the geometry is empty
 * @param[in] temp Temporal pose
 * @param[in] gs Geometry
 * @param[in] ever True for the ever semantics, false for the always semantics
 * @csqlfn #Edisjoint_tpose_geo()
 */
int
ea_disjoint_geo_tpose(const GSERIALIZED *gs, const Temporal *temp, bool ever)
{
  return ea_disjoint_tpose_geo(temp, gs, ever);
}

/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if a temporal pose and a geometry are ever
 * disjoint, 0 if not, and -1 on error or if the geometry is empty
 * @param[in] temp Temporal pose
 * @param[in] gs Geometry
 * @csqlfn #Edisjoint_tpose_geo()
 */
int
edisjoint_tpose_geo(const Temporal *temp, const GSERIALIZED *gs)
{
  return ea_disjoint_tpose_geo(temp, gs, EVER);
}

/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if a temporal pose and a geometry are always
 * disjoint,0 if not, and -1 on error or if the geometry is empty
 * @param[in] temp Temporal pose
 * @param[in] gs Geometry
 * @note aDisjoint(a, b) is equivalent to NOT eIntersects(a, b)
 * @csqlfn #Adisjoint_tpose_geo()
 */
inline int
adisjoint_tpose_geo(const Temporal *temp, const GSERIALIZED *gs)
{
  return ea_disjoint_tpose_geo(temp, gs, ALWAYS);
}

/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if a temporal pose and a pose are ever
 * disjoint, 0 if not, and -1 on error
 * @param[in] temp Temporal pose
 * @param[in] pose Pose
 * @param[in] ever True for the ever semantics, false for the always semantics
 * @csqlfn #Edisjoint_tpose_pose() #Adisjoint_tpose_pose()
 */
int
ea_disjoint_tpose_pose(const Temporal *temp, const Pose *pose,
  bool ever)
{
  return ea_spatialrel_tpose_pose(temp, pose, (Datum) NULL,
    (varfunc) &datum_geom_disjoint2d, 2, ever, INVERT_NO);
}

/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if a temporal pose and a pose are ever
 * disjoint, 0 if not, and -1 on error
 * @param[in] temp Temporal pose
 * @param[in] pose Pose
 * @param[in] ever True for the ever semantics, false for the always semantics
 * @csqlfn #Edisjoint_tpose_pose() #Adisjoint_tpose_pose()
 */
int
ea_disjoint_pose_tpose(const Pose *pose, const Temporal *temp, bool ever)
{
  return ea_disjoint_tpose_pose(temp, pose, ever);
}

/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if a temporal pose and a pose are ever
 * disjoint, 0 if not, and -1 on error
 * @param[in] temp Temporal pose
 * @param[in] pose Pose
 * @csqlfn #Edisjoint_tpose_pose()
 */
int
edisjoint_tpose_pose(const Temporal *temp, const Pose *pose)
{
  return ea_disjoint_tpose_pose(temp, pose, EVER);
}

/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if a temporal pose and a geometry are always
 * disjoint, 0 if not, and -1 on error
 * @param[in] temp Temporal pose
 * @param[in] pose Pose
 * @note aDisjoint(a, b) is equivalent to NOT eIntersects(a, b)
 * @csqlfn #Adisjoint_tpose_geo()
 */
inline int
adisjoint_tpose_pose(const Temporal *temp, const Pose *pose)
{
  return ea_disjoint_tpose_pose(temp, pose, ALWAYS);
}

/*****************************************************************************/

/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if the temporal poses are ever/always disjoint,
 * 0 if not, and -1 on error or if the temporal poses do not
 * intersect in time
 * @param[in] temp1,temp2 Temporal poses
 * @param[in] ever True for the ever semantics, false for the always semantics
 * @csqlfn #Edisjoint_tpose_tpose()
 */
int
ea_disjoint_tpose_tpose(const Temporal *temp1, const Temporal *temp2,
  bool ever)
{
  return ea_spatialrel_tpose_tpose(temp1, temp2, &datum_pose_disjoint,
    ever, false);
}

#if MEOS
/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if the temporal poses are ever disjoint, 0 if not,
 * and -1 on error or if the temporal poses do not intersect in time
 * @param[in] temp1,temp2 Temporal poses
 * @csqlfn #Edisjoint_tpose_tpose()
 */
inline int
edisjoint_tpose_tpose(const Temporal *temp1, const Temporal *temp2)
{
  return ea_disjoint_tpose_tpose(temp1, temp2, EVER);
}

/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if the temporal poses are always disjoint, 0 if
 * not, and -1 on error or if the temporal poses do not intersect
 * in time
 * @param[in] temp1,temp2 Temporal poses
 * @csqlfn #Adisjoint_tpose_tpose()
 */
inline int
adisjoint_tpose_tpose(const Temporal *temp1, const Temporal *temp2)
{
  return ea_disjoint_tpose_tpose(temp1, temp2, ALWAYS);
}
#endif /* MEOS */

/*****************************************************************************
 * Ever/always intersects
 *****************************************************************************/

/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if a temporal pose ever/always intersects a
 * geometry, 0 if not, and -1 on error or if the geometry is empty
 * @param[in] temp Temporal pose
 * @param[in] gs Geometry
 * @param[in] ever True for the ever semantics, false for the always semantics
 * @csqlfn #Eintersects_tpose_geo()
 */
int
ea_intersects_tpose_geo(const Temporal *temp, const GSERIALIZED *gs,
  bool ever)
{
  return ea_spatialrel_tpose_geo(temp, gs, (Datum) NULL, 
    (varfunc) &datum_geom_intersects2d, 2, ever, INVERT_NO);
}

/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if a temporal pose and a geometry intersect
 * 0 if not, and -1 on error or if the geometry is empty
 * @param[in] gs Geometry
 * @param[in] temp Temporal pose
 * @param[in] ever True for the ever semantics, false for the always semantics
 * @csqlfn #Eintersects_tpose_geo()
 */
int
ea_intersects_geo_tpose(const GSERIALIZED *gs, const Temporal *temp,
  bool ever)
{
  return ea_disjoint_tpose_geo(temp, gs, ever);
}

/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if a geometry and a temporal pose ever intersect,
 * 0 if not, and -1 on error or if the geometry is empty
 * @param[in] temp Temporal pose
 * @param[in] gs Geometry
 * @csqlfn #Eintersects_tpose_geo()
 */
int
eintersects_tpose_geo(const Temporal *temp, const GSERIALIZED *gs)
{
  return ea_intersects_tpose_geo(temp, gs, EVER);
}

/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if a geometry and a temporal pose always
 * intersect, 0 if not, and -1 on error or if the geometry is empty
 * @param[in] temp Temporal pose
 * @param[in] gs Geometry
 * @note aIntersects(tpose, geo) is equivalent to NOT eDisjoint(tpose, geo)
 * @csqlfn #Aintersects_tpose_geo()
 */
inline int
aintersects_tpose_geo(const Temporal *temp, const GSERIALIZED *gs)
{
  return ea_intersects_tpose_geo(temp, gs, ALWAYS);
}

/*****************************************************************************/

/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if a temporal pose ever/always intersects a
 * pose, 0 if not, and -1 on error
 * @param[in] temp Temporal pose
 * @param[in] pose Pose
 * @param[in] ever True for the ever semantics, false for the always semantics
 * @csqlfn #Eintersects_tpose_pose()
 */
int
ea_intersects_tpose_pose(const Temporal *temp, const Pose *pose,
  bool ever)
{
  return ea_spatialrel_tpose_pose(temp, pose, (Datum) NULL,
    (varfunc) &datum_geom_intersects2d, 2, ever, INVERT_NO);
}

/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if a pose ever/always intersects a temporal
 * pose, 0 if not, and -1 on error
 * @param[in] temp Temporal pose
 * @param[in] pose Pose
 * @param[in] ever True for the ever semantics, false for the always semantics
 * @csqlfn #Eintersects_tpose_pose()
 */
int
ea_intersects_pose_tpose(const Pose *pose, const Temporal *temp,
  bool ever)
{
  return ea_intersects_tpose_pose(temp, pose, ever);
}

/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if a pose and a temporal pose ever
 * intersect, 0 if not, and -1 on error
 * @param[in] temp Temporal pose
 * @param[in] pose Pose
 * @csqlfn #Eintersects_tpose_pose()
 */
inline int
eintersects_tpose_pose(const Temporal *temp, const Pose *pose)
{
  return ea_intersects_tpose_pose(temp, pose, EVER);
}

/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if a pose and a temporal pose always
 * intersect, 0 if not, and -1 on error
 * @param[in] temp Temporal pose
 * @param[in] pose Pose
 * @note aIntersects(tpose, pose) is equivalent to
 * NOT eDisjoint(tpose, pose)
 * @csqlfn #Aintersects_tpose_pose()
 */
inline int
aintersects_tpose_pose(const Temporal *temp, const Pose *pose)
{
  return ea_intersects_tpose_pose(temp, pose, ALWAYS);
}

/*****************************************************************************/

/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if a temporal pose ever/always intersects another
 * one, 0 if not, and -1 on error
 * @param[in] temp1,temp2 Temporal poses
 * @param[in] ever True for the ever semantics, false for the always semantics
 * @csqlfn #Eintersects_tpose_tpose()
 */
int
ea_intersects_tpose_tpose(const Temporal *temp1, const Temporal *temp2,
  bool ever)
{
  return ea_spatialrel_tpose_tpose(temp1, temp2,
    &datum_pose_intersects, ever, true);
}

#if MEOS
/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if the temporal poses ever intersect, 0 if not,
 * and -1 on error or if the temporal poses do not intersect in time
 * @param[in] temp1,temp2 Temporal poses
 * @csqlfn #Eintersects_tpose_tpose()
 */
inline int
eintersects_tpose_tpose(const Temporal *temp1, const Temporal *temp2)
{
  return ea_intersects_tpose_tpose(temp1, temp2, EVER);
}

/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if the temporal poses always intersect, 0 if not,
 * and -1 on error or if the temporal poses do not intersect in time
 * @param[in] temp1,temp2 Temporal poses
 * @csqlfn #Aintersects_tpose_tpose()
 */
inline int
aintersects_tpose_tpose(const Temporal *temp1, const Temporal *temp2)
{
  return ea_intersects_tpose_tpose(temp1, temp2, ALWAYS);
}
#endif /* MEOS */

/*****************************************************************************
 * Ever/always touches
 *****************************************************************************/

/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if a temporal pose and a geometry ever touch,
 * 0 if not, and -1 on error or if the geometry is empty
 * @param[in] temp Temporal pose
 * @param[in] gs Geometry
 * @param[in] ever True for the ever semantics, false for the always semantics
 * @csqlfn #Etouches_tpose_geo(), #Atouches_tpose_geo()
 */
int
ea_touches_tpose_geo(const Temporal *temp, const GSERIALIZED *gs, bool ever)
{
  return ea_spatialrel_tpose_geo(temp, gs, (Datum) NULL, 
    (varfunc) &datum_geom_touches, 2, ever, INVERT_NO);
}

/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if a temporal pose and a geometry ever touch, 0
 * if not, and -1 on error or if the geometry is empty
 * @param[in] temp Temporal pose
 * @param[in] gs Geometry
 * @param[in] ever True for the ever semantics, false for the always semantics
 * @csqlfn #Etouches_tpose_geo()
 */
int
ea_touches_geo_tpose(const GSERIALIZED *gs, const Temporal *temp, bool ever)
{
  return ea_touches_tpose_geo(temp, gs, ever);
}

/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if a temporal pose ever touches a geometry,
 * 0 if not, and -1 on error or if the geometry is empty
 * @param[in] temp Temporal pose
 * @param[in] gs Geometry
 * @csqlfn #Atouches_tpose_geo()
 */
int
etouches_tpose_geo(const Temporal *temp, const GSERIALIZED *gs)
{
  return ea_touches_tpose_geo(temp, gs, EVER);
}

/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if a temporal pose always touches a geometry,
 * 0 if not, and -1 on error or if the geometry is empty
 * @param[in] temp Temporal pose
 * @param[in] gs Geometry
 * @csqlfn #Atouches_tpose_geo()
 */
int
atouches_tpose_geo(const Temporal *temp, const GSERIALIZED *gs)
{
  return ea_touches_tpose_geo(temp, gs, ALWAYS);
}

/*****************************************************************************
 * Ever/always dwithin
 * The function only accepts points and not arbitrary geometries
 *****************************************************************************/

/**
 * @brief Return 1 if a temporal pose and a geometry are ever/always
 * within the given distance, 0 if not, -1 on error or if the geometry is empty
 * @param[in] temp Temporal pose
 * @param[in] gs Geometry
 * @param[in] dist Distance
 * @param[in] ever True for the ever semantics, false for the always semantics
 * @param[in] invert True if the arguments should be inverted
 * @csqlfn #Edwithin_tpose_geo(), #Adwithin_tpose_geo()
 */
int
ea_dwithin_tpose_geo(const Temporal *temp, const GSERIALIZED *gs,
  double dist, bool ever, bool invert)
{
  VALIDATE_TPOSE(temp, -1); VALIDATE_NOT_NULL(gs, -1);
  /* Ensure the validity of the arguments */
  if (! ensure_valid_tpose_geo(temp, gs) || gserialized_is_empty(gs) ||
      ! ensure_not_negative_datum(Float8GetDatum(dist), T_FLOAT8))
    return -1;
  return ea_spatialrel_tpose_geo(temp, gs, Float8GetDatum(dist),
    (varfunc) &datum_geom_dwithin2d, 3, ever, invert);
}

/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if a geometry and a temporal pose are ever within
 * the given distance, 0 if not, -1 on error or if the geometry is empty
 * @param[in] temp Temporal pose
 * @param[in] gs Geometry
 * @param[in] dist Distance
 * @csqlfn #Edwithin_tpose_geo()
 */
int
edwithin_tpose_geo(const Temporal *temp, const GSERIALIZED *gs, double dist)
{
  return ea_dwithin_tpose_geo(temp, gs, dist, EVER, INVERT_NO);
}

/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if a geometry and a temporal pose are always
 * within a distance, 0 if not, -1 on error or if the geometry is empty
 * @param[in] temp Temporal pose
 * @param[in] gs Geometry
 * @param[in] dist Distance
 * @csqlfn #Adwithin_tpose_geo()
 */
int
adwithin_tpose_geo(const Temporal *temp, const GSERIALIZED *gs, double dist)
{
  return ea_dwithin_tpose_geo(temp, gs, dist, ALWAYS, INVERT_NO);
}

/*****************************************************************************/

/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if a geometry and a temporal pose are ever within
 * the given distance, 0 if not, -1 on error
 * @param[in] temp Temporal pose
 * @param[in] pose Pose
 * @param[in] dist Distance
 * @csqlfn #Edwithin_tpose_pose()
 */
int
edwithin_tpose_pose(const Temporal *temp, const Pose *pose,
  double dist)
{
  VALIDATE_TPOSE(temp, -1); VALIDATE_NOT_NULL(pose, -1);
  /* Ensure the validity of the arguments */
  if (! ensure_valid_tpose_pose(temp, pose) ||
      ! ensure_not_negative_datum(Float8GetDatum(dist), T_FLOAT8))
    return -1;
  GSERIALIZED *trav = pose_to_point(pose);
  int result = ea_spatialrel_tpose_geo(temp, trav, Float8GetDatum(dist),
    (varfunc) &datum_geom_dwithin2d, 3, EVER, INVERT_NO);
  pfree(trav);
  return result;
}

/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if a geometry and a temporal pose are always
 * within a distance, 0 if not, -1 on error
 * @param[in] temp Temporal pose
 * @param[in] pose Pose
 * @param[in] dist Distance
 * @csqlfn #Adwithin_tpose_pose()
 */
int
adwithin_tpose_pose(const Temporal *temp, const Pose *pose,
  double dist)
{
  VALIDATE_TPOSE(temp, -1); VALIDATE_NOT_NULL(pose, -1);
  /* Ensure the validity of the arguments */
  if (! ensure_valid_tpose_pose(temp, pose) ||
      ! ensure_not_negative_datum(Float8GetDatum(dist), T_FLOAT8))
    return -1;
  GSERIALIZED *trav = pose_to_point(pose);
  GSERIALIZED *buffer = geom_buffer(trav, dist, "");
  int result = ea_spatialrel_tpose_geo(temp, buffer, (Datum) NULL,
    (varfunc) &datum_geom_covers, 2, ALWAYS, INVERT_NO);
  pfree(trav); pfree(buffer);
  return result;
}

/*****************************************************************************/

/**
 * @ingroup meos_internal_pose_spatial_rel_ever
 * @brief Return 1 if two temporal poses are ever/always within a
 * distance, 0 if not, -1 on error or if the temporal poses do not
 * intersect on time
 * @param[in] temp1,temp2 Temporal poses
 * @param[in] dist Distance
 * @param[in] ever True for the ever semantics, false for the always semantics
 * @csqlfn #Edwithin_tpose_tpose(), #Adwithin_tpose_tpose()
 */
int 
ea_dwithin_tpose_tpose(const Temporal *temp1, const Temporal *temp2,
  double dist, bool ever)
{
  VALIDATE_TPOSE(temp1, -1); VALIDATE_TPOSE(temp2, -1);
  /* Ensure the validity of the arguments */
  if (! ensure_valid_tpose_tpose(temp1, temp2) ||
      ! ensure_not_negative_datum(Float8GetDatum(dist), T_FLOAT8))
    return -1;

  /* Fill the lifted structure */
  LiftedFunctionInfo lfinfo;
  memset(&lfinfo, 0, sizeof(LiftedFunctionInfo));
  lfinfo.func = (varfunc) datum_pose_dwithin;
  lfinfo.numparam = 1;
  lfinfo.param[0] = Float8GetDatum(dist);
  lfinfo.argtype[0] = lfinfo.argtype[1] = temp1->temptype;
  lfinfo.restype = T_TFLOAT;
  lfinfo.reslinear = false;
  lfinfo.invert = INVERT_NO;
  lfinfo.discont = MEOS_FLAGS_LINEAR_INTERP(temp1->flags) ||
    MEOS_FLAGS_LINEAR_INTERP(temp2->flags);
  lfinfo.ever = ever;
  lfinfo.tpfn_temp = &tposesegm_dwithin_turnpt;
  return eafunc_temporal_temporal(temp1, temp2, &lfinfo);
}

/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if two temporal poses are ever within a distance,
 * 0 if not, -1 on error or if the temporal poses do not intersect
 * on time
 * @param[in] temp1,temp2 Temporal poses
 * @param[in] dist Distance
 * @csqlfn #Edwithin_tpose_tpose()
 */
inline int
edwithin_tpose_tpose(const Temporal *temp1, const Temporal *temp2,
  double dist)
{
  return ea_dwithin_tpose_tpose(temp1, temp2, dist, EVER);
}

/**
 * @ingroup meos_pose_rel_ever
 * @brief Return 1 if two temporal poses are always within a
 * distance, 0 if not, -1 on error or if the temporal poses do not
 * intersect on time
 * @param[in] temp1,temp2 Temporal poses
 * @param[in] dist Distance
 * @csqlfn #Adwithin_tpose_tpose()
 */
inline int
adwithin_tpose_tpose(const Temporal *temp1, const Temporal *temp2,
  double dist)
{
  return ea_dwithin_tpose_tpose(temp1, temp2, dist, ALWAYS);
}

/*****************************************************************************/
