/*-------------------------------------------------------------------------
 *
 * jsonb_op.c
 *   Special operators for jsonb only, used by various index access methods
 *
 * Copyright (c) 2014-2025, PostgreSQL Global Development Group
 *
 *
 * IDENTIFICATION
 *    src/backend/utils/adt/jsonb_op.c
 *
 *-------------------------------------------------------------------------
 */

/* C */
#include <assert.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
/* PostgreSQL */
#include <postgres.h>
#include <common/hashfn.h>
#include <common/int.h>
#include <utils/jsonb.h>
#include <utils/varlena.h> /* For DatumGetTextP */

#include <utils/date.h>
#include <utils/numeric.h>
#include <utils/timestamp.h>
#include <postgres_types.h>

/*****************************************************************************/

/**
 * @ingroup meos_jsonb_base_comp
 * @brief Return true if the text string exist as a top-level key or array
 * element within the JSON value
 * @param[in] jb JSONB value
 * @param[in] key Key
 * @note Derived from PostgreSQL function @p jsonb_exists()
 */
bool
jsonb_exists_internal(const Jsonb *jb, const text *key)
{
  JsonbValue  kval;
  JsonbValue *v = NULL;

  /*
   * We only match Object keys (which are naturally always Strings), or
   * string elements in arrays.  In particular, we do not match non-string
   * scalar elements.  Existence of a key/element is only considered at the
   * top level.  No recursion occurs.
   */
  kval.type = jbvString;
  kval.val.string.val = VARDATA_ANY(key);
  kval.val.string.len = VARSIZE_ANY_EXHDR(key);

  v = findJsonbValueFromContainer(&((Jsonb *) jb)->root,
    JB_FOBJECT | JB_FARRAY, &kval);
  return (v != NULL);
}

/**
 * @brief Return true if the text string exist as a top-level key or array
 * element within the JSON value
 */
Datum
datum_jsonb_exists(Datum l, Datum r)
{
  return BoolGetDatum(jsonb_exists_internal(DatumGetJsonbP(l),
    DatumGetTextP(r)));
}

/**
 * @brief Return true if the text string exist as a top-level key or array
 * element within the JSONB value
 * @note Derived from PostgreSQL function @p jsonb_delete_array()
 */
bool
jsonb_exists_array(const Jsonb *jb, const text **keys_elems,
  int keys_len, bool any)
{
  for (int i = 0; i < keys_len; i++)
  {
    JsonbValue strVal;

    strVal.type = jbvString;
    /* We rely on the array elements not being toasted */
    strVal.val.string.val = VARDATA_ANY(keys_elems[i]);
    strVal.val.string.len = VARSIZE_ANY_EXHDR(keys_elems[i]);

    bool res = findJsonbValueFromContainer(&((Jsonb *) jb)->root,
        JB_FOBJECT | JB_FARRAY, &strVal) != NULL;
    if ((any && res) || (! any && ! res))
      return any ? true : false;
  }
  return any ? false : true;
}

/**
 * @ingroup meos_jsonb_base
 * @brief Return true if the first JSON value contains the second one
 * @note Derived from PostgreSQL function @p jsonb_contains()
 */
 bool
jsonb_contains_internal(const Jsonb *jb1, const Jsonb *jb2)
{
  if (JB_ROOT_IS_OBJECT(jb1) != JB_ROOT_IS_OBJECT(jb2))
    return false;

  JsonbIterator *it1 = JsonbIteratorInit(&((Jsonb *)jb1)->root);
  JsonbIterator *it2 = JsonbIteratorInit(&((Jsonb *)jb2)->root);
  return JsonbDeepContains(&it1, &it2);
}

/**
 * @ingroup meos_jsonb_base
 * @brief Return true if the first JSON value is contained into the second one
 * @note Derived from PostgreSQL function @p jsonb_contained()
 */
bool
jsonb_contained_internal(const Jsonb *jb1, const Jsonb *jb2)
{
  return jsonb_contains_internal(jb2, jb1);
}

/**
 * @brief Return true if the first JSON value contains the second one
 */
Datum
datum_jsonb_contains(Datum l, Datum r)
{
  return BoolGetDatum(jsonb_contains_internal(DatumGetJsonbP(l),
    DatumGetJsonbP(r)));
}

/**
 * @brief Return true if the first JSON value is contained into the second one
 */
Datum
datum_jsonb_contained(Datum l, Datum r)
{
  return BoolGetDatum(jsonb_contained_internal(DatumGetJsonbP(l),
    DatumGetJsonbP(r)));
}

/**
 * @ingroup meos_jsonb_base_comp
 * @brief Return true if the two JSONB values are equal
 * @param[in] jb1,jb2 JSONB values
 * @note Derived from PostgreSQL function @p jsonb_eq()
 */
bool
jsonb_eq_internal(const Jsonb *jb1, const Jsonb *jb2)
{
  assert(jb1); assert(jb2);
  return compareJsonbContainers((JsonbContainer *) &jb1->root,
    (JsonbContainer *) &jb2->root) == 0;
}

/**
 * @ingroup meos_jsonb_base_comp
 * @brief Return true if the two JSONB values are not equal
 * @param[in] jb1,jb2 JSONB values
 * @note Derived from PostgreSQL function @p jsonb_ne()
 */
bool
jsonb_ne_internal(const Jsonb *jb1, const Jsonb *jb2)
{
  assert(jb1); assert(jb2);
  return compareJsonbContainers((JsonbContainer *) &jb1->root,
    (JsonbContainer *) &jb2->root) != 0;
}

