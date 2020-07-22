#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "eaf/eaf.h"
#include "powerpack.h"
#include "monitor.h"

#ifdef _MSC_VER
#define APPEND_LINE(buf, buflen, fmt, ...)										\
	do {																		\
		size_t len = strlen(buf);												\
		_snprintf_s(buf + len, buflen - len, _TRUNCATE, fmt, ##__VA_ARGS__);	\
	} EAF_MSVC_WARNING_GUARD(4127, while (0))
#else
#define APPEND_LINE(buf, buflen, fmt, ...)										\
	do {																		\
		size_t len = strlen(buf);												\
		snprintf(buf + len, buflen - len, fmt, ##__VA_ARGS__);					\
	} while (0)
#endif

static int _monitor_cmp_dataflow_record(const eaf_map_node_t* key1, const eaf_map_node_t* key2, void* arg);
static int _monitor_cmp_service_record_group(const eaf_map_node_t* key1, const eaf_map_node_t* key2, void* arg);
static int _monitor_cmp_service_record_split(const eaf_map_node_t* key1, const eaf_map_node_t* key2, void* arg);
static void _monitor_on_load_before(void);
static void _monitor_on_message_send_after(uint32_t from, uint32_t to, eaf_msg_t* msg, int ret);
static int _monitor_on_message_handle_before(uint32_t from, uint32_t to, eaf_msg_t* msg);
static void _monitor_on_message_handle_after(uint32_t from, uint32_t to, eaf_msg_t* msg);
static void _monitor_on_exit_before(void);

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
}eaf_monitor_ctx_t;

typedef struct eaf_monitor_ctx2
{
	struct
	{
		uv_timer_t					timer;			/**< Global refresh timer */
		struct
		{
			unsigned				running : 1;	/**< Timer is running */
		}mask;
		uv_mutex_t					objlock;		/**< Object lock */
	}refresh;

	struct
	{
		uv_rwlock_t					rwlock;			/**< rwlock for eaf_monitor_ctx_t::dataflow::record */
	}dataflow;
}eaf_monitor_ctx2_t;

static eaf_monitor_ctx2_t g_eaf_monitor_ctx2;
static eaf_monitor_ctx_t g_eaf_monitor_ctx = {
	{
		EAF_MAP_INITIALIZER(_monitor_cmp_service_record_group, NULL),
		EAF_MAP_INITIALIZER(_monitor_cmp_service_record_split, NULL),
	},																		// .serivce
	{
		0, NULL																// .group
	},
	{
		EAF_MAP_INITIALIZER(_monitor_cmp_dataflow_record, NULL),
	},																		// .dataflow
};

static eaf_powerpack_hook_t g_pp_hook = {
	EAF_LIST_NODE_INITIALIZER,
	{
		NULL,								// .on_service_init_before
		NULL,								// .on_service_init_after
		NULL,								// .on_service_exit_before
		NULL,								// .on_service_exit_after
		NULL,								// .on_service_yield
		NULL,								// .on_service_resume
		NULL,								// .on_service_register
		_monitor_on_message_handle_before,	// .on_message_send_before
		_monitor_on_message_send_after,		// .on_message_send_after
		NULL,								// .on_message_handle_before
		_monitor_on_message_handle_after,	// .on_message_handle_after
		_monitor_on_load_before,			// .on_load_before
		NULL,								// .on_load_after
		_monitor_on_exit_before,			// .on_exit_before
		NULL,								// .on_exit_after
	}
};

static int _monitor_cmp_service_record_group(const eaf_map_node_t* key1, const eaf_map_node_t* key2, void* arg)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(arg);

	monitor_service_record_t* rec_1 = EAF_CONTAINER_OF(key1, monitor_service_record_t, node_group);
	monitor_service_record_t* rec_2 = EAF_CONTAINER_OF(key2, monitor_service_record_t, node_group);

	if (rec_1->data.gid < rec_2->data.gid)
	{
		return -1;
	}
	else if (rec_1->data.gid > rec_2->data.gid)
	{
		return 1;
	}

	if (rec_1->data.sid < rec_2->data.sid)
	{
		return -1;
	}
	else if (rec_1->data.sid > rec_2->data.sid)
	{
		return 1;
	}

	return 0;
}

