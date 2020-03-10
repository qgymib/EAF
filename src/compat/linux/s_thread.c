#include <string.h>
#include <unistd.h>
#include "EAF/utils/errno.h"
#include "compat/thread.h"

static void* _eaf_thread_linux_proxy(void* params)
{
	eaf_thread_t* handler = params;
	handler->proc(handler->priv);

	return NULL;
}

int eaf_thread_init(eaf_thread_t* handler, const eaf_thread_attr_t* attr, eaf_thread_fn fn, void* arg)
{
	int ret = eaf_errno_success;

	pthread_attr_t thr_attr;
	if (pthread_attr_init(&thr_attr) < 0)
	{
		return eaf_errno_unknown;
	}

	if (attr != NULL && attr->stack_size != 0)
	{
		pthread_attr_setstacksize(&thr_attr, attr->stack_size);
	}

	handler->proc = fn;
	handler->priv = arg;
	if (pthread_create(&handler->thr, &thr_attr, _eaf_thread_linux_proxy, handler) != 0)
	{
		ret = eaf_errno_unknown;
		goto fin;
	}

fin:
	pthread_attr_destroy(&thr_attr);
	return ret;
}

void eaf_thread_exit(eaf_thread_t* handler)
{
	void* ret = NULL;
	pthread_join(handler->thr, &ret);
}

void eaf_thread_sleep(unsigned timeout)
{
	if (timeout == 0)
	{
		sched_yield();
		return;
	}

	usleep(timeout * 1000);
	return;
}
