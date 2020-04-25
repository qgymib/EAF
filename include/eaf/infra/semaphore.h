/** @file
 * Semaphore operations.
 */
#ifndef __EAF_INFRA_SEMAPHORE_H__
#define __EAF_INFRA_SEMAPHORE_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "eaf/utils/annotations.h"

/**
 * @brief Semaphore instance
 */
typedef struct eaf_sem eaf_sem_t;

/**
 * @brief Create a semaphore
 * @param count		initial count
 * @return			semaphore handler
 */
eaf_sem_t* eaf_sem_create(_In_ unsigned long count);

/**
 * @brief Destroy a semaphore
 * @param handler	semaphore handler
 */
void eaf_sem_destroy(_Inout_ eaf_sem_t* handler);

/**
 * @brief Decrements (locks) the semaphore pointed to by handler.
 * @param handler	semaphore handler
 * @param timeout	timeout in milliseconds
 * @return			eaf_errno
 */
int eaf_sem_pend(_Inout_ eaf_sem_t* handler, _In_ unsigned long timeout);

/**
 * @brief Increments (unlocks) the semaphore pointed to by handler.
 * @param handler	semaphore handler
 * @return			eaf_errno
 */
int eaf_sem_post(_Inout_ eaf_sem_t* handler);

#ifdef __cplusplus
}
#endif
#endif
