/** @file
 * Double Linked List.
 * Nodes organize as:
 * ```
 * |--------|--------|--------|--------|--------|--------|--------|
 *   HEAD ------------------------------------------------> TAIL
 *     front -------------------------------------------> back
 *       before -------------------------------------> after
 * ```
 */
#ifndef __EAF_UTILS_LIST_H__
#define __EAF_UTILS_LIST_H__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup EAF-Utils
 * @defgroup EAF-List List
 * @{
 */

#include "eaf/utils/define.h"

/**
 * @brief Static initializer for #eaf_list_t
 * @see eaf_list_t
 */
#define EAF_LIST_INITIALIZER		{ NULL, NULL, 0 }

/**
 * @brief Static initializer for #eaf_list_node_t
 * @see eaf_list_node_t
 */
#define EAF_LIST_NODE_INITIALIZER	{ NULL, NULL }

/**
 * @brief The list node.
 * This node must put in your struct.
 * @see EAF_LIST_NODE_INITIALIZER
 */
typedef struct eaf_list_node
{
	struct eaf_list_node*	p_after;	/**< Pointer to next node */
	struct eaf_list_node*	p_before;	/**< Pointer to previous node */
}eaf_list_node_t;

/**
 * @brief Double Linked List
 * @see EAF_LIST_INITIALIZER
 */
typedef struct eaf_list
{
	eaf_list_node_t*		head;		/**< Pointer to HEAD node */
	eaf_list_node_t*		tail;		/**< Pointer to TAIL node */
	size_t					size;		/**< The number of total nodes */
}eaf_list_t;

/**
 * @brief Initialize Double Linked List.
 * @param[out] handler	Pointer to list
 */
EAF_API void eaf_list_init(_Out_ eaf_list_t* handler)
	EAF_ATTRIBUTE_NONNULL(1)
	EAF_ATTRIBUTE_NOTHROW;

/**
 * @brief Insert a node to the head of the list.
 * @warning the node must not exist in any list.
 * @param[in,out] handler	Pointer to list
 * @param[in,out] node		Pointer to a new node
 */
EAF_API void eaf_list_push_front(_Inout_ eaf_list_t* handler,
	_Inout_ eaf_list_node_t* node)
	EAF_ATTRIBUTE_NONNULL(1, 2)
	EAF_ATTRIBUTE_NOTHROW;

/**
 * @brief Insert a node to the tail of the list.
 * @warning the node must not exist in any list.
 * @param[in,out] handler	Pointer to list
 * @param[in,out] node		Pointer to a new node
 */
EAF_API void eaf_list_push_back(_Inout_ eaf_list_t* handler,
	_Inout_ eaf_list_node_t* node)
	EAF_ATTRIBUTE_NONNULL(1, 2)
	EAF_ATTRIBUTE_NOTHROW;

/**
 * @brief Insert a node in front of a given node.
 * @warning the node must not exist in any list.
 * @param[in,out] handler	Pointer to list
 * @param[in,out] pos		Pointer to a exist node
 * @param[in,out] node		Pointer to a new node
 */
EAF_API void eaf_list_insert_before(_Inout_ eaf_list_t* handler,
	_Inout_ eaf_list_node_t* pos, _Inout_ eaf_list_node_t* node)
	EAF_ATTRIBUTE_NONNULL(1, 2, 3)
	EAF_ATTRIBUTE_NOTHROW;

/**
 * @brief Insert a node right after a given node.
 * @warning the node must not exist in any list.
 * @param[in,out] handler	Pointer to list
 * @param[in,out] pos		Pointer to a exist node
 * @param[in,out] node		Pointer to a new node
 */
EAF_API void eaf_list_insert_after(_Inout_ eaf_list_t* handler,
	_Inout_  eaf_list_node_t* pos, _Inout_ eaf_list_node_t* node)
	EAF_ATTRIBUTE_NONNULL(1, 2, 3)
	EAF_ATTRIBUTE_NOTHROW;

/**
 * @brief Delete a exist node
 * @warning The node must already in the list.
 * @param[in,out] handler	Pointer to list
 * @param[in,out] node		The node you want to delete
 */
EAF_API void eaf_list_erase(_Inout_ eaf_list_t* handler,
	_Inout_ eaf_list_node_t* node)
	EAF_ATTRIBUTE_NONNULL(1, 2)
	EAF_ATTRIBUTE_NOTHROW;

/**
 * @brief Get the number of nodes in the list.
 * @param[in] handler	Pointer to list
 * @return				The number of nodes
 */
EAF_API size_t eaf_list_size(_In_ const eaf_list_t* handler)
	EAF_ATTRIBUTE_NONNULL(1)
	EAF_ATTRIBUTE_NOTHROW;

/**
 * @brief Get the first node and remove it from the list.
 * @param[in,out] handler	Pointer to list
 * @return					The first node
 */
EAF_API eaf_list_node_t* eaf_list_pop_front(_Inout_ eaf_list_t* handler)
	EAF_ATTRIBUTE_NONNULL(1)
	EAF_ATTRIBUTE_NOTHROW;

/**
 * @brief Get the last node and remove it from the list.
 * @param[in,out] handler	Pointer to list
 * @return					The last node
 */
EAF_API eaf_list_node_t* eaf_list_pop_back(_Inout_ eaf_list_t* handler)
	EAF_ATTRIBUTE_NONNULL(1)
	EAF_ATTRIBUTE_NOTHROW;

/**
 * @brief Get the last node.
 * @param[in] handler	Pointer to list
 * @return				The first node
 */
EAF_API eaf_list_node_t* eaf_list_begin(_In_ const eaf_list_t* handler)
	EAF_ATTRIBUTE_NONNULL(1)
	EAF_ATTRIBUTE_NOTHROW;

/**
 * @brief Get the last node.
 * @param[in] handler	The handler of list
 * @return				The last node
 */
EAF_API eaf_list_node_t* eaf_list_end(_In_ const eaf_list_t* handler)
	EAF_ATTRIBUTE_NONNULL(1)
	EAF_ATTRIBUTE_NOTHROW;

/**
* @brief Get next node.
* @param[in] node	Current node
* @return			The next node
*/
EAF_API eaf_list_node_t* eaf_list_next(_In_ const eaf_list_node_t* node)
	EAF_ATTRIBUTE_NONNULL(1)
	EAF_ATTRIBUTE_NOTHROW;

/**
 * @brief Get previous node.
 * @param[in] node	current node
 * @return			previous node
 */
EAF_API eaf_list_node_t* eaf_list_prev(_In_ const eaf_list_node_t* node)
	EAF_ATTRIBUTE_NONNULL(1)
	EAF_ATTRIBUTE_NOTHROW;

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
#endif	/* __EAF_UTILS_LIST_INTERNAL_H__ */
