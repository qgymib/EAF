#ifndef __EAF_FILBER_INTERNAL_JMPBUF_H__
#define __EAF_FILBER_INTERNAL_JMPBUF_H__
#ifdef __cplusplus
extern "C" {
#endif

struct eaf_jmpbuf;

/**
* ��ȡ��ת������
* @return		������
*/
struct eaf_jmpbuf* eaf_get_jmpbuf(void);

/**
* �����������л�
*/
void eaf_filber_context_switch(void);

/**
* ����������
* @param jmpbuf	������
* @return		0���״η��أ�!0�����η���
*/
extern int eaf_setjmp(struct eaf_jmpbuf* jmpbuf);

#ifdef __cplusplus
}
#endif
#endif
