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
 * @brief Functions for base and time types corresponding to the external (SQL)
 * PostgreSQL functions
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

/*****************************************************************************/

/* Hash functions */

extern uint32 char_hash(char c);
extern uint64 char_hash_extended(char c, uint64 seed);
extern uint32 int16_hash(int16 val);
extern uint64 int16_hash_extended(int16 val, uint64 seed);
extern uint32 int32_hash(int32 val);
extern uint64 int32_hash_extended(int32 val, uint64 seed);
extern uint32 int64_hash(int64 val);
extern uint64 int64_hash_extended(int64 val, uint64 seed);
extern uint32 float4_hash(float4 key);
extern uint64 float4_hash_extended(float4 key, uint64 seed);
extern uint32 float8_hash(float8 key);
extern uint64 float8_hash_extended(float8 key, uint64 seed);
extern uint32 text_hash(text *key, Oid collid);
extern uint64 text_hash_extended(text *key, uint64 seed, Oid collid);

/* Functions for booleans */

extern bool bool_eq(bool arg1, bool arg2);
extern bool bool_ge(bool arg1, bool arg2);
extern bool bool_gt(bool arg1, bool arg2);
extern uint32 bool_hash(bool arg);
extern uint64 bool_hash_extended(bool arg, int64 seed);
extern bool bool_in(const char *str);
extern bool bool_le(bool arg1, bool arg2);
extern bool bool_lt(bool arg1, bool arg2);
extern bool bool_ne(bool arg1, bool arg2);
extern char *bool_out(bool b);
extern text *bool_to_text(bool b);

/* Functions for integers */

