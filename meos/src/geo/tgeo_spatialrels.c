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
 * @file
 * @brief Ever/always spatial relationships for temporal geometries
 *
 * These relationships compute the ever/always spatial relationship between the
 * arguments and return a Boolean. These functions may be used for filtering
 * purposes before applying the corresponding temporal spatial relationship.
 *
 * The following relationships are supported for geometries: contains,
 * disjoint, intersects, touches, and dwithin.
 *
 * The following relationships are supported for geographies: disjoint,
 * intersects, dwithin.
 *
 * Only disjoint, dwithin, and intersects are supported for 3D geometries.
 */

#include "point/tpoint_spatialrels.h"

/* C */
#include <assert.h>
/* MEOS */
#include <meos.h>
#include <meos_internal.h>
#include "general/lifting.h"
#include "point/pgis_types.h"
#include "point/tpoint_spatialfuncs.h"
#include "point/tpoint_tempspatialrels.h"
#include "geo/tgeo_spatialfuncs.h"

/*****************************************************************************
 * Generic ever/always spatial relationship functions
 *****************************************************************************/

// /**
 // * @brief Return true if the values are equal
 // * @note This function should be faster than the function #datum_cmp()
 // */
// bool
// datum_geo_eq(Datum l, Datum r, meosType type)
// {
  // assert(meos_basetype(type));
  // switch (type)
  // {
    // case T_GEOMETRY:
    // case T_GEOGRAPHY:
      // return geo_equals(DatumGetGserializedP(l), DatumGetGserializedP(r));
    // default: /* Error! */
    // meos_error(ERROR, MEOS_ERR_INTERNAL_TYPE_ERROR,
      // "Unknown equality function for type: %s", meostype_name(type));
    // return false;
  // }
// }

// /**
 // * @brief Return true if the points are equal
 // */
// Datum
// datum2_geo_eq(Datum geo1, Datum geo2)
// {
  // return BoolGetDatum(datum_geo_eq(geo1, geo2));
// }

/**
 * @brief Return a Datum true if two 2D geometries are disjoint
 */
Datum
datum_geom_disjoint2d(Datum geom1, Datum geom2)
{
  return BoolGetDatum(! geom_spatialrel(DatumGetGserializedP(geom1),
    DatumGetGserializedP(geom2), INTERSECTS));
}

/**
 * @brief Return a Datum true if two 3D geometries are disjoint
 */
Datum
datum_geom_disjoint3d(Datum geom1, Datum geom2)
{
  return BoolGetDatum(! geom_intersects3d(DatumGetGserializedP(geom1),
    DatumGetGserializedP(geom2)));
}

/**
 * @brief Return a Datum true if two geographies are disjoint
 */
Datum
datum_geog_disjoint(Datum geog1, Datum geog2)
{
  return BoolGetDatum(! geog_dwithin(DatumGetGserializedP(geog1),
    DatumGetGserializedP(geog2), 0.00001, true));
}

/**
 * @brief Select the appropriate intersect function
 * @note We need two parameters to cope with mixed 2D/3D arguments
 */
datum_func2
get_intersects_fn(int16 flags)
{
  if (MEOS_FLAGS_GET_GEODETIC(flags))
    return &datum_geog_intersects;
  else
    return MEOS_FLAGS_GET_Z(flags) ? 
      &datum_geom_intersects3d : &datum_geom_intersects2d;
}

/**
 * @brief Select the appropriate intersect function
 * @note We need two parameters to cope with mixed 2D/3D arguments
 */
datum_func2
get_disjoint_fn(int16 flags)
{
  if (MEOS_FLAGS_GET_GEODETIC(flags))
    return &datum_geog_disjoint;
  else
    return MEOS_FLAGS_GET_Z(flags) ? 
      &datum_geom_disjoint3d : &datum_geom_disjoint2d;
}

/*****************************************************************************/

/**
 * @brief Generic spatial relationship for the trajectory of a temporal geo
 * and a geometry/geography
 * @param[in] temp Temporal geo
 * @param[in] gs Geometry
 * @param[in] param Parameter
 * @param[in] func PostGIS function to be called
 * @param[in] numparam Number of parameters of the functions
 * @param[in] invert True if the arguments should be inverted
 * @return On error return -1
 */
