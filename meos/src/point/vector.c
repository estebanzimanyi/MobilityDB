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
 * @brief Minimalistic expandable vector data structure.
 */

#include "point/vector.h"

/* C */
#include <assert.h>
/* PostgreSQL */
#include <postgres.h>
#include <utils/float.h>
#if ! MEOS
  #include <utils/memutils.h> /* for MaxAllocSize */
#endif /* ! MEOS */
/* PostGIS */
#include <liblwgeom.h>
/* MEOS */
#include <meos_internal.h>
#include "general/temporal.h"

/*****************************************************************************
 * Vector data structure
 *****************************************************************************/

/* Constants defining the behaviour of expandable vectors */
#define VECTOR_INITIAL_CAPACITY 1024
#define VECTOR_GROW 1       /**< double the capacity to expand the vector */

/**
 * @brief Return the position to store an additional element in the vector,
 * expanding the vector if needed.
 */
static int vector_alloc(Vector *vector)
{
  /* If there is no more available space expand the vector */
  if (vector->length >= vector->capacity)
  {
    /* PostgreSQL has a limit of MaxAllocSize = 1 gigabyte - 1. By default,
     * the vector doubles the size when expanded. If doubling the size goes
     * beyond MaxAllocSize, we allocate the maximum number of elements that
     * fit within MaxAllocSize. If this maximum has been previously reached
     * and more capacity is required, an error is generated. */
    if (vector->capacity == (int) floor(MaxAllocSize / sizeof(void *)))
      elog(ERROR, "No more memory available to add elements to the vector");
    if (sizeof(void *) * (vector->capacity << 2) > MaxAllocSize)
      vector->capacity = (int) floor(MaxAllocSize / sizeof(void *));
    else
      vector->capacity <<= VECTOR_GROW;
    vector->elems = repalloc(vector->elems, sizeof(void *) * vector->capacity);
  }

  /* Return the next available entry */
  int result = vector->length++;
  return result;
}

/**
 * @brief Construct an empty vector
 */
Vector *vector_make(bool typbyval)
{
  int capacity = VECTOR_INITIAL_CAPACITY;
  Vector *result = palloc0(sizeof(Vector));
  result->elems = palloc0(sizeof(void *) * capacity);
  result->capacity = capacity;
  result->length = 0;
  MEOS_FLAGS_SET_BYVAL(result->flags, typbyval);
  return result;
}

#if MEOS
/**
 * @brief Wrapper function for creating a vector of values passed by
 * reference
 */
Vector *vector_byref_make(void)
{
  return vector_make(TYPE_BY_REF);
}

/**
 * @brief Wrapper function for creating a vector of values passed by value
 */
Vector *
vector_byvalue_make(void)
{
  return vector_make(TYPE_BY_VALUE);
}
#endif /* MEOS */

/**
 * @brief Append an element to a vector
 * @result Return the position of the vector where the element was stored
 * @pre vector is not NULL
 */
int vector_append(Vector *vector, Datum elem)
{
  assert(vector != NULL);
  int pos = vector_alloc(vector);
  vector->elems[pos] = elem;
  return pos;
}

/**
 * @brief Get the element of a vector at a position
 * @pre vector is not NULL
 * @pre pos belongs to [0, vector->length - 1]
 */
Datum
vector_at(Vector *vector, int pos)
{
  assert(vector != NULL);
  assert(pos >= 0 && pos < vector->length);
  return vector->elems[pos];
}

/**
 * @brief Set the element of a vector at a position
 * @pre vector is not NULL
 * @pre pos belongs to [0, vector->length - 1]
 */
void
vector_set(Vector *vector, int pos, Datum elem)
{
  assert(vector != NULL);
  assert(pos >= 0 && pos < vector->length);
  vector->elems[pos] = elem;
  return;
}

/**
 * @brief Set to NULL the pointer the element of a vector at a position
 * @note The value itself is NOT FREED, it is the responsibility of the calling
 * function to do it.
 * @pre vector is not NULL
 * @pre pos belongs to [0, vector->length - 1]
 */
void
vector_delete(Vector *vector, int pos)
{
  assert(vector != NULL);
  assert(pos >= 0 && pos < vector->length);
  vector->elems[pos] = (Datum) NULL;
  return;
}

/**
 * @brief Free the vector
 * @pre vector is not NULL
 */
