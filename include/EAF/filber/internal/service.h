#ifndef __EAF_FILBER_INTERNAL_SERVICE_H__
#define __EAF_FILBER_INTERNAL_SERVICE_H__
#ifdef __cplusplus
extern "C" {
#endif

struct eaf_jmpbuf;

/**
* (�ڲ��ӿ�)��ȡ��ת������
* @return		������
*/
struct eaf_jmpbuf* eaf_service_get_jmpbuf(void);

/**
* (�ڲ��ӿ�)�������л�
*/
void eaf_filber_context_switch(void);

/**
* (�ڲ��ӿ�)Э��ջ����
*/
void eaf_filber_context_return(void);

#ifdef __cplusplus
}
#endif
#endif