static int
spatialrel_tgeo_trav_geo(const Temporal *temp, const GSERIALIZED *gs,
  Datum param, varfunc func, int numparam, bool invert)
{
  /* Ensure validity of the arguments */
  if (! ensure_not_null((void *) temp) || ! ensure_not_null((void *) gs) ||
      ! ensure_valid_tgeo_geo(temp, gs) || gserialized_is_empty(gs))
    return -1;

  assert(numparam == 2 || numparam == 3);
  Datum geo = PointerGetDatum(gs);
  Datum trav = PointerGetDatum(tgeo_traversed_area(temp));
  Datum result;
  if (numparam == 2)
  {
    datum_func2 func2 = (datum_func2) func;
    result = invert ? func2(geo, trav) : func2(trav, geo);
  }
  else /* numparam == 3 */
  {
    datum_func3 func3 = (datum_func3) func;
    result = invert ? func3(geo, trav, param) : func3(trav, geo, param);
  }
  pfree(DatumGetPointer(trav));
  return result ? 1 : 0;
}

/*****************************************************************************/

/**
 * @brief Return true if two temporal geos ever/always satisfy a spatial
 * relationship
 * @param[in] temp1,temp2 Temporal points
 * @param[in] func Spatial relationship
 * @param[in] ever True for the ever semantics, false for the always semantics
 */
int
ea_spatialrel_tgeo_tgeo(const Temporal *temp1, const Temporal *temp2,
  datum_func2 func, bool ever)
{
  /* Ensure validity of the arguments */
  if (! ensure_not_null((void *) temp1) || ! ensure_not_null((void *) temp2) ||
      ! ensure_valid_tgeo_tgeo(temp1, temp2) ||
      ! ensure_same_dimensionality(temp1->flags, temp2->flags) ||
      ! ensure_same_geodetic(temp1->flags, temp2->flags))
    return -1;

  /* Fill the lifted structure */
  LiftedFunctionInfo lfinfo;
  memset(&lfinfo, 0, sizeof(LiftedFunctionInfo));
  lfinfo.func = (varfunc) func;
  lfinfo.numparam = 0;
  lfinfo.argtype[0] = lfinfo.argtype[1] = temp1->temptype;
  lfinfo.restype = T_TBOOL;
  lfinfo.reslinear = false;
  lfinfo.invert = INVERT_NO;
  lfinfo.discont = MEOS_FLAGS_LINEAR_INTERP(temp1->flags) ||
    MEOS_FLAGS_LINEAR_INTERP(temp2->flags);
  lfinfo.ever = ever;
  lfinfo.tpfunc_base = NULL;
  lfinfo.tpfunc = NULL;
  return eafunc_temporal_temporal(temp1, temp2, &lfinfo);
}

/*****************************************************************************
 * Ever/always contains
 *****************************************************************************/

/**
 * @ingroup meos_temporal_spatial_rel_ever
 * @brief Return 1 if a geometry ever contains a temporal geo, 0 if not, and
 * -1 on error or if the geometry is empty
 * @param[in] gs Geometry
 * @param[in] temp Temporal geo
 * @note The function does not accept 3D or geography since it is based on the
 * PostGIS ST_Relate function. The function tests whether the trajectory
 * intersects the interior of the geometry. Please refer to the documentation
 * of the ST_Contains and ST_Relate functions
 * https://postgis.net/docs/ST_Relate.html
 * https://postgis.net/docs/ST_Contains.html
 * @csqlfn #Econtains_geo_tgeo()
 */
int
econtains_geo_tgeo(const GSERIALIZED *gs, const Temporal *temp)
{
  /* Ensure validity of the arguments */
  if (! ensure_valid_tgeo_geo(temp, gs) || gserialized_is_empty(gs) ||
      ! ensure_has_not_Z_gs(gs) || ! ensure_has_not_Z(temp->flags))
    return -1;
  GSERIALIZED *trav = tgeo_traversed_area(temp);
  bool result = geom_relate_pattern(gs, trav, "T********");
  pfree(trav);
  return result ? 1 : 0;
}

/**
 * @ingroup meos_temporal_spatial_rel_ever
 * @brief Return 1 if a geometry always contains a temporal geo,
 * 0 if not, and -1 on error or if the geometry is empty
 * @param[in] gs Geometry
 * @param[in] temp Temporal geo
 * @note The function tests whether the trajectory is contained in the geometry.
 * https://postgis.net/docs/ST_Relate.html
 * https://postgis.net/docs/ST_Contains.html
 * @csqlfn #Acontains_geo_tgeo()
 */
