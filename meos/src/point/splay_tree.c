/******************************************************************************
 * Polymorphic implementation of Splay Trees, written in C.
 * https://github.com/Tyresius92/splay-tree/
 ******************************************************************************/

#include "point/splay_tree.h"

/* C */
#include <assert.h>
/* PostgreSQL */
#include <postgres.h>

// #include <stdio.h>
// #include <string.h>
// #include <stdint.h>

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

/*********************
 * Private functions *
 *********************/

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

/*
 * rotate_left
 *
 * given a tree and a node n, moves n's right child to be the child of n's
 * parent, and makes n the child of its right child
 *
 * CREs         n/a
 * UREs         n/a
 *
 * @param       T - tree in which rotation is occuring
 * @param       Node * - pointer to the node to be rotated
 * @return      n/a
 */
void splay_helper_rotate_left(SplayTree tree, Node *n);

/*
 * splay_helper_rotate_right
 *
 * given a tree and a node n, moves n's left child to be the child of n's
 * parent, and makes n the child of its left child
 *
 * CREs         n/a
 * UREs         n/a
 *
 * @param       T - tree in which rotation is occuring
 * @param       Node * - pointer to the node to be rotated
 * @return      n/a
 */
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

/************************
 * function definitions *
 ************************/

SplayTree splay_new(void *comparison_func)
{
  SplayTree tree = palloc(sizeof(struct splay_tree));

  tree->root = NULL;

  if (comparison_func == NULL) {
    tree->comparison_func = &strcmp;
  } else {
    tree->comparison_func = comparison_func;
  }

  return tree;
}

void splay_free(SplayTree tree)
{
  assert(tree != NULL);

  private_splay_deallocate_all_tree_nodes(tree->root);
  pfree(tree);

  tree = NULL;
}

void *splay_get_value_at_root(SplayTree tree)
{
  assert(tree != NULL);

  if (tree->root != NULL)
          return tree->root->value;
  else
          return NULL;
}

void splay_helper_rotate_left(SplayTree tree, Node *n)
{
  Node *right_child = n->right;

  n->right = right_child->left;

  if (n->right != NULL)
          n->right->parent = n;

  right_child->parent = n->parent;

  if(n->parent == NULL) {
          tree->root = right_child;
  } else if (n == n->parent->left) {
          (void) tree;
          n->parent->left = right_child;
  } else {
          (void) tree;
          n->parent->right = right_child;
  }

  right_child->left = n;
  n->parent = right_child;
}

void splay_helper_rotate_right(SplayTree tree, Node *n)
{
  Node *left_child = n->left;

  n->left = left_child->right;

  if (n->left != NULL)
          n->left->parent = n;

  left_child->parent = n->parent;

  if(n->parent == NULL) {
          tree->root = left_child;
  } else if (n == n->parent->left) {
          (void) tree;
          n->parent->left = left_child;
  } else {
          (void) tree;
          n->parent->right = left_child;
  }

  left_child->right = n;
  n->parent = left_child;
}

void splay_to_root(SplayTree tree, Node *n)
{
  Node *p = n->parent;
  while (p != NULL) {
          if (p->parent == NULL) {
                  single_rotate(tree, n);
          } else {
                  double_rotate(tree, n);
          }
          p = n->parent;
  }

  tree->root = n;
}

void single_rotate(SplayTree tree, Node *n)
{
  if (n->parent->left == n)
    zig_left(tree, n);
  else
    zig_right(tree, n);
}
void double_rotate(SplayTree tree, Node *n)
{
  Node *p = n->parent;
  Node *g = n->parent->parent;

  if (p->left == n && g->left == p) {
    zig_zig_left(tree, n);
    return;
  }

  if (p->left == n && g->right == p) {
    zig_zag_left(tree, n);
    return;
  }

  if (p->right == n && g->right == p) {
    zig_zig_right(tree, n);
    return;
  }

  if (p->right == n && g->left == p) {
    zig_zag_right(tree, n);
    return;
  }
}

void zig_left(SplayTree tree, Node *n)
{
  splay_helper_rotate_right(tree, n->parent);
}

void zig_right(SplayTree tree, Node *n)
{
  splay_helper_rotate_left(tree, n->parent);
}

