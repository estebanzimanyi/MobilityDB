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
 * @brief Functions manipulating skiplists where the elements are composed of
 * key and value pairs
 * @note See the description of skiplists in Wikipedia
 * https://en.wikipedia.org/wiki/Skip_list
 * Note also that according to
 * https://github.com/postgres/postgres/blob/master/src/backend/utils/mmgr/README#L99
 * pfree/repalloc Do Not Depend On CurrentMemoryContext
 */

#include "temporal/skiplist.h"

/* C */
#include <assert.h>
#include <limits.h>
#include <math.h>
/* PostgreSQL */
#include <postgres.h>
#include <utils/timestamp.h>
#if MEOS
  #define MaxAllocSize   ((Size) 0x3fffffff) /* 1 gigabyte - 1 */
#else
  #include <utils/memutils.h>
#endif /* MEOS */
/* MEOS */
#include <meos.h>
#include <meos_internal.h>
#include "temporal/temporal_aggfuncs.h"
#include "temporal/type_util.h"

#if ! MEOS
  extern FunctionCallInfo fetch_fcinfo();
  extern void store_fcinfo(FunctionCallInfo fcinfo);
  extern MemoryContext set_aggregation_context(FunctionCallInfo fcinfo);
  extern void unset_aggregation_context(MemoryContext ctx);
#endif /* ! MEOS */

/*****************************************************************************
 * Functions manipulating skiplists
 *****************************************************************************/

#ifdef NO_FFSL
static int
ffsl(long int i)
{
  int result = 1;
  while(! (i & 1))
  {
    result++;
    i >>= 1;
  }
  return result;
}
#endif

/**
 * @brief Get a randow value
 */
static long int
gsl_random48()
{
  return gsl_rng_get(gsl_get_aggregation_rng());
}

/**
 * @brief This simulates up to SKIPLIST_MAXLEVEL repeated coin flips without
 * spinning the RNG every time (courtesy of the internet)
 */
int
random_level()
{
  return ffsl(~(gsl_random48() & ((UINT64CONST(1) << SKIPLIST_MAXLEVEL) - 1)));
}

/**
 * @brief Return the position to store a new element in the skiplist
 * @return On error return @p INT_MAX
 */
int
skiplist_alloc(SkipList *list)
{
  /* Increase the number of values stored in the skiplist */
  list->length++;

  /* If there is unused space left by a previously deleted element, reuse it */
  if (list->freecount)
  {
    list->freecount--;
    return list->freed[list->freecount];
  }

  /* If there is no more available space expand the list */
  if (list->next >= list->capacity)
  {
    /* PostgreSQL has a limit of MaxAllocSize = 1 gigabyte - 1. By default,
     * the skiplist doubles the size when expanded. If doubling the size goes
     * beyond MaxAllocSize, we allocate the maximum number of elements that
     * fit within MaxAllocSize. If this maximum has been previously reached
     * and more capacity is required, an error is generated. */
    if (list->capacity == (int) floor(MaxAllocSize / sizeof(SkipListElem)))
    {
      meos_error(ERROR, MEOS_ERR_MEMORY_ALLOC_ERROR,
        "No more memory available to compute the aggregation");
      return INT_MAX;
    }
    if (sizeof(SkipListElem) * (list->capacity << 2) > MaxAllocSize)
      list->capacity = (int) floor(MaxAllocSize / sizeof(SkipListElem));
    else
      list->capacity <<= SKIPLIST_GROW;
    list->elems = repalloc(list->elems, sizeof(SkipListElem) * list->capacity);
  }

  /* Return the first available entry */
  list->next++;
  return list->next - 1;
}

/**
 * @brief Delete an element from the skiplist
 * @note The calling function is responsible to delete the value pointed by the
 * skiplist element. This function simply sets the pointer to NULL.
 */
// static 
void
skiplist_delete(SkipList *list, int cur)
{
  /* If the free list has not been yet created */
  if (! list->freed)
  {
    list->freecap = SKIPLIST_INITIAL_FREELIST;
#if ! MEOS
    MemoryContext ctx = set_aggregation_context(fetch_fcinfo());
#endif /* ! MEOS */
    list->freed = palloc(sizeof(int) * list->freecap);
#if ! MEOS
    unset_aggregation_context(ctx);
#endif /* ! MEOS */
  }
  /* If there is no more available space in the free list, expand it*/
  else if (list->freecount == list->freecap)
  {
    list->freecap <<= 1;
    list->freed = repalloc(list->freed, sizeof(int) * list->freecap);
  }
  /* Mark the element as free */
  list->elems[cur].value = NULL;
  list->freed[list->freecount++] = cur;
  list->length--;
  return;
}

