#include <stdlib.h>
#include "EAF/infra/thread.h"
#include "utils/memory.h"
#include "compat/thread.h"

struct eaf_thread
{
	eaf_compat_thread_t		thr;
};

eaf_thread_t* eaf_thread_create(const eaf_thread_attr_t* cfg, eaf_thread_fn fn, void* arg)
{
	eaf_thread_t* handler = EAF_MALLOC(sizeof(eaf_thread_t));
	if (handler == NULL)
	{
		return NULL;
	}

	if (eaf_compat_thread_init(&handler->thr, cfg, fn, arg) < 0)
	{
		EAF_FREE(handler);
		return NULL;
	}

	return handler;
}

void eaf_thread_destroy(eaf_thread_t* handler)
{
	eaf_compat_thread_exit(&handler->thr);
	EAF_FREE(handler);
}
