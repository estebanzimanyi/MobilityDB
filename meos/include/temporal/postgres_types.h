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
#include <utils/float.h>
#include <utils/numeric.h>
#if MEOS
#include "postgres_int_defs.h"
#else
#include <utils/date.h>
#include <utils/timestamp.h>
#endif /* MEOS */

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

/* Functions adapted from bool.c */

extern bool bool_eq(bool b1, bool b2);
extern bool bool_ge(bool b1, bool b2);
extern bool bool_gt(bool b1, bool b2);
extern uint32 bool_hash(bool b);
extern bool bool_in(const char *str);
extern bool bool_le(bool b1, bool b2);
extern bool bool_lt(bool b1, bool b2);
extern bool bool_ne(bool b1, bool b2);
extern char * bool_out(bool b);
extern text * bool_to_text(bool b);

/* Functions adapted from numeric.c */

extern Numeric numeric_in_internal(const char *str, int32 typmod);
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
extern uint64 numeric_hash_extended(Numeric num, uint64 seed);
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
extern Numeric int4_numeric_internal(int32 num);
extern int numeric_int4_internal(Numeric num);
extern Numeric int8_numeric_internal(int64 num);
extern int64 numeric_int8_internal(Numeric num);
extern Numeric int2_numeric_internal(int16 num);
extern int16 numeric_int2_internal(Numeric num);
extern Numeric float8_numeric_internal(float8 num);
extern double numeric_float8_internal(Numeric num);
extern double numeric_float8_no_overflow_internal(Numeric num);
extern Numeric float4_numeric_internal(float4 num);

/* Functions adapted from int.c and int8.c */

