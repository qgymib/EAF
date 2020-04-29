/** @file
 * eaf_msg_t is a structure used by services to communicate to each other.
 */
#ifndef __EAF_CORE_MESSAGE_H__
#define __EAF_CORE_MESSAGE_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "eaf/utils/annotations.h"

struct eaf_msg;

/**
 * @brief The type of eaf_msg_t
 * @see eaf_msg_t
 */
typedef enum eaf_msg_type
{
	eaf_msg_type_req,					/**< Request */
	eaf_msg_type_rsp,					/**< Response */
}eaf_msg_type_t;

/**
 * @brief Prototype for request handler
 * @param[in,out] req		Request
 */
typedef void(*eaf_req_handle_fn)(_Inout_ struct eaf_msg* req);

/**
 * @brief Prototype for response handler
 * @warning
 * You have to treat receipt very carefully, a non-success receipt means your
 * request was not delivered.
 * If receipt is not #eaf_msg_receipt_success, `rsp` will be a empty message, which means:
 *  + `rsp` is NOT NULL
 *  + #eaf_msg_get_data for `rsp` will return NULL
 *  + rsp->info.rr == req->info.rr
 * @note
 * If a request was sent successfully and no response received, it most likely
 * receiver not response your request.
 * @see eaf_msg_receipt
 * @param[in] receipt		Receipt for request, see #eaf_errno
 * @param[in,out] rsp		Response
 */
typedef void(*eaf_rsp_handle_fn)(_In_ int receipt, _Inout_ struct eaf_msg* rsp);

/**
 * @brief Communicate structure
 */
typedef struct eaf_msg
{
	uint32_t					id;		/**< Message ID */
	uint32_t					from;	/**< The service ID of sender */

	struct
	{
		struct
		{
			uint64_t			encs;	/**< Encoded secret, automatically filled by EAF. User must NOT modify this field. */
		}dynamics;						/**< Dynamics across request and response */
		struct
		{
			uint64_t			uuid;	/**< Universally Unique Identifier, automatically filled by EAF. User should NOT modify this field. */
			uint64_t			orig;	/**< Original Request Informations, automatically filled by EAF. User should NOT modify this field. */
		}constant;						/**< Constant across request and response */
	}info;								/**< Information collection */
}eaf_msg_t;

/**
 * @brief Create request.
 * @param[in] msg_id	Message ID
 * @param[in] size		The size of user structure
 * @param[in] rsp_fn	The response handler for the response
 * @return				The pointer of the request
 */
eaf_msg_t* eaf_msg_create_req(_In_ uint32_t msg_id, _In_ size_t size,
	_In_ eaf_rsp_handle_fn rsp_fn);

/**
 * @brief Create response.
 * @note This does not affect the reference count of original request.
 * @param[in] req		The original request
 * @param[in] size		The size of user structure
 * @return				The pointer of the response
 */
eaf_msg_t* eaf_msg_create_rsp(_In_ eaf_msg_t* req, _In_ size_t size);

/**
 * @brief Add reference count
 * @param[in,out] msg		The message you want to add reference
 */
void eaf_msg_add_ref(_Inout_ eaf_msg_t* msg);

/**
 * @brief Reduce reference count
 * @param[in,out] msg		The message you want to reduce reference
 */
void eaf_msg_dec_ref(_Inout_ eaf_msg_t* msg);

/**
 * @brief Get user structure address.
 * @param[in] msg		The message
 * @param[out] size		The size of user structure
 * @return				The address of user structure
 */
void* eaf_msg_get_data(_In_ eaf_msg_t* msg, _Out_opt_ size_t* size);

/**
 * @brief Get message type
 * @param[in] msg		Message
 * @return				#eaf_msg_type
 */
eaf_msg_type_t eaf_msg_get_type(_In_ const eaf_msg_t* msg);

/**
 * @brief Replace exist response handler with given one.
 * @warning Do NOT use this function unless you know why.
 * @param[in,out] msg	The message
 * @param[in] fn		Response handler
 */
void eaf_msg_set_rsp_fn(_Inout_ eaf_msg_t* msg, _In_ eaf_rsp_handle_fn fn);

/**
 * @brief Get response handler
 * @param[in] msg	The message
 * @return			The response handler
 */
eaf_rsp_handle_fn eaf_msg_get_rsp_fn(_In_ const eaf_msg_t* msg);

#ifdef __cplusplus
}
#endif
#endif  /* __EAF_CORE_MESSAGE_H__ */
