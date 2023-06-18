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

#include "point/splay_tree.h"

/* C */
#include <assert.h>
/* PostgreSQL */
#include <postgres.h>

/*****************************************************************************/

typedef struct Node
{
  void *value;
  struct Node *parent;
  struct Node *left;
  struct Node *right;
} Node;

struct splay_tree
{
  Node *root;
  void *comparison_func;
};

/*****************************************************************************
 * Private functions
 *****************************************************************************/

void private_splay_deallocate_all_tree_nodes(Node *n);
Node *private_splay_insert_value(Node *root, Node *new_node,
  void *comparison_func(void *val1, void *val2));
Node *splay_construct_node(void *value);
Node *private_splay_find_in_tree(SplayTree tree, void *value,
  void *comparison_func(void *val1, void *val2));
void private_splay_transplant(SplayTree tree, Node *u, Node *v);
Node *private_splay_minimum(Node *x);
Node *private_splay_maximum(Node *x);
Node *private_splay_successor_of_value(SplayTree tree, void *value,
  void *comparison_func(void *val1, void *val2));
Node *private_splay_predecessor_of_value(SplayTree tree, void *value,
  void *comparison_func(void *val1, void *val2));

void splay_helper_rotate_left(SplayTree tree, Node *n);
void splay_helper_rotate_right(SplayTree tree, Node *n);
void splay_to_root(SplayTree tree, Node *n);
void single_rotate(SplayTree tree, Node *n);
void double_rotate(SplayTree tree, Node *n);
void zig_left(SplayTree tree, Node *n);
void zig_right(SplayTree tree, Node *n);
void zig_zig_left(SplayTree tree, Node *n);
void zig_zag_left(SplayTree tree, Node *n);
void zig_zig_right(SplayTree tree, Node *n);
void zig_zag_right(SplayTree tree, Node *n);

void splay_private_inorder_map(Node *root, int depth,
  void func_to_apply(void *value, int depth, void *cl), void *cl);
void splay_private_preorder_map(Node *root, int depth,
  void func_to_apply(void *value, int depth, void *cl), void *cl);
void splay_private_postorder_map(Node *root, int depth,
  void func_to_apply(void *value, int depth, void *cl), void *cl);

/*****************************************************************************
 * Function definitions
 *****************************************************************************/

/**
 * @brief
 */
SplayTree splay_new(void *comparison_func)
{
  SplayTree tree = palloc(sizeof(struct splay_tree));
  tree->root = NULL;
  if (comparison_func == NULL)
    tree->comparison_func = &strcmp;
  else
    tree->comparison_func = comparison_func;
  return tree;
}

/**
 * @brief
 */
void splay_free(SplayTree tree)
{
  assert(tree != NULL);
  private_splay_deallocate_all_tree_nodes(tree->root);
  pfree(tree);
  tree = NULL;
}

/**
 * @brief
 */
void *splay_get_value_at_root(SplayTree tree)
{
  assert(tree != NULL);
  if (tree->root != NULL)
    return tree->root->value;
  else
    return NULL;
}

/**
 * @brief Given a tree and a node n, moves n's right child to be the child
 * of n's parent, and makes n the child of its right child
 * @param tree Tree in which rotation is occuring
 * @param n Pointer to the node to be rotated
 */
void splay_helper_rotate_left(SplayTree tree, Node *n)
{
  Node *right_child = n->right;
  n->right = right_child->left;

  if (n->right != NULL)
    n->right->parent = n;

  right_child->parent = n->parent;

  if(n->parent == NULL)
    tree->root = right_child;
  else if (n == n->parent->left)
  {
    (void) tree;
    n->parent->left = right_child;
  }
  else
  {
    (void) tree;
    n->parent->right = right_child;
  }

  right_child->left = n;
  n->parent = right_child;
}

/**
 * @brief Given a tree and a node n, moves n's left child to be the child
 * of n's parent, and makes n the child of its left child
 * @param tree Tree in which rotation is occuring
 * @param n Pointer to the node to be rotated
 */
void splay_helper_rotate_right(SplayTree tree, Node *n)
{
  Node *left_child = n->left;
  n->left = left_child->right;

  if (n->left != NULL)
    n->left->parent = n;

  left_child->parent = n->parent;

  if(n->parent == NULL)
    tree->root = left_child;
  else if (n == n->parent->left)
  {
    (void) tree;
    n->parent->left = left_child;
  }
  else
  {
    (void) tree;
    n->parent->right = left_child;
  }

  left_child->right = n;
  n->parent = left_child;
  return;
}

/**
 * @brief
 */
void splay_to_root(SplayTree tree, Node *n)
{
  Node *p = n->parent;
  while (p != NULL)
  {
    if (p->parent == NULL)
      single_rotate(tree, n);
    else
      double_rotate(tree, n);
    p = n->parent;
  }
  tree->root = n;
  return;
}

/**
 * @brief
 */
void single_rotate(SplayTree tree, Node *n)
{
  if (n->parent->left == n)
    zig_left(tree, n);
  else
    zig_right(tree, n);
  return;
}

