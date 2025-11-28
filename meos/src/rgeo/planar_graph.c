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
 * @brief Planar graph library functions.
 */

#include "rgeo/planar_graph.h"

#include <math.h>
#include <float.h>

#include "temporal/doublen.h"
#include "temporal/temporal.h"

/*****************************************************************************
 * size_t array functions
 *****************************************************************************/

/**
 * @brief Initialize an array of size_t
 * @param[in] sta Array of size_t
 * @param[in] n Number of elements in the array
 */
static void
init_size_t_array(size_t_array *sta, size_t n)
{
  sta->arr = palloc0(sizeof(size_t) * n);
  sta->count = 0;
  sta->size = n;
  return;
}

/**
 * @brief Free an array of size_t
 * @param[in] sta Array of size_t
 */
static void
free_size_t_array(size_t_array *sta)
{
  pfree(sta->arr);
  return;
}

/**
 * @brief Append a value to an array of size_t
 * @param[in] sta Array of size_t
 * @param[in] v Value
 */
static bool
append_size_t(size_t_array *sta, size_t v)
{
  if (sta->count == sta->size)
  {
    sta->size *= 2;
    size_t *new_arr = repalloc(sta->arr, sizeof(size_t) * sta->size);
    if (new_arr == NULL)
    {
      meos_error(ERROR, MEOS_ERR_INTERNAL_ERROR, "Not enough memory");
      return false;
    }
    else
      sta->arr = new_arr;
  }
  sta->arr[sta->count++] = v;
  return true;
}

/*****************************************************************************
 * Vertex functions
 *****************************************************************************/

/**
 * @brief Initialize a vertex
 * @param[in] v Vertex
 * @param[in] pos Position
 */
static void
vertex_init(Vertex *v, double2 pos)
{
  v->pos = pos;
  v->neighbors = palloc0(sizeof(size_t_array));
  init_size_t_array(v->neighbors, (size_t) 3);
  return;
}

/**
 * @brief Destroy a vertex
 * @param[in] v Vertex
 */
static void
vertex_destroy(Vertex *v)
{
  free_size_t_array(v->neighbors);
  pfree(v->neighbors);
  return;
}

/**
 * @brief Add a neighbor to a vertex
 * @param[in] v Vertex
 * @param[in] neighbor Neighbor
 */
bool
vertex_add_neighbor(Vertex *v, size_t neighbor)
{
  for (size_t i = 0; i < v->neighbors->count; ++i)
  {
    if (v->neighbors->arr[i] == neighbor)
      return false;
  }
  if (! append_size_t(v->neighbors, neighbor))
    return false;
  return true;
}

/**
 * @brief Initialize a vertex array
 * @param[in] va Vertex array
 * @param[in] n Number of elements in the array
 */
static void
init_vertex_array(Vertex_array *va, size_t n)
{
  va->arr = palloc0(sizeof(Vertex *) * n);
  va->count = 0;
  va->size = n;
  return;
}

/**
 * @brief Free a vertex array
 * @param[in] va Vertex array
 */
static void
free_vertex_array(Vertex_array *va)
{
  for (size_t i = 0; i < va->count; ++i)
  {
    vertex_destroy(va->arr[i]);
    pfree(va->arr[i]);
  }
  pfree(va->arr);
  return;
}

/**
 * @brief Apend a vertex to a vertex array
 * @param[in] va Vertex array
 * @param[in] v Vertex
 */
static bool
append_vertex(Vertex_array *va, Vertex *v)
{
  if (va->count == va->size)
  {
    va->size *= 2;
    Vertex **new_arr = repalloc(va->arr, sizeof(Vertex *) * va->size);
    if (new_arr == NULL)
    {
      meos_error(ERROR, MEOS_ERR_INTERNAL_ERROR, "Not enough memory");
      return false;
    }
    else
      va->arr = new_arr;
  }
  va->arr[va->count++] = v;
  return true;
}

/**
 * @brief Return ther vertex index from the position
 * @param[in] va Vertex array
 * @param[in] pos Position
 * @return On error return 0
 */
