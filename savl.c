// SPDX-License-Identifier: GPL-2.0-or-later

/*
 * Simple AVL tree
 *
 * Copyright 2021 Ian Pilcher <arequipeno@gmail.com>
 */

#include "savl.h"

#include <assert.h>

#define SAVL_DBL_LEFT	((int_fast8_t)	-2)
#define SAVL_LEFT	((int_fast8_t)	-1)
#define SAVL_EVEN	((int_fast8_t)	 0)
#define SAVL_RIGHT	((int_fast8_t)	 1)
#define SAVL_DBL_RIGHT	((int_fast8_t)	 2)

/**
 * Determine whether a node is the left or right child of its parent.
 *
 * @param node	The node.
 *
 * @return	<b>`SAVL_LEFT`</b> if <b>`node`</b> is the left child of its
 *		parent, <b>`SAVL_RIGHT`</b> if <b>`node`</b> is the right
 *		child of its parent, or <b>`SAVL_EVEN`</b> if <b>`node`</b>
 *		is the root of the tree.
 */
static int_fast8_t savl_which_child(const struct savl_node *const node)
{
	if (node->parent == NULL)
		return SAVL_EVEN;

	if (node->parent->left == node)
		return SAVL_LEFT;

	assert(node->parent->right == node);
	return SAVL_RIGHT;
}

/**
 * Use the known skew and relative depth of a node's subtree to calculate the
 * relative depth of the node's left child subtree.
 *
 * @param node		The node.
 * @param node_rdepth	The relative depth of the node's subtree.
 *
 * @return	The relative depth of <b>`node`</b>'s left child subtree.
 */
static int_fast8_t savl_rdepth_of_left(const struct savl_node *const node,
				       const int_fast8_t node_rdepth)
{
	/*
	 * If node is even or skewed left (or double-left), then its left
	 * subtree is at least as deep as its right subtree.
	 *
	 * So depth(node) = depth(left subtree) + 1, so
	 * depth(left subtree) = depth(node) - 1.
	 */
	if (node->skew <= SAVL_EVEN)
		return node_rdepth - 1;

	/*
	 * Otherwise, the node is skewed right (or double-right), and its
	 * right subtree is deeper.
	 *
	 * So depth(node) = depth(right subtree) + 1, and
	 * depth(right subtree) = depth(node) - 1.
	 *
	 * skew(node) = depth(right subtree) - depth(left subtree), so
	 * depth(left subtree) = depth(right subtree) - skew, and
	 * depth(left subtree) = (depth(node) - 1) - skew.
	 */
	return node_rdepth - 1 - node->skew;
}

/**
 * Use the known skew and relative depth of a node's subtree to calculate the
 * relative depth of the node's right child subtree.
 *
 * @param node		The node.
 * @param node_rdepth	The relative depth of the node's subtree.
 *
 * @return	The relative depth of <b>`node`</b>'s right child subtree.
 */
static int_fast8_t savl_rdepth_of_right(const struct savl_node *const node,
					const int_fast8_t node_rdepth)
{
	/*
	 * If node is even or skewed right (or double-right), then its right
	 * subtree is at least as deep as its left subtree.
	 *
	 * So depth(node) = depth(right subtree) + 1, so
	 * depth(right subtree) = depth(node) - 1.
	 */
	if (node->skew >= SAVL_EVEN)
		return node_rdepth - 1;

	/*
	 * Otherwise, the node is skewed left (or double-left), and its left
	 * subtree is deeper.
	 *
	 * So depth(node) = depth(left subtree) + 1, and
	 * depth(left subtree) = depth(node) - 1.
	 *
	 * skew(node) = depth(right subtree) - depth(left subtree), so
	 * depth(right subtree) = depth(left subtree) + skew, and
	 * depth(right subtree) = (depth(node) - 1) + skew.
	 */
	return node_rdepth - 1 + node->skew;
}

/**
 * Use the known skew of a node and the relative depth of its right child
 * subtree to calculate the relative depth of the node's subtree.
 *
 * @param node		The node.
 * @param right_rdepth	The relative depth of the node's right child subtree.
 *
 * @return	The relative depth of <b>`node`</b>'s subtree.
 */