extern int32 bool_to_int32(bool arg);
extern int32 div_int16_int32(int16 arg1, int32 arg2);
extern int64 div_int16_int64(int16 num1, int64 num2);
extern int32 div_int32_int16(int32 arg1, int16 arg2);
extern int64 div_int32_int64(int32 num1, int64 num2);
extern int64 div_int64_int16(int64 num1, int16 num2);
extern int64 div_int64_int32(int64 num1, int32 num2);
extern bool eq_int16_int32(int16 arg1, int32 arg2);
extern bool eq_int16_int64(int16 num1, int64 num2);
extern bool eq_int32_int16(int32 arg1, int16 arg2);
extern bool eq_int32_int64(int32 num1, int64 num2);
extern bool eq_int64_int16(int64 num1, int16 num2);
extern bool eq_int64_int32(int64 num1, int32 num2);
extern int64 float4_to_int64(float4 num);
extern int64 float8_to_int64(float8 num);
extern bool ge_int16_int32(int16 arg1, int32 arg2);
extern bool ge_int16_int64(int16 num1, int64 num2);
extern bool ge_int32_int16(int32 arg1, int16 arg2);
extern bool ge_int32_int64(int32 num1, int64 num2);
extern bool ge_int64_int16(int64 num1, int16 num2);
extern bool ge_int64_int32(int64 num1, int32 num2);
extern bool gt_int16_int32(int16 arg1, int32 arg2);
extern bool gt_int16_int64(int16 num1, int64 num2);
extern bool gt_int32_int16(int32 arg1, int16 arg2);
extern bool gt_int32_int64(int32 num1, int64 num2);
extern bool gt_int64_int16(int64 num1, int16 num2);
extern bool gt_int64_int32(int64 num1, int32 num2);
extern int16 int16_abs(int16 arg);
extern int16 int16_and(int16 arg1, int16 arg2);
extern int16 int16_div(int16 arg1, int16 arg2);
extern bool int16_eq(int16 arg1, int16 arg2);
extern bool int16_ge(int16 arg1, int16 arg2);
extern bool int16_gt(int16 arg1, int16 arg2);
extern int16 int16_in(const char *str);
extern int16 int16_larger(int16 arg1, int16 arg2);
extern bool int16_le(int16 arg1, int16 arg2);
extern bool int16_lt(int16 arg1, int16 arg2);
extern int16 int16_mi(int16 arg1, int16 arg2);
extern int16 int16_mod(int16 arg1, int16 arg2);
extern int16 int16_mul(int16 arg1, int16 arg2);
extern bool int16_ne(int16 arg1, int16 arg2);
extern int16 int16_not(int16 arg);
extern int16 int16_or(int16 arg1, int16 arg2);
extern char *int16_out(int16 arg);
extern int16 int16_pl(int16 arg1, int16 arg2);
extern int16 int16_shl(int16 arg1, int32 arg2);
extern int16 int16_shr(int16 arg1, int32 arg2);
extern int16 int16_smaller(int16 arg1, int16 arg2);
extern int32 int16_to_int32(int16 arg);
extern int64 int16_to_int64(int16 num);
extern int16 int16_um(int16 arg);
extern int16 int16_up(int16 arg);
extern int16 int16_xor(int16 arg1, int16 arg2);
extern int32 int32_abs(int32 arg);
extern int32 int32_and(int32 arg1, int32 arg2);
extern int int32_cmp(int32 l, int32 r);
extern int32 int32_div(int32 arg1, int32 arg2);
extern bool int32_eq(int32 arg1, int32 arg2);
extern int32 int32_gcd(int32 arg1, int32 arg2);
extern bool int32_ge(int32 arg1, int32 arg2);
extern bool int32_gt(int32 arg1, int32 arg2);
extern int32 int32_in(const char *str);
extern int32 int32_inc(int32 arg);
extern int32 int32_larger(int32 arg1, int32 arg2);
extern int32 int32_lcm(int32 arg1, int32 arg2);
extern bool int32_le(int32 arg1, int32 arg2);
extern bool int32_lt(int32 arg1, int32 arg2);
extern int32 int32_mi(int32 arg1, int32 arg2);
extern int32 int32_mod(int32 arg1, int32 arg2);
extern int32 int32_mul(int32 arg1, int32 arg2);
extern bool int32_ne(int32 arg1, int32 arg2);
extern int32 int32_not(int32 arg);
extern int32 int32_or(int32 arg1, int32 arg2);
extern char *int32_out(int32 arg);
extern int32 int32_pl(int32 arg1, int32 arg2);
extern int32 int32_shl(int32 arg1, int32 arg2);
extern int32 int32_shr(int32 arg1, int32 arg2);
extern int32 int32_smaller(int32 arg1, int32 arg2);
extern bool int32_to_bool(int32 arg);
extern int16 int32_to_int16(int32 arg);
extern int64 int32_to_int64(int32 num);
extern int32 int32_um(int32 arg);
extern int32 int32_up(int32 arg);
extern int32 int32_xor(int32 arg1, int32 arg2);
extern int64 int64_abs(int64 num);
extern int64 int64_and(int64 num1, int64 num2);
extern int int64_cmp(int64 l, int64 r);
extern int64 int64_dec(int64 num);
extern int64 int64_div(int64 num1, int64 num2);
extern bool int64_eq(int64 num1, int64 num2);
extern int64 int64_gcd(int64 num1, int64 num2);
extern bool int64_ge(int64 num1, int64 num2);
extern bool int64_gt(int64 num1, int64 num2);
extern int64 int64_in(const char *str);
extern int64 int64_inc(int64 num);
extern int64 int64_larger(int64 num1, int64 num2);
extern int64 int64_lcm(int64 num1, int64 num2);
extern bool int64_le(int64 num1, int64 num2);
extern bool int64_lt(int64 num1, int64 num2);
extern int64 int64_mi(int64 num1, int64 num2);
extern int64 int64_mod(int64 num1, int64 num2);
extern int64 int64_mul(int64 num1, int64 num2);
extern bool int64_ne(int64 num1, int64 num2);
extern int64 int64_not(int64 num);
extern int64 int64_or(int64 num1, int64 num2);
extern char *int64_out(int64 num);
extern int64 int64_pl(int64 num1, int64 num2);
extern int64 int64_shl(int64 num1, int32 num2);
extern int64 int64_shr(int64 num1, int32 num2);
extern int64 int64_smaller(int64 num1, int64 num2);
extern float4 int64_to_float4(int64 num);
extern float8 int64_to_float8(int64 num);
extern int16 int64_to_int16(int64 num);
extern int32 int64_to_int32(int64 num);
extern int64 int64_um(int64 num);
extern int64 int64_up(int64 num);
extern int64 int64_xor(int64 num1, int64 num2);
extern bool le_int16_int32(int16 arg1, int32 arg2);
extern bool le_int16_int64(int16 num1, int64 num2);
extern bool le_int32_int16(int32 arg1, int16 arg2);
extern bool le_int32_int64(int32 num1, int64 num2);
extern bool le_int64_int16(int64 num1, int16 num2);
extern bool le_int64_int32(int64 num1, int32 num2);
extern bool lt_int16_int32(int16 arg1, int32 arg2);
extern bool lt_int16_int64(int16 num1, int64 num2);
extern bool lt_int32_int16(int32 arg1, int16 arg2);
extern bool lt_int32_int64(int32 num1, int64 num2);
extern bool lt_int64_int16(int64 num1, int16 num2);
extern bool lt_int64_int32(int64 num1, int32 num2);
extern int32 mi_int16_int32(int16 arg1, int32 arg2);
extern int64 mi_int16_int64(int16 num1, int64 num2);
extern int32 mi_int32_int16(int32 arg1, int16 arg2);
extern int64 mi_int32_int64(int32 num1, int64 num2);
extern int64 mi_int64_int16(int64 num1, int16 num2);
extern int64 mi_int64_int32(int64 num1, int32 num2);
extern int32 mul_int16_int32(int16 arg1, int32 arg2);
extern int64 mul_int16_int64(int16 num1, int64 num2);
extern int32 mul_int32_int16(int32 arg1, int16 arg2);
extern int64 mul_int32_int64(int32 num1, int64 num2);
extern int64 mul_int64_int16(int64 num1, int16 num2);
extern int64 mul_int64_int32(int64 num1, int32 num2);
extern bool ne_int16_int32(int16 arg1, int32 arg2);
extern bool ne_int16_int64(int16 num1, int64 num2);
extern bool ne_int32_int16(int32 arg1, int16 arg2);
extern bool ne_int32_int64(int32 num1, int64 num2);
extern bool ne_int64_int16(int64 num1, int16 num2);
extern bool ne_int64_int32(int64 num1, int32 num2);
extern int32 pl_int16_int32(int16 arg1, int32 arg2);
extern int64 pl_int16_int64(int16 num1, int64 num2);
extern int32 pl_int32_int16(int32 arg1, int16 arg2);
extern int64 pl_int32_int64(int32 num1, int64 num2);
extern int64 pl_int64_int16(int64 num1, int16 num2);
extern int64 pl_int64_int32(int64 num1, int32 num2);

