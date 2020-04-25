/** @file
 * RPC support for EAF
 */
#ifndef __EAF_CORE_RPC_H__
#define __EAF_CORE_RPC_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "eaf/core/message.h"
#include "eaf/utils/annotations.h"

/**
 * @brief Service info
 * @see eaf_rpc_cfg_t::on_service_register
 */
typedef struct eaf_rpc_service_info
{
	uint32_t	service_id;		/**< Service ID */
	uint32_t	capacity;		/**< Message queue capacity */
}eaf_rpc_service_info_t;

/**
 * @brief RPC handler
 */
typedef struct eaf_rpc_cfg
{
	/**
	 * @brief Initialize callback.
	 *
	 * During initialize progress, EAF will report service information by #on_service_register.
	 * @see eaf_rpc_cfg_t::on_service_register
	 */
	void(*on_init)(void);

	/**
	 * @brief Service registered.
	 * @param info	service info
	 */
	void(*on_service_register)(_In_ const eaf_rpc_service_info_t* info);

	/**
	 * Initialize process must finish.
	 * @return		0 if success, otherwise failed.
	 */
	int(*on_init_done)(void);

	/**
	 * @brief Shutdown RPC.
	 *
	 * After this call finish, you must guarantee no existing/future `eaf_rpc_income` call.
	 */
	void(*on_exit)(void);

	/**
	 * @brief Callback when service register a event.
	 * @param service_id	service id
	 * @param event_id		event id
	 */
	void(*on_event_subscribe)(_In_ uint32_t service_id, _In_ uint32_t event_id);

	/**
	 * @brief Callback when service unregister a event.
	 * @param service_id	service id
	 * @param event_id		event id
	 */
	void(*on_event_unsubscribe)(_In_ uint32_t service_id, _In_ uint32_t event_id);

	/**
	 * @brief Send message to remote
	 * @param msg			message
	 * @return				#eaf_errno
	 */
	int(*send_msg)(_Inout_ eaf_msg_t* msg);
}eaf_rpc_cfg_t;

/**
 * @brief Initialize RPC support
 * @param cfg	RPC configuration. Must be a static variable.
 * @return		eaf_errno
 * @see			eaf_errno_t
 */
int eaf_rpc_init(_In_ const eaf_rpc_cfg_t* cfg /*static*/);

/**
 * @brief Handle incoming RPC message
 * @param msg	incoming message
 * @return		eaf_errno
 */
int eaf_rpc_income(_Inout_ eaf_msg_t* msg);

#ifdef __cplusplus
}
#endif
#endif
