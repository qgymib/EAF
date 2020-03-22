#ifndef __EAF_FILBER_INTERNAL_SERVICE_H__
#define __EAF_FILBER_INTERNAL_SERVICE_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <setjmp.h>
#include "EAF/utils/define.h"

typedef struct eaf_jmp_buf
{
	jmp_buf		env;	/** 跳转上下文 */
}eaf_jmp_buf_t;

/**
* 计算jmp_buf在给定区域的位置
* @param addr	内存区域
* @param size	内存大小
* @return		跳转位置
*/
eaf_jmp_buf_t* eaf_stack_calculate_jmpbuf(void* addr, size_t size);

/**
* 获取跳转上下文
* @return		上下文
*/
eaf_jmp_buf_t* eaf_service_get_jmpbuf(void);

/**
* 上下文切换
*/
EAF_NORETURN
void eaf_filber_context_switch(void);

/**
* 协程栈返回
*/
EAF_NORETURN
void eaf_filber_context_return(void);

/**
* 在指定栈中调用函数
*
* 栈空间由如下构成：
* | =========== BOTTOM =========== |
* |            jmp_buf             |
* |  ++++++++++++++++++++++++++++  |
* |  +                          +  |
* |  +       user_content       +  |
* |  +                          +  |
* |  ++++++++++++++++++++++++++++  |
* | ============ TOP ============= |
*
* @param jmp	基准地址
* @param fn		用户函数。类型：void (*fn)(void* arg)
* @param arg	用户参数
*/
EAF_NORETURN
void eaf_asm_stackcall(eaf_jmp_buf_t* jmp, void(*fn)(void*), void* arg);

#ifdef __cplusplus
}
#endif
#endif
