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
	jmp_buf		env;	/** ��ת������ */
}eaf_jmp_buf_t;

/**
* ����jmp_buf�ڸ��������λ��
* @param addr	�ڴ�����
* @param size	�ڴ��С
* @return		��תλ��
*/
eaf_jmp_buf_t* eaf_stack_calculate_jmpbuf(void* addr, size_t size);

/**
* ��ȡ��ת������
* @return		������
*/
eaf_jmp_buf_t* eaf_service_get_jmpbuf(void);

/**
* �������л�
*/
EAF_NORETURN
void eaf_filber_context_switch(void);

/**
* Э��ջ����
*/
EAF_NORETURN
void eaf_filber_context_return(void);

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
EAF_NORETURN
void eaf_asm_stackcall(eaf_jmp_buf_t* jmp, void(*fn)(void*), void* arg);

#ifdef __cplusplus
}
#endif
#endif
