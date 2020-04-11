#ifndef __EAF_PLUGIN_DETAIL_MSG_H__
#define __EAF_PLUGIN_DETAIL_MSG_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include "EAF/core/service.h"

/**
* �������󲢽�����Ӧ
* @param ret	eaf_msg_t*����Ӧ��Ϣ��ʹ�����֮����Ҫ����
* @param to		���շ�����ID
* @param id		��ϢID
* @param dat	��Ϣ����
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
* ���ٷ�����Ӧ
* @param ret	int�����ͽ��
* @param from	������ID
* @param req	������Ϣ
* @param dat	������
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
* ��Ϊ���ͷ���
* @param TYPE	ԭʼ����
* @param msg	���ݵ�ַ
* @return		TYPE*
*/
#define EAF_PLUGIN_MSG_ACCESS(TYPE, msg)	(*(TYPE*)eaf_msg_get_data(msg, NULL))

/**
* ����ִ�к���
* @param msg	��Ӧ
*/
void eaf_plugin_msg_proxy_handle(eaf_msg_t* msg);

/**
* �������������Ƿ�ɹ���msg���ᱻ����
* @param from	���ͷ�
* @param to		���շ�
* @return		eaf_errno
*/
int eaf_plugin_msg_send_req(uint32_t from, uint32_t to, eaf_msg_t* msg);

/**
* ��ȡ��Ӧ��Ϣ
* @param from	���ͷ�ID
* @return		��Ӧ
*/
eaf_msg_t* eaf_plugin_msg_get_rsp(uint32_t from);

#ifdef __cplusplus
}
#endif
#endif
