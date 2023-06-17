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
 * @brief Splay Tree data structure based on
 * https://github.com/Tyresius92/splay-tree/
 */

#ifndef BASIC_SPLAY_H
#define BASIC_SPLAY_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct splay_tree *SplayTree;

/**********************
 * FUNCTION CONTRACTS *
 * AND DECLARATIONS   *
 **********************/

/*
 * splay_new
 *
 * returns a pointer to a new, empty red black tree
 *
 * CREs         n/a
 * UREs         system out of memory
 *
 *
 * @param       void * - pointer to a comparison function. if NULL is passed
 *                              as argument, strcmp is assumed.
 *
 *              comparison_function
 *              @param          item being inserted (val1)
 *              @param          item from tree which we are comparing (val2)
 *              @return         int
 *                               - zero (0) if val1 == val2
 *                               - positive value (n > 0) if val1 > val2
 *                               - negative value (n < 0) if val1 < val2
 *
 * @return      pointer to empty splay_tree
 */
SplayTree splay_new(void *comparison_func);

/*
 * splay_tree_free
 *
 * given a pointer to a red black tree, deallocates the tree and all nodes
 * contained within it, then sets the value of the pointer to NULL
 *
 * CREs         tree == NULL
 * UREs         n/a
 *
 * @param       RedBlack_T - the tree to be freed
 * @return      n/a
 */
void splay_free(SplayTree tree);

/*
 * splay_is_empty
 *
 * returns true if the tree is empty, and false otherwise
 *
 * CREs         tree == NULL
 * UREs         n/a
 *
 * @param       Redblack_T - tree to be checked if empty
 * @return      bool - true if empty, false otherwise
 */
bool splay_is_empty(SplayTree tree);

/*
 *
 */
void *splay_get_value_at_root(SplayTree tree);

/*
 * insert_value
 *
 * given a value (cast to void), inserts the value into the given SPLAY Tree
 *
 * CREs         tree == NULL
 *              value == NULL
 *
 * UREs         system out of memory
 *              attempting to pass in a value which cannot be compared with
 *                      your comparison function
 *
 * @param       SplayTree - tree in which to insert value
 * @param       void * - a pointer to any item to be inserted
 * @return      n/a
 */
int splay_insert_value(SplayTree tree, void *value);

/*
 * splay_search
 *
 * given a tree and a value to search for, returns a pointer to the stored
 * value, or NULL if the value is not found. If duplicates are in the tree,
 * returns the first one found
 *
 * @param       SplayTree - tree in which to search
 * @param       void * - value to search for
 * @return      void * - pointer to the value that was found
 */
void *splay_search(SplayTree tree, void *value);

/*
 * splay_delete_value
 *
 * given a value, deletes the first instance of it that is found in the tree
 * if a value is given that is not in the tree, this function has no effect
 *
 * CREs         tree == NULL
 * UREs         n/a
 *
 * @param       SplayTree - tree to find the value in
 * @param       void * - pointer to the value to be deleted
 * @return      n/a
 */
void splay_delete_value(SplayTree tree, void *value);

/*
 * splay_tree_minimum
 *
 * given a tree, returns the minimum value stored in the tree
 *
 * CREs         tree == NULL
 * UREs         n/a
 *
 * @param       SplayTree - tree to be searched
 * @return      void * - pointer to min value
 */
void *splay_tree_minimum(SplayTree tree);

/*
 * splay_tree_maximum
 *
 * given a tree, returns the maximum value stored in the tree
 *
 * CREs         tree == NULL
 * UREs         n/a
 *
 * @param       SplayTree - tree to be searched
 * @return      void * - pointer to max value
 */
void *splay_tree_maximum(SplayTree tree);

/*
 * splay_successor_of_value
 *
 * given a tree and a value, returns the first successor of that value
 * returned value will always be distinct from value, even if there are
 * duplicates; returns NULL if no successor
 *
 * CREs         tree == NULL
 *              value == NULL
 * UREs         n/a
 *
 * @param       SplayTree - tree to be searched
 * @param       void * - value to find the successor of
 * @return      void * - value of the successor
 */
void *splay_successor_of_value(SplayTree tree, void *value);

/*
 * splay_predecessor_of_value
 *
 * given a tree and a value, returns the first predecessor of that value
 * returned value will always be distinct from value, even if there are
 * duplicates; returns NULL if no predecessor
 *
 * CREs         tree == NULL
 *              value == NULL
 * UREs         n/a
 *
 * @param       SplayTree - tree to be searched
 * @param       void * - value to find the predecessor of
 * @return      void * - value of the predecessor
 */
void *splay_predecessor_of_value(SplayTree tree, void *value);

/*
 * splay_map_inorder
 *
 * given a tree and a pointer to a function, applies the function to every
 * element stored in the tree via an inorder walk
 * example valid operations include:
 *              - print every value
 *              - increment every stored value by one
 *              - store every element in an array (stored in closure)
 *
 * CREs         tree == NULL
 *              func_to_apply == NULL
 * UREs         func_to_apply modifies tree structure by performing different
 *                      operations on each node. (for instance, subtracting 1
 *                      from the first node, 2 from the second, 3 from the
 *                      third...and n from the nth could result in the SPLAY
 *                      property being invalidated)
 *
 *
 * @param       Redblack_T - tree to apply function to
 * @param       void * - pointer to a function
 * @param       void * - a closure item; can be anything you would like to
 *                              make use of when evaluating your function
 * @return      n/a
 */
void splay_map_inorder(SplayTree tree,
                    void func_to_apply(void *value, int depth, void *cl),
                    void *cl);

/*
 * splay_map_preorder
 *
 * given a tree and a pointer to a function, applies the function to every
 * element stored in the tree via a preorder walk
 * example valid operations include:
 *              - print every value
 *              - increment every stored value by one
 *              - store every element in an array (stored in closure)
 *
 * CREs         tree == NULL
 *              func_to_apply == NULL
 * UREs         func_to_apply modifies tree structure by performing different
 *                      operations on each node. (for instance, subtracting 1
 *                      from the first node, 2 from the second, 3 from the
 *                      third...and n from the nth could result in the SPLAY
 *                      property being invalidated)
 *
 *
 * @param       Redblack_T - tree to apply function to
 * @param       void * - pointer to a function
 * @param       void * - a closure item; can be anything you would like to
 *                              make use of when evaluating your function
 * @return      n/a
 */
void splay_map_preorder(SplayTree tree,
                     void func_to_apply(void *value, int depth, void *cl),
                     void *cl);

/*
 * splay_map_postorder
 *
 * given a tree and a pointer to a function, applies the function to every
 * element stored in the tree via a postorder walk
 * example valid operations include:
 *              - print every value
 *              - increment every stored value by one
 *              - store every element in an array (stored in closure)
 *
 * CREs         tree == NULL
 *              func_to_apply == NULL
 * UREs         func_to_apply modifies tree structure by performing different
 *                      operations on each node. (for instance, subtracting 1
 *                      from the first node, 2 from the second, 3 from the
 *                      third...and n from the nth could result in the SPLAY
 *                      property being invalidated)
 *
 *
 * @param       Redblack_T - tree to apply function to
 * @param       void * - pointer to a function
 * @param       void * - a closure item; can be anything you would like to
 *                              make use of when evaluating your function
 * @return      n/a
 */
void splay_map_postorder(SplayTree tree,
                      void func_to_apply(void *value, int depth, void *cl),
                      void *cl);

#endif
