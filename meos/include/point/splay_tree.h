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
 * based on
 * https://github.com/Tyresius92/splay-tree/
 */

#ifndef __SPLAY_TREE_H__
#define __SPLAY_TREE_H__

#include <stdbool.h>

typedef struct splay_tree *SplayTree;

/*****************************************************************************
 * FUNCTION CONTRACTS AND DECLARATIONS
 *****************************************************************************/

/**
 * @brief Return a pointer to a new, empty splay tree
 * @param comparison_func Pointer to a comparison function. If NULL is passed
 * as argument, strcmp is assumed. The comparison function has two parameters
 * - val1 Item being inserted
 * - val2 Item from tree which we are comparing
 * and returns and integer as follows
 * - zero (0) if val1 == val2
 * - positive value (n > 0) if val1 > val2
 * - negative value (n < 0) if val1 < val2
 * @return Pointer to empty splay_tree
 *
 * CREs n/a
 * UREs system out of memory
 */
SplayTree splay_new(void *comparison_func);

/**
 * @brief Given a pointer to a splay tree, deallocates the tree and all nodes
 * contained within it, then sets the value of the pointer to NULL
 * @param tree Tree to be freed
 *
 * CREs tree == NULL
 * UREs n/a
 */
void splay_free(SplayTree tree);

/**
 * @brief returns true if the tree is empty, and false otherwise
 * @param tree Tree to be checked if empty
 * @return True if empty, false otherwise
 *
 * CREs tree == NULL
 * UREs n/a
 */
bool splay_is_empty(SplayTree tree);

/**
 * @brief
 */
void *splay_get_value_at_root(SplayTree tree);

/**
 * @brief Given a value (cast to void), inserts the value into the given tree
 * @param tree Tree in which to insert value
 * @param value Pointer to any item to be inserted
 *
 * CREs tree == NULL
 *      value == NULL
 * UREs system out of memory
 *      attempting to pass in a value which cannot be compared with your
 *      comparison function
 */
void splay_insert_value(SplayTree tree, void *value);

/**
 * @brief Given a tree and a value to search for, returns a pointer to the
 * stored value, or NULL if the value is not found. If duplicates are in the
 * tree, returns the first one found
 * @param tree Tree in which to search
 * @param value Value to search for
 * @return Pointer to the value that was found
 */
void *splay_search(SplayTree tree, void *value);

/**
 * @brief Given a value, deletes the first instance of it that is found in the tree
 * if a value is given that is not in the tree, this function has no effect
 * @param tree Tree to find the value in
 * @param value Pointer to the value to be deleted
 *
 * CREs tree == NULL
 * UREs n/a
 */
void splay_delete_value(SplayTree tree, void *value);

/**
 * @brief Given a tree, returns the minimum value stored in the tree
 * @param tree Tree to be searched
 * @return Pointer to minimum value
 *
 * CREs tree == NULL
 * UREs n/a
 */
void *splay_tree_minimum(SplayTree tree);

/**
 * @brief Given a tree, returns the maximum value stored in the tree
 * @param       SplayTree - tree to be searched
 * @return Pointer to maximum value
 *
 * CREs tree == NULL
 * UREs n/a
 */
void *splay_tree_maximum(SplayTree tree);

/**
 * @brief Given a tree and a value, returns the first successor of that value
 * returned value will always be distinct from value, even if there are
 * duplicates; returns NULL if no successor
 * @param tree Tree to be searched
 * @param value Value to find the successor of
 * @return Value of the successor
 *
 * CREs tree == NULL
 *      value == NULL
 * UREs n/a
 */
void *splay_successor_of_value(SplayTree tree, void *value);

/**
 * @brief Given a tree and a value, returns the first predecessor of that value
 * returned value will always be distinct from value, even if there are
 * duplicates; returns NULL if no predecessor
 * @param tree Tree to be searched
 * @param value Value to find the predecessor of
 * @return Value of the predecessor
 *
 * CREs tree == NULL
 *      value == NULL
 * UREs n/a
 *
 */
void *splay_predecessor_of_value(SplayTree tree, void *value);

/**
 * @brief Given a tree and a pointer to a function, applies the function to
 * every element stored in the tree via an inorder walk.
 *
 * Example valid operations include:
 * - print every value
 * - increment every stored value by one
 * - store every element in an array (stored in closure)
 *
 * @param tree Tree to apply function to
 * @param func_to_apply pointer to a function
 * @param cl Closure item; can be anything you would like to make use of when
 * evaluating your function
 *
 * CREs tree == NULL
 *      func_to_apply == NULL
 * UREs func_to_apply modifies tree structure by performing different
 *      operations on each node. (for instance, subtracting 1 from the first
 *      node, 2 from the second, 3 from the third...and n from the nth could
 *      result in the SPLAY property being invalidated)
 */
void splay_map_inorder(SplayTree tree,
  void func_to_apply(void *value, int depth, void *cl), void *cl);

/**
 * @brief Given a tree and a pointer to a function, applies the function to
 * every element stored in the tree via a preorder walk.
 *
 * Example valid operations include:
 * - print every value
 * - increment every stored value by one
 * - store every element in an array (stored in closure)
 *
 * @param tree Tree to apply function to
 * @param func_to_apply pointer to a function
 * @param cl Closure item, can be anything you would like to make use of when
 * evaluating your function
 *
 * CREs tree == NULL
 *      func_to_apply == NULL
 * UREs func_to_apply modifies tree structure by performing different
 *      operations on each node. (for instance, subtracting 1 from the first
 *      node, 2 from the second, 3 from the third...and n from the nth could
 *      result in the SPLAY property being invalidated)
 */
void splay_map_preorder(SplayTree tree,
  void func_to_apply(void *value, int depth, void *cl), void *cl);

/**
 * @brief Given a tree and a pointer to a function, applies the function to
 * every element stored in the tree via a postorder walk.
 *
 * Example valid operations include:
 * - print every value
 * - increment every stored value by one
 * - store every element in an array (stored in closure)
 *
 * @param tree Tree to apply function to
 * @param func_to_apply Pointer to a function
 * @param cl Closure item; can be anything you would like to make use of when
 * evaluating your function
 *
 * CREs tree == NULL
 *      func_to_apply == NULL
 * UREs func_to_apply modifies tree structure by performing different
 *      operations on each node. (for instance, subtracting 1 from the first
 *      node, 2 from the second, 3 from the third...and n from the nth could
 *      result in the SPLAY property being invalidated)
 */
void splay_map_postorder(SplayTree tree,
  void func_to_apply(void *value, int depth, void *cl), void *cl);

#endif