static size_t
get_vertex_index_from_pos(Vertex_array *va, double2 pos)
{
  for (size_t i = 0; i < va->count; ++i)
  {
    if (vec2_eq(va->arr[i]->pos, pos))
      return i;
  }
  size_t result = va->count;
  Vertex *v = palloc0(sizeof(Vertex));
  vertex_init(v, pos);
  if (! append_vertex(va, v))
    return 0;
  return result;
}

/*****************************************************************************
 * Segment functions
 *****************************************************************************/

/**
 * @brief Return a new segment constructed from the arguments
 * @param[in] start,end Bounds of the segment
 */
Segment
make_segment(double2 start, double2 end)
{
  Segment seg;
  seg.start = start;
  seg.end = end;
  return seg;
}

/**
 * @brief Return a new segment constructed from the arguments
 * @param[in] start,end Bounds of the segment
 * @param[in] start_id, end_id Identifiers of the nodes
 */
static Segment
make_segment_with_ids(double2 start, double2 end, size_t start_id,
  size_t end_id)
{
  Segment seg;
  seg.start = start;
  seg.end = end;
  seg.start_id = start_id;
  seg.end_id = end_id;
  return seg;
}

/**
 * @brief Return the length of a segment
 * @param[in] seg Segment
 */
static double
segment_length(Segment seg)
{
  return vec2_dist(seg.start, seg.end);
}

/**
 * @brief Return the normal of a segment
 * @param[in] seg Segment
 */
static double2
segment_normal(Segment seg)
{
  double a = seg.end.a - seg.start.a;
  double b = seg.end.b - seg.start.b;
  return vec2_normalize((double2) {b, -a});
}

/**
 * @brief Return the segment initialized with the arguments
 * @param[in] seg Segment
 * @param[in] start,end Start and end vertices
 * @param[in] start_id,end_id Identifiers of the start and end vertices
 */
static void
segment_set(Segment *seg, double2 start, double2 end, size_t start_id,
  size_t end_id)
{
  seg->start = start;
  seg->end = end;
  seg->start_id = start_id;
  seg->end_id = end_id;
  return;
}

/**
 * @brief Return true if a point is in a segment
 * @param[in] p Point
 * @param[in] seg Segment
 */
static bool
point_on_segment(double2 p, Segment seg)
{
  double t = (p.a - seg.start.a) / (seg.end.a - seg.start.a);
  double b = seg.start.b + (seg.end.b - seg.start.b) * t;
  if (t < MEOS_EPSILON || 1 - t < MEOS_EPSILON)
    return false;
  else
    return fabs(p.b - b) < MEOS_EPSILON;
  return true;
}

/**
 * @brief Return true if a two segments intersect
 * @param[in] seg1,seg2 Segments
 * @param[out] inter1,inter2 Intersections found
 */
