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
 * @brief Test of the periods during which a temporal point or a temporal
 * circular buffer is within a distance of a geometry, #tdwithin_tgeo_geo and
 * #tdwithin_tcbuffer_geo
 * @details Each case is built so that its answer is known in closed form, as
 * #disc_cases of ray_cast_unit.c builds its own: the coordinates are dyadic,
 * so every bound of a period falls on a whole microsecond. A point moving
 * along the x axis from -10 to 10 over 20 seconds, or a disc of radius 1/4
 * along it, is within 5/4 of the segment from (-1, 3/4) to (1, 3/4) while
 * |x| <= 2, from second 8 to second 12; within 1 of the end (4, 1) of a
 * segment at the single instant it passes x = 4, second 14; within 1/4 of
 * the square of side 2 about the
 * origin from second 8.75 to second 11.25, the instants inside it included;
 * and, starting at the centre of that square and moving along x to 20, within
 * 1/4 of it until second 1.25. Against arcs of the circle of radius 5 about
 * the origin, a point moving up the line x = 6 is within 1 of the arc from
 * (4, -3) through (5, 0) to (4, 3) at the single instant it passes (6, 0),
 * second 10, and never of the arc from (4, 3) through (-5, 0) to (4, -3),
 * whose ends are its nearest points; a point moving up the y axis from -8 to
 * 8 is within 1 of the upper half circle while 4 <= y <= 6, from second 15
 * to second 17.5, its distance to the arc being |5 - |y|| on the arc's side
 * and the distance to an end on the other. On the half circle of radius 25,
 * a point, or a disc of radius 2, moving along y = 12 from x = -40 to 40 is
 * within 12 of it, or 10 of the disc, while 13 <= |c| <= 37, 5 <= |x| <= 35:
 * two periods, from second 1.25 to 8.75 and from second 11.25 to 18.75.
 * A point moving along the x axis from -10 to 10 is within 1 of the disc of
 * radius 5 from second 4 to second 16, its inside included, and of the
 * annulus of radii 2 and 5 from second 4 to 9 and from second 11 to 16; one
 * moving up the y axis from -8 to 8 is within 1 of the half disc above the x
 * axis from second 8.75 to second 17.5.
 * Each runs at the scales 1, 2^-30 and 2^-40, which are exact on doubles, so
 * an answer depends on no magnitude.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <meos.h>
#include <meos_geo.h>
#if CBUFFER
  #include <meos_cbuffer.h>
#endif

static int failures = 0;
static int checks = 0;

/* Check that a temporal Boolean is true during exactly one period, from @p lo
 * to @p hi microseconds after the start of @p temp */
static void
check_period(const char *name, double s, const Temporal *temp,
  const Temporal *res, long long lo, long long hi)
{
  checks++;
  SpanSet *ss = res ? tbool_when_true(res) : NULL;
  long long got_lo = -1, got_hi = -1;
  int n = ss ? spanset_num_spans(ss) : 0;
  if (n == 1)
  {
    /* #spanset_span_n counts from 1 */
    Span *sp = spanset_span_n(ss, 1);
    TimestampTz t0 = temporal_start_timestamptz(temp);
    got_lo = (long long) (tstzspan_lower(sp) - t0);
    got_hi = (long long) (tstzspan_upper(sp) - t0);
    free(sp);
  }
  if (n != 1 || got_lo != lo || got_hi != hi)
  {
    failures++;
    printf("FAIL %-44s scale %-8g got %d period(s) [%lld, %lld], expected "
      "[%lld, %lld]\n", name, s, n, got_lo, got_hi, lo, hi);
  }
  free(ss);
  return;
}

/* Check that a temporal Boolean is true during exactly the @p n periods whose
 * bounds @p lo and @p hi are in microseconds after the start of @p temp */