int
acontains_geo_tgeo(const GSERIALIZED *gs, const Temporal *temp)
{
  /* Ensure validity of the arguments */
  if (! ensure_valid_tgeo_geo(temp, gs) || gserialized_is_empty(gs) ||
      ! ensure_has_not_Z_gs(gs) || ! ensure_has_not_Z(temp->flags))
    return -1;
  GSERIALIZED *trav = tgeo_traversed_area(temp);
  bool result = geom_contains(gs, trav);
  pfree(trav);
  return result ? 1 : 0;
}

/*****************************************************************************
 * Ever/always disjoint (only always works for both geometry and geography)
 *****************************************************************************/

/**
 * @ingroup meos_temporal_spatial_rel_ever
 * @brief Return 1 if a temporal geo and a geometry are ever disjoint,
 * 0 if not, and -1 on error or if the geometry is empty
 * @param[in] temp Temporal geo
 * @param[in] gs Geometry
 * @note eDisjoint(tpoint, geo) is equivalent to NOT covers(geo, trav(tpoint))
 * @note The function does not accept geography since it is based on the
 * PostGIS ST_Covers function provided by GEOS
 * @csqlfn #Edisjoint_tgeo_geo()
 */
int
edisjoint_tgeo_geo(const Temporal *temp, const GSERIALIZED *gs)
{
  /* Ensure validity of the arguments */
  if (! ensure_not_geodetic(temp->flags))
    return -1;
  datum_func2 func = &datum_geom_covers;
  int result = spatialrel_tgeo_trav_geo(temp, gs, (Datum) NULL,
    (varfunc) func, 2, INVERT);
  return INVERT_RESULT(result);
}

/**
 * @ingroup meos_temporal_spatial_rel_ever
 * @brief Return 1 if a temporal geo and a geometry are always disjoint,
 * 0 if not, and -1 on error or if the geometry is empty
 * @param[in] temp Temporal geo
 * @param[in] gs Geometry
 * @note aDisjoint(a, b) is equivalent to NOT eIntersects(a, b)
 * @csqlfn #Adisjoint_tgeo_geo()
 */
int
adisjoint_tgeo_geo(const Temporal *temp, const GSERIALIZED *gs)
{
  int result = edisjoint_tgeo_geo(temp, gs);
  return INVERT_RESULT(result);
}

/**
 * @ingroup meos_temporal_spatial_rel_ever
 * @brief Return 1 if the temporal geos are ever disjoint, 0 if not, and
 * -1 on error or if the temporal geos do not intersect in time
 * @param[in] temp1,temp2 Temporal points
 * @csqlfn #Edisjoint_tgeo_tgeo()
 */
int
edisjoint_tgeo_tgeo(const Temporal *temp1, const Temporal *temp2)
{
  datum_func2 func = get_disjoint_fn(temp1->flags);
  return ea_spatialrel_tgeo_tgeo(temp1, temp2, func, EVER);
}

/**
 * @ingroup meos_temporal_spatial_rel_ever
 * @brief Return 1 if the temporal geos are always disjoint, 0 if not, and
 * -1 on error or if the temporal geos do not intersect in time
 * @param[in] temp1,temp2 Temporal points
 * @csqlfn #Adisjoint_tgeo_tgeo()
 */
int
adisjoint_tgeo_tgeo(const Temporal *temp1, const Temporal *temp2)
{
  datum_func2 func = get_disjoint_fn(temp1->flags);
  return ea_spatialrel_tgeo_tgeo(temp1, temp2, func, ALWAYS);
}

/*****************************************************************************
 * Ever/always intersects (for both geometry and geography)
 *****************************************************************************/

/**
 * @ingroup meos_temporal_spatial_rel_ever
 * @brief Return 1 if a geometry and a temporal geo ever intersect,
 * 0 if not, and -1 on error or if the geometry is empty
 * @param[in] temp Temporal geo
 * @param[in] gs Geometry
 * @csqlfn #Eintersects_tgeo_geo()
 */
int
eintersects_tgeo_geo(const Temporal *temp, const GSERIALIZED *gs)
{
  datum_func2 func = get_intersects_fn_gs(temp->flags, gs->gflags);
  return spatialrel_tgeo_trav_geo(temp, gs, (Datum) NULL, (varfunc) func, 2,
    INVERT_NO);
}

/**
 * @ingroup meos_temporal_spatial_rel_ever
 * @brief Return 1 if a geometry and a temporal geo always intersect,
 * 0 if not, and -1 on error or if the geometry is empty
 * @param[in] temp Temporal geo
 * @param[in] gs Geometry
 * @note aIntersects(tpoint, geo) is equivalent to NOT eDisjoint(tpoint, geo)
 * @note The function does not accept geography since the eDisjoint function
 * is based on the PostGIS ST_Covers function provided by GEOS
 * @csqlfn #Aintersects_tgeo_geo()
 */
