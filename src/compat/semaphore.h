#ifndef __EAF_COMPAT_SEMAPHORE_INTERNAL_H__
#define __EAF_COMPAT_SEMAPHORE_INTERNAL_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "c_semaphore.h"

#define EAF_COMPAT_SEM_INFINITY	((unsigned long)-1)

struct eaf_compat_sem;
typedef struct eaf_compat_sem eaf_compat_sem_t;

/**
* �����ź���
* @param handler	���
* @param count		�ź�������
* @return			eaf_errno
*/
int eaf_compat_sem_init(eaf_compat_sem_t* handler, unsigned long count);

/**
* �����ź���
* @param handler	���
*/
void eaf_compat_sem_exit(eaf_compat_sem_t* handler);

/**
* �����ź���
* @param handler	���
* @param ms			��ʱʱ�䡣-1�����õȴ���0���������ȴ�
* @return			eaf_errno
*/
int eaf_compat_sem_pend(eaf_compat_sem_t* handler, unsigned long ms);

/**
* �����ź���
* @param handler	���
* @return			eaf_errno
*/
int eaf_compat_sem_post(eaf_compat_sem_t* handler);

#ifdef __cplusplus
}
#endif
#endif	/* __EAF_COMPAT_SEMAPHORE_INTERNAL_H__ */
