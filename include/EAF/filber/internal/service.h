#ifndef __EAF_FILBER_INTERNAL_SERVICE_H__
#define __EAF_FILBER_INTERNAL_SERVICE_H__
#ifdef __cplusplus
extern "C" {
#endif

struct eaf_jmpbuf;

/**
* ��ȡ��ת������
* @return		������
*/
struct eaf_jmpbuf* eaf_service_get_jmpbuf(void);

/**
* �����������л�
*/
void eaf_service_context_switch(void);

#ifdef __cplusplus
}
#endif
#endif
