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
 * @brief Aggregate functions for the pgPointCloud temporal types.
 */

CREATE FUNCTION tpcbox_extent_transfn(tpcbox, tpcbox)
  RETURNS tpcbox
  AS 'MODULE_PATHNAME', 'Tpcbox_extent_transfn'
  LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE FUNCTION tpc_extent_transfn(tpcbox, tpcpoint)
  RETURNS tpcbox
  AS 'MODULE_PATHNAME', 'Tpc_extent_transfn'
  LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE FUNCTION tpc_extent_transfn(tpcbox, tpcpatch)
  RETURNS tpcbox
  AS 'MODULE_PATHNAME', 'Tpc_extent_transfn'
  LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE AGGREGATE extent(tpcbox) (
  SFUNC = tpcbox_extent_transfn,
  STYPE = tpcbox,
  COMBINEFUNC = tpcbox_extent_transfn,
  PARALLEL = safe
);

CREATE AGGREGATE extent(tpcpoint) (
  SFUNC = tpc_extent_transfn,
  STYPE = tpcbox,
  COMBINEFUNC = tpcbox_extent_transfn,
  PARALLEL = safe
);

CREATE AGGREGATE extent(tpcpatch) (
  SFUNC = tpc_extent_transfn,
  STYPE = tpcbox,
  COMBINEFUNC = tpcbox_extent_transfn,
  PARALLEL = safe
);

/*****************************************************************************/
