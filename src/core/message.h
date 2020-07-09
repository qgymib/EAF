#ifndef __EAF_CORE_MESSAGE_INTERNAL_H__
#define __EAF_CORE_MESSAGE_INTERNAL_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "eaf/core/message.h"
#include "eaf/utils/define.h"
#include "compat/lock.h"

/**
* 获取部消息真实地址
*/
#define EAF_MSG_I2C(_msg)	EAF_CONTAINER_OF(_msg, eaf_msg_full_t, msg)

/**
* 获取部消息对外地址
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
	eaf_msg_t					msg;		/** 原始对象 */
	eaf_compat_lock_t			objlock;	/** 对象锁 */

	struct
	{
		size_t					refcnt;		/** 引用计数 */
	}cnt;

	struct
	{
		size_t					size;		/** 数据大小 */
		char					data[];		/** 数据体 */
	}data;
}eaf_msg_full_t;
#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#ifdef __cplusplus
}
#endif
#endif
