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
 * @brief STBox expandable array type.
 */

/* C */
#include <assert.h>
#include <limits.h>
/* PostgreSQL */
#include <postgres.h>
#if MEOS
  #define MaxAllocSize   ((Size) 0x3fffffff) /* 1 gigabyte - 1 */
#else
  #include <utils/memutils.h>
#endif /* MEOS */
/* MEOS */
#include <meos.h>
#include <meos_internal.h>
#include <general/temporal_boxops.h>

/*****************************************************************************/

/**
 * @brief Return the position to store an additional element in the array
 * @return On error return @p INT_MAX
 */
static int
boxarray_alloc(BoxArray *array)
{
  /* Increase the number of values stored in the array */
  array->count++;

  /* If there is no more available space expand the list */
  if (array->count >= array->maxcount)
  {
    /* PostgreSQL has a limit of MaxAllocSize = 1 gigabyte - 1. By default,
     * the array doubles the size when expanded. If doubling the size goes
     * beyond MaxAllocSize, we allocate the maximum number of elements that
     * fit within MaxAllocSize. If this maximum has been previously reached
     * and more capacity is required, an error is generated. */
    if (array->maxcount == (int) floor(MaxAllocSize / array->boxsize))
    {
      meos_error(ERROR, MEOS_ERR_MEMORY_ALLOC_ERROR,
        "No more memory available to store the bounding boxes");
      return INT_MAX;
    }
    if (array->boxsize * (array->maxcount << 2) > MaxAllocSize)
      array->maxcount = (int) floor(MaxAllocSize / array->boxsize);
    else
      array->maxcount <<= BOXARRAY_GROW;
    void *newelems = repalloc(array->elems, array->boxsize * array->maxcount);
    if (newelems != NULL)
      array->elems = newelems;
    else
    {
      meos_error(ERROR, MEOS_ERR_MEMORY_ALLOC_ERROR,
        "Error reallocating memory to store the bounding boxes");
      return INT_MAX;
    }
#if DEBUG_EXPAND
    meos_error(WARNING, 0, " Array -> %d ", array->maxcount);
#endif /* DEBUG_EXPAND */
  }

  /* Return the first available entry */
  return array->count - 1;
}

/**
 * @brief Constructs a box array
 * @param[in] boxtype Type of the box
 * @param[in] count Intial number of boxes in the array. 
 * If count < 1,the default initial capacity is allocated.
 */
BoxArray *
boxarray_make(meosType boxtype, int count)
{
  if (count < 1)
    count = BOXARRAY_INITIAL_CAPACITY;
  BoxArray *result = palloc0(sizeof(BoxArray));
  size_t boxsize = bbox_get_size(boxtype);
  result->elems = palloc0(boxsize * count);
  result->boxtype = boxtype;
  result->boxsize = boxsize;
  result->maxcount = count;
  return result;
}

/**
 * @ingroup meos_internal_temporal_agg
 * @brief Delete the box array and free its allocated memory
 * @param[in] array Bounding box array
 */
void
boxarray_free(BoxArray *array)
{
  if (! array)
    return;
  if (array->elems)
    pfree(array->elems);
  pfree(array);
  return;
}

/**
 * @brief Constructs a array from the array of values values
 * @param[in] box Box to add
 * @param[inout] array Box array
 */
bool
boxarray_add(BoxArray *array, const void *box)
{
  assert(box);
  int idx = boxarray_alloc(array);
  if (idx == INT_MAX)
    return false;
  uint8_t *elemptr = (uint8_t *) array->elems;
  elemptr += array->boxsize * idx;
  memcpy(elemptr, box, array->boxsize);
  return true;
}

/**
 * @brief Get the n-th bounding box from the array
 * @param[in] array Array 
 * @param[in] n Index
 * @param[out] result Bounding box
 */
void
boxarray_n(const BoxArray *array, int n, void *result)
{
  if (n < 0 || n >= array->count)
    return;
  uint8_t *elemptr = ((uint8_t *) array->elems) + (array->boxsize * n);
  memcpy(result, elemptr, array->boxsize);
  return;
}

/*****************************************************************************/