static void
check_periods(const char *name, double s, const Temporal *temp,
  const Temporal *res, int n, const long long *lo, const long long *hi)
{
  checks++;
  SpanSet *ss = res ? tbool_when_true(res) : NULL;
  int got = ss ? spanset_num_spans(ss) : 0;
  bool ok = res && got == n;
  TimestampTz t0 = temporal_start_timestamptz(temp);
  for (int i = 0; ok && i < n; i++)
  {
    /* #spanset_span_n counts from 1 */
    Span *sp = spanset_span_n(ss, i + 1);
    ok = (long long) (tstzspan_lower(sp) - t0) == lo[i] &&
      (long long) (tstzspan_upper(sp) - t0) == hi[i];
    free(sp);
  }
  if (! ok)
  {
    failures++;
    printf("FAIL %-44s scale %-8g got %d period(s):", name, s, got);
    for (int i = 1; i <= got; i++)
    {
      Span *sp = spanset_span_n(ss, i);
      printf(" [%lld, %lld]", (long long) (tstzspan_lower(sp) - t0),
        (long long) (tstzspan_upper(sp) - t0));
      free(sp);
    }
    printf(", expected %d\n", n);
  }
  free(ss);
  return;
}

/* The arcs of the circle of radius 5 about the origin, and of the half circle
 * of radius 25 */
static void
arc_cases(double s)
{
  char wkt[512];
  snprintf(wkt, sizeof(wkt), "[POINT(%.17g %.17g)@2001-01-01 00:00:00+00, "
    "POINT(%.17g %.17g)@2001-01-01 00:00:20+00]", 6 * s, -10 * s, 6 * s,
    10 * s);
  Temporal *up6 = tgeompoint_in(wkt);
  snprintf(wkt, sizeof(wkt), "[POINT(0 %.17g)@2001-01-01 00:00:00+00, "
    "POINT(0 %.17g)@2001-01-01 00:00:20+00]", -8 * s, 8 * s);
  Temporal *upy = tgeompoint_in(wkt);
  snprintf(wkt, sizeof(wkt), "[POINT(%.17g %.17g)@2001-01-01 00:00:00+00, "
    "POINT(%.17g %.17g)@2001-01-01 00:00:20+00]", -40 * s, 12 * s, 40 * s,
    12 * s);
  Temporal *chord = tgeompoint_in(wkt);

  snprintf(wkt, sizeof(wkt), "CIRCULARSTRING(%.17g %.17g,%.17g 0,%.17g "
    "%.17g)", 4 * s, -3 * s, 5 * s, 4 * s, 3 * s);
  GSERIALIZED *right = geom_in(wkt, -1);
  snprintf(wkt, sizeof(wkt), "CIRCULARSTRING(%.17g %.17g,%.17g 0,%.17g "
    "%.17g)", 4 * s, 3 * s, -5 * s, 4 * s, -3 * s);
  GSERIALIZED *left = geom_in(wkt, -1);
  snprintf(wkt, sizeof(wkt), "CIRCULARSTRING(%.17g 0,0 %.17g,%.17g 0)",
    5 * s, 5 * s, -5 * s);
  GSERIALIZED *half = geom_in(wkt, -1);
  snprintf(wkt, sizeof(wkt), "CIRCULARSTRING(%.17g 0,0 %.17g,%.17g 0)",
    25 * s, 25 * s, -25 * s);
  GSERIALIZED *half25 = geom_in(wkt, -1);

  const long long graze[] = {10000000}, band_lo[] = {15000000},
    band_hi[] = {17500000}, two_lo[] = {1250000, 11250000},
    two_hi[] = {8750000, 18750000};
  Temporal *r;
  r = tdwithin_tgeo_geo(up6, right, s, false);
  check_periods("point grazing the outside of an arc", s, up6, r, 1, graze,
    graze);
  free(r);
  r = tdwithin_tgeo_geo(up6, left, s, false);
  check_periods("point passing the gap of an arc", s, up6, r, 0, NULL, NULL);
  free(r);
  r = tdwithin_tgeo_geo(upy, half, s, false);
  check_periods("point crossing the band of an arc", s, upy, r, 1, band_lo,
    band_hi);
  free(r);
  r = tdwithin_tgeo_geo(chord, half25, 12 * s, false);
  check_periods("point twice in the band of an arc", s, chord, r, 2, two_lo,
    two_hi);
  free(r);
#if CBUFFER
  snprintf(wkt, sizeof(wkt), "[Cbuffer(Point(%.17g %.17g),%.17g)@2001-01-01 "
    "00:00:00+00, Cbuffer(Point(%.17g %.17g),%.17g)@2001-01-01 00:00:20+00]",
    -40 * s, 12 * s, 2 * s, 40 * s, 12 * s, 2 * s);
  Temporal *disc = tcbuffer_in(wkt);
  r = tdwithin_tcbuffer_geo(disc, half25, 10 * s);
  check_periods("disc twice in the band of an arc", s, disc, r, 2, two_lo,
    two_hi);
  free(r);
  free(disc);
#endif

  free(up6); free(upy); free(chord);
  free(right); free(left); free(half); free(half25);
  return;
}

