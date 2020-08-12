/** @file
 * Semaphore operations.
 */
#ifndef __EAF_INFRA_SEMAPHORE_H__
#define __EAF_INFRA_SEMAPHORE_H__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup EAF-Infra
 * @defgroup EAF-Semaphore Semaphore
 * @{
 */

#include "eaf/utils/define.h"

/**
 * @brief Infinite timeout
 */
#define EAF_SEM_INFINITY	((unsigned long)-1)

/**
 * @brief Semaphore instance
 */
typedef struct eaf_sem eaf_sem_t;

/**
 * @brief Create a semaphore
 * @param[in] count	Initial count
 * @return			Semaphore handler
 */
EAF_API eaf_sem_t* eaf_sem_create(_In_ unsigned long count);

/**
 * @brief Destroy a semaphore
 * @param[in] handler	Semaphore handler
 */
EAF_API void eaf_sem_destroy(_Post_invalid_ eaf_sem_t* handler)
	EAF_ATTRIBUTE_NONNULL(1);

/**
 * @brief Decrements (locks) the semaphore pointed to by handler.
 * @param[in,out] handler	Semaphore handler
 * @param[in] timeout		Timeout in milliseconds
 * @return					#eaf_errno
 */
EAF_API int eaf_sem_pend(_Inout_ eaf_sem_t* handler, _In_ unsigned long timeout)
	EAF_ATTRIBUTE_NONNULL(1);

/**
 * @brief Increments (unlocks) the semaphore pointed to by handler.
 * @param[in,out] handler	Semaphore handler
 * @return					#eaf_errno
 */
EAF_API int eaf_sem_post(_Inout_ eaf_sem_t* handler)
	EAF_ATTRIBUTE_NONNULL(1);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
#endif
