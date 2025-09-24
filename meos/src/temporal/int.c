/*-------------------------------------------------------------------------
 *
 * int.c
 *    Functions for the built-in integer types (except int8).
 *
 * Portions Copyright (c) 1996-2025, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *    src/backend/utils/adt/int.c
 *
 *-------------------------------------------------------------------------
 */
/*
 * OLD COMMENTS
 *    I/O routines:
 *     int2in, int2out, int2recv, int2send
 *     int4in, int4out, int4recv, int4send
 *     int2vectorin, int2vectorout, int2vectorrecv, int2vectorsend
 *    Boolean operators:
 *     inteq, intne, intlt, intle, intgt, intge
 *    Arithmetic operators:
 *     intpl, intmi, int4mul, intdiv
 *
 *    Arithmetic operators:
 *     intmod
 */

/* C */
#include <assert.h>
#include <limits.h>
/* PostgreSQL */
#include "postgres.h"
#include "common/int.h"
/* MEOS */
#include "../include/meos.h"

extern int pg_itoa(int16 i, char *a);
extern int pg_ltoa(int32 number, char *a);
extern int16 pg_strtoint16_safe(const char *s, void *escontext);
extern int32 pg_strtoint32_safe(const char *s, void *escontext);


/*****************************************************************************
 *   USER I/O ROUTINES                             *
 *****************************************************************************/

/**
 * @ingroup meos_base_int
 * @brief Return an int16 number from its string representation
 * @note Derived from PostgreSQL function @p int2in()
 */
int16
int2_in(char *str)
{
  return (pg_strtoint16_safe(str, NULL));
}

/**
 * @ingroup meos_base_int
 * @brief Return the string representation of an int16 number
 * @note Derived from PostgreSQL function @p int2out()
 */
char *
int2_out(int16 arg)
{
  char *result = (char *) palloc(7);  /* sign, 5 digits, '\0' */
  pg_itoa(arg, result);
  return result;
}

/*****************************************************************************
 *   PUBLIC ROUTINES                             *
 *****************************************************************************/

/**
 * @ingroup meos_base_int
 * @brief Return an int32 number from its string representation
 * @note Derived from PostgreSQL function @p int4in()
 */
int32
int4_in(char *str)
{
  return pg_strtoint32_safe(str, NULL);
}

/**
 * @ingroup meos_base_int
 * @brief Return the string representation of an int32 number
 * @note Derived from PostgreSQL function @p int4out()
 */
char *
int4_out(int32 arg)
{
  char *result = (char *) palloc(12);  /* sign, 10 digits, '\0' */
  pg_ltoa(arg, result);
  return result;
}

/*
 *    ===================
 *    CONVERSION ROUTINES
 *    ===================
 */

/**
 * @ingroup meos_base_int
 * @brief Convert an int16 number into an int32 number
 * @note Derived from PostgreSQL function @p i2toi4()
 */
int32
int2_to_int4(int16 arg)
{
  return ((int32) arg);
}

/**
 * @ingroup meos_base_int
 * @brief Convert an int32 number into an int16 number
 * @note Derived from PostgreSQL function @p i4toi2()
 */
int16
int4_to_int2(int32 arg)
{
  if (unlikely(arg < SHRT_MIN) || unlikely(arg > SHRT_MAX))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "smallint out of range");
    return SHRT_MAX;
  }
  return ((int16) arg);
}

/**
 * @ingroup meos_base_int
 * @brief Convert an int32 number into a boolean value
 * @note Derived from PostgreSQL function @p int4bool()
 */
bool
int4_to_bool(int32 arg)
{
  if (arg == 0)
    return (false);
  else
    return (true);
}

/**
 * @ingroup meos_base_int
 * @brief Convert a boolean value into an int32 number
 * @note Derived from PostgreSQL function @p boolint4()
 */
/* Cast bool -> int4 */
int32
bool_to_int4(bool arg)
{
  if (arg == false)
    return (0);
  else
    return (1);
}

/*
 *    ============================
 *    COMPARISON OPERATOR ROUTINES
 *    ============================
 */

/*
 *    inteq      - returns 1 iff arg1 == arg2
 *    intne      - returns 1 iff arg1 != arg2
 *    intlt      - returns 1 iff arg1 < arg2
 *    intle      - returns 1 iff arg1 <= arg2
 *    intgt      - returns 1 iff arg1 > arg2
 *    intge      - returns 1 iff arg1 >= arg2
 */

