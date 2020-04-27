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
 * @param[in] req		Request
 */
typedef void(*eaf_req_handle_fn)(_Inout_ struct eaf_msg* req);

/**
 * @brief Prototype for response handler
 * @param[in] rsp		Response
 */
typedef void(*eaf_rsp_handle_fn)(_Inout_ struct eaf_msg* rsp);

/**
 * @brief Communicate structure
 */
typedef struct eaf_msg
{
	eaf_msg_type_t				type;	/**< Message type */
	uint32_t					id;		/**< Message ID */
	uint32_t					from;	/**< The service ID of sender */

	struct
	{
		struct
		{
			eaf_rsp_handle_fn	rfn;	/**< response handle function, automatically filled by EAF. user should not modify this field. */
			uintptr_t			orig;	/**< original request address, automatically filled by EAF. user should not modify this field. */
			uintptr_t			uid;	/**< resource id, not initialized by default. use at your wish. */
		}rr;							/**< information for request/response. */
	}info;								/**< information collection */
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
 * @param[in,out] msg		The message you want to add reference
 */
void eaf_msg_dec_ref(_Inout_ eaf_msg_t* msg);

/**
 * @brief Get user structure address.
 * @param[in] msg		The message
 * @param[in] size		The size of user structure
 * @return				The address of user structure
 */
void* eaf_msg_get_data(_In_ eaf_msg_t* msg, _Out_opt_ size_t* size);

#ifdef __cplusplus
}
#endif
#endif  /* __EAF_CORE_MESSAGE_H__ */
