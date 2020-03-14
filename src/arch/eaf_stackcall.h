#ifndef __EAF_ARCH_STACKCALL_INTERNAL_H__
#define __EAF_ARCH_STACKCALL_INTERNAL_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

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
* |             MAGIC              |
* | ============ TOP ============= |
*
* @param jmp	��׼��ַ
* @param fn		�û����������ͣ�void (*fn)(void* arg)
* @param arg	�û�����
*/
void eaf_asm_stackcall(struct eaf_jmpbuf* jmp, void(*fn)(void*), void* arg);

#ifdef __cplusplus
}
#endif
#endif