void zig_zig_left(SplayTree tree, Node *n)
{
  Node *g = n->parent->parent;
  Node *p = n->parent;

  splay_helper_rotate_right(tree, g);
  splay_helper_rotate_right(tree, p);
}

void zig_zag_left(SplayTree tree, Node *n)
{
  Node *g = n->parent->parent;
  Node *p = n->parent;

  splay_helper_rotate_right(tree, p);
  splay_helper_rotate_left(tree, g);
}

void zig_zig_right(SplayTree tree, Node *n)
{
  Node *g = n->parent->parent;
  Node *p = n->parent;

  splay_helper_rotate_left(tree, g);
  splay_helper_rotate_left(tree, p);
}

void zig_zag_right(SplayTree tree, Node *n)
{
  Node *g = n->parent->parent;
  Node *p = n->parent;

  splay_helper_rotate_left(tree, p);
  splay_helper_rotate_right(tree, g);
}

void private_splay_deallocate_all_tree_nodes(Node *n) {
  if (n == NULL)
    return;

  if (n->left != NULL)
    private_splay_deallocate_all_tree_nodes(n->left);
  if (n->right != NULL)
    private_splay_deallocate_all_tree_nodes(n->right);

  pfree(n);
}

bool splay_is_empty(SplayTree tree)
{
  assert(tree != NULL);

  return tree->root == NULL;
}

int splay_insert_value(SplayTree tree, void *value)
{
  assert(tree != NULL && value != NULL);

  Node *new_node = splay_construct_node(value);
  tree->root = private_splay_insert_value(tree->root, new_node, tree->comparison_func);

  splay_to_root(tree, new_node);

  return 0;
}

Node *splay_construct_node(void *value)
{
  Node *new_node = (Node *) palloc(sizeof(Node));

  new_node->parent = NULL;
  new_node->left = NULL;
  new_node->right = NULL;
  new_node->value = value;

  return new_node;
}

Node *private_splay_insert_value(Node *root, Node *new_node,
  void *comparison_func(void *val1, void *val2))
{
  if (root == NULL) {
    return new_node;
  }

  if ((int)(intptr_t) comparison_func(new_node->value, root->value) < 0) {
    root->left = private_splay_insert_value(root->left, new_node, comparison_func);
    root->left->parent = root;
  } else {
    root->right = private_splay_insert_value(root->right, new_node, comparison_func);
    root->right->parent = root;
  }

  return root;
}

void *splay_search(SplayTree tree, void *value)
{
  Node *result = private_splay_find_in_tree(tree, value, tree->comparison_func);

  if (result != NULL) {
    splay_to_root(tree, result);
    return (void *) result->value;
  }

  return result; //AKA return NULL
}

Node *private_splay_find_in_tree(SplayTree tree, void *value,
  void *comparison_func(void *val1, void *val2))
{
  bool found = false;
  Node *curr = tree->root;
  int c = 0;

  while (!found && curr != NULL) {
    c = (int)(intptr_t) comparison_func(value, curr->value);

    if (c == 0) {
      found = true;
    } else if (c < 0) {
      curr = curr->left;
    } else if (c > 0) {
      curr = curr->right;
    }
  }

  return curr;
}

void splay_delete_value(SplayTree tree, void *value)
{
  Node *z = private_splay_find_in_tree(tree, value, tree->comparison_func);

  if (z == NULL)
    return;

  splay_to_root(tree, z);

  if (z->left == NULL) {
    private_splay_transplant(tree, z, z->right);
  } else if (z->right == NULL) {
    private_splay_transplant(tree, z, z->left);
  } else {
    Node *y = private_splay_minimum(z->right);
    if (y->parent != z) {
      private_splay_transplant(tree, y, y->right);
      y->right = z->right;
      y->right->parent = y;
    }
    private_splay_transplant(tree, z, y);
    y->left = z->left;
    y->left->parent = y;
  }

  pfree(z);
}

void private_splay_transplant(SplayTree tree, Node *u, Node *v)
{
  if (u->parent == NULL) {
    tree->root = v;
  } else if (u == u->parent->left) {
    u->parent->left = v;
  } else {
    u->parent->right = v;
  }

  if (v != NULL)
    v->parent = u->parent;
}

