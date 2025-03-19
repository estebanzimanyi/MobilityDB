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
 * @defgroup meos_pose_base Functions for static poses
 * @ingroup meos_pose
 * @brief Functions for static poses
 *
 * @defgroup meos_pose_set Functions for pose sets
 * @ingroup meos_pose
 * @brief Functions for pose sets
 *
 * @defgroup meos_pose_inout Input and output functions
 * @ingroup meos_pose
 * @brief Input and output functions for temporal poses
 *
 * @defgroup meos_pose_constructor Constructor functions
 * @ingroup meos_pose
 * @brief Constructor functions for temporal poses
 *
 * @defgroup meos_pose_conversion Conversion functions
 * @ingroup meos_pose
 * @brief Conversion functions for temporal poses
 *
 * @defgroup meos_pose_accessor Accessor functions
 * @ingroup meos_pose
 * @brief Accessor functions for temporal poses
 *
 * @defgroup meos_pose_transf Transformation functions
 * @ingroup meos_pose
 * @brief Transformation functions for temporal poses
 *
 * @defgroup meos_pose_modif Modification functions
 * @ingroup meos_pose
 * @brief Modification functions for temporal poses
 *
 * @defgroup meos_pose_restrict Restriction functions
 * @ingroup meos_pose
 * @brief Restriction functions for temporal poses
 *
 * @defgroup meos_pose_comp Comparison functions
 * @ingroup meos_pose
 * @brief Comparison functions for temporal poses
 *
 * @defgroup meos_pose_comp_ever Ever and always comparison functions
 * @ingroup meos_pose_comp
 * @brief Ever and always comparison functions for temporal poses
 *
 * @defgroup meos_pose_comp_temp Temporal comparison functions
 * @ingroup meos_pose_comp
 * @brief Temporal comparison functions for temporal poses
 *
 * @defgroup meos_pose_bbox Bounding box functions
 * @ingroup meos_pose
 * @brief Bounding box functions for temporal poses
 *
 * @defgroup meos_pose_bbox_topo Topological functions
 * @ingroup meos_pose_bbox
 * @brief Topological functions for temporal poses
 *
 * @defgroup meos_pose_bbox_pos Position functions
 * @ingroup meos_pose_bbox
 * @brief Position functions for temporal poses
 *
 * @defgroup meos_pose_dist Distance functions
 * @ingroup meos_pose
 * @brief Distance functions for temporal poses
 *
 * @defgroup meos_pose_agg Aggregate functions
 * @ingroup meos_pose
 * @brief Aggregate functions for temporal poses
 *
 * @defgroup meos_pose_analytics Analytics functions
 * @ingroup meos_pose
 * @brief Analytics functions for temporal poses
 *
 * @defgroup meos_pose_analytics_simplify Simplification functions
 * @ingroup meos_pose_analytics
 * @brief Simplification functions for temporal poses
 *
 * @defgroup meos_pose_analytics_reduction Reduction functions
 * @ingroup meos_pose_analytics
 * @brief Reduction functions for temporal poses
 *
 * @defgroup meos_pose_analytics_similarity Similarity functions
 * @ingroup meos_pose_analytics
 * @brief Similarity functions for temporal poses
 *
 * @defgroup meos_pose_analytics_tile Tile functions
 * @ingroup meos_pose_analytics
 * @brief Tile functions for temporal poses
 */

/*****************************************************************************/

/**
 * @defgroup meos_pose_base_inout Input and output functions
 * @ingroup meos_pose_base
 * @brief Input and output functions for static poses
 *
 * @defgroup meos_pose_base_constructor Constructor functions
 * @ingroup meos_pose_base
 * @brief Constructor functions for static poses
 *
 * @defgroup meos_pose_base_conversion Conversion functions
 * @ingroup meos_pose_base
 * @brief Conversion functions for static poses
 *
 * @defgroup meos_pose_base_accessor Accessor functions
 * @ingroup meos_pose_base
 * @brief Accessor functions for static poses
 *
 * @defgroup meos_pose_base_transf Transformation functions
 * @ingroup meos_pose_base
 * @brief Transformation functions for static poses
 *
 * @defgroup meos_pose_base_srid Spatial reference system functions
 * @ingroup meos_pose_base
 * @brief Spatial reference system functions for static poses
 *
 * @defgroup meos_pose_base_comp Comparison functions
 * @ingroup meos_pose_base
 * @brief Comparison functions for static poses
 */

/*****************************************************************************/

/**
 * @defgroup meos_pose_set_inout Input and output functions
 * @ingroup meos_pose_set
 * @brief Input and output functions for pose sets
 *
 * @defgroup meos_pose_set_constructor Constructor functions
 * @ingroup meos_pose_set
 * @brief Constructor functions for pose sets
 *
 * @defgroup meos_pose_set_conversion Conversion functions
 * @ingroup meos_pose_set
 * @brief Conversion functions for pose sets
 *
 * @defgroup meos_pose_set_accessor Accessor functions
 * @ingroup meos_pose_set
 * @brief Accessor functions for pose sets
 *
 * @defgroup meos_pose_set_transf Transformation functions
 * @ingroup meos_pose_set
 * @brief Transformation functions for pose sets
 *
 * @defgroup meos_pose_set_setops Set operations
 * @ingroup meos_pose_set
 * @brief Set operations for pose sets
 */

/*****************************************************************************/
