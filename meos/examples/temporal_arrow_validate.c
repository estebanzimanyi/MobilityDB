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
 * @brief Validate the MEOS Arrow C Data Interface export against a canonical
 * external Arrow consumer
 *
 * @details This builds representative temporal values, converts each through
 * #meos_temporal_to_arrow, and hands the produced `ArrowSchema`/`ArrowArray`
 * to nanoarrow, a small dependency-free implementation of the Arrow C Data
 * Interface. nanoarrow imports the schema and array with no MEOS knowledge
 * and runs full specification validation (`ArrowArrayViewInitFromSchema` +
 * `ArrowArrayViewSetArray` + `ArrowArrayViewValidate` at the FULL level),
 * which walks the entire nested
 * `Struct{ ..., seqs:List<Struct{ ..., insts:List<Struct{t, v}>}>}` tree.
 * The export is also self-checked through #meos_temporal_arrow_roundtrip as
 * a control.
 *
 * The full set of value-leaf tiers is exercised, one representative per
 * distinct value-leaf encoding:
 *
 * - The fully decomposed nested-Struct leaf, represented by the temporal
 *   circular buffer (`Struct{x,y,r}`), the temporal pose and the temporal
 *   network point. A 2D pose decomposes to `Struct{x,y,theta}` (three
 *   Float64 children); a 3D pose to `Struct{x,y,z,W,X,Y,Z}` (seven Float64
 *   children, the trailing four being the orientation quaternion). A
 *   network point decomposes to a mixed `Struct{rid:Int64, pos:Float64}`
 *   whose two children differ in width and format. The circular buffer is
 *   the homogeneous-Float64 control; the pose and the network point each
 *   exercise the decomposed leaf on their own field set.
 * - The opaque LargeBinary leaf and the rigid-geometry struct leaf:
 *   temporal geometry and geography carry their per-instant value as an
 *   opaque int64-offset LargeBinary "Z" leaf; a temporal point-cloud point
 *   and patch carry the same opaque LargeBinary "Z" leaf named "pcpoint"
 *   and "pcpatch"; a temporal rigid geometry carries a
 *   `Struct{ref:LargeBinary, ...pose fields}` whose leading "ref" child is
 *   the shared reference geometry as EWKB. These use a LargeBinary +
 *   int64-offset value-leaf encoding distinct from the decomposed tier.
 * - The scalar fixed-width leaf, represented by the temporal big integer
 *   (Int64 "l", an 8-byte buffer plus validity) and the temporal H3 index
 *   (UInt64 "L", an H3 cell index carried as an unsigned 64-bit integer,
 *   distinct from the big integer's signed leaf). Both are discrete-valued
 *   types (step interpolation, no linear). Only non-sentinel int64 values
 *   are used for the big integer: the type maximum is a documented
 *   backend-crashing value, exactly as the canonical big integer pg_regress
 *   coverage deliberately avoids it.
 *
 * @code
 * gcc -Wall -g -I/usr/local/include -Inanoarrow -o temporal_arrow_validate \
 *   temporal_arrow_validate.c nanoarrow/nanoarrow.c -L/usr/local/lib -lmeos
 * @endcode
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* nanoarrow is the canonical external Arrow C Data Interface oracle. It
 * supplies the ArrowSchema / ArrowArray / ArrowArrayStream ABI structs
 * (verbatim from the Arrow specification format/abi.h) and the C Data
 * Interface guard macro ARROW_C_DATA_INTERFACE. MEOS vendors the SAME two
 * structs in its own ABI-identical header (include guard
 * MEOS_ARROW_C_DATA_INTERFACE_H, same upstream source). Including nanoarrow
 * first and pre-defining MEOS's include guard makes the single canonical
 * definition apply on both sides; libmeos was compiled against the
 * ABI-identical copy, so the binary interface is unchanged. */
#include "nanoarrow/nanoarrow.h"
#define MEOS_ARROW_C_DATA_INTERFACE_H
#include <meos.h>
#include <meos_internal.h>
#include <meos_cbuffer.h>
#include <meos_geo.h>
#include <meos_h3.h>
#include <meos_npoint.h>
#include <meos_pose.h>
#include <meos_rgeo.h>
#include <meos_pointcloud.h>
/* A standalone MEOS program builds and registers a point-cloud value
 * exactly as meos/examples/tpcbox_rtree.c and
 * meos/test/pointcloud_valgrind.c do (in a PG backend mobilitydb_init
 * registers the schema lazily from the pointcloud_formats catalog and
 * PC_Patch builds the patch). pc_api.h / pc_api_internal.h are
 * pgPointCloud's vendored headers (PCSCHEMA, pc_schema_from_xml, the
 * PCPOINTLIST / PCPATCH builders); pointcloud/pgsql_compat.h is the MEOS
 * serialization shim (meos_pc_patch_serialize, SERIALIZED_PATCH). */
#include <pointcloud/pgsql_compat.h>
#include "pc_api.h"
#include "pc_api_internal.h"

/**
 * @brief Hand one already-built temporal value's Arrow export to the
 * external nanoarrow consumer for full specification validation, then
 * self-check it through the MEOS round-trip as a control
 *
 * @details @p temp is consumed (freed) by this function. nanoarrow imports
 * the schema and array with zero MEOS knowledge and validates the whole
 * nested tree at the FULL level (validity buffers, list offset buffers,
 * the LargeBinary/Struct value leaf, child counts, lengths and null
 * counts). The MEOS self round-trip is only a control on top of the
 * external verdict.
 */
static int
validate_temp(const char *label, Temporal *temp)
{
  if (! temp)
  {
    printf("[%s] FAIL: constructor returned NULL\n", label);
    return 1;
  }

  struct ArrowSchema schema;
  struct ArrowArray array;
  if (! meos_temporal_to_arrow(temp, &schema, &array))
  {
    printf("[%s] FAIL: meos_temporal_to_arrow\n", label);
    free(temp);
    return 1;
  }

  int rc = 0;
  struct ArrowError error;
  struct ArrowArrayView view;
  memset(&view, 0, sizeof(view));

  /* Independent external parse of the MEOS-produced schema. */
  if (ArrowArrayViewInitFromSchema(&view, &schema, &error) != NANOARROW_OK)
  {
    printf("[%s] FAIL: nanoarrow ArrowArrayViewInitFromSchema: %s\n", label,
      ArrowErrorMessage(&error));
    rc = 1;
    goto done;
  }
  /* Bind the MEOS-produced array: nanoarrow reads every buffer, derives
   * sizes from the list offsets, and checks the list/struct child wiring
   * (including the int64-offset LargeBinary value leaf). */
  if (ArrowArrayViewSetArray(&view, &array, &error) != NANOARROW_OK)
  {
    printf("[%s] FAIL: nanoarrow ArrowArrayViewSetArray: %s\n", label,
      ArrowErrorMessage(&error));
    rc = 1;
    goto done;
  }
  /* Full Arrow C Data Interface content validation. */
  if (ArrowArrayViewValidate(&view, NANOARROW_VALIDATION_LEVEL_FULL,
      &error) != NANOARROW_OK)
  {
    printf("[%s] FAIL: nanoarrow ArrowArrayViewValidate(FULL): %s\n", label,
      ArrowErrorMessage(&error));
    rc = 1;
    goto done;
  }

  /* Descend the externally parsed view to the per-instant value leaf and
   * report the storage type nanoarrow resolved for it. The path is the
   * fixed contract
   * top:Struct -> seqs:List -> seq:Struct -> insts:List -> inst:Struct
   * -> [t, v]; the value leaf is children[1] of the inst struct. This
   * proves nanoarrow actually walked into and FULL-validated the
   * value-leaf encoding (the opaque LargeBinary "Z" leaf for temporal
   * geometry/geography and point-cloud values, the scalar Int64/UInt64
   * leaf for a big integer / H3 index, the decomposed Struct for a
   * circular buffer / pose / network point, the Struct{ref:LargeBinary,...}
   * for a rigid geometry), not merely the outer skeleton. */
  {
    struct ArrowArrayView *v = &view;            /* top struct */
    const char *leafdesc = NULL;
    char leafbuf[96];
    if (v->n_children == 5)
    {
      struct ArrowArrayView *seqs = v->children[4];        /* seqs list */
      if (seqs->n_children == 1)
      {
        struct ArrowArrayView *seq = seqs->children[0];    /* seq struct */
        if (seq->n_children == 3)
        {
          struct ArrowArrayView *insts = seq->children[2]; /* insts list */
          if (insts->n_children == 1)
          {
            struct ArrowArrayView *inst = insts->children[0]; /* inst */
            if (inst->n_children == 2)
            {
              struct ArrowArrayView *vleaf = inst->children[1];
              if (vleaf->storage_type == NANOARROW_TYPE_LARGE_BINARY)
                leafdesc = "LargeBinary value leaf";
              else if (vleaf->storage_type == NANOARROW_TYPE_INT64)
                leafdesc = "scalar Int64 value leaf";
              else if (vleaf->storage_type == NANOARROW_TYPE_UINT64)
                leafdesc = "UInt64 value leaf";
              else if (vleaf->storage_type == NANOARROW_TYPE_STRUCT &&
                vleaf->n_children >= 1 &&
                vleaf->children[0]->storage_type ==
                  NANOARROW_TYPE_LARGE_BINARY)
                leafdesc = "Struct value leaf with LargeBinary ref child";
              else if (vleaf->storage_type == NANOARROW_TYPE_STRUCT)
              {
                /* Report the resolved child count so the pose 2D
                 * Struct{x,y,theta} (3) and 3D Struct{x,y,z,W,X,Y,Z}
                 * (7) field sets are visible in the verdict, distinct
                 * from the circular buffer Struct{x,y,r} (3) and the
                 * network point Struct{rid,pos} (2). */
                snprintf(leafbuf, sizeof(leafbuf),
                  "decomposed Struct value leaf (%d children)",
                  (int) vleaf->n_children);
                leafdesc = leafbuf;
              }
            }
          }
        }
      }
    }
    if (! leafdesc)
    {
      printf("[%s] FAIL: could not locate the value leaf in the parsed "
        "view\n", label);
      rc = 1;
      goto done;
    }
    printf("[%s] value leaf: %s\n", label, leafdesc);
  }

  /* Control: MEOS reads its own export back to the same value. */
  Temporal *back = meos_temporal_arrow_roundtrip(temp);
  if (! back)
  {
    printf("[%s] FAIL: meos_temporal_arrow_roundtrip\n", label);
    rc = 1;
    goto done;
  }
  char *s1 = temporal_out(temp, 6);
  char *s2 = temporal_out(back, 6);
  if (strcmp(s1, s2) != 0)
  {
    printf("[%s] FAIL: self round-trip mismatch\n  in : %s\n  out: %s\n",
      label, s1, s2);
    rc = 1;
  }
  free(s1);
  free(s2);
  free(back);

  if (rc == 0)
    printf("[%s] PASS: nanoarrow FULL-validate + self round-trip\n", label);

done:
  ArrowArrayViewReset(&view);
  schema.release(&schema);
  array.release(&array);
  free(temp);
  return rc;
}

/**
 * @brief Validate one temporal circular buffer value (the fully decomposed
 * nested-Struct value leaf, kept as the decomposed-tier control)
 */
static int
validate_cbuffer(const char *label, const char *in)
{
  return validate_temp(label, tcbuffer_in(in));
}

/**
 * @brief Validate one temporal big integer value (the scalar fixed-width
 * Int64 "l" value leaf)
 *
 * @details The caller passes only non-sentinel int64 values. The type
 * maximum (`9223372036854775807`, `INT64_MAX`) is a documented
 * backend-crashing value and is never used here, exactly as the canonical
 * big integer pg_regress coverage deliberately avoids it.
 */
static int
validate_tbigint(const char *label, const char *in)
{
  return validate_temp(label, tbigint_in(in));
}

/**
 * @brief Validate one temporal H3 index value (the scalar UInt64 "L" value
 * leaf, an H3 cell index carried as an unsigned 64-bit integer)
 *
 * @details An H3 cell index is a 64-bit value binary-identical to a signed
 * int64 but exported as the Arrow unsigned 64-bit "L" leaf, distinct from
 * the Int64 "l" leaf of a temporal big integer. A temporal H3 index is a
 * discrete-valued type: a sequence uses step interpolation (there is no
 * linear interpolation for cell indices), so the value leaf is a plain
 * contiguous UInt64 buffer with no per-instant offsets. The cell indices
 * used here are real valid H3 cells mirroring the canonical
 * 290_th3index_arrow pg_regress coverage.
 */
static int
validate_th3index(const char *label, const char *in)
{
  return validate_temp(label, th3index_in(in));
}

/**
 * @brief Validate one temporal pose value (the fully decomposed
 * nested-Struct value leaf: a 2D pose is `Struct{x,y,theta}`, a 3D pose is
 * `Struct{x,y,z,W,X,Y,Z}`)
 *
 * @details A temporal pose is built the canonical way through its string
 * input function #tpose_in; the 2D form is `Pose(Point(x y),theta)` and the
 * 3D form is `Pose(Point Z(x y z),W,X,Y,Z)` with a unit orientation
 * quaternion. nanoarrow walks into the value-leaf Struct and FULL-validates
 * its three (2D) or seven (3D) Float64 children, distinct from the circular
 * buffer `Struct{x,y,r}` representative; the pose field set is exercised
 * here on its own evidence.
 */
static int
validate_tpose(const char *label, const char *in)
{
  return validate_temp(label, tpose_in(in));
}

/**
 * @brief Validate one temporal network point value (the fully decomposed
 * mixed `Struct{rid:Int64, pos:Float64}` value leaf)
 *
 * @details The route identifiers used by the literals must exist in the
 * ways network the standalone MEOS build reads from the installed
 * `ways1000.csv` (route identifiers 0 to 1000); the relative positions are
 * fractions in [0, 1]. This exercises a decomposed value leaf whose two
 * children differ in width and format (Int64 "l" and Float64 "g"), unlike
 * the homogeneous Float64 children of the circular buffer leaf.
 */
static int
validate_tnpoint(const char *label, const char *in)
{
  return validate_temp(label, tnpoint_in(in));
}

/**
 * @brief Validate one temporal geometry value (opaque int64-offset
 * LargeBinary "Z" value leaf)
 */
static int
validate_tgeometry(const char *label, const char *in)
{
  return validate_temp(label, tgeometry_in(in));
}

/**
 * @brief Validate one temporal geography value (opaque int64-offset
 * LargeBinary "Z" value leaf, geodetic flag carried verbatim)
 */
static int
validate_tgeography(const char *label, const char *in)
{
  return validate_temp(label, tgeography_in(in));
}

/**
 * @brief Validate one temporal rigid geometry value (the value leaf is a
 * Struct whose leading "ref" child is the shared reference geometry as a
 * LargeBinary EWKB, followed by the per-instant pose fields)
 *
 * @details A temporal rigid geometry has no string input function; it is
 * built the canonical way, from a reference geometry plus a temporal pose
 * via #geo_tpose_to_trgeo (the same path the Arrow reader uses to
 * reconstruct it).
 */
static int
validate_trgeo(const char *label, const char *geo_wkt, const char *tpose_in_str)
{
  GSERIALIZED *gs = geom_in(geo_wkt, -1);
  if (! gs)
  {
    printf("[%s] FAIL: geom_in returned NULL\n", label);
    return 1;
  }
  Temporal *tp = tpose_in(tpose_in_str);
  if (! tp)
  {
    printf("[%s] FAIL: tpose_in returned NULL\n", label);
    free(gs);
    return 1;
  }
  Temporal *trgeo = geo_tpose_to_trgeo(gs, tp);
  free(gs);
  free(tp);
  return validate_temp(label, trgeo);
}

/* Canonical pcid = 1 schema (three int32_t X/Y/Z dimensions scaled by
 * 0.01), byte-identical to the one
 * mobilitydb/datagen/pointcloud/random_tpcpoint.sql installs in
 * pointcloud_formats and the one tpc_wkb_roundtrip.c's
 * PCPOINT_HEX_PCID1_111 layout matches. The point-cloud value leaf is
 * serialized with its pcid only; the schema is resolved out-of-band, so
 * registering it once lets meos_temporal_to_arrow compute the bbox and
 * the round-trip rebuild every tpcpoint/tpcpatch value. */
#define PCID1_SCHEMA_XML \
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" \
  "<pc:PointCloudSchema xmlns:pc=\"http://pointcloud.org/schemas/PC/1.1\"" \
  " xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">" \
  "<pc:dimension><pc:position>1</pc:position><pc:size>4</pc:size>" \
  "<pc:description>X coordinate</pc:description><pc:name>X</pc:name>" \
  "<pc:interpretation>int32_t</pc:interpretation><pc:scale>0.01</pc:scale>" \
  "</pc:dimension>" \
  "<pc:dimension><pc:position>2</pc:position><pc:size>4</pc:size>" \
  "<pc:description>Y coordinate</pc:description><pc:name>Y</pc:name>" \
  "<pc:interpretation>int32_t</pc:interpretation><pc:scale>0.01</pc:scale>" \
  "</pc:dimension>" \
  "<pc:dimension><pc:position>3</pc:position><pc:size>4</pc:size>" \
  "<pc:description>Z coordinate</pc:description><pc:name>Z</pc:name>" \
  "<pc:interpretation>int32_t</pc:interpretation><pc:scale>0.01</pc:scale>" \
  "</pc:dimension>" \
  "<pc:metadata><Metadata name=\"srid\">0</Metadata></pc:metadata>" \
  "</pc:PointCloudSchema>"

/* A pcid = 1 SERIALIZED_POINT in hex: [4-byte vl_len_ slot][4-byte pcid
 * little-endian = 1][3 x 4-byte int32 dimensions]. The layout and the
 * (1,1,1) seed are the ones documented and exercised by the canonical
 * meos/examples/tpc_wkb_roundtrip.c; distinct points only change the
 * three trailing int32 little-endian dimension words. The hex-WKB body
 * is parsed schema-free so no schema is needed to build a tpcpoint. */
#define PCPOINT_HEX_111 "0000000001000000010000000100000001000000"
#define PCPOINT_HEX_222 "0000000001000000020000000200000002000000"
#define PCPOINT_HEX_333 "0000000001000000030000000300000003000000"

/* The MEOS schema cache, populated by main(). */
static PCSCHEMA *pcid1_schema = NULL;

/**
 * @brief Build a tpcpoint instant from a serialized hex point body
 * @details @p hexbody is parsed schema-free by pcpoint_hex_in (mirroring
 * tpc_wkb_roundtrip.c). The value Datum is the varlena pointer; MEOS's
 * Datum is uintptr_t so the (Datum) cast is the portable equivalent of
 * PostgreSQL's PointerGetDatum.
 */
static TInstant *
tpcpoint_inst(const char *hexbody, const char *ts)
{
  Pcpoint *pt = pcpoint_hex_in(hexbody);
  if (! pt)
    return NULL;
  return tinstant_make((Datum) pt, T_TPCPOINT, pg_timestamptz_in(ts, -1));
}

/**
 * @brief Build a tpcpatch instant from an explicit list of X/Y/Z points
 * @details Mirrors meos/test/pointcloud_valgrind.c: assemble a
 * PCPOINTLIST against the registered pcid = 1 PCSCHEMA, form an
 * uncompressed PCPATCH and serialize it to the SERIALIZED_PATCH varlena
 * that is the tpcpatch instant value. This is the canonical way a
 * standalone MEOS program builds a patch (in a PG backend PC_Patch does
 * it). The bytes are built fresh in-process, not hand-encoded.
 */
static TInstant *
tpcpatch_inst(const double (*pts)[3], int npts, const char *ts)
{
  PCPOINTLIST *pl = pc_pointlist_make(npts);
  PCDIMENSION *xd = pc_schema_get_dimension(pcid1_schema, 0);
  PCDIMENSION *yd = pc_schema_get_dimension(pcid1_schema, 1);
  PCDIMENSION *zd = pc_schema_get_dimension(pcid1_schema, 2);
  for (int i = 0; i < npts; i++)
  {
    PCPOINT *pt = pc_point_make(pcid1_schema);
    pc_double_to_ptr(pt->data + xd->byteoffset, xd->interpretation,
      pts[i][0]);
    pc_double_to_ptr(pt->data + yd->byteoffset, yd->interpretation,
      pts[i][1]);
    pc_double_to_ptr(pt->data + zd->byteoffset, zd->interpretation,
      pts[i][2]);
    pc_pointlist_add_point(pl, pt);
  }
  PCPATCH *patch = (PCPATCH *) pc_patch_uncompressed_from_pointlist(pl);
  SERIALIZED_PATCH *ser = meos_pc_patch_serialize(patch, NULL);
  if (! ser)
    return NULL;
  return tinstant_make((Datum) ser, T_TPCPATCH,
    pg_timestamptz_in(ts, -1));
}

/**
 * @brief Validate one already-built temporal point-cloud point value
 * (the per-instant value is an opaque int64-offset LargeBinary "Z" leaf
 * whose name "pcpoint" discriminates it from the general-geometry leaf)
 */
static int
validate_tpcpoint(const char *label, Temporal *temp)
{
  return validate_temp(label, temp);
}

/**
 * @brief Validate one already-built temporal point-cloud patch value
 * (same opaque LargeBinary "Z" leaf encoding as tpcpoint, leaf name
 * "pcpatch")
 */
static int
validate_tpcpatch(const char *label, Temporal *temp)
{
  return validate_temp(label, temp);
}

int
main(void)
{
  meos_initialize();

  /* Pre-populate the MEOS schema cache for pcid = 1 the canonical
   * standalone way (meos/examples/tpcbox_rtree.c). The point-cloud
   * value leaf serializes the pcid only; meos_temporal_to_arrow and the
   * round-trip resolve the schema out-of-band from this cache, exactly
   * as the PG backend resolves it from the pointcloud_formats catalog. */
  pcid1_schema = pc_schema_from_xml(PCID1_SCHEMA_XML);
  if (! pcid1_schema)
  {
    printf("[setup] FAIL: pc_schema_from_xml(pcid 1)\n");
    meos_finalize();
    return 1;
  }
  pcid1_schema->pcid = 1;
  meos_pc_schema_register(1, pcid1_schema);

  int rc = 0;

  /* ---- Decomposed-tier control: temporal circular buffer ---- */
  rc |= validate_cbuffer("cbuffer-instant",
    "Cbuffer(Point(1 2),0.5)@2000-01-01");
  rc |= validate_cbuffer("cbuffer-sequence-inclusive",
    "[Cbuffer(Point(1 2),0.5)@2000-01-01, "
    "Cbuffer(Point(3 4),1.5)@2000-01-02]");
  rc |= validate_cbuffer("cbuffer-sequence-exclusive-negatives",
    "(Cbuffer(Point(-1 -2),0.25)@2000-01-01, "
    "Cbuffer(Point(0 0),3)@2000-01-02, "
    "Cbuffer(Point(0 0),3)@2000-01-03)");
  rc |= validate_cbuffer("cbuffer-sequence-set",
    "{[Cbuffer(Point(1 1),0.5)@2000-01-01, "
    "Cbuffer(Point(2 2),1)@2000-01-02], "
    "[Cbuffer(Point(3 3),1.5)@2000-01-03, "
    "Cbuffer(Point(4 4),2)@2000-01-04]}");

  /* ---- Decomposed tier: temporal pose ----
   * Validated independently of the circular buffer representative. A 2D
   * pose decomposes to a Struct{x,y,theta} (three Float64 children); a 3D
   * pose to a Struct{x,y,z,W,X,Y,Z} (seven Float64 children, the trailing
   * four being the orientation quaternion). Both field sets are exercised
   * across instant, sequence (inclusive and exclusive bounds, single
   * instant, negative theta) and sequence set. The literals mirror the
   * canonical 103_tpose_arrow pg_regress coverage; a pose is linearly
   * interpolated, so an exclusive upper bound needs no value repeat. */
  /* 2D poses: Struct{x,y,theta}. */
  rc |= validate_tpose("tpose-2d-instant",
    "Pose(Point(1 1),0.5)@2000-01-01");
  rc |= validate_tpose("tpose-2d-instant-srid",
    "SRID=3812;Pose(Point(1 2),1)@2000-01-01");
  rc |= validate_tpose("tpose-2d-single-instant-seq",
    "[Pose(Point(1 1),0.2)@2000-01-01]");
  rc |= validate_tpose("tpose-2d-sequence-inclusive",
    "[Pose(Point(1 1),0.2)@2000-01-01, Pose(Point(2 2),0.5)@2000-01-02, "
    "Pose(Point(3 3),0.9)@2000-01-03]");
  rc |= validate_tpose("tpose-2d-sequence-exclusive-negatives",
    "(Pose(Point(-1 -2),-0.5)@2000-01-01, "
    "Pose(Point(0 0),0)@2000-01-02, "
    "Pose(Point(3 1),-1.25)@2000-01-03)");
  rc |= validate_tpose("tpose-2d-discrete",
    "{Pose(Point(1 1),0.5)@2000-01-01, Pose(Point(2 2),1)@2000-01-02, "
    "Pose(Point(1 1),-0.5)@2000-01-03}");
  rc |= validate_tpose("tpose-2d-sequence-set",
    "{[Pose(Point(1 1),0.2)@2000-01-01, Pose(Point(2 2),0.5)@2000-01-02], "
    "[Pose(Point(3 3),0.6)@2000-01-04, Pose(Point(4 4),0.8)@2000-01-05]}");
  /* 3D poses: Struct{x,y,z,W,X,Y,Z} with a unit orientation quaternion. */
  rc |= validate_tpose("tpose-3d-instant",
    "Pose(Point Z(1 1 1),0.5,0.5,0.5,0.5)@2000-01-01");
  rc |= validate_tpose("tpose-3d-instant-srid",
    "SRID=3812;Pose(Point Z(1 2 3),1,0,0,0)@2000-01-01");
  rc |= validate_tpose("tpose-3d-single-instant-seq",
    "[Pose(Point Z(1 1 1),1,0,0,0)@2000-01-01]");
  rc |= validate_tpose("tpose-3d-sequence-inclusive",
    "[Pose(Point Z(1 1 1),0.5,0.5,0.5,0.5)@2000-01-01, "
    "Pose(Point Z(2 2 2),1,0,0,0)@2000-01-02]");
  rc |= validate_tpose("tpose-3d-discrete",
    "{Pose(Point Z(1 1 1),1,0,0,0)@2000-01-01, "
    "Pose(Point Z(2 2 2),0,1,0,0)@2000-01-02}");
  rc |= validate_tpose("tpose-3d-sequence-set",
    "{[Pose(Point Z(1 1 1),1,0,0,0)@2000-01-01, "
    "Pose(Point Z(2 2 2),0,0,0,1)@2000-01-02], "
    "[Pose(Point Z(3 3 3),0,1,0,0)@2000-01-04]}");

  /* ---- Decomposed tier: temporal network point ----
   * The value leaf is a mixed Struct{rid:Int64 "l", pos:Float64 "g"}; the
   * two children differ in width and format, so this independently
   * exercises the decomposed-Struct value-leaf path for a heterogeneous
   * leaf (the circular buffer above is a homogeneous Float64 leaf). The
   * literals mirror the canonical 084_tnpoint_arrow pg_regress coverage:
   * route identifiers 1, 2, 3 exist in the ways network the standalone
   * build reads from the installed ways1000.csv, and the positions are
   * fractions in [0, 1]. A temporal network point supports LINEAR
   * interpolation (default) and STEP; an exclusive upper bound with STEP
   * needs the last value repeated, exactly like the circular buffer and
   * geometry cases. */
  /* Instant: one pseudo-sequence of one instant. */
  rc |= validate_tnpoint("tnpoint-instant",
    "Npoint(1, 0.5)@2000-01-01");
  /* Single-instant sequence. */
  rc |= validate_tnpoint("tnpoint-single-instant-seq",
    "[Npoint(1, 0.5)@2000-01-01]");
  /* Discrete sequence over distinct routes. */
  rc |= validate_tnpoint("tnpoint-discrete",
    "{Npoint(1, 0.3)@2000-01-01, Npoint(2, 0.5)@2000-01-02, "
    "Npoint(3, 0.6)@2000-01-03}");
  /* Linear sequence with inclusive bounds along one route. */
  rc |= validate_tnpoint("tnpoint-sequence-linear-inclusive",
    "[Npoint(1, 0.2)@2000-01-01, Npoint(1, 0.4)@2000-01-02, "
    "Npoint(1, 0.5)@2000-01-03]");
  /* Linear sequence with exclusive bounds (no value repeat needed for
   * linear interpolation), boundary positions 0 and 1. */
  rc |= validate_tnpoint("tnpoint-sequence-linear-exclusive",
    "(Npoint(2, 0.0)@2000-01-01, Npoint(2, 0.5)@2000-01-02, "
    "Npoint(2, 1.0)@2000-01-03)");
  /* Step sequence with inclusive bounds (mirrors the canonical 084
   * coverage; the step rule needs no value repeat for an inclusive upper
   * bound). */
  rc |= validate_tnpoint("tnpoint-step-sequence",
    "Interp=Step;[Npoint(2, 0.2)@2000-01-01, Npoint(2, 0.4)@2000-01-02, "
    "Npoint(2, 0.7)@2000-01-03]");
  /* Step sequence with an exclusive upper bound (the step rule needs the
   * last value repeated). */
  rc |= validate_tnpoint("tnpoint-step-sequence-exclusive",
    "Interp=Step;[Npoint(3, 0.2)@2000-01-01, Npoint(3, 0.5)@2000-01-02, "
    "Npoint(3, 0.5)@2000-01-03)");
  /* Sequence set. */
  rc |= validate_tnpoint("tnpoint-sequence-set",
    "{[Npoint(1, 0.2)@2000-01-01, Npoint(1, 0.4)@2000-01-02], "
    "[Npoint(2, 0.6)@2000-01-04, Npoint(2, 0.7)@2000-01-05]}");

  /* ---- Scalar Int64 tier: temporal big integer ("l" leaf) ----
   * The big integer value leaf is a scalar fixed-width Int64 buffer plus
   * validity. The earlier conformance probes predate the big integer type
   * and never exercised this leaf, so it is validated here on its own
   * evidence. The literals mirror the canonical 024_temporal_arrow
   * pg_regress coverage (instant including a large non-sentinel value,
   * exclusive-bound step sequence repeating the last value, negative
   * values, single-instant sequence, discrete set, step sequence,
   * sequence set) and additionally exercise the larger non-sentinel
   * magnitudes the canonical coverage establishes as safe. The type
   * maximum 9223372036854775807 (INT64_MAX) is a documented
   * backend-crashing value and is deliberately never used, exactly as the
   * canonical pg_regress coverage avoids it; the largest value here is
   * 9000000000000000000. */
  /* Instant: one pseudo-sequence of one instant. */
  rc |= validate_tbigint("tbigint-instant",
    "42@2000-01-01");
  /* Instant with a large non-sentinel value (one short of the canonical
   * large literal; INT64_MAX itself is the backend-crashing sentinel and
   * is never used). */
  rc |= validate_tbigint("tbigint-instant-large",
    "9000000000000000000@2000-01-01");
  /* Instant with the additional non-sentinel magnitudes. */
  rc |= validate_tbigint("tbigint-instant-5e9",
    "5000000000@2000-01-01");
  rc |= validate_tbigint("tbigint-instant-negative",
    "-3000000000@2000-01-01");
  /* Single-instant sequence. */
  rc |= validate_tbigint("tbigint-single-instant-seq",
    "[42@2000-01-01]");
  /* Step sequence with an exclusive upper bound. The big integer type is
   * discrete and defaults to step interpolation, so an exclusive upper
   * bound requires the last value to repeat. */
  rc |= validate_tbigint("tbigint-sequence-exclusive-step",
    "[1@2000-01-01, 2@2000-01-02, 2@2000-01-03)");
  /* Closed sequence mixing zero, a negative and a positive value. */
  rc |= validate_tbigint("tbigint-sequence-negatives",
    "[0@2000-01-01, -3000000000@2000-01-02, 9000000000000000000@2000-01-03]");
  /* Discrete sequence (set of instants). */
  rc |= validate_tbigint("tbigint-discrete",
    "{1@2000-01-01, 2@2000-01-02, 3@2000-01-03}");
  /* Step sequence. */
  rc |= validate_tbigint("tbigint-step-sequence",
    "Interp=Step;[1@2000-01-01, 2@2000-01-02, 3@2000-01-03]");
  /* Sequence set. */
  rc |= validate_tbigint("tbigint-sequence-set",
    "{[1@2000-01-01, 2@2000-01-02], [3@2000-01-03, 4@2000-01-04]}");
  /* Sequence set spanning large non-sentinel magnitudes. */
  rc |= validate_tbigint("tbigint-sequence-set-large",
    "{[5000000000@2000-01-01, 5000000000@2000-01-02], "
    "[9000000000000000000@2000-01-03, 9000000000000000000@2000-01-04]}");

  /* ---- Scalar tier: temporal H3 index (UInt64 "L" leaf) ----
   * An H3 cell index is a 64-bit value exported as the unsigned 64-bit
   * "L" value leaf, distinct from a temporal big integer's signed "l"
   * leaf. The cell indices are real valid H3 cells mirroring the
   * canonical 290_th3index_arrow pg_regress coverage. A temporal H3
   * index is discrete: a sequence is step interpolated (no linear). The
   * step rule allows an exclusive upper bound only when the last value
   * repeats, so the exclusive-bound case repeats its final cell. */
  /* Instant from a hexadecimal H3 cell index. */
  rc |= validate_th3index("th3index-instant-hex",
    "831c02fffffffff@2001-01-01");
  /* Instant from the decimal form of the same value class. */
  rc |= validate_th3index("th3index-instant-decimal",
    "590464338553208831@2001-01-01");
  /* Single-instant sequence. */
  rc |= validate_th3index("th3index-single-instant-seq",
    "[831c02fffffffff@2001-01-01]");
  /* Discrete sequence. */
  rc |= validate_th3index("th3index-discrete",
    "{831c02fffffffff@2001-01-01, 831c00fffffffff@2001-01-02, "
    "880326b885fffff@2001-01-03}");
  /* Step sequence with closed bounds. */
  rc |= validate_th3index("th3index-step-sequence",
    "Interp=Step;[831c02fffffffff@2001-01-01, "
    "831c00fffffffff@2001-01-02, 880326b885fffff@2001-01-03]");
  /* Step sequence with an exclusive upper bound (the step rule requires
   * the last value to repeat). */
  rc |= validate_th3index("th3index-step-sequence-exclusive",
    "Interp=Step;(831c02fffffffff@2001-01-01, "
    "880326b885fffff@2001-01-02, 880326b885fffff@2001-01-03)");
  /* Sequence set. */
  rc |= validate_th3index("th3index-sequence-set",
    "{[831c02fffffffff@2001-01-01, 831c00fffffffff@2001-01-02], "
    "[880326b885fffff@2001-01-03, 880326b88dfffff@2001-01-04]}");

  /* ---- Opaque tier: temporal geometry (LargeBinary "Z" leaf) ----
   * The literals mirror the canonical 024_temporal_arrow pg_regress
   * coverage (instant polygon/linestring, explicit SRID, 3D, discrete
   * mixed-geometry set, step sequence, linear point sequence, sequence
   * set). A temporal geometry sequence with a general geometry value is
   * step interpolated, so an exclusive upper bound would have to repeat
   * the last value; the canonical coverage uses closed bounds and a
   * negative-coordinate point exercises the negative-buffer path. */
  /* Instant: one pseudo-sequence of one instant. */
  rc |= validate_tgeometry("tgeometry-instant-polygon",
    "Polygon((1 1,4 4,7 1,1 1))@2000-01-01");
  /* Instant with an explicit SRID. */
  rc |= validate_tgeometry("tgeometry-instant-srid",
    "SRID=3812;Polygon((1 1,4 4,7 1,1 1))@2000-01-01");
  /* Instant with a 3D linestring value. */
  rc |= validate_tgeometry("tgeometry-instant-3d",
    "Linestring(1 1 1,2 2 2)@2000-01-01");
  /* Single-instant sequence. */
  rc |= validate_tgeometry("tgeometry-single-instant-seq",
    "[Point(1 2)@2000-01-01]");
  /* Discrete sequence mixing point, linestring, polygon values. */
  rc |= validate_tgeometry("tgeometry-discrete-mixed",
    "{Point(1 1)@2000-01-01, Linestring(1 1,3 3)@2000-01-02, "
    "Polygon((1 1,4 4,7 1,1 1))@2000-01-03}");
  /* Step sequence (closed bounds; the step rule needs no value repeat
   * when the upper bound is inclusive). */
  rc |= validate_tgeometry("tgeometry-step-sequence",
    "Interp=Step;[Linestring(1 1,2 2)@2000-01-01, Point(2 2)@2000-01-02, "
    "Point(1 1)@2000-01-03]");
  /* Linear point sequence with a negative coordinate. */
  rc |= validate_tgeometry("tgeometry-sequence-linear-negatives",
    "[Point(-1 -2)@2000-01-01, Point(0 0)@2000-01-02, "
    "Point(3 1)@2000-01-03]");
  /* Sequence set. */
  rc |= validate_tgeometry("tgeometry-sequence-set",
    "{[Point(1 1)@2000-01-01, Point(2 2)@2000-01-02], "
    "[Point(3 3)@2000-01-03, Point(4 4)@2000-01-04]}");

  /* ---- Opaque tier: temporal geography (LargeBinary "Z" leaf) ----
   * Same opaque encoding as temporal geometry with the geodetic flag
   * carried verbatim; literals mirror the canonical 024 coverage. */
  rc |= validate_tgeography("tgeography-instant",
    "Point(1 2)@2000-01-01");
  rc |= validate_tgeography("tgeography-single-instant-seq",
    "[Point(1 2)@2000-01-01]");
  rc |= validate_tgeography("tgeography-sequence-inclusive",
    "[Point(1 1)@2000-01-01, Point(2 2)@2000-01-02, "
    "Point(3 3)@2000-01-03]");
  rc |= validate_tgeography("tgeography-sequence-set",
    "{[Point(1 1)@2000-01-01, Point(2 2)@2000-01-02], "
    "[Point(3 3)@2000-01-03, Point(4 4)@2000-01-04]}");

  /* ---- Opaque tier: temporal rigid geometry ----
   * value leaf = Struct{ref:LargeBinary EWKB, ...pose fields}. */
  /* Instant. */
  rc |= validate_trgeo("trgeo-instant",
    "Polygon((1 1,2 2,3 1,1 1))",
    "Pose(Point(1 1),0.5)@2000-01-01");
  /* Instant with an explicit SRID on the reference geometry. */
  rc |= validate_trgeo("trgeo-instant-srid",
    "SRID=4326;Polygon((1 1,2 2,3 1,1 1))",
    "SRID=4326;Pose(Point(1 2),0.5)@2000-01-01");
  /* Single-instant sequence. */
  rc |= validate_trgeo("trgeo-single-instant-seq",
    "Polygon((1 1,2 2,3 1,1 1))",
    "[Pose(Point(1 1),0.3)@2000-01-01]");
  /* Multi-instant linear sequence. */
  rc |= validate_trgeo("trgeo-sequence",
    "Polygon((1 1,2 2,3 1,1 1))",
    "[Pose(Point(1 1),0.3)@2000-01-01, Pose(Point(1 1),0.4)@2000-01-02, "
    "Pose(Point(1 1),0.5)@2000-01-03]");
  /* Step sequence (exclusive bounds with the step-interpolation rule). */
  rc |= validate_trgeo("trgeo-step-sequence",
    "Polygon((1 1,2 2,3 1,1 1))",
    "Interp=Step;[Pose(Point(1 2),0.5)@2000-01-01, "
    "Pose(Point(3 4),0.5)@2000-01-02]");
  /* Sequence set. */
  rc |= validate_trgeo("trgeo-sequence-set",
    "Polygon((1 1,2 2,3 1,1 1))",
    "Interp=Step;{[Pose(Point(1 1),0.2)@2000-01-01, "
    "Pose(Point(1 1),0.4)@2000-01-02, Pose(Point(1 1),0.5)@2000-01-03], "
    "[Pose(Point(2 2),0.6)@2000-01-04, Pose(Point(2 2),0.6)@2000-01-05]}");

  /* ---- Opaque tier: temporal point cloud point (LargeBinary "Z" leaf
   * named "pcpoint") ----
   * tpcpoint defaults to step interpolation, so sequences use closed
   * bounds; the shapes mirror the canonical 450_tpc_arrow pg_regress
   * coverage (instant, discrete sequence, step sequence, sequence set)
   * plus a single-instant sequence consistent with the other tiers. */
  rc |= validate_tpcpoint("tpcpoint-instant",
    (Temporal *) tpcpoint_inst(PCPOINT_HEX_111, "2024-01-01"));
  rc |= validate_tpcpoint("tpcpoint-single-instant-seq",
    (Temporal *) tsequence_make(
      (TInstant *[]){ tpcpoint_inst(PCPOINT_HEX_111, "2024-01-01") },
      1, true, true, STEP, true));
  rc |= validate_tpcpoint("tpcpoint-discrete-sequence",
    (Temporal *) tsequence_make(
      (TInstant *[]){ tpcpoint_inst(PCPOINT_HEX_111, "2024-01-01"),
        tpcpoint_inst(PCPOINT_HEX_222, "2024-01-02"),
        tpcpoint_inst(PCPOINT_HEX_333, "2024-01-03") },
      3, true, true, DISCRETE, true));
  rc |= validate_tpcpoint("tpcpoint-step-sequence",
    (Temporal *) tsequence_make(
      (TInstant *[]){ tpcpoint_inst(PCPOINT_HEX_111, "2024-01-01"),
        tpcpoint_inst(PCPOINT_HEX_222, "2024-01-02"),
        tpcpoint_inst(PCPOINT_HEX_333, "2024-01-03") },
      3, true, true, STEP, true));
  rc |= validate_tpcpoint("tpcpoint-sequence-set",
    (Temporal *) tsequenceset_make(
      (TSequence *[]){
        tsequence_make(
          (TInstant *[]){ tpcpoint_inst(PCPOINT_HEX_111, "2024-01-01"),
            tpcpoint_inst(PCPOINT_HEX_222, "2024-01-02") },
          2, true, true, STEP, true),
        tsequence_make(
          (TInstant *[]){ tpcpoint_inst(PCPOINT_HEX_333, "2024-01-03") },
          1, true, true, STEP, true) },
      2, true));

  /* ---- Opaque tier: temporal point cloud patch (LargeBinary "Z" leaf
   * named "pcpatch") ----
   * Patches are built fresh in-process from explicit X/Y/Z point lists
   * against the registered pcid = 1 schema, mirroring the canonical
   * 450_tpc_arrow PC_Patch(ARRAY[PC_MakePoint(1, ...)]) coverage. */
  {
    static const double s12[][3] = { {1, 1, 1}, {2, 2, 2} };
    static const double s56[][3] = { {5, 5, 5}, {6, 6, 6} };
    static const double s3[][3]  = { {3, 3, 3} };
    rc |= validate_tpcpatch("tpcpatch-instant",
      (Temporal *) tpcpatch_inst(s12, 2, "2024-01-01"));
    rc |= validate_tpcpatch("tpcpatch-single-instant-seq",
      (Temporal *) tsequence_make(
        (TInstant *[]){ tpcpatch_inst(s12, 2, "2024-01-01") },
        1, true, true, STEP, true));
    rc |= validate_tpcpatch("tpcpatch-discrete-sequence",
      (Temporal *) tsequence_make(
        (TInstant *[]){ tpcpatch_inst(s12, 2, "2024-01-01"),
          tpcpatch_inst(s56, 2, "2024-01-02"),
          tpcpatch_inst(s3, 1, "2024-01-03") },
        3, true, true, DISCRETE, true));
    rc |= validate_tpcpatch("tpcpatch-step-sequence",
      (Temporal *) tsequence_make(
        (TInstant *[]){ tpcpatch_inst(s12, 2, "2024-01-01"),
          tpcpatch_inst(s56, 2, "2024-01-02"),
          tpcpatch_inst(s3, 1, "2024-01-03") },
        3, true, true, STEP, true));
    rc |= validate_tpcpatch("tpcpatch-sequence-set",
      (Temporal *) tsequenceset_make(
        (TSequence *[]){
          tsequence_make(
            (TInstant *[]){ tpcpatch_inst(s12, 2, "2024-01-01"),
              tpcpatch_inst(s56, 2, "2024-01-02") },
            2, true, true, STEP, true),
          tsequence_make(
            (TInstant *[]){ tpcpatch_inst(s3, 1, "2024-01-03") },
            1, true, true, STEP, true) },
        2, true));
  }

  meos_finalize();
  printf("==== %s ====\n", rc ? "OVERALL FAIL" : "OVERALL PASS");
  return rc ? 1 : 0;
}
