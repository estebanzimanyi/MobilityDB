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

#ifndef __PQUEUE__H__
#define __PQUEUE__H__

#include <stddef.h>

/**
* Debugging macro .
*
* Checks for a NULL pointer, and prints the error message, source file and
* line via 'stderr' .
* If the check fails the program exits with error code (-1) .
*/
#define NP_CHECK(ptr) \
  { \
    if (NULL == (ptr)) { \
      elog(ERROR, "%s:%d NULL POINTER: %s n", \
        __FILE__, __LINE__, #ptr); \
      exit(-1);  \
    } \
  } \

#define DEBUG(msg) elog(WARNING, "%s:%d %s", __FILE__, __LINE__, (msg))

/**
* Priority Queue Structure
*/
typedef struct PQueue_s
{
  /* The actual size of heap at a certain time */
  size_t length;
  /* The amount of allocated memory for the heap */
  size_t capacity;
  /* An array of (void*), the actual max-heap */
  void **elems;
  /* A pointer to a comparator function, used to prioritize elements */
  int (*cmp)(const void *d1, const void *d2);
} PQueue;

/**
 * @brief Create a new priority queue.
 * @param cmp Pointer to a comparator function establishing priorities.
*/
PQueue *pqueue_make(int (*cmp)(const void *d1, const void *d2));

/**
 * @brief De-allocates memory for a priority queue
 */
void pqueue_free(PQueue *q);

/**
 * @brief Add an element to the priority queue
 */
void pqueue_enqueue(PQueue *q, const void *data);

/**
 * @brief Remove the element with the greatest priority from within the queue
 */
void *pqueue_dequeue(PQueue *q);

/*****************************************************************************/

#endif