int
aintersects_tgeo_geo(const Temporal *temp, const GSERIALIZED *gs)
{
  int result = edisjoint_tgeo_geo(temp, gs);
  return INVERT_RESULT(result);
}

/**
 * @ingroup meos_temporal_spatial_rel_ever
 * @brief Return 1 if the temporal geos ever intersect, 0 if not, and
 * -1 on error or if the temporal geos do not intersect in time
 * @param[in] temp1,temp2 Temporal points
 * @csqlfn #Eintersects_tgeo_tgeo()
 */
int
eintersects_tgeo_tgeo(const Temporal *temp1, const Temporal *temp2)
{
  datum_func2 func = get_intersects_fn(temp1->flags);
  return ea_spatialrel_tgeo_tgeo(temp1, temp2, func, EVER);
}

/**
 * @ingroup meos_temporal_spatial_rel_ever
 * @brief Return 1 if the temporal geos always intersect, 0 if not, and
 * -1 on error or if the temporal geos do not intersect in time
 * @param[in] temp1,temp2 Temporal points
 * @csqlfn #Aintersects_tgeo_tgeo()
 */
int
aintersects_tgeo_tgeo(const Temporal *temp1, const Temporal *temp2)
{
  datum_func2 func = get_intersects_fn(temp1->flags);
  return ea_spatialrel_tgeo_tgeo(temp1, temp2, func, EVER);
}

/*****************************************************************************
 * Ever/always touches
 * The function does not accept geography since it is based on the PostGIS
 * ST_Boundary function
 *****************************************************************************/

/**
 * @ingroup meos_temporal_spatial_rel_ever
 * @brief Return 1 if a temporal geo and a geometry ever touch, 0 if not, and
 * -1 on error or if the geometry is empty
 * @param[in] temp Temporal geo
 * @param[in] gs Geometry
 * @csqlfn #Etouches_tgeo_geo()
 */
int
etouches_tgeo_geo(const Temporal *temp, const GSERIALIZED *gs)
{
  /* Ensure validity of the arguments */
  if (! ensure_not_null((void *) temp) || ! ensure_not_null((void *) gs) ||
      ! ensure_not_geodetic(temp->flags) || gserialized_is_empty(gs) ||
      ! ensure_valid_tgeo_geo(temp, gs))
    return -1;

  /* There is no need to do a bounding box test since this is done in
   * the SQL function definition */
  datum_func2 func = get_intersects_fn_gs(temp->flags, gs->gflags);
  GSERIALIZED *trav = tgeo_traversed_area(temp);
  GSERIALIZED *gsbound = geom_boundary(gs);
  bool result = false;
  if (gsbound && ! gserialized_is_empty(gsbound))
    result = func(GserializedPGetDatum(gsbound), GserializedPGetDatum(trav));
  /* TODO */
  // else if (MEOS_FLAGS_LINEAR_INTERP(temp->flags))
  // {
    // /* The geometry is a point or a multipoint -> the boundary is empty */
    // GSERIALIZED *tempbound = geom_boundary(trav);
    // if (tempbound)
    // {
      // result = func(GserializedPGetDatum(tempbound), GserializedPGetDatum(gs));
      // pfree(tempbound);
    // }
  // }
  pfree(trav); pfree(gsbound);
  return result ? 1 : 0;
}

/**
 * @ingroup meos_temporal_spatial_rel_ever
 * @brief Return 1 if a temporal geo and a geometry always touch, 0 if not,
 * and -1 on error or if the geometry is empty
 * @param[in] temp Temporal geo
 * @param[in] gs Geometry
 * @csqlfn #Atouches_tgeo_geo()
 */
int
atouches_tgeo_geo(const Temporal *temp, const GSERIALIZED *gs)
{
  /* Ensure validity of the arguments */
  if (! ensure_not_null((void *) temp) || ! ensure_not_null((void *) gs) ||
      ! ensure_valid_tgeo_geo(temp, gs) || gserialized_is_empty(gs))
    return -1;

  /* There is no need to do a bounding box test since this is done in
   * the SQL function definition */
  GSERIALIZED *gsbound = geom_boundary(gs);
  bool result = false;
  if (gsbound && ! gserialized_is_empty(gsbound))
  {
    Temporal *temp1 = tpoint_restrict_geom(temp, gsbound, NULL, REST_MINUS);
    result = (temp1 == NULL);
    if (temp1)
      pfree(temp1);
  }
  pfree(gsbound);
  return result ? 1 : 0;
}

