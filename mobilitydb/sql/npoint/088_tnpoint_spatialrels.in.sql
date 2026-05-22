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
 * @brief Ever/always and temporal spatial relationships for temporal network
 * points, composed over the position trajectory (cast to tgeompoint). A
 * network point is planar, so its relationships coincide with those of its
 * geometry position.
 */

/*****************************************************************************
 * eIntersects, aIntersects
 *****************************************************************************/

CREATE FUNCTION eIntersects(tnpoint, geometry)
  RETURNS boolean LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.eIntersects($1::@extschema@.tgeompoint, $2) $$;
CREATE FUNCTION eIntersects(geometry, tnpoint)
  RETURNS boolean LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.eIntersects($1, $2::@extschema@.tgeompoint) $$;
CREATE FUNCTION eIntersects(tnpoint, tnpoint)
  RETURNS boolean LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.eIntersects($1::@extschema@.tgeompoint, $2::@extschema@.tgeompoint) $$;

CREATE FUNCTION aIntersects(tnpoint, geometry)
  RETURNS boolean LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.aIntersects($1::@extschema@.tgeompoint, $2) $$;
CREATE FUNCTION aIntersects(geometry, tnpoint)
  RETURNS boolean LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.aIntersects($1, $2::@extschema@.tgeompoint) $$;
CREATE FUNCTION aIntersects(tnpoint, tnpoint)
  RETURNS boolean LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.aIntersects($1::@extschema@.tgeompoint, $2::@extschema@.tgeompoint) $$;

/*****************************************************************************
 * eDisjoint, aDisjoint
 *****************************************************************************/

CREATE FUNCTION eDisjoint(tnpoint, geometry)
  RETURNS boolean LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.eDisjoint($1::@extschema@.tgeompoint, $2) $$;
CREATE FUNCTION eDisjoint(geometry, tnpoint)
  RETURNS boolean LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.eDisjoint($1, $2::@extschema@.tgeompoint) $$;
CREATE FUNCTION eDisjoint(tnpoint, tnpoint)
  RETURNS boolean LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.eDisjoint($1::@extschema@.tgeompoint, $2::@extschema@.tgeompoint) $$;

CREATE FUNCTION aDisjoint(tnpoint, geometry)
  RETURNS boolean LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.aDisjoint($1::@extschema@.tgeompoint, $2) $$;
CREATE FUNCTION aDisjoint(geometry, tnpoint)
  RETURNS boolean LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.aDisjoint($1, $2::@extschema@.tgeompoint) $$;
CREATE FUNCTION aDisjoint(tnpoint, tnpoint)
  RETURNS boolean LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.aDisjoint($1::@extschema@.tgeompoint, $2::@extschema@.tgeompoint) $$;

/*****************************************************************************
 * eTouches, aTouches
 *****************************************************************************/

CREATE FUNCTION eTouches(tnpoint, geometry)
  RETURNS boolean LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.eTouches($1::@extschema@.tgeompoint, $2) $$;
CREATE FUNCTION eTouches(geometry, tnpoint)
  RETURNS boolean LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.eTouches($1, $2::@extschema@.tgeompoint) $$;

CREATE FUNCTION aTouches(tnpoint, geometry)
  RETURNS boolean LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.aTouches($1::@extschema@.tgeompoint, $2) $$;
CREATE FUNCTION aTouches(geometry, tnpoint)
  RETURNS boolean LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.aTouches($1, $2::@extschema@.tgeompoint) $$;

/*****************************************************************************
 * eDwithin, aDwithin
 *****************************************************************************/

CREATE FUNCTION eDwithin(tnpoint, geometry, dist float)
  RETURNS boolean LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.eDwithin($1::@extschema@.tgeompoint, $2, $3) $$;
CREATE FUNCTION eDwithin(geometry, tnpoint, dist float)
  RETURNS boolean LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.eDwithin($1, $2::@extschema@.tgeompoint, $3) $$;
CREATE FUNCTION eDwithin(tnpoint, tnpoint, dist float)
  RETURNS boolean LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.eDwithin($1::@extschema@.tgeompoint, $2::@extschema@.tgeompoint, $3) $$;

CREATE FUNCTION aDwithin(tnpoint, geometry, dist float)
  RETURNS boolean LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.aDwithin($1::@extschema@.tgeompoint, $2, $3) $$;
CREATE FUNCTION aDwithin(geometry, tnpoint, dist float)
  RETURNS boolean LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.aDwithin($1, $2::@extschema@.tgeompoint, $3) $$;
CREATE FUNCTION aDwithin(tnpoint, tnpoint, dist float)
  RETURNS boolean LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.aDwithin($1::@extschema@.tgeompoint, $2::@extschema@.tgeompoint, $3) $$;

/*****************************************************************************
 * tIntersects, tDisjoint, tTouches  (return tbool)
 *****************************************************************************/

CREATE FUNCTION tIntersects(tnpoint, geometry)
  RETURNS tbool LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.tIntersects($1::@extschema@.tgeompoint, $2) $$;
CREATE FUNCTION tIntersects(tnpoint, tnpoint)
  RETURNS tbool LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.tIntersects($1::@extschema@.tgeompoint, $2::@extschema@.tgeompoint) $$;

CREATE FUNCTION tDisjoint(tnpoint, geometry)
  RETURNS tbool LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.tDisjoint($1::@extschema@.tgeompoint, $2) $$;
CREATE FUNCTION tDisjoint(tnpoint, tnpoint)
  RETURNS tbool LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.tDisjoint($1::@extschema@.tgeompoint, $2::@extschema@.tgeompoint) $$;

CREATE FUNCTION tTouches(tnpoint, geometry)
  RETURNS tbool LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.tTouches($1::@extschema@.tgeompoint, $2) $$;

/*****************************************************************************
 * tDwithin  (return tbool)
 *****************************************************************************/

CREATE FUNCTION tDwithin(tnpoint, geometry, dist float)
  RETURNS tbool LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.tDwithin($1::@extschema@.tgeompoint, $2, $3) $$;
CREATE FUNCTION tDwithin(tnpoint, tnpoint, dist float)
  RETURNS tbool LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
  AS $$ SELECT @extschema@.tDwithin($1::@extschema@.tgeompoint, $2::@extschema@.tgeompoint, $3) $$;

/*****************************************************************************/