/**
 * @ingroup meos_internal_temporal_agg
 * @brief Delete the skiplist and free its allocated memory
 * @param[in] list Skiplist
 */
void
skiplist_free(SkipList *list)
{
  if (! list)
    return;
  if (list->extra)
    pfree(list->extra);
  if (list->freed)
    pfree(list->freed);
  if (list->elems)
  {
    /* Free the element values of the skiplist if they are not NULL */
    int cur = 0;
    while (cur != -1)
    {
      SkipListElem *e = &list->elems[cur];
      if (e->value)
        pfree(e->value);
      cur = e->next[0];
    }
    /* Free the element list */
    pfree(list->elems);
  }
  pfree(list);
  return;
}

#if DEBUG_BUILD
/* Maximum length of the skiplist string */
#define MAX_SKIPLIST_LEN 65536

/**
 * @brief Output the skiplist in graphviz dot format for visualisation and
 * debugging purposes
 */
void
skiplist_print(const SkipList *list)
{
  size_t len = 0;
  char buf[MAX_SKIPLIST_LEN];
  len += snprintf(buf + len, MAX_SKIPLIST_LEN - len - 1, 
    "digraph skiplist {\n");
  len += snprintf(buf + len, MAX_SKIPLIST_LEN - len - 1, "\trankdir = LR;\n");
  len += snprintf(buf + len, MAX_SKIPLIST_LEN - len - 1, 
    "\tnode [shape = record];\n");
  int cur = 0;
  while (cur != -1)
  {
    SkipListElem *e = &list->elems[cur];
    len += snprintf(buf + len, MAX_SKIPLIST_LEN - len - 1, 
      "\telm%d [label=\"", cur);
    for (int l = e->height - 1; l > 0; l--)
      len += snprintf(buf + len, MAX_SKIPLIST_LEN - len - 1, "<p%d>|", l);
    if (! e->value)
      len += snprintf(buf + len, MAX_SKIPLIST_LEN - len - 1, "<p0>\"];\n");
    else
    {
      Span s;
      temporal_set_tstzspan(e->value, &s);
      /* The second argument of span_out is not used for spans */
      char *val = span_out(&s, Int32GetDatum(0));
      len +=  snprintf(buf + len, MAX_SKIPLIST_LEN - len - 1, "<p0>%s\"];\n",
        val);
      pfree(val);
    }
    if (e->next[0] != -1)
    {
      for (int l = 0; l < e->height; l++)
      {
        int next = e->next[l];
        len += snprintf(buf + len, MAX_SKIPLIST_LEN - len - 1, 
          "\telm%d:p%d -> elm%d:p%d ", cur, l, next, l);
        if (l == 0)
          len += snprintf(buf + len, MAX_SKIPLIST_LEN - len - 1, 
            "[weight=100];\n");
        else
          len += snprintf(buf + len, MAX_SKIPLIST_LEN - len - 1, ";\n");
      }
    }
    cur = e->next[0];
  }
  snprintf(buf + len, MAX_SKIPLIST_LEN - len - 1, "}\n");
  meos_error(WARNING, 0, "SKIPLIST: %s", buf);
  return;
}
#endif

/**
 * @brief Return the value at the head of the skiplist
 */
void *
skiplist_headval(SkipList *list)
{
  return list->elems[list->elems[0].next[0]].value;
}

/**
 * @brief Constructs a skiplist from the keys and values
 * @param[in] capacity Capacity of the skiplist, if it is equal to 0, the
 * default capacity is used
 * @param[in] key_size Size in bytes of the keys
 * @param[in] value_size Size in bytes of the values
 * @param[in] comp_fn Comparison function for elements. It is applied to the
 * keys if they are given, otherwise to the values
 * @param[in] merge_fn Merge function for elements when the element already
 * exists in the list
 */
SkipList *
skiplist_make(int capacity, size_t key_size, size_t value_size,
  int (*comp_fn)(void *, void *), void (*merge_fn)(void *, void *))
{
#if ! MEOS
  MemoryContext oldctx = set_aggregation_context(fetch_fcinfo());
#endif /* ! MEOS */
  /* A value 0 of capacity creates a skiplist with the default capacity */
  if (! capacity)
    capacity = SKIPLIST_INITIAL_CAPACITY;
  SkipList *result = palloc0(sizeof(SkipList));
  result->key_size = key_size;
  result->value_size = value_size;
  result->capacity = capacity;
  result->length = 0;
  result->next = 2;
  result->extra = NULL;
  result->extrasize = 0;
  result->comp_fn = comp_fn;
  result->merge_fn = merge_fn;
  result->elems = palloc0(sizeof(SkipListElem) * capacity);
  /* Set head and tail */
  SkipListElem *head = &result->elems[0];
  SkipListElem *tail = &result->elems[1];
  head->height = 0;
  head->next[0] = 1;
  tail->height = 0;
  tail->next[0] = -1;
  result->tail = 1;
#if ! MEOS
  MemoryContextSwitchTo(oldctx);
#endif /* ! MEOS */
  return result;
}

