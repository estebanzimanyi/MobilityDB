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
 * @defgroup meos_rgeo_base Functions for static geometries
 * @ingroup meos_rgeo
 * @brief Functions for static geometries
 *
 * @defgroup meos_rgeo_set Functions for spatial sets
 * @ingroup meos_rgeo
 * @brief Functions for spatial sets
 *
 * @defgroup meos_rgeo_box Functions for spatiotemporal boxes
 * @ingroup meos_rgeo
 * @brief Functions for spatiotemporal boxes
 *
 * @defgroup meos_rgeo_inout Input and output functions
 * @ingroup meos_rgeo
 * @brief Input and output functions for temporal rigid geometries
 *
 * @defgroup meos_rgeo_constructor Constructor functions
 * @ingroup meos_rgeo
 * @brief Constructor functions for temporal rigid geometries
 *
 * @defgroup meos_rgeo_conversion Conversion functions
 * @ingroup meos_rgeo
 * @brief Conversion functions for temporal rigid geometries
 *
 * @defgroup meos_rgeo_accessor Accessor functions
 * @ingroup meos_rgeo
 * @brief Accessor functions for temporal rigid geometries
 *
 * @defgroup meos_rgeo_transf Transformation functions
 * @ingroup meos_rgeo
 * @brief Transformation functions for temporal rigid geometries
 *
 * @defgroup meos_rgeo_restrict Restriction functions
 * @ingroup meos_rgeo
 * @brief Restriction functions for temporal rigid geometries
 *
 * @defgroup meos_rgeo_comp Comparison functions
 * @ingroup meos_rgeo
 * @brief Comparison functions for temporal rigid geometries
 *
 *   @defgroup meos_rgeo_comp_ever Ever and always comparison functions
 *   @ingroup meos_rgeo_comp
 *   @brief Ever and always comparison functions for temporal rigid geometries
 *
 *   @defgroup meos_rgeo_comp_temp Temporal comparison functions
 *   @ingroup meos_rgeo_comp
 *   @brief Temporal comparison functions for temporal rigid geometries
 *
 * @defgroup meos_rgeo_bbox Bounding box functions
 * @ingroup meos_rgeo
 * @brief Bounding box functions for temporal rigid geometries
 *
 *   @defgroup meos_rgeo_bbox_split Split functions
 *   @ingroup meos_rgeo_bbox
 *   @brief Split functions for temporal rigid geometries
 *
 *   @defgroup meos_rgeo_bbox_topo Topological functions
 *   @ingroup meos_rgeo_bbox
 *   @brief Topological functions for temporal rigid geometries
 *
 *   @defgroup meos_rgeo_bbox_pos Position functions
 *   @ingroup meos_rgeo_bbox
 *   @brief Position functions for temporal rigid geometries
 *
 * @defgroup meos_rgeo_distance Distance functions
 * @ingroup meos_rgeo
 * @brief Distance functions for temporal rigid geometries
 *
 * @defgroup meos_rgeo_srid Spatial reference system functions
 * @ingroup meos_rgeo
 * @brief Spatial reference system functions for temporal rigid geometries
 *
 * @defgroup meos_rgeo_rel Spatial relationship functions
 * @ingroup meos_rgeo
 * @brief Spatial relationship functions for temporal rigid geometries
 *
 *   @defgroup meos_rgeo_rel_ever Ever/always relationship functions
 *   @ingroup meos_rgeo_rel
 *   @brief Ever/always relationship functions for temporal rigid geometries
 *
 *   @defgroup meos_rgeo_rel_temp Temporal relationship functions
 *   @ingroup meos_rgeo_rel
 *   @brief Temporal relationship functions for temporal rigid geometries
 *
 * @defgroup meos_rgeo_agg Aggregate functions
 * @ingroup meos_rgeo
 * @brief Aggregate functions for temporal rigid geometries
 *
 * @defgroup meos_rgeo_tile Tile functions
 * @ingroup meos_rgeo
 * @brief Tile functions for temporal rigid geometries
 */

/*****************************************************************************/

