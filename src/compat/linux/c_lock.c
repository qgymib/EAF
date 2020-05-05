#include "eaf/utils/errno.h"
#define _GNU_SOURCE
#include "compat/lock.h"

int eaf_compat_lock_init(eaf_compat_lock_t* handler, eaf_lock_attr_t attr)
{
	pthread_mutexattr_t	mutex_attr;
	if (pthread_mutexattr_init(&mutex_attr) < 0)
	{
		return eaf_errno_unknown;
	}

	if (attr == eaf_lock_attr_recursive)
	{
		pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
	}

	int ret = eaf_errno_success;
	if (pthread_mutex_init(&handler->obj, &mutex_attr) < 0)
	{
		ret = eaf_errno_unknown;
	}
	pthread_mutexattr_destroy(&mutex_attr);

	return ret;
}

void eaf_compat_lock_exit(eaf_compat_lock_t* handler)
{
	pthread_mutex_destroy(&handler->obj);
}

int eaf_compat_lock_enter(eaf_compat_lock_t* handler)
{
	return pthread_mutex_lock(&handler->obj);
}

int eaf_compat_lock_leave(eaf_compat_lock_t* handler)
{
	return pthread_mutex_unlock(&handler->obj);
}
