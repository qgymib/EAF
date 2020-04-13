#ifndef __EAF_CORE_MESSAGE_H__
#define __EAF_CORE_MESSAGE_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

struct eaf_msg;

typedef enum eaf_msg_type
{
	eaf_msg_type_req,					/** 请求 */
	eaf_msg_type_rsp,					/** 响应 */
	eaf_msg_type_evt,					/** 事件 */
}eaf_msg_type_t;

/**
* 请求消息处理函数
* @param msg		消息
*/
typedef void(*eaf_req_handle_fn)(struct eaf_msg* msg);

/**
* 响应消息处理函数
* @param msg		消息
*/
typedef void(*eaf_rsp_handle_fn)(struct eaf_msg* msg);

/**
* 事件处理函数
* @param msg		事件
* @param arg		自定义参数
*/
typedef void(*eaf_evt_handle_fn)(struct eaf_msg* msg, void* arg);

typedef struct eaf_msg
{
	eaf_msg_type_t				type;	/** 消息类型 */
	uint32_t					id;		/** 消息ID */
	uint32_t					from;	/** 发送者服务ID */
	uint32_t					to;		/** 接受者服务ID */

	struct
	{
		struct
		{
			eaf_rsp_handle_fn	rfn;	/** response handle function, automatically filled by EAF. user should not modify this field. */
			uintptr_t			orig;	/** original request address, automatically filled by EAF. user should not modify this field. */
			uintptr_t			uid;	/** resource id, not initialized by default. use at your wish. */
		}rr;							/** information for request/response. */
	}info;
}eaf_msg_t;

/**
 * 创建请求
 * @param msg_id	消息ID
 * @param size_t	消息大小
 * @param rsp_fn	处理响应消息函数
 * @return			消息句柄
 */
eaf_msg_t* eaf_msg_create_req(uint32_t msg_id, size_t size, eaf_rsp_handle_fn rsp_fn);

/**
 * 创建响应
 * @param msg_id	消息ID
 * @param size_t	消息大小
 * @return			消息句柄
 */
eaf_msg_t* eaf_msg_create_rsp(eaf_msg_t* req, size_t size);

/**
 * 创建事件
 * @param msg_id	消息ID
 * @param size_t	消息大小
 * @return			消息句柄
 */
eaf_msg_t* eaf_msg_create_evt(uint32_t evt_id, size_t size);

/**
 * 增加引用
 * @param msg		消息句柄
 */
void eaf_msg_add_ref(eaf_msg_t* msg);

/**
 * 减少引用
 * @param msg		消息句柄
 */
void eaf_msg_dec_ref(eaf_msg_t* msg);

/**
 * 获取数据地址
 * @param msg		消息句柄
 * @param size		数据大小
 * @return			数据地址
 */
void* eaf_msg_get_data(eaf_msg_t* msg, size_t* size);

#ifdef __cplusplus
}
#endif
#endif  /* __EAF_CORE_MESSAGE_H__ */
