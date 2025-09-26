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
 * @brief Functions for base and time types corresponding to external
 * PostgreSQL functions in order to bypass the function manager @p fmgr.c.
 */

#ifndef POSTGRES_TYPES_H
#define POSTGRES_TYPES_H

/* PostgreSQL */
#include <postgres.h>
#include "utils/numeric.h"
#include <utils/date.h>
#include <utils/timestamp.h>

#if POSTGRESQL_VERSION_NUMBER < 170000
/*
 * Infinite intervals are represented by setting all fields to the minimum or
 * maximum integer values.
 */
#define INTERVAL_NOBEGIN(i)  \
  do {  \
    (i)->time = PG_INT64_MIN;  \
    (i)->day = PG_INT32_MIN;  \
    (i)->month = PG_INT32_MIN;  \
  } while (0)

#define INTERVAL_IS_NOBEGIN(i)  \
  ((i)->month == PG_INT32_MIN && (i)->day == PG_INT32_MIN && (i)->time == PG_INT64_MIN)

#define INTERVAL_NOEND(i)  \
  do {  \
    (i)->time = PG_INT64_MAX;  \
    (i)->day = PG_INT32_MAX;  \
    (i)->month = PG_INT32_MAX;  \
  } while (0)

#define INTERVAL_IS_NOEND(i)  \
  ((i)->month == PG_INT32_MAX && (i)->day == PG_INT32_MAX && (i)->time == PG_INT64_MAX)

#define INTERVAL_NOT_FINITE(i) (INTERVAL_IS_NOBEGIN(i) || INTERVAL_IS_NOEND(i))
#endif /* POSTGRESQL_VERSION_NUMBER < 170000 */

/* Functions adapted from hashfunc.c */

extern uint32 char_hash(char c);
extern uint64 char_hash_extended(char c, uint64 seed);
extern uint32 int2_hash(int16 val);
extern uint64 int2_hash_extended(int16 val, uint64 seed);
extern uint32 int4_hash(int32 val);
extern uint64 int4_hash_extended(int32 val, uint64 seed);
extern uint32 int8_hash(int64 val);
extern uint64 int8_hash_extended(int64 val, uint64 seed);
extern uint32 float4_hash(float4 key);
extern uint64 float4_hash_extended(float4 key, uint64 seed);
extern uint32 float8_hash(float8 key);
extern uint64 float8_hash_extended(float8 key, uint64 seed);
extern uint32 text_hash(text *key, Oid collid);
extern uint64 text_hash_extended(text *key, uint64 seed, Oid collid);

/* Functions adapted from numeric.c */

extern Numeric numeric_in_internal(char *str, int32 typmod);
extern char *numeric_out_internal(Numeric num);
extern Numeric numeric_internal(Numeric num, int32 typmod);
extern Numeric numeric_abs_internal(Numeric num);
extern Numeric numeric_uplus_internal(Numeric num);
extern Numeric numeric_uminus_internal(Numeric num);
extern int numeric_sign_internal(Numeric num);
extern Numeric numeric_round_internal(Numeric num, int32 scale);
extern Numeric numeric_trunc_internal(Numeric num, int32 scale);
extern Numeric numeric_ceil_internal(Numeric num);
extern Numeric numeric_floor_internal(Numeric num);
extern int32 width_bucket_numeric_internal(Numeric operand, Numeric bound1,
  Numeric bound2, int32 count);
extern int numeric_cmp_internal(Numeric num1, Numeric num2);
extern bool numeric_eq_internal(Numeric num1, Numeric num2);
extern bool numeric_ne_internal(Numeric num1, Numeric num2);
extern bool numeric_gt_internal(Numeric num1, Numeric num2);
extern bool numeric_ge_internal(Numeric num1, Numeric num2);
extern bool numeric_lt_internal(Numeric num1, Numeric num2);
extern bool numeric_le_internal(Numeric num1, Numeric num2);
extern bool in_range_numeric_numeric_internal(Numeric val, Numeric base,
  Numeric offset, bool sub, bool less);
