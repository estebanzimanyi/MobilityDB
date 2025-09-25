/*-------------------------------------------------------------------------
 *
 * int8.c
 *    Internal 64-bit integer operations
 *
 * Portions Copyright (c) 1996-2025, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *    src/backend/utils/adt/int8.c
 *
 *-------------------------------------------------------------------------
 */

/* C */
#include <assert.h>
#include <limits.h>
/* PostgreSQL */
#include "postgres.h"
#include "common/int.h"
/* MEOS */
#include "../include/meos.h"

/* Sign + the most decimal digits an 8-byte number could have */
#define MAXINT8LEN 20

extern int pg_lltoa(int64 value, char *a);
extern int64 pg_strtoint64_safe(const char *s, void *escontext);

/***********************************************************************
 **
 **    Routines for 64-bit integers.
 **
 ***********************************************************************/

/*----------------------------------------------------------
 * Formatting and conversion routines.
 *---------------------------------------------------------*/

/**
 * @ingroup meos_base_int
 * @brief Return an int64 number from its string representation
 * @note Derived from PostgreSQL function @p int8in()
 */
int64
int8_in(char *str)
{
  return pg_strtoint64_safe(str, NULL);
}

/**
 * @ingroup meos_base_int
 * @brief Return the string representation of an int64 number
 * @note Derived from PostgreSQL function @p int8out()
 */
char *
int8_out(int64 num)
{
  char    buf[MAXINT8LEN + 1];
  char     *result;
  int      len;

  len = pg_lltoa(num, buf) + 1;

  /*
   * Since the length is already known, we do a manual palloc() and memcpy()
   * to avoid the strlen() call that would otherwise be done in pstrdup().
   */
  result = palloc(len);
  memcpy(result, buf, len);
  return result;
}

/*----------------------------------------------------------
 *  Relational operators for int8s, including cross-data-type comparisons.
 *---------------------------------------------------------*/

/**
 * @ingroup meos_base_int
 * @brief Return true if two int64 numbers are equal
 * @note Derived from PostgreSQL function @p int8eq()
 */