/* Functions for numeric */

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
extern Numeric int32_to_numeric(int32 num);
extern int32 numeric_to_int32(Numeric num);
extern Numeric int64_to_numeric(int64 num);
extern int64 numeric_to_int64(Numeric num);
extern Numeric int16_to_numeric(int16 num);
extern int16 numeric_to_int16(Numeric num);
extern float8 numeric_to_float8(Numeric num);
extern float8 numeric_float8_no_overflow_internal(Numeric num);
extern Numeric float4_to_numeric(float4 num);
extern Numeric float8_to_numeric(float8 num);

/* Functions for floats */

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
extern uint32 float4_hash(float4 key);
extern uint64 float4_hash_extended(float4 key, uint64 seed);
extern float4 float4_in(char *num);
extern float4 float4_larger(float4 num1, float4 num2);
extern char *float4_out(float4 num);
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
extern float8 float84_div(float8 num1, float4 num2);
extern float8 float8_abs(float8 num);
extern float8 float8_acos(float8 num);
extern float8 float8_acosd(float8 num);
extern float8 float8_acosh(float8 num);
extern double float8_angular_difference(double degrees1, double degrees2);
extern float8 float8_asin(float8 num);
extern float8 float8_asind(float8 num);
extern float8 float8_asinh(float8 num);
extern float8 float8_atan(float8 num);
extern float8 float8_atan2(float8 num1, float8 num2);
extern float8 float8_atan2d(float8 num1, float8 num2);
extern float8 float8_atand(float8 num);
extern float8 float8_atanh(float8 num);
extern float8 float8_cbrt(float8 num);
extern float8 float8_ceil(float8 num);
extern int float8_cmp_internal(float8 a, float8 b);
extern float8 float8_cos(float8 num);
extern float8 float8_cosd(float8 num);
extern float8 float8_cosh(float8 num);
extern float8 float8_cot(float8 num);
extern float8 float8_cotd(float8 num);
extern float8 float8_exp(float8 num);
extern float8 float8_floor(float8 num);
extern float8 float8_gamma(float8 num);
extern uint32 float8_hash(float8 key);
extern uint64 float8_hash_extended(float8 key, uint64 seed);
extern float8 float8_in(const char *str);
extern float8 float8_larger(float8 num1, float8 num2);
extern float8 float8_lgamma(float8 num);
extern float8 float8_ln(float8 num);
extern float8 float8_log10(float8 num);
extern char *float8_out(double num, int maxdd);
extern float8 float8_pi(void);
extern float8 float8_pow(float8 num1, float8 num2);
extern float8 float8_round(float8 d, int maxdd);
extern float8 float8_rint(float8 num);
extern float8 float8_sign(float8 num);
extern float8 float8_sin(float8 num);
extern float8 float8_sind(float8 num);
extern float8 float8_sinh(float8 num);
extern float8 float8_smaller(float8 num1, float8 num2);
extern float8 float8_sqrt(float8 num);
extern float8 float8_tan(float8 num);
extern float8 float8_tand(float8 num);
extern float8 float8_tanh(float8 num);
extern float4 float8_to_float4(float8 num);
extern int16 float8_to_int2(float8 num);
extern int32 float8_to_int4(float8 num);
extern float8 float8_trunc(float8 num);
extern float8 float8_um(float8 num);
extern float8 float8_up(float8 num);
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
 
