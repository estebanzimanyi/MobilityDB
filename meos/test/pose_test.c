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
 * @brief A simple program that tests the funtions for the temporal pose
 * types in MEOS, that is, pose, poseset, and tpose.
 *
 * The program can be build as follows
 * @code
 * gcc -Wall -g -I/usr/local/include -o pose_test pose_test.c -L/usr/local/lib -lmeos
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
  Pose *pose1 = pose_in(pose1_in);
  char *pose1_out = pose_as_text(pose1, 6);
  char *pose2_in = "SRID=5676;Pose(Point(2 2), 1)";
  Pose *pose2 = pose_in(pose2_in);
  char *pose2_out = pose_as_text(pose2, 6);

  char *pose1_3d_in = "SRID=5676;Pose(Point(1 1 1), 1, 0, 0, 0)";
  Pose *pose1_3d = pose_in(pose1_3d_in);
  char *pose1_3d_out = pose_as_text(pose1_3d, 6);
  char *pose2_3d_in = "SRID=5676;Pose(Point(2 2 2), 0.5, 0.5, 0.5, 0.5)";
  Pose *pose2_3d = pose_in(pose2_3d_in);
  char *pose2_3d_out = pose_as_text(pose2_3d, 6);

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
  Temporal *tpose1 = tpose_in(tpose1_in);
  char *tpose1_out = tspatial_as_text(tpose1, 6);
  char *tpose2_in = "SRID=5676;[Pose(Point(3 3), 1)@2001-01-01, Pose(Point(1 1), 1)@2001-01-03]";
  Temporal *tpose2 = tpose_in(tpose2_in);
  char *tpose2_out = tspatial_as_text(tpose2, 6);

  char *tpose1_3d_in = "SRID=5676;[Pose(Point(1 1 1), 0, 0, 0, 1)@2001-01-01, Pose(Point(3 3 3), 0.5, 0.5, 0.5, 0.5)@2001-01-03]";
  Temporal *tpose1_3d = tpose_in(tpose1_3d_in);
  char *tpose1_3d_out = tspatial_as_text(tpose1_3d, 6);
  // char *tpose2_3d_in = "SRID=5676;[Pose(Point(3 3 3), 0.5, 0.5, 0.5, 0.5)@2001-01-01,
    // Pose(Point(1 1), 1, 1, 1, 1)@2001-01-03]";
  // Temporal *tpose2_3d = tpose_in(tpose2_3d_in);
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
  Pose *pose_result;
  Set *poseset_result;
  STBox *stbox_result;
  Temporal *tbool_result;
  Temporal *tfloat_result;
  Temporal *tgeompt_result;
  Temporal *tpose_result;
  Pose **posearray_result;

  /* Execute and print the result for the functions of the API */

  printf("****************************************************************\n");
  printf("* Pose types *\n");
  printf("****************************************************************\n");

  /* Input and output functions */

  /* char *pose_as_ewkt(const Pose *pose, int maxdd); */
  char_result = pose_as_ewkt(pose1, 6);
  printf("pose_as_ewkt(%s, 6): %s\n", pose1_out, char_result);
  free(char_result);

  /* char *pose_as_hexwkb(const Pose *pose, uint8_t variant, size_t *size); */
  char_result = pose_as_hexwkb(pose1, 1, &size);
  printf("pose_as_hexwkb(%s, 1, %ld): %s\n", pose1_out, size, char_result);
  free(char_result);

  /* char *pose_as_text(const Pose *pose, int maxdd); */
  char_result = pose_as_text(pose1, 6);
  printf("pose_as_text(%s, 6): %s\n", pose1_out, char_result);
  free(char_result);

  // /* uint8_t *pose_as_wkb(const Pose *pose, uint8_t variant, size_t *size_out); */
  // binchar_result = pose_as_wkb(const Pose *pose, uint8_t variant, size_t *size_out);

  // /* Pose *pose_from_hexwkb(const char *hexwkb); */
  // pose_result = pose_from_hexwkb(const char *hexwkb);
  // char_result = pose_as_text(pose_result, 6);
  // printf("pose_from_hexwkb(%s): %s\n", xxx, char_result);
  // free(pose_result); free(char_result);

  // /* Pose *pose_from_wkb(const uint8_t *wkb, size_t size); */
  // pose_result = pose_from_wkb(const uint8_t *wkb, size);
  // char_result = pose_as_text(pose_result, 6);
  // printf("pose_from_wkb(%s): %s\n", xxx, char_result);
  // free(pose_result); free(char_result);

  /* Pose *pose_in(const char *str); */
  pose_result = pose_in(pose1_in);
  char_result = pose_as_text(pose_result, 6);
  printf("pose_in(%s): %s\n", pose1_in, char_result);
  free(pose_result); free(char_result);

  /* char *pose_out(const Pose *pose, int maxdd); */
  char_result = pose_out(pose1, 6);
  printf("pose_out(%s, 6): %s\n", pose1_out, char_result);
  free(char_result);

  /* Constructor functions */
  printf("****************************************************************\n");

  /* Pose *pose_copy(const Pose *pose); */
  pose_result = pose_copy(pose1);
  char_result = pose_as_text(pose_result, 6);
  printf("pose_copy(%s): %s\n", pose1_out, char_result);
  free(pose_result); free(char_result);

  /* Pose *pose_make_2d(double x, double y, double theta, int32_t srid, bool geodetic); */
  pose_result = pose_make_2d(x1, y1, theta1, 5676, false);
  char_result = pose_as_text(pose_result, 6);
  printf("pose_make_2d(%lf, %lf, %lf, 5676, false): %s\n", x1, y1, theta1, char_result);
  free(pose_result); free(char_result);

  /* Pose *pose_make_3d(double x, double y, double z, double W, double X, double Y, double Z, int32_t srid, bool geodetic); */
  pose_result = pose_make_3d(x1, y1, z1, X1, Y1, Z1, W1, 5676, false);
  char_result = pose_as_text(pose_result, 6);
  printf("pose_make_3d(%lf, %lf, %lf, %lf, %lf, %lf, %lf, 5676, false): %s\n", x1, y1, z1, X1, Y1, Z1, W1, char_result);
  free(pose_result); free(char_result);

  /* Conversion functions */
  printf("****************************************************************\n");

  /* GSERIALIZED *pose_to_point(const Pose *pose); */
  geom_result = pose_to_point(pose1);
  char_result = geo_as_text(geom_result, 6);
  printf("pose_to_point(%s): %s\n", pose1_out, char_result);
  free(geom_result); free(char_result);

  /* STBox *pose_to_stbox(const Pose *pose); */
  stbox_result = pose_to_stbox(pose1);
  char_result = stbox_out(stbox_result, 6);
  printf("pose_to_stbox(%s): %s\n", pose1_out, char_result);
  free(stbox_result); free(char_result);

  /* Accessor functions */
  printf("****************************************************************\n");

  /* uint32 pose_hash(const Pose *pose); */
  uint32_result = pose_hash(pose1);
  printf("pose_hash(%s, true): %u\n", pose1_out, uint32_result);

  /* uint64 pose_hash_extended(const Pose *pose, uint64 seed); */
  uint64_result = pose_hash_extended(pose1, 1);
  printf("pose_hash_extended(%s, 1): %lu\n", pose1_out, uint64_result);

  /* GSERIALIZED *pose_orientation(const Pose *pose); */
  float8array_result = pose_orientation(pose1_3d);
  printf("pose_orientation(%s): (%lf, %lf, %lf, %lf)\n", pose1_3d_out,
    float8array_result[0], float8array_result[1], float8array_result[2], float8array_result[3]);

  /* double pose_rotation(const Pose *pose); */
  float8_result = pose_rotation(pose1);
  printf("pose_rotation(%s, true): %lf\n", pose1_out, float8_result);

  /* Transformation functions */
  printf("****************************************************************\n");

  /* Pose *pose_round(const Pose *pose, int maxdd); */
  pose_result = pose_round(pose1, 6);
  char_result = pose_as_text(pose_result, 6);
  printf("pose_round(%s): %s\n", pose1_out, char_result);
  free(pose_result); free(char_result);

  /* Pose **posearr_round(Pose **cbarr, int count, int maxdd); */
  posearray[0] = pose1;
  posearray[1] = pose2;
  posearray_result = posearr_round(posearray, 2, 6);
  printf("posearr_round({%s, %s}): {", pose1_out, pose2_out);
  for (int i = 0; i < poseset1->count; i++)
  {
    char_result = pose_as_text(posearray_result[i], 6);
    printf("%s", char_result);
    if (i < poseset1->count - 1)
      printf(", ");
    else
      printf("}\n");
    free(posearray_result[i]);
    free(char_result);
  }
  free(posearray_result);

  /* Spatial reference system functions */
  printf("****************************************************************\n");

  /* void pose_set_srid(Pose *pose, int32_t srid); */
  Pose *p = pose_make_2d(x2, y2, theta2, 0, false);
  char *p_out = pose_as_ewkt(p, 6);
  pose_set_srid(p, 5676);
  char_result = pose_as_text(pose_result, 6);
  printf("pose_set_srid(%s, 5676): %s\n", p_out, char_result);
  free(p); free(p_out); free(pose_result); free(char_result);

  /* void pose_set_srid(Pose *pose, int32_t srid); */
  p = pose_make_3d(x2, y2, z2, X2, Y2, Z2, W2, 0, false);
  p_out = pose_as_ewkt(p, 6);
  pose_set_srid(p, 5676);
  char_result = pose_as_text(pose_result, 6);
  printf("pose_set_srid(%s, 5676): %s\n", p_out, char_result);
  free(p); free(p_out); free(char_result);

  /* int32_t pose_srid(const Pose *pose); */
  int32_result = pose_srid(pose1);
  printf("pose_srid(%s, true): %d\n", pose1_out, int32_result);

  /* Pose *pose_transform(const Pose *pose, int32_t srid); */
  pose_result = pose_transform(pose1, 3857);
  char_result = pose_as_text(pose_result, 6);
  printf("pose_transform(%s): %s\n", pose1_out, char_result);
  free(pose_result); free(char_result);

  /* Pose *pose_transform_pipeline(const Pose *pose, const char *pipelinestr, int32_t srid, bool is_forward); */
  pose_result = pose_transform_pipeline(pose1, "urn:ogc:def:coordinateOperation:EPSG::16031", 16031, true);
  char_result = pose_as_text(pose_result, 6);
  printf("pose_transform_pipeline(%s, \"urn:ogc:def:coordinateOperation:EPSG::16031\", 16031, true): %s\n", pose1_out, char_result);
  free(pose_result); free(char_result);

  /* Bounding box functions */
  printf("****************************************************************\n");

  /* STBox *pose_tstzspan_to_stbox(const Pose *pose, const Span *s); */
  stbox_result = pose_tstzspan_to_stbox(pose1, tstzspan1);
  char_result = stbox_out(stbox_result, 6);
  printf("pose_tstzspan_to_stbox(%s, %s): %s\n", pose1_out, tstzspan1_out, char_result);
  free(stbox_result); free(char_result);

  /* STBox *pose_timestamptz_to_stbox(const Pose *pose, TimestampTz t); */
  stbox_result = pose_timestamptz_to_stbox(pose1, tstz1);
  char_result = stbox_out(stbox_result, 6);
  printf("pose_timestamptz_to_stbox(%s, %s): %s\n", pose1_out, tstz1_out, char_result);
  free(stbox_result); free(char_result);

  /* Distance functions */
  printf("****************************************************************\n");

  /* double distance_pose_geo(const Pose *pose, const GSERIALIZED *gs); */
  float8_result = distance_pose_geo(pose1, geom1);
  printf("distance_pose_geo(%s, %s): %lf\n", pose1_out, geom1_out, float8_result);

  /* double distance_pose_pose(const Pose *pose1, const Pose *pose2); */
  float8_result = distance_pose_pose(pose1, pose2);
  printf("distance_pose_pose(%s, %s): %lf\n", pose1_out, pose2_out, float8_result);

  /* double distance_pose_stbox(const Pose *cb, const STBox *box); */
  float8_result = distance_pose_stbox(pose1, stbox1);
  printf("distance_pose_stbox(%s, %s): %lf\n", pose1_out, stbox1_out, float8_result);

  /* Comparison functions */
  printf("****************************************************************\n");

  /* int pose_cmp(const Pose *pose1, const Pose *pose2); */
  int32_result = pose_cmp(pose1, pose2);
  printf("pose_cmp(%s, %s): %d\n", pose1_out, pose2_out, int32_result);

  /* bool pose_eq(const Pose *pose1, const Pose *pose2); */
  bool_result = pose_eq(pose1, pose2);
  printf("pose_eq(%s, %s): %c\n", pose1_out, pose2_out, bool_result ? 't' : 'n');

  /* bool pose_ge(const Pose *pose1, const Pose *pose2); */
  bool_result = pose_ge(pose1, pose2);
  printf("pose_ge(%s, %s): %c\n", pose1_out, pose2_out, bool_result ? 't' : 'n');

  /* bool pose_gt(const Pose *pose1, const Pose *pose2); */
  bool_result = pose_gt(pose1, pose2);
  printf("pose_gt(%s, %s): %c\n", pose1_out, pose2_out, bool_result ? 't' : 'n');

  /* bool pose_le(const Pose *pose1, const Pose *pose2); */
  bool_result = pose_le(pose1, pose2);
  printf("pose_le(%s, %s): %c\n", pose1_out, pose2_out, bool_result ? 't' : 'n');

  /* bool pose_lt(const Pose *pose1, const Pose *pose2); */
  bool_result = pose_lt(pose1, pose2);
  printf("pose_lt(%s, %s): %c\n", pose1_out, pose2_out, bool_result ? 't' : 'n');

  /* bool pose_ne(const Pose *pose1, const Pose *pose2); */
  bool_result = pose_ne(pose1, pose2);
  printf("pose_ne(%s, %s): %c\n", pose1_out, pose2_out, bool_result ? 't' : 'n');

  /* bool pose_nsame(const Pose *pose1, const Pose *pose2); */
  bool_result = pose_nsame(pose1, pose2);
  printf("pose_nsame(%s, %s): %c\n", pose1_out, pose2_out, bool_result ? 't' : 'n');

  /* bool pose_same(const Pose *pose1, const Pose *pose2); */
  bool_result = pose_same(pose1, pose2);
  printf("pose_same(%s, %s): %c\n", pose1_out, pose2_out, bool_result ? 't' : 'n');

  /******************************************************************************
   * Functions for pose sets
   ******************************************************************************/

  /* Input and output functions */
  printf("****************************************************************\n");

  /* Set *poseset_in(const char *str); */
  poseset_result = poseset_in(poseset1_in);
  char_result = spatialset_as_text(poseset_result, 6);
  printf("poseset_in(%s): %s\n", poseset1_in, char_result);
  free(poseset_result); free(char_result);

  /* char *spatialset_as_text(const Set *s, int maxdd); */
  char_result = spatialset_as_text(poseset1, 6);
  printf("spatialset_as_text(%s, 6): %s\n", poseset1_out, char_result);
  free(char_result);

  /* Constructor functions */
  printf("****************************************************************\n");

  /* Set *poseset_make(Pose **values, int count); */
  posearray[0] = pose1;
  posearray[1] = pose2;
  poseset_result = poseset_make(posearray, 2);
  char_result = spatialset_as_text(poseset_result, 6);
  printf("poseset_make({%s, %s}): %s\n", pose1_out, pose2_out, char_result);
  free(poseset_result); free(char_result);

  /* Conversion functions */
  printf("****************************************************************\n");

  /* Set *pose_to_set(const Pose *cb); */
  poseset_result = pose_to_set(pose1);
  char_result = spatialset_as_text(poseset_result, 6);
  printf("pose_to_set(%s): %s\n", pose1_out, char_result);
  free(poseset_result); free(char_result);

  /* Accessor functions */
  printf("****************************************************************\n");

  /* Pose *poseset_end_value(const Set *s); */
  pose_result = poseset_end_value(poseset1);
  char_result = pose_as_text(pose_result, 6);
  printf("poseset_end_value(%s): %s\n", poseset1_out, char_result);
  free(pose_result); free(char_result);

  /* Pose *poseset_start_value(const Set *s); */
  pose_result = poseset_start_value(poseset1);
  char_result = pose_as_text(pose_result, 6);
  printf("poseset_start_value(%s): %s\n", poseset1_out, char_result);
  free(pose_result); free(char_result);

  /* bool poseset_value_n(const Set *s, int n, Pose **result); */
  bool_result = poseset_value_n(poseset1, 1, &pose_result);
  char_result = pose_as_text(pose_result, 6);
  printf("poseset_value_n(%s, 1, %s): %c\n", poseset1_out, pose1_out, bool_result ? 't' : 'n');
  free(pose_result); free(char_result);

  /* Pose **poseset_values(const Set *s); */
  posearray_result = poseset_values(poseset1);
  printf("poseset_values(%s): {", poseset1_out);
  for (int i = 0; i < poseset1->count; i++)
  {
    char_result = pose_as_text(posearray_result[i], 6);
    printf("%s", char_result);
    if (i < poseset1->count - 1)
      printf(", ");
    else
      printf("}\n");
    free(posearray_result[i]);
    free(char_result);
  }
  free(posearray_result);

  /* Set operations */
  printf("****************************************************************\n");

