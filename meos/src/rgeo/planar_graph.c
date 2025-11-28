/*****************************************************************************
 *
 * planar_graph.h
 *    Planar graph library functions.
 *
 * Portions Copyright (c) 2019, Maxime Schoemans, Esteban Zimanyi,
 *    Universite Libre de Bruxelles
 * Portions Copyright (c) 1996-2019, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *****************************************************************************/

#include "rgeo/planar_graph.h"

#include <math.h>
#include <float.h>

#include "temporal/doublen.h"
#include "temporal/temporal.h"

/*****************************************************************************
 * size_t array functions
 *****************************************************************************/

static void
init_size_t_array(size_t_array *sta, size_t n)
{
  sta->arr = palloc0(sizeof(size_t) * n);
  sta->count = 0;
  sta->size = n;
}

static void
free_size_t_array(size_t_array *sta)
{
  pfree(sta->arr);
}

static void
append_size_t(size_t_array *sta, size_t v)
{
  if (sta->count == sta->size)
  {
    sta->size *= 2;
    size_t *new_arr = repalloc(sta->arr, sizeof(size_t) * sta->size);
    if (new_arr == NULL)
      ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR),
        errmsg("Not enough memory")));
    else
      sta->arr = new_arr;
  }
  sta->arr[sta->count++] = v;
}

/*****************************************************************************
 * Vertex functions
 *****************************************************************************/

static void
vertex_init(Vertex *v, double2 pos)
{
  v->pos = pos;
  v->neighbors = palloc0(sizeof(size_t_array));
  init_size_t_array(v->neighbors, (size_t) 3);
}

static void
vertex_destroy(Vertex *v)
{
  free_size_t_array(v->neighbors);
  pfree(v->neighbors);
}

static void
vertex_add_neighbor(Vertex *v, size_t neighbor)
{
  for (size_t i = 0; i < v->neighbors->count; ++i)
  {
    if (v->neighbors->arr[i] == neighbor)
      return;
  }
  append_size_t(v->neighbors, neighbor);
}

static void
init_vertex_array(Vertex_array *va, size_t n)
{
  va->arr = palloc0(sizeof(Vertex *) * n);
  va->count = 0;
  va->size = n;
}

static void
free_vertex_array(Vertex_array *va)
{
  for (size_t i = 0; i < va->count; ++i)
  {
    vertex_destroy(va->arr[i]);
    pfree(va->arr[i]);
  }
  pfree(va->arr);
}

static void
append_vertex(Vertex_array *va, Vertex *v)
{
  if (va->count == va->size)
  {
    va->size *= 2;
    Vertex **new_arr = repalloc(va->arr, sizeof(Vertex *) * va->size);
    if (new_arr == NULL)
      ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR),
        errmsg("Not enough memory")));
    else
      va->arr = new_arr;
  }
  va->arr[va->count++] = v;
}

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
  append_vertex(va, v);
  return result;
}

/*****************************************************************************
 * Segment functions
 *****************************************************************************/

Segment
make_segment(double2 start, double2 end)
{
  Segment seg;
  seg.start = start;
  seg.end = end;
  return seg;
}

static Segment
make_segment_with_ids(double2 start, double2 end, size_t start_id, size_t end_id)
{
  Segment seg;
  seg.start = start;
  seg.end = end;
  seg.start_id = start_id;
  seg.end_id = end_id;
  return seg;
}

static double
segment_length(Segment seg)
{
  return vec2_dist(seg.start, seg.end);
}

static double2
segment_normal(Segment seg)
{
  double a = seg.end.a - seg.start.a;
  double b = seg.end.b - seg.start.b;
  return vec2_normalize((double2) {b, -a});
}

static void
segment_set(Segment *seg, double2 start, double2 end, size_t start_id, size_t end_id)
{
  seg->start = start;
  seg->end = end;
  seg->start_id = start_id;
  seg->end_id = end_id;
}

static bool
point_on_segment(double2 p, Segment seg)
{
  double t = (p.a - seg.start.a) / (seg.end.a - seg.start.a);
  double b = seg.start.b + (seg.end.b - seg.start.b) * t;
  if (t < MEOS_EPSILON || 1 - t < MEOS_EPSILON)
    return false;
  else
    return fabs(p.b - b) < MEOS_EPSILON;
}