/* Functions for text */

extern uint32 char_hash(char c);
extern uint64 char_hash_extended(char c, uint64 seed);
extern text *cstring2text(const char *str);
extern text *int32_to_bin(int32 num);
extern text *int32_to_hex(int32 num);
extern text *int32_to_oct(int32 num);
extern text *int64_to_bin(int64 num);
extern text *int64_to_hex(int64 num);
extern text *int64_to_oct(int64 num);
extern bool pg_text_ge(text *txt1, text *txt2);
extern bool pg_text_gt(text *txt1, text *txt2);
extern text *pg_text_larger(text *txt1, text *txt2);
extern bool pg_text_le(text *txt1, text *txt2);
extern bool pg_text_lt(text *txt1, text *txt2);
extern text *pg_text_overlay(text *txt1, text *txt2, int sp, int sl);
extern Datum pg_text_pattern_ge(text *txt1, text *txt2);
extern Datum pg_text_pattern_gt(text *txt1, text *txt2);
extern Datum pg_text_pattern_le(text *txt1, text *txt2);
extern bool pg_text_pattern_lt(text *txt1, text *txt2);
extern Datum pg_text_reverse(text *txt);
extern text *pg_text_smaller(text *txt1, text *txt2);
extern char *text2cstring(const text *txt);
extern text *text_cat(text *txt1, text *txt2);
extern int text_cmp(const text *txt1, const text *txt2);
extern text *text_copy(const text *txt);
extern bool text_eq(text *txt1, text *txt2);
extern uint32 text_hash(text *key, Oid collid);
extern uint64 text_hash_extended(text *key, uint64 seed, Oid collid);
extern text *text_in(const char *str);
extern text *text_initcap(const text *txt);
extern int32 text_len(text *txt);
extern text *text_lower(const text *txt);
extern bool text_ne(text *txt1, text *txt2);
extern int32 text_octetlen(text *txt);
extern char *text_out(text *txt);
extern text *text_overlay_no_len(text *txt1, text *txt2, int sp);
extern text *text_upper(const text *txt);
extern text *textcat_text_text(const text *txt1, const text *txt2);

/* Functions for date */

