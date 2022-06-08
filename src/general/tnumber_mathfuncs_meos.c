/*****************************************************************************
 *
 * This MobilityDB code is provided under The PostgreSQL License.
 * Copyright (c) 2016-2022, Université libre de Bruxelles and MobilityDB
 * contributors
 *
 * MobilityDB includes portions of PostGIS version 3 source code released
 * under the GNU General Public License (GPLv2 or later).
 * Copyright (c) 2001-2022, PostGIS contributors
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
 * @file tnumber_mathfuncs.c
 * @brief Mathematical operators (+, -, *, /) and functions (round, degrees)
 * for temporal number.
 */

#include "general/tnumber_mathfuncs.h"

/* C */
#include <assert.h>
#include <math.h>
/* MobilityDB */
#include <libmeos.h>
#include "general/tnumber_mathfuncs.h"
#include "general/temporal_util.h"

/*****************************************************************************
 * Temporal addition
 *****************************************************************************/

/**
 * @ingroup libmeos_temporal_math
 * @brief Return the temporal addition of a number and a temporal number
 * @sqlop @p +
 */
Temporal *
add_number_tnumber(Datum number, CachedType basetype, const Temporal *tnumber)
{
  return arithop_tnumber_number(tnumber, number, basetype, ADD, &datum_add,
    INVERT);
}

/**
 * @ingroup libmeos_temporal_math
 * @brief Return the temporal addition of a temporal number and a number
 * @sqlop @p +
 */
Temporal *
add_tnumber_number(const Temporal *tnumber, Datum number, CachedType basetype)
{
  return arithop_tnumber_number(tnumber, number, basetype, ADD, &datum_add,
    INVERT_NO);
}

/**
 * @ingroup libmeos_temporal_math
 * @brief Return the temporal addition of the temporal numbers
 * @sqlop @p +
 */
Temporal *
add_tnumber_tnumber(const Temporal *tnumber1, const Temporal *tnumber2)
{
  return arithop_tnumber_tnumber(tnumber1, tnumber2, ADD, &datum_add, NULL);
}

/*****************************************************************************
 * Temporal subtraction
 *****************************************************************************/

/**
 * @ingroup libmeos_temporal_math
 * @brief Return the temporal subtraction of a number and a temporal number
 * @sqlop @p -
 */
Temporal *
sub_number_tnumber(Datum number, CachedType basetype, const Temporal *tnumber)
{
  return arithop_tnumber_number(tnumber, number, basetype, SUB, &datum_sub,
    INVERT);
}

/**
 * @ingroup libmeos_temporal_math
 * @brief Return the temporal subtraction of a temporal number and a number
 * @sqlop @p -
 */
Temporal *
sub_tnumber_number(const Temporal *tnumber, Datum number, CachedType basetype)
{
  return arithop_tnumber_number(tnumber, number, basetype, SUB, &datum_sub,
    INVERT_NO);
}

/**
 * @ingroup libmeos_temporal_math
 * @brief Return the temporal subtraction of the temporal numbers
 * @sqlop @p -
 */
Temporal *
sub_tnumber_tnumber(const Temporal *tnumber1, const Temporal *tnumber2)
{
  return arithop_tnumber_tnumber(tnumber1, tnumber2, SUB, &datum_sub, NULL);
}

/*****************************************************************************
 * Temporal multiplication
 *****************************************************************************/

/**
 * @ingroup libmeos_temporal_math
 * @brief Return the temporal multiplication of a number and a temporal number
 * @sqlop @p *
 */
Temporal *
mult_number_tnumber(Datum number, CachedType basetype, const Temporal *tnumber)
{
  return arithop_tnumber_number(tnumber, number, basetype, MULT, &datum_mult,
    INVERT);
}

/**
 * @ingroup libmeos_temporal_math
 * @brief Return the temporal multiplication of a temporal number and a number
 * @sqlop @p *
 */
Temporal *
mult_tnumber_number(const Temporal *tnumber, Datum number, CachedType basetype)
{
  return arithop_tnumber_number(tnumber, number, basetype, MULT, &datum_mult,
    INVERT_NO);
}

/**
 * @ingroup libmeos_temporal_math
 * @brief Return the temporal multiplication of the temporal numbers
 * @sqlop @p *
 */
Temporal *
mult_tnumber_tnumber(const Temporal *tnumber1, const Temporal *tnumber2)
{
  return arithop_tnumber_tnumber(tnumber1, tnumber2, MULT, &datum_mult,
    &tnumber_mult_tp_at_timestamp);
}

/*****************************************************************************
 * Temporal division
 *****************************************************************************/

/**
 * @ingroup libmeos_temporal_math
 * @brief Return the temporal division of a number and a temporal number
 * @sqlop @p /
 */
Temporal *
div_number_tnumber(Datum number, CachedType basetype, const Temporal *tnumber)
{
  return arithop_tnumber_number(tnumber, number, basetype, DIV, &datum_div,
    INVERT);
}

/**
 * @ingroup libmeos_temporal_math
 * @brief Return the temporal division of a temporal number and a number
 * @sqlop @p /
 */
Temporal *
div_tnumber_number(const Temporal *tnumber, Datum number, CachedType basetype)
{
  return arithop_tnumber_number(tnumber, number, basetype, DIV, &datum_div,
    INVERT_NO);
}

/**
 * @ingroup libmeos_temporal_math
 * @brief Return the temporal division of the temporal numbers
 * @sqlop @p /
 */
Temporal *
div_tnumber_tnumber(const Temporal *tnumber1, const Temporal *tnumber2)
{
  return arithop_tnumber_tnumber(tnumber1, tnumber2, DIV, &datum_div,
    &tnumber_div_tp_at_timestamp);
}

/*****************************************************************************/