static bool
segments_compute_intersections(Segment seg_1, Segment seg_2, double2 *inter_1, double2 *inter_2)
{
  double2 normal_1 = segment_normal(seg_1);
  double2 normal_2 = segment_normal(seg_2);
  Segment seg_12;
  if (!vec2_eq(seg_1.start, seg_2.start))
    seg_12 = make_segment(seg_1.start, seg_2.start);
  else
    seg_12 = make_segment(seg_1.start, seg_2.end);
  double2 normal_12 = segment_normal(seg_12);
  if (fabs(vec2_dot(normal_1, normal_2) - 1) < MEOS_EPSILON ||
    fabs(vec2_dot(normal_1, normal_2) + 1) < MEOS_EPSILON)
  {
    if (fabs(vec2_dot(normal_1, normal_12) - 1) < MEOS_EPSILON ||
      fabs(vec2_dot(normal_1, normal_12) + 1) < MEOS_EPSILON)
    {
      bool s_1_i_2 = point_on_segment(seg_1.start, seg_2);
      bool e_1_i_2 = point_on_segment(seg_1.end, seg_2);
      bool s_2_i_1 = point_on_segment(seg_2.start, seg_1);
      bool e_2_i_1 = point_on_segment(seg_2.end, seg_1);
      if ((!s_1_i_2 && !e_1_i_2) || (!s_2_i_1 && !e_2_i_1))
        return false;
      else
      {
        *inter_1 = s_2_i_1 ? seg_2.start : seg_2.end;
        *inter_2 = s_1_i_2 ? seg_1.start : seg_1.end;
        return true;
      }
    }
    else
      return false;
  }
  else
  {
    double2 s_1 = seg_1.start;
    double2 e_1 = seg_1.end;
    double2 s_2 = seg_2.start;
    double2 e_2 = seg_2.end;
    double denom = (e_1.a - s_1.a) * (e_2.b - s_2.b) - (e_1.b - s_1.b) * (e_2.a - s_2.a);
    double t_1 = ((s_1.b - s_2.b) * (e_2.a - s_2.a) - (s_1.a - s_2.a) * (e_2.b - s_2.b)) / denom;
    double t_2 = ((s_1.b - s_2.b) * (e_1.a - s_1.a) - (s_1.a - s_2.a) * (e_1.b - s_1.b)) / denom;
    if (t_1 < MEOS_EPSILON || 1 - t_1 < MEOS_EPSILON  || t_2 < MEOS_EPSILON || 1 - t_2 < MEOS_EPSILON)
      return false;
    else
    {
      double2 p;
      p.a = s_1.a + (e_1.a - s_1.a) * t_1;
      p.b = s_1.b + (e_1.b - s_1.b) * t_1;
      if (vec2_dist(p, s_1) < MEOS_EPSILON)
      {
        *inter_1 = s_1;
        *inter_2 = s_1;
      }
      else if (vec2_dist(p, e_1) < MEOS_EPSILON)
      {
        *inter_1 = e_1;
        *inter_2 = e_1;
      }
      else if (vec2_dist(p, s_2) < MEOS_EPSILON)
      {
        *inter_1 = s_2;
        *inter_2 = s_2;
      }
      else if (vec2_dist(p, e_2) < MEOS_EPSILON)
      {
        *inter_1 = e_2;
        *inter_2 = e_2;
      }
      else
      {
        *inter_1 = p;
        *inter_2 = p;
      }
      return true;
    }
  }
}

static void
segment_set_ids(Segment *seg, size_t start_id, size_t end_id)
{
  seg->start_id = start_id;
  seg->end_id = end_id;
}

static void
init_segment_array(Segment_array *sa, size_t n)
{
  sa->arr = palloc0(sizeof(Segment) * n);
  sa->count = 0;
  sa->size = n;
}

static void
free_segment_array(Segment_array *sa)
{
  pfree(sa->arr);
}

static void
append_segment(Segment_array *sa, Segment seg)
{
  if (sa->count == sa->size)
  {
    sa->size *= 2;
    Segment *new_arr = repalloc(sa->arr, sizeof(Segment) * sa->size);
    if (new_arr == NULL)
      ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR),
        errmsg("Not enough memory")));
    else
      sa->arr = new_arr;
  }
  sa->arr[sa->count++] = seg;
}

