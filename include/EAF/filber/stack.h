#ifndef __EAF_FILBER_STACK_H__
#define __EAF_FILBER_STACK_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "EAF/filber/internal/jmpbuf.h"
#include "EAF/filber/internal/stack.h"

/**
* ��ָ��ջ�е��ú���
* @param addr	�ڴ��ַ
* @param size	�ڴ��С������Ϊsizeof(void*)��������
* @param proc	�û�����
* @param priv	�û�����
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
