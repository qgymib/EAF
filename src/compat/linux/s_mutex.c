#include "EAF/utils/errno.h"
#include "compat/mutex.h"

int eaf_mutex_init(eaf_mutex_t* handler, eaf_mutex_attr_t attr)
{
	pthread_mutexattr_t	mutex_attr;
	if (pthread_mutexattr_init(&mutex_attr) < 0)
	{
		return eaf_errno_unknown;
	}

	if (attr == eaf_mutex_attr_recursive)
	{
		pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE_NP);
	}

	int ret = eaf_errno_success;
	if (pthread_mutex_init(&handler->obj, &mutex_attr) < 0)
	{
		ret = eaf_errno_unknown;
	}
	pthread_mutexattr_destroy(&mutex_attr);

	return ret;
}

void eaf_mutex_exit(eaf_mutex_t* handler)
{
	pthread_mutex_destroy(&handler->obj);
}

int eaf_mutex_enter(eaf_mutex_t* handler)
{
	return pthread_mutex_lock(&handler->obj);
}

int eaf_mutex_leave(eaf_mutex_t* handler)
{
	return pthread_mutex_unlock(&handler->obj);
}
