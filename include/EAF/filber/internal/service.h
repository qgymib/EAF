#ifndef __EAF_FILBER_INTERNAL_SERVICE_H__
#define __EAF_FILBER_INTERNAL_SERVICE_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "EAF/filber/internal/stack.h"

/**
* (内部接口)获取跳转上下文
* @return		上下文
*/
eaf_jmp_buf_t* eaf_service_get_jmpbuf(void);

/**
* (内部接口)上下文切换
*/
void eaf_filber_context_switch(void);

/**
* (内部接口)协程栈返回
*/
void eaf_filber_context_return(void);

#ifdef __cplusplus
}
#endif
#endif
