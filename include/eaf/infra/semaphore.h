/** @file
 * Semaphore operations.
 */
#ifndef __EAF_INFRA_SEMAPHORE_H__
#define __EAF_INFRA_SEMAPHORE_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "eaf/utils/define.h"

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
EAF_API void eaf_sem_destroy(_Post_invalid_ eaf_sem_t* handler);

/**
 * @brief Decrements (locks) the semaphore pointed to by handler.
 * @param[in] handler	Semaphore handler
 * @param[in] timeout	Timeout in milliseconds
 * @return				#eaf_errno
 */
EAF_API int eaf_sem_pend(_Inout_ eaf_sem_t* handler, _In_ unsigned long timeout);

/**
 * @brief Increments (unlocks) the semaphore pointed to by handler.
 * @param[in] handler	Semaphore handler
 * @return				#eaf_errno
 */
EAF_API int eaf_sem_post(_Inout_ eaf_sem_t* handler);

#ifdef __cplusplus
}
#endif
#endif