extern int32 bool_to_int4(bool num);
extern int64 float4_to_int8(float4 num);
extern int64 float8_to_int8(float8 num);
extern int32 int24_div(int16 num1, int32 num2);
extern bool int24_eq(int16 num1, int32 num2);
extern bool int24_ge(int16 num1, int32 num2);
extern bool int24_gt(int16 num1, int32 num2);
extern bool int24_le(int16 num1, int32 num2);
extern bool int24_lt(int16 num1, int32 num2);
extern int32 int24_mi(int16 num1, int32 num2);
extern int32 int24_mul(int16 num1, int32 num2);
extern bool int24_ne(int16 num1, int32 num2);
extern int32 int24_pl(int16 num1, int32 num2);
extern int64 int28_div(int16 num1, int64 num2);
extern bool int28_eq(int16 num1, int64 num2);
extern bool int28_ge(int16 num1, int64 num2);
extern bool int28_gt(int16 num1, int64 num2);
extern bool int28_le(int16 num1, int64 num2);
extern bool int28_lt(int16 num1, int64 num2);
extern int64 int28_mi(int16 num1, int64 num2);
extern int64 int28_mul(int16 num1, int64 num2);
extern bool int28_ne(int16 num1, int64 num2);
extern int64 int28_pl(int16 num1, int64 num2);
extern int16 int2_abs(int16 num);
extern int16 int2_and(int16 num1, int16 num2);
extern int16 int2_div(int16 num1, int16 num2);
extern bool int2_eq(int16 num1, int16 num2);
extern bool int2_ge(int16 num1, int16 num2);
extern bool int2_gt(int16 num1, int16 num2);
extern int16 int2_in(const char *str);
extern int16 int2_lnumer(int16 num1, int16 num2);
extern bool int2_le(int16 num1, int16 num2);
extern bool int2_lt(int16 num1, int16 num2);
extern int16 int2_mi(int16 num1, int16 num2);
extern int16 int2_mod(int16 num1, int16 num2);
extern int16 int2_mul(int16 num1, int16 num2);
extern bool int2_ne(int16 num1, int16 num2);
extern int16 int2_not(int16 num);
extern int16 int2_or(int16 num1, int16 num2);
extern char *int2_out(int16 num);
extern int16 int2_pl(int16 num1, int16 num2);
extern int16 int2_shl(int16 num1, int32 num2);
extern int16 int2_shr(int16 num1, int32 num2);
extern int16 int2_smaller(int16 num1, int16 num2);
extern int32 int2_to_int4(int16 num);
extern int64 int2_to_int8(int16 num);
extern int16 int2_um(int16 num);
extern int16 int2_up(int16 num);
extern int16 int2_xor(int16 num1, int16 num2);
extern int32 int42_div(int32 num1, int16 num2);
extern bool int42_eq(int32 num1, int16 num2);
extern bool int42_ge(int32 num1, int16 num2);
extern bool int42_gt(int32 num1, int16 num2);
extern bool int42_le(int32 num1, int16 num2);
extern bool int42_lt(int32 num1, int16 num2);
extern int32 int42_mi(int32 num1, int16 num2);
extern int32 int42_mul(int32 num1, int16 num2);
extern bool int42_ne(int32 num1, int16 num2);
extern int32 int42_pl(int32 num1, int16 num2);
extern int64 int48_div(int32 num1, int64 num2);
extern bool int48_eq(int32 num1, int64 num2);
extern bool int48_ge(int32 num1, int64 num2);
extern bool int48_gt(int32 num1, int64 num2);
extern bool int48_le(int32 num1, int64 num2);
extern bool int48_lt(int32 num1, int64 num2);
extern int64 int48_mi(int32 num1, int64 num2);
extern int64 int48_mul(int32 num1, int64 num2);
extern bool int48_ne(int32 num1, int64 num2);
extern int64 int48_pl(int32 num1, int64 num2);
extern int32 int4_abs(int32 num);
extern int32 int4_and(int32 num1, int32 num2);
extern int int4_cmp(int32 l, int32 r);
extern int32 int4_div(int32 num1, int32 num2);
extern bool int4_eq(int32 num1, int32 num2);
extern int32 int4_gcd(int32 num1, int32 num2);
extern bool int4_ge(int32 num1, int32 num2);
extern bool int4_gt(int32 num1, int32 num2);
extern int32 int4_in(const char *str);
extern int32 int4_inc(int32 num);
extern int32 int4_lnumer(int32 num1, int32 num2);
extern int32 int4_lcm(int32 num1, int32 num2);
extern bool int4_le(int32 num1, int32 num2);
extern bool int4_lt(int32 num1, int32 num2);
extern int32 int4_mi(int32 num1, int32 num2);
extern int32 int4_mod(int32 num1, int32 num2);
extern int32 int4_mul(int32 num1, int32 num2);
extern bool int4_ne(int32 num1, int32 num2);
extern int32 int4_not(int32 num);
extern int32 int4_or(int32 num1, int32 num2);
extern char *int4_out(int32 num);
extern int32 int4_pl(int32 num1, int32 num2);
extern int32 int4_shl(int32 num1, int32 num2);
extern int32 int4_shr(int32 num1, int32 num2);
extern int32 int4_smaller(int32 num1, int32 num2);
extern bool int4_to_bool(int32 num);
extern int16 int4_to_int2(int32 num);
extern int64 int4_to_int8(int32 num);
extern int32 int4_um(int32 num);
extern int32 int4_up(int32 num);
extern int32 int4_xor(int32 num1, int32 num2);
extern int64 int82_div(int64 num1, int16 num2);
extern bool int82_eq(int64 num1, int16 num2);
extern bool int82_ge(int64 num1, int16 num2);
extern bool int82_gt(int64 num1, int16 num2);
extern bool int82_le(int64 num1, int16 num2);
extern bool int82_lt(int64 num1, int16 num2);
extern int64 int82_mi(int64 num1, int16 num2);
extern int64 int82_mul(int64 num1, int16 num2);
extern bool int82_ne(int64 num1, int16 num2);
extern int64 int82_pl(int64 num1, int16 num2);
extern int64 int84_div(int64 num1, int32 num2);
extern bool int84_eq(int64 num1, int32 num2);
extern bool int84_ge(int64 num1, int32 num2);
extern bool int84_gt(int64 num1, int32 num2);
extern bool int84_le(int64 num1, int32 num2);
extern bool int84_lt(int64 num1, int32 num2);
extern int64 int84_mi(int64 num1, int32 num2);
extern int64 int84_mul(int64 num1, int32 num2);
extern bool int84_ne(int64 num1, int32 num2);
extern int64 int84_pl(int64 num1, int32 num2);
extern int64 int8_abs(int64 num);
extern int64 int8_and(int64 num1, int64 num2);
extern int int8_cmp(int64 l, int64 r);
extern int64 int8_dec(int64 num);
extern int64 int8_div(int64 num1, int64 num2);
extern bool int8_eq(int64 num1, int64 num2);
extern int64 int8_gcd(int64 num1, int64 num2);
extern bool int8_ge(int64 num1, int64 num2);
extern bool int8_gt(int64 num1, int64 num2);
extern int64 int8_in(const char *str);
extern int64 int8_inc(int64 num);
extern int64 int8_lnumer(int64 num1, int64 num2);
extern int64 int8_lcm(int64 num1, int64 num2);
extern bool int8_le(int64 num1, int64 num2);
extern bool int8_lt(int64 num1, int64 num2);
extern int64 int8_mi(int64 num1, int64 num2);
extern int64 int8_mod(int64 num1, int64 num2);
extern int64 int8_mul(int64 num1, int64 num2);
extern bool int8_ne(int64 num1, int64 num2);
extern int64 int8_not(int64 num);
extern int64 int8_or(int64 num1, int64 num2);
extern char *int8_out(int64 num);
extern int64 int8_pl(int64 num1, int64 num2);
extern int64 int8_shl(int64 num1, int32 num2);
extern int64 int8_shr(int64 num1, int32 num2);
extern int64 int8_smaller(int64 num1, int64 num2);
extern float4 int8_to_float4(int64 num);
extern float8 int8_to_float8(int64 num);
extern int16 int8_to_int2(int64 num);
extern int32 int8_to_int4(int64 num);
extern int64 int8_um(int64 num);
extern int64 int8_up(int64 num);
extern int64 int8_xor(int64 num1, int64 num2);

