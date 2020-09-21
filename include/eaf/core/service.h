/** @file
 * EAF service defines.
 */
#ifndef __EAF_CORE_SERVICE_H__
#define __EAF_CORE_SERVICE_H__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup EAF-Core
 * @defgroup EAF-Service Service
 * @{
 */

#include "eaf/core/internal/service.h"
#include "eaf/core/message.h"
#include "eaf/infra/thread.h"

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
 * @brief A static initializer for #eaf_hook_t
 * @see eaf_hook_t
 */
#define EAF_HOOK_INITIALIZER		{ EAF_REPEAT(15, NULL) }

typedef enum eaf_service_attribute
{
	/**
	 * @brief Service will still alive during teardown stage.
	 * @see eaf_teardown()
	 */
	eaf_service_attribute_alive	= 0x01 << 0x00,
} eaf_service_attribute_t;

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
	uint32_t						attribute;		/**< Service attribute, see #eaf_service_attribute_t */
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

typedef enum eaf_exit_executor
{
	eaf_exit_executor_eaf,
	eaf_exit_executor_user,
}eaf_exit_executor_t;

typedef struct eaf_cleanup_summary
{
	eaf_exit_executor_t		executor;	/**< Who called #eaf_exit() */
	int						reason;		/**< #eaf_errno */
}eaf_cleanup_summary_t;

/**
 * @brief Hook
 */
typedef struct eaf_hook
{
	/**
	 * @brief Hook a service is going to init
	 * @see eaf_entrypoint_t::on_init
	 * @param[in] id	Service ID
	 */
	void(*on_service_init_before)(_In_ uint32_t id);

	/**
	 * @brief Hook a service already init
	 * @see eaf_entrypoint_t::on_init
	 * @param[in] id	Service ID
	 */
	void(*on_service_init_after)(_In_ uint32_t id, _In_ int ret);

	/**
	 * @brief Hook a service is going to exit
	 * @see eaf_entrypoint_t::on_exit
	 * @param[in] id	Service ID
	 */
	void(*on_service_exit_before)(_In_ uint32_t id);

	/**
	 * @brief Hook a service already exit
	 * @see eaf_entrypoint_t::on_exit
	 * @param[in] id	Service ID
	 */
	void(*on_service_exit_after)(_In_ uint32_t id);

	/**
	 * @brief Hook a service just enter yield state.
	 * @see eaf_yield
	 * @see eaf_yield_ext()
	 * @param[in] id	Service ID
	 */
	void(*on_service_yield)(_In_ uint32_t id);

	/**
	 * @brief Hook a service will leave yield state.
	 * @see eaf_resume()
	 * @param[in] id	Service ID
	 */
	void(*on_service_resume)(_In_ uint32_t id);

	/**
	 * @brief Hook a service is going to register
	 * @see eaf_register()
	 * @param[in] id	Service ID
	 * @return				#eaf_errno
	 */
	int(*on_service_register)(_In_ uint32_t id);

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
	int(*on_message_send_before)(_In_ uint32_t from, _In_ uint32_t to, _Inout_ eaf_msg_t* msg);

	/**
	 * @brief Hook when a message is done send.
	 *
	 * This hook is called when user already send a request/response.
	 *
	 * @see eaf_send_req()
	 * @see eaf_send_rsp()
	 * @param[in] from		Who send this message
	 * @param[in] to		Who will receive this message
	 * @param[in,out] msg	The message
	 * @param[in] ret		Send result
	 */
	void(*on_message_send_after)(_In_ uint32_t from, _In_ uint32_t to, _Inout_ eaf_msg_t* msg, _In_ int ret);

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
	int(*on_message_handle_before)(_In_ uint32_t from, _In_ uint32_t to, _Inout_ eaf_msg_t* msg);

	/**
	 * @brief Hook a service is just handle message.
	 *
	 * This hook is called when a service just handle a request/response.
	 * @param[in] from		Who send this message
	 * @param[in] to		Who will receive this message
	 * @param[in,out] msg	The message
	 */
	void(*on_message_handle_after)(_In_ uint32_t from, _In_ uint32_t to, _Inout_ eaf_msg_t* msg);

	/**
	 * @brief Hook EAF is going to load.
	 */
	void(*on_load_before)(void);

	/**
	 * @brief Hook EAF is loaded.
	 * @param[in] ret	Load result
	 */
	void(*on_load_after)(int ret);

	/**
	 * @brief Hook before #eaf_exit() take effect
	 * @see eaf_exit()
	 */
	void(*on_exit_before)(void);

	/**
	 * @brief Hook after #eaf_exit() take effect
	 * @see eaf_exit()
	 */
	void(*on_exit_after)(void);
}eaf_hook_t;

/**
 * @brief Resume service
 * @param[in] srv_id	Service ID
 * @return				#eaf_errno
 */
EAF_API int eaf_resume(_In_ uint32_t srv_id);

/**
 * @brief Initialize EAF
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
EAF_API int eaf_init(_In_ const eaf_group_table_t* /*static*/ info, _In_ size_t size)
	EAF_ATTRIBUTE_NONNULL(1);

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
 * @brief Teardown EAF.
 *
 * Teardown is a weak version #eaf_exit. If this function is called, EAF will
 * close every service which has no #eaf_service_attribute_alive
 *
 * @return #eaf_errno
 */