extern int32 cmp_date_timestamp(DateADT date, Timestamp dt2);
extern int32 cmp_date_timestamptz(DateADT date, TimestampTz dt2);
extern int32 cmp_timestamp_date(Timestamp dt1, DateADT date);
extern int32 cmp_timestamptz_date(TimestampTz dt1, DateADT date);
extern Numeric date_extract(DateADT date, text *units);
extern uint32 date_hash(DateADT date);
extern uint64 date_hash_extended(DateADT date, int64 seed);
extern bool date_is_finite(DateADT date);
extern Timestamp date_to_timestamp(DateADT date);
extern Timestamp date_to_timestamptz(DateADT date);
extern bool eq_date_timestamp(DateADT date, Timestamp dt2);
extern bool eq_date_timestamptz(DateADT date, TimestampTz dt2);
extern bool eq_timestamp_date(Timestamp dt1, DateADT date);
extern bool eq_timestamptz_date(TimestampTz dt1, DateADT date);
extern bool ge_date_timestamp(DateADT date, Timestamp dt2);
extern bool ge_date_timestamptz(DateADT date, TimestampTz dt2);
extern bool ge_timestamp_date(Timestamp dt1, DateADT date);
extern bool ge_timestamptz_date(TimestampTz dt1, DateADT date);
extern bool gt_date_timestamp(DateADT date, Timestamp dt2);
extern bool gt_date_timestamptz(DateADT date, TimestampTz dt2);
extern bool gt_timestamp_date(Timestamp dt1, DateADT date);
extern bool gt_timestamptz_date(TimestampTz dt1, DateADT date);
extern bool le_date_timestamp(DateADT date, Timestamp dt2);
extern bool le_date_timestamptz(DateADT date, TimestampTz dt2);
extern bool le_timestamp_date(Timestamp dt1, DateADT date);
extern bool le_timestamptz_date(TimestampTz dt1, DateADT date);
extern bool lt_date_timestamp(DateADT date, Timestamp dt2);
extern bool lt_date_timestamptz(DateADT date, TimestampTz dt2);
extern bool lt_timestamp_date(Timestamp dt1, DateADT date);
extern bool lt_timestamptz_date(TimestampTz dt1, DateADT date);
extern Interval * minus_date_date(DateADT date1, DateADT date2);
extern DateADT minus_date_int(DateADT date, int32 days);
extern DateADT minus_date_interval(DateADT date, Interval *span);
extern bool ne_date_timestamp(DateADT date, Timestamp dt2);
extern bool ne_date_timestamptz(DateADT date, TimestampTz dt2);
extern bool ne_timestamp_date(Timestamp dt1, DateADT date);
extern bool ne_timestamptz_date(TimestampTz dt1, DateADT date);
extern int pg_date_cmp(DateADT date1, DateADT date2);
extern bool pg_date_eq(DateADT date1, DateADT date2);
extern bool pg_date_ge(DateADT date1, DateADT date2);
extern bool pg_date_gt(DateADT date1, DateADT date2);
extern DateADT pg_date_in(const char *str);
extern DateADT pg_date_larger(DateADT date1, DateADT date2);
extern bool pg_date_le(DateADT date1, DateADT date2);
extern bool pg_date_lt(DateADT date1, DateADT date2);
extern DateADT pg_date_make(int year, int mon, int mday);
extern bool pg_date_ne(DateADT date1, DateADT date2);
extern char * pg_date_out(DateADT date);
extern DateADT pg_date_smaller(DateADT date1, DateADT date2);
extern DateADT add_date_int(DateADT date, int32 days);
extern DateADT add_date_interval(DateADT date, Interval *span);
extern bool time_overlaps(TimeADT *ts1, TimeADT *te1, TimeADT *ts2, TimeADT *te2);
extern DateADT timestamp_to_date(Timestamp timestamp);
extern DateADT timestamptz_to_date(TimestampTz timestamp);
 
/* Functions for time */

extern TimeADT interval_to_time(Interval *span);
extern TimeADT minus_time_interval(TimeADT date, Interval *span);
extern Interval *minus_time_time(TimeADT time1, TimeADT time2);
extern int pg_time_cmp(TimeADT time1, TimeADT time2);
extern bool pg_time_eq(TimeADT time1, TimeADT time2);
extern bool pg_time_ge(TimeADT time1, TimeADT time2);
extern bool pg_time_gt(TimeADT time1, TimeADT time2);
extern uint32 pg_time_hash(TimeADT time);
extern uint64 pg_time_hash_extended(TimeADT time, int32 seed);
extern TimeADT pg_time_in(const char *str, int32 typmod);
extern TimeADT pg_time_larger(TimeADT time1, TimeADT time2);
extern bool pg_time_le(TimeADT time1, TimeADT time2);
extern bool pg_time_lt(TimeADT time1, TimeADT time2);
extern bool pg_time_ne(TimeADT time1, TimeADT time2);
extern char *pg_time_out(TimeADT time);
extern float8 pg_time_part(TimeADT time, const text *units);
extern TimeADT pg_time_scale(TimeADT date, int32 typmod);
extern TimeADT pg_time_smaller(TimeADT time1, TimeADT time2);
extern TimeADT plus_time_interval(TimeADT date, Interval *span);
extern Numeric time_extract(TimeADT time, const text *units);
extern TimeADT time_make(int tm_hour, int tm_min, double sec);
extern Interval *time_to_interval(TimeADT date);
extern TimeADT timestamp_to_time(Timestamp timestamp);
extern TimeADT timestamptz_to_time(TimestampTz timestamp);
 
