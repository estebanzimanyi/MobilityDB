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
 * @brief Output, point-accessor, and trajectory-similarity functions for
 * temporal network points, composed over the position trajectory cast to
 * tgeompoint.
 */

/*****************************************************************************
 * Well-known text/binary output and geometry conversion
 *****************************************************************************/

CREATE FUNCTION asEWKT(tnpoint, maxdecimaldigits int4 DEFAULT 15)
  RETURNS text LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.asEWKT($1::@extschema@.tgeompoint, $2) $$;

CREATE FUNCTION asEWKB(tnpoint, endianenconding text DEFAULT '')
  RETURNS bytea LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.asEWKB($1::@extschema@.tgeompoint, $2) $$;

CREATE FUNCTION asHexEWKB(tnpoint, endianenconding text DEFAULT '')
  RETURNS text LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.asHexEWKB($1::@extschema@.tgeompoint, $2) $$;

CREATE FUNCTION geometry(tnpoint)
  RETURNS geometry LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT ($1::@extschema@.tgeompoint)::@extschema@.geometry $$;

/*****************************************************************************
 * Point accessors
 *****************************************************************************/

CREATE FUNCTION getX(tnpoint)
  RETURNS tfloat LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.getX($1::@extschema@.tgeompoint) $$;
CREATE FUNCTION getY(tnpoint)
  RETURNS tfloat LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.getY($1::@extschema@.tgeompoint) $$;
CREATE FUNCTION getZ(tnpoint)
  RETURNS tfloat LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.getZ($1::@extschema@.tgeompoint) $$;

CREATE FUNCTION azimuth(tnpoint)
  RETURNS tfloat LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.azimuth($1::@extschema@.tgeompoint) $$;
CREATE FUNCTION angularDifference(tnpoint)
  RETURNS tfloat LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.angularDifference($1::@extschema@.tgeompoint) $$;
CREATE FUNCTION direction(tnpoint)
  RETURNS double precision LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.direction($1::@extschema@.tgeompoint) $$;

CREATE FUNCTION bearing(tnpoint, geometry)
  RETURNS tfloat LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.bearing($1::@extschema@.tgeompoint, $2) $$;
CREATE FUNCTION bearing(tnpoint, tnpoint)
  RETURNS tfloat LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.bearing($1::@extschema@.tgeompoint, $2::@extschema@.tgeompoint) $$;

/*****************************************************************************
 * Trajectory similarity
 *****************************************************************************/

CREATE FUNCTION frechetDistance(tnpoint, tnpoint)
  RETURNS double precision LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.frechetDistance($1::@extschema@.tgeompoint, $2::@extschema@.tgeompoint) $$;
CREATE FUNCTION dynTimeWarpDistance(tnpoint, tnpoint)
  RETURNS double precision LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.dynTimeWarpDistance($1::@extschema@.tgeompoint, $2::@extschema@.tgeompoint) $$;
CREATE FUNCTION hausdorffDistance(tnpoint, tnpoint)
  RETURNS double precision LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.hausdorffDistance($1::@extschema@.tgeompoint, $2::@extschema@.tgeompoint) $$;

CREATE FUNCTION frechetDistancePath(tnpoint, tnpoint)
  RETURNS SETOF warp LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT * FROM @extschema@.frechetDistancePath($1::@extschema@.tgeompoint, $2::@extschema@.tgeompoint) $$;
CREATE FUNCTION dynTimeWarpPath(tnpoint, tnpoint)
  RETURNS SETOF warp LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT * FROM @extschema@.dynTimeWarpPath($1::@extschema@.tgeompoint, $2::@extschema@.tgeompoint) $$;

/*****************************************************************************
 * Spatiotemporal boxes, simplicity, MVT and measure
 *****************************************************************************/

CREATE FUNCTION stboxes(tnpoint)
  RETURNS stbox[] LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.stboxes($1::@extschema@.tgeompoint) $$;
CREATE FUNCTION splitNStboxes(tnpoint, integer)
  RETURNS stbox[] LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.splitNStboxes($1::@extschema@.tgeompoint, $2) $$;
CREATE FUNCTION splitEachNStboxes(tnpoint, integer)
  RETURNS stbox[] LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.splitEachNStboxes($1::@extschema@.tgeompoint, $2) $$;

CREATE FUNCTION isSimple(tnpoint)
  RETURNS boolean LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.isSimple($1::@extschema@.tgeompoint) $$;

CREATE FUNCTION asMVTGeom(tnpoint, bounds stbox, extent integer DEFAULT 4096,
    buffer integer DEFAULT 256, clip boolean DEFAULT true)
  RETURNS geom_times LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.asMVTGeom($1::@extschema@.tgeompoint, $2, $3, $4, $5) $$;

CREATE FUNCTION geoMeasure(tnpoint, tfloat, boolean DEFAULT false)
  RETURNS geometry LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.geoMeasure($1::@extschema@.tgeompoint, $2, $3) $$;

/*****************************************************************************/
