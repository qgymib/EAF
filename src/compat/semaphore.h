#ifndef __EAF_COMPAT_SEMAPHORE_INTERNAL_H__
#define __EAF_COMPAT_SEMAPHORE_INTERNAL_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "c_semaphore.h"

struct eaf_sem;
typedef struct eaf_sem eaf_sem_t;

/**
* 创建信号量
* @param handler	句柄
* @param count		信号量计数
* @return			eaf_errno
*/
int eaf_sem_init(eaf_sem_t* handler, unsigned count);

/**
* 销毁信号量
* @param handler	句柄
*/
void eaf_sem_exit(eaf_sem_t* handler);

/**
* 减少信号量
* @param handler	句柄
* @param ms			超时时间。-1：永久等待；0：非阻塞等待
* @return			eaf_errno
*/
int eaf_sem_pend(eaf_sem_t* handler, unsigned ms);

/**
* 增加信号量
* @param handler	句柄
* @return			eaf_errno
*/
int eaf_sem_post(eaf_sem_t* handler);

#ifdef __cplusplus
}
#endif
#endif	/* __EAF_COMPAT_SEMAPHORE_INTERNAL_H__ */
