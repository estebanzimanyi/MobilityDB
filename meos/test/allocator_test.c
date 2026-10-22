/*****************************************************************************
 *
 * This MobilityDB code is provided under The PostgreSQL License.
 * Copyright (c) 2016-2026, Université libre de Bruxelles and MobilityDB
 * contributors
 *
 * MobilityDB includes portions of PostGIS version 3 source code released
 * under the GNU General Public License (GPLv2 or later).
 * Copyright (c) 2001-2026, PostGIS contributors
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
 * @brief A program that tests the pluggable allocator hook of the MEOS API
 * @details It installs a counting allocator via #meos_initialize_allocator,
 * builds and
 * frees a temporal value, and verifies that (a) the custom allocator was
 * actually invoked (interposition) and (b) the tracked live bytes return to
 * zero after freeing (no leak, so free routes through the hook too). It then
 * installs an allocator that knows the pointers it handed out and runs the
 * geometry operations, whose objects liblwgeom builds, checking that the hook
 * frees nothing it did not allocate and gets back everything it did. Finally
 * it reinstalls the default (libc) allocator.
 *
 * The program can be build as follows
 * @code
 * gcc -Wall -g -I/usr/local/include -o allocator_test allocator_test.c -L/usr/local/lib -lmeos
 * @endcode
 */

#include <assert.h>
#include <malloc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <meos.h>
#include <meos_geo.h>
#include <meos_pose.h>

/* The palloc/pfree entry points route MEOS working-memory allocations through
 * the hook. They are exported by libmeos but are not part of the typed API, so
 * declare them here. */
extern void *palloc(size_t size);
extern void pfree(void *ptr);

/*
 * Counting allocator. It is pointer-preserving (as a real embedder such as
 * DuckDB is), tracking the live byte count through malloc_usable_size so that
 * a value allocated through it can be released either with the hook (pfree) or
 * with libc free.
 */
static size_t live_bytes = 0;
static long n_malloc = 0;
static long n_realloc = 0;
static long n_free = 0;

static void *
counting_malloc(size_t size)
{
  void *p = malloc(size);
  if (p)
  {
    live_bytes += malloc_usable_size(p);
    n_malloc++;
  }
  return p;
}

static void *
counting_realloc(void *ptr, size_t size)
{
  size_t old = ptr ? malloc_usable_size(ptr) : 0;
  void *p = realloc(ptr, size);
  if (p)
  {
    live_bytes += malloc_usable_size(p);
    live_bytes -= old;
    n_realloc++;
  }
  return p;
}

static void
counting_free(void *ptr)
{
  if (ptr)
  {
    live_bytes -= malloc_usable_size(ptr);
    n_free++;
    free(ptr);
  }
}

/*
 * Owning allocator. It records every pointer it hands out in an open-addressing
 * table, so that a free of a pointer it never allocated, as a host allocator
 * that owns its blocks would see it, is counted rather than released blindly.
 */
#define OWN_SLOTS (1 << 16)
#define OWN_TOMB ((void *) 1)
static void *own_slot[OWN_SLOTS];
static size_t own_live = 0;
static long own_foreign = 0;

static size_t
own_hash(void *p)
{
  return (size_t) ((((uintptr_t) p) >> 4) * 11400714819323198485ull) &
    (OWN_SLOTS - 1);
}

static void
own_put(void *p)
{
  size_t i = own_hash(p);
  while (own_slot[i] && own_slot[i] != OWN_TOMB)
    i = (i + 1) & (OWN_SLOTS - 1);
  own_slot[i] = p;
  own_live += malloc_usable_size(p);
}

/* Forget a pointer the allocator handed out; count one it never did */
static void
own_take(void *p)
{
  for (size_t i = own_hash(p); own_slot[i]; i = (i + 1) & (OWN_SLOTS - 1))
  {
    if (own_slot[i] == p)
    {
      own_slot[i] = OWN_TOMB;
      own_live -= malloc_usable_size(p);
      return;
    }
  }
  own_foreign++;
}

static void *
owning_malloc(size_t size)
{
  void *p = malloc(size);
  if (p)
    own_put(p);
  return p;
}

static void *
owning_realloc(void *ptr, size_t size)
{
  if (ptr)
    own_take(ptr);
  void *p = realloc(ptr, size);
  if (p)
    own_put(p);
  return p;
}

static void
owning_free(void *ptr)
{
  if (ptr)
  {
    own_take(ptr);
    free(ptr);
  }
}

/* Geometry operations whose objects liblwgeom builds */
static void
geo_case_pose_distance(void)
{
  Pose *pose1 = pose_make_2d(1, 1, 0.5, false, 0);
  Pose *pose2 = pose_make_2d(4, 5, 0, false, 0);
  double dist = distance_pose_pose(pose1, pose2);
  assert(dist == 5.0);
  pfree(pose1); pfree(pose2);
}

static void
geo_case_pose_to_point(void)
{
  Pose *pose = pose_make_3d(1, 2, 3, 1, 0, 0, 0, false, 0);
  GSERIALIZED *gs = pose_to_point(pose);
  assert(gs != NULL);
  pfree(gs); pfree(pose);
}

