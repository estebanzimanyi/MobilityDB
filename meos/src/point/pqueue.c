/*****************************************************************************
 *
 * This MobilityDB code is provided under The PostgreSQL License.
 * Copyright (c) 2016-2023, Université libre de Bruxelles and MobilityDB
 * contributors
 *
 * MobilityDB includes portions of PostGIS version 3 source code released
 * under the GNU General Public License (GPLv2 or later).
 * Copyright (c) 2001-2023, PostGIS contributors
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
 * @brief Priority Queue data structure derived from
 * https://github.com/nomemory/c-generic-pqueue/
 */

#include "point/pqueue.h"

/* C */
#include <math.h>
/* PostgreSQL */
#include <postgres.h>
#if MEOS
  #define MaxAllocSize   ((Size) 0x3fffffff) /* 1 gigabyte - 1 */
#else
  #include <utils/memutils.h>
#endif /* MEOS */

// #include <stdlib.h>
// #include <stdio.h>

/*****************************************************************************/

/* Constants defining the behaviour of priority queues */

#define PQUEUE_INITIAL_CAPACITY 1024
#define PQUEUE_GROW 1       /**< double the capacity to expand the skiplist */

/* Util macros */
#define LEFT(x) (2 * (x) + 1)
#define RIGHT(x) (2 * (x) + 2)
#define PARENT(x) ((x) / 2)

/**
 * @brief Expand the queue if there is no more space
 */
static void
queue_expand(PQueue *queue)
{
  /* If there is no more available space expand the queue */
  if (queue->length >= queue->capacity)
  {
    /* PostgreSQL has a limit of MaxAllocSize = 1 gigabyte - 1. By default,
     * the queue doubles the size when expanded. If doubling the size goes
     * beyond MaxAllocSize, we allocate the maximum number of elements that
     * fit within MaxAllocSize. If this maximum has been previously reached
     * and more capacity is required, an error is generated. */
    if (queue->capacity == (int) floor(MaxAllocSize / sizeof(void *)))
      elog(ERROR, "No more memory available to expand the priority queue");
    if (sizeof(void *) * (queue->capacity << 2) > MaxAllocSize)
      queue->capacity = (int) floor(MaxAllocSize / sizeof(void *));
    else
      queue->capacity <<= PQUEUE_GROW;
    queue->elems = repalloc(queue->elems, sizeof(void *) * queue->capacity);
  }
  return;
}

/**
 * @brief Turn an "almost-heap" into a heap
 */
void pqueue_heapify(PQueue *q, size_t idx)
{
  /* left index, right index, largest */
  void *tmp = NULL;
  size_t l_idx, r_idx, lrg_idx;
  NP_CHECK(q);

  l_idx = LEFT(idx);
  r_idx = RIGHT(idx);

  /* Left child exists, compare left child with its parent */
  if (l_idx < q->length && q->cmp(q->elems[l_idx], q->elems[idx]) > 0)
    lrg_idx = l_idx;
  else
    lrg_idx = idx;

  /* Right child exists, compare right child with the largest element */
  if (r_idx < q->length && q->cmp(q->elems[r_idx], q->elems[lrg_idx]) > 0)
    lrg_idx = r_idx;

  /* At this point largest element was determined */
  if (lrg_idx != idx)
  {
    /* Swap between the index at the largest element */
    tmp = q->elems[lrg_idx];
    q->elems[lrg_idx] = q->elems[idx];
    q->elems[idx] = tmp;
    /* Heapify again */
    pqueue_heapify(q, lrg_idx);
  }
}

/**
 * @brief Allocate memory for a new priority queue
 *
 * 'cmp' function:
 *   returns 0 if d1 and d2 have the same priorities
 *   returns [negative value] if d1 have a smaller priority than d2
 *   returns [positive value] if d1 have a greater priority than d2
*/
PQueue *
pqueue_make(int (*cmp)(const void *d1, const void *d2))
{
  PQueue *res = NULL;
  NP_CHECK(cmp);
  res = malloc(sizeof(*res));
  NP_CHECK(res);
  res->cmp = cmp;
  /* The inner representation of elements inside the queue is an array of void* */
  res->elems = palloc(PQUEUE_INITIAL_CAPACITY * sizeof(*(res->elems)));
  NP_CHECK(res->elems);
  res->length = 0;
  res->capacity = PQUEUE_INITIAL_CAPACITY;
  return (res);
}

/**
 * @brief De-allocate memory for a priority queue
 */
void
pqueue_free(PQueue *q)
{
  if (NULL == q)
  {
    DEBUG("Priority Queue is already NULL. Nothing to free.");
    return;
  }
  pfree(q->elems);
  pfree(q);
}

/**
 * @brief Add a new element to the priority queue
 */
void
pqueue_enqueue(PQueue *q, const void *elem)
{
  size_t i;
  void *tmp = NULL;
  NP_CHECK(q);
  if (q->length >= q->capacity)
    queue_expand(q);

  /* Adds element last */
  q->elems[q->length] = (void *) elem;
  i = q->length;
  q->length++;
  /* The new element is swapped with its parent as long as its
   * precedence is higher */
  while(i > 0 && q->cmp(q->elems[i], q->elems[PARENT(i)]) > 0)
  {
    tmp = q->elems[i];
    q->elems[i] = q->elems[PARENT(i)];
    q->elems[PARENT(i)] = tmp;
    i = PARENT(i);
  }
}

/**
 * @brief Return the element with the highest priority from the queue
 */
void *
pqueue_dequeue(PQueue *q)
{
  void *result = NULL;
  NP_CHECK(q);
  if (q->length < 1)
  {
     /* Priority Queue is empty */
     DEBUG("Priority Queue underflow . Cannot remove another element .");
     return NULL;
  }
  result = q->elems[0];
  q->elems[0] = q->elems[q->length - 1];
  q->length--;
  /* Restore heap property */
  pqueue_heapify(q, 0);
  return (result);
}

/*****************************************************************************/