/**
 * @defgroup meos_rgeo_base_inout Input and output functions
 * @ingroup meos_rgeo_base
 * @brief Input and output functions for static geometries
 *
 * @defgroup meos_rgeo_base_constructor Constructor functions
 * @ingroup meos_rgeo_base
 * @brief Constructor functions for static geometries
 *
 * @defgroup meos_rgeo_base_conversion Conversion functions
 * @ingroup meos_rgeo_base
 * @brief Conversion functions for static geometries
 *
 * @defgroup meos_rgeo_base_accessor Accessor functions
 * @ingroup meos_rgeo_base
 * @brief Accessor functions for static geometries
 *
 * @defgroup meos_rgeo_base_transf Transformation functions
 * @ingroup meos_rgeo_base
 * @brief Transformation functions for static geometries
 *
 * @defgroup meos_rgeo_base_srid Spatial reference system functions
 * @ingroup meos_rgeo_base
 * @brief Spatial reference system functions for temporal geos
 *
 * @defgroup meos_rgeo_base_spatial Spatial processing functions
 * @ingroup meos_rgeo_base
 * @brief Spatial processing functions for static geometries
 *
 * @defgroup meos_rgeo_base_rel Spatial relationship functions
 * @ingroup meos_rgeo_base
 * @brief Spatial relationship functions for temporal geos
 *
 * @defgroup meos_rgeo_base_bbox Bounding box functions
 * @ingroup meos_rgeo_base
 * @brief Bounding box functions for static geometries
 *
 * @defgroup meos_rgeo_base_distance Distance functions
 * @ingroup meos_rgeo_base
 * @brief Distance functions for static geometries
 *
 * @defgroup meos_rgeo_base_comp Comparison functions
 * @ingroup meos_rgeo_base
 * @brief Comparison functions for static geometries
 */

/*****************************************************************************/

/**
 * @defgroup meos_rgeo_set_inout Input and output functions
 * @ingroup meos_rgeo_set
 * @brief Input and output functions for spatial sets
 *
 * @defgroup meos_rgeo_set_constructor Constructor functions
 * @ingroup meos_rgeo_set
 * @brief Constructor functions for spatial sets
 *
 * @defgroup meos_rgeo_set_conversion Conversion functions
 * @ingroup meos_rgeo_set
 * @brief Conversion functions for spatial sets
 *
 * @defgroup meos_rgeo_set_accessor Accessor functions
 * @ingroup meos_rgeo_set
 * @brief Accessor functions for spatial sets
 *
 * @defgroup meos_rgeo_set_srid Spatial reference system functions
 * @ingroup meos_rgeo_set
 * @brief Spatial reference system functions for spatial sets
 *
 * @defgroup meos_rgeo_set_setops Set operations
 * @ingroup meos_rgeo_set
 * @brief Set operations for geometry sets
 */

/*****************************************************************************/

/**
 * @defgroup meos_rgeo_box_inout Input and output functions
 * @ingroup meos_rgeo_box
 * @brief Input and output functions for spatiotemporal boxes
 *
 * @defgroup meos_rgeo_box_constructor Constructor functions
 * @ingroup meos_rgeo_box
 * @brief Constructor functions for spatiotemporal boxes
 *
 * @defgroup meos_rgeo_box_conversion Conversion functions
 * @ingroup meos_rgeo_box
 * @brief Conversion functions for spatiotemporal boxes
 *
 * @defgroup meos_rgeo_box_accessor Accessor functions
 * @ingroup meos_rgeo_box
 * @brief Accessor functions for spatiotemporal boxes
 *
 * @defgroup meos_rgeo_box_transf Transformation functions
 * @ingroup meos_rgeo_box
 * @brief Transformation functions for spatiotemporal boxes
 *
 * @defgroup meos_rgeo_box_srid Spatial reference system functions
 * @ingroup meos_rgeo_box
 * @brief Spatial reference system functions for spatiotemporal boxes
 *
 * @defgroup meos_rgeo_box_bbox Bounding box functions
 * @ingroup meos_rgeo_box
 * @brief Bounding box functions for spatiotemporal boxes
 *
 * @defgroup meos_rgeo_box_topo Topological functions
 * @ingroup meos_rgeo_box_bbox
 * @brief Topological functions for spatiotemporal boxes
 *
 * @defgroup meos_rgeo_box_pos Position functions
 * @ingroup meos_rgeo_box_bbox
 * @brief Position functions for spatiotemporal boxes
 *
 * @defgroup meos_rgeo_box_set Set functions
 * @ingroup meos_rgeo_box
 * @brief Set functions for spatiotemporal boxes
 *
 * @defgroup meos_rgeo_box_comp Comparison functions
 * @ingroup meos_rgeo_box
 * @brief Comparison functions for spatiotemporal boxes
 *
 * @defgroup meos_rgeo_box_index Index functions
 * @ingroup meos_rgeo_box
 * @brief In-memory RTree index for spatiotemporal boxes
 */

/*****************************************************************************/