static int_fast8_t savl_rdepth_from_right(const struct savl_node *const node,
					  const int_fast8_t right_rdepth)
{
	/*
	 * If node is even or skewed right (including double-right), the depth
	 * of its subtree is determined by the depth of its right child subtree.
	 */
	if (node->skew >= SAVL_EVEN)
		return right_rdepth + 1;

	/*
	 * Otherwise, the depth of the node's subtree is determined by the depth
	 * of its left child subtree.
	 *
	 * We know that skew = depth(right subtree) - depth(left subtree), so
	 * skew + depth(left subtree) = depth(right subtree), and
	 * depth(left subtree) = depth(right subtree) - skew.
	 */
	return right_rdepth - node->skew + 1;
}

/**
 * Use the known skew of a node and the relative depth of its left child subtree
 * to calculate the relative depth of the node's subtree.
 *
 * @param node		The node.
 * @param left_rdepth	The relative depth of the node's left child subtree.
 *
 * @return	The relative depth of <b>`node`</b>'s subtree.
 */
static int_fast8_t savl_rdepth_from_left(const struct savl_node *const node,
					 const int_fast8_t left_rdepth)
{
	/*
	 * If node is even or skewed left (including double-left), the depth of
	 * its subtree is determined by the depth of its left child subtree.
	 */
	if (node->skew <= SAVL_EVEN)
		return left_rdepth + 1;

	/*
	 * Otherwise, the depth of the node's subtree is determined by the depth
	 * of its right child subtree.
	 *
	 * We know that skew = depth(right subtree) - depth(left subtree), so
	 * skew + depth(left subtree) = depth(right subtree), and
	 * depth(right subtree) = depth(left subtree) + skew.
	 */
	return left_rdepth + node->skew + 1;
}

/**
 * Perform a left promotion on a subtree.
 *
 * Often referred to as a "right rotation," the left child of the subtree's
 * root is "promoted" to become the new root of the subtree.  The pre-promotion
 * root is "demoted" to become the new root's right child.  The former right
 * child of the new root (if any) becomes the left child of the old root.
 *
 *	       (OR)                                (NR)
 *	       /  \                   \            /  \
 *	      /    \       ----------- \          /    \
 *	     /      \      ----------- /         /      \
 *	  (NR)      (RM)              /       (LM)      (OR)
 *	  /  \                                          /  \
 *	(LM) (M)                                      (M)  (RM)
 *
 * Legend:
 *	* **OR** - original (pre-promotion) root of the subtree
 *	* **NR** - new (post-promotion) root of the subtree
 *	* **LM** - leftmost node
 *	* **M** - middle node
 *	* **RM** - rightmost node
 *
 * Each of the leftmost, middle, and rightmost "nodes" may be a leaf node
 * (depth = 1) or the root of a subtree (depth > 1), or may not exist
 * (depth = 0).
 *
 * @param[in,out] subtree	Double pointer to the root of the subtree.
 *				<b>`*subtree`</b> is set to the new root.
 *
 * @return	The change (if any) to the depth of the subtree.
 */
static int_fast8_t savl_promote_left(struct savl_node **const subtree)
{
	int_fast8_t new_rdepth_OR, new_rdepth_NR;

	struct savl_node *const restrict OR = *subtree;
	struct savl_node *const restrict NR = OR->left;
	struct savl_node *const restrict M = NR->right;  /* may be NULL */

	/* Calculate relative depths; rdepth of middle is defined to be 0 */
	const int_fast8_t rdepth_M = 0;
	const int_fast8_t rdepth_NR = savl_rdepth_from_right(NR, rdepth_M);
	const int_fast8_t rdepth_LM = savl_rdepth_of_left(NR, rdepth_NR);
	const int_fast8_t rdepth_OR =savl_rdepth_from_left(OR, rdepth_NR);
	const int_fast8_t rdepth_RM = savl_rdepth_of_right(OR, rdepth_OR);

	/* Rearrange the nodes in the tree */
	*subtree = NR;
	NR->parent = OR->parent;
	NR->right = OR;
	OR->parent = NR;
	OR->left = M;
	if (M != NULL)
		M->parent = OR;

	/*
	 * The depth and skew of the leftmost, middle, and rightmost subtrees
	 * didn't change, so use them to calculate the new skew and depth for
	 * the old and new root nodes.
	 */
	OR->skew = rdepth_RM - rdepth_M;
	new_rdepth_OR = savl_rdepth_from_right(OR, rdepth_RM);
	/* assert(new_rdepth_OR == savl_rdepth_from_left(OR, rdepth_M)); */
	NR->skew = new_rdepth_OR - rdepth_LM;
	new_rdepth_NR = savl_rdepth_from_left(NR, rdepth_LM);
	/* assert(new_rdepth_NR == savl_rdepth_from_right(NR, new_rdepth_OR)); */

	/* Return change in subtree depth */
	return new_rdepth_NR - rdepth_OR;
}

