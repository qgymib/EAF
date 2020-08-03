#ifndef __EAF_POWERPACK_TIMER_INTERNAL_H__
#define __EAF_POWERPACK_TIMER_INTERNAL_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "powerpack.h"

typedef struct timer_record
{
	eaf_list_node_t				node;					/**< List node */

	struct
	{
		uv_timer_t				timer;					/**< timer */
	}uv;

	struct
	{
		eaf_msg_t				req;					/**< Original request information */
		uint32_t				msec;					/**<  */
	}data;
}timer_record_t;

typedef struct timer_ctx
{
	struct
	{
		unsigned				inited : 1;				/** Mask for initialized */
		unsigned				looping : 1;
		unsigned				inited_notifier : 1;	/** Mask for initialize timer_uv_ctx_t::gnotifier */
	}mask;

	eaf_list_t					idle_queue;				/**< Record that just created */
	eaf_list_t					busy_queue;				/**< Record that wait for libuv callback */
	eaf_list_t					dead_queue;				/**< Record that will be destroy */

	eaf_powerpack_hook_t		hook;					/**< PowerPack hook */
}timer_ctx_t;

typedef struct timer_uv_ctx
{
	uv_mutex_t					glock;					/**< Global lock */
	uv_async_t					gnotifier;				/**< Notifier */
}timer_uv_ctx_t;

#ifdef __cplusplus
}
#endif
#endif
