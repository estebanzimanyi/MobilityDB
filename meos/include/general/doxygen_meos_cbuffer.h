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
 * @defgroup meos_cbuffer_base Functions for static circular buffers
 * @ingroup meos_cbuffer
 * @brief Functions for static circular buffers
 *
 * @defgroup meos_cbuffer_set Functions for circular buffer sets
 * @ingroup meos_cbuffer
 * @brief Functions for circular buffer sets
 *
 * @defgroup meos_cbuffer_inout Input and output functions
 * @ingroup meos_cbuffer
 * @brief Input and output functions for temporal circular buffers
 *
 * @defgroup meos_cbuffer_constructor Constructor functions
 * @ingroup meos_cbuffer
 * @brief Constructor functions for temporal circular buffers
 *
 * @defgroup meos_cbuffer_conversion Conversion functions
 * @ingroup meos_cbuffer
 * @brief Conversion functions for temporal circular buffers
 *
 * @defgroup meos_cbuffer_accessor Accessor functions
 * @ingroup meos_cbuffer
 * @brief Accessor functions for temporal circular buffers
 *
 * @defgroup meos_cbuffer_transf Transformation functions
 * @ingroup meos_cbuffer
 * @brief Transformation functions for temporal circular buffers
 *
 * @defgroup meos_cbuffer_modif Modification functions
 * @ingroup meos_cbuffer
 * @brief Modification functions for temporal circular buffers
 *
 * @defgroup meos_cbuffer_restrict Restriction functions
 * @ingroup meos_cbuffer
 * @brief Restriction functions for temporal circular buffers
 *
 * @defgroup meos_cbuffer_comp Comparison functions
 * @ingroup meos_cbuffer
 * @brief Comparison functions for temporal circular buffers
 *
 * @defgroup meos_cbuffer_comp_ever Ever and always comparison functions
 * @ingroup meos_cbuffer_comp
 * @brief Ever and always comparison functions for temporal circular buffers
 *
 * @defgroup meos_cbuffer_comp_temp Temporal comparison functions
 * @ingroup meos_cbuffer_comp
 * @brief Temporal comparison functions for temporal circular buffers
 *
 * @defgroup meos_cbuffer_bbox Bounding box functions
 * @ingroup meos_cbuffer
 * @brief Bounding box functions for temporal circular buffers
 *
 * @defgroup meos_cbuffer_bbox_topo Topological functions
 * @ingroup meos_cbuffer_bbox
 * @brief Topological functions for temporal circular buffers
 *
 * @defgroup meos_cbuffer_bbox_pos Position functions
 * @ingroup meos_cbuffer_bbox
 * @brief Position functions for temporal circular buffers
 *
 * @defgroup meos_cbuffer_dist Distance functions
 * @ingroup meos_cbuffer
 * @brief Distance functions for temporal circular buffers
 *
 * @defgroup meos_cbuffer_agg Aggregate functions
 * @ingroup meos_cbuffer
 * @brief Aggregate functions for temporal circular buffers
 *
 * @defgroup meos_cbuffer_analytics Analytics functions
 * @ingroup meos_cbuffer
 * @brief Analytics functions for temporal circular buffers
 *
 * @defgroup meos_cbuffer_analytics_simplify Simplification functions
 * @ingroup meos_cbuffer_analytics
 * @brief Simplification functions for temporal circular buffers
 *
 * @defgroup meos_cbuffer_analytics_reduction Reduction functions
 * @ingroup meos_cbuffer_analytics
 * @brief Reduction functions for temporal circular buffers
 *
 * @defgroup meos_cbuffer_analytics_similarity Similarity functions
 * @ingroup meos_cbuffer_analytics
 * @brief Similarity functions for temporal circular buffers
 *
 * @defgroup meos_cbuffer_analytics_tile Tile functions
 * @ingroup meos_cbuffer_analytics
 * @brief Tile functions for temporal circular buffers
 */

/*****************************************************************************/

/**
 * @defgroup meos_cbuffer_base_inout Input and output functions
 * @ingroup meos_cbuffer_base
 * @brief Input and output functions for static circular buffers
 *
 * @defgroup meos_cbuffer_base_constructor Constructor functions
 * @ingroup meos_cbuffer_base
 * @brief Constructor functions for static circular buffers
 *
 * @defgroup meos_cbuffer_base_conversion Conversion functions
 * @ingroup meos_cbuffer_base
 * @brief Conversion functions for static circular buffers
 *
 * @defgroup meos_cbuffer_base_accessor Accessor functions
 * @ingroup meos_cbuffer_base
 * @brief Accessor functions for static circular buffers
 *
 * @defgroup meos_cbuffer_base_transf Transformation functions
 * @ingroup meos_cbuffer_base
 * @brief Transformation functions for static circular buffers
 *
 * @defgroup meos_cbuffer_base_srid Spatial reference system functions
 * @ingroup meos_cbuffer_base
 * @brief Spatial reference system functions for static circular buffers
 *
 * @defgroup meos_cbuffer_base_comp Comparison functions
 * @ingroup meos_cbuffer_base
 * @brief Comparison functions for static circular buffers
 */

/*****************************************************************************/

/**
 * @defgroup meos_cbuffer_set_inout Input and output functions
 * @ingroup meos_cbuffer_set
 * @brief Input and output functions for circular buffer sets
 *
 * @defgroup meos_cbuffer_set_constructor Constructor functions
 * @ingroup meos_cbuffer_set
 * @brief Constructor functions for circular buffer sets
 *
 * @defgroup meos_cbuffer_set_conversion Conversion functions
 * @ingroup meos_cbuffer_set
 * @brief Conversion functions for circular buffer sets
 *
 * @defgroup meos_cbuffer_set_accessor Accessor functions
 * @ingroup meos_cbuffer_set
 * @brief Accessor functions for circular buffer sets
 *
 * @defgroup meos_cbuffer_set_transf Transformation functions
 * @ingroup meos_cbuffer_set
 * @brief Transformation functions for circular buffer sets
 *
 * @defgroup meos_cbuffer_set_setops Set operations
 * @ingroup meos_cbuffer_set
 * @brief Set operations for circular buffer sets
 */

/*****************************************************************************/
