#ifndef __EAF_COMPAT_SEMAPHORE_WIN32_INTERNAL_H__
#define __EAF_COMPAT_SEMAPHORE_WIN32_INTERNAL_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>

struct eaf_sem
{
	HANDLE	sem;
};

#ifdef __cplusplus
}
#endif
#endif	/* __EAF_COMPAT_SEMAPHORE_WIN32_INTERNAL_H__ */
