/** @file
 * Lock operations.
 */
#ifndef __EAF_INFRA_LOCK_H__
#define __EAF_INFRA_LOCK_H__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup EAF-Infra
 * @defgroup EAF-Lock Lock
 * @{
 */

#include "eaf/utils/define.h"

/**
 * @brief Lock instance
 */
typedef struct eaf_lock eaf_lock_t;

/**
 * @brief Lock attribute
 */
typedef enum eaf_lock_attr
{
	eaf_lock_attr_normal,		/**< Normal lock */
	eaf_lock_attr_recursive,	/**< Recursive lock */
}eaf_lock_attr_t;

/**
 * @brief Create a lock
 * @param[in] attr	Attribute
 * @return			Lock handler
 */
EAF_API eaf_lock_t* eaf_lock_create(_In_ eaf_lock_attr_t attr);

/**
 * @brief Destroy a lock
 * @param[in,out] handler	the lock you want to destroy
 */
EAF_API void eaf_lock_destroy(_Post_invalid_ eaf_lock_t* handler);

/**
 * @brief Enter critical section
 * @param[in,out] handler	The lock
 */
EAF_API void eaf_lock_enter(_Inout_ eaf_lock_t* handler);

/**
 * @brief Leave critical section
 * @param[in,out] handler	The lock
 */
EAF_API void eaf_lock_leave(_Inout_ eaf_lock_t* handler);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
#endif