/**
 * @brief
 */
void double_rotate(SplayTree tree, Node *n)
{
  Node *p = n->parent;
  Node *g = n->parent->parent;

  if (p->left == n && g->left == p)
  {
    zig_zig_left(tree, n);
    return;
  }

  if (p->left == n && g->right == p)
  {
    zig_zag_left(tree, n);
    return;
  }

  if (p->right == n && g->right == p)
  {
    zig_zig_right(tree, n);
    return;
  }

  if (p->right == n && g->left == p)
  {
    zig_zag_right(tree, n);
    return;
  }
}

/**
 * @brief
 */
void zig_left(SplayTree tree, Node *n)
{
  splay_helper_rotate_right(tree, n->parent);
  return;
}

/**
 * @brief
 */
void zig_right(SplayTree tree, Node *n)
{
  splay_helper_rotate_left(tree, n->parent);
  return;
}

/**
 * @brief
 */
void zig_zig_left(SplayTree tree, Node *n)
{
  Node *g = n->parent->parent;
  Node *p = n->parent;

  splay_helper_rotate_right(tree, g);
  splay_helper_rotate_right(tree, p);
  return;
}

/**
 * @brief
 */
void zig_zag_left(SplayTree tree, Node *n)
{
  Node *g = n->parent->parent;
  Node *p = n->parent;

  splay_helper_rotate_right(tree, p);
  splay_helper_rotate_left(tree, g);
  return;
}

/**
 * @brief
 */
void zig_zig_right(SplayTree tree, Node *n)
{
  Node *g = n->parent->parent;
  Node *p = n->parent;

  splay_helper_rotate_left(tree, g);
  splay_helper_rotate_left(tree, p);
  return;
}

/**
 * @brief
 */
void zig_zag_right(SplayTree tree, Node *n)
{
  Node *g = n->parent->parent;
  Node *p = n->parent;

  splay_helper_rotate_left(tree, p);
  splay_helper_rotate_right(tree, g);
  return;
}

/**
 * @brief
 */
void private_splay_deallocate_all_tree_nodes(Node *n)
{
  if (n == NULL)
    return;

  if (n->left != NULL)
    private_splay_deallocate_all_tree_nodes(n->left);
  if (n->right != NULL)
    private_splay_deallocate_all_tree_nodes(n->right);

  pfree(n);
  return;
}

/**
 * @brief
 */
bool splay_is_empty(SplayTree tree)
{
  assert(tree != NULL);
  return tree->root == NULL;
}

/**
 * @brief
 */
void splay_insert_value(SplayTree tree, void *value)
{
  assert(tree != NULL && value != NULL);
  Node *new_node = splay_construct_node(value);
  tree->root = private_splay_insert_value(tree->root, new_node, tree->comparison_func);
  splay_to_root(tree, new_node);
  return;
}

/**
 * @brief
 */
Node *splay_construct_node(void *value)
{
  Node *new_node = (Node *) palloc(sizeof(Node));
  new_node->parent = NULL;
  new_node->left = NULL;
  new_node->right = NULL;
  new_node->value = value;
  return new_node;
}

/**
 * @brief
 */
Node *private_splay_insert_value(Node *root, Node *new_node,
  void *comparison_func(void *val1, void *val2))
{
  if (root == NULL)
    return new_node;

  if ((int)(intptr_t) comparison_func(new_node->value, root->value) < 0)
  {
    root->left = private_splay_insert_value(root->left, new_node, comparison_func);
    root->left->parent = root;
  }
  else
  {
    root->right = private_splay_insert_value(root->right, new_node, comparison_func);
    root->right->parent = root;
  }
  return root;
}

/**
 * @brief
 */
void *splay_search(SplayTree tree, void *value)
{
  Node *result = private_splay_find_in_tree(tree, value, tree->comparison_func);
  if (result != NULL)
  {
    splay_to_root(tree, result);
    return (void *) result->value;
  }
  return result; //AKA return NULL
}

/**
 * @brief
 */
Node *private_splay_find_in_tree(SplayTree tree, void *value,
  void *comparison_func(void *val1, void *val2))
{
  bool found = false;
  Node *curr = tree->root;
  while (!found && curr != NULL)
  {
    int c = (int)(intptr_t) comparison_func(value, curr->value);
    if (c == 0)
      found = true;
    else if (c < 0)
      curr = curr->left;
    else if (c > 0)
      curr = curr->right;
  }
  return curr;
}

/**
 * @brief
 */
void splay_delete_value(SplayTree tree, void *value)
{
  Node *z = private_splay_find_in_tree(tree, value, tree->comparison_func);
  if (z == NULL)
    return;

  splay_to_root(tree, z);
  if (z->left == NULL)
    private_splay_transplant(tree, z, z->right);
  else if (z->right == NULL)
    private_splay_transplant(tree, z, z->left);
  else
  {
    Node *y = private_splay_minimum(z->right);
    if (y->parent != z)
    {
      private_splay_transplant(tree, y, y->right);
      y->right = z->right;
      y->right->parent = y;
    }
    private_splay_transplant(tree, z, y);
    y->left = z->left;
    y->left->parent = y;
  }
  pfree(z);
  return;
}

