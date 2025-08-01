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
 * @brief Functions manipulating skiplists
 * @note See the description of skip lists in Wikipedia
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

/*****************************************************************************/

/* Constants defining the behaviour of skip lists */

#define SKIPLIST_INITIAL_CAPACITY 1024
#define SKIPLIST_GROW 1       /**< double the capacity to expand the skiplist */
#define SKIPLIST_INITIAL_FREELIST 32

/**
 * @brief Enumeration for the relative position of a given element into a
 * skiplist
 */
typedef enum
{
  BEFORE,
  DURING,
  AFTER
} RelativeTimePos;

/*****************************************************************************
 * Parameter tests
 *****************************************************************************/

bool
ensure_same_skiplist_subtype(SkipList *state, uint8 subtype)
{
  Temporal *head = (Temporal *) skiplist_headval(state);
  if (head->subtype != subtype)
  {
    meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
      "Cannot aggregate temporal values of different subtype");
    return false;
  }
  return true;
}

/*****************************************************************************
 * Functions manipulating skip lists
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

static long int
gsl_random48()
{
  return gsl_rng_get(gsl_get_aggregation_rng());
}

/**
 * @brief This simulates up to SKIPLIST_MAXLEVEL repeated coin flips without
 * spinning the RNG every time (courtesy of the internet)
 */
static int
random_level()
{
  return ffsl(~(gsl_random48() & ((UINT64CONST(1) << SKIPLIST_MAXLEVEL) - 1)));
}

/**
 * @brief Return the position to store an additional element in the skiplist
 * @return On error return @p INT_MAX
 */
