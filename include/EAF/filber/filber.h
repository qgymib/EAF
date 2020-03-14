#ifndef __EAF_FILBER_FILBER_H__
#define __EAF_FILBER_FILBER_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

#include "EAF/filber/internal/service.h"

/**
* 让度执行权，直到显式resume
* @note	无栈协程，只能在顶层调用
*/
#define eaf_yield	\
	do {\
		if (setjmp(eaf_service_get_jmpbuf()->env) != 0) {\
			break;\
		}\
		eaf_filber_context_switch();\
	} while (0)

/**
* 从协程中返回
*/
#define eaf_return	\
		eaf_filber_context_return()

/**
* 允许指定服务继续执行
* @param srv_id	服务ID
* @return		eaf_errno
*/
int eaf_resume(uint32_t srv_id);

#ifdef __cplusplus
}
#endif
#endif  /* __EAF_FILBER_FILBER_H__ */
