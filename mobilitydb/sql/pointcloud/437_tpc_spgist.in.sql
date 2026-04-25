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
 *****************************************************************************/

/**
 * @file
 * @brief SP-GiST quadtree and kd-tree opclasses on tpcpoint / tpcpatch
 *   using STBox as the lossy storage type. pcid is dropped at the
 *   index level and recovered by the operator's recheck on the actual
 *   leaf value.
 *
 * Reuses the existing stbox_spgist_* support functions; only a fresh
 * compress method (Tpc_spgist_compress) is needed to derive an STBox
 * from a tpcpoint / tpcpatch leaf entry.
 */

CREATE FUNCTION tpc_spgist_compress(internal)
  RETURNS internal
  AS 'MODULE_PATHNAME', 'Tpc_spgist_compress'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

/*****************************************************************************
 * tpcpoint
 *****************************************************************************/

CREATE OPERATOR CLASS tpcpoint_quadtree_ops
  DEFAULT FOR TYPE tpcpoint USING spgist AS
  -- overlaps
  OPERATOR  3    && (tpcpoint, tstzspan),
  OPERATOR  3    && (tpcpoint, tpcbox),
  OPERATOR  3    && (tpcpoint, tpcpoint),
  -- same
  OPERATOR  6    ~= (tpcpoint, tstzspan),
  OPERATOR  6    ~= (tpcpoint, tpcbox),
  OPERATOR  6    ~= (tpcpoint, tpcpoint),
  -- contains
  OPERATOR  7    @> (tpcpoint, tstzspan),
  OPERATOR  7    @> (tpcpoint, tpcbox),
  OPERATOR  7    @> (tpcpoint, tpcpoint),
  -- contained by
  OPERATOR  8    <@ (tpcpoint, tstzspan),
  OPERATOR  8    <@ (tpcpoint, tpcbox),
  OPERATOR  8    <@ (tpcpoint, tpcpoint),
  -- adjacent
  OPERATOR  17   -|- (tpcpoint, tstzspan),
  OPERATOR  17   -|- (tpcpoint, tpcbox),
  OPERATOR  17   -|- (tpcpoint, tpcpoint),
  -- functions
  FUNCTION  1    stbox_spgist_config(internal, internal),
  FUNCTION  2    stbox_quadtree_choose(internal, internal),
  FUNCTION  3    stbox_quadtree_picksplit(internal, internal),
  FUNCTION  4    stbox_quadtree_inner_consistent(internal, internal),
  FUNCTION  5    stbox_spgist_leaf_consistent(internal, internal),
  FUNCTION  6    tpc_spgist_compress(internal);

CREATE OPERATOR CLASS tpcpoint_kdtree_ops
  FOR TYPE tpcpoint USING spgist AS
  -- overlaps
  OPERATOR  3    && (tpcpoint, tstzspan),
  OPERATOR  3    && (tpcpoint, tpcbox),
  OPERATOR  3    && (tpcpoint, tpcpoint),
  -- same
  OPERATOR  6    ~= (tpcpoint, tstzspan),
  OPERATOR  6    ~= (tpcpoint, tpcbox),
  OPERATOR  6    ~= (tpcpoint, tpcpoint),
  -- contains
  OPERATOR  7    @> (tpcpoint, tstzspan),
  OPERATOR  7    @> (tpcpoint, tpcbox),
  OPERATOR  7    @> (tpcpoint, tpcpoint),
  -- contained by
  OPERATOR  8    <@ (tpcpoint, tstzspan),
  OPERATOR  8    <@ (tpcpoint, tpcbox),
  OPERATOR  8    <@ (tpcpoint, tpcpoint),
  -- adjacent
  OPERATOR  17   -|- (tpcpoint, tstzspan),
  OPERATOR  17   -|- (tpcpoint, tpcbox),
  OPERATOR  17   -|- (tpcpoint, tpcpoint),
  -- functions
  FUNCTION  1    stbox_spgist_config(internal, internal),
  FUNCTION  2    stbox_kdtree_choose(internal, internal),
  FUNCTION  3    stbox_kdtree_picksplit(internal, internal),
  FUNCTION  4    stbox_kdtree_inner_consistent(internal, internal),
  FUNCTION  5    stbox_spgist_leaf_consistent(internal, internal),
  FUNCTION  6    tpc_spgist_compress(internal);