static int
skiplist_alloc(SkipList *list)
{
  /* Increase the number of values stored in the skip list */
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
     * the skip list doubles the size when expanded. If doubling the size goes
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
static void
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

/**
 * @brief Output the skiplist in graphviz dot format for visualisation and
 * debugging purposes
 */
#if DEBUG_BUILD
/* Maximum length of the skiplist string */
#define MAX_SKIPLIST_LEN 65536

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

/*****************************************************************************/

/**
 * @brief Reads the state value from the buffer
 * @param[in] state State
 * @param[in] data Structure containing the data
 * @param[in] size Size of the structure
 */
void
aggstate_set_extra(SkipList *state, void *data, size_t size)
{
#if ! MEOS
  MemoryContext ctx;
  if(! AggCheckCallContext(fetch_fcinfo(), &ctx))
    elog(ERROR, "Transition function called in non-aggregate context");
  MemoryContext oldctx = MemoryContextSwitchTo(ctx);
#endif /* ! MEOS */
  state->extra = palloc(size);
  state->extrasize = size;
  memcpy(state->extra, data, size);
#if ! MEOS
  MemoryContextSwitchTo(oldctx);
#endif /* ! MEOS */
  return;
}

/**
 * @brief Return the value at the head of the skiplist
 */
void *
skiplist_headval(SkipList *list)
{
  return list->elems[list->elems[0].next[0]].value;
}

/**
 * @brief Constructs a skiplist from the array of values values
 * @param[in] values Array of values
 * @param[in] count Number of elements in the array
 */
SkipList *
skiplist_make()
{
#if ! MEOS
  MemoryContext oldctx = set_aggregation_context(fetch_fcinfo());
#endif /* ! MEOS */
  SkipList *result = palloc0(sizeof(SkipList));
  int capacity = SKIPLIST_INITIAL_CAPACITY;
  result->capacity = capacity;
  result->next = 2;
  result->tail = 1;
  result->elems = palloc0(sizeof(SkipListElem) * capacity);
  /* Set head and tail elements */
  SkipListElem *head = &result->elems[0];
  SkipListElem *tail = &result->elems[1];
  head->height = 0;
  head->next[0] = 1;
  tail->height = 0;
  tail->next[0] = -1;
#if ! MEOS
  MemoryContextSwitchTo(oldctx);
#endif /* ! MEOS */
  return result;
}

/**
 * @brief Determine the relative position of a span and a timestamptz
 */
static RelativeTimePos
pos_span_timestamptz(const Span *s, TimestampTz t)
{
  if (left_span_value(s, TimestampTzGetDatum(t)))
    return BEFORE;
  if (right_span_value(s, TimestampTzGetDatum(t)))
    return AFTER;
  return DURING;
}

/**
 * @brief Determine the relative position of two periods
 */
static RelativeTimePos
pos_span_span(const Span *s1, const Span *s2)
{
  if (left_span_span(s1, s2))
    return BEFORE;
  if (left_span_span(s2, s1))
    return AFTER;
  return DURING;
}

/**
 * @brief Comparison function used for skiplists
 */
static RelativeTimePos
skiplist_elempos(const SkipList *list, Span *s, int cur)
{
  if (cur == 0)
    return AFTER; /* Head is -inf */
  if (cur == -1 || cur == list->tail)
    return BEFORE; /* Tail is +inf */

  Temporal *temp = (Temporal *) list->elems[cur].value;
  if (temp->subtype == TINSTANT)
    return pos_span_timestamptz(s, ((TInstant *) temp)->t);
  else /* temp->subtype == TSEQUENCE */
    return pos_span_span(s, &((TSequence *) temp)->period);
}

/**
 * @brief Insert a new set of values to the skiplist while performing the 
 * aggregation between the new values that overlap with the values in the list
 * @details The complexity of this function is
 * - average: O(count*log(n))
 * - worst case: O(n + count*log(n)) (when period spans the whole list so
 *   everything has to be deleted)
 * @param[in,out] list Skiplist
 * @param[in] values Array of values
 * @param[in] count Number of elements in the array
 * @param[in] func Function used when aggregating temporal values, may be NULL
 * for the merge aggregate function
 * @param[in] crossings True if turning points are added in the segments when
 * aggregating temporal value
 */
void
skiplist_splice(SkipList *list, void **values, int count, datum_func2 func,
  bool crossings)
{
#if ! MEOS
  MemoryContext oldctx;
#endif /* ! MEOS */

  /* Count the number of elements that will be merged with the new values */
  int spliced_count = 0;
  /* Height of the element at which the new values will be merged, initialized
   * to the root for an empty list */
  int height = list->elems[0].height;
  int update[SKIPLIST_MAXLEVEL];
  SkipListElem *head, *tail;

  /* Remove from the list the values that overlap with the new values (if any)
   * and compute their aggregation */
  if (list->length > 0)
  {
    /* Temporal aggregation cannot mix instants and sequences */
    Temporal *temp1 = (Temporal *) skiplist_headval(list);
    Temporal *temp2 = (Temporal *) values[0];
    if (temp1->subtype != temp2->subtype)
    {
      meos_error(ERROR, MEOS_ERR_AGGREGATION_ERROR,
        "Cannot aggregate temporal values of different subtype");
      return;
    }
    if (MEOS_FLAGS_LINEAR_INTERP(temp1->flags) !=
        MEOS_FLAGS_LINEAR_INTERP(temp2->flags))
    {
      meos_error(ERROR, MEOS_ERR_AGGREGATION_ERROR,
        "Cannot aggregate temporal values of different interpolation");
      return;
    }

    /* Compute the span of the new values */
    Span s;
    uint8 subtype = ((Temporal *) values[0])->subtype;
    if (subtype == TINSTANT)
    {
      TInstant *first = (TInstant *) values[0];
      TInstant *last = (TInstant *) values[count - 1];
      span_set(TimestampTzGetDatum(first->t), TimestampTzGetDatum(last->t),
        true, true, T_TIMESTAMPTZ, T_TSTZSPAN, &s);
    }
    else /* subtype == TSEQUENCE */
    {
      TSequence *first = (TSequence *) values[0];
      TSequence *last = (TSequence *) values[count - 1];
      span_set(first->period.lower, last->period.upper, first->period.lower_inc,
        last->period.upper_inc, T_TIMESTAMPTZ, T_TSTZSPAN, &s);
    }

    /* Find the list values that are strictly before the span of new values */
    memset(update, 0, sizeof(update));
    height = list->elems[0].height;
    SkipListElem *e = &list->elems[0];
    int cur = 0;
    for (int level = height - 1; level >= 0; level--)
    {
      while (e->next[level] != -1 &&
        skiplist_elempos(list, &s, e->next[level]) == AFTER)
      {
        cur = e->next[level];
        e = &list->elems[cur];
      }
      update[level] = cur;
    }
    int lower, upper;
    cur = lower = e->next[0];
    e = &list->elems[cur];

    /* Count the number of elements that will be merged with the new values */
    while (skiplist_elempos(list, &s, cur) == DURING)
    {
      cur = e->next[0];
      e = &list->elems[cur];
      spliced_count++;
    }
    upper = cur;

    /* Delete spliced-out elements (if any) but remember their values for later */
    void **spliced = NULL;
    if (spliced_count != 0)
    {
      cur = lower;
      spliced = palloc(sizeof(void *) * spliced_count);
      spliced_count = 0;
      while (cur != upper && cur != -1)
      {
        for (int level = 0; level < height; level++)
        {
          SkipListElem *prev = &list->elems[update[level]];
          if (prev->next[level] != cur)
            break;
          prev->next[level] = list->elems[cur].next[level];
        }
        spliced[spliced_count++] = list->elems[cur].value;
        skiplist_delete(list, cur);
        cur = list->elems[cur].next[0];
      }
    }

    /* Level down head & tail if necessary */
    head = &list->elems[0];
    tail = &list->elems[list->tail];
    while (head->height > 1 && head->next[head->height - 1] == list->tail)
    {
      head->height--;
      tail->height--;
      height--;
    }

    /* If we are not in a gap, compute the aggregation */
    if (spliced_count != 0)
    {
      int newcount = 0;
      void **newvalues;
      if (subtype == TINSTANT)
        newvalues = (void **) tinstant_tagg((const TInstant **) spliced,
          spliced_count, (const TInstant **) values, count, func, &newcount);
      else /* subtype == TSEQUENCE */
        newvalues = (void **) tsequence_tagg((const TSequence **) spliced,
          spliced_count, (const TSequence **) values, count, func, crossings,
          &newcount);

      /* Delete the spliced-out values */
      for (int i = 0; i < spliced_count; i++)
        pfree(spliced[i]);
      pfree(spliced);

      values = newvalues;
      count = newcount;
    }
  }

  /* Insert new elements */
  for (int i = count - 1; i >= 0; i--)
  {
    int rheight = random_level();
    if (rheight > height)
    {
      for (int l = height; l < rheight; l++)
        update[l] = 0;
      /* Head & tail must be updated since a repalloc may have been done in
         the last call to skiplist_alloc */
      head = &list->elems[0];
      tail = &list->elems[list->tail];
      /* Grow head and tail as appropriate */
      head->height = rheight;
      tail->height = rheight;
    }
    /* Get the location for the new element and store it */
    int new = skiplist_alloc(list);
    SkipListElem *newelm = &list->elems[new];
#if ! MEOS
    oldctx = set_aggregation_context(fetch_fcinfo());
#endif /* ! MEOS */
    newelm->value = temporal_copy(values[i]);
#if ! MEOS
    unset_aggregation_context(oldctx);
#endif /* ! MEOS */
    newelm->height = rheight;

    for (int level = 0; level < rheight; level++)
    {
      newelm->next[level] = list->elems[update[level]].next[level];
      list->elems[update[level]].next[level] = new;
      if (level >= height && update[0] != list->tail)
        newelm->next[level] = list->tail;
    }
    if (rheight > height)
      height = rheight;
  }

  /* Free memory */
  if (spliced_count != 0)
    pfree_array((void **) values, count);
  return;
}

/**
 * @brief Return the values contained in the skiplist
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

/**
 * @brief Return a copy of the temporal values contained in the skiplist
 */
Temporal **
skiplist_temporal_values(SkipList *list)
{
  Temporal **result = palloc(sizeof(Temporal *) * list->length);
  int cur = list->elems[0].next[0];
  int count = 0;
  while (cur != list->tail)
  {
    result[count++] = temporal_copy(list->elems[cur].value);
    cur = list->elems[cur].next[0];
  }
  return result;
}

/*****************************************************************************/