/**
 * Peform a right promotion on a subtree.
 *
 * Often referred to as a "left rotation," the right child of the subtree's
 * root is "promoted" to become the new root of the subtree.  The pre-promotion
 * root is "demoted" to become the new root's left child.  The former left child
 * of the new root (if any) becomes the right child of the old root.
 *
 *	     (OR)                                  (NR)
 *	     /  \                     \            /  \
 *	    /    \         ----------- \          /    \
 *	   /      \        ----------- /         /      \
 *	(LM)      (NR)                /       (OR)      (RM)
 *	          /  \                        /  \
 *	        (M)  (RM)                  (LM)  (M)
 * Legend:
 *	* **OR** - original (pre-promotion) root of the subtree
 *	* **NR** - new (post-promotion) root of the subtree
 *	* **LM** - leftmost node
 *	* **M** - middle node
 *	* **RM** - rightmost node
 *
 * Each of the leftmost, middle, and rightmost "nodes" may be a leaf node
 * (depth = 1) or the root of a subtree (depth > 1), or may not exist
 * (depth = 0).
 *
 * @param[in,out] subtree	Double pointer to the root of the subtree.
 *				<b>`*subtree`</b> is set to the new root.
 *
 * @return	The change (if any) to the depth of the subtree.
 */
static int_fast8_t savl_promote_right(struct savl_node **const subtree)
{
	int_fast8_t new_rdepth_OR, new_rdepth_NR;

	struct savl_node *const restrict OR = *subtree;
	struct savl_node *const restrict NR = OR->right;
	struct savl_node *const restrict M = NR->left;  /* may be NULL */

	/* Calculate relative depths; rdepth of middle is defined to be 0 */
	const int_fast8_t rdepth_M = 0;
	const int_fast8_t rdepth_NR = savl_rdepth_from_left(NR, rdepth_M);
	const int_fast8_t rdepth_RM = savl_rdepth_of_right(NR, rdepth_NR);
	const int_fast8_t rdepth_OR = savl_rdepth_from_right(OR, rdepth_NR);
	const int_fast8_t rdepth_LM = savl_rdepth_of_left(OR, rdepth_OR);

	/* Rearrange the nodes in the tree */
	*subtree = NR;
	NR->parent = OR->parent;
	NR->left = OR;
	OR->parent = NR;
	OR->right = M;
	if (M != NULL)
		M->parent = OR;

	/*
	 * The depth and skew of the rightmost, middle, and leftmost subtrees
	 * didn't change, so use them to calculate the new skew and rdepth for
	 * the old and new root nodes.
	 */
	OR->skew = rdepth_M - rdepth_LM;
	new_rdepth_OR = savl_rdepth_from_left(OR, rdepth_LM);
	/* assert(new_rdepth_OR == savl_rdepth_from_right(OR, rdepth_M)); */
	NR->skew = rdepth_RM - new_rdepth_OR;
	new_rdepth_NR = savl_rdepth_from_right(NR, rdepth_RM);
	/* assert(new_rdepth_NR == savl_rdepth_from_left(NR, new_rdepth_OR)); */

	/* Return change in subtree depth */
	return new_rdepth_NR - rdepth_OR;
}

