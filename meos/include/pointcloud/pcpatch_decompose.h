/*****************************************************************************
 *
 * This MobilityDB code is provided under The PostgreSQL License.
 * Copyright (c) 2016-2026, Université libre de Bruxelles and MobilityDB
 * contributors
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
 *****************************************************************************/

/**
 * @file
 * @brief Per-point decomposition primitive for pcpatch.
 *
 * Provides a single C-side entry point that other MEOS pointcloud
 * operations (per-point @c atTpcbox, @c eIntersects, fast
 * @c points(tpcpatch) SRF, …) delegate into. Decomposes a serialized
 * @c Pcpatch into its constituent points, applies a caller-supplied
 * predicate, and rebuilds a new @c Pcpatch from the survivors — all
 * in C, no SQL roundtrip via @c PC_Explode.
 *
 * Depends on the transitional @c pgsql_compat shim while pgPointCloud's
 * @c pc_(point|patch)_(de)serialize helpers remain in @c pgsql/ rather
 * than @c lib/. See @c pgsql_compat.h for the kill-switch story.
 */

#ifndef __PCPATCH_DECOMPOSE_H__
#define __PCPATCH_DECOMPOSE_H__

#include <stdbool.h>

#include <meos_pointcloud.h>     /* for Pcpatch */
#include "pc_api.h"              /* for PCPOINT */

/**
 * @brief Per-point predicate signature.
 *
 * Returns @c true to keep the point in the rebuilt patch, @c false to
 * drop it. The opaque @c extra argument is whatever the caller of
 * @c pcpatch_filter_per_point passes through verbatim — typically a
 * bbox struct, a geometry pointer, or a small parameter pack.
 */
typedef bool (*pcpatch_pointpred_fn)(const PCPOINT *pt, void *extra);

/**
 * @brief Decompose a pcpatch, apply a predicate per point, rebuild.
 *
 * @param pa     Source pcpatch (must not be NULL).
 * @param pred   Predicate. Returns true to keep, false to drop.
 * @param extra  Caller-supplied state passed verbatim to @c pred.
 * @return Newly allocated pcpatch holding only the surviving points
 *         (always uncompressed-form output), or @c NULL when every
 *         point was dropped or the schema for @c pa->pcid cannot be
 *         resolved via the schema hook. Caller owns the result.
 */
extern Pcpatch *pcpatch_filter_per_point(const Pcpatch *pa,
  pcpatch_pointpred_fn pred, void *extra);

#endif /* __PCPATCH_DECOMPOSE_H__ */