/* Functions for timestamps */

extern Timestamp add_timestamp_interval(Timestamp timestamp, const Interval *interval);
extern Timestamp add_timestamptz_interval(TimestampTz timestamp, const Interval *interval);
extern Timestamp add_timestamptz_interval_at_zone(TimestampTz timestamp, const Interval *interval, const text *zone);
extern int32 cmp_timestamp_timestamptz(Timestamp timestamp, TimestampTz dt2);
extern int32 cmp_timestamptz_timestamp(TimestampTz dt1, Timestamp timestamp);
extern bool eq_timestamp_timestamptz(Timestamp timestamp, TimestampTz dt2);
extern bool eq_timestamptz_timestamp(TimestampTz dt1, Timestamp timestamp);
extern TimestampTz float8_to_timestamptz(float8 seconds);
extern bool ge_timestamp_timestamptz(Timestamp timestamp, TimestampTz dt2);
extern bool ge_timestamptz_timestamp(TimestampTz dt1, Timestamp timestamp);
extern bool gt_timestamp_timestamptz(Timestamp timestamp, TimestampTz dt2);
extern bool gt_timestamptz_timestamp(TimestampTz dt1, Timestamp timestamp);
extern bool le_timestamp_timestamptz(Timestamp timestamp, TimestampTz dt2);
extern bool le_timestamptz_timestamp(TimestampTz dt1, Timestamp timestamp);
extern bool lt_timestamp_timestamptz(Timestamp timestamp, TimestampTz dt2);
extern bool lt_timestamptz_timestamp(TimestampTz dt1, Timestamp timestamp);
extern Timestamp minus_timestamp_interval(Timestamp timestamp, const Interval *interval);
extern TimestampTz minus_timestamptz_interval(TimestampTz timestamp, const Interval *interval);
extern TimestampTz minus_timestamptz_interval_at_zone(TimestampTz timestamp, const Interval *interval, const text *zone);
extern bool ne_timestamp_timestamptz(Timestamp timestamp, TimestampTz dt2);
extern bool ne_timestamptz_timestamp(TimestampTz dt1, Timestamp timestamp);
extern Interval *pg_interval_justify_days(const Interval *interval);
extern Interval *pg_interval_justify_hours(const Interval *interval);
extern text *pg_timeofday(void);
extern Interval *pg_timestamp_age(Timestamp dt1, Timestamp dt2);
extern TimestampTz pg_timestamp_at_local(Timestamp timestamp);
extern Timestamp pg_timestamp_bin(const Interval *stride, Timestamp timestamp, Timestamp origin);
extern int32 pg_timestamp_cmp(Timestamp dt1, Timestamp dt2);
extern bool pg_timestamp_eq(Timestamp dt1, Timestamp dt2);
extern bool pg_timestamp_ge(Timestamp dt1, Timestamp dt2);
extern bool pg_timestamp_gt(Timestamp dt1, Timestamp dt2);
extern int32 pg_timestamp_hash(Timestamp timestamp);
extern uint64 pg_timestamp_hash_extended(TimestampTz timestamp, uint64 seed);
extern Timestamp pg_timestamp_in(const char *str, int32 typmod);
extern TimestampTz pg_timestamp_izone(Timestamp timestamp, const Interval *zone);
extern Timestamp pg_timestamp_larger(Timestamp dt1, Timestamp dt2);
extern bool pg_timestamp_le(Timestamp dt1, Timestamp dt2);
extern bool pg_timestamp_lt(Timestamp dt1, Timestamp dt2);
extern Interval *minus_timestamp_timestamp(Timestamp dt1, Timestamp dt2);
extern Interval *minus_timestamptz_timestamptz(TimestampTz dt1, TimestampTz dt2);
extern bool pg_timestamp_ne(Timestamp dt1, Timestamp dt2);
extern char *pg_timestamp_out(Timestamp timestamp);
extern float8 pg_timestamp_part(Timestamp timestamp, const text *units);
extern Timestamp pg_timestamp_scale(Timestamp timestamp, int32 typmod);
extern Timestamp pg_timestamp_smaller(Timestamp dt1, Timestamp dt2);
extern Timestamp pg_timestamp_trunc(const text *units, Timestamp timestamp);
extern TimestampTz pg_timestamp_zone(Timestamp timestamp, const text *zone);
extern Interval *pg_timestamptz_age(TimestampTz dt1, TimestampTz dt2);
extern TimestampTz pg_timestamptz_bin(const Interval *stride, TimestampTz timestamp, TimestampTz origin);
extern int32 pg_timestamptz_hash(TimestampTz timestamp);
extern uint64 pg_timestamptz_hash_extended(TimestampTz timestamp, uint64 seed);
extern TimestampTz pg_timestamptz_in(const char *str, int32 typmod);
extern Timestamp pg_timestamptz_izone(const Interval *zone, TimestampTz timestamp);
extern char *pg_timestamptz_out(TimestampTz dt);
extern float8 pg_timestamptz_part(TimestampTz timestamp, const text *units);
extern TimestampTz pg_timestamptz_scale(TimestampTz timestamp, int32 typmod);
extern TimestampTz pg_timestamptz_trunc(const text *units, TimestampTz timestamp);
extern TimestampTz pg_timestamptz_trunc_zone(const text *units, TimestampTz timestamp, const text *zone);
extern Timestamp pg_timestamptz_zone(const text *zone, TimestampTz timestamp);
extern Numeric timestamp_extract(Timestamp timestamp, const text *units);
extern bool timestamp_is_finite(Timestamp timestamp);
extern Timestamp timestamp_make(int32 year, int32 month, int32 mday, int32 hour, int32 min, float8 sec);
extern bool timestamp_overlaps(Timestamp ts1, Timestamp te1, Timestamp ts2, Timestamp te2);
extern TimestampTz timestamp_to_timestamptz(Timestamp timestamp);
extern TimestampTz timestamptz_at_local(TimestampTz timestamp);
extern Interval *timestamptz_extract(TimestampTz timestamp, const text *units);
extern TimestampTz timestamptz_make(int year, int month, int day, int hour, int min, double sec);
extern TimestampTz timestamptz_make_at_timezone(int year, int month, int day, int hour, int min, double sec, const text *zone);
extern TimestampTz timestamptz_shift(TimestampTz t, const Interval *interv);
extern Timestamp timestamptz_to_timestamp(TimestampTz timestamp);

