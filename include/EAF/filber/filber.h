#ifndef __EAF_FILBER_FILBER_H__
#define __EAF_FILBER_FILBER_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

#include "EAF/filber/internal/service.h"
#include "EAF/filber/internal/jmpbuf.h"

/**
* �ö�ִ��Ȩ��ֱ����ʽresume
* @note	��ջЭ�̣�ֻ���ڶ������
*/
#define eaf_filber_yield()	\
	do {\
		if (eaf_asm_setjmp(eaf_service_get_jmpbuf()) != 0) {\
			break;\
		}\
		eaf_filber_context_switch();\
	} while (0)

/**
* ��Э���з���
*/
#define eaf_filber_return	\
	do {\
		eaf_filber_context_return();\
	} while (0)

/**
* ����ָ���������ִ��
* @param srv_id	����ID
* @return		eaf_errno
*/
int eaf_filber_resume(uint32_t srv_id);

#ifdef __cplusplus
}
#endif
#endif  /* __EAF_FILBER_FILBER_H__ */
