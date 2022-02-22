// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * @file
 *
 * Simple AVL tree
 *
 * Copyright 2021-2022 Ian Pilcher <arequipeno@gmail.com>
 */

#ifndef SAVL_H_INCLUDED
#define SAVL_H_INCLUDED

#include <inttypes.h>
#include <stddef.h>

/**
 * AVL tree node structure.
 *
 * This structure should be embedded within the data structures that are to be
 * stored in the tree.
 *
 * @see	SAVL_NODE_CONTAINER
 */
struct savl_node {
	struct savl_node	*parent;
	struct savl_node	*left;
	struct savl_node	*right;
	int_fast8_t		skew;
};

/**
 * Returns a pointer to the data structure that contains a {@link savl_node}.
 *
 * For example:
 *
 *	struct product {
 *		unsigned int		sku;
 *      	unsigned int		price;
 *		struct savl_node	avl;
 *		char			*description;
 *	};
 *
 *	struct product *node_to_product(struct savl_node *node)
 *	{
 *		return SAVL_NODE_CONTAINER(node, struct product, avl);
 *	}
 *
 * @param ptr		The {@link savl_node}.
 * @param type		The type of the containing structure.
 * @param member	The name of the {@link savl_node} member within the
 *			containing structure.
 *
 * @see	savl_node
 */
#define SAVL_NODE_CONTAINER(ptr, type, member)	\
	((type *)(((const unsigned char *)(ptr)) - offsetof(type, member)))

/**
 * Type used for passing keys to comparison callback functions.
 *
 * Signed or unsigned integer keys can be passed directly to the comparison
 * function in the <b>`.i`</b> or <b>`.u`</b> members.  Other types of keys are
 * passed by reference in the <b>`.p`</b> member.
 *
 * @see savl_cmpfn
 */
union savl_key {
	intptr_t	i;
	uintptr_t	u;
	const void	*p;
};

/**
 * Comparison callback function type.
 *
 * Comparison functions must return a value that is less than zero, zero, or
 * greater than zero, depending on whether the value represented by <b>`key`</b>
 * is less than, equal to, or greater than the key value of the structure
 * containing <b>`node`</b>.  For example:
 *
 *	struct product {
 *		unsigned int		sku;
 *		unsigned int		price;
 *		struct savl_node	avl;
 *		char			*description;
 *	};
 *
 *	int compare_skus(union savl_key key, const struct savl_node *node)
 *	{
 *		const struct product *prod;
 *		unsigned int sku;
 *
 *		prod = SAVL_NODE_CONTAINER(node, struct product, avl);
 *		sku = key.u;
 *
 *		if (sku < prod->sku)
 *			return -1;
 *
 *		if (sku > prod->sku)
 *			return 1;
 *
 *		return 0;
 *	}
 *
 * @see savl_key
 * @see SAVL_NODE_CONTAINER
 */
typedef int (*savl_cmpfn)(union savl_key key, const struct savl_node *node);

/**
 * Callback function type to free the data structure that contains a
 * {@link savl_node}.  For example:
 *
 *	struct product {
 *		unsigned int		sku;
 *		unsigned int		price;
 *		struct savl_node	avl;
 *		char			*description;
 *	};
 *
 *	void free_sku(struct savl_node *node)
 *	{
 *		struct product *prod;
 *
 *		prod = SAVL_NODE_CONTAINER(node, struct product, avl);
 *		free(prod->description);
 *		free(prod);
 *	}
 *
 * @see SAVL_NODE_CONTAINER
 */
typedef void (*savl_freefn)(struct savl_node *node);

/*
 * Functions are documented in avl.c
 */

struct savl_node *savl_add(struct savl_node **const tree,
			   const savl_cmpfn cmpfn, const union savl_key key,
			   struct savl_node *const new, const _Bool replace);

struct savl_node *savl_try_add(struct savl_node **const tree,
			       const savl_cmpfn cmpfn, const union savl_key key,
			       struct savl_node *const new);

struct savl_node *savl_force_add(struct savl_node **const tree,
				 const savl_cmpfn cmpfn,
				 const union savl_key key,
				 struct savl_node *const new);

struct savl_node *savl_remove(struct savl_node **const tree,
			      const savl_cmpfn cmp, const union savl_key key);

void savl_remove_node(struct savl_node *const node,
		      struct savl_node **const tree);

struct savl_node *savl_get(struct savl_node *const tree, const savl_cmpfn cmpfn,
			   const union savl_key key);

void savl_free(struct savl_node **const tree, const savl_freefn freefn);
struct savl_node *savl_next(struct savl_node *node);
struct savl_node *savl_first(struct savl_node *node);
struct savl_node *savl_last(struct savl_node *node);
struct savl_node *savl_prev(struct savl_node *node);

#endif	/* SAVL_H_INCLUDED */