/* Functions for intervals */

extern Interval *add_interval_interval(const Interval *interval1, const Interval *interval2);
extern Interval *div_interval_float8(const Interval *interval, float8 factor);
extern Numeric interval_extract(const Interval *interval, const text *units);
extern bool interval_is_finite(const Interval *interval);
extern Interval *interval_make(int32 years, int32 months, int32 weeks, int32 days, int32 hours, int32 mins, double secs);
extern Interval *interval_negate(const Interval *interval);
extern Interval *minus_interval_interval(const Interval *interval1, const Interval *interval2);
extern Interval *mul_float8_interval(float8 factor, const Interval *interval);
extern Interval *mul_interval_float8(const Interval *interval, float8 factor);
extern int32 pg_interval_cmp(const Interval *interval1, const Interval *interval2);
extern bool pg_interval_eq(const Interval *interval1, const Interval *interval2);
extern bool pg_interval_ge(const Interval *interval1, const Interval *interval2);
extern bool pg_interval_gt(const Interval *interval1, const Interval *interval2);
extern int32 pg_interval_hash(const Interval *interval);
extern uint64 pg_interval_hash_extended(const Interval *interval, uint64 seed);
extern Interval *pg_interval_in(const char *str, int32 typmod);
extern Interval *pg_interval_justify_interval(const Interval *interval);
extern Interval *pg_interval_larger(const Interval *interval1, const Interval *interval2);
extern bool pg_interval_le(const Interval *interval1, const Interval *interval2);
extern bool pg_interval_lt(const Interval *interval1, const Interval *interval2);
extern bool pg_interval_ne(const Interval *interval1, const Interval *interval2);
extern char *pg_interval_out(const Interval *interval);
extern float8 pg_interval_part(const Interval *interval, const text *units);
extern Interval *pg_interval_scale(const Interval *interval, int32 typmod);
extern Interval *pg_interval_smaller(const Interval *interval1, const Interval *interval2);
extern Interval *pg_interval_trunc(const text *units, const Interval *interval);

/*****************************************************************************/

#endif /* POSTGRES_TYPES_H */
