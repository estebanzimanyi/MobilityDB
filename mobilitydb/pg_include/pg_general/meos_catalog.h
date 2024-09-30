/*****************************************************************************
 *
 * This MobilityDB code is provided under The PostgreSQL License.
 * Copyright (c) 2016-2024, Université libre de Bruxelles and MobilityDB
 * contributors
 *
 * MobilityDB includes portions of PostGIS version 3 source code released
 * under the GNU General Public License (GPLv2 or later).
 * Copyright (c) 2001-2024, PostGIS contributors
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
 * @brief Functions for building a cache of type and operator Oids.
 */

#ifndef __PG_MEOS_CATALOG_H__
#define __PG_MEOS_CATALOG_H__

/* PostgreSQL */
#include <postgres.h>
#include <common/hashfn.h>
/* MEOS */
#include <meos.h>
#include "general/meos_catalog.h"

/*****************************************************************************/

/**
 * @brief Structure to represent the type cache hash table
 */
typedef struct
{
  Oid typoid;        /**< Oid of the type (hashtable key) */
  meosType type;     /**< Type enum */
  char status;       /* hash status */
} oid_type_entry;

/**
 * @brief Define a hashtable mapping type Oids to a structure containing
 * operator and type enums
 */
#define SH_PREFIX oid_type
#define SH_ELEMENT_TYPE oid_type_entry
#define SH_KEY_TYPE Oid
#define SH_KEY typoid
#define SH_HASH_KEY(tb, key) hash_bytes_uint32(key)
#define SH_EQUAL(tb, a, b) a == b
#define SH_SCOPE static inline
#define SH_DEFINE
#define SH_DECLARE
#include "lib/simplehash.h"

/**
 * @brief Structure to represent the operator cache hash table
 */
typedef struct
{
  Oid oproid;        /**< Oid of the operator (hashtable key) */
  meosOper oper;     /**< Operator type enum */
  meosType ltype;    /**< Type enum of the left argument */
  meosType rtype;    /**< Type enum of the right argument */
  char status;       /* hash status */
} oid_oper_entry;

/**
 * @brief Define a hashtable mapping operator Oids to a structure containing
 * operator and type enums
 */
#define SH_PREFIX oid_oper
#define SH_ELEMENT_TYPE oid_oper_entry
#define SH_KEY_TYPE Oid
#define SH_KEY oproid
#define SH_HASH_KEY(tb, key) hash_bytes_uint32(key)
#define SH_EQUAL(tb, a, b) a == b
#define SH_SCOPE static inline
#define SH_DEFINE
#define SH_DECLARE
#include "lib/simplehash.h"

/*****************************************************************************
 * Global variables defined in the file globals.c
 *****************************************************************************/

/**
 * @brief Global variable that states whether the type and operator Oid caches
 * have been initialized
 * @details
 * - Global variable array that keeps the type Oids used in MobilityDB
 * - Global hash table that keeps the operator Oids used in MobilityDB
 * - Global variable 3-dimensional array that keeps the operator Oids used
 *   in MobilityDB.
 *   The first dimension corresponds to the operator class (e.g., <=), the
 *   second and third dimensions correspond, respectively, to the left and
 *   right arguments of the operator. A value 0 is stored in the cell of the
 *   array ifthe operator class is not defined for the left and right types.
 */

typedef struct
{
  Oid type_oid[NO_MEOS_TYPES];
  struct oid_type_hash *oid_type;
  struct oid_oper_hash *oid_oper;
  Oid oper_args_oid[NO_MEOS_TYPES][NO_MEOS_TYPES][NO_MEOS_TYPES];
} mobilitydb_constants;

/*****************************************************************************/

/* MobilityDB functions */

extern Oid type_oid(meosType t);
extern Oid oper_oid(meosOper op, meosType lt, meosType rt);
extern meosType oid_type(Oid typoid);
extern meosOper oid_oper(Oid oproid, meosType *ltype, meosType *rtype);

extern bool range_basetype(meosType type);
extern bool ensure_range_basetype(meosType type);
extern meosType basetype_rangetype(meosType type);
extern meosType basetype_multirangetype(meosType type);

/*****************************************************************************/

#endif /* __PG_MEOS_CATALOG_H__ */

