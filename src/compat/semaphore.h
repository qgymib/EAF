#ifndef __EAF_COMPAT_SEMAPHORE_INTERNAL_H__
#define __EAF_COMPAT_SEMAPHORE_INTERNAL_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "c_semaphore.h"

struct eaf_sem;
typedef struct eaf_sem eaf_sem_t;

/**
* �����ź���
* @param handler	���
* @param count		�ź�������
* @return			eaf_errno
*/
int eaf_sem_init(eaf_sem_t* handler, unsigned count);

/**
* �����ź���
* @param handler	���
*/
void eaf_sem_exit(eaf_sem_t* handler);

/**
* �����ź���
* @param handler	���
* @param ms			��ʱʱ�䡣-1�����õȴ���0���������ȴ�
* @return			eaf_errno
*/
int eaf_sem_pend(eaf_sem_t* handler, unsigned ms);

/**
* �����ź���
* @param handler	���
* @return			eaf_errno
*/
int eaf_sem_post(eaf_sem_t* handler);

#ifdef __cplusplus
}
#endif
#endif	/* __EAF_COMPAT_SEMAPHORE_INTERNAL_H__ */
