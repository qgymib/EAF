#ifndef __EAF_INFRA_SEMAPHORE_H__
#define __EAF_INFRA_SEMAPHORE_H__
#ifdef __cplusplus
extern "C" {
#endif

struct eaf_sem;
typedef struct eaf_sem eaf_sem_t;

/**
* Create semaphore
* @param count		initial count
* @return			semaphore handler
*/
eaf_sem_t* eaf_sem_create(unsigned long count);

/**
* Destroy semaphore
* @param handler	semaphore handler
*/
void eaf_sem_destroy(eaf_sem_t* handler);

/**
* Decrements (locks) the semaphore pointed to by handler.
* @param handler	semaphore handler
* @param timeout	timeout in milliseconds
* @return			eaf_errno
*/
int eaf_sem_pend(eaf_sem_t* handler, unsigned long timeout);

/**
* Increments (unlocks) the semaphore pointed to by handler.
* @param handler	semaphore handler
* @return			eaf_errno
*/
int eaf_sem_post(eaf_sem_t* handler);

#ifdef __cplusplus
}
#endif
#endif
