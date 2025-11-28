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
 * @brief A simple program that tests the funtions for the temporal rgeo
 * types in MEOS, that is, rgeo, poseset, and tpose.
 *
 * The program can be build as follows
 * @code
 * gcc -Wall -g -I/usr/local/include -o rgeo_test rgeo_test.c -L/usr/local/lib -lmeos
 * @endcode
 */

#include <stdio.h>
#include <stdlib.h>
#include <meos.h>
#include <meos_pose.h>

/* Main program */
int main(void)
{
  /* Initialize MEOS */
  meos_initialize();
  meos_initialize_timezone("UTC");

  /* Create values to test the functions of the API */

  double theta1 = 1.5707963267948966, theta2 = 3.141592653589793;
  double x1 = 1, y1 = 1, z1 = 1, X1 = 1, Y1 = 0, Z1 = 0, W1 = 0;
  double x2 = 2, y2 = 2, z2 = 2, X2 = 0.5, Y2 = 0.5, Z2 = 0.5, W2 = .5;

  char *tstz1_in = "2001-01-01";
  TimestampTz tstz1 = timestamptz_in(tstz1_in, -1);
  char *tstz1_out = timestamptz_out(tstz1);
  char *tstzspan1_in = "[2001-01-01, 2001-01-03]";
  Span *tstzspan1 = tstzspan_in(tstzspan1_in);
  char *tstzspan1_out = tstzspan_out(tstzspan1);

  text *text_null = text_in("NULL");

  char *pose1_in = "SRID=5676;Pose(Point(1 1), 1)";
  Pose *pose1 = rgeo_in(pose1_in);
  char *pose1_out = rgeo_as_text(pose1, 6);
  char *pose2_in = "SRID=5676;Pose(Point(2 2), 1)";
  Pose *pose2 = rgeo_in(pose2_in);
  char *pose2_out = rgeo_as_text(pose2, 6);

  char *pose1_3d_in = "SRID=5676;Pose(Point(1 1 1), 1, 0, 0, 0)";
  Pose *pose1_3d = rgeo_in(pose1_3d_in);
  char *pose1_3d_out = rgeo_as_text(pose1_3d, 6);
  char *pose2_3d_in = "SRID=5676;Pose(Point(2 2 2), 0.5, 0.5, 0.5, 0.5)";
  Pose *pose2_3d = rgeo_in(pose2_3d_in);
  char *pose2_3d_out = rgeo_as_text(pose2_3d, 6);

  char *poseset1_in = "SRID=5676;{\"Pose(Point(1 1), 1)\", \"Pose(Point(2 2), 1)\"}";
  Set *poseset1 = poseset_in(poseset1_in);
  char *poseset1_out = spatialset_as_text(poseset1, 6);

  char *geom1_in = "SRID=5676;Point(1 1)";
  GSERIALIZED *geom1 = geom_in(geom1_in, -1);
  char *geom1_out = geo_as_text(geom1, 6);

  Pose *posearray[2];

  char *tfloat1_in = "[1@2001-01-01, 3@2001-01-03]";
  Temporal *tfloat1 = tfloat_in(tfloat1_in);
  char *tfloat1_out = tfloat_out(tfloat1, 6);

  char *stbox1_in = "SRID=5676;STBOX XT(((1,1),(3,3)),[2001-01-01, 2001-01-03])";
  STBox *stbox1 = stbox_in(stbox1_in);
  char *stbox1_out = stbox_out(stbox1, 6);

  char *stbox1_3d_in = "SRID=5676;STBOX ZT(((1,1,1),(3,3,3)),[2001-01-01, 2001-01-03])";
  STBox *stbox1_3d = stbox_in(stbox1_3d_in);
  char *stbox1_3d_out = stbox_out(stbox1_3d, 6);

  char *tgeompt1_in = "SRID=5676;[Point(1 1)@2001-01-01, Point(3 3)@2001-01-03]";
  Temporal *tgeompt1 = tgeompoint_in(tgeompt1_in);
  char *tgeompt1_out = tspatial_as_text(tgeompt1, 6);

  char *tpose1_in = "SRID=5676;[Pose(Point(1 1), 1)@2001-01-01, Pose(Point(3 3), 1)@2001-01-03]";
  Temporal *tpose1 = trgeo_in(tpose1_in);
  char *tpose1_out = tspatial_as_text(tpose1, 6);
  char *tpose2_in = "SRID=5676;[Pose(Point(3 3), 1)@2001-01-01, Pose(Point(1 1), 1)@2001-01-03]";
  Temporal *tpose2 = trgeo_in(tpose2_in);
  char *tpose2_out = tspatial_as_text(tpose2, 6);

  char *tpose1_3d_in = "SRID=5676;[Pose(Point(1 1 1), 0, 0, 0, 1)@2001-01-01, Pose(Point(3 3 3), 0.5, 0.5, 0.5, 0.5)@2001-01-03]";
  Temporal *tpose1_3d = trgeo_in(tpose1_3d_in);
  char *tpose1_3d_out = tspatial_as_text(tpose1_3d, 6);
  // char *tpose2_3d_in = "SRID=5676;[Pose(Point(3 3 3), 0.5, 0.5, 0.5, 0.5)@2001-01-01,
    // Pose(Point(1 1), 1, 1, 1, 1)@2001-01-03]";
  // Temporal *tpose2_3d = trgeo_in(tpose2_3d_in);
  // char *tpose2_3d_out = tspatial_as_text(tpose2_3d, 6);

  /* Create the result types for the functions of the API */

  bool bool_result;
  int32_t int32_result;
  uint32_t uint32_result;
  uint64_t uint64_result;
  double float8_result;
  double *float8array_result;
  char *char_result;
  size_t size;

  GSERIALIZED *geom_result;
  Set *geomset_result;
  Pose *rgeo_result;
  Set *poseset_result;
  STBox *stbox_result;
  Temporal *tbool_result;
  Temporal *tfloat_result;
  Temporal *tgeompt_result;
  Temporal *trgeo_result;
  Pose **posearray_result;

  /* Execute and print the result for the functions of the API */

  printf("****************************************************************\n");
  printf("* Rigid geometry types *\n");
  printf("****************************************************************\n");

  /*****************************************************************************
   * Input/output functions
   *****************************************************************************/
  printf("****************************************************************\n");

  /* uint8_t *trgeo_as_ewkb(const Temporal *temp, const char *endian, size_t *size); */
  binchar_result = trgeo_as_ewkb(trgeom1, "XDR", &size);
  printf("trgeo_as_ewkb(%s, \"XDR\", %ld): ", tfloat1_out, size);
  fwrite(binchar_result, size, 1, stdout);
  printf("\n");
  free(binchar_result);

  /* char *trgeo_as_ewkt(const Temporal *temp, int precision); */
  char_result = trgeo_as_ewkt(trgeom1, 0.1);
  printf("trgeo_as_ewkt(%s): %s\n", trgeom1_out, char_result);
  free(char_result);

  /* char *trgeo_as_mfjson(const Temporal *temp, int option, int precision, const char *srs); */
  char_result = trgeo_as_mfjson(trgeom1, 6, 5, "EPSG:4326");
  printf("trgeo_as_mfjson(%s): %s\n", trgeom1_out, char_result);
  free(char_result);

  /* char *trgeo_as_hexewkb(const Temporal *temp, const char *endian); */
  char_result = trgeo_as_hexewkb(trgeom1, "XDR");
  printf("trgeo_as_hexewkb(%s, \"XDR\"): %s\n", trgeom1_out, char_result);
  free(char_result);

  /* char *trgeo_as_text(const Temporal *temp, int precision); */
  char_result = trgeo_as_text(trgeom1, 0.1);
  printf("trgeo_as_text(%s): %s\n", trgeom1_out, char_result);
  free(char_result);

  /* GSERIALIZED *trgeo_from_ewkb(const uint8_t *wkb, size_t wkb_size, int32_t srid); */
  geom_result = trgeo_from_ewkb(trgeom1_wkb, trgeom_size_wkb, 5676);
  char_result = trgeo_as_ewkt(trgeom_result, 6);
  printf("trgeo_from_ewkb(");
  fwrite(trgeom1_wkb, trgeom_size_wkb, 1, stdout);
  printf(", %ld): %s\n", size, char_result);
  free(trgeom_result); free(char_result);

  /* GSERIALIZED *trgeo_from_mfjson(const char *mfjson); */
  geom_result = trgeo_from_mfjson(trgeom1_mfjson);
  char_result = trgeo_as_ewkt(trgeom_result, 6);
  printf("trgeo_from_mfjson(%s): %s\n", trgeom1_mfjson, char_result);
  free(trgeom_result); free(char_result);

  /* GSERIALIZED *trgeo_from_text(const char *wkt, int32_t srid); */
  geom_result = trgeo_from_text(point_wkt_in, 5676);
  char_result = trgeo_as_ewkt(trgeom_result, 6);
  printf("trgeo_from_text(%s): %s\n", point_wkt_in, char_result);
  free(trgeom_result); free(char_result);

  /* GSERIALIZED *trgeo_from_hexewkb(const char *wkt); */
  geom_result = trgeo_from_hexewkb(geog1_hexwkb);
  char_result = trgeo_as_ewkt(trgeom_result, 6);
  printf("trgeo_from_hexewkb(%s): %s\n", geog1_hexwkb, char_result);
  free(trgeom_result); free(char_result);

  /* GSERIALIZED *trgeo_in(const char *str, int32 typmod); */
  geog_result = trgeo_in(geog1_in, -1);
  char_result = trgeo_as_ewkt(geog_result, 6);
  printf("trgeo_in(%s): %s\n", geog1_in, char_result);
  free(geog_result); free(char_result);

  /* char *trgeo_out(const Temporal *temp); */
  char_result = trgeo_out(trgeom1);
  printf("trgeo_out(%s): %s\n", trgeom1_out, char_result);
  free(char_result);

  /*****************************************************************************
   * Constructor functions
   *****************************************************************************/
  printf("****************************************************************\n");

  /* Temporal *trgeo_make(const Temporal *tpoint, const Temporal *tfloat); */
  trgeo_result = trgeo_make(tgeompt1, tfloat1);
  char_result = tspatial_as_text(trgeo_result, 6);
  printf("trgeo_make(%s, %s): %s\n", tgeompt1_out, tfloat1_out, char_result);
  free(trgeo_result); free(char_result);

  /*****************************************************************************
   * Accessor functions
   *****************************************************************************/
  printf("****************************************************************\n");

  /* Set *trgeo_end_value(const Temporal *temp); */
  rgeo_result = trgeo_end_value(tpose1);
  char_result = rgeo_out(rgeo_result, 6);
  printf("trgeo_end_value(%s): %s", tpose1_out, char_result);
  free(rgeo_result); free(char_result);

  /* Set *trgeo_points(const Temporal *temp); */
  geomset_result = trgeo_points(tpose1);
  char_result = spatialset_as_text(geomset_result, 6);
  printf("trgeo_points(%s, 6): %s", tpose1_out, char_result);
  free(geomset_result); free(char_result);

  /* Set *trgeo_rotation(const Temporal *temp); */
  tfloat_result = trgeo_rotation(tpose1);
  char_result = tfloat_out(tfloat_result, 6);
  printf("trgeo_rotation(%s, 6): %s", tpose1_out, char_result);
  free(tfloat_result); free(char_result);

  /* Set *trgeo_start_value(const Temporal *temp); */
  rgeo_result = trgeo_start_value(tpose1);
  char_result = rgeo_out(rgeo_result, 6);
  printf("trgeo_start_value(%s, 6): %s", tpose1_out, char_result);
  free(rgeo_result); free(char_result);

  /* GSERIALIZED *trgeo_trajectory(const Temporal *temp, bool merge_union); */
  geom_result = trgeo_trajectory(tpose1);
  char_result = geo_as_text(geom_result, 6);
  printf("trgeo_trajectory(%s, true): %s\n", tpose1_out, char_result);
  free(geom_result); free(char_result);

  // /* GSERIALIZED *trgeo_value_at_timestamptz(const Temporal *temp, TimestampTz t, bool strict, Pose **value); */
  // bool_result = trgeo_value_at_timestamptz(tpose1, tstz1, true, &rgeo_result);
  // char_result = geo_as_text(geom_result, 6);
  // printf("trgeo_value_at_timestamptz(%s, %s, true, %s): %s\n", tpose1_out, tstz1_out, char_result,
    // bool_result ? "true" : "false");
  // free(char_result);

  /* GSERIALIZED *trgeo_value_n(const Temporal *temp, int n, Pose **result); */
  bool_result = trgeo_value_n(tpose1, 2, &rgeo_result);
  char_result = rgeo_as_text(rgeo_result, 6);
  printf("trgeo_value_n(%s, 2, %s): %s\n", tpose1_out, char_result,
    bool_result ? "true" : "false");
  free(rgeo_result); free(char_result);

  /* GSERIALIZED *trgeo_values(const Temporal *temp, int *count); */
  posearray_result = trgeo_values(tpose1, &int32_result);
  printf("trgeo_values({%s, %d}): {", tpose1_out, int32_result);
  for (int i = 0; i < 2; i++)
  {
    char_result = rgeo_as_text(posearray_result[i], 6);
    printf("%s", char_result);
    if (i < 1)
      printf(", ");
    else
      printf("}\n");
    free(posearray_result[i]);
    free(char_result);
  }
  free(posearray_result);

  /*****************************************************************************
   * Conversion functions
   *****************************************************************************/
  printf("****************************************************************\n");

  /* Temporal *trgeo_to_tpoint(const Temporal *temp); */
  tgeompt_result = trgeo_to_tpoint(tpose1);
  char_result = tspatial_as_text(tgeompt_result, 6);
  printf("trgeo_to_tpoint(%s, 6): %s\n", tpose1_out, char_result);
  free(tgeompt_result); free(char_result);

  /*****************************************************************************
   * Restriction functions
   *****************************************************************************/
  printf("****************************************************************\n");

  /* Temporal *trgeo_at_geom(const Temporal *temp, const GSERIALIZED *gs); */
  trgeo_result = trgeo_at_geom(tpose1, geom1, NULL);
  char_result = tspatial_as_text(trgeo_result, 6);
  printf("trgeo_at_geom(%s, %s, NULL): %s\n", tpose1_out, geom1_out, char_result);
  free(trgeo_result); free(char_result);

  /* Temporal *trgeo_at_geom(const Temporal *temp, const GSERIALIZED *gs); */
  Span *zspan = floatspan_in("[0,3]");
  trgeo_result = trgeo_at_geom(tpose1_3d, geom1, zspan);
  char_result = tspatial_as_text(trgeo_result, 6);
  printf("trgeo_at_geom(%s, %s, NULL): %s\n", tpose1_out, geom1_out, char_result);
  free(zspan); free(trgeo_result); free(char_result);

  /* Temporal *trgeo_at_pose(const Temporal *temp, const Pose *cb); */
  trgeo_result = trgeo_at_pose(tpose1, pose1);
  char_result = tspatial_as_text(trgeo_result, 6);
  printf("trgeo_at_pose(%s, %s): %s\n", tpose1_out, pose1_out, char_result);
  free(trgeo_result); free(char_result);

  /* Temporal *trgeo_at_stbox(const Temporal *temp, const STBox *box, bool border_inc); */
  trgeo_result = trgeo_at_stbox(tpose1, stbox1, true);
  char_result = tspatial_as_text(trgeo_result, 6);
  printf("trgeo_at_stbox(%s, %s, true): %s\n", tpose1_out, stbox1_out, char_result);
  free(trgeo_result); free(char_result);

  /* Temporal *trgeo_at_stbox(const Temporal *temp, const STBox *box, bool border_inc); */
  trgeo_result = trgeo_at_stbox(tpose1_3d, stbox1_3d, true);
  char_result = tspatial_as_text(trgeo_result, 6);
  printf("trgeo_at_stbox(%s, %s, true): %s\n", tpose1_3d_out, stbox1_3d_out, char_result);
  free(trgeo_result); free(char_result);

  /* Temporal *trgeo_minus_geom(const Temporal *temp, const GSERIALIZED *gs); */
  trgeo_result = trgeo_minus_geom(tpose1, geom1, NULL);
  char_result = trgeo_result ? tspatial_as_text(trgeo_result, 6) : text_out(text_null);
  printf("trgeo_minus_geom(%s, %s, NULL): %s\n", tpose1_out, geom1_out, char_result);
  if (trgeo_result)
    free(trgeo_result);
  free(char_result);

  /* Temporal *trgeo_minus_pose(const Temporal *temp, const Pose *cb); */
  trgeo_result = trgeo_minus_pose(tpose1, pose1);
  char_result = trgeo_result ? tspatial_as_text(trgeo_result, 6) : text_out(text_null);
  printf("trgeo_minus_pose(%s, %s): %s\n", tpose1_out, pose1_out, char_result);
  if (trgeo_result)
    free(trgeo_result);
  free(char_result);

  /* Temporal *trgeo_minus_stbox(const Temporal *temp, const STBox *box, bool border_inc); */
  trgeo_result = trgeo_minus_stbox(tpose1, stbox1, true);
  char_result = trgeo_result ? tspatial_as_text(trgeo_result, 6) : text_out(text_null);
  printf("trgeo_minus_stbox(%s, %s, true): %s\n", tpose1_out, stbox1_out, char_result);
  if (trgeo_result)
    free(trgeo_result);
  free(char_result);

  /*****************************************************************************
   * Distance functions
   *****************************************************************************/
  printf("****************************************************************\n");

  /* Temporal *tdistance_trgeo_geo(const Temporal *temp, const GSERIALIZED *gs); */
  tfloat_result = tdistance_trgeo_geo(tpose1, geom1);
  char_result = tfloat_out(tfloat_result, 6);
  printf("tdistance_trgeo_geo(%s, %s): %s\n", tpose1_out, geom1_out, char_result);
  free(tfloat_result); free(char_result);

  /* Temporal *tdistance_trgeo_pose(const Temporal *temp, const Pose *cb); */
  tfloat_result = tdistance_trgeo_pose(tpose1, pose1);
  char_result = tfloat_out(tfloat_result, 6);
  printf("tdistance_trgeo_pose(%s, %s): %s\n", tpose1_out, pose1_out, char_result);
  free(tfloat_result); free(char_result);

  /* Temporal *tdistance_trgeo_tpose(const Temporal *temp1, const Temporal *temp2); */
  tfloat_result = tdistance_trgeo_tpose(tpose1, tpose2);
  char_result = tfloat_out(tfloat_result, 6);
  printf("tdistance_trgeo_tpose(%s, %s): %s\n", tpose1_out, tpose2_out, char_result);
  free(tfloat_result); free(char_result);

  /* double nad_trgeo_geo(const Temporal *temp, const GSERIALIZED *gs); */
  float8_result = nad_trgeo_geo(tpose1, geom1);
  printf("nad_trgeo_geo(%s, %s): %lf\n", tpose1_out, geom1_out, float8_result);

  /* double nad_trgeo_pose(const Temporal *temp, const Pose *cb); */
  float8_result = nad_trgeo_pose(tpose1, pose1);
  printf("nad_trgeo_pose(%s, %s): %lf\n", tpose1_out, pose1_out, float8_result);

  /* double nad_trgeo_stbox(const Temporal *temp, const STBox *box); */
  float8_result = nad_trgeo_stbox(tpose1, stbox1);
  printf("nad_trgeo_stbox(%s, %s): %lf\n", tpose1_out, stbox1_out, float8_result);

  /* double nad_trgeo_tpose(const Temporal *temp1, const Temporal *temp2); */
  float8_result = nad_trgeo_tpose(tpose1, tpose2);
  printf("nad_trgeo_tpose(%s, %s): %lf\n", tpose1_out, tpose2_out, float8_result);

  /* TInstant *nai_trgeo_geo(const Temporal *temp, const GSERIALIZED *gs); */
  trgeo_result = (Temporal *) nai_trgeo_geo(tpose1, geom1);
  char_result = tspatial_as_text(trgeo_result, 6);
  printf("nai_trgeo_geo(%s, %s): %s\n", tpose1_out, geom1_out, char_result);
  free(trgeo_result); free(char_result);

  /* TInstant *nai_trgeo_pose(const Temporal *temp, const Pose *cb); */
  trgeo_result = (Temporal *) nai_trgeo_pose(tpose1, pose1);
  char_result = tspatial_as_text(trgeo_result, 6);
  printf("nai_trgeo_pose(%s, %s): %s\n", tpose1_out, pose1_out, char_result);
  free(trgeo_result); free(char_result);

  /* TInstant *nai_trgeo_tpose(const Temporal *temp1, const Temporal *temp2); */
  trgeo_result = (Temporal *) nai_trgeo_tpose(tpose1, tpose2);
  char_result = tspatial_as_text(trgeo_result, 6);
  printf("nai_trgeo_tpose(%s, %s): %s\n", tpose1_out, tpose2_out, char_result);
  free(trgeo_result); free(char_result);

  /* GSERIALIZED *shortestline_trgeo_geo(const Temporal *temp, const GSERIALIZED *gs); */
  geom_result = shortestline_trgeo_geo(tpose1, geom1);
  char_result = geo_as_text(geom_result, 6);
  printf("shortestline_trgeo_geo(%s, %s): %s\n", tpose1_out, geom1_out, char_result);
  free(geom_result); free(char_result);

  /* GSERIALIZED *shortestline_trgeo_pose(const Temporal *temp, const Pose *cb); */
  geom_result = shortestline_trgeo_pose(tpose1, pose1);
  char_result = geo_as_text(geom_result, 6);
  printf("shortestline_trgeo_pose(%s, %s): %s\n", tpose1_out, pose1_out, char_result);
  free(geom_result); free(char_result);

  /* GSERIALIZED *shortestline_trgeo_tpose(const Temporal *temp1, const Temporal *temp2); */
  geom_result = shortestline_trgeo_tpose(tpose1, tpose2);
  char_result = geo_as_text(geom_result, 6);
  printf("shortestline_trgeo_tpose(%s, %s): %s\n", tpose1_out, tpose2_out, char_result);
  free(geom_result); free(char_result);

  // /*****************************************************************************
   // * Comparison functions
   // *****************************************************************************/

  /* Ever/always comparison functions */
  printf("****************************************************************\n");

  /* int always_eq_rgeo_tpose(const Pose *cb, const Temporal *temp); */
  int32_result = always_eq_rgeo_tpose(pose1, tpose1);
  printf("always_eq_rgeo_tpose(%s, %s): %d\n", pose1_out, tpose1_out, int32_result);

  /* int always_eq_trgeo_pose(const Temporal *temp, const Pose *cb); */
  int32_result = always_eq_trgeo_pose(tpose1, pose1);
  printf("always_eq_trgeo_pose(%s, %s): %d\n", tpose1_out, pose1_out, int32_result);

  /* int always_eq_trgeo_tpose(const Temporal *temp1, const Temporal *temp2); */
  int32_result = always_eq_trgeo_tpose(tpose1, tpose2);
  printf("always_eq_trgeo_tpose(%s, %s): %d\n", tpose1_out, tpose2_out, int32_result);

  /* int always_ne_rgeo_tpose(const Pose *cb, const Temporal *temp); */
  int32_result = always_ne_rgeo_tpose(pose1, tpose1);
  printf("always_ne_rgeo_tpose(%s, %s): %d\n", pose1_out, tpose1_out, int32_result);

  /* int always_ne_trgeo_pose(const Temporal *temp, const Pose *cb); */
  int32_result = always_ne_trgeo_pose(tpose1, pose1);
  printf("always_ne_trgeo_pose(%s, %s): %d\n", tpose1_out, pose1_out, int32_result);

  /* int always_ne_trgeo_tpose(const Temporal *temp1, const Temporal *temp2); */
  int32_result = always_ne_trgeo_tpose(tpose1, tpose2);
  printf("always_ne_trgeo_tpose(%s, %s): %d\n", tpose1_out, tpose2_out, int32_result);

  /* int ever_eq_rgeo_tpose(const Pose *cb, const Temporal *temp); */
  int32_result = ever_eq_rgeo_tpose(pose1, tpose1);
  printf("ever_eq_rgeo_tpose(%s, %s): %d\n", pose1_out, tpose1_out, int32_result);

  /* int ever_eq_trgeo_pose(const Temporal *temp, const Pose *cb); */
  int32_result = ever_eq_trgeo_pose(tpose1, pose1);
  printf("ever_eq_trgeo_pose(%s, %s): %d\n", tpose1_out, pose1_out, int32_result);

  /* int ever_eq_trgeo_tpose(const Temporal *temp1, const Temporal *temp2); */
  int32_result = ever_eq_trgeo_tpose(tpose1, tpose2);
  printf("ever_eq_trgeo_tpose(%s, %s): %d\n", tpose1_out, tpose2_out, int32_result);

  /* int ever_ne_rgeo_tpose(const Pose *cb, const Temporal *temp); */
  int32_result = ever_ne_rgeo_tpose(pose1, tpose1);
  printf("ever_ne_rgeo_tpose(%s, %s): %d\n", pose1_out, tpose1_out, int32_result);

  /* int ever_ne_trgeo_pose(const Temporal *temp, const Pose *cb); */
  int32_result = ever_ne_trgeo_pose(tpose1, pose1);
  printf("ever_ne_trgeo_pose(%s, %s): %d\n", tpose1_out, pose1_out, int32_result);

  /* int ever_ne_trgeo_tpose(const Temporal *temp1, const Temporal *temp2); */
  int32_result = ever_ne_trgeo_tpose(tpose1, tpose2);
  printf("ever_ne_trgeo_tpose(%s, %s): %d\n", tpose1_out, tpose2_out, int32_result);

  /* Temporal comparison functions */
  printf("****************************************************************\n");

  /* Temporal *teq_rgeo_tpose(const Pose *cb, const Temporal *temp); */
  tbool_result = teq_rgeo_tpose(pose1, tpose1);
  char_result = tbool_out(tbool_result);
  printf("teq_rgeo_tpose(%s, %s): %s\n", pose1_out, tpose1_out, char_result);
  free(tbool_result); free(char_result);

  /* Temporal *teq_trgeo_pose(const Temporal *temp, const Pose *cb); */
  tbool_result = teq_trgeo_pose(tpose1, pose1);
  char_result = tbool_out(tbool_result);
  printf("teq_trgeo_pose(%s, %s): %s\n", tpose1_out, pose1_out, char_result);
  free(tbool_result); free(char_result);

  /* Temporal *tne_rgeo_tpose(const Pose *cb, const Temporal *temp); */
  tbool_result = tne_rgeo_tpose(pose1, tpose1);
  char_result = tbool_out(tbool_result);
  printf("tne_rgeo_tpose(%s, %s): %s\n", pose1_out, tpose1_out, char_result);
  free(tbool_result); free(char_result);

  /* Temporal *tne_trgeo_pose(const Temporal *temp, const Pose *cb); */
  tbool_result = tne_trgeo_pose(tpose1, pose1);
  char_result = tbool_out(tbool_result);
  printf("tne_trgeo_pose(%s, %s): %s\n", tpose1_out, pose1_out, char_result);
  free(tbool_result); free(char_result);

  printf("****************************************************************\n");

  /* Clean up */

  free(tstz1_out);
  free(tstzspan1); free(tstzspan1_out);
  free(text_null);
  free(pose1); free(pose1_out);
  free(pose2); free(pose2_out);
  free(pose1_3d); free(pose1_3d_out);
  free(pose2_3d); free(pose2_3d_out);
  free(poseset1);
  free(poseset1_out);
  free(geom1); free(geom1_out);
  free(tfloat1); free(tfloat1_out);
  free(stbox1); free(stbox1_out);
  free(tgeompt1); free(tgeompt1_out);
  free(tpose1); free(tpose1_out);
  free(tpose2); free(tpose2_out);

  /* Finalize MEOS */
  meos_finalize();

  return 0;
}