/*****************************************************************************
 * Ever/always dwithin (for both geometry and geography)
 * The function only accepts points and not arbitrary geometries/geographies
 *****************************************************************************/

/**
 * @ingroup meos_temporal_spatial_rel_ever
 * @brief Return 1 if a geometry and a temporal geo are ever within the
 * given distance, 0 if not, -1 on error or if the geometry is empty
 * @param[in] temp Temporal geo
 * @param[in] gs Geometry
 * @param[in] dist Distance
 * @csqlfn #Edwithin_tgeo_geo()
 */
int
edwithin_tgeo_geo(const Temporal *temp, const GSERIALIZED *gs, double dist)
{
  /* Ensure validity of the arguments */
  if (! ensure_not_negative_datum(Float8GetDatum(dist), T_FLOAT8))
    return -1;
  datum_func3 func = get_dwithin_fn_gs(temp->flags, gs->gflags);
  return spatialrel_tgeo_trav_geo(temp, gs, Float8GetDatum(dist),
    (varfunc) func, 3, INVERT_NO);
}

/**
 * @ingroup meos_temporal_spatial_rel_ever
 * @brief Return 1 if a geometry and a temporal geo are always within a
 * distance, 0 if not, -1 on error or if the geometry is empty
 * @param[in] temp Temporal geo
 * @param[in] gs Geometry
 * @param[in] dist Distance
 * @note The function is not available for 3D or geograhies since it is based
 * on thePostGIS ST_Buffer() function which is performed by GEOS
 * @csqlfn #Adwithin_tgeo_geo()
 */
int
adwithin_tgeo_geo(const Temporal *temp, const GSERIALIZED *gs, double dist)
{
  /* Ensure validity of the arguments */
  if (! ensure_not_geodetic(temp->flags) || ! ensure_has_not_Z(temp->flags) ||
      ! ensure_not_negative_datum(Float8GetDatum(dist), T_FLOAT8))
    return -1;

  GSERIALIZED *buffer = geom_buffer(gs, dist, "");
  datum_func2 func = &datum_geom_covers;
  int result = spatialrel_tgeo_trav_geo(temp, buffer, (Datum) NULL,
    (varfunc) func, 2, INVERT);
  pfree(buffer);
  return result;
}

/*****************************************************************************/

/**
 * @brief Return true if the temporal geos are ever within a distance
 * @param[in] inst1,inst2 Temporal points
 * @param[in] dist Distance
 * @param[in] func DWithin function (2D or 3D)
 * @pre The temporal geos are synchronized
 */
static bool
ea_dwithin_tgeoinst_tgeoinst(const TInstant *inst1, const TInstant *inst2,
  double dist, datum_func3 func)
{
  assert(inst1); assert(inst2);
  Datum value1 = tinstant_val(inst1);
  Datum value2 = tinstant_val(inst2);
  /* Result is the same for both EVER and ALWAYS */
  return DatumGetBool(func(value1, value2, Float8GetDatum(dist)));
}

/**
 * @brief Return true if two temporal geos are ever within a distance
 * @param[in] seq1,seq2 Temporal points
 * @param[in] dist Distance
 * @param[in] func DWithin function (2D or 3D)
 * @param[in] ever True for the ever semantics, false for the always semantics
 * @pre The temporal geos are synchronized
 */
static bool
ea_dwithin_tgeoseq_tgeoseq(const TSequence *seq1,
  const TSequence *seq2, double dist, datum_func3 func, bool ever)
{
  assert(seq1); assert(seq2);
  bool ret_loop = ever ? true : false;
  for (int i = 0; i < seq1->count; i++)
  {
    bool res = ea_dwithin_tgeoinst_tgeoinst(TSEQUENCE_INST_N(seq1, i),
      TSEQUENCE_INST_N(seq2, i), dist, func);
    if ((ever && res) || (! ever && ! res))
      return ret_loop;
  }
  return ! ret_loop;
}

/**
 * @brief Return true if two temporal geos are ever within a distance
 * @param[in] ss1,ss2 Temporal points
 * @param[in] dist Distance
 * @param[in] func DWithin function (2D or 3D)
 * @param[in] ever True for the ever semantics, false for the always semantics
 * @pre The temporal geos are synchronized
 */
