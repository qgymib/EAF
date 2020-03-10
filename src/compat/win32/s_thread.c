#include "EAF/utils/errno.h"
#include "compat/thread.h"

static DWORD WINAPI _eaf_thread_win32_proxy(LPVOID lpParam)
{
	eaf_thread_t* impl = lpParam;

	impl->proc(impl->priv);
	return 0;
}

int eaf_thread_init(eaf_thread_t* handler, const eaf_thread_attr_t* attr, eaf_thread_fn fn, void* arg)
{
	handler->proc = fn;
	handler->priv = arg;
	handler->thr = CreateThread(NULL, attr != NULL ? attr->stack_size : 0, _eaf_thread_win32_proxy, handler, 0, NULL);
	if (handler->thr == NULL)
	{
		return eaf_errno_unknown;
	}

	if (attr != NULL)
	{
		SetPriorityClass(handler->thr, attr->priority);
	}
	return eaf_errno_success;
}

void eaf_thread_exit(eaf_thread_t* handler)
{
	WaitForSingleObject(handler->thr, INFINITE);
	CloseHandle(handler->thr);
}

unsigned long eaf_thread_id(void)
{
	return GetCurrentThreadId();
}

void eaf_thread_sleep(unsigned timeout)
{
	if (timeout == 0)
	{
		SwitchToThread();
		return;
	}

	Sleep(timeout);
}
