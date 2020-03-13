#ifndef __EAF_ARCH_SETJMP_INTERNAL_H__
#define __EAF_ARCH_SETJMP_INTERNAL_H__
#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

#include "asm_eaf_jmpbuf.h"

struct eaf_jmpbuf;
typedef struct eaf_jmpbuf eaf_jmpbuf_t;

/**
* ����������
* @note EAF��ʵ�ֻ���Ᵽ�淵�ص�ַָ���ָ��
* @param env	���滷��
* @return		0���״η��أ�<0�����η���
*/
int eaf_setjmp(eaf_jmpbuf_t* env);

/**
* ��ת
* @note EAF��ʵ�ֻ����ָ����ص�ַָ���ָ��
* @param env	��ת����
* @param val	ֵ
*/
void eaf_longjmp(eaf_jmpbuf_t* env, int val);

#ifdef __cplusplus
};
#endif	/* __cplusplus */
#endif	/* __EAF_ARCH_SETJMP_INTERNAL_H__ */
