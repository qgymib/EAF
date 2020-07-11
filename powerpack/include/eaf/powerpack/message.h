/** @file
 * Enhance message operations
 */
#ifndef __EAF_POWERPACK_MESSAGE_H__
#define __EAF_POWERPACK_MESSAGE_H__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup PowerPack
 * @defgroup PowerPack-Message Message
 * @{
 */

#include "eaf/eaf.h"
#include "eaf/powerpack/define.h"

/**
* @brief Send request and wait for response.
* @param[out] _rsp	a pointer to store response. remember to destroy it
* @param[in] _from	the sender's service id
* @param[in] _to	the receiver's service id
* @param[in] _req	the request
* @param[in] _dec	the number of reference you want to decrease
*/
#define eaf_send_req_sync(_rsp, _from, _to, _req, _dec)	\
	do {\
		(_req)->from = (_from);\
		_eaf_local->unsafe[0].ww.w1 = _to;\
		_eaf_local->unsafe[0].ww.w2 = _dec;\
		eaf_yield_ext(eaf_powerpack_message_commit, _req);\
		(_rsp) = (eaf_msg_t*)(_eaf_local->unsafe[0].v_ptr);\
	} while (0)

/**
* @brief Generate a request, send to peer, and parse into data
* @param[out] ret		Operation result
* @param[in,out] p_dat	Pointer to store response
* @param[in] to			The service which will receive the request
* @param[in] MSG_ID		Message ID
* @param[in] TYPE		Message type
* @param[in] ...		Message fields
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
			EAF_MSVC_PUSH_WARNNING(4204)\
			*(TYPE*)eaf_msg_get_data(req, NULL) = (TYPE){ __VA_ARGS__ };\
			EAF_MSVC_POP_WARNNING()\
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
 * @brief A macro to help create and send request
 * @param[out] ret		A integer to store result
 * @param[in] msg_id	Message ID
 * @param[in] msg_size	The size of message
 * @param[in] rsp_fn	The function for handle response
 * @param[in] from		The sender Service ID
 * @param[in] to		The receiver Service ID
 * @param[in] code		The code to fill message
 */
#define EAF_SEND_REQUEST(ret, msg_id, msg_size, rsp_fn, from, to, code)	\
	do {\
		eaf_msg_t* _1 = eaf_msg_create_req(msg_id, msg_size, rsp_fn);\
		if (_1 == NULL) {\
			ret = eaf_errno_memory;\
			break;\
		}\
		{ code };\
		ret = eaf_send_req(from, to, _1);\
		eaf_msg_dec_ref(_1);\
	} while (0)

/**
 * @brief Internal operation for #eaf_send_req_sync
 * @note This function should only for internal usage.
 * @see eaf_send_req_sync
 * @param[in,out] local	service local storage
 * @param[in,out] arg	eaf_msg_t*
 */
void eaf_powerpack_message_commit(_Inout_ eaf_service_local_t* local,
	_Inout_opt_ void* arg);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
#endif
