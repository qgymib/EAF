#include <stdlib.h>
#include "EAF/infra/semaphore.h"
#include "utils/memory.h"
#include "compat/semaphore.h"

struct eaf_sem
{
	eaf_compat_sem_t	sem;
};

eaf_sem_t* eaf_sem_create(unsigned long count)
{
	eaf_sem_t* handler = EAF_MALLOC(sizeof(eaf_sem_t));
	if (handler == NULL)
	{
		return NULL;
	}

	if (eaf_compat_sem_init(&handler->sem, count) < 0)
	{
		EAF_FREE(handler);
		return NULL;
	}

	return handler;
}

void eaf_sem_destroy(eaf_sem_t* handler)
{
	eaf_compat_sem_exit(&handler->sem);
	EAF_FREE(handler);
}

int eaf_sem_pend(eaf_sem_t* handler, unsigned long ms)
{
	return eaf_compat_sem_pend(&handler->sem, ms);
}

int eaf_sem_post(eaf_sem_t* handler)
{
	return eaf_compat_sem_post(&handler->sem);
}
