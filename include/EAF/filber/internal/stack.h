#ifndef __EAF_FILBER_INTERNAL_STACK_H__
#define __EAF_FILBER_INTERNAL_STACK_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <setjmp.h>

typedef struct eaf_jmp_buf
{
	jmp_buf		env;
}eaf_jmp_buf_t;

eaf_jmp_buf_t* eaf_stack_calculate_jmpbuf(void* addr, size_t size);

/**
* ��ָ��ջ�е��ú���
*
* ջ�ռ������¹��ɣ�
* | =========== BOTTOM =========== |
* |            jmp_buf             |
* |  ++++++++++++++++++++++++++++  |
* |  +                          +  |
* |  +       user_content       +  |
* |  +                          +  |
* |  ++++++++++++++++++++++++++++  |
* | ============ TOP ============= |
*
* @param jmp	��׼��ַ
* @param fn		�û����������ͣ�void (*fn)(void* arg)
* @param arg	�û�����
*/
extern void eaf_asm_stackcall(eaf_jmp_buf_t* jmp, void(*fn)(void*), void* arg);

#ifdef __cplusplus
}
#endif
#endif
