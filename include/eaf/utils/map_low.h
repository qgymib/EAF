/** @file
 * eaf_map_low is a sorted associative container that contains data structures
 * with unique keys. Keys are sorted by using the comparison function Compare.
 * Search, removal, and insertion operations have logarithmic complexity. Maps
 * are usually implemented as red-black trees.
 * Low level operation are available for advanced usage.
 */
#ifndef __EAF_UTILS_MAP_LOW_H__
#define __EAF_UTILS_MAP_LOW_H__
#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

/**
 * @ingroup EAF-Utils
 * @defgroup EAF-MapLow MapLow
 * @{
 */

#include "eaf/utils/define.h"

/**
 * @brief Static initializer for #eaf_map_low_t
 * @see eaf_map_low_t
 */
#define EAF_MAP_LOW_INITIALIZER			{ NULL }

/**
 * @brief Static initializer for #eaf_map_low_node_t
 * @see eaf_map_low_node_t
 */
#define EAF_MAP_LOW_NODE_INITIALIZER	{ NULL, NULL, NULL }

/**
 * @brief find helper
 * @param ret			result
 * @param p_table		eaf_map_low_t*
 * @param USER_TYPE		user type
 * @param user			user data
 * @param user_vs_orig	compare rule
 */
#define EAF_MAP_LOW_FIND_HELPER(ret, p_table, USER_TYPE, user, user_vs_orig)	\
	do {\
		int flag_success = 0;\
		eaf_map_low_t* __table = p_table;\
		eaf_map_low_node_t* node = __table->rb_root;\
		ret = NULL;\
		while (node) {\
			USER_TYPE* orig = EAF_CONTAINER_OF(node, USER_TYPE, node);\
			int cmp_ret = user_vs_orig;\
			if (cmp_ret < 0) {\
				node = node->rb_left;\
			} else if (cmp_ret > 0) {\
				node = node->rb_right;\
			} else {\
				flag_success = 1;\
				ret = orig;\
				break;\
			}\
		}\
		if (flag_success) {\
			break;\
		}\
	} while (0)

/**
 * @brief insert helper
 * @param ret				result
 * @param p_table			eaf_map_low_t*
 * @param USER_TYPE			user type
 * @param user				address
 * @param user_vs_orig		compare rule
 */
#define EAF_MAP_LOW_INSERT_HELPER(ret, p_table, USER_TYPE, user, user_vs_orig)	\
	do {\
		int flag_failed = 0;\
		eaf_map_low_t* __table = p_table;\
		eaf_map_low_node_t **new_node = &(__table->rb_root), *parent = NULL;\
		ret = eaf_errno_success;\
		while (*new_node) {\
			USER_TYPE* orig = EAF_CONTAINER_OF(*new_node, USER_TYPE, node);\
			int cmp_ret = user_vs_orig;\
			parent = *new_node;\
			if (cmp_ret < 0) {\
				new_node = &((*new_node)->rb_left);\
			} else if (cmp_ret > 0) {\
				new_node = &((*new_node)->rb_right);\
			} else {\
				flag_failed = 1;\
				ret = eaf_errno_duplicate;\
				break;\
			}\
		}\
		if (flag_failed) {\
			break;\
		}\
		eaf_map_low_link_node(&(user)->node, parent, new_node);\
		eaf_map_low_insert_color(&(user)->node, __table);\
	} while (0)

/**
 * @brief eaf_map_low node
 * @see EAF_MAP_LOW_NODE_INITIALIZER
 */
typedef struct eaf_map_low_node
{
	struct eaf_map_low_node*	__rb_parent_color;	/**< parent node | color */
	struct eaf_map_low_node*	rb_right;			/**< right node */
	struct eaf_map_low_node*	rb_left;			/**< left node */
}eaf_map_low_node_t;

/**
 * @brief red-black tree
 * @see EAF_MAP_LOW_INITIALIZER
 */
typedef struct eaf_map_low
{
	eaf_map_low_node_t*			rb_root;			/**< root node */
}eaf_map_low_t;

/**
 * @brief Returns an iterator to the beginning
 * @param root		The pointer to the map
 * @return			An iterator
 */
EAF_API eaf_map_low_node_t* eaf_map_low_first(_In_ const eaf_map_low_t* root)
	EAF_ATTRIBUTE_NONNULL(1)
	EAF_ATTRIBUTE_NOTHROW;

/**
 * @brief Returns an iterator to the end
 * @param root		The pointer to the map
 * @return			An iterator
 */
EAF_API eaf_map_low_node_t* eaf_map_low_last(_In_ const eaf_map_low_t* root)
	EAF_ATTRIBUTE_NONNULL(1)
	EAF_ATTRIBUTE_NOTHROW;

/**
 * @brief Get an iterator next to the given one.
 * @param node		Current iterator
 * @return			Next iterator
 */
EAF_API eaf_map_low_node_t* eaf_map_low_next(_In_ const eaf_map_low_node_t* node)
	EAF_ATTRIBUTE_NONNULL(1)
	EAF_ATTRIBUTE_NOTHROW;

/**
 * @brief Get an iterator before the given one.
 * @param node		Current iterator
 * @return			Previous iterator
 */
EAF_API eaf_map_low_node_t* eaf_map_low_prev(_In_ const eaf_map_low_node_t* node)
	EAF_ATTRIBUTE_NONNULL(1)
	EAF_ATTRIBUTE_NOTHROW;

/**
 * @brief Inserting data into the tree.
 *
 * The insert instead must be implemented
 * in two steps: First, the code must insert the element in order as a red leaf
 * in the tree, and then the support library function #eaf_map_low_insert_color
 * must be called.
 *
 * @param node		The node you want to insert
 * @param parent	The position you want to insert
 * @param rb_link	Will be set to `node`
 * @see eaf_map_low_insert_color
 */
EAF_API void eaf_map_low_link_node(_Inout_ eaf_map_low_node_t* node,
	_Inout_opt_ eaf_map_low_node_t* parent, _Outptr_ eaf_map_low_node_t** rb_link)
	EAF_ATTRIBUTE_NONNULL(1, 2, 3)
	EAF_ATTRIBUTE_NOTHROW;

/**
 * @brief re-balancing ("recoloring") the tree.
 * @param node		The node just linked
 * @param root		The map
 * @see eaf_map_low_link_node
 */
EAF_API void eaf_map_low_insert_color(_Inout_ eaf_map_low_node_t* node,
	_Inout_ eaf_map_low_t* root)
	EAF_ATTRIBUTE_NONNULL(1, 2)
	EAF_ATTRIBUTE_NOTHROW;

/**
 * @brief Delete the node from the map.
 * @warning The node must already in the map.
 * @param root		The pointer to the map
 * @param node		The node
 */
EAF_API void eaf_map_low_erase(_Inout_ eaf_map_low_t* root,
	_Inout_ eaf_map_low_node_t* node)
	EAF_ATTRIBUTE_NONNULL(1, 2)
	EAF_ATTRIBUTE_NOTHROW;

/**
 * @}
 */

#ifdef __cplusplus
};
#endif	/* __cplusplus */
#endif
