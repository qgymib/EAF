#ifndef __EAF_FILBER_STACK_H__
#define __EAF_FILBER_STACK_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "EAF/filber/internal/jmpbuf.h"
#include "EAF/filber/internal/stack.h"

/**
* 在指定栈中调用函数
* @param addr	内存地址
* @param size	内存大小，必须为sizeof(void*)的整数倍
* @param proc	用户函数
* @param priv	用户参数
*/
#define eaf_stack_call(addr, size, fn, arg)	\
	do {\
		struct eaf_jmpbuf* p_buf = eaf_stack_calculate_jmpbuf(addr, size);\
		if (eaf_setjmp(p_buf) != 0) {\
			break;\
		}\
		eaf_asm_stackcall(p_buf, fn, arg);\
	} while (0)

#ifdef __cplusplus
}
#endif
#endif
