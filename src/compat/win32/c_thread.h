#ifndef __EAF_COMPAT_THREAD_WIN32_INTERNAL_H__
#define __EAF_COMPAT_THREAD_WIN32_INTERNAL_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>

struct eaf_thread
{
	HANDLE	thr;
	void	(*proc)(void* arg);
	void*	priv;
};

struct eaf_thread_storage
{
	DWORD	storage;
};

#ifdef __cplusplus
}
#endif
#endif