static bool
segments_compute_intersections(Segment seg1, Segment seg2, double2 *inter1,
  double2 *inter2)
{
  double2 normal1 = segment_normal(seg1);
  double2 normal2 = segment_normal(seg2);
  Segment seg_12;
  if (!vec2_eq(seg1.start, seg2.start))
    seg_12 = make_segment(seg1.start, seg2.start);
  else
    seg_12 = make_segment(seg1.start, seg2.end);
  double2 normal_12 = segment_normal(seg_12);
  if (fabs(vec2_dot(normal1, normal2) - 1) < MEOS_EPSILON ||
    fabs(vec2_dot(normal1, normal2) + 1) < MEOS_EPSILON)
  {
    if (fabs(vec2_dot(normal1, normal_12) - 1) < MEOS_EPSILON ||
      fabs(vec2_dot(normal1, normal_12) + 1) < MEOS_EPSILON)
    {
      bool s1_i2 = point_on_segment(seg1.start, seg2);
      bool e1_i2 = point_on_segment(seg1.end, seg2);
      bool s2_i1 = point_on_segment(seg2.start, seg1);
      bool e2_i1 = point_on_segment(seg2.end, seg1);
      if ((! s1_i2 && ! e1_i2) || (! s2_i1 && ! e2_i1))
        return false;
      else
      {
        *inter1 = s2_i1 ? seg2.start : seg2.end;
        *inter2 = s1_i2 ? seg1.start : seg1.end;
        return true;
      }
    }
    else
      return false;
  }
  else
  {
    double2 s1 = seg1.start;
    double2 e1 = seg1.end;
    double2 s2 = seg2.start;
    double2 e2 = seg2.end;
    double denom = (e1.a - s1.a) * (e2.b - s2.b) - (e1.b - s1.b) * (e2.a - s2.a);
    double t1 = ((s1.b - s2.b) * (e2.a - s2.a) - (s1.a - s2.a) * (e2.b - s2.b)) / denom;
    double t2 = ((s1.b - s2.b) * (e1.a - s1.a) - (s1.a - s2.a) * (e1.b - s1.b)) / denom;
    if (t1 < MEOS_EPSILON || 1 - t1 < MEOS_EPSILON  || t2 < MEOS_EPSILON || 1 - t2 < MEOS_EPSILON)
      return false;
    else
    {
      double2 p;
      p.a = s1.a + (e1.a - s1.a) * t1;
      p.b = s1.b + (e1.b - s1.b) * t1;
      if (vec2_dist(p, s1) < MEOS_EPSILON)
      {
        *inter1 = s1;
        *inter2 = s1;
      }
      else if (vec2_dist(p, e1) < MEOS_EPSILON)
      {
        *inter1 = e1;
        *inter2 = e1;
      }
      else if (vec2_dist(p, s2) < MEOS_EPSILON)
      {
        *inter1 = s2;
        *inter2 = s2;
      }
      else if (vec2_dist(p, e2) < MEOS_EPSILON)
      {
        *inter1 = e2;
        *inter2 = e2;
      }
      else
      {
        *inter1 = p;
        *inter2 = p;
      }
      return true;
    }
  }
}

/**
 * @brief Set the identifiers of a segments
 * @param[in] seg Segment
 * @param[in] start_id,end_id Identifiers
 */
static void
segment_set_ids(Segment *seg, size_t start_id, size_t end_id)
{
  seg->start_id = start_id;
  seg->end_id = end_id;
  return;
}

/**
 * @brief Initialize a segment array
 * @param[in] sa Segment array
 * @param[in] n Number of segments in the array
 */
static void
init_segment_array(Segment_array *sa, size_t n)
{
  sa->arr = palloc0(sizeof(Segment) * n);
  sa->count = 0;
  sa->size = n;
  return;
}

/**
 * @brief Free a segment array
 * @param[in] sa Segment array
 */
static void
free_segment_array(Segment_array *sa)
{
  pfree(sa->arr);
  return;
}

/**
 * @brief Append a segment to a segment array
 * @param[in] sa Segment array
 * @param[in] seg Segment
 */
static bool
append_segment(Segment_array *sa, Segment seg)
{
  if (sa->count == sa->size)
  {
    sa->size *= 2;
    Segment *new_arr = repalloc(sa->arr, sizeof(Segment) * sa->size);
    if (new_arr == NULL)
    {
      meos_error(ERROR, MEOS_ERR_INTERNAL_ERROR, "Not enough memory");
      return false;
    }
    else
      sa->arr = new_arr;
  }
  sa->arr[sa->count++] = seg;
  return true;
}

/**
 * @brief Add a segment to a segment array
 * @param[in] sa Segment array
 * @param[in] seg Segment
 */
static bool
add_segment(Segment_array *sa, Segment seg)
{
  for (size_t i = 0; i < sa->count; ++i)
  {
    if (sa->arr[i].start_id == seg.start_id &&
      sa->arr[i].end_id == seg.end_id)
      return true;
  }
  return append_segment(sa, seg);
}

/*****************************************************************************
 * Graph functions
 *****************************************************************************/

/**
 * @brief Initialize a graph
 * @param[in] g Graph
 * @param[in] n Number of vertices
 */
