#ifndef __EAF_CORE_MESSAGE_INTERNAL_H__
#define __EAF_CORE_MESSAGE_INTERNAL_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "EAF/core/message.h"
#include "EAF/utils/define.h"
#include "compat/mutex.h"

/**
* 获取部消息真实地址
*/
#define EAF_MSG_I2C(_msg)	EAF_CONTAINER_OF(_msg, eaf_msg_full_t, msg)

/**
* 获取部消息对外地址
*/
#define EAF_MSG_C2I(_msg)	(&((_msg)->msg))

typedef struct eaf_msg_full
{
	eaf_msg_t					msg;		/** 原始对象 */
	eaf_mutex_t					objlock;	/** 对象锁 */

	union
	{
		struct
		{
			eaf_rsp_handle_fn	rsp_fn;		/** 响应处理函数 */
		}req;

		struct
		{
			eaf_rsp_handle_fn	rsp_fn;		/** 响应处理函数 */
		}rsp;
	}info;

	struct
	{
		unsigned				refcnt;		/** 引用计数 */
	}cnt;

	struct
	{
		size_t					size;		/** 数据大小 */
		char					data[];		/** 数据体 */
	}data;
}eaf_msg_full_t;

#ifdef __cplusplus
}
#endif
#endif
