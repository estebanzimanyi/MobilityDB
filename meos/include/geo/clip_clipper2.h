/*****************************************************************************
 * SPIKE: Clipper2-based polygon clipping for MEOS
 * One-function smoke test to prove the C++ build pipeline + C-callable shim.
 *****************************************************************************/

#ifndef __CLIP_CLIPPER2_H__
#define __CLIP_CLIPPER2_H__

#include <postgres.h>
#include <liblwgeom.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SPIKE: intersect two POLYGONs via Clipper2.
 * @param subj 2D POLYGON
 * @param clip 2D POLYGON
 * @return new GSERIALIZED with the intersection, or NULL on empty
 *
 * This spike accepts only single POLYGON (no MULTIPOLYGON, no holes) to
 * keep the smoke test minimal. Sufficient to prove the build pipeline
 * works; the production adapter will handle the full surface.
 */
extern GSERIALIZED *clipper2_intersect_spike(const GSERIALIZED *subj,
  const GSERIALIZED *clip);

#ifdef __cplusplus
}
#endif

#endif
