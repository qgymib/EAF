#ifndef __EAF_ARCH_STACKCALL_INTERNAL_H__
#define __EAF_ARCH_STACKCALL_INTERNAL_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

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
* |             MAGIC              |
* | ============ TOP ============= |
*
* @param jmp	基准地址
* @param fn		用户函数。类型：void (*fn)(void* arg)
* @param arg	用户参数
*/
void eaf_asm_stackcall(struct eaf_jmpbuf* jmp, void(*fn)(void*), void* arg);

#ifdef __cplusplus
}
#endif
#endif
