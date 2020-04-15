#ifndef __EAF_COMPAT_THREAD_LINUX_INTERNAL_H__
#define __EAF_COMPAT_THREAD_LINUX_INTERNAL_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>

struct eaf_compat_thread
{
	pthread_t		thr;
	void			(*proc)(void* arg);
	void*			priv;
};

struct eaf_thread_storage
{
	pthread_key_t	storage;
};

#ifdef __cplusplus
}
#endif
#endif	/* __EAF_COMPAT_THREAD_LINUX_INTERNAL_H__ */
