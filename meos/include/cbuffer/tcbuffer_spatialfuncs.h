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
 * @brief Temporal distance for temporal network points.
 */

#ifndef __TCBUFFER_SPATIALFUNCS_H__
#define __TCBUFFER_SPATIALFUNCS_H__

/* PostgreSQL */
#include <postgres.h>
/* MEOS */
#include "temporal/temporal.h"
#include "cbuffer/cbuffer.h"

/*****************************************************************************/

/* Traversed area functions */

extern GSERIALIZED *tcbufferinst_trav_area(const TInstant *inst);
extern GSERIALIZED *tcbufferseq_trav_area(const TSequence *seq);
extern GSERIALIZED *tcbufferseqset_trav_area(const TSequenceSet *ss);
extern GSERIALIZED *tcbuffersegm_trav_area(const TInstant *inst1,
  const TInstant *inst2);

/* Restriction functions */

extern Temporal *tcbuffer_restrict_cbuffer(const Temporal *temp,
  const Cbuffer *cb, bool atfunc);
extern Temporal *tcbuffer_restrict_stbox(const Temporal *temp,
 const STBox *box, bool border_inc, bool atfunc);
extern Temporal *tcbuffer_restrict_geom(const Temporal *temp,
  const GSERIALIZED *gs, bool atfunc);

/*****************************************************************************/

#endif /* __TCBUFFER_SPATIALFUNCS_H__ */
