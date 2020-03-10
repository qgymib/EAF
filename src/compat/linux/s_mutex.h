#ifndef __EAF_COMPAT_MUTEX_LINUX_INTERNAL_H__
#define __EAF_COMPAT_MUTEX_LINUX_INTERNAL_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>

struct eaf_mutex
{
	pthread_mutex_t	obj;
};

#ifdef __cplusplus
}
#endif
#endif