/**
 * Search a tree for a key.
 *
 * If the key is found, its node is returned via <b>`*result`</b> and the
 * function returns <b>`SAVL_EVEN`</b>.
 *
 * If the key is not found, <b>`*result`</b> is set to the node that will
 * become the new node's parent (if a node with the new key is immediately added
 * to the tree), and the return value indicates whether the new node will be its
 * left (<b>`SAVL_LEFT`</b>) or right (<b>`SAVL_RIGHT`</b>) child.
 *
 * (If the tree is empty, <b>`*result`</b> is set to <b>`NULL`</b> and
 * <b>`SAVL_EVEN`</b> is returned.)
 *
 * @param node		The root of the tree to be searched.
 * @param cmpfn		Comparison function.
 * @param key		The key.
 * @param[out] result	Output parameter used to return the key's node (or its
 *			prospective parent).
 *
 * @return	An integer value that indicates whether the new entry will be
 *		the left child of the parent (<b>`SAVL_LEFT`</b>), the right
 *		child of the parent (<b>`SAVL_RIGHT`</b>), or a replacement for
 *		the parent (<b>`SAVL_EVEN`</b>).
 */
static int_fast8_t savl_search(struct savl_node *node, const savl_cmpfn cmpfn,
			       const union savl_key key,
			       struct savl_node **const result)
{
	int cmp_result = 0;

	if (node != NULL) {

		while (1) {

			cmp_result = cmpfn(key, node);

			if (cmp_result < 0 && node->left != NULL) {
				node = node->left;
				continue;
			}

			if (cmp_result > 0 && node->right != NULL) {
				node = node->right;
				continue;
			}

			break;
		}
	}

	*result = node;

	/* Return only the sign of the result, i.e. -1, 0, or 1 */
	return (cmp_result > 0) - (cmp_result < 0);
}

/**
 * Replace a node in a tree.
 *
 * The key of the new node must compare equal to the key of the old node.
 *
 * @param new		The new node.  <b>`new->parent`</b> must point to the
 *			to the node to be replaced.
 * @param[out] tree	A double pointer to the root node of the tree.  If the
 *			the root node is being replaced, <b>`*tree`</b> will be
 *			set to the new root node.
 *
 * @return	The pre-existing node that was replaced.
 */
static struct savl_node *savl_replace(struct savl_node *const new,
				      struct savl_node **const tree)
{
	struct savl_node *const old = new->parent;

	new->parent = old->parent;
	switch (savl_which_child(old)) {
		case SAVL_LEFT:		new->parent->left = new;
					break;
		case SAVL_RIGHT:	new->parent->right = new;
					break;
		case SAVL_EVEN:		*tree = new;
					break;
	}

	new->left = old->left;
	if (new->left != NULL)
		new->left->parent = new;

	new->right = old->right;
	if (new->right != NULL)
		new->right->parent = new;

	new->skew = old->skew;

	return old;
}

/**
 * Rebalance a tree after a new node has been added.
 *
 * Works upward from <b>`node`</b> to the root of the tree, updating node skews
 * and rebalancing if necessary.
 *
 * @param node		The parent of the newly added node.
 * @param which_child	Indicates which child was added.
 * @param[out] tree	A double pointer to the root node of the tree.  If the
 *			root of the tree is changed by rebalancing,
 *			<b>`*tree`</b> is set to the new root.
 */
static void savl_add_rebalance(struct savl_node *node, int_fast8_t which_child,
			       struct savl_node **const tree)
{
	int_fast8_t growth;

	while (node != NULL) {

		node->skew += which_child;

		/*
		 * If node's skew is now even, then it was the (previously)
		 * shallower child subtree that grew, so the depth of this
		 * node's subtree hasn't changed
		 */
		if (node->skew == SAVL_EVEN)
			return;

		which_child = savl_which_child(node);

		/*
		 * Node is now singly or doubly skewed, due to addition, so we
		 * know that its subtree depth increased.
		 */
		growth = 1;

		/* If skew is still OK (single), propagate growth upward */
		if (node->skew == SAVL_LEFT || node->skew == SAVL_RIGHT) {
			node = node->parent;
			continue;
		}

		if (node->skew == SAVL_DBL_LEFT) {
			if (node->left->skew == SAVL_RIGHT)
				growth += savl_promote_right(&node->left);
			growth += savl_promote_left(&node);
		}
		else {
			if (node->right->skew == SAVL_LEFT)
				growth += savl_promote_left(&node->right);
			growth += savl_promote_right(&node);
		}

		switch (which_child) {
			case SAVL_EVEN:		*tree = node;
						break;
			case SAVL_LEFT:		node->parent->left = node;
						break;
			case SAVL_RIGHT:	node->parent->right = node;
						break;
		}

		assert(growth == 0);
		return;
	}
}

