#ifndef __EAF_CORE_SERVICE_H__
#define __EAF_CORE_SERVICE_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "EAF/core/internal/service.h"
#include "EAF/core/message.h"

/**
* coroutine body
*/
#define eaf_reenter					EAF_COROUTINE_REENTER()

/**
* Suspend service until `eaf_resume` is called
*/
#define eaf_yield					eaf_yield_ext(NULL, NULL)

/**
* Like `eaf_yield`, service will suspend until `eaf_resume` is called,
* and when service was suspended, `fn` will be called immediately.
* `fn` is a function with proto `void(*)(uint32_t service_id, void* arg)`.
*/
#define eaf_yield_ext(fn, arg)		EAF_COROUTINE_YIELD(fn, arg, EAF_COROUTINE_YIELD_TOKEN)

typedef struct eaf_service_msgmap
{
	uint32_t						msg_id;			/** 请求消息ID */
	eaf_req_handle_fn				fn;				/** 消息处理函数 */
}eaf_service_msgmap_t;

typedef struct eaf_service_info
{
	size_t							msg_table_size;	/** 处理映射表大小 */
	const eaf_service_msgmap_t*		msg_table;		/** 处理映射表 */

	/**
	* 初始化回调
	* @param arg	自定义参数
	* @return		0：成功；<0：失败
	*/
	int (*on_init)(void);

	/**
	* 去初始化回调
	* @param arg	自定义参数
	*/
	void (*on_exit)(void);
}eaf_service_info_t;

typedef struct eaf_service_table
{
	uint32_t						srv_id;			/** 服务ID */
	uint32_t						msgq_size;		/** 消息队列大小 */
}eaf_service_table_t;

typedef struct eaf_thread_table
{
	uint16_t						proprity;		/** 线程优先级 */
	uint16_t						cpuno;			/** CPU核心亲和性 */
	uint32_t						stacksize;		/** 线程栈大小 */

	struct
	{
		size_t						size;			/** 配置表大小 */
		eaf_service_table_t*		table;			/** 配置表 */
	}service;
}eaf_thread_table_t;

/**
* 允许指定服务继续执行
* @param srv_id	服务ID
* @return		eaf_errno
*/
int eaf_resume(uint32_t srv_id);

/**
* 配置EAF平台
* @param info	信息列表。必须为全局变量
* @param size	列表长度
* @return		eaf_errno
*/
int eaf_setup(const eaf_thread_table_t* info /*static*/, size_t size);

/**
* 开启EAF平台
* 函数返回时，所有服务均已初始化完毕
* @return		eaf_errno
*/
int eaf_load(void);

/**
* 清理EAF平台
* @return		eaf_errno
*/
int eaf_cleanup(void);

/**
* 注册服务
* @param srv_id	服务ID
* @param info	服务信息。必须为全局变量
* @return		eaf_errno
*/
int eaf_register(uint32_t srv_id, const eaf_service_info_t* info /*static*/);

/**
* 事件订阅
* @param srv_id	服务ID
* @param evt_id	事件ID
* @param fn		处理函数
* @param arg	自定义参数
* @return		eaf_errno
*/
int eaf_subscribe(uint32_t srv_id, uint32_t evt_id, eaf_evt_handle_fn fn, void* arg);

/**
* 取消事件订阅
* @param srv_id	服务ID
* @param evt_id	事件ID
* @param fn		处理函数
* @param arg	自定义参数
* @return		eaf_errno
*/
int eaf_unsubscribe(uint32_t srv_id, uint32_t evt_id, eaf_evt_handle_fn fn, void* arg);

/**
* 发送请求数据
* @param from	发送方服务ID
* @param to	接收方服务ID
* @param req	请求数据
* @return		eaf_errno
*/
int eaf_send_req(uint32_t from, uint32_t to, eaf_msg_t* req);

/**
* 发送响应数据
* @param from	发送方服务ID
* @param rsp	响应数据
* @return		eaf_errno
*/
int eaf_send_rsp(uint32_t from, eaf_msg_t* rsp);

/**
* 发送广播数据
* @param from	发送方服务ID
* @param evt	广播数据
* @return		eaf_errno
*/
int eaf_send_evt(uint32_t from, eaf_msg_t* evt);

/**
* Get self's ID
* @return		service id
*/
uint32_t eaf_service_self(void);

#ifdef __cplusplus
}
#endif
#endif
