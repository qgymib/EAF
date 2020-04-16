#include <stdlib.h>
#include "EAF/infra/lock.h"
#include "utils/memory.h"
#include "compat/lock.h"

struct eaf_lock
{
	eaf_compat_lock_t	objlock;
};

eaf_lock_t* eaf_lock_create(eaf_lock_attr_t attr)
{
	eaf_lock_t* handler = EAF_MALLOC(sizeof(eaf_lock_t));
	if (handler == NULL)
	{
		return NULL;
	}

	if (eaf_compat_lock_init(&handler->objlock, attr) < 0)
	{
		EAF_FREE(handler);
		return NULL;
	}

	return handler;
}

void eaf_lock_destroy(eaf_lock_t* handler)
{
	eaf_compat_lock_exit(&handler->objlock);
	EAF_FREE(handler);
}

void eaf_lock_enter(eaf_lock_t* handler)
{
	eaf_compat_lock_enter(&handler->objlock);
}

void eaf_lock_leave(eaf_lock_t* handler)
{
	eaf_compat_lock_leave(&handler->objlock);
}