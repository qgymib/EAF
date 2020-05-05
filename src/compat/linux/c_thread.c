#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define _GNU_SOURCE
#define __USE_GNU
#include <pthread.h>
#include <sys/syscall.h>
#include <sched.h>
#include "eaf/utils/errno.h"
#include "compat/thread.h"

static void* _eaf_thread_linux_proxy(void* params)
{
	eaf_compat_thread_t* handler = params;
	handler->proc(handler->priv);

	return NULL;
}

int eaf_compat_thread_init(eaf_compat_thread_t* handler, const eaf_thread_attr_t* cfg, eaf_thread_fn fn, void* arg)
{
	int ret = eaf_errno_success;

	pthread_attr_t thr_attr;
	if (pthread_attr_init(&thr_attr) < 0)
	{
		return eaf_errno_unknown;
	}

	if (cfg != NULL && (cfg->valid & EAF_THREAD_VALID_STACKSIZE))
	{
		pthread_attr_setstacksize(&thr_attr, cfg->field.stacksize);
	}

	if (cfg != NULL && (cfg->valid & EAF_THREAD_VALID_PRIORITY))
	{
		struct sched_param sp;
		sp.sched_priority = (int)cfg->field.priority;
		pthread_attr_setschedparam(&thr_attr, &sp);
	}

	handler->proc = fn;
	handler->priv = arg;
	if (pthread_create(&handler->thr, &thr_attr, _eaf_thread_linux_proxy, handler) != 0)
	{
		ret = eaf_errno_unknown;
		goto fin;
	}

	if (cfg != NULL && (cfg->valid & EAF_THREAD_VALID_AFFINITY))
	{
        cpu_set_t* c_set;
		if ((c_set = malloc(sizeof(cpu_set_t))) == NULL)
		{
			goto fin;
		}
		CPU_ZERO(c_set);
		CPU_SET(cfg->field.affinity, c_set);
		pthread_setaffinity_np(handler->thr, sizeof(*c_set), c_set);
        free(c_set);
	}

fin:
	pthread_attr_destroy(&thr_attr);
	return ret;
}

void eaf_compat_thread_exit(eaf_compat_thread_t* handler)
{
	void* ret = NULL;
	pthread_join(handler->thr, &ret);
}

unsigned long eaf_compat_thread_id(void)
{
	return syscall(__NR_gettid);
}

void eaf_compat_thread_sleep(unsigned timeout)
{
	if (timeout == 0)
	{
		sched_yield();
		return;
	}

	usleep(timeout * 1000);
	return;
}

int eaf_thread_storage_init(eaf_thread_storage_t* handler)
{
	return pthread_key_create(&handler->storage, NULL) == 0 ? eaf_errno_success : eaf_errno_unknown;
}

void eaf_thread_storage_exit(eaf_thread_storage_t* handler)
{
	pthread_key_delete(handler->storage);
}

int eaf_thread_storage_set(eaf_thread_storage_t* handler, void* val)
{
	return pthread_setspecific(handler->storage, val) == 0 ? eaf_errno_success : eaf_errno_unknown;
}

void* eaf_thread_storage_get(eaf_thread_storage_t* handler)
{
	return pthread_getspecific(handler->storage);
}
