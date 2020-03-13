#ifndef __EAF_COMPAT_MUTEX_WIN32_INTERNAL_H__
#define __EAF_COMPAT_MUTEX_WIN32_INTERNAL_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>

struct eaf_mutex
{
	CRITICAL_SECTION	obj;
};

#ifdef __cplusplus
}
#endif
#endif
