/*****************************************************************************
 *
 * This MobilityDB code is provided under The PostgreSQL License.
 * Copyright (c) 2016-2024, Université libre de Bruxelles and MobilityDB
 * contributors
 *
 * MobilityDB includes portions of PostGIS version 3 source code released
 * under the GNU General Public License (GPLv2 or later).
 * Copyright (c) 2001-2024, PostGIS contributors
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
 * @brief Sections for the documentation of the MobilityDB API: Circular buffers
 */

/*****************************************************************************
 * Definition of the modules of the MobilityDB API
 * These modules follow the sections of the MobilityDB documentation although
 * some subsections are merged into a single submodule
 *****************************************************************************/

/**
 * @defgroup mobilitydb_cbuffer_base Functions for static circular buffers
 * @ingroup mobilitydb_cbuffer
 * @brief Functions for static circular buffers
 *
 * @defgroup mobilitydb_cbuffer_set Functions for circular buffer sets
 * @ingroup mobilitydb_cbuffer
 * @brief Functions for circular buffer sets
 *
 * @defgroup mobilitydb_cbuffer_box Bounding box functions
 * @ingroup mobilitydb_cbuffer
 * @brief Bounding box functions for circular buffers
 *
 * @defgroup mobilitydb_cbuffer_inout Input and output functions
 * @ingroup mobilitydb_cbuffer
 * @brief Input and output functions for temporal circular buffers
 *
 * @defgroup mobilitydb_cbuffer_constructor Constructor functions
 * @ingroup mobilitydb_cbuffer
 * @brief Constructor functions for temporal circular buffers
 *
 * @defgroup mobilitydb_cbuffer_conversion Conversion functions
 * @ingroup mobilitydb_cbuffer
 * @brief Conversion functions for temporal circular buffers
 *
 * @defgroup mobilitydb_cbuffer_accessor Accessor functions
 * @ingroup mobilitydb_cbuffer
 * @brief Accessor functions for temporal circular buffers
 *
 * @defgroup mobilitydb_cbuffer_transf Transformation functions
 * @ingroup mobilitydb_cbuffer
 * @brief Transformation functions for temporal circular buffers
 *
 * @defgroup mobilitydb_cbuffer_restrict Restriction functions
 * @ingroup mobilitydb_cbuffer
 * @brief Restriction functions for temporal circular buffers
 *
 * @defgroup mobilitydb_cbuffer_modif Modification functions
 * @ingroup mobilitydb_cbuffer
 * @brief Modification functions for temporal circular buffers
 *
 * @defgroup mobilitydb_cbuffer_bbox Bounding box functions
 * @ingroup mobilitydb_cbuffer
 * @brief Bounding box functions for temporal circular buffers
 *
 *   @defgroup mobilitydb_cbuffer_bbox_topo Topological functions
 *   @ingroup mobilitydb_cbuffer_bbox
 *   @brief Topological functions for temporal circular buffers
 *
 *   @defgroup mobilitydb_cbuffer_bbox_pos Position functions
 *   @ingroup mobilitydb_cbuffer_bbox
 *   @brief Position functions for temporal circular buffers
 *
 * @defgroup mobilitydb_cbuffer_dist Distance functions
 * @ingroup mobilitydb_cbuffer
 * @brief Distance functions for temporal circular buffers
 *
 * @defgroup mobilitydb_cbuffer_comp Comparison functions
 * @ingroup mobilitydb_cbuffer
 * @brief Comparison functions for temporal circular buffers
 *
 *   @defgroup mobilitydb_cbuffer_comp_ever Ever and always comparison functions
 *   @ingroup mobilitydb_cbuffer_comp
 *   @brief Ever and always comparison functions for temporal circular buffers
 *
 *   @defgroup mobilitydb_cbuffer_comp_temp Temporal comparison functions
 *   @ingroup mobilitydb_cbuffer_comp
 *   @brief Comparison functions for temporal circular buffers
 *
 * @defgroup mobilitydb_cbuffer_srid Spatial reference system functions
 * @ingroup mobilitydb_cbuffer
 * @brief Spatial reference system functions for temporal circular bufferstemporal circular buffers
 *
 * @defgroup mobilitydb_cbuffer_rel Spatial relationship functions
 * @ingroup mobilitydb_cbuffer
 * @brief Spatial relationship functions for temporal circular buffers
 *
 *   @defgroup mobilitydb_cbuffer_rel_ever Ever and always spatial relationship functions
 *   @ingroup mobilitydb_cbuffer_rel
 *   @brief Ever and always spatial relationship functions for temporal circular buffers
 *
 *   @defgroup mobilitydb_cbuffer_rel_temp Temporal spatial relationship functions
 *   @ingroup mobilitydb_cbuffer_rel
 *   @brief Temporal spatial relationship functions for temporal circular buffers
 *
 * @defgroup mobilitydb_cbuffer_agg Aggregate functions
 * @ingroup mobilitydb_cbuffer
 * @brief Aggregate functions for temporal circular buffers
 *
 * @defgroup mobilitydb_cbuffer_analytics Analytics functions
 * @ingroup mobilitydb_cbuffer
 * @brief Analytics functions for temporal circular buffers
 *
 *   @defgroup mobilitydb_cbuffer_analytics_simplify Simplification functions
 *   @ingroup mobilitydb_cbuffer_analytics
 *   @brief Simplification functions for temporal circular buffers
 *
 *   @defgroup mobilitydb_cbuffer_analytics_reduction Reduction functions
 *   @ingroup mobilitydb_cbuffer_analytics
 *   @brief Reduction functions for temporal circular buffers
 *
 *   @defgroup mobilitydb_cbuffer_analytics_similarity Similarity functions
 *   @ingroup mobilitydb_cbuffer_analytics
 *   @brief Similarity functions for temporal circular buffers
 *
 *   @defgroup mobilitydb_cbuffer_analytics_tile Tile functions
 *   @ingroup mobilitydb_cbuffer_analytics
 *   @brief Tile functions for temporal circular buffers
 */

/*****************************************************************************/
/**
 * @defgroup mobilitydb_cbuffer_base_inout Input and output functions
 * @ingroup mobilitydb_cbuffer_base
 * @brief Input and output functions for static circular buffers
 *
 * @defgroup mobilitydb_cbuffer_base_constructor Constructor functions
 * @ingroup mobilitydb_cbuffer_base
 * @brief Constructor functions for static circular buffers
 *
 * @defgroup mobilitydb_cbuffer_base_conversion Conversion functions
 * @ingroup mobilitydb_cbuffer_base
 * @brief Conversion functions for static circular buffers
 *
 * @defgroup mobilitydb_cbuffer_base_accessor Accessor functions
 * @ingroup mobilitydb_cbuffer_base
 * @brief Accessor functions for static circular buffers
 *
 * @defgroup mobilitydb_cbuffer_base_transf Transformation functions
 * @ingroup mobilitydb_cbuffer_base
 * @brief Transformation functions for static circular buffers
 *
 * @defgroup mobilitydb_cbuffer_base_srid Spatial reference system functions
 * @ingroup mobilitydb_cbuffer_base
 * @brief Spatial reference system functions for static circular buffers
 *
 * @defgroup mobilitydb_cbuffer_base_comp Comparison functions
 * @ingroup mobilitydb_cbuffer_base
 * @brief Comparison functions for static circular buffers
 */

/*****************************************************************************/

