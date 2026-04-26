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
 * @brief @c pcpatch_filter_per_point — the decompose / filter / rebuild
 * primitive that per-point operators delegate into.
 */

#include "pointcloud/pcpatch_decompose.h"

/* C */
#include <assert.h>
/* PostgreSQL */
#include <postgres.h>
/* MEOS */
#include <meos.h>
#include "pointcloud/pcpatch.h"
#include "pointcloud/pgsql_compat.h"
#include "pointcloud/meos_schema_hook.h"
/* pgPointCloud */
#include "pc_api.h"
#include "pc_api_internal.h"   /* for pc_patch_uncompressed_make / _add_point */

/*****************************************************************************/

Pcpatch *
pcpatch_filter_per_point(const Pcpatch *pa, pcpatch_pointpred_fn pred,
  void *extra)
{
  assert(pa); assert(pred);

  /* Schema is needed both for deserialize and for rebuilding. */
  PCSCHEMA *schema = meos_pc_schema(pa->pcid);
  if (! schema)
  {
    meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
      "pcpatch_filter_per_point: no schema registered for pcid %u",
      pa->pcid);
    return NULL;
  }

  /* Pcpatch is byte-compatible with SERIALIZED_PATCH (see pcpatch.h). */
  PCPATCH *patch = MEOS_PC_PATCH_DESERIALIZE(
    (const SERIALIZED_PATCH *) pa, schema);
  if (! patch)
    return NULL;

  PCPOINTLIST *pl = pc_pointlist_from_patch(patch);
  if (! pl)
  {
    pc_patch_free(patch);
    return NULL;
  }

  /* Build the survivor patch in uncompressed form. Capacity = pl->npoints
   * is an upper bound; pc_patch_uncompressed_add_point grows internally
   * if exceeded but pre-sizing avoids reallocation churn. */
  PCPATCH_UNCOMPRESSED *out = pc_patch_uncompressed_make(schema, pl->npoints);
  for (uint32_t i = 0; i < pl->npoints; i++)
  {
    PCPOINT *pt = pc_pointlist_get_point(pl, i);
    if (pred(pt, extra))
      pc_patch_uncompressed_add_point(out, pt);
  }

  Pcpatch *result = NULL;
  if (out->npoints > 0)
  {
    /* Recompute extent + stats so the serialized header is consistent
     * with the survivor set (bounds matter for downstream bbox prune). */
    pc_patch_uncompressed_compute_extent(out);
    pc_patch_uncompressed_compute_stats(out);
    SERIALIZED_PATCH *ser = MEOS_PC_PATCH_SERIALIZE((PCPATCH *) out, NULL);
    result = (Pcpatch *) ser;
  }

  pc_patch_free((PCPATCH *) out);
  pc_pointlist_free(pl);
  pc_patch_free(patch);
  return result;
}

/*****************************************************************************/
