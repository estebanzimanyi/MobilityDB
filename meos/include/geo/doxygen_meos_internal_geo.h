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
 * @defgroup meos_internal_geo_base Internal functions for static geometries
 * @ingroup meos_internal_geo
 * @brief Internal functions for static geometries
 *
 * @defgroup meos_internal_geo_set Internal functions for geometry sets
 * @ingroup meos_internal_geo
 * @brief Internal functions for geometry sets
 *
 * @defgroup meos_internal_geo_box Internal functions for spatiotemporal boxes
 * @ingroup meos_internal_geo
 * @brief Internal functions for spatiotemporal boxes
 *
 * @defgroup meos_internal_geo_inout Internal input and output functions
 * @ingroup meos_internal_geo
 * @brief Internal input and output functions for temporal geometries
 *
 * @defgroup meos_internal_geo_constructor Internal constructor functions
 * @ingroup meos_internal_geo
 * @brief Internal constructor functions for temporal geometries
 *
 * @defgroup meos_internal_geo_conversion Internal conversion functions
 * @ingroup meos_internal_geo
 * @brief Internal conversion functions for temporal geometries
 *
 * @defgroup meos_internal_geo_accessor Internal accessor functions
 * @ingroup meos_internal_geo
 * @brief Internal accessor functions for temporal geometries
 *
 * @defgroup meos_internal_geo_transf Internal transformation functions
 * @ingroup meos_internal_geo
 * @brief Internal transformation functions for temporal geometries
 *
 * @defgroup meos_internal_geo_restrict Internal restriction functions
 * @ingroup meos_internal_geo
 * @brief Internal restriction functions for temporal geometries
 *
 * @defgroup meos_internal_geo_comp Internal comparison functions
 * @ingroup meos_internal_geo
 * @brief Internal comparison functions for temporal geometries
 *
 *   @defgroup meos_internal_geo_comp_ever Internal ever and always comparison functions
 *   @ingroup meos_internal_geo_comp
 *   @brief Internal ever and always comparison functions for temporal geometries
 *
 *   @defgroup meos_internal_geo_comp_temp Internal temporal comparison functions
 *   @ingroup meos_internal_geo_comp
 *   @brief Internal temporal comparison functions for temporal geometries
 *
 * @defgroup meos_internal_geo_bbox Internal bounding box functions
 * @ingroup meos_internal_geo
 * @brief Internal bounding box functions for temporal geometries
 *
 *   @defgroup meos_internal_geo_bbox_topo Internal topological functions
 *   @ingroup meos_internal_geo_bbox
 *   @brief Internal topological functions for temporal geometries
 *
 *   @defgroup meos_internal_geo_bbox_pos Internal position functions
 *   @ingroup meos_internal_geo_bbox
 *   @brief Internal position functions for temporal geometries
 *
 * @defgroup meos_internal_geo_dist Internal distance functions
 * @ingroup meos_internal_geo
 * @brief Internal distance functions for temporal geometries
 *
 * @defgroup meos_internal_geo_srid Internal spatial reference system functions
 * @ingroup meos_internal_geo
 * @brief Internal spatial reference system functions for temporal geos
 *
 * @defgroup meos_internal_geo_rel Internal spatial relationship functions
 * @ingroup meos_internal_geo
 * @brief Internal spatial relationship functions for temporal geos
 *
 *   @defgroup meos_internal_geo_rel_ever Internal ever/always relationship functions
 *   @ingroup meos_internal_geo_rel
 *   @brief Internal ever/always relationship functions for temporal geometries
 *
 *   @defgroup meos_internal_geo_rel_temp Internal temporal relationship functions
 *   @ingroup meos_internal_geo_rel
 *   @brief Internal temporal relationship functions for temporal geometries
 *
 * @defgroup meos_internal_geo_agg Internal aggregate functions
 * @ingroup meos_internal_geo
 * @brief Internal aggregate functions for temporal geometries
 *
 * @defgroup meos_internal_geo_tile Internal tile functions
 * @ingroup meos_internal_geo
 * @brief Internal tile functions for temporal geometries
 */

/*****************************************************************************/

