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

struct eaf_msg;

/**
 * @brief The type of eaf_msg_t
 * @see eaf_msg_t
 */
typedef enum eaf_msg_type
{
	eaf_msg_type_req,					/**< Request */
	eaf_msg_type_rsp,					/**< Response */
	eaf_msg_type_evt,					/**< Event */
}eaf_msg_type_t;

/**
 * @brief Prototype for request handler
 * @param req		Request
 */
typedef void(*eaf_req_handle_fn)(struct eaf_msg* req);

/**
 * @brief Prototype for response handler
 * @param rsp		Response
 */
typedef void(*eaf_rsp_handle_fn)(struct eaf_msg* rsp);

/**
 * @brief Prototype for event handler
 * @param evt		Event
 * @param arg		User defined argument
 */
typedef void(*eaf_evt_handle_fn)(struct eaf_msg* evt, void* arg);

/**
 * @brief A communicate structure
 */
typedef struct eaf_msg
{
	eaf_msg_type_t				type;	/**< Message type */
	uint32_t					id;		/**< Message ID */
	uint32_t					from;	/**< The service ID of sender */
	uint32_t					to;		/**< The service ID of receiver */

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
 * @param msg_id	Message ID
 * @param size		The size of user structure
 * @param rsp_fn	The response handler for the response
 * @return			The pointer of the request
 */
eaf_msg_t* eaf_msg_create_req(uint32_t msg_id, size_t size, eaf_rsp_handle_fn rsp_fn);

/**
 * @brief Create response.
 * @note This does not affect the reference count of original request.
 * @param req		The original request
 * @param size		The size of user structure
 * @return			The pointer of the response
 */
eaf_msg_t* eaf_msg_create_rsp(eaf_msg_t* req, size_t size);

/**
 * @brief Create event.
 * @param evt_id	Event id
 * @param size		The size of user structure
 * @return			The pointer of the event
 */
eaf_msg_t* eaf_msg_create_evt(uint32_t evt_id, size_t size);

/**
 * @brief Add reference count
 * @param msg		The message you want to add reference
 */
void eaf_msg_add_ref(eaf_msg_t* msg);

/**
 * @brief Reduce reference count
 * @param msg		The message you want to add reference
 */
void eaf_msg_dec_ref(eaf_msg_t* msg);

/**
 * @brief Get user structure address.
 * @param msg		The message
 * @param size		The size of user structure
 * @return			The address of user structure
 */
void* eaf_msg_get_data(eaf_msg_t* msg, size_t* size);

#ifdef __cplusplus
}
#endif
#endif  /* __EAF_CORE_MESSAGE_H__ */
