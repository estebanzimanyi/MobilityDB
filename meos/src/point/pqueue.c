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
 * @brief Priority Queue data structure
 * https://en.wikipedia.org/wiki/Priority_queue
 * derived from
 * https://github.com/nomemory/c-generic-pqueue/
 */

#include "point/pqueue.h"

/* C */
#include <assert.h>
#include <math.h>
/* PostgreSQL */
#include <postgres.h>
#if ! MEOS
  #include <utils/memutils.h> /* for MaxAllocSize */
#endif /* i MEOS */

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
static void queue_expand(PQueue *queue)
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
void pqueue_heapify(PQueue *queue, size_t idx)
{
  /* left index, right index, smallest index */
  size_t l_idx, r_idx, small_idx;
  NP_CHECK(queue);

  l_idx = LEFT(idx);
  r_idx = RIGHT(idx);

  /* Left child exists, compare left child with its parent */
  if (l_idx < queue->length && queue->cmp(queue->elems[l_idx], queue->elems[idx]) < 0)
    small_idx = l_idx;
  else
    small_idx = idx;

  /* Right child exists, compare right child with the smallest element */
  if (r_idx < queue->length && queue->cmp(queue->elems[r_idx], queue->elems[small_idx]) < 0)
    small_idx = r_idx;

  /* At this point smallest element was determined */
  if (small_idx != idx)
  {
    /* Swap between the index at the smallest element */
    void *tmp = queue->elems[small_idx];
    queue->elems[small_idx] = queue->elems[idx];
    queue->elems[idx] = tmp;
    /* Heapify again */
    pqueue_heapify(queue, small_idx);
  }
}

/**
 * @brief Create a new priority queue
 * @param cmp Compare function that returns
 * - 0 if d1 and d2 have the same priorities
 * - [negative value] if d1 have a smaller priority than d2
 * - [positive value] if d1 have a greater priority than d2
 * @pre The argument function is not NULL
*/
PQueue *pqueue_make(int (*cmp)(const void *d1, const void *d2))
{
  assert(cmp != NULL);
  PQueue *result = NULL;
  result = palloc(sizeof(*result));
  NP_CHECK(result);
  result->cmp = cmp;
  /* The inner representation of elements inside the queue is an array of void* */
  result->elems = palloc(PQUEUE_INITIAL_CAPACITY * sizeof(*(result->elems)));
  NP_CHECK(result->elems);
  result->length = 0;
  result->capacity = PQUEUE_INITIAL_CAPACITY;
  return (result);
}

/**
 * @brief De-allocate memory for a priority queue
 * @pre queue is not NULL
 */
void pqueue_free(PQueue *queue)
{
  assert(queue != NULL);
  pfree(queue->elems);
  pfree(queue);
  queue = NULL;
  return;
}

/**
 * @brief Add a new element to the priority queue
 * @pre queue is not NULL and elemen is not NULL
 */
void pqueue_enqueue(PQueue *queue, const void *elem)
{
  assert(queue != NULL && elem != NULL);
  if (queue->length >= queue->capacity)
    queue_expand(queue);

  /* Adds element last */
  queue->elems[queue->length] = (void *) elem;
  size_t i = queue->length;
  queue->length++;
  /* The new element is swapped with its parent as long as its
   * precedence is smaller */
  while(i > 0 && queue->cmp(queue->elems[i], queue->elems[PARENT(i)]) < 0)
  {
    void *tmp = queue->elems[i];
    queue->elems[i] = queue->elems[PARENT(i)];
    queue->elems[PARENT(i)] = tmp;
    i = PARENT(i);
  }
}

/**
 * @brief Return the element with the highest priority from the queue
 * @pre queue is not NULL
 */
void *
pqueue_dequeue(PQueue *queue)
{
  assert(queue != NULL);
  void *result = NULL;
  if (queue->length < 1)
  {
     /* Priority Queue is empty */
     elog(WARNING, "Priority Queue underflow: Cannot remove another element.");
     return NULL;
  }
  result = queue->elems[0];
  queue->elems[0] = queue->elems[queue->length - 1];
  queue->length--;
  /* Restore heap property */
  pqueue_heapify(queue, 0);
  return (result);
}

/*****************************************************************************/