/* The areas arcs bound: the disc of radius 5 about the origin, the annulus
 * of radii 2 and 5, and the half disc above the x axis */
static void
region_cases(double s)
{
  char wkt[512];
  snprintf(wkt, sizeof(wkt), "[POINT(%.17g 0)@2001-01-01 00:00:00+00, "
    "POINT(%.17g 0)@2001-01-01 00:00:20+00]", -10 * s, 10 * s);
  Temporal *alongx = tgeompoint_in(wkt);
  snprintf(wkt, sizeof(wkt), "[POINT(0 %.17g)@2001-01-01 00:00:00+00, "
    "POINT(0 %.17g)@2001-01-01 00:00:20+00]", -8 * s, 8 * s);
  Temporal *upy = tgeompoint_in(wkt);

  snprintf(wkt, sizeof(wkt), "CURVEPOLYGON(CIRCULARSTRING(%.17g 0,%.17g 0,"
    "%.17g 0))", 5 * s, -5 * s, 5 * s);
  GSERIALIZED *disc = geom_in(wkt, -1);
  snprintf(wkt, sizeof(wkt), "CURVEPOLYGON(CIRCULARSTRING(%.17g 0,%.17g 0,"
    "%.17g 0),CIRCULARSTRING(%.17g 0,%.17g 0,%.17g 0))", 5 * s, -5 * s, 5 * s,
    2 * s, -2 * s, 2 * s);
  GSERIALIZED *annulus = geom_in(wkt, -1);
  snprintf(wkt, sizeof(wkt), "CURVEPOLYGON(COMPOUNDCURVE(CIRCULARSTRING("
    "%.17g 0,0 %.17g,%.17g 0),(%.17g 0,%.17g 0)))", 5 * s, 5 * s, -5 * s,
    -5 * s, 5 * s);
  GSERIALIZED *halfdisc = geom_in(wkt, -1);

  const long long disc_lo[] = {4000000}, disc_hi[] = {16000000},
    ann_lo[] = {4000000, 11000000}, ann_hi[] = {9000000, 16000000},
    half_lo[] = {8750000}, half_hi[] = {17500000};
  Temporal *r;
  r = tdwithin_tgeo_geo(alongx, disc, s, false);
  check_periods("point through a disc", s, alongx, r, 1, disc_lo, disc_hi);
  free(r);
  r = tdwithin_tgeo_geo(alongx, annulus, s, false);
  check_periods("point through an annulus", s, alongx, r, 2, ann_lo, ann_hi);
  free(r);
  r = tdwithin_tgeo_geo(upy, halfdisc, s, false);
  check_periods("point through a half disc", s, upy, r, 1, half_lo, half_hi);
  free(r);
#if CBUFFER
  snprintf(wkt, sizeof(wkt), "[Cbuffer(Point(%.17g 0),%.17g)@2001-01-01 "
    "00:00:00+00, Cbuffer(Point(%.17g 0),%.17g)@2001-01-01 00:00:20+00]",
    -10 * s, 0.5 * s, 10 * s, 0.5 * s);
  Temporal *cdisc = tcbuffer_in(wkt);
  r = tdwithin_tcbuffer_geo(cdisc, annulus, 0.5 * s);
  check_periods("disc through an annulus", s, cdisc, r, 2, ann_lo, ann_hi);
  free(r);
  free(cdisc);
#endif

  free(alongx); free(upy);
  free(disc); free(annulus); free(halfdisc);
  return;
}

