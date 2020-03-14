#ifndef __EAF_FILBER_INTERNAL_JMPBUF_H__
#define __EAF_FILBER_INTERNAL_JMPBUF_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

struct eaf_jmpbuf;

/**
* 获取jmpbuf大小
* @return		sizeof(struct eaf_jmpbuf)
*/
size_t eaf_jmpbuf_size(void);

/**
* 保存上下文
* @param jmpbuf	上下文
* @return		0：首次返回；!0：二次返回
*/
extern int eaf_asm_setjmp(struct eaf_jmpbuf* jmpbuf);

#ifdef __cplusplus
}
#endif
#endif