static int _monitor_cmp_service_record_split(const eaf_map_node_t* key1, const eaf_map_node_t* key2, void* arg)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(arg);

	monitor_service_record_t* rec_1 = EAF_CONTAINER_OF(key1, monitor_service_record_t, node_split);
	monitor_service_record_t* rec_2 = EAF_CONTAINER_OF(key2, monitor_service_record_t, node_split);

	if (rec_1->data.sid == rec_2->data.sid)
	{
		return 0;
	}
	return rec_1->data.sid < rec_2->data.sid ? -1 : 1;
}

static int _monitor_cmp_dataflow_record(const eaf_map_node_t* key1, const eaf_map_node_t* key2, void* arg)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(arg);

	monitor_dataflow_record_t* rec_1 = EAF_CONTAINER_OF(key1, monitor_dataflow_record_t, node);
	monitor_dataflow_record_t* rec_2 = EAF_CONTAINER_OF(key2, monitor_dataflow_record_t, node);

	if (rec_1->data.from < rec_2->data.from)
	{
		return -1;
	}
	else if (rec_1->data.from > rec_2->data.from)
	{
		return 1;
	}

	if (rec_1->data.to < rec_2->data.to)
	{
		return -1;
	}
	else if (rec_1->data.to > rec_2->data.to)
	{
		return 1;
	}

	return 0;
}

static monitor_service_record_t* _monitor_find_serivce(uint32_t sid)
{
	monitor_service_record_t tmp_key;
	tmp_key.data.sid = sid;

	eaf_map_node_t* it = eaf_map_find(&g_eaf_monitor_ctx.serivce.record_split, &tmp_key.node_split);
	return it != NULL ? EAF_CONTAINER_OF(it, monitor_service_record_t, node_split) : NULL;
}

static void _monitor_update_counter_send(uint32_t sid)
{
	monitor_service_record_t* record = _monitor_find_serivce(sid);

	uv_mutex_lock(&record->objlock);
	{
		record->counter.flush_send++;
	}
	uv_mutex_unlock(&record->objlock);

	/* no race condition, so it is ok outside of mutex protection */
	record->counter.total_send++;
}

static void _monitor_dataflow_insert_new_record(uint32_t from, uint32_t to)
{
	monitor_dataflow_record_t* record = malloc(sizeof(*record));
	if (record == NULL)
	{/* no memory, no need to try again */
		return;
	}

	record->data.from = from;
	record->data.to = to;
	record->data.count = 1;

	int ret;
	uv_rwlock_wrlock(&g_eaf_monitor_ctx2.dataflow.rwlock);
	{
		ret = eaf_map_insert(&g_eaf_monitor_ctx.dataflow.record, &record->node);
	}
	uv_rwlock_wrunlock(&g_eaf_monitor_ctx2.dataflow.rwlock);

	/* Must success */
	assert(ret == 0);
}

static void _monitor_update_message_path(uint32_t from, uint32_t to)
{
	monitor_dataflow_record_t tmp_key;
	tmp_key.data.from = from;
	tmp_key.data.to = to;

	int flag_need_insert = 0;
	uv_rwlock_rdlock(&g_eaf_monitor_ctx2.dataflow.rwlock);
	do
	{
		eaf_map_node_t* it = eaf_map_find(&g_eaf_monitor_ctx.dataflow.record, &tmp_key.node);
		if (it == NULL)
		{
			flag_need_insert = 1;
			break;
		}

		/* It is OK to modify `count' directly because it won't change rbtree's layout */
		monitor_dataflow_record_t* record = EAF_CONTAINER_OF(it, monitor_dataflow_record_t, node);
		record->data.count++;
	} EAF_MSVC_WARNING_GUARD(4127, while (0));
	uv_rwlock_rdunlock(&g_eaf_monitor_ctx2.dataflow.rwlock);

	if (flag_need_insert)
	{
		_monitor_dataflow_insert_new_record(from, to);
	}
}

