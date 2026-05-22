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
 * @brief Analytic functions for temporal network points
 *
 * Simplification restricts the original tnpoint to the timestamps that
 * survive simplifying its position trajectory (cast to tgeompoint),
 * preserving the route+fraction channel. The span functions operate purely
 * on the time dimension via the generic temporal kernels.
 */

/*****************************************************************************
 * spans and splitN/splitEachN spans (pure time-dimension)
 *****************************************************************************/

CREATE FUNCTION spans(tnpoint)
  RETURNS tstzspan[]
  AS 'MODULE_PATHNAME', 'Temporal_spans'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION splitNSpans(tnpoint, integer)
  RETURNS tstzspan[]
  AS 'MODULE_PATHNAME', 'Temporal_split_n_spans'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION splitEachNSpans(tnpoint, integer)
  RETURNS tstzspan[]
  AS 'MODULE_PATHNAME', 'Temporal_split_each_n_spans'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

/*****************************************************************************
 * Simplification (via tgeompoint composition)
 *****************************************************************************/

CREATE FUNCTION minDistSimplify(tnpoint, float)
  RETURNS tnpoint
  LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE AS $$
    SELECT @extschema@.atTime(
      $1,
      @extschema@.set(@extschema@.timestamps(
        @extschema@.minDistSimplify($1::@extschema@.tgeompoint, $2))))
  $$;

CREATE FUNCTION minTimeDeltaSimplify(tnpoint, interval)
  RETURNS tnpoint
  LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE AS $$
    SELECT @extschema@.atTime(
      $1,
      @extschema@.set(@extschema@.timestamps(
        @extschema@.minTimeDeltaSimplify($1::@extschema@.tgeompoint, $2))))
  $$;

CREATE FUNCTION maxDistSimplify(tnpoint, float, boolean DEFAULT TRUE)
  RETURNS tnpoint
  LANGUAGE SQL IMMUTABLE PARALLEL SAFE AS $$
    SELECT @extschema@.atTime(
      $1,
      @extschema@.set(@extschema@.timestamps(
        @extschema@.maxDistSimplify($1::@extschema@.tgeompoint, $2, $3))))
  $$;

CREATE FUNCTION douglasPeuckerSimplify(tnpoint, float, boolean DEFAULT TRUE)
  RETURNS tnpoint
  LANGUAGE SQL IMMUTABLE PARALLEL SAFE AS $$
    SELECT @extschema@.atTime(
      $1,
      @extschema@.set(@extschema@.timestamps(
        @extschema@.douglasPeuckerSimplify($1::@extschema@.tgeompoint, $2, $3))))
  $$;

/*****************************************************************************/
