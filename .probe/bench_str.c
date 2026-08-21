/* SPDX-License-Identifier: PostgreSQL */
/*
 * bench_str.c — measure the reload path: building an RTree by repeated
 * rtree_insert (what a binding must do today to rebuild a persisted index)
 * against rtree_load, the Sort-Tile-Recursive bulk build.
 *
 * Correctness is checked before timing is reported: both trees must answer a
 * set of window queries with the identical id set. A faster build that answers
 * differently is not a faster build.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <meos.h>
#include <meos_geo.h>

static double
now_s(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double) ts.tv_sec + (double) ts.tv_nsec / 1e9;
}

static int
cmp_i64(const void *a, const void *b)
{
  int64 x = *(const int64 *) a, y = *(const int64 *) b;
  return (x > y) - (x < y);
}

int
main(int argc, char **argv)
{
  int n = (argc > 1) ? atoi(argv[1]) : 500000;
  meos_initialize();

  /* Build the entry set once; generation is not part of any measurement. */
  STBox *boxes = malloc(sizeof(STBox) * (size_t) n);
  int64 *ids = malloc(sizeof(int64) * (size_t) n);
  char buf[256];
  for (int i = 0; i < n; i++)
  {
    int x = i % 1000, y = i / 1000;
    snprintf(buf, sizeof(buf),
      "STBOX XT(((%d,%d),(%d,%d)),[2000-01-01,2000-01-02])", x, y, x + 1, y + 1);
    STBox *b = stbox_in(buf);
    memcpy(&boxes[i], b, sizeof(STBox));
    free(b);
    ids[i] = i;
  }

  /* A: one-at-a-time insert — today's rebuild path. */
  double t0 = now_s();
  RTree *ins = rtree_create_stbox();
  for (int i = 0; i < n; i++)
    rtree_insert(ins, &boxes[i], ids[i]);
  double t_insert = now_s() - t0;

  /* B: STR bulk load. */
  t0 = now_s();
  RTree *pak = rtree_create_stbox();
  rtree_load(pak, boxes, ids, n);
  double t_load = now_s() - t0;

  /* Correctness: identical id sets over a spread of windows. */
  int mismatches = 0, total_hits = 0;
  double q_ins = 0, q_pak = 0;
  for (int q = 0; q < 200; q++)
  {
    int x = (q * 37) % 900, y = (q * 53) % 400;
    snprintf(buf, sizeof(buf),
      "STBOX XT(((%d,%d),(%d,%d)),[2000-01-01,2000-01-02])", x, y, x + 20, y + 20);
    STBox *query = stbox_in(buf);

    MeosArray *ra = meos_array_create(sizeof(int64));
    MeosArray *rb = meos_array_create(sizeof(int64));
    double s = now_s(); rtree_search(ins, RTREE_OVERLAPS, query, ra); q_ins += now_s() - s;
    s = now_s();        rtree_search(pak, RTREE_OVERLAPS, query, rb); q_pak += now_s() - s;

    int ca = meos_array_count(ra), cb = meos_array_count(rb);
    if (ca != cb) { mismatches++; }
    else
    {
      int64 *va = malloc(sizeof(int64) * (size_t) (ca ? ca : 1));
      int64 *vb = malloc(sizeof(int64) * (size_t) (cb ? cb : 1));
      for (int k = 0; k < ca; k++) va[k] = *(int64 *) meos_array_get(ra, k);
      for (int k = 0; k < cb; k++) vb[k] = *(int64 *) meos_array_get(rb, k);
      qsort(va, (size_t) ca, sizeof(int64), cmp_i64);
      qsort(vb, (size_t) cb, sizeof(int64), cmp_i64);
      if (ca && memcmp(va, vb, sizeof(int64) * (size_t) ca) != 0) mismatches++;
      free(va); free(vb);
    }
    total_hits += ca;
    meos_array_destroy(ra); meos_array_destroy(rb);
    free(query);
  }

  printf("n=%d\n", n);
  printf("  build by insert : %8.3f s\n", t_insert);
  printf("  build by load   : %8.3f s   (%.1fx faster)\n", t_load, t_insert / t_load);
  printf("  200 queries insert-tree: %7.4f s\n", q_ins);
  printf("  200 queries packed-tree: %7.4f s   (%.2fx)\n", q_pak, q_ins / q_pak);
  printf("  hits=%d  mismatching queries=%d  %s\n", total_hits, mismatches,
         mismatches ? "*** RESULTS DIFFER ***" : "identical result sets");

  rtree_free(ins); rtree_free(pak);
  free(boxes); free(ids);
  meos_finalize();
  return mismatches ? 1 : 0;
}
