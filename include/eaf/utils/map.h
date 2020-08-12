/** @file
 * eaf_map is a high level wrapper for eaf_map_low
 * @see eaf_map_low
 */
#ifndef __EAF_UTILS_MAP_H__
#define __EAF_UTILS_MAP_H__
#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

/**
 * @ingroup EAF-Utils
 * @defgroup EAF-Map Map
 * @{
 */

#include <stddef.h>
#include "eaf/utils/map_low.h"

/**
 * @brief Static initializer for #eaf_map_t
 * @see eaf_map_t
 * @param[in] cmp	Compare function
 * @param[in] arg	Argument for compare function
 */
#define EAF_MAP_INITIALIZER(cmp, arg)	{ EAF_MAP_LOW_INITIALIZER, { cmp, arg }, 0 }

/**
 * @brief Static initializer for #eaf_map_node_t
 * @see eaf_map_node_t
 */
#define EAF_MAP_NODE_INITIALIZER		EAF_MAP_LOW_NODE_INITIALIZER

/**
 * @brief The node for map
 * @see eaf_map_t
 * @see EAF_MAP_NODE_INITIALIZER
 */
typedef eaf_map_low_node_t eaf_map_node_t;

/**
 * @brief Compare function.
 * @param key1	The key in the map
 * @param key2	The key user given
 * @param arg	User defined argument
 * @return		-1 if key1 < key2. 1 if key1 > key2. 0 if key1 == key2.
 */
typedef int(*eaf_map_cmp_fn)(_In_ const eaf_map_node_t* key1,
	_In_ const eaf_map_node_t* key2, _Inout_opt_ void* arg);

/**
 * @brief Map implemented as red-black tree
 * @see EAF_MAP_INITIALIZER
 */
typedef struct eaf_map
{
	eaf_map_low_t		map_low;	/**< Underlying structure */

	struct
	{
		eaf_map_cmp_fn	cmp;		/**< Pointer to compare function */
		void*			arg;		/**< User defined argument, which will passed to compare function */
	}cmp;							/**< Compare function data */

	size_t				size;		/**< The number of nodes */
}eaf_map_t;

/**
 * @brief Initialize the map referenced by handler.
 * @param handler	The pointer to the map
 * @param cmp		The compare function. Must not NULL
 * @param arg		User defined argument. Can be anything
 */
EAF_API void eaf_map_init(_Out_ eaf_map_t* handler, _In_ eaf_map_cmp_fn cmp,
	_Inout_opt_ void* arg)
	EAF_ATTRIBUTE_NONNULL(1, 2)
	EAF_ATTRIBUTE_NOTHROW;

/**
 * @brief Insert the node into map.
 * @warning the node must not exist in any map.
 * @param handler	The pointer to the map
 * @param node		The node
 * @return			0 if success, -1 otherwise
 */
EAF_API int eaf_map_insert(_Inout_ eaf_map_t* handler, _Inout_ eaf_map_node_t* node)
	EAF_ATTRIBUTE_NONNULL(1, 2);

/**
 * @brief Delete the node from the map.
 * @warning The node must already in the map.
 * @param handler	The pointer to the map
 * @param node		The node
 */
EAF_API void eaf_map_erase(_Inout_ eaf_map_t* handler, _Inout_ eaf_map_node_t* node)
	EAF_ATTRIBUTE_NONNULL(1, 2);

/**
 * @brief Get the number of nodes in the map.
 * @param handler	The pointer to the map
 * @return			The number of nodes
 */
EAF_API size_t eaf_map_size(_In_ const eaf_map_t* handler)
	EAF_ATTRIBUTE_NONNULL(1)
	EAF_ATTRIBUTE_NOTHROW;

/**
 * @brief Finds element with specific key
 * @param handler	The pointer to the map
 * @param key		The key
 * @return			An iterator point to the found node
 */
EAF_API eaf_map_node_t* eaf_map_find(_In_ const eaf_map_t* handler,
	_In_ const eaf_map_node_t* key)
	EAF_ATTRIBUTE_NONNULL(1, 2);

/**
 * @brief Returns an iterator to the first element not less than the given key
 * @param handler	The pointer to the map
 * @param key		The key
 * @return			An iterator point to the found node
 */
EAF_API eaf_map_node_t* eaf_map_find_lower(_In_ const eaf_map_t* handler,
	_In_ const eaf_map_node_t* key)
	EAF_ATTRIBUTE_NONNULL(1, 2);

/**
 * @brief Returns an iterator to the first element greater than the given key
 * @param handler	The pointer to the map
 * @param key		The key
 * @return			An iterator point to the found node
 */
EAF_API eaf_map_node_t* eaf_map_find_upper(_In_ const eaf_map_t* handler,
	_In_ const eaf_map_node_t* key)
	EAF_ATTRIBUTE_NONNULL(1, 2);

/**
 * @brief Returns an iterator to the beginning
 * @param handler	The pointer to the map
 * @return			An iterator
 */
EAF_API eaf_map_node_t* eaf_map_begin(_In_ const eaf_map_t* handler)
	EAF_ATTRIBUTE_NONNULL(1)
	EAF_ATTRIBUTE_NOTHROW;

/**
 * @brief Returns an iterator to the end
 * @param handler	The pointer to the map
 * @return			An iterator
 */
EAF_API eaf_map_node_t* eaf_map_end(_In_ const eaf_map_t* handler)
	EAF_ATTRIBUTE_NONNULL(1)
	EAF_ATTRIBUTE_NOTHROW;

/**
 * @brief Get an iterator next to the given one.
 * @param node		Current iterator
 * @return			Next iterator
 */
EAF_API eaf_map_node_t* eaf_map_next(_In_ const eaf_map_node_t* node)
	EAF_ATTRIBUTE_NONNULL(1)
	EAF_ATTRIBUTE_NOTHROW;

/**
 * @brief Get an iterator before the given one.
 * @param node		Current iterator
 * @return			Previous iterator
 */
EAF_API eaf_map_node_t* eaf_map_prev(_In_ const eaf_map_node_t* node)
	EAF_ATTRIBUTE_NONNULL(1)
	EAF_ATTRIBUTE_NOTHROW;

/**
 * @}
 */

#ifdef __cplusplus
};
#endif	/* __cplusplus */
#endif
