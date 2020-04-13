#ifndef __EAF_CORE_RPC_H__
#define __EAF_CORE_RPC_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "EAF/core/message.h"

typedef struct eaf_rpc_service_info
{
	uint32_t	service_id;		/** service id */
	uint32_t	capacity;		/** message queue capacity */
}eaf_rpc_service_info_t;

typedef struct eaf_rpc_cfg
{
	/**
	* RPC need to initialize.
	* During initialize progress, EAF will report service information by `on_service_register`.
	*/
	void(*on_init)(void);

	/**
	* Service registered.
	* @param info	service info
	*/
	void(*on_service_register)(eaf_rpc_service_info_t* info);

	/**
	* Initialize process must finish.
	* @return		zero: success. non-zero: failure
	*/
	int(*on_init_done)(void);

	/**
	* Shutdown RPC.
	* After this call finish, you must guarantee no existing/future `eaf_rpc_income` call.
	*/
	void(*on_exit)(void);

	/**
	* service subscribe event.
	* @param service_id		service id
	* @param event_id		event id
	*/
	void(*on_event_subscribe)(uint32_t service_id, uint32_t event_id);

	/**
	* service unsubscribe event
	* @param service_id		service id
	* @param event_id		event id
	*/
	void(*on_event_unsubscribe)(uint32_t service_id, uint32_t event_id);

	/**
	* send message to remote
	* @param msg			message
	* @return				eaf_errno
	*/
	int(*send_msg)(eaf_msg_t* msg);
}eaf_rpc_cfg_t;

/**
* Initialize RPC support
* @param cfg	RPC configuration. Must be a static variable.
* @return		eaf_errno
* @see			eaf_errno_t
*/
int eaf_rpc_init(const eaf_rpc_cfg_t* cfg /*static*/);

/**
* Handle incoming RPC message
* @param msg	incoming message
* @return		eaf_errno
*/
int eaf_rpc_income(eaf_msg_t* msg);

#ifdef __cplusplus
}
#endif
#endif
