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
 * @brief Planar graph library functions
 */

#ifndef __PLANAR_GRAPH_H__
#define __PLANAR_GRAPH_H__

#include <postgres.h>
#include <liblwgeom.h>

#include "temporal/doublen.h"

/*****************************************************************************
 * Struct definitions
 *****************************************************************************/

/* Vertex types */

typedef struct {
  size_t count;
  size_t size;
  size_t *arr;
} size_t_array;

typedef struct {
  double2 pos;
  size_t_array *neighbors;
} Vertex;

typedef struct {
  size_t count;
  size_t size;
  Vertex **arr;
} Vertex_array;

/* Segment types */

typedef struct {
  double2 start;
  double2 end;
  size_t start_id;
  size_t end_id;
} Segment;

typedef struct {
  size_t count;
  size_t size;
  Segment *arr;
} Segment_array;

/* Graph */

typedef struct
{
  Vertex_array *vertices;
  Segment_array *segments;
} Graph;

/*****************************************************************************/

/* Segment functions */

extern Segment make_segment(double2 start, double2 end);

/* Graph functions */

extern void init_graph(Graph *g, size_t n);
extern bool add_segment_to_graph(Graph *g, Segment seg);
extern POINTARRAY *get_cycle_from_graph(Graph *g);
extern void free_graph(Graph *g);

/*****************************************************************************/

#endif /* __PLANAR_GRAPH_H__ */
