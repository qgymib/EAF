#ifndef __EAF_FILBER_INTERNAL_SERVICE_H__
#define __EAF_FILBER_INTERNAL_SERVICE_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "EAF/filber/internal/stack.h"

/**
* (�ڲ��ӿ�)��ȡ��ת������
* @return		������
*/
eaf_jmp_buf_t* eaf_service_get_jmpbuf(void);

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