void
init_graph(Graph *g, size_t n)
{
  g->vertices = palloc0(sizeof(Vertex_array));
  g->segments = palloc0(sizeof(Segment_array));
  init_vertex_array(g->vertices, n);
  init_segment_array(g->segments, n);
}

/**
 * @brief Free a graph
 * @param[in] g Graph
 */
void
free_graph(Graph *g)
{
  free_vertex_array(g->vertices);
  free_segment_array(g->segments);
  pfree(g->vertices);
  pfree(g->segments);
}

/**
 * @brief Add two neighbors to a graph
 * @param[in] g Graph
 * @param[in] id1,id2 Indentifiers
 */
static bool
graph_add_neighbors(Graph *g, size_t id1, size_t id2)
{
  if (id1 != id2)
  {
    if (! vertex_add_neighbor(g->vertices->arr[id1], id2))
      return false;
    if (! vertex_add_neighbor(g->vertices->arr[id2], id1))
      return false;
  }
  return true;
}

/**
 * @brief Add a segment to a graph
 * @param[in] g Graph
 * @param[in] seg segment
 */
bool
add_segment_to_graph(Graph *g, Segment seg)
{
  if (segment_length(seg) < MEOS_EPSILON)
    return true;
  size_t start_id = get_vertex_index_from_pos(g->vertices, seg.start);
  size_t end_id = get_vertex_index_from_pos(g->vertices, seg.end);
  segment_set_ids(&seg, start_id, end_id);
  if (! graph_add_neighbors(g, start_id, end_id))
    return false;
  Segment_array *new_segs = palloc0(sizeof(Segment_array));
  init_segment_array(new_segs, 1);
  if (! add_segment(new_segs, seg))
    return false;
  size_t_array seg_events;
  size_t_array vertex_events;
  init_size_t_array(&seg_events, g->segments->count);
  init_size_t_array(&vertex_events, g->segments->count);
  for (size_t i = 0; i < g->segments->count; ++i)
  {
    Segment seg_i = g->segments->arr[i];
    Segment_array *old_segs = new_segs;
    new_segs = palloc0(sizeof(Segment_array));
    init_segment_array(new_segs, old_segs->count);
    for (size_t j = 0; j < old_segs->count; ++j)
    {
      Segment segj = old_segs->arr[j];
      double2 interi;
      double2 interj;
      bool intersects = segments_compute_intersections(seg_i, segj, &interi,
        &interj);
      if (intersects)
      {
        size_t interi_id = get_vertex_index_from_pos(g->vertices, interi);
        size_t interj_id = get_vertex_index_from_pos(g->vertices, interj);
        if (! append_size_t(&seg_events, i))
          return false;
        if (! append_size_t(&vertex_events, interi_id))
          return false;
        if (interj_id != segj.start_id && interj_id != segj.end_id)
        {
          double2 pos = g->vertices->arr[interj_id]->pos;
          if (! add_segment(new_segs,
            make_segment_with_ids(segj.start, pos, segj.start_id, interj_id)))
              return false;
          if (! graph_add_neighbors(g, segj.start_id, interj_id))
            return false;
          if (! add_segment(new_segs,
            make_segment_with_ids(segj.end, pos, segj.end_id, interj_id)))
              return false;
          if (! graph_add_neighbors(g, segj.end_id, interj_id))
            return false;
        }
      }
      else
        if (! add_segment(new_segs, segj))
          return false;
    }
    free_segment_array(old_segs);
    pfree(old_segs);
  }
  for (size_t i = 0; i < seg_events.count; ++i)
  {
    size_t seg_id = seg_events.arr[i];
    Segment seg_i = g->segments->arr[seg_id];
    size_t vertex_id = vertex_events.arr[i];
    if (vertex_id != seg_i.start_id && vertex_id != seg_i.end_id)
    {
      double2 pos = g->vertices->arr[vertex_id]->pos;
      segment_set(&g->segments->arr[seg_id], seg_i.start, pos, seg_i.start_id,
        vertex_id);
      graph_add_neighbors(g, seg_i.start_id, vertex_id);
      if (! add_segment(g->segments,
        make_segment_with_ids(seg_i.end, pos, seg_i.end_id, vertex_id)))
          return false;
      graph_add_neighbors(g, seg_i.end_id, vertex_id);
    }
  }
  free_size_t_array(&seg_events);
  free_size_t_array(&vertex_events);
  bool ok = true;
  for (size_t i = 0; i < new_segs->count; ++i)
  {
    ok |= add_segment(g->segments, new_segs->arr[i]);
    if (! ok)
      return false;
  }
  free_segment_array(new_segs);
  pfree(new_segs);
  return true;
}

