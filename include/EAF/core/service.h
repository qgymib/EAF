#ifndef __EAF_CORE_SERVICE_H__
#define __EAF_CORE_SERVICE_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "EAF/core/message.h"

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

/**
* 注册服务
* @param srv_id	服务ID
* @param info	服务信息。必须为全局变量
* @return		eaf_errno
*/
int eaf_service_register(uint32_t srv_id, const eaf_service_info_t* info);

/**
* 事件订阅
* @param srv_id	服务ID
* @param evt_id	事件ID
* @param fn		处理函数
* @param arg	自定义参数
* @return		eaf_errno
*/
int eaf_service_subscribe(uint32_t srv_id, uint32_t evt_id, eaf_evt_handle_fn fn, void* arg);

/**
* 取消事件订阅
* @param srv_id	服务ID
* @param evt_id	事件ID
* @param fn		处理函数
* @param arg	自定义参数
* @return		eaf_errno
*/
int eaf_service_unsubscribe(uint32_t srv_id, uint32_t evt_id, eaf_evt_handle_fn fn, void* arg);

/**
* 发送请求数据
* @param from	发送方服务ID
* @param to	接收方服务ID
* @param req	请求数据
* @return		eaf_errno
*/
int eaf_service_send_req(uint32_t from, uint32_t to, eaf_msg_t* req);

/**
* 发送响应数据
* @param from	发送方服务ID
* @param rsp	响应数据
* @return		eaf_errno
*/
int eaf_service_send_rsp(uint32_t from, eaf_msg_t* rsp);

/**
* 发送广播数据
* @param from	发送方服务ID
* @param evt	广播数据
* @return		eaf_errno
*/
int eaf_service_send_evt(uint32_t from, eaf_msg_t* evt);

#ifdef __cplusplus
}
#endif
#endif
