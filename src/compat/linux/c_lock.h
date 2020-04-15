#ifndef __EAF_COMPAT_MUTEX_LINUX_INTERNAL_H__
#define __EAF_COMPAT_MUTEX_LINUX_INTERNAL_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>

struct eaf_compat_lock
{
	pthread_mutex_t	obj;
};

#ifdef __cplusplus
}
#endif
#endif
