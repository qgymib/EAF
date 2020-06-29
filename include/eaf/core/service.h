/** @file
 * EAF service defines.
 */
#ifndef __EAF_CORE_SERVICE_H__
#define __EAF_CORE_SERVICE_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "eaf/core/internal/service.h"
#include "eaf/core/message.h"
#include "eaf/infra/thread.h"
#include "eaf/utils/define.h"

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
 * `fn` is a function with protocol #eaf_yield_hook_fn.
 *
 * @param[in] fn	Callback
 * @param[in] arg	User defined argument passed to callback
 * @see eaf_yield_hook_fn
 * @see eaf_resume
 */
#define eaf_yield_ext(fn, arg)		EAF_COROUTINE_YIELD(fn, arg, EAF_COROUTINE_YIELD_TOKEN)

/**
 * @brief Service states
 *
 * INIT1
 *  /|\       |--------|
 *  \|/      \|/       |
 * INIT0 --> IDLE --> BUSY --> PEND
 *   |       \|/      /|\       |
 *   | ----> EXIT      |--------|
 */
typedef enum eaf_service_state
{
	eaf_service_state_init0,					/**< Init */
	eaf_service_state_init1,					/**< Init but user yield */
	eaf_service_state_idle,						/**< No pending work */
	eaf_service_state_busy,						/**< Busy */
	eaf_service_state_pend,						/**< Wait for resume */
	eaf_service_state_exit,						/**< Exit */
}eaf_service_state_t;

/**
 * @brief Request table
 */
typedef struct eaf_message_table
{
	uint32_t						msg_id;			/**< Request ID */
	eaf_msg_handle_fn				fn;				/**< The handler of request */
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
 * @brief Hook
 */
typedef struct eaf_hook
{
	/**
	 * @brief Hook when a message is going to be send.
	 *
	 * This hook is called when user want to send a request/response.
	 * If a non-zero code is returned, the message will not be send, and the code
	 * will be returned to user.
	 *
	 * @see eaf_send_req()
	 * @see eaf_send_rsp()
	 * @param[in] from		Who send this message
	 * @param[in] to		Who will receive this message
	 * @param[in,out] msg	The message
	 * @return				#eaf_errno
	 */
	int(*on_msg_send)(_In_ uint32_t from, _In_ uint32_t to, _Inout_ struct eaf_msg* msg);

	/**
	 * @brief Hook when destination cannot found.
	 * @see eaf_send_req()
	 * @see eaf_send_rsp()
	 * @param[in] from		Who send this message
	 * @param[in] to		Who will receive this message
	 * @param[in,out] msg	The message
	 * @return				#eaf_errno
	 */
	int(*on_dst_not_found)(_In_ uint32_t from, _In_ uint32_t to, _Inout_ struct eaf_msg* msg);

	/**
	 * @brief Hook a service is going to handle message.
	 *
	 * This hook is called when a service is going to handle a
	 * request/response.
	 * If a non-zero code is returned, the service will not handle this message.
	 *
	 * @warning EAF do NOT automatically generate a response for any return code.
	 * @param[in] from		Who send this message
	 * @param[in] to		Who will receive this message
	 * @param[in,out] msg	The message
	 * @return				#eaf_errno
	 */
	int(*on_pre_msg_process)(_In_ uint32_t from, _In_ uint32_t to, _Inout_ struct eaf_msg* msg);

	/**
	 * @brief Hook a service is just handle message.
	 *
	 * This hook is called when a service just handle a request/response.
	 * @param[in] from		Who send this message
	 * @param[in] to		Who will receive this message
	 * @param[in,out] msg	The message
	 */
	void(*on_post_msg_process)(_In_ uint32_t from, _In_ uint32_t to, _Inout_ struct eaf_msg* msg);

	/**
	 * @brief Hook a service just enter yield state.
	 * @see eaf_yield
	 * @see eaf_yield_ext()
	 * @param[in] srv_id	Service ID
	 */
	void(*on_post_yield)(_In_ uint32_t srv_id);

	/**
	 * @brief Hook a service will leave yield state.
	 * @see eaf_resume()
	 * @param[in] srv_id	Service ID
	 */
	void(*on_post_resume)(_In_ uint32_t srv_id);

	/**
	 * @brief Hook a service is going to register
	 * @see eaf_register()
	 * @param[in] srv_id	Service ID
	 * @return				#eaf_errno
	 */
	int(*on_pre_register)(_In_ uint32_t srv_id);

	/**
	 * @brief Hook before #eaf_cleanup() take effect
	 * @see eaf_cleanup()
	 * @return				#eaf_errno
	 */
	int(*on_pre_cleanup)(void);

	/**
	 * @brief Hook after #eaf_cleanup() take effect
	 * @see eaf_cleanup()
	 */
	void(*on_post_cleanup)(void);
}eaf_hook_t;

/**
 * @brief Resume service
 * @param[in] srv_id	Service ID
 * @return				#eaf_errno
 */
EAF_API int eaf_resume(_In_ uint32_t srv_id);

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
 * @param[in] info	Service group table. Must be globally accessible.
 * @param[in] size	The size of service group table
 * @return			#eaf_errno
 */
EAF_API int eaf_setup(_In_ const eaf_group_table_t* info /*static*/, _In_ size_t size);

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
EAF_API int eaf_load(void);

/**
 * @brief Tear down EAF.
 *
 * At this stage, EAF will tear down every registered services.
 * After this call return, EAF guarantee every registered services was exited.
 *
 * @return			#eaf_errno
 */
EAF_API int eaf_cleanup(void);

/**
 * @brief Register service
 *
 * Register service info into EAF. This function can only be called after
 * #eaf_setup and before #eaf_load.
 *
 * @warning The parameter `info` must be globally accessible.
 * @see eaf_setup
 * @see eaf_load
 * @param[in] srv_id	Service ID
 * @param[in] entry		Service entry. Must be globally accessible.
 * @return				#eaf_errno
 */
EAF_API int eaf_register(_In_ uint32_t srv_id, _In_ const eaf_entrypoint_t* entry /*static*/);

/**
 * @brief Send request
 * @param[in] from		Who send this request
 * @param[in] to		Who will receive this request
 * @param[in,out] req	The request
 * @return				#eaf_errno
 */
EAF_API int eaf_send_req(_In_ uint32_t from, _In_ uint32_t to, _Inout_ eaf_msg_t* req);

/**
 * @brief Send response
 * @param[in] from		Who send this response
 * @param[in] to		Who will receive this response
 * @param[in,out] rsp	The response
 * @return				#eaf_errno
 */
EAF_API int eaf_send_rsp(_In_ uint32_t from, _In_ uint32_t to, _Inout_ eaf_msg_t* rsp);

/**
 * @brief Inject a system wide hook
 * @param[in] hook	Hook, must be a global resource
 * @param[in] size	sizeof(*hook)
 * @return			#eaf_errno
 */
EAF_API int eaf_inject(_In_ const eaf_hook_t* hook, _In_ size_t size);

/**
 * @brief Get caller's service id
 * @return			Service ID
 */
EAF_API uint32_t eaf_service_self(void);

/**
 * @brief Get service state
 * @note If EAF is not running or Service ID not found, #eaf_service_state_exit
 *   is returned.
 * @param[in] id	Service ID
 * @return			Service state
 */
EAF_API eaf_service_state_t eaf_service_get_state(_In_ uint32_t id);

#ifdef __cplusplus
}
#endif
#endif
