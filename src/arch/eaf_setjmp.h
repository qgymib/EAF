#ifndef __EAF_ARCH_SETJMP_INTERNAL_H__
#define __EAF_ARCH_SETJMP_INTERNAL_H__
#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

#include "asm_eaf_jmpbuf.h"

struct eaf_jmpbuf;
typedef struct eaf_jmpbuf eaf_jmpbuf_t;

/**
* 保存上下文
* @note EAF的实现会额外保存返回地址指向的指令
* @param env	保存环境
* @return		0：首次返回；<0：二次返回
*/
int eaf_setjmp(eaf_jmpbuf_t* env);

/**
* 跳转
* @note EAF的实现会额外恢复返回地址指向的指令
* @param env	跳转环境
* @param val	值
*/
void eaf_longjmp(eaf_jmpbuf_t* env, int val);

#ifdef __cplusplus
};
#endif	/* __cplusplus */
#endif	/* __EAF_ARCH_SETJMP_INTERNAL_H__ */