/**
 * @brief Comparison function used for skiplist elements
 * @param[in,out] list Skiplist
 * @param[in] key Key
 * @param[in] value Value
 * @param[in] cur Array index of the element to compare
 */
static int
skiplist_elempos(const SkipList *list, void *key, void *value, int cur)
{
  if (cur == 0)
    return 1; /* Head is -inf */
  if (cur == -1 || cur == list->tail)
    return -1; /* Tail is +inf */

  void *key_cur = (Temporal *) list->elems[cur].key;
  void *value_cur = (Temporal *) list->elems[cur].value;
  /* Apply the comparison function to either the key (if given) or the value */
  return key ? list->comp_fn(key, key_cur) : list->comp_fn(value, value_cur);
}

/**
 * @brief Splice the skiplist with the key and value using the merge function 
 * if the element exists, otherwise add the new element to the skiplist
 * @param[in,out] list Skiplist
 * @param[in] key Key
 * @param[in] value Value
 */
void
skiplist_splice_single(SkipList *list, void *key, void *value)
{
  assert(list); assert(value);

  /* Find the elements that are strictly before the new element */
  int update[SKIPLIST_MAXLEVEL];
  memset(update, 0, sizeof(update));
  int height = list->elems[0].height;
  SkipListElem *elem = &list->elems[0];
  int cur = 0;
  for (int level = height - 1; level >= 0; level--)
  {
    while (elem->next[level] != -1 &&
      skiplist_elempos(list, key, value, elem->next[level]) == 1)
    {
      cur = elem->next[level];
      elem = &list->elems[cur];
    }
    update[level] = cur;
  }
  cur = elem->next[0];
  elem = &list->elems[cur];

  /* Merge the new value with the existing one */
  if (skiplist_elempos(list, key, value, cur) == 0)
  {
    list->merge_fn(value, list->elems[cur].value);
  }
  else
  /* Insert new element */
  {
    int rheight = random_level();
    if (rheight > height)
    {
      for (int l = height; l < rheight; l++)
        update[l] = 0;
      /* Head & tail must be updated since a repalloc may have been done in
         the last call to skiplist_alloc */
      SkipListElem *head = &list->elems[0];
      SkipListElem *tail = &list->elems[list->tail];
      /* Grow head and tail as appropriate */
      head->height = rheight;
      tail->height = rheight;
    }

    /* Get the location for the new element */
    int new = skiplist_alloc(list);
    SkipListElem *newelem = &list->elems[new];

    /* Set the key (if any) and value of the new element */
#if ! MEOS
    MemoryContext oldctx = set_aggregation_context(fetch_fcinfo());
#endif /* ! MEOS */
    if (key)
      memcpy(newelem->key, value, list->key_size);
    memcpy(newelem->value, value, list->value_size);
#if ! MEOS
    unset_aggregation_context(oldctx);
#endif /* ! MEOS */
    newelem->height = rheight;

    /* Store the new element in the location found */
    for (int level = 0; level < rheight; level++)
    {
      newelem->next[level] = list->elems[update[level]].next[level];
      list->elems[update[level]].next[level] = new;
      if (level >= height && update[0] != list->tail)
        newelem->next[level] = list->tail;
    }
    if (rheight > height)
      height = rheight;
  }
  return;
}

/**
 * @brief Splice the skiplist iterating over the array of values
 * @param[in,out] list Skiplist
 * @param[in] keys Array of keys
 * @param[in] values Array of values
 * @param[in] count Number of elements in the array
 */
void
skiplist_splice(SkipList *list, void **keys, void **values, int count)
{
  assert(list->length > 0);
  for (int i = 0; i < count; i++)
    skiplist_splice_single(list, keys[i], values[i]);
  return ;
}

/**
 * @brief Return the values contained in the skiplist
 * @param[in,out] list Skiplist
 * @note The values are not freed from the skiplist
 */
void **
skiplist_values(SkipList *list)
{
#if ! MEOS
  MemoryContext ctx = set_aggregation_context(fetch_fcinfo());
#endif /* ! MEOS */
  void **result = palloc(sizeof(void *) * list->length);
  int cur = list->elems[0].next[0];
  int count = 0;
  while (cur != list->tail)
  {
    result[count++] = list->elems[cur].value;
    cur = list->elems[cur].next[0];
  }
#if ! MEOS
  unset_aggregation_context(ctx);
#endif /* ! MEOS */
  return result;
}

/*****************************************************************************/
