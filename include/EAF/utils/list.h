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

#include <stddef.h>

/**
 * @brief The list node.
 * This node must put in your struct.
 */
typedef struct eaf_list_node
{
	struct eaf_list_node*	p_after;	/**< Pointer to next node */
	struct eaf_list_node*	p_before;	/**< Pointer to previous node */
}eaf_list_node_t;

/**
 * @brief Double Linked List
 */
typedef struct eaf_list
{
	eaf_list_node_t*		head;		/**< Pointer to HEAD node */
	eaf_list_node_t*		tail;		/**< Pointer to TAIL node */
	size_t					size;		/**< The number of total nodes */
}eaf_list_t;

/**
 * @brief Double Linked List Node Initializer.
 */
#define EAF_LIST_NODE_INITIALIZER	{ NULL, NULL }

/**
 * @brief Double Linked List Initializer.
 */
#define EAF_LIST_INITIALIZER		{ NULL, NULL, 0 }

/**
 * @brief Initialize Double Linked List.
 * @param handler	Pointer to list
 */
void eaf_list_init(eaf_list_t* handler);

/**
 * @brief Insert a node to the head of the list.
 * @warning the node must not exist in any list.
 * @param handler	Pointer to list
 * @param node		Pointer to a new node
 */
void eaf_list_push_front(eaf_list_t* handler, eaf_list_node_t* node);

/**
 * @brief Insert a node to the tail of the list.
 * @warning the node must not exist in any list.
 * @param handler	Pointer to list
 * @param node		Pointer to a new node
 */
void eaf_list_push_back(eaf_list_t* handler, eaf_list_node_t* node);

/**
 * @brief Insert a node in front of a given node.
 * @warning the node must not exist in any list.
 * @param handler	Pointer to list
 * @param pos		Pointer to a exist node
 * @param node		Pointer to a new node
 */
void eaf_list_insert_before(eaf_list_t* handler, eaf_list_node_t* pos, eaf_list_node_t* node);

/**
 * @brief Insert a node right after a given node.
 * @warning the node must not exist in any list.
 * @param handler	Pointer to list
 * @param pos		Pointer to a exist node
 * @param node		Pointer to a new node
 */
void eaf_list_insert_after(eaf_list_t* handler, eaf_list_node_t* pos, eaf_list_node_t* node);

/**
 * @brief Delete a exist node
 * @warning The node must already in the list.
 * @param handler	Pointer to list
 * @param node		The node you want to delete
 */
void eaf_list_erase(eaf_list_t* handler, eaf_list_node_t* node);

/**
 * @brief Get the number of nodes in the list.
 * @param handler	Pointer to list
 * @return			The number of nodes
 */
size_t eaf_list_size(const eaf_list_t* handler);

/**
 * @brief Get the first node and remove it from the list.
 * @param handler	Pointer to list
 * @return			The first node
 */
eaf_list_node_t* eaf_list_pop_front(eaf_list_t* handler);

/**
 * @brief Get the last node and remove it from the list.
 * @param handler	Pointer to list
 * @return			The last node
 */
eaf_list_node_t* eaf_list_pop_back(eaf_list_t* handler);

/**
 * @brief Get the last node.
 * @param handler	Pointer to list
 * @return			The first node
 */
eaf_list_node_t* eaf_list_begin(const eaf_list_t* handler);

/**
 * @brief Get the last node.
 * @param handler	The handler of list
 * @return			The last node
 */
eaf_list_node_t* eaf_list_end(const eaf_list_t* handler);

/**
* @brief Get next node.
* @param handler	Pointer to list
* @param node		Current node
* @return			The next node
*/
eaf_list_node_t* eaf_list_next(const eaf_list_t* handler, const eaf_list_node_t* node);

/**
 * @brief Get previous node.
 * @param handler	the handler of list
 * @param node		current node
 * @return			previous node
 */
eaf_list_node_t* eaf_list_prev(const eaf_list_t* handler, const eaf_list_node_t* node);

#ifdef __cplusplus
}
#endif
#endif	/* __EAF_UTILS_LIST_INTERNAL_H__ */