/**
 * Add a node to a tree, potentially replacing a node with an equal key (if
 * any).
 *
 * If the tree already contains a node with a key equal to <b>`key`</b>, the
 * behavior is determined by the <b>`replace`</b> parameter.
 *
 * @param[in,out] tree	A double pointer to the root node of the tree.  If the
 *			root is changed (by rebalancing, replacement of the root
 *			node, or addition to an empty tree), <b>`*tree`</b> will
 *			be changed to point to the new root node.
 * @param cmpfn		Comparison function.
 * @param key		The key.
 * @param new		The node to be added.  It's key must compare equal to
 *			<b>`key`</b>.
 * @param replace	If the tree already contains a node with a key equal to
 *			<b>`key`</b>, should the new node be inserted in its
 *			place?
 *
 * @return	<b>`NULL`</b> if the tree did not already contain a node with a
 *		key equal to <b>`key`</b>, or a pointer to the pre-existing
 *		key (which may have been replaced, depending on the value of
 *		the <b>`replace`</b> parameter).
 */
struct savl_node *savl_add(struct savl_node **const tree,
			   const savl_cmpfn cmpfn, const union savl_key key,
			   struct savl_node *const new, const _Bool replace)
{
	int_fast8_t which_child;

	new->left = NULL;
	new->right = NULL;
	new->skew = SAVL_EVEN;

	/* If tree is empty, new node becomes the root node */
	if (*tree == NULL) {
		new->parent = NULL;
		*tree = new;
		return NULL;
	}

	/* Find existing node with equal key or prospective parent */
	which_child = savl_search(*tree, cmpfn, key, &new->parent);

	/*
	 * If node with equal key already exists, replace it (if asked) and
	 * return the pre-existing node.
	 */
	if (which_child == SAVL_EVEN) {
		if (replace)
			return savl_replace(new, tree);  /* returns old node */
		else
			return new->parent;  /* new->parent == old node */
	}

	/* Add new node to tree */
	if (which_child == SAVL_LEFT) {
		assert(new->parent->left == NULL);
		new->parent->left = new;
	}
	else {
		assert(new->parent->right == NULL);
		new->parent->right = new;
	}

	/* Adjust parent's skew and rebalance tree */
	savl_add_rebalance(new->parent, which_child, tree);

	return NULL;
}

/**
 * Add a node to a tree, if the tree does not already contain a node with the
 * same key.
 *
 * @param[in,out] tree	A double pointer to the root node of the tree.  If the
 *			root is changed (by rebalancing, replacement of the root
 *			node, or addition to an empty tree), <b>`*tree`</b> will
 *			be changed to point to the new root node.
 * @param cmpfn		Comparison function.
 * @param key		The key.
 * @param new		The node to be added.  It's key must compare equal to
 *			<b>`key`</b>.
 *
 * @return	<b>`NULL`</b> if the node was added, or a pointer to the
 *		pre-existing node with an equal key.
 */
struct savl_node *savl_try_add(struct savl_node **const tree,
			       const savl_cmpfn cmpfn, const union savl_key key,
			       struct savl_node *const new)
{
	return savl_add(tree, cmpfn, key, new, 0);
}

/**
 * Add a node to the tree, replacing a node with an equal key (if any).
 *
 * @param[in,out] tree	A double pointer to the root node of the tree.  If the
 *			root is changed (by rebalancing, replacement of the root
 *			node, or addition to an empty tree), <b>`*tree`</b> will
 *			be changed to point to the new root node.
 * @param cmpfn		Comparison function.
 * @param key		The key.
 * @param new		The node to be added.  It's key must compare equal to
 *			<b>`key`</b>.
 *
 * @return	The node that was replaced (if any), or <b>`NULL`</b>.
 */
struct savl_node *savl_force_add(struct savl_node **const tree,
				 const savl_cmpfn cmpfn,
				 const union savl_key key,
				 struct savl_node *const new)
{
	return savl_add(tree, cmpfn, key, new, 1);
}

