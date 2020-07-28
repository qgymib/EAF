#ifndef __EAF_CORE_MESSAGE_INTERNAL_H__
#define __EAF_CORE_MESSAGE_INTERNAL_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "eaf/core/message.h"
#include "eaf/utils/define.h"
#include "compat/lock.h"

/**
* ��ȡ����Ϣ��ʵ��ַ
*/
#define EAF_MSG_I2C(_msg)	EAF_CONTAINER_OF(_msg, eaf_msg_full_t, msg)

/**
* ��ȡ����Ϣ�����ַ
*/
#define EAF_MSG_C2I(_msg)	(&((_msg)->msg))

/**
 * Check if a message is request
 */
#define EAF_MSG_IS_REQ(msg)	((msg)->info.dynamics.encs & EAF_MSG_ENCS_REQ)

#define EAF_MSG_ENCS_REQ	(0x01 << 15)	/**< ENCS: REQ */

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4200)
#endif
typedef struct eaf_msg_full
{
	eaf_msg_t					msg;		/** ԭʼ���� */
	eaf_compat_lock_t			objlock;	/** ������ */

	struct
	{
		size_t					refcnt;		/** ���ü��� */
	}cnt;

	struct
	{
		size_t					size;		/** ���ݴ�С */
		char					data[];		/** ������ */
	}data;
}eaf_msg_full_t;
#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#ifdef __cplusplus
}
#endif
#endif