static bool
ea_dwithin_tgeoseqset_tgeoseqset(const TSequenceSet *ss1,
  const TSequenceSet *ss2, double dist, datum_func3 func, bool ever)
{
  assert(ss1); assert(ss2);
  bool ret_loop = ever ? true : false;
  for (int i = 0; i < ss1->count; i++)
  {
    const TSequence *seq1 = TSEQUENCESET_SEQ_N(ss1, i);
    const TSequence *seq2 = TSEQUENCESET_SEQ_N(ss2, i);
    bool res = 
      ea_dwithin_tgeoseq_tgeoseq(seq1, seq2, dist, func, ever);
    if ((ever && res) || (! ever && ! res))
      return ret_loop;
  }
  return ! ret_loop;
}

/*****************************************************************************/

/**
 * @brief Return 1 if two temporal geos are ever within a distance,
 * 0 if not, -1 if the temporal geos do not intersect on time
 * @pre The temporal geos are synchronized
 */
int
ea_dwithin_tgeo_tgeo_sync(const Temporal *sync1, const Temporal *sync2,
  double dist, bool ever)
{
  datum_func3 func = get_dwithin_fn(sync1->flags, sync2->flags);
  assert(temptype_subtype(sync1->subtype));
  switch (sync1->subtype)
  {
    case TINSTANT:
      return ea_dwithin_tgeoinst_tgeoinst((TInstant *) sync1,
        (TInstant *) sync2, dist, func);
    case TSEQUENCE:
      return ea_dwithin_tgeoseq_tgeoseq((TSequence *) sync1,
          (TSequence *) sync2, dist, func, ever);
    default: /* TSEQUENCESET */
      return ea_dwithin_tgeoseqset_tgeoseqset((TSequenceSet *) sync1,
        (TSequenceSet *) sync2, dist, func, ever);
  }
}

/**
 * @ingroup meos_internal_temporal_spatial_rel_ever
 * @brief Return 1 if two temporal geos are ever within a distance,
 * 0 if not, -1 on error or if the temporal geos do not intersect on time
 * @param[in] temp1,temp2 Temporal points
 * @param[in] dist Distance
 * @param[in] ever True for the ever semantics, false for the always semantics
 * @csqlfn #Edwithin_tgeo_tgeo()
 */
int
ea_dwithin_tgeo_tgeo(const Temporal *temp1, const Temporal *temp2,
  double dist, bool ever)
{
  /* Ensure validity of the arguments */
  if (! ensure_valid_tgeo_tgeo(temp1, temp2) ||
      ! ensure_not_negative_datum(Float8GetDatum(dist), T_FLOAT8))
    return -1;

  Temporal *sync1, *sync2;
  /* Return NULL if the temporal geos do not intersect in time
   * The operation is synchronization without adding crossings */
  if (! intersection_temporal_temporal(temp1, temp2, SYNCHRONIZE_NOCROSS,
    &sync1, &sync2))
    return -1;

  bool result = ea_dwithin_tgeo_tgeo_sync(sync1, sync2, dist, ever);
  pfree(sync1); pfree(sync2);
  return result ? 1 : 0;
}

/**
 * @ingroup meos_temporal_spatial_rel_ever
 * @brief Return 1 if two temporal geos are ever within a distance,
 * 0 if not, -1 on error or if the temporal geos do not intersect on time
 * @param[in] temp1,temp2 Temporal points
 * @param[in] dist Distance
 * @csqlfn #Edwithin_tgeo_tgeo()
 */
int
edwithin_tgeo_tgeo(const Temporal *temp1, const Temporal *temp2,
  double dist)
{
  /* Ensure validity of the arguments */
  if (! ensure_not_null((void *) temp1) || ! ensure_not_null((void *) temp2))
    return -1;
  return ea_dwithin_tgeo_tgeo(temp1, temp2, dist, EVER);
}

/**
 * @ingroup meos_temporal_spatial_rel_ever
 * @brief Return 1 if two temporal geos are always within a distance,
 * 0 if not, -1 on error or if the temporal geos do not intersect on time
 * @param[in] temp1,temp2 Temporal points
 * @param[in] dist Distance
 * @csqlfn #Adwithin_tgeo_tgeo()
 */
int
adwithin_tgeo_tgeo(const Temporal *temp1, const Temporal *temp2,
  double dist)
{
  /* Ensure validity of the arguments */
  if (! ensure_not_null((void *) temp1) || ! ensure_not_null((void *) temp2))
    return -1;
  return ea_dwithin_tgeo_tgeo(temp1, temp2, dist, ALWAYS);
}

/*****************************************************************************/
