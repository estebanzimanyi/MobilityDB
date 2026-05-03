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
 * @brief First piece of test coverage for the KEYVALUE skiplist surface.
 *
 * Exercises the supported streaming pattern documented in
 * `doc/drafts/keyval_skiplist_continuation_plan.md` section 14:
 * search-then-splice-on-miss with `count = 1`, caller-managed in-place
 * mutation via `merge_fn` returning `left`. The same pattern as
 * `meos/examples/ais_expand_skiplist.c` but reduced to int32 keys and
 * int64 counters so the test does not pull in any temporal type and can
 * verify outputs by direct integer comparison.
 *
 * What the test catches:
 *   * Bug 5 in PR #685 (`skiplist_splice` KEYVALUE branch failing to
 *     assign the freshly-allocated key to `newelem->key`). Every
 *     search-after-splice in the loop dereferences `newelem->key`
 *     through the user-supplied `comp_fn`; without the fix
 *     (`40718a71b` on `bug-audit/all`), the test crashes or returns
 *     spurious not-found results.
 *   * Bug 7 in PR #685 (`memset(update, 0, sizeof(&update))` truncated
 *     to pointer size). With the fix (`b72935f3d`), `update[]` is
 *     correctly zeroed across all SKIPLIST_MAXLEVEL slots; without
 *     the fix the level pointers are corrupted and search may miss
 *     valid elements.
 *   * Memory leaks on the canonical path. Verified ASan-clean.
 *
 * What the test does NOT exercise:
 *   * The batch-merge path inside `keyval_skiplist_merge`. Bugs 1-4
 *     in that function remain unvalidated; the streaming pattern
 *     bypasses it entirely. See
 *     `doc/drafts/keyval_skiplist_continuation_plan.md` sections
 *     9-10 for the per-bug details.
 *
 * The program can be built as follows:
 * @code
 * gcc -Wall -g -I/usr/local/include -o keyval_skiplist_test \
 *     keyval_skiplist_test.c -L/usr/local/lib -lmeos
 * @endcode
 *
 * Run under ASan to verify leak-freedom:
 * @code
 * gcc -Wall -g -fsanitize=address -I/usr/local/include \
 *     -o keyval_skiplist_test.asan keyval_skiplist_test.c \
 *     -L/usr/local/lib -lmeos
 * ASAN_OPTIONS=detect_leaks=1 ./keyval_skiplist_test.asan
 * @endcode
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <meos.h>
/* SkipList APIs and the SkipListElem layout are MEOS-internal */
#include <meos_internal.h>

#define EXPECTED_UNIQUE_KEYS 5
#define RANDOM_SEED 42

typedef struct
{
  int32_t key;
  int64_t count;
} keyval_record;

/**
 * @brief Comparison function on the embedded `key` field of two
 * keyval_record values.
 *
 * Matches the AIS example's pattern: the skiplist is created with
 * `key_size = 0` and the key is embedded in the value. `comp_fn`
 * is then passed value pointers (treated as keys).
 */
static int
kv_record_comp(void *a, void *b)
{
  int32_t k1 = ((keyval_record *) a)->key;
  int32_t k2 = ((keyval_record *) b)->key;
  return (k1 < k2) ? -1 : (k1 > k2) ? 1 : 0;
}

/**
 * @brief Merge two keyval_record values by adding `right->count` into
 * `left->count` and returning `left`.
 *
 * Documents the supported mutate-in-place convention: returns `left`,
 * leaves `right` untouched. The skiplist machinery never invokes this
 * directly under the streaming pattern (the test mutates the slot
 * directly via `list->elems[pos]`), but it is provided to
 * `skiplist_make` so the surface is exercised.
 */
static void *
kv_record_merge(void *left, void *right)
{
  ((keyval_record *) left)->count +=
    ((keyval_record *) right)->count;
  return left;
}

/* Test fixture: 5 unique keys with the listed per-key target counts.
 * Total 100 records once the per-key arrays are flattened. */
static const int32_t fixture_keys[EXPECTED_UNIQUE_KEYS] =
  { 100, 200, 300, 400, 500 };
static const int     fixture_target_counts[EXPECTED_UNIQUE_KEYS] =
  {  10,  20,  30,  15,  25 };

