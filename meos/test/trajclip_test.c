/*****************************************************************************
 *
 * Verification harness for the Clipper2-backed trajectory-vs-polygon clip.
 * Reproduces the two audit cases from doc/drafts/FAST_CLIP_ANALYSIS.md
 * (recovered from origin/clip-clipper2-prod history) and prints whether
 * each output matches the expected inside-polygon segment.
 *
 * Build:
 *   gcc -Wall -g -O2 -I<prefix>/include -o trajclip_test trajclip_test.c \
 *     -L<prefix>/lib -lmeos
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <meos.h>
#include <meos_geo.h>

static int
run_case(const char *name, const char *traj_in, const char *poly_in,
  const char *expect_out)
{
  Temporal *traj = (Temporal *) tgeompoint_in(traj_in);
  GSERIALIZED *poly = (GSERIALIZED *) geom_in(poly_in, -1);
  if (! traj || ! poly)
  {
    printf("[FAIL] %s — parse failed\n", name);
    return 1;
  }
  Temporal *clipped = tgeo_at_geom(traj, poly);
  char *got = clipped ? tspatial_as_text(clipped, 6) : strdup("(empty)");
  int pass = expect_out ? (strcmp(got, expect_out) == 0) : (clipped == NULL);
  printf("%s %s\n  in:       %s\n  poly:     %s\n  expected: %s\n  got:      %s\n",
    pass ? "[PASS]" : "[FAIL]", name, traj_in, poly_in,
    expect_out ? expect_out : "(empty)", got);
  free(got);
  if (clipped) free(clipped);
  free(traj);
  free(poly);
  return pass ? 0 : 1;
}

int
main(void)
{
  meos_initialize();
  meos_initialize_timezone("UTC");

  int fails = 0;

  /* Bug A repro — entry/exit on a triangle. The pre-fix fast clip dropped
   * every diagonal edge and returned an empty result. */
  fails += run_case("triangle entry/exit",
    "[POINT(0 5)@2026-01-01 00:00:00, POINT(10 5)@2026-01-01 02:00:00]",
    "POLYGON((0 0,10 0,5 10,0 0))",
    "{[POINT(2.5 5)@2026-01-01 00:30:00+00, POINT(7.5 5)@2026-01-01 01:30:00+00]}");

  /* Bug B repro — diamond. The parity sweep treated the exit at (10,0) as
   * a second entry and kept the inside-flag set through the trajectory's
   * end at (11,0). */
  fails += run_case("diamond entry/exit",
    "[POINT(-1 0)@2026-01-01 00:00:00, POINT(11 0)@2026-01-01 12:00:00]",
    "POLYGON((5 -5,10 0,5 5,0 0,5 -5))",
    "{[POINT(0 0)@2026-01-01 01:00:00+00, POINT(10 0)@2026-01-01 11:00:00+00]}");

  /* Trajectory entirely inside polygon — full sequence preserved. */
  fails += run_case("fully inside",
    "[POINT(2 2)@2026-01-01 00:00:00, POINT(3 3)@2026-01-01 01:00:00]",
    "POLYGON((0 0,10 0,10 10,0 10,0 0))",
    "{[POINT(2 2)@2026-01-01 00:00:00+00, POINT(3 3)@2026-01-01 01:00:00+00]}");

  /* Trajectory entirely outside polygon — empty result. */
  fails += run_case("fully outside",
    "[POINT(20 20)@2026-01-01 00:00:00, POINT(30 30)@2026-01-01 01:00:00]",
    "POLYGON((0 0,10 0,10 10,0 10,0 0))",
    NULL);

  /* Trajectory passes through a hole — should produce two inside-segments. */
  fails += run_case("polygon with hole",
    "[POINT(0 5)@2026-01-01 00:00:00, POINT(10 5)@2026-01-01 02:00:00]",
    "POLYGON((0 0,10 0,10 10,0 10,0 0),(3 3,7 3,7 7,3 7,3 3))",
    "{[POINT(0 5)@2026-01-01 00:00:00+00, POINT(3 5)@2026-01-01 00:36:00+00], [POINT(7 5)@2026-01-01 01:24:00+00, POINT(10 5)@2026-01-01 02:00:00+00]}");

  meos_finalize();
  printf("\n%d failure(s)\n", fails);
  return fails == 0 ? 0 : 1;
}