EAF_API int eaf_teardown(void);

/**
 * @brief Exit EAF.
 *
 * At this stage, EAF will exit every registered services.
 * EAF does not guarantee every service is exited after this call.
 *
 * @note This is a asynchronous call.
 * @param[in] reason	Exit reason
 * @return				#eaf_errno
 */
EAF_API int eaf_exit(_In_ int reason);

/**
 * @brief Wait for EAF exit and cleanup all resources
 * @param[out] summary	Platform summary
 * @return				#eaf_errno
 */
EAF_API int eaf_cleanup(_Out_opt_ eaf_cleanup_summary_t* summary);

/**
 * @brief Register service
 *
 * Register service info into EAF. This function can only be called after
 * #eaf_init() and before #eaf_load().
 *
 * @warning The parameter `info` must be globally accessible.
 * @see eaf_init()
 * @see eaf_load()
 * @param[in] id		Service ID
 * @param[in] entry		Service entry. Must be globally accessible.
 * @return				#eaf_errno
 */
EAF_API int eaf_register(_In_ uint32_t id, _In_ const eaf_entrypoint_t* /*static*/ entry)
	EAF_ATTRIBUTE_NONNULL(2);

/**
 * @brief Send request
 * @param[in] from		Who send this request
 * @param[in] to		Who will receive this request
 * @param[in,out] req	The request
 * @return				#eaf_errno
 */
EAF_API int eaf_send_req(_In_ uint32_t from, _In_ uint32_t to, _Inout_ eaf_msg_t* req)
	EAF_ATTRIBUTE_NONNULL(3);

/**
 * @brief Send response
 * @param[in] from		Who send this response
 * @param[in] to		Who will receive this response
 * @param[in,out] rsp	The response
 * @return				#eaf_errno
 */
EAF_API int eaf_send_rsp(_In_ uint32_t from, _In_ uint32_t to, _Inout_ eaf_msg_t* rsp)
	EAF_ATTRIBUTE_NONNULL(3);

/**
 * @brief Inject a system wide hook
 * @param[in] hook	Hook, must be a global resource
 * @param[in] size	sizeof(*hook)
 * @return			#eaf_errno
 */
EAF_API int eaf_inject(_In_ const eaf_hook_t* /* static */ hook, _In_ size_t size)
	EAF_ATTRIBUTE_NONNULL(1);

/**
 * @brief Undo inject system wide hook.
 * @param[in] hook	The hook already registered
 * @return			#eaf_errno
 */
EAF_API int eaf_uninject(_In_ const eaf_hook_t* hook)
	EAF_ATTRIBUTE_NONNULL(1);

/**
 * @brief Get caller's service id
 * @return			Service ID
 */
EAF_API uint32_t eaf_service_self(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
#endif