static float _monitor_calculate_cpu(uint64_t use, uint64_t total)
{
	if (total == 0)
	{
		return 0.0f;
	}

	float ret = ((float)use / (float)total) * 100;
	const float gate = 99.9f;

	return ret >= gate ? gate : ret;
}

static const char* _monitor_state_2_string(eaf_service_state_t state)
{
	switch (state)
	{
	case eaf_service_state_init:
		return "INIT";

	case eaf_service_state_init_yield:
		return "INIT_Y";

	case eaf_service_state_idle:
		return "IDLE";

	case eaf_service_state_busy:
		return "BUSY";

	case eaf_service_state_yield:
		return "YIELD";

	case eaf_service_state_exit:
		return "EXIT";

	default:
		break;
	}

	return "UNKNOWN";
}

static void _monitor_print_tree_nolock(char* buffer, size_t size)
{
	buffer[0] = '\0';

	APPEND_LINE(buffer, size, "[SERVICE]    [STATE]   [RECV]   [SEND] [MSGQ] [TIME] [CPU%%]\n");

	uint32_t last_gid = (uint32_t)-1;
	monitor_group_record_t* g_record = NULL;

	eaf_map_node_t* it = eaf_map_begin(&g_eaf_monitor_ctx.serivce.record_group);
	for (; it != NULL; it = eaf_map_next(it))
	{
		monitor_service_record_t* record = EAF_CONTAINER_OF(it, monitor_service_record_t, node_group);

		if (last_gid != record->data.gid)
		{
			last_gid = record->data.gid;
			g_record = &g_eaf_monitor_ctx.group.table[last_gid];
			APPEND_LINE(buffer, size, "%u:%lu\n", (unsigned)record->data.gid, g_record->gls->tid);
		}

		APPEND_LINE(buffer, size, "|-%#010"PRIx32" %-7s %8"PRIu32" %8"PRIu32" %6u %6u   %4.1f\n",
			record->data.sid,
			_monitor_state_2_string(record->data.sls->state),
			record->counter.flush_recv,
			record->counter.flush_send,
			(unsigned)eaf_message_queue_size(record->data.sls),
			(unsigned)(record->counter.flush_use_time / 1000 / 1000),
			_monitor_calculate_cpu(record->counter.flush_use_time, g_record->counter.flush_use_time));
	}
}

static void _monitor_reset_flush_nolock(void)
{
	/* reset service counter */
	eaf_map_node_t* it = eaf_map_begin(&g_eaf_monitor_ctx.serivce.record_group);
	for (; it != NULL; it = eaf_map_next(it))
	{
		monitor_service_record_t* record = EAF_CONTAINER_OF(it, monitor_service_record_t, node_group);

		/* Reset counter */
		uv_mutex_lock(&record->objlock);
		{
			record->counter.flush_send = 0;
			record->counter.flush_recv = 0;
			record->counter.flush_use_time = 0;
		}
		uv_mutex_unlock(&record->objlock);
	}

	/* reset group counter */
	size_t i;
	for (i = 0; i < g_eaf_monitor_ctx.group.size; i++)
	{
		monitor_group_record_t* record = &g_eaf_monitor_ctx.group.table[i];

		uv_mutex_lock(&record->objlock);
		{
			record->counter.flush_use_time = 0;
		}
		uv_mutex_unlock(&record->objlock);
	}
}

static int _monitor_on_message_handle_before(uint32_t from, uint32_t to, eaf_msg_t* msg)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(from, msg);

	monitor_service_record_t* record = _monitor_find_serivce(to);
	assert(record != NULL);

	record->temp.use_time_start = uv_hrtime();
	return 0;
}