/**
 * Rebalance a tree after a node has been deleted.
 *
 * Works upward from <b>`node`</b> to the root of the tree, updating node skews
 * and rebalancing if necessary.
 *
 * @param node		The parent of the deleted node.
 * @param which_child	Indicates which child subtree shrank.
 * @param[out] tree	A double pointer to the root node of the tree.  If the
 *			root of the tree is changed by rebalancing,
 *			<b>`*tree`</b> is set to the new root.
 */
static void savl_del_rebalance(struct savl_node *node, int_fast8_t which_child,
			       struct savl_node **const tree)
{
	int_fast8_t growth;

	while (node != NULL) {

		node->skew -= which_child;

		/*
		 * If node is not even, then its shallower subtree was the one
		 * that shrank, and its depth hasn't changed.  If it's singly
		 * skewed, no further rebalancing is needed.
		 */
		if (node->skew == SAVL_LEFT || node->skew == SAVL_RIGHT)
			return;

		which_child = savl_which_child(node);

		/*
		 * If node is now evenly skewed, then its deeper subtree shrank,
		 * so this node's subtree also shrank.  Propagate upward.
		 */
		if (node->skew == SAVL_EVEN) {
			node = node->parent;
			continue;
		}

		/* Node is doubly skewed, so depth didn't change (see above) */
		growth = 0;

		if (node->skew == SAVL_DBL_LEFT) {
			if (node->left->skew == SAVL_RIGHT)
				growth += savl_promote_right(&node->left);
			growth += savl_promote_left(&node);
		}
		else {
			if (node->right->skew == SAVL_LEFT)
				growth += savl_promote_left(&node->right);
			growth += savl_promote_right(&node);
		}

		switch (which_child) {
			case SAVL_EVEN:		*tree = node;
						break;
			case SAVL_LEFT:		node->parent->left = node;
						break;
			case SAVL_RIGHT:	node->parent->right = node;
						break;
		}

		/* Rebalancing may or may not have shrunk subtree */
		if (growth == 0)
			return;

		assert(growth == -1);
		node = node->parent;  /* Subtree shrank; propagate upward */
	}
}

/**
 * Delete a node that has 0 or 1 children from the tree.
 *
 * @param node		The node to be deleted.
 * @param[out] tree	A double pointer to the root node of the tree.  If the
 *			root node is changed (because it is deleted or the tree
 *			is rebalanced), <b>`*tree`</b> will be set to the new
 *			root node.
 */
static void savl_del_simple(struct savl_node *const node,
			    struct savl_node **const tree)
{
	struct savl_node *repl;
	int_fast8_t which_child;

	if (node->left != NULL)
		repl = node->left;
	else
		repl = node->right;  /* NULL if leaf node */

	if (repl != NULL)
		repl->parent = node->parent;

	which_child = savl_which_child(node);

	switch (which_child) {
		case SAVL_EVEN:		*tree = repl;
					return;
		case SAVL_LEFT:		node->parent->left = repl;
					break;
		case SAVL_RIGHT:	node->parent->right = repl;
					break;
	}

	savl_del_rebalance(node->parent, which_child, tree);
}

/**
 * Prepares a node to be used as a replacement for a node with 2 children that
 * is being deleted.  The replacement is taken from the to be deleted node's
 * left subtree.
 *
 * If the replacement node has a (left) child, the child is moved up to take the
 * replacement's position in the tree.
 *
 * The returned replacement node is modified.
 *
 *	* The replacement's <b>`skew`</b> indicates whether is is the to be
 *	  deleted node's immediate left child (<b>`SAVL_LEFT`</b>) or a right
 *	  child deeper in the node's left subtree (<b>`SAVL_RIGHT`</b>).
 *
 *	* The replacement's <b>`parent`</b> points to the node at which
 *	  rebalancing will begin.  If the replacement is not a direct child,
 *	  this will be its previous parent.  (I.e. the field will not be
 *	  changed.)  If the replacement is a direct child of the to be deleted
 *	  node, rebalancing must begin at the replacement node itself (in its
 *	  new position), so the replacement's <b>`parent`</b> will point to the
 *	  replacement itself.
 *
 * The <b>`skew`</b> of the replacement node's former parent is not updated.
 *
 * @param node	The node that is being deleted.
 *
 * @return	The replacement node.
 */