/*****************************************************************************
 * tpcpatch
 *****************************************************************************/

CREATE OPERATOR CLASS tpcpatch_quadtree_ops
  DEFAULT FOR TYPE tpcpatch USING spgist AS
  -- overlaps
  OPERATOR  3    && (tpcpatch, tstzspan),
  OPERATOR  3    && (tpcpatch, tpcbox),
  OPERATOR  3    && (tpcpatch, tpcpatch),
  -- same
  OPERATOR  6    ~= (tpcpatch, tstzspan),
  OPERATOR  6    ~= (tpcpatch, tpcbox),
  OPERATOR  6    ~= (tpcpatch, tpcpatch),
  -- contains
  OPERATOR  7    @> (tpcpatch, tstzspan),
  OPERATOR  7    @> (tpcpatch, tpcbox),
  OPERATOR  7    @> (tpcpatch, tpcpatch),
  -- contained by
  OPERATOR  8    <@ (tpcpatch, tstzspan),
  OPERATOR  8    <@ (tpcpatch, tpcbox),
  OPERATOR  8    <@ (tpcpatch, tpcpatch),
  -- adjacent
  OPERATOR  17   -|- (tpcpatch, tstzspan),
  OPERATOR  17   -|- (tpcpatch, tpcbox),
  OPERATOR  17   -|- (tpcpatch, tpcpatch),
  -- functions
  FUNCTION  1    stbox_spgist_config(internal, internal),
  FUNCTION  2    stbox_quadtree_choose(internal, internal),
  FUNCTION  3    stbox_quadtree_picksplit(internal, internal),
  FUNCTION  4    stbox_quadtree_inner_consistent(internal, internal),
  FUNCTION  5    stbox_spgist_leaf_consistent(internal, internal),
  FUNCTION  6    tpc_spgist_compress(internal);

CREATE OPERATOR CLASS tpcpatch_kdtree_ops
  FOR TYPE tpcpatch USING spgist AS
  -- overlaps
  OPERATOR  3    && (tpcpatch, tstzspan),
  OPERATOR  3    && (tpcpatch, tpcbox),
  OPERATOR  3    && (tpcpatch, tpcpatch),
  -- same
  OPERATOR  6    ~= (tpcpatch, tstzspan),
  OPERATOR  6    ~= (tpcpatch, tpcbox),
  OPERATOR  6    ~= (tpcpatch, tpcpatch),
  -- contains
  OPERATOR  7    @> (tpcpatch, tstzspan),
  OPERATOR  7    @> (tpcpatch, tpcbox),
  OPERATOR  7    @> (tpcpatch, tpcpatch),
  -- contained by
  OPERATOR  8    <@ (tpcpatch, tstzspan),
  OPERATOR  8    <@ (tpcpatch, tpcbox),
  OPERATOR  8    <@ (tpcpatch, tpcpatch),
  -- adjacent
  OPERATOR  17   -|- (tpcpatch, tstzspan),
  OPERATOR  17   -|- (tpcpatch, tpcbox),
  OPERATOR  17   -|- (tpcpatch, tpcpatch),
  -- functions
  FUNCTION  1    stbox_spgist_config(internal, internal),
  FUNCTION  2    stbox_kdtree_choose(internal, internal),
  FUNCTION  3    stbox_kdtree_picksplit(internal, internal),
  FUNCTION  4    stbox_kdtree_inner_consistent(internal, internal),
  FUNCTION  5    stbox_spgist_leaf_consistent(internal, internal),
  FUNCTION  6    tpc_spgist_compress(internal);

/*****************************************************************************/
