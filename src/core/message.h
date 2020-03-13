#ifndef __EAF_CORE_MESSAGE_INTERNAL_H__
#define __EAF_CORE_MESSAGE_INTERNAL_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "EAF/core/message.h"
#include "EAF/utils/define.h"
#include "compat/mutex.h"

/**
* ��ȡ����Ϣ��ʵ��ַ
*/
#define EAF_MSG_I2C(_msg)	EAF_CONTAINER_OF(_msg, eaf_msg_full_t, msg)

/**
* ��ȡ����Ϣ�����ַ
*/
#define EAF_MSG_C2I(_msg)	(&((_msg)->msg))

typedef struct eaf_msg_full
{
	eaf_msg_t					msg;		/** ԭʼ���� */
	eaf_mutex_t					objlock;	/** ������ */

	union
	{
		struct
		{
			eaf_rsp_handle_fn	rsp_fn;		/** ��Ӧ������ */
		}req;

		struct
		{
			eaf_rsp_handle_fn	rsp_fn;		/** ��Ӧ������ */
		}rsp;
	}info;

	struct
	{
		unsigned				refcnt;		/** ���ü��� */
	}cnt;

	struct
	{
		size_t					size;		/** ���ݴ�С */
		char					data[];		/** ������ */
	}data;
}eaf_msg_full_t;

#ifdef __cplusplus
}
#endif
#endif
