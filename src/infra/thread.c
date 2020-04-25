#include <stdlib.h>
#include "eaf/infra/thread.h"
#include "utils/memory.h"
#include "compat/thread.h"

struct eaf_thread
{
	eaf_compat_thread_t		thr;
};

eaf_thread_t* eaf_thread_create(_In_ const eaf_thread_attr_t* cfg, _In_ eaf_thread_fn fn, _Inout_opt_ void* arg)
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

void eaf_thread_destroy(_Inout_ eaf_thread_t* handler)
{
	eaf_compat_thread_exit(&handler->thr);
	EAF_FREE(handler);
}

unsigned long eaf_thread_id(void)
{
	return eaf_compat_thread_id();
}
