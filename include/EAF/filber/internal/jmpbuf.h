#ifndef __EAF_FILBER_INTERNAL_JMPBUF_H__
#define __EAF_FILBER_INTERNAL_JMPBUF_H__
#ifdef __cplusplus
extern "C" {
#endif

struct eaf_jmpbuf;

/**
* 获取跳转上下文
* @return		上下文
*/
struct eaf_jmpbuf* eaf_get_jmpbuf(void);

/**
* 进行上下文切换
*/
void eaf_filber_context_switch(void);

/**
* 保存上下文
* @param jmpbuf	上下文
* @return		0：首次返回；!0：二次返回
*/
extern int eaf_setjmp(struct eaf_jmpbuf* jmpbuf);

#ifdef __cplusplus
}
#endif
#endif
