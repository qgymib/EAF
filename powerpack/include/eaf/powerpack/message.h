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
 * @brief Message Service ID
 */
#define EAF_MESSAGE_ID		(0x00030000)

/**
 * @brief A macro to help create and send request
 * @note This macro does not require Message Service is running.
 * @param[out] ret		A integer to store result
 * @param[in] msg_id	Message ID
 * @param[in] msg_size	The size of message
 * @param[in] rsp_fn	The function for handle response
 * @param[in] from		The sender Service ID
 * @param[in] to		The receiver Service ID
 * @param[in] code		The code to fill message
 */
#define EAF_MESSAGE_SEND_REQUEST(ret, msg_id, msg_size, rsp_fn, from, to, code)	\
	do {\
		eaf_msg_t* _0 = eaf_msg_create_req(msg_id, msg_size, rsp_fn);\
		if (_0 == NULL) {\
			ret = eaf_errno_memory;\
			break;\
		}\
		{ code };\
		ret = eaf_send_req(from, to, _0);\
		eaf_msg_dec_ref(_0);\
	} EAF_MSVC_WARNING_GUARD(4127, while (0))

/**
 * @brief A macro to help send request and parse response.
 *
 * Use `_0' to access request in `code_serialize', use `_1' to access response
 * in `code_deserialize'.
 *
 * @note This macro require Message Service is running.
 * @warning After this macro, context will be invalid.
 * @warning `_0' is only valid in `code_serialize', `_1' is only valid in
 *   `code_deserialize'.
 * @param[out] ret				The result
 * @param[in] msg_id			Request Message ID
 * @param[in] msg_size			The size of request message
 * @param[in] from				Who send this message
 * @param[in] to				Who receive this message
 * @param[in] code_serialize	Code to fill request
 * @param[in] code_deserialize	Code to parse response
 */
#define EAF_MESSAGE_CALL_FILBER(ret, msg_id, msg_size, from, to, code_serialize, code_deserialize)	\
	do {\
		eaf_msg_t* _0 = eaf_msg_create_req(msg_id, msg_size, eaf_message_internal_response_handler);\
		if (_0 == NULL) {\
			ret = eaf_errno_memory;\
			break;\
		}\
		{ code_serialize };\
		_eaf_local->unsafe[0].ww.w1 = from;\
		_eaf_local->unsafe[0].ww.w2 = to;\
		eaf_yield_ext(eaf_message_internal_proxy, _0); _0 = NULL;\
		eaf_msg_t* _1;\
		if ((ret = eaf_message_internal_finalize(_eaf_local->unsafe[0].v_uint64, &_1)) < 0) {\
			break;\
		}\
		{ code_deserialize };\
		eaf_msg_dec_ref(_1);\
	} EAF_MSVC_WARNING_GUARD(4127, while (0))

/**
 * @brief Initialize Message Service
 * @return	#eaf_errno
 */
int eaf_message_init(void);

/**
 * @brief Exit Message Service
 * @warning This function must be called after #eaf_cleanup().
 */
void eaf_message_exit(void);

/**
 * @private
 * @brief Send request within message service.
 * @param[in,out] local	Service local Information
 * @param[in,out] arg	Request message
 */
void eaf_message_internal_proxy(_Inout_ eaf_service_local_t* local, _Inout_opt_ void* arg);

/**
 * @private
 * @brief Response handler for user request.
 * @param[in] from		Who send response
 * @param[in] to		Who receive response
 * @param[in,out] msg	The response message
 */
void eaf_message_internal_response_handler(_In_ uint32_t from, _In_ uint32_t to, _Inout_ eaf_msg_t* msg);

/**
 * @private
 * @brief Get response and return operation result.
 * @param[in] uuid		uuid from Service Local Information
 * @param[out] rsp		The response message with reference count set to 1.
 * @return				Operation result.
 */
int eaf_message_internal_finalize(_In_ uint64_t uuid, _Out_ eaf_msg_t** rsp);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
#endif