/**
 * @defgroup meos_internal_geo_base_inout Internal input and output functions
 * @ingroup meos_internal_geo_base
 * @brief Internal input and output functions for static geometries
 *
 * @defgroup meos_internal_geo_base_constructor Internal constructor functions
 * @ingroup meos_internal_geo_base
 * @brief Internal constructor functions for static geometries
 *
 * @defgroup meos_internal_geo_base_conversion Internal conversion functions
 * @ingroup meos_internal_geo_base
 * @brief Internal conversion functions for static geometries
 *
 * @defgroup meos_internal_geo_base_accessor Internal accessor functions
 * @ingroup meos_internal_geo_base
 * @brief Internal accessor functions for static geometries
 *
 * @defgroup meos_internal_geo_base_transf Internal transformation functions
 * @ingroup meos_internal_geo_base
 * @brief Internal transformation functions for static geometries
 *
 * @defgroup meos_internal_geo_base_srid Internal spatial reference system functions
 * @ingroup meos_internal_geo_base
 * @brief Internal spatial reference system functions for temporal geos
 *
 * @defgroup meos_internal_geo_base_spatial Internal spatial processing functions
 * @ingroup meos_internal_geo_base
 * @brief Internal spatial processing functions for static geometries
 *
 * @defgroup meos_internal_geo_base_rel Internal spatial relationship functions
 * @ingroup meos_internal_geo_base
 * @brief Internal spatial relationship functions for temporal geos
 *
 * @defgroup meos_internal_geo_base_bbox Internal bounding box functions
 * @ingroup meos_internal_geo_base
 * @brief Internal bounding box functions for static geometries
 *
 * @defgroup meos_internal_geo_base_comp Internal comparison functions
 * @ingroup meos_internal_geo_base
 * @brief Internal comparison functions for static geometries
 */

/*****************************************************************************/

/**
 * @defgroup meos_internal_geo_set_inout Internal input and output functions
 * @ingroup meos_internal_geo_set
 * @brief Internal input and output functions for geometry sets
 *
 * @defgroup meos_internal_geo_set_constructor Internal constructor functions
 * @ingroup meos_internal_geo_set
 * @brief Internal constructor functions for geometry sets
 *
 * @defgroup meos_internal_geo_set_conversion Internal conversion functions
 * @ingroup meos_internal_geo_set
 * @brief Internal conversion functions for geometry sets
 *
 * @defgroup meos_internal_geo_set_accessor Internal accessor functions
 * @ingroup meos_internal_geo_set
 * @brief Internal accessor functions for geometry sets
 *
 * @defgroup meos_internal_geo_set_setops Internal set operations
 * @ingroup meos_internal_geo_set
 * @brief Internal set operations for geometry sets
 */

/*****************************************************************************/

/**
 * @defgroup meos_internal_geo_box_inout Internal input and output functions
 * @ingroup meos_internal_geo_box
 * @brief Internal input and output functions for spatiotemporal boxes
 *
 * @defgroup meos_internal_geo_box_constructor Internal constructor functions
 * @ingroup meos_internal_geo_box
 * @brief Internal constructor functions for spatiotemporal boxes
 *
 * @defgroup meos_internal_geo_box_conversion Internal conversion functions
 * @ingroup meos_internal_geo_box
 * @brief Internal conversion functions for spatiotemporal boxes
 *
 * @defgroup meos_internal_geo_box_accessor Internal accessor functions
 * @ingroup meos_internal_geo_box
 * @brief Internal accessor functions for spatiotemporal boxes
 *
 * @defgroup meos_internal_geo_box_transf Internal transformation functions
 * @ingroup meos_internal_geo_box
 * @brief Internal transformation functions for spatiotemporal boxes
 *
 * @defgroup meos_internal_geo_box_srid Internal spatial reference system functions
 * @ingroup meos_internal_geo_box
 * @brief Internal spatial reference system functions for spatiotemporal boxes
 *
 * @defgroup meos_internal_geo_box_topo Internal topological functions
 * @ingroup meos_internal_geo_box
 * @brief Internal topological functions for spatiotemporal boxes
 *
 * @defgroup meos_internal_geo_box_pos Internal position functions
 * @ingroup meos_internal_geo_box
 * @brief Internal position functions for spatiotemporal boxes
 *
 * @defgroup meos_internal_geo_box_set Internal set functions
 * @ingroup meos_internal_geo_box
 * @brief Internal set functions for spatiotemporal boxes
 *
 * @defgroup meos_internal_geo_box_comp Internal comparison functions
 * @ingroup meos_internal_geo_box
 * @brief Internal comparison functions for spatiotemporal boxes
 */

/*****************************************************************************/
