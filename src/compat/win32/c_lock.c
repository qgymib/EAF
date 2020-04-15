#include "EAF/utils/errno.h"
#include "compat/lock.h"

int eaf_compat_lock_init(eaf_compat_lock_t* handler, eaf_lock_attr_t attr)
{
	(void)attr;
	InitializeCriticalSection(&handler->obj);
	return eaf_errno_success;
}

void eaf_compat_lock_exit(eaf_compat_lock_t* handler)
{
	(void)handler;
}

int eaf_compat_lock_enter(eaf_compat_lock_t* handler)
{
	EnterCriticalSection(&handler->obj);
	return eaf_errno_success;
}

int eaf_compat_lock_leave(eaf_compat_lock_t* handler)
{
	LeaveCriticalSection(&handler->obj);
	return eaf_errno_success;
}