/**
 * @brief
 */
void private_splay_transplant(SplayTree tree, Node *u, Node *v)
{
  if (u->parent == NULL)
    tree->root = v;
  else if (u == u->parent->left)
    u->parent->left = v;
  else
    u->parent->right = v;

  if (v != NULL)
    v->parent = u->parent;
  return;
}

/**
 * @brief
 */
void *splay_tree_minimum(SplayTree tree)
{
  Node *n = private_splay_minimum(tree->root);
  if (n == NULL)
    return NULL;

  splay_to_root(tree, n);
  return n->value;
}

/**
 * @brief
 */
void *splay_tree_maximum(SplayTree tree)
{
  Node *n = private_splay_maximum(tree->root);
  if (n == NULL)
    return NULL;

  splay_to_root(tree, n);
  return n->value;
}

/**
 * @brief
 */
Node *private_splay_minimum(Node *x)
{
  while (x->left != NULL)
    x = x->left;
  return x;
}

/**
 * @brief
 */
Node *private_splay_maximum(Node *x)
{
  while (x->right != NULL)
    x = x->right;
  return x;
}

/**
 * @brief
 */
void *splay_successor_of_value(SplayTree tree, void *value)
{
  Node *n = private_splay_successor_of_value(tree, value, tree->comparison_func);
  if (n == NULL)
    return NULL;

  splay_to_root(tree, n);
  return n->value;
}

/**
 * @brief
 */
void *splay_predecessor_of_value(SplayTree tree, void *value)
{
  Node *n = private_splay_predecessor_of_value(tree, value, tree->comparison_func);
  if (n == NULL)
    return NULL;

  splay_to_root(tree, n);
  return n->value;
}

/**
 * @brief
 */
Node *private_splay_successor_of_value(SplayTree tree, void *value,
  void *comparison_func(void *val1, void *val2))
{
  Node *curr_node = tree->root;
  Node *successor = NULL;
  while (curr_node != NULL)
  {
    int c = (int)(intptr_t) comparison_func(value, curr_node->value);
    if (c < 0)
    {
      successor = curr_node;
      curr_node = curr_node->left;
    }
    else
      curr_node = curr_node->right;
  }
  return successor;
}

/**
 * @brief
 */
Node *private_splay_predecessor_of_value(SplayTree tree, void *value,
  void *comparison_func(void *val1, void *val2))
{
  Node *curr_node = tree->root;
  Node *successor = NULL;
  while (curr_node != NULL)
  {
    int c = (int)(intptr_t) comparison_func(value, curr_node->value);
    if (c > 0)
    {
      successor = curr_node;
      curr_node = curr_node->right;
    }
    else
      curr_node = curr_node->left;
  }
  return successor;
}

/**
 * @brief
 */
void splay_map_inorder(SplayTree tree,
  void func_to_apply(void *value, int depth, void *cl), void *cl)
{
  int depth = 0;
  splay_private_inorder_map(tree->root, depth, func_to_apply, cl);
  return;
}

/**
 * @brief
 */
void splay_private_inorder_map(Node *root, int depth,
  void func_to_apply(void *value, int depth, void *cl), void *cl)
{
  if (root->left != NULL)
    splay_private_inorder_map(root->left, depth + 1, func_to_apply, cl);

  func_to_apply(root->value, depth, cl);

  if (root->right != NULL)
    splay_private_inorder_map(root->right, depth + 1, func_to_apply, cl);
  return;
}

/**
 * @brief
 */
void splay_map_preorder(SplayTree tree,
  void func_to_apply(void *value, int depth, void *cl), void *cl)
{
  int depth = 0;
  splay_private_preorder_map(tree->root, depth, func_to_apply, cl);
  return;
}

/**
 * @brief
 */
void splay_private_preorder_map(Node *root, int depth,
  void func_to_apply(void *value, int depth, void *cl), void *cl)
{
  func_to_apply(root->value, depth, cl);

  if (root->left != NULL)
    splay_private_preorder_map(root->left, depth + 1, func_to_apply, cl);

  if (root->right != NULL)
    splay_private_preorder_map(root->right, depth + 1, func_to_apply, cl);
  return;
}

/**
 * @brief
 */
void splay_map_postorder(SplayTree tree,
  void func_to_apply(void *value, int depth, void *cl), void *cl)
{
  int depth = 0;
  splay_private_postorder_map(tree->root, depth, func_to_apply, cl);
  return;
}

/**
 * @brief
 */
void splay_private_postorder_map(Node *root, int depth,
  void func_to_apply(void *value, int depth, void *cl), void *cl)
{
  if (root->left != NULL)
    splay_private_postorder_map(root->left, depth + 1, func_to_apply, cl);

  if (root->right != NULL)
    splay_private_postorder_map(root->right, depth + 1, func_to_apply, cl);

  func_to_apply(root->value, depth, cl);
  return;
}

/*****************************************************************************/
