/*****************************************************************************
 *
 * This MobilityDB code is provided under The PostgreSQL License.
 * Copyright (c) 2016-2023, Université libre de Bruxelles and MobilityDB
 * contributors
 *
 * MobilityDB includes portions of PostGIS version 3 source code released
 * under the GNU General Public License (GPLv2 or later).
 * Copyright (c) 2001-2023, PostGIS contributors
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
 * @brief Splay Tree data structure
 * https://en.wikipedia.org/wiki/Splay_tree
 * derived from
 * https://github.com/Tyresius92/splay-tree/
 */

#ifndef __SPLAY_TREE_H__
#define __SPLAY_TREE_H__

#include <stdbool.h>

typedef struct splay_tree *SplayTree;

/*****************************************************************************/

extern SplayTree splay_new(void *comparison_func);
extern void splay_free(SplayTree tree);
extern bool splay_is_empty(SplayTree tree);
extern void *splay_get_value_at_root(SplayTree tree);
extern void splay_insert_value(SplayTree tree, void *value);
extern void *splay_search(SplayTree tree, void *value);
extern void splay_delete_value(SplayTree tree, void *value);
extern void *splay_tree_minimum(SplayTree tree);
extern void *splay_tree_maximum(SplayTree tree);
extern void *splay_successor_of_value(SplayTree tree, void *value);
extern void *splay_predecessor_of_value(SplayTree tree, void *value);
extern void splay_map_inorder(SplayTree tree,
  void func_to_apply(void *value, int depth, void *cl), void *cl);
extern void splay_map_preorder(SplayTree tree,
  void func_to_apply(void *value, int depth, void *cl), void *cl);
extern void splay_map_postorder(SplayTree tree,
  void func_to_apply(void *value, int depth, void *cl), void *cl);

/*****************************************************************************/

#endif
