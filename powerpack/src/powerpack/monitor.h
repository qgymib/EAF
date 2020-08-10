#ifndef __EAF_POWERPACK_MONITOR_INTERNAL_H__
#define __EAF_POWERPACK_MONITOR_INTERNAL_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "powerpack.h"

typedef struct monitor_dataflow_record
{
	eaf_map_node_t					node;			/**< node for eaf_monitor_ctx_t::dataflow::record */
	struct
	{
		uint32_t					from;			/** Who send this message */
		uint32_t					to;				/** Who will receive this message */
		size_t						count;			/** Message count */
	}data;
}monitor_dataflow_record_t;

typedef struct monitor_service_record
{
	eaf_map_node_t					node_group;		/**< node for eaf_monitor_ctx_t::serivce::record_group */
	eaf_map_node_t					node_split;		/**< node for eaf_monitor_ctx_t::serivce::record_split */

	uv_mutex_t						objlock;		/**< Object lock */

	struct
	{
		uint32_t					gid;			/**< Group ID */
		uint32_t					sid;			/**< Service ID. It must be unique */
		eaf_service_local_t*		sls;			/**< Service Local Storage */
	}data;

	struct
	{
		uint32_t					flush_send;		/**< The number of message send */
		uint32_t					flush_recv;		/**< The number of message recv */
		uint64_t					flush_use_time;	/**< Use time in nanoseconds */

		uint64_t					total_send;		/**< The total number of message send */
		uint64_t					total_recv;		/**< The total number of message recv */
	}counter;

	struct
	{
		uint64_t					use_time_start;	/**< The start point of use time */
	}temp;
}monitor_service_record_t;

typedef struct monitor_group_record
{
	uv_mutex_t						objlock;		/**< Object lock */
	eaf_group_local_t*				gls;			/**< Group Local Storage */

	struct
	{
		uint64_t					flush_use_time;	/**< Use time in nanoseconds */
	}counter;
}monitor_group_record_t;

typedef struct eaf_monitor_ctx
{
	struct
	{
		eaf_map_t					record_group;	/**< Group by GID, search by `gid' and `sid' */
		eaf_map_t					record_split;	/**< Independent record, search by `sid' */
	}serivce;

	struct
	{
		size_t						size;			/**< Array size */
		monitor_group_record_t*		table;			/** Group record */
	}group;

	struct
	{
		eaf_map_t					record;			/**< A insert-only table for record message flow */
	}dataflow;

	struct
	{
		unsigned					timeout_sec;	/**< Refresh timeout in seconds */
	}config;

	eaf_powerpack_hook_t			hook;			/**< PowerPack hook */
}eaf_monitor_ctx_t;

typedef struct eaf_monitor_ctx2
{
	struct
	{
		unsigned					refresh_timer_running : 1;
	}mask;

	struct
	{
		uv_mutex_t					objlock;		/**< Object lock */
	}refresh;

	struct
	{
		uv_rwlock_t					rwlock;			/**< rwlock for eaf_monitor_ctx_t::dataflow::record */
	}dataflow;
}eaf_monitor_ctx2_t;

#ifdef __cplusplus
}
#endif
#endif