/**
 * @ingroup meos_jsonb_base_comp
 * @brief Return true if the first JSONB value is less than the second one
 * @param[in] jb1,jb2 JSONB values
 * @note Derived from PostgreSQL function @p jsonb_lt()
 */
bool
jsonb_lt_internal(const Jsonb *jb1, const Jsonb *jb2)
{
  assert(jb1); assert(jb2);
  return compareJsonbContainers((JsonbContainer *) &jb1->root,
    (JsonbContainer *) &jb2->root) < 0;
}

/**
 * @ingroup meos_jsonb_base_comp
 * @brief Return true if the first JSONB value is greater than the second one
 * @param[in] jb1,jb2 JSONB values
 * @note Derived from PostgreSQL function @p jsonb_gt()
 */
bool
jsonb_gt_internal(const Jsonb *jb1, const Jsonb *jb2)
{
  assert(jb1); assert(jb2);
  return compareJsonbContainers((JsonbContainer *) &jb1->root,
    (JsonbContainer *) &jb2->root) > 0;
}

/**
 * @ingroup meos_jsonb_base_comp
 * @brief Return true if the first JSONB value is less than or equal to the
 * second one
 * @param[in] jb1,jb2 JSONB values
 * @note Derived from PostgreSQL function @p jsonb_le()
 */
bool
jsonb_le_internal(const Jsonb *jb1, const Jsonb *jb2)
{
  assert(jb1); assert(jb2);
  return compareJsonbContainers((JsonbContainer *) &jb1->root,
    (JsonbContainer *) &jb2->root) <= 0;
}

/**
 * @ingroup meos_jsonb_base_comp
 * @brief Return true if the first JSONB value is greater than or equal to the
 * second one
 * @param[in] jb1,jb2 JSONB values
 * @note Derived from PostgreSQL function @p jsonb_ge()
 */
bool
jsonb_ge_internal(const Jsonb *jb1, const Jsonb *jb2)
{
  assert(jb1); assert(jb2);
  return compareJsonbContainers((JsonbContainer *) &jb1->root,
    (JsonbContainer *) &jb2->root) >= 0;
}

/**
 * @ingroup meos_jsonb_base_comp
 * @brief Return -1, 0, or 1 depending on whether the first JSONB value
 * is less than, equal to, or greater than the second one
 * @param[in] jb1,jb2 JSONB values
 * @note Derived from PostgreSQL function @p jsonb_cmp()
 */
int
jsonb_cmp_internal(const Jsonb *jb1, const Jsonb *jb2)
{
  assert(jb1); assert(jb2);
  return compareJsonbContainers((JsonbContainer *) &jb1->root,
    (JsonbContainer *) &jb2->root) <= 0;
}

/**
 * @ingroup meos_jsonb_base
 * @brief Return the hash value of a temporal value
 * @note Derived from PostgreSQL function @p jsonb_hash()
 */
uint32
jsonb_hash_internal(Jsonb *jb)
{
  JsonbIterator *it;
  JsonbValue  v;
  JsonbIteratorToken r;
  uint32    hash = 0;

  if (JB_ROOT_COUNT(jb) == 0)
    return (0);

  it = JsonbIteratorInit(&jb->root);

  while ((r = JsonbIteratorNext(&it, &v, false)) != WJB_DONE)
  {
    switch (r)
    {
        /* Rotation is left to JsonbHashScalarValue() */
      case WJB_BEGIN_ARRAY:
        hash ^= JB_FARRAY;
        break;
      case WJB_BEGIN_OBJECT:
        hash ^= JB_FOBJECT;
        break;
      case WJB_KEY:
      case WJB_VALUE:
      case WJB_ELEM:
        JsonbHashScalarValue(&v, &hash);
        break;
      case WJB_END_ARRAY:
      case WJB_END_OBJECT:
        break;
      default:
        elog(ERROR, "invalid JsonbIteratorNext rc: %d", (int) r);
    }
  }

  return hash;
}

/**
 * @ingroup meos_jsonb_base
 * @brief Return the 64-bit hash of a JSONB value using a seed
 * @note Derived from PostgreSQL function @p jsonb_hash_extended()
 */
uint64
jsonb_hash_extended_internal(Jsonb *jb, uint64 seed)
{
  JsonbIterator *it;
  JsonbValue  v;
  JsonbIteratorToken r;
  uint64    hash = 0;

  if (JB_ROOT_COUNT(jb) == 0)
    return seed;

  it = JsonbIteratorInit(&jb->root);

  while ((r = JsonbIteratorNext(&it, &v, false)) != WJB_DONE)
  {
    switch (r)
    {
        /* Rotation is left to JsonbHashScalarValueExtended() */
      case WJB_BEGIN_ARRAY:
        hash ^= ((uint64) JB_FARRAY) << 32 | JB_FARRAY;
        break;
      case WJB_BEGIN_OBJECT:
        hash ^= ((uint64) JB_FOBJECT) << 32 | JB_FOBJECT;
        break;
      case WJB_KEY:
      case WJB_VALUE:
      case WJB_ELEM:
        JsonbHashScalarValueExtended(&v, &hash, seed);
        break;
      case WJB_END_ARRAY:
      case WJB_END_OBJECT:
        break;
      default:
        elog(ERROR, "invalid JsonbIteratorNext rc: %d", (int) r);
    }
  }

  return hash;
}

/*****************************************************************************/