/**
 * @brief Get the start identifier of a graph
 * @param[in] g Graph
 */
static size_t
graph_get_start_id(Graph *g)
{
  size_t start_id = 0;
  Vertex *start_v = g->vertices->arr[0];
  for (size_t i = 1; i < g->vertices->count; ++i)
  {
    Vertex *v = g->vertices->arr[i];
    if (v->pos.a < start_v->pos.a ||
      (v->pos.a == start_v->pos.a && v->pos.b < start_v->pos.b))
    {
      start_id = i;
      start_v = v;
    }
  }
  return start_id;
}

/**
 * @brief Return the next identifier 
 * @param[in] g Graph
 * @param[in] curr_id,prev_id Current and previous identifiers
 * @param[in] n Number of vertices
 */
static size_t
graph_get_next_id(Graph *g, size_t curr_id, size_t prev_id, size_t n)
{
  size_t_array *neighbors = g->vertices->arr[curr_id]->neighbors;
  size_t next_id;
  double2 next_pos;
  double min_angle = 7;
  double2 curr_pos = g->vertices->arr[curr_id]->pos;
  double2 prev_pos = n == 0 ?
    (double2) {curr_pos.a - 1, curr_pos.b} :
    g->vertices->arr[prev_id]->pos;
  for (size_t i = 0; i < neighbors->count; ++i)
  {
    size_t neighbor_id = neighbors->arr[i];
    double2 neighbor_pos = g->vertices->arr[neighbor_id]->pos;
    double angle = vec2_angle(prev_pos, curr_pos, neighbor_pos);
    if (angle < MEOS_EPSILON)
      continue;
    if (angle  < min_angle || (fabs(angle - min_angle) < MEOS_EPSILON &&
      vec2_dist(curr_pos, neighbor_pos) < vec2_dist(curr_pos, next_pos)))
    {
      next_id = neighbor_id;
      next_pos = neighbor_pos;
      min_angle = angle;
    }
  }
  return next_id;
}

/**
 * @brief Return a cycle from a graph
 * @param[in] g Graph
 */
POINTARRAY *
get_cycle_from_graph(Graph *g)
{
  size_t start_id = graph_get_start_id(g);
  size_t_array cycle;
  init_size_t_array(&cycle, g->vertices->count);
  if (! append_size_t(&cycle, start_id))
    return NULL;
  size_t curr_id = start_id;
  size_t prev_id;
  while (cycle.count - 1 <= g->vertices->count)
  {
    size_t next_id = graph_get_next_id(g, curr_id, prev_id, cycle.count - 1);
    if (! append_size_t(&cycle, next_id))
      return NULL;
    if (next_id == start_id)
      break;
    else
    {
      prev_id = curr_id;
      curr_id = next_id;
    }
  }
  if (cycle.count - 1 > g->vertices->count)
  {
    meos_error(ERROR, MEOS_ERR_INTERNAL_ERROR,
      "Could not find a cycle in the graph");
    return NULL;
  }
  POINTARRAY *result = ptarray_construct(false, false, cycle.count);
  for (size_t i = 0; i < cycle.count; ++i)
  {
    size_t vertex_id = cycle.arr[i];
    double2 pos = g->vertices->arr[vertex_id]->pos;
    POINT4D p = (POINT4D) {pos.a, pos.b, 0, 0};
    ptarray_set_point4d(result, i, &p);
  }
  free_size_t_array(&cycle);
  return result;
}

/*****************************************************************************/