static void
add_segment(Segment_array *sa, Segment seg)
{
  for (size_t i = 0; i < sa->count; ++i)
  {
    if (sa->arr[i].start_id == seg.start_id &&
      sa->arr[i].end_id == seg.end_id)
      return;
  }
  append_segment(sa, seg);
}

/*****************************************************************************
 * Graph functions
 *****************************************************************************/

void
init_graph(Graph *g, size_t n)
{
  g->vertices = palloc0(sizeof(Vertex_array));
  g->segments = palloc0(sizeof(Segment_array));
  init_vertex_array(g->vertices, n);
  init_segment_array(g->segments, n);
}

void
free_graph(Graph *g)
{
  free_vertex_array(g->vertices);
  free_segment_array(g->segments);
  pfree(g->vertices);
  pfree(g->segments);
}

static void
graph_add_neighbors(Graph *g, size_t id_1, size_t id_2)
{
  if (id_1 != id_2)
  {
    vertex_add_neighbor(g->vertices->arr[id_1], id_2);
    vertex_add_neighbor(g->vertices->arr[id_2], id_1);
  }
}

void
add_segment_to_graph(Graph *g, Segment seg)
{
  if (segment_length(seg) < MEOS_EPSILON)
    return;
  size_t start_id = get_vertex_index_from_pos(g->vertices, seg.start);
  size_t end_id = get_vertex_index_from_pos(g->vertices, seg.end);
  segment_set_ids(&seg, start_id, end_id);
  graph_add_neighbors(g, start_id, end_id);
  Segment_array *new_segs = palloc0(sizeof(Segment_array));
  init_segment_array(new_segs, 1);
  add_segment(new_segs, seg);
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
      Segment seg_j = old_segs->arr[j];
      double2 inter_i;
      double2 inter_j;
      bool intersects = segments_compute_intersections(seg_i, seg_j, &inter_i, &inter_j);
      if (intersects)
      {
        size_t inter_i_id = get_vertex_index_from_pos(g->vertices, inter_i);
        size_t inter_j_id = get_vertex_index_from_pos(g->vertices, inter_j);
        append_size_t(&seg_events, i);
        append_size_t(&vertex_events, inter_i_id);
        if (inter_j_id != seg_j.start_id && inter_j_id != seg_j.end_id)
        {
          double2 pos = g->vertices->arr[inter_j_id]->pos;
          add_segment(new_segs,
            make_segment_with_ids(seg_j.start, pos, seg_j.start_id, inter_j_id));
          graph_add_neighbors(g, seg_j.start_id, inter_j_id);
          add_segment(new_segs,
            make_segment_with_ids(seg_j.end, pos, seg_j.end_id, inter_j_id));
          graph_add_neighbors(g, seg_j.end_id, inter_j_id);
        }
      }
      else
        add_segment(new_segs, seg_j);
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
      segment_set(&g->segments->arr[seg_id], seg_i.start, pos, seg_i.start_id, vertex_id);
      graph_add_neighbors(g, seg_i.start_id, vertex_id);
      add_segment(g->segments,
        make_segment_with_ids(seg_i.end, pos, seg_i.end_id, vertex_id));
      graph_add_neighbors(g, seg_i.end_id, vertex_id);
    }
  }
  free_size_t_array(&seg_events);
  free_size_t_array(&vertex_events);
  for (size_t i = 0; i < new_segs->count; ++i)
    add_segment(g->segments, new_segs->arr[i]);
  free_segment_array(new_segs);
  pfree(new_segs);
}

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

POINTARRAY *
get_cycle_from_graph(Graph *g)
{
  size_t start_id = graph_get_start_id(g);
  size_t_array cycle;
  init_size_t_array(&cycle, g->vertices->count);
  append_size_t(&cycle, start_id);
  size_t curr_id = start_id;
  size_t prev_id;
  while (cycle.count - 1 <= g->vertices->count)
  {
    size_t next_id = graph_get_next_id(g, curr_id, prev_id, cycle.count - 1);
    append_size_t(&cycle, next_id);
    if (next_id == start_id)
      break;
    else
    {
      prev_id = curr_id;
      curr_id = next_id;
    }
  }
  if (cycle.count - 1 > g->vertices->count)
    ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR),
      errmsg("Could not find a cycle in the graph")));
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