void
vector_free(Vector *vector)
{
  assert(vector != NULL);
  if (vector->elems)
  {
    /* Free the element values of the vector if they are passed by reference
     * and they are not NULL */
    if (MEOS_FLAGS_GET_BYVAL(vector->flags))
    {
      for (int i = 0; i < vector->length; i++)
      {
        void *ptr = DatumGetPointer(vector->elems[i]);
        if (ptr)
          pfree(ptr);
      }
    }
    /* Free the element list */
    pfree(vector->elems);
  }
  pfree(vector);
  return;
}

/*****************************************************************************/
#if 0 /* not used */
/*****************************************************************************
 * List data structure
 *****************************************************************************/

/* Constants defining the behaviour of lists */

#define LIST_INITIAL_CAPACITY 1024
#define LIST_GROW 1       /**< double the capacity to expand the list */
#define LIST_INITIAL_FREELIST 32

/**
 * Structure to represent list elements
 */
typedef struct
{
  void *value;
  int next;
} ListElem;

/**
 * Structure to represent lists
 */
typedef struct
{
  int capacity;
  int next;
  int length;
  int *freed;
  int freecount;
  int freecap;
  int tail;
  ListElem *elems;
} List;

/**
 * @brief Return the position to store an additional element in the list
 */
static int
list_alloc(List *list)
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
     * the list doubles the size when expanded. If doubling the size goes
     * beyond MaxAllocSize, we allocate the maximum number of elements that
     * fit within MaxAllocSize. If this maximum has been previously reached
     * and more capacity is required, an error is generated. */
    if (list->capacity == (int) floor(MaxAllocSize / sizeof(ListElem)))
      elog(ERROR, "No more memory available to add elements to the list");
    if (sizeof(ListElem) * (list->capacity << 2) > MaxAllocSize)
      list->capacity = (int) floor(MaxAllocSize / sizeof(ListElem));
    else
      list->capacity <<= LIST_GROW;
    list->elems = repalloc(list->elems, sizeof(ListElem) * list->capacity);
  }

  /* Return the first available entry */
  list->next++;
  return list->next - 1;
}

/**
 * @brief Delete an element from the list
 * @note The calling function is responsible to delete the value pointed by the
 * list element. This function simply sets the pointer to NULL.
 */
static void
list_delete(List *list, int cur)
{
  /* If the free list has not been yet created */
  if (! list->freed)
  {
    list->freecap = LIST_INITIAL_FREELIST;
    list->freed = palloc(sizeof(int) * list->freecap);
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
 * @brief Free the list
 */
void
list_free(List *list)
{
  assert(list);
  if (list->freed)
    pfree(list->freed);
  if (list->elems)
  {
    /* Free the element values of the list if they are not NULL */
    int cur = 0;
    while (cur != -1)
    {
      ListElem *e = &list->elems[cur];
      if (e->value)
        pfree(e->value);
      cur = e->next;
    }
    /* Free the element list */
    pfree(list->elems);
  }
  pfree(list);
  return;
}

/**
 * @brief Return the value at the head of the list
 */
void *
list_headval(List *list)
{
  return list->elems[list->elems->next].value;
}

/**
 * @brief Return the value at the tail of the skiplist
 */
void *
list_tailval(List *list)
{
  /* This is O(n) */
  ListElem *e = &list->elems[0];
  while (e->next != list->tail)
    e = &list->elems[e->next];
  return e->value;
}

/**
 * @brief Constructs a list from the array of values
 * @param[in] values Array of values
 * @param[in] count Number of elements in the array
 */
List *
list_make(void **values, int count)
{
  assert(count > 0);

  int capacity = LIST_INITIAL_CAPACITY;
  count += 2; /* Account for head and tail */
  while (capacity <= count)
    capacity <<= 1;
  List *result = palloc0(sizeof(List));
  result->elems = palloc0(sizeof(ListElem) * capacity);
  result->capacity = capacity;
  result->next = count;
  result->length = count - 2;

  /* Fill values first */
  result->elems[0].value = NULL; /* set head value to NULL */
  for (int i = 0; i < count - 2; i++)
    result->elems[i + 1].value = values[i];
  result->elems[count - 1].value = NULL; /* set tail value to NULL */
  result->tail = count - 1;
  return result;
}

#endif /* not used */

/*****************************************************************************/