static void _monitor_on_message_handle_after(uint32_t from, uint32_t to, eaf_msg_t* msg)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(from, msg);

	monitor_service_record_t* record = _monitor_find_serivce(to);
	assert(record != NULL);

	uint64_t use_time = uv_hrtime() - record->temp.use_time_start;

	uv_mutex_lock(&record->objlock);
	{
		record->counter.flush_recv++;
		record->counter.flush_use_time += use_time;
	}
	uv_mutex_unlock(&record->objlock);

	/* no race condition, so it is ok outside of mutex protection */
	record->counter.total_recv++;

	monitor_group_record_t* g_record = &g_eaf_monitor_ctx.group.table[record->data.gid];
	uv_mutex_lock(&g_record->objlock);
	{
		g_record->counter.flush_use_time += use_time;
	}
	uv_mutex_unlock(&g_record->objlock);
}

static void _monitor_on_uv_timer_close(uv_handle_t* handle)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(handle);

	g_eaf_monitor_ctx2.refresh.mask.running = 0;
}

static void _monitor_on_exit_before(void)
{
	uv_timer_stop(&g_eaf_monitor_ctx2.refresh.timer);
	uv_close((uv_handle_t*)&g_eaf_monitor_ctx2.refresh.timer, _monitor_on_uv_timer_close);

	/* wait until timer stop */
	while (g_eaf_monitor_ctx2.refresh.mask.running)
	{
		eaf_thread_sleep(10);
	}
}

static void _monitor_on_load_before(void)
{
	g_eaf_monitor_ctx.group.size = eaf_group_size();
	g_eaf_monitor_ctx.group.table = malloc(sizeof(monitor_service_record_t) * g_eaf_monitor_ctx.group.size);
	assert(g_eaf_monitor_ctx.group.table != NULL);

	size_t i;
	eaf_group_local_t* gls;
	for (gls = eaf_group_begin(), i = 0; gls != NULL; gls = eaf_group_next(gls), i++)
	{
		g_eaf_monitor_ctx.group.table[i].gls = gls;
		g_eaf_monitor_ctx.group.table[i].counter.flush_use_time = 0;
		uv_mutex_init(&g_eaf_monitor_ctx.group.table[i].objlock);

		eaf_service_local_t* sls = eaf_service_begin(gls);
		for (; sls != NULL; sls = eaf_service_next(gls, sls))
		{
			monitor_service_record_t* record = malloc(sizeof(*record));
			assert(record != NULL);

			record->data.gid = i;
			record->data.sid = sls->id;
			record->data.sls = sls;
			record->counter.flush_send = 0;
			record->counter.flush_recv = 0;
			record->counter.flush_use_time = 0;
			record->counter.total_recv = 0;
			record->counter.total_send = 0;

			if (uv_mutex_init(&record->objlock) < 0)
			{
				free(record);
				continue;
			}

			if (eaf_map_insert(&g_eaf_monitor_ctx.serivce.record_split, &record->node_split) < 0)
			{
				free(record);
				continue;
			}
			eaf_map_insert(&g_eaf_monitor_ctx.serivce.record_group, &record->node_group);
		}
	}
}

static void _monitor_on_message_send_after(uint32_t from, uint32_t to, eaf_msg_t* msg, int ret)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(msg);

	/* No need to record failure case */
	if (ret < 0)
	{
		return;
	}

	_monitor_update_message_path(from, to);
	_monitor_update_counter_send(from);
}

/**
 * @brief uv timer callback
 */
static void _monitor_on_timer_cb(uv_timer_t* handle)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(handle);

	/* We cannot block on this lock because it might affect libuv loop */
	if (uv_mutex_trylock(&g_eaf_monitor_ctx2.refresh.objlock) < 0)
	{
		return;
	}

	/* reset all necessary fields */
	_monitor_reset_flush_nolock();

	/* unlock refresh lock */
	uv_mutex_unlock(&g_eaf_monitor_ctx2.refresh.objlock);
}

