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
 * @brief Bounding box operators for spatiotemporal values
 */

#ifndef __TSPATIAL_BOXOPS_H__
#define __TSPATIAL_BOXOPS_H__

/* PostgreSQL */
#include <postgres.h>
/* PostGIS */
#include <liblwgeom.h>
/* MEOS */
#include <meos.h>
#include "temporal/meos_catalog.h"

/*****************************************************************************/

/* Functions computing the bounding box at the creation of a temporal point */

extern void tgeoinst_set_stbox(const TInstant *inst, STBox *box);
extern void tgeoinstarr_set_stbox(const TInstant **instants, int count,
  STBox *box);
extern void tgeoseq_expand_stbox(TSequence *seq, const TInstant *inst);

extern void tspatialinst_set_stbox(const TInstant *inst, STBox *box);
extern void tspatialinstarr_set_stbox(const TInstant **instants, int count,
  bool lower_inc, bool upper_inc, interpType interp, void *box);
extern void tspatialseqarr_set_stbox(const TSequence **sequences, int count,
  STBox *box);
extern void tspatialseq_expand_stbox(TSequence *seq, const TInstant *inst);

extern void spatialarr_set_bbox(const Datum *values, meosType basetype,
  int count, void *box);

/* Generic box functions */

extern bool boxop_tspatial_stbox(const Temporal *temp, const STBox *box,
  bool (*func)(const STBox *, const STBox *), bool invert);
extern bool boxop_tspatial_tspatial(const Temporal *temp1, const Temporal *temp2,
  bool (*func)(const STBox *, const STBox *));
  
/*****************************************************************************/

#endif /* __TSPATIAL_BOXOPS_H__ */
