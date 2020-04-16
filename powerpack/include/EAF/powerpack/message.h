#ifndef __EAF_POWERPACK_MESSAGE_H__
#define __EAF_POWERPACK_MESSAGE_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "EAF/core/service.h"

/**
* Send request and wait for response.
* @param _rsp	a pointer to store response. remember to destroy it
* @param _from	the sender's service id
* @param _to	the receiver's service id
* @param _req	the request
* @param _dec	the number of reference you want to decrease
*/
#define eaf_send_req_sync(_rsp, _from, _to, _req, _dec)	\
	do {\
		(_req)->from = (_from); (_req)->to = (_to);\
		_eaf_local->unsafe.v_ulong = _dec;\
		eaf_yield_ext(eaf_powerpack_message_commit, req);\
		(_rsp) = (eaf_msg_t*)_eaf_local->unsafe.v_ptr;\
	} while (0)

/**
* Generate a request, send to peer, and parse into data
* @param ret		operation result
* @param p_dat		pointer to store response
* @param to			the service which will receive the request
* @param MSG_ID		message id
* @param TYPE		message type
* @param ...		message fields
*/
#define eaf_msg_call(ret, p_dat, to, MSG_ID, TYPE, ...)	\
	do {\
		eaf_msg_t* rsp;\
		eaf_msg_t* req = eaf_msg_create_req(MSG_ID, sizeof(TYPE), NULL);\
		if (req == NULL) {\
			(ret) = eaf_errno_memory;\
			break;\
		}\
		{\
			*(TYPE*)eaf_msg_get_data(req, NULL) = (TYPE){ __VA_ARGS__ };\
		}\
		eaf_send_req_sync(rsp, _eaf_local->id, to, req, 1);\
		if (rsp == NULL) {\
			(ret) = eaf_errno_unknown;\
			break;\
		}\
		ret = eaf_errno_success;\
		{\
			size_t size;\
			void* p_buffer = eaf_msg_get_data(rsp, &size);\
			memcpy(p_dat, p_buffer, size);\
		}\
		eaf_msg_dec_ref(rsp);\
	} while (0)

/**
* Internal operation for `eaf_send_req_sync`
* @param local	service local storage
* @param arg	eaf_msg_t*
*/
void eaf_powerpack_message_commit(eaf_service_local_t* local, void* arg);

#ifdef __cplusplus
}
#endif
#endif