int eaf_monitor_init(unsigned sec)
{
	int ret = eaf_errno_success;
	if (uv_rwlock_init(&g_eaf_monitor_ctx2.dataflow.rwlock) < 0)
	{
		return eaf_errno_unknown;
	}
	if (uv_mutex_init(&g_eaf_monitor_ctx2.refresh.objlock) < 0)
	{
		ret = eaf_errno_unknown;
		goto err_init_refresh_lock;
	}

	g_eaf_monitor_ctx2.refresh.mask.running = 0;
	if (uv_timer_init(eaf_uv_get(), &g_eaf_monitor_ctx2.refresh.timer) < 0)
	{
		ret = eaf_errno_unknown;
		goto err_init_timer;
	}

	if ((ret = eaf_powerpack_hook_register(&g_pp_hook, sizeof(g_pp_hook))) < 0)
	{
		goto err_register_hook;
	}

	uint64_t timeout = (uint64_t)sec * 1000;

	g_eaf_monitor_ctx2.refresh.mask.running = 1;
	if (uv_timer_start(&g_eaf_monitor_ctx2.refresh.timer, _monitor_on_timer_cb, timeout, timeout) < 0)
	{
		goto err_start_timer;
	}

	eaf_uv_mod();
	return eaf_errno_success;

err_start_timer:
	g_eaf_monitor_ctx2.refresh.mask.running = 0;
	eaf_powerpack_hook_unregister(&g_pp_hook);
err_register_hook:
	uv_close((uv_handle_t*)&g_eaf_monitor_ctx2.refresh.timer, NULL);
err_init_timer:
	uv_mutex_destroy(&g_eaf_monitor_ctx2.refresh.objlock);
err_init_refresh_lock:
	uv_rwlock_destroy(&g_eaf_monitor_ctx2.dataflow.rwlock);
	return ret;
}

void eaf_monitor_exit(void)
{
	eaf_map_node_t* it;

	it = eaf_map_begin(&g_eaf_monitor_ctx.serivce.record_split);
	while (it != NULL)
	{
		eaf_map_node_t* tmp = it;
		it = eaf_map_next(it);
		eaf_map_erase(&g_eaf_monitor_ctx.serivce.record_split, tmp);

		monitor_service_record_t* record = EAF_CONTAINER_OF(tmp, monitor_service_record_t, node_split);
		eaf_map_erase(&g_eaf_monitor_ctx.serivce.record_group, &record->node_group);
		uv_mutex_destroy(&record->objlock);
		free(record);
	}

	it = eaf_map_begin(&g_eaf_monitor_ctx.dataflow.record);
	while (it != NULL)
	{
		eaf_map_node_t* tmp = it;
		it = eaf_map_next(it);
		eaf_map_erase(&g_eaf_monitor_ctx.dataflow.record, tmp);

		monitor_dataflow_record_t* record = EAF_CONTAINER_OF(it, monitor_dataflow_record_t, node);
		free(record);
	}

	uv_rwlock_destroy(&g_eaf_monitor_ctx2.dataflow.rwlock);
	uv_mutex_destroy(&g_eaf_monitor_ctx2.refresh.objlock);

	size_t i;
	for (i = 0; i < g_eaf_monitor_ctx.group.size; i++)
	{
		uv_mutex_destroy(&g_eaf_monitor_ctx.group.table[i].objlock);
	}
	free(g_eaf_monitor_ctx.group.table);
	g_eaf_monitor_ctx.group.table = NULL;
}

void eaf_monitor_print_tree(char* buffer, size_t size)
{
	/**
	 * The refresh process will reset necessary fields.
	 *
	 * For performance consideration, the refresh process is separated into
	 * multiple steps, which means during refresh process, the report might odd.
	 *
	 * So it is better wait for refresh to finish.
	 */

	uv_mutex_lock(&g_eaf_monitor_ctx2.refresh.objlock);
	_monitor_print_tree_nolock(buffer, size);
	uv_mutex_unlock(&g_eaf_monitor_ctx2.refresh.objlock);
}
