/** @file
 * eaf_msg_t is a structure used by services to communicate to each other.
 */
#ifndef __EAF_CORE_MESSAGE_H__
#define __EAF_CORE_MESSAGE_H__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup EAF-Core
 * @defgroup EAF-Message Message
 * @{
 */

#include <stdint.h>
#include <stddef.h>
#include "eaf/utils/define.h"

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
 * @brief Prototype for message handler
 * @warning
 * If msg is response, you have to treat receipt very carefully, a non-success
 * receipt means your request was not delivered. You can get receipt by
 * #eaf_msg_get_receipt().
 * If receipt is not #eaf_errno_success, `rsp` will be a empty message, which
 * means:
 *  + `rsp` is NOT NULL
 *  + #eaf_msg_get_data() for `rsp` will return NULL
 *  + rsp->info.rr == req->info.rr
 * @note
 * If a request was sent successfully and no response received, it most likely
 * receiver not response your request.
 * @see eaf_msg_receipt
 * @param[in] from		Who send this message
 * @param[in] to		Who will receive this message
 * @param[in,out] msg	The message
 */
typedef void(*eaf_msg_handle_fn)(_In_ uint32_t from, _In_ uint32_t to, _Inout_ struct eaf_msg* msg);

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
			/**
			 * @brief Encoded secret, automatically filled by EAF
			 *
			 * bit     | usage
			 * ------- | -----
			 * 00      | Request or response
			 * 01 - 31 | not used
			 * 32 - 63 | receipt
			 *
			 * @warning User must NOT modify this field.
			 */
			uint64_t			encs;
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
EAF_API eaf_msg_t* eaf_msg_create_req(_In_ uint32_t msg_id, _In_ size_t size,
	_In_ eaf_msg_handle_fn rsp_fn);

/**
 * @brief Create response.
 * @note This does not affect the reference count of original request.
 * @param[in] req		The original request
 * @param[in] size		The size of user structure
 * @return				The pointer of the response
 */
EAF_API eaf_msg_t* eaf_msg_create_rsp(_In_ eaf_msg_t* req, _In_ size_t size);

/**
 * @brief Add reference count
 * @param[in,out] msg		The message you want to add reference
 */
EAF_API void eaf_msg_add_ref(_Inout_ eaf_msg_t* msg);

/**
 * @brief Reduce reference count
 * @param[in,out] msg		The message you want to reduce reference
 */
EAF_API void eaf_msg_dec_ref(_Inout_ eaf_msg_t* msg);

/**
 * @brief Get user structure address.
 * @param[in] msg		The message
 * @param[out] size		The size of user structure
 * @return				The address of user structure
 */
EAF_API void* eaf_msg_get_data(_In_ eaf_msg_t* msg, _Out_opt_ size_t* size);

/**
 * @brief Get message type
 * @param[in] msg		Message
 * @return				#eaf_msg_type
 */
EAF_API eaf_msg_type_t eaf_msg_get_type(_In_ const eaf_msg_t* msg);

/**
 * @brief Replace exist response handler with given one.
 * @warning Do NOT use this function unless you know why.
 * @param[in,out] msg	The message
 * @param[in] fn		Response handler
 */
EAF_API void eaf_msg_set_rsp_fn(_Inout_ eaf_msg_t* msg, _In_ eaf_msg_handle_fn fn);

/**
 * @brief Get response handler
 * @param[in] msg	The message
 * @return			The response handler
 */
EAF_API eaf_msg_handle_fn eaf_msg_get_rsp_fn(_In_ const eaf_msg_t* msg);

/**
 * @brief Set receipt
 * @note End-user should not use this function.
 * @param[in,out] msg	The message
 * @param[in] receipt	Receipt
 */
EAF_API void eaf_msg_set_receipt(_Inout_ eaf_msg_t* msg, _In_ int receipt);

/**
 * @brief Get receipt
 * @param[in] msg	The message
 * @return			Receipt
 */
EAF_API int eaf_msg_get_receipt(_In_ const eaf_msg_t* msg);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
#endif  /* __EAF_CORE_MESSAGE_H__ */