static void
within_cases(double s)
{
  char wkt[512];
  /* The point and the disc moving along x from -10 s to 10 s over 20 s */
  snprintf(wkt, sizeof(wkt), "[POINT(%.17g 0)@2001-01-01 00:00:00+00, "
    "POINT(%.17g 0)@2001-01-01 00:00:20+00]", -10 * s, 10 * s);
  Temporal *point = tgeompoint_in(wkt);
  /* The point from the centre of the square along x to 20 s */
  snprintf(wkt, sizeof(wkt), "[POINT(0 0)@2001-01-01 00:00:00+00, "
    "POINT(%.17g 0)@2001-01-01 00:00:20+00]", 20 * s);
  Temporal *leaving = tgeompoint_in(wkt);

  snprintf(wkt, sizeof(wkt), "LINESTRING(%.17g %.17g,%.17g %.17g)", -s,
    0.75 * s, s, 0.75 * s);
  GSERIALIZED *seg = geom_in(wkt, -1);
  snprintf(wkt, sizeof(wkt), "LINESTRING(%.17g %.17g,%.17g %.17g)", 4 * s, s,
    4 * s, 3 * s);
  GSERIALIZED *end = geom_in(wkt, -1);
  snprintf(wkt, sizeof(wkt), "POLYGON((%.17g %.17g,%.17g %.17g,%.17g %.17g,"
    "%.17g %.17g,%.17g %.17g))", -s, -s, s, -s, s, s, -s, s, -s, -s);
  GSERIALIZED *square = geom_in(wkt, -1);

  Temporal *r;
  r = tdwithin_tgeo_geo(point, seg, 1.25 * s, false);
  check_period("point within 5/4 of the segment", s, point, r, 8000000,
    12000000);
  free(r);
#if CBUFFER
  /* The disc of radius 1/4 along the same motion, where circular buffers
   * are built */
  snprintf(wkt, sizeof(wkt), "[Cbuffer(Point(%.17g 0),%.17g)@2001-01-01 "
    "00:00:00+00, Cbuffer(Point(%.17g 0),%.17g)@2001-01-01 00:00:20+00]",
    -10 * s, 0.25 * s, 10 * s, 0.25 * s);
  Temporal *disc = tcbuffer_in(wkt);
  r = tdwithin_tcbuffer_geo(disc, seg, s);
  check_period("disc within 1 of the segment", s, disc, r, 8000000,
    12000000);
  free(r);
  free(disc);
#endif
  r = tdwithin_tgeo_geo(point, end, s, false);
  check_period("point grazing the end of a segment", s, point, r, 14000000,
    14000000);
  free(r);
  r = tdwithin_tgeo_geo(point, square, 0.25 * s, false);
  check_period("point through the square", s, point, r, 8750000, 11250000);
  free(r);
  r = tdwithin_tgeo_geo(leaving, square, 0.25 * s, false);
  check_period("point leaving the square", s, leaving, r, 0, 1250000);
  free(r);

  free(point); free(leaving);
  free(seg); free(end); free(square);
  return;
}

int
main(void)
{
  meos_initialize();
  meos_initialize_timezone("UTC");
  const double scales[] = {1.0, 0x1p-30, 0x1p-40};
  for (size_t k = 0; k < sizeof(scales) / sizeof(scales[0]); k++)
  {
    within_cases(scales[k]);
    arc_cases(scales[k]);
    region_cases(scales[k]);
  }
  printf("%d of %d checks pass\n", checks - failures, checks);
  meos_finalize();
  return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
