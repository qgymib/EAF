#ifndef __EAF_CORE_SERVICE_INTERNAL_H__
#define __EAF_CORE_SERVICE_INTERNAL_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "eaf/core/service.h"

typedef enum eaf_ctx_state
{
	eaf_ctx_state_init,								/**< Initialize state */
	eaf_ctx_state_busy,								/**< Running state */
	eaf_ctx_state_teardown,							/**< Teardown state */
	eaf_ctx_state_exit,								/**< Exit state */
}eaf_ctx_state_t;

typedef struct eaf_msgq_record
{
	eaf_list_node_t					node;			/**< List node */

	union
	{
		struct
		{
			eaf_msg_handle_fn		req_fn;			/**< Request handler */
		}req;
	}info;

	struct
	{
		uint32_t					from;
		uint32_t					to;
		struct eaf_service*			service;		/**< Point to service */
		eaf_msg_full_t*				msg;			/**< Message */
	}data;
}eaf_msgq_record_t;

typedef struct eaf_service
{
	const eaf_entrypoint_t*			entry;			/**< Service entrypoint information */

	struct
	{
		unsigned					alive : 1;		/**< Still alive in teardown stage */
		unsigned					lazyload : 1;	/**< Delay initialize unitl incoming request */
	}mask;

	struct
	{
		eaf_service_local_t			local;			/**< Service Local Information */
		eaf_list_node_t				node;			/**< List node. Either in ready_list or wait_list */
	}runtime;

	struct
	{
		eaf_msgq_record_t*			cur_msg;		/**< Current process message */
		eaf_list_t					queue;			/**< Message queue */
		size_t						capacity;		/**< The max capacity of message queue */
	}msgq;
}eaf_service_t;

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4200)
#endif
typedef struct eaf_group
{
	eaf_compat_lock_t				objlock;		/**< Thread lock */
	eaf_compat_thread_t				working;		/**< Thread handler */
	size_t							index;			/**< Group index */

	struct
	{
		eaf_group_local_t			local;			/**< Group Local Storage */
		eaf_service_t*				cur_run;		/**< The current running service */
		eaf_list_t					busy_list;		/**< INIT/BUSY */
		eaf_list_t					wait_list;		/**< INIT_YIELD/IDLE/PEND */
	}coroutine;

	struct
	{
		eaf_compat_sem_t			sem;			/**< Message queue semaphore */
	}msgq;

	struct
	{
		size_t						size;			/**< The length of service table */
		eaf_service_t				table[];		/**< Service table */
	}service;
}eaf_group_t;
#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

typedef struct eaf_ctx
{
	eaf_ctx_state_t					state;			/**< Global EAF state */
	eaf_compat_sem_t				ready;			/**< Exit semaphore */
	eaf_thread_storage_t			tls;			/**< Thread local storage */
	eaf_cleanup_summary_t			summary;		/**< Exit summary */

	struct
	{
		unsigned					failure : 1;	/**< Initialize failure */
	}mask;

	struct
	{
		size_t						size;			/**< The length of group table */
		eaf_group_t**				table;			/**< Group table */
	}group;

	const eaf_hook_t*				hook;			/**< Hook */
}eaf_ctx_t;

#ifdef __cplusplus
}
#endif
#endif
