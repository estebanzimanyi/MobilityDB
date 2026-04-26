/*****************************************************************************
 * SPIKE: minimal Clipper2-backed polygon-intersection adapter.
 *
 * Goal: prove that we can compile vendored Clipper2 C++ inside MEOS, link
 * it from the C side, and round-trip GSERIALIZED -> Clipper2 -> GSERIALIZED.
 * Single POLYGON ∩ single POLYGON only. The production adapter will handle
 * MULTIPOLYGON, holes, the four Boolean operations, and the four
 * _mdb_internal_clip_* SQL entry points.
 *****************************************************************************/

extern "C" {
#include "geo/clip_clipper2.h"
#include <postgres.h>
#include <fmgr.h>
#include <liblwgeom.h>
#include "geo/tgeo_spatialfuncs.h"  /* for geo_serialize */

/* Force the symbol's PG magic block to live in this TU as well, so the
 * V1 wrapper below has the macros it needs. */
}

#include "clipper2/clipper.h"

using namespace Clipper2Lib;

/* Scale factor: ×10^7 captures lon/lat to ~11 mm at the equator.
 * int64_t MAX_COORD ≈ 2.3e18, so coords up to ±2.3e11 fit safely. */
static constexpr double CLIP_SCALE = 1e7;

extern "C" GSERIALIZED *
clipper2_intersect_spike(const GSERIALIZED *subj_g, const GSERIALIZED *clip_g)
{
  /* Deserialize */
  LWGEOM *subj_lw = lwgeom_from_gserialized(subj_g);
  LWGEOM *clip_lw = lwgeom_from_gserialized(clip_g);
  if (!subj_lw || !clip_lw) {
    if (subj_lw) lwgeom_free(subj_lw);
    if (clip_lw) lwgeom_free(clip_lw);
    return nullptr;
  }
  if (subj_lw->type != POLYGONTYPE || clip_lw->type != POLYGONTYPE) {
    lwgeom_free(subj_lw); lwgeom_free(clip_lw);
    elog(ERROR, "clipper2_intersect_spike: only POLYGONTYPE accepted in spike");
    return nullptr;
  }

  LWPOLY *subj_poly = lwgeom_as_lwpoly(subj_lw);
  LWPOLY *clip_poly = lwgeom_as_lwpoly(clip_lw);

  /* Convert outer ring only (spike — no holes) */
  auto ring_to_path = [](const POINTARRAY *pa) -> Path64 {
    Path64 path;
    path.reserve(pa->npoints);
    for (uint32_t i = 0; i < pa->npoints - 1; i++) {
      const POINT2D *p = (const POINT2D *) getPoint_internal(pa, i);
      path.emplace_back(static_cast<int64_t>(std::llround(p->x * CLIP_SCALE)),
                        static_cast<int64_t>(std::llround(p->y * CLIP_SCALE)));
    }
    return path;
  };

  Paths64 subjects = { ring_to_path(subj_poly->rings[0]) };
  Paths64 clips    = { ring_to_path(clip_poly->rings[0]) };

  /* Run Clipper2 boolean intersection */
  Paths64 solution = Intersect(subjects, clips, FillRule::EvenOdd);

  int32_t srid = gserialized_get_srid(subj_g);
  lwgeom_free(subj_lw);
  lwgeom_free(clip_lw);

  if (solution.empty()) return nullptr;

  /* Convert first output path back to LWPOLY (spike — single polygon) */
  const Path64 &p0 = solution[0];
  POINTARRAY *pa = ptarray_construct_empty(LW_FALSE, LW_FALSE,
    static_cast<uint32_t>(p0.size() + 1));
  for (const auto &pt : p0) {
    POINT4D pt4 = { pt.x / CLIP_SCALE, pt.y / CLIP_SCALE, 0.0, 0.0 };
    ptarray_append_point(pa, &pt4, LW_TRUE);
  }
  /* Close ring */
  if (!p0.empty()) {
    const auto &first = p0.front();
    POINT4D pt4 = { first.x / CLIP_SCALE, first.y / CLIP_SCALE, 0.0, 0.0 };
    ptarray_append_point(pa, &pt4, LW_TRUE);
  }

  LWPOLY *out_poly = lwpoly_construct_empty(srid, LW_FALSE, LW_FALSE);
  lwpoly_add_ring(out_poly, pa);
  LWGEOM *out_lw = lwpoly_as_lwgeom(out_poly);
  lwgeom_add_bbox(out_lw);

  GSERIALIZED *result = geo_serialize(out_lw);
  lwgeom_free(out_lw);
  return result;
}

/* PG V1 wrapper so SQL can call this directly during the spike. */
extern "C" {

PGDLLEXPORT Datum clipper2_intersect_spike_v1(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(clipper2_intersect_spike_v1);

Datum
clipper2_intersect_spike_v1(PG_FUNCTION_ARGS)
{
  GSERIALIZED *subj = (GSERIALIZED *) PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));
  GSERIALIZED *clip = (GSERIALIZED *) PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(1));
  GSERIALIZED *result = clipper2_intersect_spike(subj, clip);
  pfree(subj); pfree(clip);
  if (!result) PG_RETURN_NULL();
  PG_RETURN_POINTER(result);
}

} /* extern "C" */