int
main(void)
{
  meos_initialize();
  meos_initialize_timezone("UTC");

  /* ---- Setup: build a record stream with shuffled keys ---- */
  int total_records = 0;
  for (int i = 0; i < EXPECTED_UNIQUE_KEYS; i++)
    total_records += fixture_target_counts[i];

  keyval_record *records = malloc(total_records * sizeof(keyval_record));
  if (! records)
  {
    fprintf(stderr, "FAIL: malloc for records array\n");
    meos_finalize();
    return 1;
  }
  int idx = 0;
  for (int i = 0; i < EXPECTED_UNIQUE_KEYS; i++)
    for (int j = 0; j < fixture_target_counts[i]; j++)
    {
      records[idx].key = fixture_keys[i];
      records[idx].count = 1;
      idx++;
    }
  /* Fisher-Yates shuffle so records arrive in non-key-sorted order */
  srand(RANDOM_SEED);
  for (int i = total_records - 1; i > 0; i--)
  {
    int j = rand() % (i + 1);
    keyval_record tmp = records[i];
    records[i] = records[j];
    records[j] = tmp;
  }

  /* ---- Phase 1: build the per-key accumulator skiplist ---- */
  /* key_size = 0 — keys are embedded in values, comp_fn dispatches on
   * the embedded `.key` field. Same as ais_expand_skiplist.c. */
  SkipList *list = skiplist_make(0, sizeof(keyval_record),
    &kv_record_comp, &kv_record_merge);
  if (! list)
  {
    fprintf(stderr, "FAIL: skiplist_make returned NULL\n");
    free(records);
    meos_finalize();
    return 1;
  }

  for (int i = 0; i < total_records; i++)
  {
    keyval_record *rec_p = &records[i];
    int pos = skiplist_search(list, NULL, rec_p);
    if (pos < 0)
    {
      /* Not found: splice the slot, then re-search to land in it.
       * Bug 5 (skiplist_splice's KEYVALUE branch failing to assign
       * newkey to newelem->key) would make the re-search dereference
       * NULL inside comp_fn and either crash or return -1 again. */
      keyval_record initial = { .key = rec_p->key, .count = 0 };
      keyval_record *initial_p = &initial;
      skiplist_splice(list, NULL, (void **) &initial_p, 1, NULL, false,
        KEYVALUE);
      pos = skiplist_search(list, NULL, rec_p);
      if (pos < 0)
      {
        fprintf(stderr,
          "FAIL: search after splice returned not-found for key %"
          PRId32 " (record %d)\n", rec_p->key, i);
        skiplist_free(list);
        free(records);
        meos_finalize();
        return 1;
      }
    }
    /* Mutate the slot in place: this is the supported pattern */
    SkipListElem *e = (SkipListElem *) &list->elems[pos];
    keyval_record *slot = (keyval_record *) e->value;
    slot->count += rec_p->count;
  }

  /* ---- Phase 2: walk the skiplist and verify per-key counts ---- */
  int verified_keys = 0;
  for (int cur = list->elems[0].next[0]; cur != list->tail;
       cur = list->elems[cur].next[0])
  {
    keyval_record *slot = (keyval_record *) list->elems[cur].value;
    int found_target = -1;
    for (int i = 0; i < EXPECTED_UNIQUE_KEYS; i++)
      if (fixture_keys[i] == slot->key)
      {
        found_target = i;
        break;
      }
    if (found_target < 0)
    {
      fprintf(stderr, "FAIL: skiplist contains unexpected key %" PRId32 "\n",
        slot->key);
      skiplist_free(list);
      free(records);
      meos_finalize();
      return 1;
    }
    if ((int64_t) fixture_target_counts[found_target] != slot->count)
    {
      fprintf(stderr,
        "FAIL: key %" PRId32 " has count %" PRId64 ", expected %d\n",
        slot->key, slot->count, fixture_target_counts[found_target]);
      skiplist_free(list);
      free(records);
      meos_finalize();
      return 1;
    }
    verified_keys++;
  }
  if (verified_keys != EXPECTED_UNIQUE_KEYS)
  {
    fprintf(stderr,
      "FAIL: skiplist contains %d unique keys, expected %d\n",
      verified_keys, EXPECTED_UNIQUE_KEYS);
    skiplist_free(list);
    free(records);
    meos_finalize();
    return 1;
  }

  printf("OK: %d records folded into %d unique keys, all per-key counts match\n",
    total_records, EXPECTED_UNIQUE_KEYS);

  /* ---- Cleanup ---- */
  skiplist_free(list);
  free(records);
  meos_finalize();
  return 0;
}