void *splay_tree_minimum(SplayTree tree)
{
  Node *n = private_splay_minimum(tree->root);

  if (n == NULL) {
    return NULL;
  } else {
    splay_to_root(tree, n);
    return n->value;
  }
}

void *splay_tree_maximum(SplayTree tree)
{
  Node *n = private_splay_maximum(tree->root);

  if (n == NULL) {
    return NULL;
  } else {
    splay_to_root(tree, n);
    return n->value;
  }
}

Node *private_splay_minimum(Node *x)
{
  while (x->left != NULL) {
    x = x->left;
  }

  return x;
}

Node *private_splay_maximum(Node *x)
{
  while (x->right != NULL) {
          x = x->right;
  }

  return x;
}

void *splay_successor_of_value(SplayTree tree, void *value)
{
  Node *n = private_splay_successor_of_value(tree, value, tree->comparison_func);

  if (n == NULL) {
    return NULL;
  } else {
    splay_to_root(tree, n);
    return n->value;
  }
}

void *splay_predecessor_of_value(SplayTree tree, void *value)
{
  Node *n =  private_splay_predecessor_of_value(tree, value, tree->comparison_func);
  if (n == NULL) {
    return NULL;
  } else {
    splay_to_root(tree, n);
    return n->value;
  }
}

Node *private_splay_successor_of_value(SplayTree tree, void *value,
                                 void *comparison_func(void *val1, void *val2))
{
  Node *curr_node = tree->root;
  Node *successor = NULL;
  int c;

  while (curr_node != NULL) {
    c = (int)(intptr_t) comparison_func(value, curr_node->value);

    if (c < 0) {
      successor = curr_node;
      curr_node = curr_node->left;
    } else {
      curr_node = curr_node->right;
    }
  }

  return successor;
}

Node *private_splay_predecessor_of_value(SplayTree tree, void *value,
  void *comparison_func(void *val1, void *val2))
{
  Node *curr_node = tree->root;
  Node *successor = NULL;
  int c;

  while (curr_node != NULL) {
    c = (int)(intptr_t) comparison_func(value, curr_node->value);
    if (c > 0) {
      successor = curr_node;
      curr_node = curr_node->right;
    } else {
      curr_node = curr_node->left;
    }
  }

  return successor;
}

void splay_map_inorder(SplayTree tree,
  void func_to_apply(void *value, int depth, void *cl), void *cl)
{
  int depth = 0;
  splay_private_inorder_map(tree->root, depth, func_to_apply, cl);
}

void splay_private_inorder_map(Node *root, int depth,
  void func_to_apply(void *value, int depth, void *cl), void *cl)
{
  if (root->left != NULL)
    splay_private_inorder_map(root->left, depth + 1, func_to_apply, cl);

  func_to_apply(root->value, depth, cl);

  if (root->right != NULL)
    splay_private_inorder_map(root->right, depth + 1, func_to_apply, cl);
}

void splay_map_preorder(SplayTree tree,
  void func_to_apply(void *value, int depth, void *cl), void *cl)
{
  int depth = 0;
  splay_private_preorder_map(tree->root, depth, func_to_apply, cl);
}

void splay_private_preorder_map(Node *root, int depth,
  void func_to_apply(void *value, int depth, void *cl), void *cl)
{
  func_to_apply(root->value, depth, cl);

  if (root->left != NULL)
    splay_private_preorder_map(root->left, depth + 1, func_to_apply, cl);

  if (root->right != NULL)
    splay_private_preorder_map(root->right, depth + 1, func_to_apply, cl);
}

void splay_map_postorder(SplayTree tree,
  void func_to_apply(void *value, int depth, void *cl), void *cl)
{
  int depth = 0;
  splay_private_postorder_map(tree->root, depth, func_to_apply, cl);
}

void splay_private_postorder_map(Node *root, int depth,
  void func_to_apply(void *value, int depth, void *cl), void *cl)
{
  if (root->left != NULL)
    splay_private_postorder_map(root->left, depth + 1, func_to_apply, cl);

  if (root->right != NULL)
    splay_private_postorder_map(root->right, depth + 1, func_to_apply, cl);

  func_to_apply(root->value, depth, cl);
}