extern int numeric_hash(Numeric key);
extern uint64 numeric_hash_extended(Numeric key, uint64 seed);
extern Numeric numeric_add_internal(Numeric num1, Numeric num2);
extern Numeric numeric_sub_internal(Numeric num1, Numeric num2);
extern Numeric numeric_mul_internal(Numeric num1, Numeric num2);
extern Numeric numeric_div_internal(Numeric num1, Numeric num2);
extern Numeric numeric_div_trunc_internal(Numeric num1, Numeric num2);
extern Numeric numeric_mod_internal(Numeric num1, Numeric num2);
extern Numeric numeric_inc_internal(Numeric num);
extern Numeric numeric_smaller_internal(Numeric num1, Numeric num2);
extern Numeric numeric_larger_internal(Numeric num1, Numeric num2);
extern Numeric numeric_gcd_internal(Numeric num1, Numeric num2);
extern Numeric numeric_lcm_internal(Numeric num1, Numeric num2);
extern Numeric numeric_fac_internal(int64 num);
extern Numeric numeric_sqrt_internal(Numeric num);
extern Numeric numeric_exp_internal(Numeric num);
extern Numeric numeric_ln_internal(Numeric num);
extern Numeric numeric_log_internal(Numeric num1, Numeric num2);
extern Numeric numeric_power_internal(Numeric num1, Numeric num2);
extern int numeric_scale_internal(Numeric num);
extern int numeric_min_scale_internal(Numeric num);
extern Numeric numeric_trim_scale_internal(Numeric num);
extern Numeric int4_numeric_internal(int32 val);
extern int numeric_int4_internal(Numeric num);
extern Numeric int8_numeric_internal(int64 val);
extern int64 numeric_int8_internal(Numeric num);
extern Numeric int2_numeric_internal(int16 val);
extern int16 numeric_int2_internal(Numeric num);
extern Numeric float8_numeric_internal(float8 val);
extern double numeric_float8_internal(Numeric num);
extern double numeric_float8_no_overflow_internal(Numeric num);
extern Numeric float4_numeric_internal(float4 val);

/* Functions adapted from int.c */

extern int32 int4_in(const char *str);
extern char *int4_out(int32 val);

/* Functions adapted from int8.c */

extern int64 int8_in(const char *str);
extern char *int8_out(int64 val);

/* Functions adapted from float.c */

extern float4 float4_in(const char *str);

extern float8 float8_angular_difference(float8 num1, float8 num2);
extern float8 float8_atan(float8 num);
extern float8 float8_atan2(float8 num1, float8 num2);
extern float8 float8_cos(float8 num);
extern float8 float8_exp(float8 num);
extern float8 float8_in(const char *str);
extern float8 float8_ln(float8 num);
extern float8 float8_log10(float8 num);
extern char *float8_out(double num, int maxdd);
extern float8 float8_round(float8 num, int maxdd);
extern float8 float8_sin(float8 num);

/* Functions adadpted from timestamp.c */

extern void interval_negate(const Interval *interval, Interval *result);
extern Interval *add_interval_interval(Interval *interval1, Interval *interval2);
extern Interval *minus_interval_interval(Interval *interval1, Interval *interval2);

/* Functions adapted from hashfn.h and hashfn.c */

extern uint32 hash_bytes_uint32(uint32 k);
extern uint32 pg_hashint8(int64 val);
extern uint32 pg_hashfloat8(float8 key);
extern uint64 hash_bytes_uint32_extended(uint32 k, uint64 seed);
extern uint64 pg_hashint8extended(int64 val, uint64 seed);
extern uint64 pg_hashfloat8extended(float8 key, uint64 seed);
extern uint32 pg_hashtext(text *key);
extern uint64 pg_hashtextextended(text *key, uint64 seed);

/*****************************************************************************/

#endif /* POSTGRES_TYPES_H */
