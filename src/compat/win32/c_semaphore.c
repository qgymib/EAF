#include "EAF/utils/errno.h"
#include "compat/semaphore.h"

int eaf_compat_sem_init(eaf_compat_sem_t* handler, unsigned long count)
{
	handler->sem = CreateSemaphore(NULL, count, LONG_MAX, NULL);
	if (handler->sem == NULL)
	{
		return eaf_errno_unknown;
	}
	return eaf_errno_success;
}

void eaf_compat_sem_exit(eaf_compat_sem_t* handler)
{
	CloseHandle(handler->sem);
}

int eaf_compat_sem_pend(eaf_compat_sem_t* handler, unsigned long timeout)
{
	return WaitForSingleObject(handler->sem, timeout == (unsigned long)-1 ? INFINITE : timeout)
		== WAIT_OBJECT_0 ? eaf_errno_success : eaf_errno_timeout;
}

int eaf_compat_sem_post(eaf_compat_sem_t* handler)
{
	return ReleaseSemaphore(handler->sem, 1, NULL) ? eaf_errno_success : eaf_errno_unknown;
}
