/** @file
* EAF service defines.
*/
#ifndef __EAF_CORE_SERVICE_H__
#define __EAF_CORE_SERVICE_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "EAF/core/internal/service.h"
#include "EAF/core/message.h"
#include "EAF/infra/thread.h"
#include "EAF/utils/annotations.h"

/**
 * @brief Coroutine body
 *
 * The coroutine body must be surrounded by `{}`. Wthin coroutine body, you
 * are able to use coroutine series functions like #eaf_yield or #eaf_yield_ext.
 *
 * @see eaf_yield
 * @see eaf_yield_ext
 */
#define eaf_reenter					EAF_COROUTINE_REENTER()

/**
 * @brief Suspend service until #eaf_resume is called
 * @see eaf_resume
 */
#define eaf_yield					eaf_yield_ext(NULL, NULL)

/**
 * @brief The extended version of #eaf_yield
 *
 * Like #eaf_yield, service will suspend until #eaf_resume is called,
 * and when service was suspended, `fn` will be called immediately.
 * `fn` is a function with proto #eaf_yield_hook_fn.
 *
 * @see eaf_yield_hook_fn
 * @see eaf_resume
 */
#define eaf_yield_ext(fn, arg)		EAF_COROUTINE_YIELD(fn, arg, EAF_COROUTINE_YIELD_TOKEN)

/**
 * @brief Request table
 */
typedef struct eaf_message_table
{
	uint32_t						msg_id;			/**< Request ID */
	eaf_req_handle_fn				fn;				/**< The handler of request */
}eaf_message_table_t;

/**
 * @brief Service initialize info
 */
typedef struct eaf_entrypoint
{
	size_t							msg_table_size;	/**< The sizeof request table */
	const eaf_message_table_t*		msg_table;		/**< Request table */

	/**
	 * @brief Initialize callback
	 * @return		0 if success, -1 otherwise
	 */
	int (*on_init)(void);

	/**
	 * @brief Exit callback
	 */
	void (*on_exit)(void);
}eaf_entrypoint_t;

/**
 * @brief Service configure
 */
typedef struct eaf_service_table
{
	uint32_t						srv_id;			/**< Service ID */
	uint32_t						msgq_size;		/**< The capacity of message queue */
}eaf_service_table_t;

/**
 * @brief Service group configure
 */
typedef struct eaf_group_table
{
	eaf_thread_attr_t				attr;			/**< Thread attribute */

	struct
	{
		size_t						size;			/**< The size of configure table */
		eaf_service_table_t*		table;			/**< The pointer of configure table */
	}service;										/**< Configure table */
}eaf_group_table_t;

/**
 * @brief Resume service
 * @param srv_id	Service ID
 * @return			#eaf_errno
 */
int eaf_resume(_In_ uint32_t srv_id);

/**
 * @brief Setup EAF
 *
 * This will allocate every necessary resource and wait for service register.
 * Service must registered into EAF after this function call and before #eaf_load
 * is called.
 *
 * @warning The parameter `info` must be globally accessible.
 * @see eaf_load
 * @see eaf_register
 * @param info		Service group table. Must be globally accessible.
 * @param size		The size of service group table
 * @return			#eaf_errno
 */
int eaf_setup(_In_ const eaf_group_table_t* info /*static*/, _In_ size_t size);

/**
 * @brief Load EAF.
 * 
 * At this stage, EAF will try to initialize every registered services.
 * If a service failed to load, other service in the same group will be 
 * exited.
 * After this call return, EAF guarantee every registered services was either
 * initialized or exited.
 *
 * @return			#eaf_errno
 */
int eaf_load(void);

/**
 * @brief Tear down EAF.
 *
 * At this stage, EAF will tear down every registered services.
 * After this call return, EAF guarantee every registered services was exited.
 *
 * @return			#eaf_errno
 */
int eaf_cleanup(void);

/**
 * @brief Register service
 *
 * Register service info into EAF. This function can only be called after
 * #eaf_setup and before #eaf_load.
 *
 * @warning The parameter `info` must be globally accessible.
 * @see eaf_setup
 * @see eaf_load
 * @param srv_id	Service ID
 * @param entry		Service entrypoint. Must be globally accessible.
 * @return			#eaf_errno
 */
int eaf_register(_In_ uint32_t srv_id, _In_ const eaf_entrypoint_t* entry /*static*/);

/**
 * @brief Subscribe event.
 * @param srv_id	Service ID
 * @param evt_id	Event ID
 * @param fn		Event handler
 * @param arg		User defined argument
 * @return			#eaf_errno
 */
int eaf_subscribe(_In_ uint32_t srv_id, _In_ uint32_t evt_id,
	_In_ eaf_evt_handle_fn fn, _In_ void* arg);

/**
 * @brief Unsubscribe event.
 * @param srv_id	Service ID
 * @param evt_id	Event ID
 * @param fn		Event handler
 * @param arg		User defined argument
 * @return			#eaf_errno
 */
int eaf_unsubscribe(_In_ uint32_t srv_id, _In_ uint32_t evt_id,
	_In_ eaf_evt_handle_fn fn, _In_ void* arg);

/**
 * @brief Send request
 * @param from		The service id of sender
 * @param to		The service id of receiver
 * @param req		The request
 * @return			#eaf_errno
 */
int eaf_send_req(_In_ uint32_t from, _In_ uint32_t to, _Inout_ eaf_msg_t* req);

/**
 * @brief Send response
 * @param from		The service id of sender
 * @param rsp		The response
 * @return			#eaf_errno
 */
int eaf_send_rsp(_In_ uint32_t from, _Inout_ eaf_msg_t* rsp);

/**
 * @brief Send Event
 * @param from		The service id of sender
 * @param evt		The event
 * @return			#eaf_errno
 */
int eaf_send_evt(_In_ uint32_t from, _Inout_ eaf_msg_t* evt);

/**
 * @brief Get caller's service id
 * @return			service id
 */
uint32_t eaf_service_self(void);

#ifdef __cplusplus
}
#endif
#endif
