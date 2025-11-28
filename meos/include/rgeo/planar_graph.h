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

#ifndef __PLANAR_GRAPH_H__
#define __PLANAR_GRAPH_H__

#include <postgres.h>
#include <liblwgeom.h>
// #include <catalog/pg_type.h>

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
extern void add_segment_to_graph(Graph *g, Segment seg);
extern POINTARRAY *get_cycle_from_graph(Graph *g);
extern void free_graph(Graph *g);

/*****************************************************************************/

#endif /* __PLANAR_GRAPH_H__ */