static struct savl_node *savl_left_repl(const struct savl_node *const node)
{
	struct savl_node *repl;
	int_fast8_t which_child;

	/* Find rightmost node in node's left subtree */
	for (repl = node->left; repl->right != NULL; repl = repl->right);

	/* Is replacement node's immediate left child? */
	which_child = savl_which_child(repl);

	/* Replacement's left child (if any) takes its place */
	if (which_child == SAVL_LEFT) {
		/* Replacement is node's immediate left child */
		repl->parent->left = repl->left;
		if (repl->left != NULL)
			repl->left->parent = repl->parent;
		/* Must point to node where rebalancing will start */
		repl->parent = repl;
	}
	else {
		/* Replacement is a right child in node's left subtree */
		repl->parent->right = repl->left;
		if (repl->left != NULL)
			repl->left->parent = repl->parent;
	}

	/* Temporarily "stash" which_child value in replacement's skew */
	repl->skew = which_child;

	return repl;
}

/**
 * Prepares a node to be used as a replacement for a node with 2 children that
 * is being deleted.  The replacement is taken from the to be deleted node's
 * right subtree.
 *
 * If the replacement node has a (right) child, the child is moved up to take
 * the replacement's position in the tree.
 *
 * The returned replacement node is modified.
 *
 *	* The replacement's <b>`skew`</b> inidicates whether it is the to be
 *	  deleted node's immediate right child (<b>`SAVL_RIGHT`</b>) or a left
 *	  child deeper in the node's right subtree (<b>`SAVL_LEFT`</b>).
 *
 *	* The replacement's <b>`parent`</b> points to the node at which
 *	  rebalancing will begin.  If the replacement is not a direct child,
 *	  this will be its previous parent.  (I.e. the field will not be
 *	  changed.)  If the replacment is a direct child of the to be deleted
 *	  node, rebalancing must begin at the replacement node itself (in its
 *	  new position), so the replacement's <b>`parent`</b> will point to the
 *	  replacment itself.
 *
 * The <b>`skew`</b> of the replacement node's former parent is not updated.
 *
 * @param node	The node that is being deleted.
 *
 * @return	The replacement node.
 */
static struct savl_node *savl_right_repl(struct savl_node *node)
{
	int_fast8_t which_child;

	/* Find leftmost node in node's right subtree */
	for (node = node->right; node->left != NULL; node = node->left);

	/* Is replacement node's immediate right child? */
	which_child = savl_which_child(node);

	/* Replacement's right child (if any) takes its place */
	if (which_child == SAVL_RIGHT) {
		/* Replacement is to be deleted node's immediate right child */
		node->parent->right = node->right;
		if (node->right != NULL)
			node->right->parent = node->parent;
		/* node->parent must point to where rebalancing will start */
		node->parent = node;
	}
	else {
		/* Replacement is a left child deeper in node's right subtree */
		node->parent->left = node->right;
		if (node->right != NULL)
			node->right->parent = node->parent;
	}

	/* Temporarily "stash" which_child value in replacement's skew */
	node->skew = which_child;

	return node;
}

/**
 * Delete a node that has 2 children from the tree.
 *
 * @param node		The node to be deleted.
 * @param[out] tree	A double pointer to the root node of the tree.  If the
 *			root node is changed (because it is deleted or the tree
 *			is rebalanced), <b>`*tree`</b> will be set to the new
 *			root node.
 */
static void savl_del_complex(struct savl_node *const node,
			     struct savl_node **const tree)
{
	static _Bool which_repl;

	struct savl_node *repl, *repl_parent;
	int_fast8_t which_child, which_subtree;

	/* Get replacement from deeper subtree (if any) or just alternate */
	which_subtree = node->skew;
	if (which_subtree == SAVL_EVEN) {
		which_repl = !which_repl;
		which_subtree = which_repl ? SAVL_LEFT : SAVL_RIGHT;
	}

	if (which_subtree == SAVL_LEFT)
		repl = savl_left_repl(node);
	else
		repl = savl_right_repl(node);

	which_child = repl->skew;
	repl_parent = repl->parent;

	switch (savl_which_child(node)) {
		case SAVL_EVEN:		*tree = repl;
					break;
		case SAVL_LEFT:		node->parent->left = repl;
					break;
		case SAVL_RIGHT:	node->parent->right = repl;
					break;
	}

