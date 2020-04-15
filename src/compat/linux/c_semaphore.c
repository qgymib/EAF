#include <errno.h>
#include "EAF/utils/errno.h"
#include "compat/thread.h"
#include "compat/semaphore.h"

int eaf_compat_sem_init(eaf_compat_sem_t* handler, unsigned long count)
{
	return sem_init(&handler->sem, 0, count);
}

void eaf_compat_sem_exit(eaf_compat_sem_t* handler)
{
	sem_destroy(&handler->sem);
}

int eaf_compat_sem_pend(eaf_compat_sem_t* handler, unsigned long timeout)
{
	int ret = 0;
	if (timeout == (unsigned)-1)
	{
		while ((ret = sem_wait(&handler->sem)) != 0 && errno == EINTR);
		return ret == 0 ? eaf_errno_success : eaf_errno_unknown;
	}

	if (timeout == 0)
	{
		return sem_trywait(&handler->sem) == 0 ? 0 : -1;
	}

	unsigned times = (timeout + 9) / 10;
	while (((ret = sem_trywait(&handler->sem)) != 0) && (times-- > 0))
	{
		eaf_compat_thread_sleep(10);
	}

	return ret == 0 ? eaf_errno_success : eaf_errno_timeout;
}

int eaf_compat_sem_post(eaf_compat_sem_t* handler)
{
	return sem_post(&handler->sem);
}