/**
 * @ingroup meos_base_int
 * @brief Return true if two int32 numbers are equal
 * @note Derived from PostgreSQL function @p int4eq()
 */
bool
int4_eq(int32 arg1, int32 arg2)
{
  return (arg1 == arg2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if two int32 numbers are not equal
 * @note Derived from PostgreSQL function @p int4ne()
 */
bool
int4_ne(int32 arg1, int32 arg2)
{
  return (arg1 != arg2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if the first int32 number is less than the second one
 * @note Derived from PostgreSQL function @p int4lt()
 */
bool
int4_lt(int32 arg1, int32 arg2)
{
  return (arg1 < arg2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if the first int32 number is less than or equal to the
 * second one
 * @note Derived from PostgreSQL function @p int4le()
 */
bool
int4_le(int32 arg1, int32 arg2)
{
  return (arg1 <= arg2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if the first int32 number is greater than the second one
 * @note Derived from PostgreSQL function @p int4gt()
 */
bool
int4_gt(int32 arg1, int32 arg2)
{
  return (arg1 > arg2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if the first int32 number is greater than or equal to the
 * second one
 * @note Derived from PostgreSQL function @p int4ge()
 */
bool
int4_ge(int32 arg1, int32 arg2)
{
  return (arg1 >= arg2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if two int16 values are equal
 * @note Derived from PostgreSQL function @p int2eq()
 */
bool
int2_eq(int16 arg1, int16 arg2)
{
  return (arg1 == arg2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if two int16 values are not equal
 * @note Derived from PostgreSQL function @p int2ne()
 */
bool
int2_ne(int16 arg1, int16 arg2)
{
  return (arg1 != arg2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if the first int16 number is less than the second one
 * @note Derived from PostgreSQL function @p int2lt()
 */
bool
int2_lt(int16 arg1, int16 arg2)
{
  return (arg1 < arg2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if the first int16 number is less than or equal to the
 * second one
 * @note Derived from PostgreSQL function @p int2le()
 */
bool
int2_le(int16 arg1, int16 arg2)
{
  return (arg1 <= arg2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if the first int16 number is greater than the second one
 * @note Derived from PostgreSQL function @p int2gt()
 */
bool
int2_gt(int16 arg1, int16 arg2)
{
  return (arg1 > arg2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if the first int16 number is greater than or equal to the
 * second one
 * @note Derived from PostgreSQL function @p int2ge()
 */
bool
int2_ge(int16 arg1, int16 arg2)
{
  return (arg1 >= arg2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if an int16 number and an int32 number are equal
 * @note Derived from PostgreSQL function @p int24eq()
 */
bool
int24_eq(int16 arg1, int32 arg2)
{
  return (arg1 == arg2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if an int16 number and an int32 number are not equal
 * @note Derived from PostgreSQL function @p int24ne()
 */
bool
int24_ne(int16 arg1, int32 arg2)
{
  return (arg1 != arg2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if an int16 number is less than and an int32 number
 * @note Derived from PostgreSQL function @p int24lt()
 */
bool
int24_lt(int16 arg1, int32 arg2)
{
  return (arg1 < arg2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if an int16 number is less than or equal to an int32
 * number
 * @note Derived from PostgreSQL function @p int24le()
 */
bool
int24_le(int16 arg1, int32 arg2)
{
  return (arg1 <= arg2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if an int16 number is greater than an int32 number
 * @note Derived from PostgreSQL function @p int24gt()
 */
bool
int24_gt(int16 arg1, int32 arg2)
{
  return (arg1 > arg2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if an int16 number is greater than or equal to an int32
 * number
 * @note Derived from PostgreSQL function @p int24ge()
 */
bool
int24_ge(int16 arg1, int32 arg2)
{
  return (arg1 >= arg2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if an int32 number and an int16 number are equal
 * @note Derived from PostgreSQL function @p int42eq()
 */
bool
int42_eq(int32 arg1, int16 arg2)
{
  return (arg1 == arg2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if an int32 number and an int16 number are not equal
 * @note Derived from PostgreSQL function @p int42ne()
 */
bool
int42_ne(int32 arg1, int16 arg2)
{
  return (arg1 != arg2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if an int32 number is less than an int16 number
 * @note Derived from PostgreSQL function @p int42lt()
 */
bool
int42_lt(int32 arg1, int16 arg2)
{
  return (arg1 < arg2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if an int32 number is less than or equal to an int16
 * number
 * @note Derived from PostgreSQL function @p int42le()
 */
bool
int42_le(int32 arg1, int16 arg2)
{
  return (arg1 <= arg2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if an int32 number is greater than an int16 number
 * @note Derived from PostgreSQL function @p int42gt()
 */
bool
int42_gt(int32 arg1, int16 arg2)
{
  return (arg1 > arg2);
}

/**
 * @ingroup meos_base_int
 * @brief Return true if an int32 number is less than or equal to an int16
 * number
 * @note Derived from PostgreSQL function @p int42ge()
 */
bool
int42_ge(int32 arg1, int16 arg2)
{
  return (arg1 >= arg2);
}


/*----------------------------------------------------------*/


/*
 *    int[24]pl    - returns arg1 + arg2
 *    int[24]mi    - returns arg1 - arg2
 *    int[24]mul    - returns arg1 * arg2
 *    int[24]div    - returns arg1 / arg2
 */
 
/**
 * @ingroup meos_base_int
 * @brief Return the unary minus of an int32 number
 * @note Derived from PostgreSQL function @p int4um()
 */
int32
int4_um(int32 arg)
{
  if (unlikely(arg == PG_INT32_MIN))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "integer out of range");
    return INT_MAX;
  }
  return (-arg);
}

/**
 * @ingroup meos_base_int
 * @brief Return the unary plus of an int32 number
 * @note Derived from PostgreSQL function @p int4up()
 */
int32
int4_up(int32 arg)
{
  return (arg);
}

/**
 * @ingroup meos_base_int
 * @brief Return the adition of two int32 number
 * @note Derived from PostgreSQL function @p int4pl()
 */
int32
int4_pl(int32 arg1, int32 arg2)
{
  int32 result;

  if (unlikely(pg_add_s32_overflow(arg1, arg2, &result)))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "integer out of range");
    return INT_MAX;
  }
  return result;
}

/**
 * @ingroup meos_base_int
 * @brief Return the subraction of two int32 number
 * @note Derived from PostgreSQL function @p int4mi()
 */
int32
int4_mi(int32 arg1, int32 arg2)
{
  int32 result;

  if (unlikely(pg_sub_s32_overflow(arg1, arg2, &result)))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "integer out of range");
    return INT_MAX;
  }
  return result;
}

/**
 * @ingroup meos_base_int
 * @brief Return the multiplication of two int32 number
 * @note Derived from PostgreSQL function @p int4mul()
 */
int32
int4_mul(int32 arg1, int32 arg2)
{
  int32 result;

  if (unlikely(pg_mul_s32_overflow(arg1, arg2, &result)))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "integer out of range");
    return INT_MAX;
  }
  return result;
}

/**
 * @ingroup meos_base_int
 * @brief Return the division of two int32 number
 * @note Derived from PostgreSQL function @p int4div()
 */
int32
int4_div(int32 arg1, int32 arg2)
{
  int32 result;

  if (arg2 == 0)
  {
    meos_error(ERROR, MEOS_ERR_DIVISION_BY_ZERO, "division by zero");
    return INT_MAX;
  }

  /*
   * INT_MIN / -1 is problematic, since the result can't be represented on a
   * two's-complement machine.  Some machines produce INT_MIN, some produce
   * zero, some throw an exception.  We can dodge the problem by recognizing
   * that division by -1 is the same as negation.
   */
  if (arg2 == -1)
  {
    if (unlikely(arg1 == PG_INT32_MIN))
    {
      meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "integer out of range");
      return INT_MAX;
    }
    result = -arg1;
    return result;
  }

  /* No overflow is possible */

  result = arg1 / arg2;

  return result;
}

/**
 * @ingroup meos_base_int
 * @brief Return an int32 number incremented by one
 * @note Derived from PostgreSQL function @p int4inc()
 */
int32
int4_inc(int32 arg)
{
  int32 result;
  if (unlikely(pg_add_s32_overflow(arg, 1, &result)))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "integer out of range");
    return INT_MAX;
  }

  return result;
}

/**
 * @ingroup meos_base_int
 * @brief Return the unary minus of an int16 number
 * @note Derived from PostgreSQL function @p int2um()
 */
int16
int2_um(int16 arg)
{
  if (unlikely(arg == PG_INT16_MIN))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "smallint out of range");
    return SHRT_MAX;
  }
  return (-arg);
}

/**
 * @ingroup meos_base_int
 * @brief Return the unary plus of an int16 number
 * @note Derived from PostgreSQL function @p int2up()
 */
int16
int2_up(int16 arg)
{
  return (arg);
}

/**
 * @ingroup meos_base_int
 * @brief Return the addition of two int16 numbers
 * @note Derived from PostgreSQL function @p int2pl()
 */
int16
int2_pl(int16 arg1, int16 arg2)
{
  int16    result;

  if (unlikely(pg_add_s16_overflow(arg1, arg2, &result)))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "smallint out of range");
    return SHRT_MAX;
  }
  return result;
}

/**
 * @ingroup meos_base_int
 * @brief Return the subtraction of two int16 numbers
 * @note Derived from PostgreSQL function @p int2mi()
 */
int16
int2_mi(int16 arg1, int16 arg2)
{
  int16    result;

  if (unlikely(pg_sub_s16_overflow(arg1, arg2, &result)))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "smallint out of range");
    return SHRT_MAX;
  }
  return result;
}

/**
 * @ingroup meos_base_int
 * @brief Return the multiplication of two int16 numbers
 * @note Derived from PostgreSQL function @p int2mul()
 */
int16
int2_mul(int16 arg1, int16 arg2)
{
  int16    result;

  if (unlikely(pg_mul_s16_overflow(arg1, arg2, &result)))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "smallint out of range");
    return SHRT_MAX;
  }

  return result;
}

/**
 * @ingroup meos_base_int
 * @brief Return the division of two int16 numbers
 * @note Derived from PostgreSQL function @p int2div()
 */
int16
int2_div(int16 arg1, int16 arg2)
{
  int16    result;

  if (arg2 == 0)
  {
    meos_error(ERROR, MEOS_ERR_DIVISION_BY_ZERO, "division by zero");
    return SHRT_MAX;
  }

  /*
   * SHRT_MIN / -1 is problematic, since the result can't be represented on
   * a two's-complement machine.  Some machines produce SHRT_MIN, some
   * produce zero, some throw an exception.  We can dodge the problem by
   * recognizing that division by -1 is the same as negation.
   */
  if (arg2 == -1)
  {
    if (unlikely(arg1 == PG_INT16_MIN))
    {
      meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "smallint out of range");
      return SHRT_MAX;
    }
    result = -arg1;
    return result;
  }

  /* No overflow is possible */

  result = arg1 / arg2;

  return result;
}

/**
 * @ingroup meos_base_int
 * @brief Return the addition of an int16 and an int32 numbers
 * @note Derived from PostgreSQL function @p int24pl()
 */
int32
int24_pl(int16 arg1, int32 arg2)
{
  int32 result;

  if (unlikely(pg_add_s32_overflow((int32) arg1, arg2, &result)))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "integer out of range");
    return INT_MAX;
  }
  return result;
}

/**
 * @ingroup meos_base_int
 * @brief Return the subtraction of an int16 and an int32 numbers
 * @note Derived from PostgreSQL function @p int24mi()
 */
int32
int24_mi(int16 arg1, int32 arg2)
{
  int32 result;

  if (unlikely(pg_sub_s32_overflow((int32) arg1, arg2, &result)))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "integer out of range");
    return INT_MAX;
  }
  return result;
}

/**
 * @ingroup meos_base_int
 * @brief Return the multiplication of an int16 and an int32 numbers
 * @note Derived from PostgreSQL function @p int24mul()
 */
int32
int24_mul(int16 arg1, int32 arg2)
{
  int32 result;

  if (unlikely(pg_mul_s32_overflow((int32) arg1, arg2, &result)))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "integer out of range");
    return INT_MAX;
  }
  return result;
}

/**
 * @ingroup meos_base_int
 * @brief Return the division of an int16 and an int32 numbers
 * @note Derived from PostgreSQL function @p int24div()
 */
int32
int24_div(int16 arg1, int32 arg2)
{
  if (unlikely(arg2 == 0))
  {
    meos_error(ERROR, MEOS_ERR_DIVISION_BY_ZERO, "division by zero");
    return INT_MAX;
  }

  /* No overflow is possible */
  return ((int32) arg1 / arg2);
}

/**
 * @ingroup meos_base_int
 * @brief Return the addition of an int32 and an int16 numbers
 * @note Derived from PostgreSQL function @p int42pl()
 */
int32
int42_pl(int32 arg1, int16 arg2)
{
  int32 result;

  if (unlikely(pg_add_s32_overflow(arg1, (int32) arg2, &result)))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "integer out of range");
    return INT_MAX;
  }
  return result;
}

/**
 * @ingroup meos_base_int
 * @brief Return the subtraction of an int32 and an int16 numbers
 * @note Derived from PostgreSQL function @p int42mi()
 */
int32
int42_mi(int32 arg1, int16 arg2)
{
  int32 result;

  if (unlikely(pg_sub_s32_overflow(arg1, (int32) arg2, &result)))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "integer out of range");
    return INT_MAX;
  }
  return result;
}

/**
 * @ingroup meos_base_int
 * @brief Return the multiplication of an int32 and an int16 numbers
 * @note Derived from PostgreSQL function @p int42mul()
 */
int32
int42_mul(int32 arg1, int16 arg2)
{
  int32 result;

  if (unlikely(pg_mul_s32_overflow(arg1, (int32) arg2, &result)))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "integer out of range");
    return INT_MAX;
  }
  return result;
}

/**
 * @ingroup meos_base_int
 * @brief Return the division of an int32 and an int16 numbers
 * @note Derived from PostgreSQL function @p int42div()
 */
int32
int42_div(int32 arg1, int16 arg2)
{
  int32 result;

  if (unlikely(arg2 == 0))
  {
    meos_error(ERROR, MEOS_ERR_DIVISION_BY_ZERO, "division by zero");
    return INT_MAX;
  }

  /*
   * INT_MIN / -1 is problematic, since the result can't be represented on a
   * two's-complement machine.  Some machines produce INT_MIN, some produce
   * zero, some throw an exception.  We can dodge the problem by recognizing
   * that division by -1 is the same as negation.
   */
  if (arg2 == -1)
  {
    if (unlikely(arg1 == PG_INT32_MIN))
    {
      meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "integer out of range");
      return INT_MAX;
    }
    result = -arg1;
    return result;
  }

  /* No overflow is possible */

  result = arg1 / arg2;

  return result;
}

/**
 * @ingroup meos_base_int
 * @brief Return the modulo of two int32 numbers
 * @note Derived from PostgreSQL function @p int4mod()
 */
int32
int4_mod(int32 arg1, int32 arg2)
{
  if (unlikely(arg2 == 0))
  {
    meos_error(ERROR, MEOS_ERR_DIVISION_BY_ZERO, "division by zero");
    return INT_MAX;
  }

  /*
   * Some machines throw a floating-point exception for INT_MIN % -1, which
   * is a bit silly since the correct answer is perfectly well-defined,
   * namely zero.
   */
  if (arg2 == -1)
    return (0);

  /* No overflow is possible */

  return (arg1 % arg2);
}

/**
 * @ingroup meos_base_int
 * @brief Return the modulo of two int16 numbers
 * @note Derived from PostgreSQL function @p int2mod()
 */
int16
int2_mod(int16 arg1, int16 arg2)
{
  if (unlikely(arg2 == 0))
  {
    meos_error(ERROR, MEOS_ERR_DIVISION_BY_ZERO, "division by zero");
    return SHRT_MAX;
  }

  /*
   * Some machines throw a floating-point exception for INT_MIN % -1, which
   * is a bit silly since the correct answer is perfectly well-defined,
   * namely zero.  (It's not clear this ever happens when dealing with
   * int16, but we might as well have the test for safety.)
   */
  if (arg2 == -1)
    return (0);

  /* No overflow is possible */

  return (arg1 % arg2);
}

/**
 * @ingroup meos_base_int
 * @brief Return the absolute value of an int32 number
 * @note Derived from PostgreSQL function @p int4abs()
 */
int32
int4_abs(int32 arg)
{
  int32 result;

  if (unlikely(arg == PG_INT32_MIN))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "integer out of range");
    return INT_MAX;
  }
  result = (arg < 0) ? -arg : arg;
  return result;
}

/**
 * @ingroup meos_base_int
 * @brief Return the absolute value of an int16 number
 * @note Derived from PostgreSQL function @p int2abs()
 */
int16
int2_abs(int16 arg)
{
  int16    result;

  if (unlikely(arg == PG_INT16_MIN))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "smallint out of range");
    return SHRT_MAX;
  }
  result = (arg < 0) ? -arg : arg;
  return result;
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
 * Special care must be taken if either input is INT_MIN --- gcd(0, INT_MIN),
 * gcd(INT_MIN, 0) and gcd(INT_MIN, INT_MIN) are all equal to abs(INT_MIN),
 * which cannot be represented as a 32-bit signed integer.
 */
static int32
int4gcd_internal(int32 arg1, int32 arg2)
{
  int32    swap;
  int32    a1,
        a2;

  /*
   * Put the greater absolute value in arg1.
   *
   * This would happen automatically in the loop below, but avoids an
   * expensive modulo operation, and simplifies the special-case handling
   * for INT_MIN below.
   *
   * We do this in negative space in order to handle INT_MIN.
   */
  a1 = (arg1 < 0) ? arg1 : -arg1;
  a2 = (arg2 < 0) ? arg2 : -arg2;
  if (a1 > a2)
  {
    swap = arg1;
    arg1 = arg2;
    arg2 = swap;
  }

  /* Special care needs to be taken with INT_MIN.  See comments above. */
  if (arg1 == PG_INT32_MIN)
  {
    if (arg2 == 0 || arg2 == PG_INT32_MIN)
    {
      meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "integer out of range");
    }

    /*
     * Some machines throw a floating-point exception for INT_MIN % -1,
     * which is a bit silly since the correct answer is perfectly
     * well-defined, namely zero.  Guard against this and just return the
     * result, gcd(INT_MIN, -1) = 1.
     */
    if (arg2 == -1)
      return 1;
  }

  /* Use the Euclidean algorithm to find the GCD */
  while (arg2 != 0)
  {
    swap = arg2;
    arg2 = arg1 % arg2;
    arg1 = swap;
  }

  /*
   * Make sure the result is positive. (We know we don't have INT_MIN
   * anymore).
   */
  if (arg1 < 0)
    arg1 = -arg1;

  return arg1;
}

/**
 * @ingroup meos_base_int
 * @brief Return the greatest common denominator of two int32 numbers
 * @note Derived from PostgreSQL function @p int4gcd()
 */
int32
int4_gcd(int32 arg1, int32 arg2)
{
  int32 result = int4gcd_internal(arg1, arg2);
  return result;
}

/**
 * @ingroup meos_base_int
 * @brief Return the least common multiple of two int32 numbers
 * @note Derived from PostgreSQL function @p int4lcm()
 */
int32
int4_lcm(int32 arg1, int32 arg2)
{
  int32    gcd;
  int32 result;

  /*
   * Handle lcm(x, 0) = lcm(0, x) = 0 as a special case.  This prevents a
   * division-by-zero error below when x is zero, and an overflow error from
   * the GCD computation when x = INT_MIN.
   */
  if (arg1 == 0 || arg2 == 0)
    return (0);

  /* lcm(x, y) = abs(x / gcd(x, y) * y) */
  gcd = int4gcd_internal(arg1, arg2);
  arg1 = arg1 / gcd;

  if (unlikely(pg_mul_s32_overflow(arg1, arg2, &result)))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "integer out of range");
    return INT_MAX;
  }

  /* If the result is INT_MIN, it cannot be represented. */
  if (unlikely(result == PG_INT32_MIN))
  {
    meos_error(ERROR, MEOS_ERR_VALUE_OUT_OF_RANGE, "integer out of range");
    return INT_MAX;
  }

  if (result < 0)
    result = -result;

  return result;
}

/**
 * @ingroup meos_base_int
 * @brief Return the larger of two int16 numbers
 * @note Derived from PostgreSQL function @p int2larger()
 */
int16
int2_larger(int16 arg1, int16 arg2)
{
  return ((arg1 > arg2) ? arg1 : arg2);
}

/**
 * @ingroup meos_base_int
 * @brief Return the smaller of two int16 numbers
 * @note Derived from PostgreSQL function @p int2smaller()
 */
int16
int2_smaller(int16 arg1, int16 arg2)
{
  return ((arg1 < arg2) ? arg1 : arg2);
}

/**
 * @ingroup meos_base_int
 * @brief Return the larger of two int32 numbers
 * @note Derived from PostgreSQL function @p int4larger()
 */
int32
int4_larger(int32 arg1, int32 arg2)
{
  return ((arg1 > arg2) ? arg1 : arg2);
}

/**
 * @ingroup meos_base_int
 * @brief Return the smaller of two int32 numbers
 * @note Derived from PostgreSQL function @p int4smaller()
 */
int32
int4_smaller(int32 arg1, int32 arg2)
{
  return ((arg1 < arg2) ? arg1 : arg2);
}

/*
 * Bit-pushing operators
 *
 *    int[24]and    - returns arg1 & arg2
 *    int[24]or    - returns arg1 | arg2
 *    int[24]xor    - returns arg1 # arg2
 *    int[24]not    - returns ~arg1
 *    int[24]shl    - returns arg1 << arg2
 *    int[24]shr    - returns arg1 >> arg2
 */

/**
 * @ingroup meos_base_int
 * @brief Return the bit and of two int32 numbers
 * @note Derived from PostgreSQL function @p int4and()
 */
int32
int4_and(int32 arg1, int32 arg2)
{
  return (arg1 & arg2);
}

/**
 * @ingroup meos_base_int
 * @brief Return the bit or of two int32 numbers
 * @note Derived from PostgreSQL function @p int4or()
 */
int32
int4_or(int32 arg1, int32 arg2)
{
  return (arg1 | arg2);
}

/**
 * @ingroup meos_base_int
 * @brief Return the bit xor of two int32 numbers
 * @note Derived from PostgreSQL function @p int4xor()
 */
int32
int4_xor(int32 arg1, int32 arg2)
{
  return (arg1 ^ arg2);
}

/**
 * @ingroup meos_base_int
 * @brief Return the first int32 number shifted to the left by the second one
 * @note Derived from PostgreSQL function @p int4shl()
 */
int32
int4_shl(int32 arg1, int32 arg2)
{
  return (arg1 << arg2);
}

/**
 * @ingroup meos_base_int
 * @brief Return the first int32 number shifted to the right by the second one
 * @note Derived from PostgreSQL function @p int4shr()
 */
int32
int4_shr(int32 arg1, int32 arg2)
{
  return (arg1 >> arg2);
}

/**
 * @ingroup meos_base_int
 * @brief Return the bit not of two int32 numbers
 * @note Derived from PostgreSQL function @p int4not()
 */
int32
int4_not(int32 arg)
{
  return (~arg);
}

/**
 * @ingroup meos_base_int
 * @brief Return the bit and of two int16 numbers
 * @note Derived from PostgreSQL function @p int2and()
 */
int16
int2_and(int16 arg1, int16 arg2)
{
  return (arg1 & arg2);
}

/**
 * @ingroup meos_base_int
 * @brief Return the bit or of two int16 numbers
 * @note Derived from PostgreSQL function @p int2or()
 */
int16
int2_or(int16 arg1, int16 arg2)
{
  return (arg1 | arg2);
}

/**
 * @ingroup meos_base_int
 * @brief Return the bit xor of two int16 numbers
 * @note Derived from PostgreSQL function @p int2xor()
 */
int16
int2_xor(int16 arg1, int16 arg2)
{
  return (arg1 ^ arg2);
}

/**
 * @ingroup meos_base_int
 * @brief Return the bit not of an int16 number
 * @note Derived from PostgreSQL function @p int2not()
 */
int16
int2_not(int16 arg)
{
  return (~arg);
}

/**
 * @ingroup meos_base_int
 * @brief Return the first int16 number shifted to the left by the second one
 * @note Derived from PostgreSQL function @p int2shl()
 */
int16
int2_shl(int16 arg1, int32 arg2)
{
  return (arg1 << arg2);
}

/**
 * @ingroup meos_base_int
 * @brief Return the first int16 number shifted to the right by the second one
 * @note Derived from PostgreSQL function @p int2shr()
 */
int16
int2_shr(int16 arg1, int32 arg2)
{
  return (arg1 >> arg2);
}

/*****************************************************************************/