#ifndef __EAF_PLUGIN_DETAIL_MSG_H__
#define __EAF_PLUGIN_DETAIL_MSG_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include "EAF/core/service.h"

/**
* 发送请求并接受响应
* @param ret	eaf_msg_t*：响应消息，使用完成之后需要销毁
* @param to		接收方服务ID
* @param id		消息ID
* @param dat	消息内容
*/
#define EAF_PLUGIN_MSG_SEND_REQ(ret, from, to, id, dat)	\
	do {\
		eaf_msg_t* msg = eaf_msg_create_req(id, sizeof(dat), eaf_plugin_msg_proxy_handle);\
		if (msg == NULL) {\
			ret = NULL;\
			break;\
		}\
		memcpy(eaf_msg_get_data(msg, NULL), &dat, sizeof(dat));\
		if (eaf_plugin_msg_send_req(from, to, msg) < 0) {\
			ret = NULL;\
			break;\
		}\
		eaf_yield;\
		ret = eaf_plugin_msg_get_rsp(from);\
	} while (0)

/**
* 快速发送响应
* @param ret	int：发送结果
* @param from	发送者ID
* @param req	请求消息
* @param dat	数据体
*/
#define EAF_PLUGIN_MSG_SEND_RSP(ret, from, req, dat)	\
	do {\
		eaf_msg_t* msg = eaf_msg_create_rsp(req, sizeof(dat));\
		if(msg == NULL) {\
			ret = eaf_errno_memory;\
			break;\
		}\
		memcpy(eaf_msg_get_data(msg, NULL), &dat, sizeof(dat));\
		ret = eaf_send_rsp(from, msg);\
		eaf_msg_dec_ref(msg);\
	} while (0)

/**
* 作为类型访问
* @param TYPE	原始类型
* @param msg	数据地址
* @return		TYPE*
*/
#define EAF_PLUGIN_MSG_ACCESS(TYPE, msg)	(*(TYPE*)eaf_msg_get_data(msg, NULL))

/**
* 代理执行函数
* @param msg	响应
*/
void eaf_plugin_msg_proxy_handle(eaf_msg_t* msg);

/**
* 发送请求。无论是否成功，msg均会被销毁
* @param from	发送方
* @param to		接收方
* @return		eaf_errno
*/
int eaf_plugin_msg_send_req(uint32_t from, uint32_t to, eaf_msg_t* msg);

/**
* 获取响应消息
* @param from	发送方ID
* @return		响应
*/
eaf_msg_t* eaf_plugin_msg_get_rsp(uint32_t from);

#ifdef __cplusplus
}
#endif
#endif
