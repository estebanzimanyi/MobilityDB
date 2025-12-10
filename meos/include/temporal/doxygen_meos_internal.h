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
 * @brief MEOS Developer's Documentation: Internal API
 */

/*****************************************************************************
 * Sections of the MEOS internal API
 *****************************************************************************/

/**
 * @defgroup meos_internal_setspan Internal functions for set and span types
 * @ingroup meos_internal
 * @brief Internal functions for set and span types
 *
 * @defgroup meos_internal_box Internal functions for temporal boxes
 * @ingroup meos_internal
 * @brief Internal functions for temporal boxes
 *
 * @defgroup meos_internal_temporal Internal functions for temporal types
 * @ingroup meos_internal
 * @brief Internal functions for temporal types
 *
 * @defgroup meos_internal_geo Internal functions for temporal geometries
 * @ingroup meos_internal
 * @brief Internal functions for temporal geometries
 *
 * @defgroup meos_internal_cbuffer Internal functions for temporal circular buffers
 * @ingroup meos_internal
 * @brief Internal functions for temporal circular buffers
 *
 * @defgroup meos_internal_json Internal functions for temporal circular buffers
 * @ingroup meos_internal
 * @brief Internal functions for temporal circular buffers
 *
 * @defgroup meos_internal_npoint Internal functions for temporal network points
 * @ingroup meos_internal
 * @brief Internal functions for temporal network points
 *
 * @defgroup meos_internal_pose Internal functions for temporal poses
 * @ingroup meos_internal
 * @brief Internal functions for temporal poses
 *
 * @defgroup meos_internal_rgeo Internal functions for temporal rigid geometries
 * @ingroup meos_internal
 * @brief Internal functions for temporal rigid geometries
 */

/*****************************************************************************/

/**
 * @defgroup meos_internal_setspan_inout Internal input and output functions
 * @ingroup meos_internal_setspan
 * @brief Internal input and output functions for set and span types
 *
 * @defgroup meos_internal_setspan_constructor Internal constructor functions
 * @ingroup meos_internal_setspan
 * @brief Internal constructor functions for set and span types
 *
 * @defgroup meos_internal_setspan_conversion Internal conversion functions
 * @ingroup meos_internal_setspan
 * @brief Internal conversion functions for set and span types
 *
 * @defgroup meos_internal_setspan_accessor Internal accessor functions
 * @ingroup meos_internal_setspan
 * @brief Internal accessor functions for set and span types
 *
 * @defgroup meos_internal_setspan_transf Internal transformation functions
 * @ingroup meos_internal_setspan
 * @brief Internal transformation functions for set and span types
 *
 * @defgroup meos_internal_setspan_bbox Internal bounding box functions
 * @ingroup meos_internal_setspan
 * @brief Internal bounding box functions for set and span types
 *
 * @defgroup meos_internal_setspan_topo Internal topological functions
 * @ingroup meos_internal_setspan_bbox
 * @brief Internal topological functions for set and span types
 *
 * @defgroup meos_internal_setspan_pos Internal position functions
 * @ingroup meos_internal_setspan_bbox
 * @brief Internal position functions for set and span types
 *
 * @defgroup meos_internal_setspan_set Internal set functions
 * @ingroup meos_internal_setspan
 * @brief Internal set functions for set and span types
 *
 * @defgroup meos_internal_setspan_dist Internal distance functions
 * @ingroup meos_internal_setspan
 * @brief Internal distance functions for set and span types
 *
 * @defgroup meos_internal_setspan_agg Internal aggregate functions
 * @ingroup meos_internal_setspan
 * @brief Internal aggregate functions for set and span types
 */

/*****************************************************************************/

/**
 * @defgroup meos_internal_box_constructor Internal constructor functions
 * @ingroup meos_internal_box
 * @brief Internal constructor functions for box types
 *
 * @defgroup meos_internal_box_conversion Internal conversion functions
 * @ingroup meos_internal_box
 * @brief Internal conversion functions for box types
 *
 * @defgroup meos_internal_box_transf Internal transformation functions
 * @ingroup meos_internal_box
 * @brief Internal transformation functions for box types
 *
 * @defgroup meos_internal_box_set Internal set functions
 * @ingroup meos_internal_box
 * @brief Internal set functions for box types
  */

/*****************************************************************************/

/**
 * @defgroup meos_internal_temporal_inout Internal input and output functions
 * @ingroup meos_internal_temporal
 * @brief Internal input and output functions for temporal types
 *
 * @defgroup meos_internal_temporal_constructor Internal constructor functions
 * @ingroup meos_internal_temporal
 * @brief Internal constructor functions for temporal types
 *
 * @defgroup meos_internal_temporal_conversion Internal conversion functions
 * @ingroup meos_internal_temporal
 * @brief Internal conversion functions for temporal types
 *
 * @defgroup meos_internal_temporal_accessor Internal accessor functions
 * @ingroup meos_internal_temporal
 * @brief Internal accessor functions for temporal types
 *
 * @defgroup meos_internal_temporal_transf Internal transformation functions
 * @ingroup meos_internal_temporal
 * @brief Internal transformation functions for temporal types
 *
 * @defgroup meos_internal_temporal_modif Internal modification functions
 * @ingroup meos_internal_temporal
 * @brief Internal modification functions for temporal types
 *
 * @defgroup meos_internal_temporal_restrict Internal restriction functions
 * @ingroup meos_internal_temporal
 * @brief Internal restriction functions for temporal types
 *
 * @defgroup meos_internal_temporal_comp Internal comparison functions
 * @ingroup meos_internal_temporal
 * @brief Internal comparison functions for temporal types
 *
 * @defgroup meos_internal_temporal_bbox Internal bounding box functions
 * @ingroup meos_internal_temporal
 * @brief Internal bounding box functions for temporal types
 *
 * @defgroup meos_internal_temporal_math Internal mathematical functions
 * @ingroup meos_internal_temporal
 * @brief Internal mathematical functions for temporal types
 *
 * @defgroup meos_internal_temporal_dist Internal distance functions
 * @ingroup meos_internal_temporal
 * @brief Internal distance functions for temporal types
 *
 * @defgroup meos_internal_temporal_comp_trad Internal traditional comparison functions
 * @ingroup meos_internal_temporal_comp
 * @brief Internal tranditional comparison functions for temporal types
 *
 * @defgroup meos_internal_temporal_comp_ever Internal ever/always comparison functions
 * @ingroup meos_internal_temporal_comp
 * @brief Internal ever and always comparison functions for temporal types
 *
 * @defgroup meos_internal_temporal_comp_temp Internal temporal comparison functions
 * @ingroup meos_internal_temporal_comp
 * @brief Internal temporal comparison functions for temporal types
 *
 * @defgroup meos_internal_temporal_spatial Internal spatial functions
 * @ingroup meos_internal_temporal
 * @brief Internal spatial functions for temporal geos
 *
 * @defgroup meos_internal_temporal_spatial_accessor Internal spatial accessor functions
 * @ingroup meos_internal_temporal_spatial
 * @brief Internal spatial accessor functions for temporal geos
 *
 * @defgroup meos_internal_temporal_spatial_transf Internal spatial transformation functions
 * @ingroup meos_internal_temporal_spatial
 * @brief Internal spatial transformation functions for temporal geos
 *
 * @defgroup meos_internal_temporal_agg Internal aggregate functions
 * @ingroup meos_internal_temporal
 * @brief Internal aggregate functions for temporal types
 */

/*****************************************************************************/