static void
geo_case_geom_in_out(void)
{
  GSERIALIZED *gs = geom_in("Linestring(1 1,2 2,3 1)", -1);
  char *hexwkb = geo_out(gs);
  char *wkt = geo_as_text(gs, 6);
  assert(hexwkb != NULL && wkt != NULL);
  pfree(wkt); pfree(hexwkb); pfree(gs);
}

static void
geo_case_trajectory(void)
{
  Temporal *temp = tgeompoint_in(
    "[Point(1 1)@2001-01-01, Point(3 3)@2001-01-02, Point(3 1)@2001-01-03]");
  GSERIALIZED *traj = tpoint_trajectory(temp, false);
  assert(traj != NULL);
  pfree(traj); pfree(temp);
}

static void
geo_case_gaps_by_distance(void)
{
  TInstant *instants[3];
  const char *wkt[3] = {"Point(1 1)", "Point(2 1)", "Point(20 1)"};
  const char *ts[3] = {"2001-01-01", "2001-01-02", "2001-01-03"};
  for (int i = 0; i < 3; i++)
  {
    GSERIALIZED *gs = geom_in(wkt[i], -1);
    instants[i] = tpointinst_make(gs, timestamptz_in(ts[i], -1));
    pfree(gs);
  }
  TSequenceSet *ss = tsequenceset_make_gaps(instants, 3, LINEAR, NULL, 5.0);
  assert(ss != NULL && temporal_num_sequences((Temporal *) ss) == 2);
  pfree(ss);
  for (int i = 0; i < 3; i++)
    pfree(instants[i]);
}

/* Main program */
int main(void)
{
  const char *tint_str = "[1@2001-01-01, 2@2001-01-02, 3@2001-01-03]";

  /* Initialize MEOS (which installs the default libc allocator) */
  meos_initialize();
  meos_initialize_timezone("UTC");

  /* Warm up the lazily-initialized global caches (type cache, timezone and
   * collation) under the default allocator, so that they do not leave residual
   * live bytes once the counting allocator is installed. */
  Temporal *warm = tint_in(tint_str);
  char *warm_out = tint_out(warm);
  pfree(warm_out);
  pfree(warm);

  printf("****************************************************************\n");
  printf("* Allocator hook *\n");
  printf("****************************************************************\n");

  /* Install the counting allocator */
  meos_initialize_allocator(counting_malloc, counting_realloc, counting_free);

  /* Nothing has been allocated through the hook yet */
  assert(live_bytes == 0);

  /* Build a temporal value: its working memory now routes through the hook */
  Temporal *temp = tint_in(tint_str);
  assert(n_malloc > 0);         /* interposition: the custom malloc ran */
  assert(live_bytes > 0);       /* the value's memory is tracked by the hook */
  size_t peak = live_bytes;

  /* Exercise more of the allocation path */
  char *temp_out = tint_out(temp);
  assert(temp_out != NULL);

  printf("custom malloc calls: %ld\n", n_malloc);
  printf("custom realloc calls: %ld\n", n_realloc);
  printf("live bytes after building a tint value: %zu\n", peak);

  /* Free everything through the hook (pfree routes to the custom free) */
  pfree(temp_out);
  pfree(temp);

  printf("custom free calls: %ld\n", n_free);
  printf("live bytes after freeing: %zu\n", live_bytes);

  /* Interposition of free + zero-leak: every hook allocation was released
   * through the hook, so the tracked live bytes return to zero */
  assert(n_free > 0);
  assert(live_bytes == 0);

  printf("Allocator hook interposition and zero-leak: OK\n");
  printf("****************************************************************\n");

  /* Geometry operations under an allocator that owns its blocks: the objects
   * liblwgeom builds come from the hook as the ones MEOS builds do, so the
   * hook frees only what it allocated and gets all of it back */
  struct { const char *name; void (*fn)(void); } geo_cases[] = {
    {"distance_pose_pose", geo_case_pose_distance},
    {"pose_to_point", geo_case_pose_to_point},
    {"geom_in, geo_out, geo_as_text", geo_case_geom_in_out},
    {"tpoint_trajectory", geo_case_trajectory},
    {"tsequenceset_make_gaps by distance", geo_case_gaps_by_distance},
  };
  int ngeo = (int) (sizeof(geo_cases) / sizeof(geo_cases[0]));
  /* Warm the lazily-initialized caches these operations read */
  meos_initialize_allocator(NULL, NULL, NULL);
  for (int i = 0; i < ngeo; i++)
    geo_cases[i].fn();
  meos_initialize_allocator(owning_malloc, owning_realloc, owning_free);
  for (int i = 0; i < ngeo; i++)
  {
    own_foreign = 0;
    size_t live0 = own_live;
    geo_cases[i].fn();
    printf("%s: %ld foreign free(s), %zu byte(s) left\n", geo_cases[i].name,
      own_foreign, own_live - live0);
    assert(own_foreign == 0);
    assert(own_live == live0);
  }
  printf("Allocator hook owns the liblwgeom objects: OK\n");
  printf("****************************************************************\n");

  /* Reinstall the default allocator and finalize MEOS */
  meos_initialize_allocator(NULL, NULL, NULL);
  meos_finalize();

  return 0;
}