/* Functions adapted from float.c */

extern float8 degrees_internal(float8 num);
extern float8 float48_div(float4 num1, float8 num2);
extern bool float48_eq(float4 num1, float8 num2);
extern bool float48_ge(float4 num1, float8 num2);
extern bool float48_gt(float4 num1, float8 num2);
extern bool float48_le(float4 num1, float8 num2);
extern bool float48_lt(float4 num1, float8 num2);
extern float8 float48_mi(float4 num1, float8 num2);
extern float8 float48_mul(float4 num1, float8 num2);
extern bool float48_ne(float4 num1, float8 num2);
extern float8 float48_pl(float4 num1, float8 num2);
extern float4 float4_abs(float4 num);
extern int float4_cmp_internal(float4 a, float4 b);
extern float4 float4_in(const char *num);
extern float4 float4_larger(float4 num1, float4 num2);
extern char * float4_out(float4 num);
extern float4 float4_smaller(float4 num1, float4 num2);
extern int16 float4_to_int2(float4 num);
extern int32 float4_to_int4(float4 num);
extern float8 float4_tod(float4 num);
extern float4 float4_um(float4 num);
extern float4 float4_up(float4 num);
extern bool float84_eq(float8 num1, float4 num2);
extern bool float84_ge(float8 num1, float4 num2);
extern bool float84_gt(float8 num1, float4 num2);
extern bool float84_le(float8 num1, float4 num2);
extern bool float84_lt(float8 num1, float4 num2);
extern float8 float84_mi(float8 num1, float4 num2);
extern float8 float84_mul(float8 num1, float4 num2);
extern bool float84_ne(float8 num1, float4 num2);
extern float8 float84_pl(float8 num1, float4 num2);
extern float8 float84div(float8 num1, float4 num2);
extern float8 float8_abs(float8 num);
extern float8 float8_acosd(float8 num);
extern float8 float8_acosh(float8 num);
extern float8 float8_asind(float8 num);
extern float8 float8_asinh(float8 num);
extern float8 float8_atan(float8 num);
extern float8 float8_atan2(float8 num1, float8 num2);
extern float8 float8_atan2d(float8 num1, float8 num2);
extern float8 float8_atand(float8 num);
extern float8 float8_atanh(float8 num);
extern int float8_cmp_internal(float8 a, float8 b);
extern float8 float8_cos(float8 num);
extern float8 float8_cosd(float8 num);
extern float8 float8_cosh(float8 num);
extern float8 float8_cotd(float8 num);
extern float8 float8_gamma(float8 num);
extern float8 float8_in(const char *str);
extern float8 float8_larger(float8 num1, float8 num2);
extern float8 float8_lgamma(float8 num);
extern char * float8_out(float8 num, int maxdd);
extern float8 float8_pi(void);
extern float8 float8_rint(float8 num);
extern float8 float8_round(float8 num, int maxdd);
extern float8 float8_sin(float8 num);
extern float8 float8_sind(float8 num);
extern float8 float8_sinh(float8 num);
extern float8 float8_smaller(float8 num1, float8 num2);
extern float8 float8_tand(float8 num);
extern float8 float8_tanh(float8 num);
extern float4 float8_to_float4(float8 num);
extern int16 float8_to_int2(float8 num);
extern int32 float8_to_int4(float8 num);
extern float8 float8_um(float8 num);
extern float8 float8_up(float8 num);
extern float8 float8_angular_difference(float8 degrees1, float8 degrees2);
extern float8 float8_exp(float8 num);
extern float8 float8_ln(float8 num);
extern float8 float8_log10(float8 num);
extern float8 float8_round(float8 num, int maxdd);
extern float4 int2_to_float4(int16 num);
extern float8 int2_to_float8(int16 num);
extern float4 int4_to_float4(int32 num);
extern float8 int4_to_float8(int32 num);
extern float4 pg_float4_div(float4 num1, float4 num2);
extern bool pg_float4_eq(float4 num1, float4 num2);
extern bool pg_float4_ge(float4 num1, float4 num2);
extern bool pg_float4_gt(float4 num1, float4 num2);
extern bool pg_float4_le(float4 num1, float4 num2);
extern bool pg_float4_lt(float4 num1, float4 num2);
extern float4 pg_float4_mi(float4 num1, float4 num2);
extern float4 pg_float4_mul(float4 num1, float4 num2);
extern bool pg_float4_ne(float4 num1, float4 num2);
extern float4 pg_float4_pl(float4 num1, float4 num2);
extern float8 pg_float8_div(float8 num1, float8 num2);
extern bool pg_float8_eq(float8 num1, float8 num2);
extern bool pg_float8_ge(float8 num1, float8 num2);
extern bool pg_float8_gt(float8 num1, float8 num2);
extern bool pg_float8_le(float8 num1, float8 num2);
extern bool pg_float8_lt(float8 num1, float8 num2);
extern float8 pg_float8_mi(float8 num1, float8 num2);
extern float8 pg_float8_mul(float8 num1, float8 num2);
extern bool pg_float8_ne(float8 num1, float8 num2);
extern float8 pg_float8_pl(float8 num1, float8 num2);
extern float8 radians_internal(float8 num);
extern int32 width_bucket_float8_internal(float8 operand, float8 bound1, float8 bound2, int32 count);

/* Functions adadpted from timestamp.c */

extern void interval_negate(const Interval *interval, Interval *result);
extern Interval *pg_interval_justify_hours(const Interval *span);

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
