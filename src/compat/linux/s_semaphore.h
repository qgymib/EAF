#ifndef __EAF_COMPAT_SEMAPHORE_LINUX_INTERNAL_H__
#define __EAF_COMPAT_SEMAPHORE_LINUX_INTERNAL_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <semaphore.h>

struct eaf_sem
{
	sem_t	sem;
};

#ifdef __cplusplus
}
#endif
#endif	/* __EAF_COMPAT_SEMAPHORE_LINUX_INTERNAL_H__ */
