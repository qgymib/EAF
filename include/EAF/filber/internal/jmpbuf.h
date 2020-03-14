#ifndef __EAF_FILBER_INTERNAL_JMPBUF_H__
#define __EAF_FILBER_INTERNAL_JMPBUF_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

struct eaf_jmpbuf;

/**
* ��ȡjmpbuf��С
* @return		sizeof(struct eaf_jmpbuf)
*/
size_t eaf_jmpbuf_size(void);

/**
* ����������
* @param jmpbuf	������
* @return		0���״η��أ�!0�����η���
*/
extern int eaf_asm_setjmp(struct eaf_jmpbuf* jmpbuf);

#ifdef __cplusplus
}
#endif
#endif
