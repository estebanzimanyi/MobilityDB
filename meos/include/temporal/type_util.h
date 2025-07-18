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
 * @brief Miscellaneous utility functions for temporal types.
 */

#ifndef __TYPE_UTIL_H__
#define __TYPE_UTIL_H__

/* PostgreSQL */
#include <postgres.h>
/* MEOS */
#include <meos.h>
#include <meos_geo.h>
#include <meos_npoint.h>
#include "temporal/doublen.h"
#include "temporal/meos_catalog.h"

/*****************************************************************************/

/** Symbolic constants for the *_to_arr() functions */
#define FREE_ALL        true
#define FREE            false

/*****************************************************************************/

/* Miscellaneous functions */

extern Datum datum_copy(Datum value, meosType typid);
extern double datum_double(Datum d, meosType type);
extern Datum double_datum(double d, meosType type);
extern bytea *bstring2bytea(const uint8_t *wkb, size_t size);

/* Input/output functions */

extern bool basetype_in(const char *str, meosType type, bool end, Datum *result);
extern char *basetype_out(Datum value, meosType type, int maxdd);

/* Array functions */

extern void pfree_array(void **array, int count);
extern char *stringarr_to_string(char **strings, int count, size_t outlen,
  char *prefix, char open, char close, bool quotes, bool spaces);

/* Sort functions */

extern void datumarr_sort(Datum *values, int count, meosType basetype);
extern void tstzarr_sort(TimestampTz *times, int count);
extern void spanarr_sort(Span *spans, int count);
extern void tinstarr_sort(TInstant **instants, int count);
extern void tseqarr_sort(TSequence **sequences, int count);

/* Remove duplicate functions */

extern int datumarr_remove_duplicates(Datum *values, int count,
  meosType basetype);
extern int tstzarr_remove_duplicates(TimestampTz *values, int count);
extern int tinstarr_remove_duplicates(const TInstant **instants, int count);

/* Text functions */


/* Arithmetic functions */

extern Datum datum_add(Datum l, Datum r, meosType type);
extern Datum datum_sub(Datum l, Datum r, meosType type);
extern Datum datum_mult(Datum l, Datum r, meosType type);
extern Datum datum_div(Datum l, Datum r, meosType type);

/* Comparison functions on datums */

extern int datum_cmp(Datum l, Datum r, meosType type);
extern bool datum_eq(Datum l, Datum r, meosType type);
extern bool datum_ne(Datum l, Datum r, meosType type);
extern bool datum_lt(Datum l, Datum r, meosType type);
extern bool datum_le(Datum l, Datum r, meosType type);
extern bool datum_gt(Datum l, Datum r, meosType type);
extern bool datum_ge(Datum l, Datum r, meosType type);

extern Datum datum2_eq(Datum l, Datum r, meosType type);
extern Datum datum2_ne(Datum l, Datum r, meosType type);
extern Datum datum2_lt(Datum l, Datum r, meosType type);
extern Datum datum2_le(Datum l, Datum r, meosType type);
extern Datum datum2_gt(Datum l, Datum r, meosType type);
extern Datum datum2_ge(Datum l, Datum r, meosType type);

/* Hypothenuse functions */

extern double hypot3d(double x, double y, double z);

/*****************************************************************************/

#endif /* __TYPE_UTIL_H__ */