	repl->parent = node->parent;

	repl->left = node->left;
	if (repl->left != NULL)
		repl->left->parent = repl;

	repl->right = node->right;
	if (repl->right != NULL)
		repl->right->parent = repl;

	repl->skew = node->skew;

	savl_del_rebalance(repl_parent, which_child, tree);
}

/**
 * Remove a node from the tree.
 *
 * @param node		The node to be deleted.
 * @param[out] tree	A double pointer to the root node of the tree.  If the
 *			root node is changed (because it is deleted or the tree
 *			is rebalanced), <b>`*tree`</b> will be set to the new
 *			root node.
 */
void savl_remove_node(struct savl_node *const node,
		      struct savl_node **const tree)
{
	if (node->left == NULL || node->right == NULL)
		savl_del_simple(node, tree);
	else
		savl_del_complex(node, tree);

	node->parent = NULL;
	node->left = NULL;
	node->right = NULL;
	node->skew = SAVL_EVEN;
}

/**
 * Remove a key from the tree.
 *
 * @param[in,out] tree	A double pointer to the root node of the tree.  If the
 *			root node is changed (because it is deleted or the tree
 *			is rebalanced), <b>`*tree`</b> will be set to the new
 *			root node.
 * @param cmpfn		Comparison function.
 * @param key		The key.
 *
 * @return	The node that was removed (or <b>`NULL`</b> if the tree did not
 *		contain a matching node.
 */
struct savl_node *savl_remove(struct savl_node **const tree,
			      const savl_cmpfn cmpfn, const union savl_key key)
{
	struct savl_node *node;

	if (savl_search(*tree, cmpfn, key, &node) != SAVL_EVEN || node == NULL)
		return NULL;

	savl_remove_node(node, tree);

	return node;
}

/**
 * Find the first node in a tree (or subtree).
 *
 * @param node	The tree (or subtree).
 *
 * @return	The first node in the (sub)tree.
 */
struct savl_node *savl_first(struct savl_node *node)
{
	while (node->left != NULL)
		node = node->left;

	return node;
}

/**
 * Find the last node in a tree (or subtree).
 *
 * @param node	The tree (or subtree).
 *
 * @return	The last node in the (sub)tree.
 */
struct savl_node *savl_last(struct savl_node *node)
{
	while (node->right != NULL)
		node = node->right;

	return node;
}

/**
 * Find the next node in the tree.
 *
 * @param node	The node from which to begin traversing the tree.
 *
 * @return	The next node in the tree.
 */
struct savl_node *savl_next(struct savl_node *node)
{
	if (node->right != NULL) {
		node = node->right;
		while (node->left != NULL)
			node = node->left;
		return node;
	}

	while (savl_which_child(node) == SAVL_RIGHT)
		node = node->parent;

	return node->parent;
}

/**
 * Find the previous node in the tree.
 *
 * @param node	The node from which to begin traversing the tree.
 *
 * @return	The previous node in the tree.
 */
struct savl_node *savl_prev(struct savl_node *node)
{
	if (node->left != NULL) {
		node = node->left;
		while (node->right != NULL)
			node = node->right;
		return node;
	}

	while (savl_which_child(node) == SAVL_LEFT)
		node = node->parent;

	return node->parent;
}

/**
 * Free all of the nodes in a tree.
 *
 * @param[in,out] tree	A double pointer to the root node of the tree.
 *			<b>`*tree`</b> is set to <b>`NULL`</b>.
 * @param freefn	A callback function to free the data structure that
 *			contains a node (and any associated resources).
 */
void savl_free(struct savl_node **const tree, const savl_freefn freefn)
{
	struct savl_node *node, *next;

	node = savl_first(*tree);

	while (node != NULL) {

		if (node->left != NULL) {
			node = node->left;
			continue;
		}

		if (node->right != NULL) {
			node = node->right;
			continue;
		}


		switch (savl_which_child(node)) {
			case SAVL_LEFT:		node->parent->left = NULL;
						break;
			case SAVL_RIGHT:	node->parent->right = NULL;
						break;
			case SAVL_EVEN:		*tree = NULL;
						break;
		}

		next = node->parent;
		freefn(node);
		node = next;
	}
}