bool
int8_eq(int64 num1, int64 num2)
{
  return (num1 == num2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if two int64 numbers are not equal
 * @note Derived from PostgreSQL function @p int8ne()
 */
bool
int8_ne(int64 num1, int64 num2)
{
  return (num1 != num2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if the first int64 number is less than the second one
 * @note Derived from PostgreSQL function @p int8lt()
 */
bool
int8_lt(int64 num1, int64 num2)
{
  return (num1 < num2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if the first int64 number is greater than the second one
 * @note Derived from PostgreSQL function @p int8gt()
 */
bool
int8_gt(int64 num1, int64 num2)
{
  return (num1 > num2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if the first int64 number is less than or equal to the
 * second one
 * @note Derived from PostgreSQL function @p int8le()
 */
bool
int8_le(int64 num1, int64 num2)
{
  return (num1 <= num2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if the first int64 number is greater than or equal to the
 * second one
 * @note Derived from PostgreSQL function @p int8ge()
 */
bool
int8_ge(int64 num1, int64 num2)
{
  return (num1 >= num2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if an int64 number is equal to an int32 number
 * @note Derived from PostgreSQL function @p int84eq()
 */
bool
int84_eq(int64 num1, int32 num2)
{
  return (num1 == num2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if an int64 number is not equal to an int32 number
 * @note Derived from PostgreSQL function @p int84ne()
 */
bool
int84_ne(int64 num1, int32 num2)
{
  return (num1 != num2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if an int64 number is less than an int32 number
 * @note Derived from PostgreSQL function @p int84lt()
 */
bool
int84_lt(int64 num1, int32 num2)
{
  return (num1 < num2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if an int64 number is greater than an int32 number
 * @note Derived from PostgreSQL function @p int84gt()
 */
bool
int84_gt(int64 num1, int32 num2)
{
  return (num1 > num2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if an int64 number is less than or equal to an
 * int32 number
 * @note Derived from PostgreSQL function @p int84le()
 */
bool
int84_le(int64 num1, int32 num2)
{
  return (num1 <= num2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if an int64 number is greater than or equal to an int32
 * number
 * @note Derived from PostgreSQL function @p int84ge()
 */
bool
int84_ge(int64 num1, int32 num2)
{
  return (num1 >= num2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if an int32 number is equal to an int64 number
 * @note Derived from PostgreSQL function @p int48eq()
 */
bool
int48_eq(int32 num1, int64 num2)
{
  return (num1 == num2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if an int32 number is not equal to an int64 number
 * @note Derived from PostgreSQL function @p int48ne()
 */
bool
int48_ne(int32 num1, int64 num2)
{
  return (num1 != num2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if an int32 number is less than an int64 number
 * @note Derived from PostgreSQL function @p int48lt()
 */
bool
int48_lt(int32 num1, int64 num2)
{
  return (num1 < num2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if an int32 number is greater than an int64 number
 * @note Derived from PostgreSQL function @p int48gt()
 */
bool
int48_gt(int32 num1, int64 num2)
{
  return (num1 > num2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if an int32 number is less than or equal to an int64
 * number
 * @note Derived from PostgreSQL function @p int48le()
 */
bool
int48_le(int32 num1, int64 num2)
{
  return (num1 <= num2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if an int32 number is greater than or equal to an int64
 * number
 * @note Derived from PostgreSQL function @p int48ge()
 */
bool
int48_ge(int32 num1, int64 num2)
{
  return (num1 >= num2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if an int64 number is equal to an int16 number
 * @note Derived from PostgreSQL function @p int82eq()
 */
bool
int82_eq(int64 num1, int16 num2)
{
  return (num1 == num2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if an int64 number is not equal to an int16 number
 * @note Derived from PostgreSQL function @p int82ne()
 */
bool
int82_ne(int64 num1, int16 num2)
{
  return (num1 != num2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if an int64 number is less than an int16 number
 * @note Derived from PostgreSQL function @p int82lt()
 */
bool
int82_lt(int64 num1, int16 num2)
{
  return (num1 < num2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if an int64 number is greater than an int16 number
 * @note Derived from PostgreSQL function @p int82gt()
 */
bool
int82_gt(int64 num1, int16 num2)
{
  return (num1 > num2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if an int64 number is less than or equal to an int16
 * number
 * @note Derived from PostgreSQL function @p int82le()
 */
bool
int82_le(int64 num1, int16 num2)
{
  return (num1 <= num2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if an int64 number is greater than or equal to an int16
 * number
 * @note Derived from PostgreSQL function @p int82ge()
 */
bool
int82_ge(int64 num1, int16 num2)
{
  return (num1 >= num2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if an int16 number is equal to an int64 number
 * @note Derived from PostgreSQL function @p int28eq()
 */
bool
int28_eq(int16 num1, int64 num2)
{
  return (num1 == num2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if an int16 number is not equal to an int64 number
 * @note Derived from PostgreSQL function @p int28ne()
 */
bool
int28_ne(int16 num1, int64 num2)
{
  return (num1 != num2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if an int16 number is less than an int64 number
 * @note Derived from PostgreSQL function @p int28lt()
 */
bool
int28_lt(int16 num1, int64 num2)
{
  return (num1 < num2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if an int16 number is greater than an int64 number
 * @note Derived from PostgreSQL function @p int28gt()
 */
bool
int28_gt(int16 num1, int64 num2)
{
  return (num1 > num2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if an int16 number is less than or equal to an int64
 * number
 * @note Derived from PostgreSQL function @p int28le()
 */
bool
int28_le(int16 num1, int64 num2)
{
  return (num1 <= num2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if an int16 number is greater than or equal to an int64
 * number
 * @note Derived from PostgreSQL function @p int28ge()
 */
bool
int28_ge(int16 num1, int64 num2)
{
  return (num1 >= num2);
}

/*----------------------------------------------------------
 *  Arithmetic operators on 64-bit integers.
 *---------------------------------------------------------*/

/**
 * @ingroup meos_base_int
 * @brief Return the unary minus of an int64 number
 * @note Derived from PostgreSQL function @p int8um()
 */
int64
int8_um(int64 num)
{
  int64 result;
  if (unlikely(num == PG_INT64_MIN))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "bigint out of range");
    return LONG_MAX;
  }
  result = -num;
  return result;
}

/**
 * @ingroup meos_base_int
 * @brief Return the unary plus of an int64 number
 * @note Derived from PostgreSQL function @p int8up()
 */
int64
int8_up(int64 num)
{
  return num;
}

/**
 * @ingroup meos_base_int
 * @brief Return the addition of two int64 numbers
 * @note Derived from PostgreSQL function @p int8pl()
 */
int64
int8_pl(int64 num1, int64 num2)
{
  int64 result;
  if (unlikely(pg_add_s64_overflow(num1, num2, &result)))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "bigint out of range");
    return LONG_MAX;
  }
  return result;
}

/**
 * @ingroup meos_base_int
 * @brief Return the subtraction of two int64 numbers
 * @note Derived from PostgreSQL function @p int8mi()
 */
int64
int8_mi(int64 num1, int64 num2)
{
  int64 result;
  if (unlikely(pg_sub_s64_overflow(num1, num2, &result)))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "bigint out of range");
    return LONG_MAX;
  }
  return result;
}

/**
 * @ingroup meos_base_int
 * @brief Return the multiplication of two int64 numbers
 * @note Derived from PostgreSQL function @p int8mul()
 */
int64
int8_mul(int64 num1, int64 num2)
{
  int64 result;
  if (unlikely(pg_mul_s64_overflow(num1, num2, &result)))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "bigint out of range");
    return LONG_MAX;
  }
  return result;
}

/**
 * @ingroup meos_base_int
 * @brief Return the division of two int64 numbers
 * @note Derived from PostgreSQL function @p int8div()
 */
int64
int8_div(int64 num1, int64 num2)
{
  int64 result;

  if (num2 == 0)
  {
    meos_error(ERROR, MEOS_ERR_DIVISION_BY_ZERO, "division by zero");
    return LONG_MAX;
  }

  /*
   * INT64_MIN / -1 is problematic, since the result can't be represented on
   * a two's-complement machine.  Some machines produce INT64_MIN, some
   * produce zero, some throw an exception.  We can dodge the problem by
   * recognizing that division by -1 is the same as negation.
   */
  if (num2 == -1)
  {
    if (unlikely(num1 == PG_INT64_MIN))
    {
      meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "bigint out of range");
      return LONG_MAX;
    }
    result = -num1;
    return result;
  }

  /* No overflow is possible */
  return num1 / num2;
}

/**
 * @ingroup meos_base_int
 * @brief Return the absolute value of an int64 number
 * @note Derived from PostgreSQL function @p int8abs()
 */
int64
int8_abs(int64 num)
{
  int64 result;
  if (unlikely(num == PG_INT64_MIN))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "bigint out of range");
    return LONG_MAX;
  }
  return (num < 0) ? -num : num;
}

/**
 * @ingroup meos_base_int
 * @brief Return the modulo of an int64 number by another one
 * @note Derived from PostgreSQL function @p int8mod()
 */
int64
int8_mod(int64 num1, int64 num2)
{

  if (unlikely(num2 == 0))
  {
    meos_error(ERROR, MEOS_ERR_DIVISION_BY_ZERO, "division by zero");
    return LONG_MAX;
  }

  /*
   * Some machines throw a floating-point exception for INT64_MIN % -1,
   * which is a bit silly since the correct answer is perfectly
   * well-defined, namely zero.
   */
  if (num2 == -1)
    return (0);

  /* No overflow is possible */

  return (num1 % num2);
}

/*
 * Greatest Common Divisor
 *
 * Returns the largest positive integer that exactly divides both inputs.
 * Special cases:
 *   - gcd(x, 0) = gcd(0, x) = abs(x)
 *       because 0 is divisible by anything
 *   - gcd(0, 0) = 0
 *       complies with the previous definition and is a common convention
 *
 * Special care must be taken if either input is INT64_MIN ---
 * gcd(0, INT64_MIN), gcd(INT64_MIN, 0) and gcd(INT64_MIN, INT64_MIN) are
 * all equal to abs(INT64_MIN), which cannot be represented as a 64-bit signed
 * integer.
 */
static int64
int8gcd_internal(int64 num1, int64 num2)
{
  int64 swap;
  int64 a1, a2;

  /*
   * Put the greater absolute value in num1.
   *
   * This would happen automatically in the loop below, but avoids an
   * expensive modulo operation, and simplifies the special-case handling
   * for INT64_MIN below.
   *
   * We do this in negative space in order to handle INT64_MIN.
   */
  a1 = (num1 < 0) ? num1 : -num1;
  a2 = (num2 < 0) ? num2 : -num2;
  if (a1 > a2)
  {
    swap = num1;
    num1 = num2;
    num2 = swap;
  }

  /* Special care needs to be taken with INT64_MIN.  See comments above. */
  if (num1 == PG_INT64_MIN)
  {
    if (num2 == 0 || num2 == PG_INT64_MIN)
    {
      meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "bigint out of range");
      return LONG_MAX;
    }

    /*
     * Some machines throw a floating-point exception for INT64_MIN % -1,
     * which is a bit silly since the correct answer is perfectly
     * well-defined, namely zero.  Guard against this and just return the
     * result, gcd(INT64_MIN, -1) = 1.
     */
    if (num2 == -1)
      return 1;
  }

  /* Use the Euclidean algorithm to find the GCD */
  while (num2 != 0)
  {
    swap = num2;
    num2 = num1 % num2;
    num1 = swap;
  }

  /*
   * Make sure the result is positive. (We know we don't have INT64_MIN
   * anymore).
   */
  if (num1 < 0)
    num1 = -num1;

  return num1;
}

/**
 * @ingroup meos_base_int
 * @brief Return the greatest common denominator of an int64 number
 * @note Derived from PostgreSQL function @p int8gcd()
 */
int64
int8_gcd(int64 num1, int64 num2)
{
  return int8gcd_internal(num1, num2);
}

/**
 * @ingroup meos_base_int
 * @brief Return the least common multiple of an int64 number
 * @note Derived from PostgreSQL function @p int8lcm()
 */
int64
int8_lcm(int64 num1, int64 num2)
{
  int64 gcd;
  int64 result;

  /*
   * Handle lcm(x, 0) = lcm(0, x) = 0 as a special case.  This prevents a
   * division-by-zero error below when x is zero, and an overflow error from
   * the GCD computation when x = INT64_MIN.
   */
  if (num1 == 0 || num2 == 0)
    return 0;

  /* lcm(x, y) = abs(x / gcd(x, y) * y) */
  gcd = int8gcd_internal(num1, num2);
  num1 = num1 / gcd;

  if (unlikely(pg_mul_s64_overflow(num1, num2, &result)))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "bigint out of range");
    return LONG_MAX;
  }

  /* If the result is INT64_MIN, it cannot be represented. */
  if (unlikely(result == PG_INT64_MIN))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "bigint out of range");
    return LONG_MAX;
  }

  if (result < 0)
    result = -result;

  return result;
}

/**
 * @ingroup meos_base_int
 * @brief Return an int64 number incremented by one
 * @note Derived from PostgreSQL function @p int8inc()
 */
int64
int8_inc(int64 num)
{
  int64 result;
  if (unlikely(pg_add_s64_overflow(num, 1, &result)))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "bigint out of range");
    return LONG_MAX;
  }
  return result;
}

/**
 * @ingroup meos_base_int
 * @brief Return an int64 number decremented by one
 * @note Derived from PostgreSQL function @p int8dec()
 */
int64
int8_dec(int64 num)
{
  int64 result;
  if (unlikely(pg_sub_s64_overflow(num, 1, &result)))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "bigint out of range");
    return LONG_MAX;
  }
  return result;
}

/**
 * @ingroup meos_base_int
 * @brief Return the larger of two int64 numbers
 * @note Derived from PostgreSQL function @p int8larger()
 */
int64
int8_larger(int64 num1, int64 num2)
{
  return ((num1 > num2) ? num1 : num2);
}

/**
 * @ingroup meos_base_int
 * @brief Return the smaller of two int64 numbers
 * @note Derived from PostgreSQL function @p int8smaller()
 */
int64
int8_smaller(int64 num1, int64 num2)
{
  return ((num1 < num2) ? num1 : num2);
}

/**
 * @ingroup meos_base_int
 * @brief Return the addition of an int64 number and an int32 number
 * @note Derived from PostgreSQL function @p int84pl()
 */
int64
int84_pl(int64 num1, int32 num2)
{
  int64 result;
  if (unlikely(pg_add_s64_overflow(num1, (int64) num2, &result)))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "bigint out of range");
    return LONG_MAX;
  }
  return result;
}

/**
 * @ingroup meos_base_int
 * @brief Return the subtraction of an int64 number and an int32 number
 * @note Derived from PostgreSQL function @p int84mi()
 */
int64
int84_mi(int64 num1, int32 num2)
{
  int64 result;
  if (unlikely(pg_sub_s64_overflow(num1, (int64) num2, &result)))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "bigint out of range");
    return LONG_MAX;
  }
  return result;
}

/**
 * @ingroup meos_base_int
 * @brief Return the multiplication of an int64 number and an int32 number
 * @note Derived from PostgreSQL function @p int84mul()
 */
int64
int84_mul(int64 num1, int32 num2)
{
  int64 result;
  if (unlikely(pg_mul_s64_overflow(num1, (int64) num2, &result)))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "bigint out of range");
    return LONG_MAX;
  }
  return result;
}

/**
 * @ingroup meos_base_int
 * @brief Return the division of an int64 number and an int32 number
 * @note Derived from PostgreSQL function @p int84div()
 */
int64
int84_div(int64 num1, int32 num2)
{
  int64 result;
  if (num2 == 0)
  {
    meos_error(ERROR, MEOS_ERR_DIVISION_BY_ZERO, "division by zero");
    return LONG_MAX;
  }

  /*
   * INT64_MIN / -1 is problematic, since the result can't be represented on
   * a two's-complement machine.  Some machines produce INT64_MIN, some
   * produce zero, some throw an exception.  We can dodge the problem by
   * recognizing that division by -1 is the same as negation.
   */
  if (num2 == -1)
  {
    if (unlikely(num1 == PG_INT64_MIN))
    {
      meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "bigint out of range");
      return LONG_MAX;
    }
    result = -num1;
    return result;
  }

  /* No overflow is possible */
  return num1 / num2;
}

/**
 * @ingroup meos_base_int
 * @brief Return the addition of an int32 number and an int64 number
 * @note Derived from PostgreSQL function @p int48pl()
 */
int64
int48_pl(int32 num1, int64 num2)
{
  int64 result;
  if (unlikely(pg_add_s64_overflow((int64) num1, num2, &result)))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "bigint out of range");
    return LONG_MAX;
  }
  return result;
}

/**
 * @ingroup meos_base_int
 * @brief Return the subtraction of an int32 number and an int64 number
 * @note Derived from PostgreSQL function @p int48mi()
 */
int64
int48_mi(int32 num1, int64 num2)
{
  int64 result;
  if (unlikely(pg_sub_s64_overflow((int64) num1, num2, &result)))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "bigint out of range");
    return LONG_MAX;
  }
  return result;
}

/**
 * @ingroup meos_base_int
 * @brief Return the multiplication of an int32 number and an int64 number
 * @note Derived from PostgreSQL function @p int48mul()
 */
int64
int48_mul(int32 num1, int64 num2)
{
  int64 result;
  if (unlikely(pg_mul_s64_overflow((int64) num1, num2, &result)))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "bigint out of range");
    return LONG_MAX;
  }
  return result;
}

/**
 * @ingroup meos_base_int
 * @brief Return the division of an int32 number and an int64 number
 * @note Derived from PostgreSQL function @p int48div()
 */
int64
int48_div(int32 num1, int64 num2)
{
  if (unlikely(num2 == 0))
  {
    meos_error(ERROR, MEOS_ERR_DIVISION_BY_ZERO, "division by zero");
    return LONG_MAX;
  }

  /* No overflow is possible */
  return ((int64) num1 / num2);
}

/**
 * @ingroup meos_base_int
 * @brief Return the addition of an int64 number and an int16 number
 * @note Derived from PostgreSQL function @p int82pl()
 */
int64
int82_pl(int64 num1, int16 num2)
{
  int64 result;
  if (unlikely(pg_add_s64_overflow(num1, (int64) num2, &result)))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "bigint out of range");
    return LONG_MAX;
  }
  return result;
}

/**
 * @ingroup meos_base_int
 * @brief Return the subtraction of an int64 number and an int16 number
 * @note Derived from PostgreSQL function @p int82mi()
 */
int64
int82_mi(int64 num1, int16 num2)
{
  int64 result;
  if (unlikely(pg_sub_s64_overflow(num1, (int64) num2, &result)))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "bigint out of range");
    return LONG_MAX;
  }
  return result;
}

/**
 * @ingroup meos_base_int
 * @brief Return the multiplication of an int64 number and an int16 number
 * @note Derived from PostgreSQL function @p int82mul()
 */
int64
int82_mul(int64 num1, int16 num2)
{
  int64 result;
  if (unlikely(pg_mul_s64_overflow(num1, (int64) num2, &result)))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "bigint out of range");
    return LONG_MAX;
  }
  return result;
}

/**
 * @ingroup meos_base_int
 * @brief Return the division of an int64 number and an int16 number
 * @note Derived from PostgreSQL function @p int82div()
 */
int64
int82_div(int64 num1, int16 num2)
{
  int64 result;
  if (unlikely(num2 == 0))
  {
    meos_error(ERROR, MEOS_ERR_DIVISION_BY_ZERO, "division by zero");
    return LONG_MAX;
  }

  /*
   * INT64_MIN / -1 is problematic, since the result can't be represented on
   * a two's-complement machine.  Some machines produce INT64_MIN, some
   * produce zero, some throw an exception.  We can dodge the problem by
   * recognizing that division by -1 is the same as negation.
   */
  if (num2 == -1)
  {
    if (unlikely(num1 == PG_INT64_MIN))
    {
      meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "bigint out of range");
    }
    result = -num1;
    return result;
  }

  /* No overflow is possible */
  return num1 / num2;
}

/**
 * @ingroup meos_base_int
 * @brief Return the addition of an int16 number and an int64 number
 * @note Derived from PostgreSQL function @p int28pl()
 */
int64
int28_pl(int16 num1, int64 num2)
{
  int64 result;
  if (unlikely(pg_add_s64_overflow((int64) num1, num2, &result)))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "bigint out of range");
    return LONG_MAX;
  }
  return result;
}

/**
 * @ingroup meos_base_int
 * @brief Return the subraction of an int16 number and an int64 number
 * @note Derived from PostgreSQL function @p int28mi()
 */
int64
int28_mi(int16 num1, int64 num2)
{
  int64 result;
  if (unlikely(pg_sub_s64_overflow((int64) num1, num2, &result)))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "bigint out of range");
    return LONG_MAX;
  }
  return result;
}

/**
 * @ingroup meos_base_int
 * @brief Return the multiplication of an int16 number and an int64 number
 * @note Derived from PostgreSQL function @p int28mul()
 */
int64
int28_mul(int16 num1, int64 num2)
{
  int64 result;
  if (unlikely(pg_mul_s64_overflow((int64) num1, num2, &result)))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "bigint out of range");
    return LONG_MAX;
  }
  return result;
}

/**
 * @ingroup meos_base_int
 * @brief Return the division of an int16 number and an int64 number
 * @note Derived from PostgreSQL function @p int28div()
 */
int64
int28_div(int16 num1, int64 num2)
{
  if (unlikely(num2 == 0))
  {
    meos_error(ERROR, MEOS_ERR_DIVISION_BY_ZERO, "division by zero");
    return LONG_MAX;
  }
  /* No overflow is possible */
  return ((int64) num1 / num2);
}

/* Binary arithmetics
 *
 *    int8and    - returns num1 & num2
 *    int8or    - returns num1 | num2
 *    int8xor    - returns num1 # num2
 *    int8not    - returns ~num1
 *    int8shl    - returns num1 << num2
 *    int8shr    - returns num1 >> num2
 */

/**
 * @ingroup meos_base_int
 * @brief Return the binary and of two int64 numbers
 * @note Derived from PostgreSQL function @p int8and()
 */
int64
int8_and(int64 num1, int64 num2)
{
  return (num1 & num2);
}

/**
 * @ingroup meos_base_int
 * @brief Return the binary or of two int64 numbers
 * @note Derived from PostgreSQL function @p int8or()
 */
int64
int8_or(int64 num1, int64 num2)
{
  return (num1 | num2);
}

/**
 * @ingroup meos_base_int
 * @brief Return the binary xor of two int64 numbers
 * @note Derived from PostgreSQL function @p int8xor()
 */
int64
int8_xor(int64 num1, int64 num2)
{
  return (num1 ^ num2);
}

/**
 * @ingroup meos_base_int
 * @brief Return the binary not of an int64 number
 * @note Derived from PostgreSQL function @p int8not()
 */
int64
int8_not(int64 num)
{
  return (~num);
}

/**
 * @ingroup meos_base_int
 * @brief Return an int64 number shifted to the left by another one
 * @note Derived from PostgreSQL function @p int8shl()
 */
int64
int8_shl(int64 num1, int32 num2)
{
  return (num1 << num2);
}

/**
 * @ingroup meos_base_int
 * @brief Return an int64 number shifted to the right by another one
 * @note Derived from PostgreSQL function @p int8shr()
 */
int64
int8_shr(int64 num1, int32 num2)
{
  return (num1 >> num2);
}

/*----------------------------------------------------------
 *  Conversion operators.
 *---------------------------------------------------------*/

/**
 * @ingroup meos_base_int
 * @brief Convert an int32 number into an int64 number
 * @note Derived from PostgreSQL function @p int48()
 */
int64
int4_to_int8(int32 num)
{
  return ((int64) num);
}

/**
 * @ingroup meos_base_int
 * @brief Convert an int64 number into an int32 number
 * @note Derived from PostgreSQL function @p int84()
 */
int32
int8_to_int4(int64 num)
{
  if (unlikely(num < PG_INT32_MIN) || unlikely(num > PG_INT32_MAX))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "integer out of range");
    return INT_MAX;
  }
  return ((int32) num);
}

/**
 * @ingroup meos_base_int
 * @brief Convert a int16 number into an int64 number
 * @note Derived from PostgreSQL function @p int28()
 */
int64
int2_to_int8(int16 num)
{
  return ((int64) num);
}

/**
 * @ingroup meos_base_int
 * @brief Convert an int64 number into an int16 number
 * @note Derived from PostgreSQL function @p int82()
 */
int16
int8_to_int2(int64 num)
{
  if (unlikely(num < PG_INT16_MIN) || unlikely(num > PG_INT16_MAX))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "smallint out of range");
    return SHRT_MAX;
  }
  return ((int16) num);
}

/**
 * @ingroup meos_base_int
 * @brief Convert an int64 number into a float4 number
 * @note Derived from PostgreSQL function @p i8tod()
 */

float8
int8_to_float8(int64 num)
{
  return ((float8) num);
}

/**
 * @ingroup meos_base_int
 * @brief Convert a float8 number into an int64 number
 * @note Derived from PostgreSQL function @p dtoi8()
 */
int64
float8_to_int8(float8 num)
{
  /*
   * Get rid of any fractional part in the input.  This is so we don't fail
   * on just-out-of-range values that would round into range.  Note
   * assumption that rint() will pass through a NaN or Inf unchanged.
   */
  num = rint(num);

  /* Range check */
  if (unlikely(isnan(num) || !FLOAT8_FITS_IN_INT64(num)))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "bigint out of range");
  }
  return ((int64) num);
}

/**
 * @ingroup meos_base_int
 * @brief Convert an int64 number into a float4 number
 * @note Derived from PostgreSQL function @p i8tof()
 */
float4
int8_to_float4(int64 num)
{
  return ((float4) num);
}

/**
 * @ingroup meos_base_int
 * @brief Convert a float4 number into an int64 number
 * @note Derived from PostgreSQL function @p ftoi8()
 */
int64
float4_to_int8(float4 num)
{
  /*
   * Get rid of any fractional part in the input.  This is so we don't fail
   * on just-out-of-range values that would round into range.  Note
   * assumption that rint() will pass through a NaN or Inf unchanged.
   */
  num = rint(num);

  /* Range check */
  if (unlikely(isnan(num) || !FLOAT4_FITS_IN_INT64(num)))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "bigint out of range");
    return LONG_MAX;
  }
  return ((int64) num);
}

/*****************************************************************************/