/* bool contained_pose_set(const Pose *cb, const Set *s); */
  bool_result = contained_pose_set(pose1, poseset1);
  printf("contained_pose_set(%s, %s): %c\n", pose1_out, poseset1_out, bool_result ? 't' : 'n');

  /* bool contains_set_pose(const Set *s, Pose *cb); */
  bool_result = contains_set_pose(poseset1, pose1);
  printf("contains_set_pose(%s, %s): %c\n", poseset1_out, pose1_out, bool_result ? 't' : 'n');

  /* Set *intersection_pose_set(const Pose *cb, const Set *s); */
  poseset_result = intersection_pose_set(pose1, poseset1);
  char_result = spatialset_as_text(poseset_result, 6);
  printf("intersection_pose_set(%s, %s): %s\n", pose1_out, poseset1_out, char_result);
  free(poseset_result); free(char_result);

  /* Set *intersection_set_pose(const Set *s, const Pose *cb); */
  poseset_result = intersection_set_pose(poseset1, pose1);
  char_result = spatialset_as_text(poseset_result, 6);
  printf("intersection_set_pose(%s, %s): %s\n", pose1_out, poseset1_out, char_result);
  free(poseset_result); free(char_result);

  /* Set *minus_pose_set(const Pose *cb, const Set *s); */
  poseset_result = minus_pose_set(pose1, poseset1);
  char_result = poseset_result ? 
    spatialset_as_text(poseset_result, 6) : text_out(text_null);
  printf("minus_pose_set(%s, %s): %s\n", pose1_out, poseset1_out, char_result);
  free(poseset_result); free(char_result);

  /* Set *minus_set_pose(const Set *s, const Pose *cb); */
  poseset_result = minus_set_pose(poseset1, pose1);
  char_result = poseset_result ? 
    spatialset_as_text(poseset_result, 6) : text_out(text_null);
  printf("minus_set_pose(%s, %s): %s\n", poseset1_out, pose1_out, char_result);
  free(poseset_result); free(char_result);

  // /* Set *pose_union_transfn(Set *state, const Pose *cb); */
  // poseset_result = pose_union_transfn(poseset1, pose1);
  // char_result = spatialset_as_text(poseset_result, 6);
  // printf("pose_union(%s, %s): %s\n", pose_union_transfn, char_result);
  // free(poseset_result); free(char_result);

  /* Set *union_pose_set(const Pose *cb, const Set *s); */
  poseset_result = union_pose_set(pose1, poseset1);
  char_result = spatialset_as_text(poseset_result, 6);
  printf("union_pose_set(%s, %s): %s\n", pose1_out, poseset1_out, char_result);
  free(poseset_result); free(char_result);

  /* Set *union_set_pose(const Set *s, const Pose *cb); */
  poseset_result = union_set_pose(poseset1, pose1);
  char_result = spatialset_as_text(poseset_result, 6);
  printf("union_set_pose(%s, %s): %s\n", poseset1_out, pose1_out, char_result);
  free(poseset_result); free(char_result);

  /*===========================================================================*
   * Functions for the temporal pose type
   *===========================================================================*/

  /*****************************************************************************
   * Input/output functions
   *****************************************************************************/
  printf("****************************************************************\n");

  /* Temporal *tpose_in(const char *str); */
  tpose_result = tpose_in(tpose1_in);
  char_result = tspatial_as_text(tpose_result, 6);
  printf("tpose_in(%s): %s\n", tpose1_out, char_result);
  free(tpose_result); free(char_result);

  /*****************************************************************************
   * Constructor functions
   *****************************************************************************/
  printf("****************************************************************\n");

  /* Temporal *tpose_make(const Temporal *tpoint, const Temporal *tfloat); */
  tpose_result = tpose_make(tgeompt1, tfloat1);
  char_result = tspatial_as_text(tpose_result, 6);
  printf("tpose_make(%s, %s): %s\n", tgeompt1_out, tfloat1_out, char_result);
  free(tpose_result); free(char_result);

  /*****************************************************************************
   * Accessor functions
   *****************************************************************************/
  printf("****************************************************************\n");

  /* Set *tpose_end_value(const Temporal *temp); */
  pose_result = tpose_end_value(tpose1);
  char_result = pose_out(pose_result, 6);
  printf("tpose_end_value(%s): %s", tpose1_out, char_result);
  free(pose_result); free(char_result);

  /* Set *tpose_points(const Temporal *temp); */
  geomset_result = tpose_points(tpose1);
  char_result = spatialset_as_text(geomset_result, 6);
  printf("tpose_points(%s, 6): %s", tpose1_out, char_result);
  free(geomset_result); free(char_result);

  /* Set *tpose_rotation(const Temporal *temp); */
  tfloat_result = tpose_rotation(tpose1);
  char_result = tfloat_out(tfloat_result, 6);
  printf("tpose_rotation(%s, 6): %s", tpose1_out, char_result);
  free(tfloat_result); free(char_result);

  /* Set *tpose_start_value(const Temporal *temp); */
  pose_result = tpose_start_value(tpose1);
  char_result = pose_out(pose_result, 6);
  printf("tpose_start_value(%s, 6): %s", tpose1_out, char_result);
  free(pose_result); free(char_result);

  /* GSERIALIZED *tpose_trajectory(const Temporal *temp, bool merge_union); */
  geom_result = tpose_trajectory(tpose1);
  char_result = geo_as_text(geom_result, 6);
  printf("tpose_trajectory(%s, true): %s\n", tpose1_out, char_result);
  free(geom_result); free(char_result);

  // /* GSERIALIZED *tpose_value_at_timestamptz(const Temporal *temp, TimestampTz t, bool strict, Pose **value); */
  // bool_result = tpose_value_at_timestamptz(tpose1, tstz1, true, &pose_result);
  // char_result = geo_as_text(geom_result, 6);
  // printf("tpose_value_at_timestamptz(%s, %s, true, %s): %s\n", tpose1_out, tstz1_out, char_result,
    // bool_result ? "true" : "false");
  // free(char_result);

  /* GSERIALIZED *tpose_value_n(const Temporal *temp, int n, Pose **result); */
  bool_result = tpose_value_n(tpose1, 2, &pose_result);
  char_result = pose_as_text(pose_result, 6);
  printf("tpose_value_n(%s, 2, %s): %s\n", tpose1_out, char_result,
    bool_result ? "true" : "false");
  free(pose_result); free(char_result);

  /* GSERIALIZED *tpose_values(const Temporal *temp, int *count); */
  posearray_result = tpose_values(tpose1, &int32_result);
  printf("tpose_values({%s, %d}): {", tpose1_out, int32_result);
  for (int i = 0; i < 2; i++)
  {
    char_result = pose_as_text(posearray_result[i], 6);
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

  /* Temporal *tpose_to_tpoint(const Temporal *temp); */
  tgeompt_result = tpose_to_tpoint(tpose1);
  char_result = tspatial_as_text(tgeompt_result, 6);
  printf("tpose_to_tpoint(%s, 6): %s\n", tpose1_out, char_result);
  free(tgeompt_result); free(char_result);

  /*****************************************************************************
   * Restriction functions
   *****************************************************************************/
  printf("****************************************************************\n");

  /* Temporal *tpose_at_geom(const Temporal *temp, const GSERIALIZED *gs); */
  tpose_result = tpose_at_geom(tpose1, geom1, NULL);
  char_result = tspatial_as_text(tpose_result, 6);
  printf("tpose_at_geom(%s, %s, NULL): %s\n", tpose1_out, geom1_out, char_result);
  free(tpose_result); free(char_result);

  /* Temporal *tpose_at_geom(const Temporal *temp, const GSERIALIZED *gs); */
  Span *zspan = floatspan_in("[0,3]");
  tpose_result = tpose_at_geom(tpose1_3d, geom1, zspan);
  char_result = tspatial_as_text(tpose_result, 6);
  printf("tpose_at_geom(%s, %s, NULL): %s\n", tpose1_out, geom1_out, char_result);
  free(zspan); free(tpose_result); free(char_result);

  /* Temporal *tpose_at_pose(const Temporal *temp, const Pose *cb); */
  tpose_result = tpose_at_pose(tpose1, pose1);
  char_result = tspatial_as_text(tpose_result, 6);
  printf("tpose_at_pose(%s, %s): %s\n", tpose1_out, pose1_out, char_result);
  free(tpose_result); free(char_result);

  /* Temporal *tpose_at_stbox(const Temporal *temp, const STBox *box, bool border_inc); */
  tpose_result = tpose_at_stbox(tpose1, stbox1, true);
  char_result = tspatial_as_text(tpose_result, 6);
  printf("tpose_at_stbox(%s, %s, true): %s\n", tpose1_out, stbox1_out, char_result);
  free(tpose_result); free(char_result);

  /* Temporal *tpose_at_stbox(const Temporal *temp, const STBox *box, bool border_inc); */
  tpose_result = tpose_at_stbox(tpose1_3d, stbox1_3d, true);
  char_result = tspatial_as_text(tpose_result, 6);
  printf("tpose_at_stbox(%s, %s, true): %s\n", tpose1_3d_out, stbox1_3d_out, char_result);
  free(tpose_result); free(char_result);

  /* Temporal *tpose_minus_geom(const Temporal *temp, const GSERIALIZED *gs); */
  tpose_result = tpose_minus_geom(tpose1, geom1, NULL);
  char_result = tpose_result ? tspatial_as_text(tpose_result, 6) : text_out(text_null);
  printf("tpose_minus_geom(%s, %s, NULL): %s\n", tpose1_out, geom1_out, char_result);
  if (tpose_result)
    free(tpose_result);
  free(char_result);

  /* Temporal *tpose_minus_pose(const Temporal *temp, const Pose *cb); */
  tpose_result = tpose_minus_pose(tpose1, pose1);
  char_result = tpose_result ? tspatial_as_text(tpose_result, 6) : text_out(text_null);
  printf("tpose_minus_pose(%s, %s): %s\n", tpose1_out, pose1_out, char_result);
  if (tpose_result)
    free(tpose_result);
  free(char_result);

  /* Temporal *tpose_minus_stbox(const Temporal *temp, const STBox *box, bool border_inc); */
  tpose_result = tpose_minus_stbox(tpose1, stbox1, true);
  char_result = tpose_result ? tspatial_as_text(tpose_result, 6) : text_out(text_null);
  printf("tpose_minus_stbox(%s, %s, true): %s\n", tpose1_out, stbox1_out, char_result);
  if (tpose_result)
    free(tpose_result);
  free(char_result);

  /*****************************************************************************
   * Distance functions
   *****************************************************************************/
  printf("****************************************************************\n");

  /* Temporal *tdistance_tpose_geo(const Temporal *temp, const GSERIALIZED *gs); */
  tfloat_result = tdistance_tpose_geo(tpose1, geom1);
  char_result = tfloat_out(tfloat_result, 6);
  printf("tdistance_tpose_geo(%s, %s): %s\n", tpose1_out, geom1_out, char_result);
  free(tfloat_result); free(char_result);

  /* Temporal *tdistance_tpose_pose(const Temporal *temp, const Pose *cb); */
  tfloat_result = tdistance_tpose_pose(tpose1, pose1);
  char_result = tfloat_out(tfloat_result, 6);
  printf("tdistance_tpose_pose(%s, %s): %s\n", tpose1_out, pose1_out, char_result);
  free(tfloat_result); free(char_result);

  /* Temporal *tdistance_tpose_tpose(const Temporal *temp1, const Temporal *temp2); */
  tfloat_result = tdistance_tpose_tpose(tpose1, tpose2);
  char_result = tfloat_out(tfloat_result, 6);
  printf("tdistance_tpose_tpose(%s, %s): %s\n", tpose1_out, tpose2_out, char_result);
  free(tfloat_result); free(char_result);

  /* double nad_tpose_geo(const Temporal *temp, const GSERIALIZED *gs); */
  float8_result = nad_tpose_geo(tpose1, geom1);
  printf("nad_tpose_geo(%s, %s): %lf\n", tpose1_out, geom1_out, float8_result);

  /* double nad_tpose_pose(const Temporal *temp, const Pose *cb); */
  float8_result = nad_tpose_pose(tpose1, pose1);
  printf("nad_tpose_pose(%s, %s): %lf\n", tpose1_out, pose1_out, float8_result);

  /* double nad_tpose_stbox(const Temporal *temp, const STBox *box); */
  float8_result = nad_tpose_stbox(tpose1, stbox1);
  printf("nad_tpose_stbox(%s, %s): %lf\n", tpose1_out, stbox1_out, float8_result);

  /* double nad_tpose_tpose(const Temporal *temp1, const Temporal *temp2); */
  float8_result = nad_tpose_tpose(tpose1, tpose2);
  printf("nad_tpose_tpose(%s, %s): %lf\n", tpose1_out, tpose2_out, float8_result);

  /* TInstant *nai_tpose_geo(const Temporal *temp, const GSERIALIZED *gs); */
  tpose_result = (Temporal *) nai_tpose_geo(tpose1, geom1);
  char_result = tspatial_as_text(tpose_result, 6);
  printf("nai_tpose_geo(%s, %s): %s\n", tpose1_out, geom1_out, char_result);
  free(tpose_result); free(char_result);

  /* TInstant *nai_tpose_pose(const Temporal *temp, const Pose *cb); */
  tpose_result = (Temporal *) nai_tpose_pose(tpose1, pose1);
  char_result = tspatial_as_text(tpose_result, 6);
  printf("nai_tpose_pose(%s, %s): %s\n", tpose1_out, pose1_out, char_result);
  free(tpose_result); free(char_result);

  /* TInstant *nai_tpose_tpose(const Temporal *temp1, const Temporal *temp2); */
  tpose_result = (Temporal *) nai_tpose_tpose(tpose1, tpose2);
  char_result = tspatial_as_text(tpose_result, 6);
  printf("nai_tpose_tpose(%s, %s): %s\n", tpose1_out, tpose2_out, char_result);
  free(tpose_result); free(char_result);

  /* GSERIALIZED *shortestline_tpose_geo(const Temporal *temp, const GSERIALIZED *gs); */
  geom_result = shortestline_tpose_geo(tpose1, geom1);
  char_result = geo_as_text(geom_result, 6);
  printf("shortestline_tpose_geo(%s, %s): %s\n", tpose1_out, geom1_out, char_result);
  free(geom_result); free(char_result);

  /* GSERIALIZED *shortestline_tpose_pose(const Temporal *temp, const Pose *cb); */
  geom_result = shortestline_tpose_pose(tpose1, pose1);
  char_result = geo_as_text(geom_result, 6);
  printf("shortestline_tpose_pose(%s, %s): %s\n", tpose1_out, pose1_out, char_result);
  free(geom_result); free(char_result);

  /* GSERIALIZED *shortestline_tpose_tpose(const Temporal *temp1, const Temporal *temp2); */
  geom_result = shortestline_tpose_tpose(tpose1, tpose2);
  char_result = geo_as_text(geom_result, 6);
  printf("shortestline_tpose_tpose(%s, %s): %s\n", tpose1_out, tpose2_out, char_result);
  free(geom_result); free(char_result);

  // /*****************************************************************************
   // * Comparison functions
   // *****************************************************************************/

  /* Ever/always comparison functions */
  printf("****************************************************************\n");

  /* int always_eq_pose_tpose(const Pose *cb, const Temporal *temp); */
  int32_result = always_eq_pose_tpose(pose1, tpose1);
  printf("always_eq_pose_tpose(%s, %s): %d\n", pose1_out, tpose1_out, int32_result);

  /* int always_eq_tpose_pose(const Temporal *temp, const Pose *cb); */
  int32_result = always_eq_tpose_pose(tpose1, pose1);
  printf("always_eq_tpose_pose(%s, %s): %d\n", tpose1_out, pose1_out, int32_result);

  /* int always_eq_tpose_tpose(const Temporal *temp1, const Temporal *temp2); */
  int32_result = always_eq_tpose_tpose(tpose1, tpose2);
  printf("always_eq_tpose_tpose(%s, %s): %d\n", tpose1_out, tpose2_out, int32_result);

  /* int always_ne_pose_tpose(const Pose *cb, const Temporal *temp); */
  int32_result = always_ne_pose_tpose(pose1, tpose1);
  printf("always_ne_pose_tpose(%s, %s): %d\n", pose1_out, tpose1_out, int32_result);

  /* int always_ne_tpose_pose(const Temporal *temp, const Pose *cb); */
  int32_result = always_ne_tpose_pose(tpose1, pose1);
  printf("always_ne_tpose_pose(%s, %s): %d\n", tpose1_out, pose1_out, int32_result);

  /* int always_ne_tpose_tpose(const Temporal *temp1, const Temporal *temp2); */
  int32_result = always_ne_tpose_tpose(tpose1, tpose2);
  printf("always_ne_tpose_tpose(%s, %s): %d\n", tpose1_out, tpose2_out, int32_result);

  /* int ever_eq_pose_tpose(const Pose *cb, const Temporal *temp); */
  int32_result = ever_eq_pose_tpose(pose1, tpose1);
  printf("ever_eq_pose_tpose(%s, %s): %d\n", pose1_out, tpose1_out, int32_result);

  /* int ever_eq_tpose_pose(const Temporal *temp, const Pose *cb); */
  int32_result = ever_eq_tpose_pose(tpose1, pose1);
  printf("ever_eq_tpose_pose(%s, %s): %d\n", tpose1_out, pose1_out, int32_result);

  /* int ever_eq_tpose_tpose(const Temporal *temp1, const Temporal *temp2); */
  int32_result = ever_eq_tpose_tpose(tpose1, tpose2);
  printf("ever_eq_tpose_tpose(%s, %s): %d\n", tpose1_out, tpose2_out, int32_result);

  /* int ever_ne_pose_tpose(const Pose *cb, const Temporal *temp); */
  int32_result = ever_ne_pose_tpose(pose1, tpose1);
  printf("ever_ne_pose_tpose(%s, %s): %d\n", pose1_out, tpose1_out, int32_result);

  /* int ever_ne_tpose_pose(const Temporal *temp, const Pose *cb); */
  int32_result = ever_ne_tpose_pose(tpose1, pose1);
  printf("ever_ne_tpose_pose(%s, %s): %d\n", tpose1_out, pose1_out, int32_result);

  /* int ever_ne_tpose_tpose(const Temporal *temp1, const Temporal *temp2); */
  int32_result = ever_ne_tpose_tpose(tpose1, tpose2);
  printf("ever_ne_tpose_tpose(%s, %s): %d\n", tpose1_out, tpose2_out, int32_result);

  /* Temporal comparison functions */
  printf("****************************************************************\n");

  /* Temporal *teq_pose_tpose(const Pose *cb, const Temporal *temp); */
  tbool_result = teq_pose_tpose(pose1, tpose1);
  char_result = tbool_out(tbool_result);
  printf("teq_pose_tpose(%s, %s): %s\n", pose1_out, tpose1_out, char_result);
  free(tbool_result); free(char_result);

  /* Temporal *teq_tpose_pose(const Temporal *temp, const Pose *cb); */
  tbool_result = teq_tpose_pose(tpose1, pose1);
  char_result = tbool_out(tbool_result);
  printf("teq_tpose_pose(%s, %s): %s\n", tpose1_out, pose1_out, char_result);
  free(tbool_result); free(char_result);

  /* Temporal *tne_pose_tpose(const Pose *cb, const Temporal *temp); */
  tbool_result = tne_pose_tpose(pose1, tpose1);
  char_result = tbool_out(tbool_result);
  printf("tne_pose_tpose(%s, %s): %s\n", pose1_out, tpose1_out, char_result);
  free(tbool_result); free(char_result);

  /* Temporal *tne_tpose_pose(const Temporal *temp, const Pose *cb); */
  tbool_result = tne_tpose_pose(tpose1, pose1);
  char_result = tbool_out(tbool_result);
  printf("tne_tpose_pose(%s, %s): %s\n", tpose1_out, pose1_out, char_result);
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
