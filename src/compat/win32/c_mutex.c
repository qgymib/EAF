#include "EAF/utils/errno.h"
#include "compat/mutex.h"

int eaf_mutex_init(eaf_mutex_t* handler, eaf_mutex_attr_t attr)
{
	(void)attr;
	InitializeCriticalSection(&handler->obj);
	return eaf_errno_success;
}

void eaf_mutex_exit(eaf_mutex_t* handler)
{
	(void)handler;
}

int eaf_mutex_enter(eaf_mutex_t* handler)
{
	EnterCriticalSection(&handler->obj);
	return eaf_errno_success;
}

int eaf_mutex_leave(eaf_mutex_t* handler)
{
	LeaveCriticalSection(&handler->obj);
	return eaf_errno_success;
}
