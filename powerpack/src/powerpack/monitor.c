/**
 * @file
 * Monitor rely on libuv, so it is important to follow libuv's coding rule:
 * single thread.
 *
 * Monitor has following working procedure:
 * 1. [main] eaf_monitor_init:
 *    Initialize monitor and register callback to powerpack.
 * 2. [main] _monitor_on_load_before:
 *    Detect service layout and create necessary structs.
 * 3. [libuv] _monitor_on_loop_init
 *    Initialize and start libuv related resources.
 * 4. [libuv] _monitor_on_loop_exit
 *    Stop and cleanup libuv related resources.
 * 5. [main] eaf_monitor_exit
 *    Cleanup other resources.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "string.h"
#include "monitor.h"

#define MODULE	"monitor"
#define LOG_TRACE(fmt, ...)	EAF_LOG_TRACE(MODULE, fmt, ##__VA_ARGS__)

static int _monitor_cmp_dataflow_record(const eaf_map_node_t* key1, const eaf_map_node_t* key2, void* arg);
static int _monitor_cmp_service_record_group(const eaf_map_node_t* key1, const eaf_map_node_t* key2, void* arg);
static int _monitor_cmp_service_record_split(const eaf_map_node_t* key1, const eaf_map_node_t* key2, void* arg);
static void _monitor_on_load_before(void);
static void _monitor_on_message_send_after(uint32_t from, uint32_t to, eaf_msg_t* msg, int ret);
static int _monitor_on_message_handle_before(uint32_t from, uint32_t to, eaf_msg_t* msg);
static void _monitor_on_message_handle_after(uint32_t from, uint32_t to, eaf_msg_t* msg);

static eaf_monitor_ctx2_t g_monitor_ctx2;
static eaf_monitor_ctx_t g_monitor_ctx = {
	{
		EAF_MAP_INITIALIZER(_monitor_cmp_service_record_group, NULL),
		EAF_MAP_INITIALIZER(_monitor_cmp_service_record_split, NULL),
	},																		// .serivce
	{
		0, NULL
	},																		// .group
	{
		EAF_MAP_INITIALIZER(_monitor_cmp_dataflow_record, NULL),
	},																		// .dataflow
	{
		0
	},																		// .config
	{
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
			NULL,								// .on_exit_before
			NULL,								// .on_exit_after
		},
		NULL,
		NULL,
	},																		// .hook
};

static void _monitor_reset_flush_nolock(void)
{
	/* reset service counter */
	eaf_map_node_t* it = eaf_map_begin(&g_monitor_ctx.serivce.record_group);
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
	for (i = 0; i < g_monitor_ctx.group.size; i++)
	{
		monitor_group_record_t* record = &g_monitor_ctx.group.table[i];

		uv_mutex_lock(&record->objlock);
		{
			record->counter.flush_use_time = 0;
		}
		uv_mutex_unlock(&record->objlock);
	}
}

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

	eaf_map_node_t* it = eaf_map_find(&g_monitor_ctx.serivce.record_split, &tmp_key.node_split);
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
	uv_rwlock_wrlock(&g_monitor_ctx2.dataflow.rwlock);
	{
		ret = eaf_map_insert(&g_monitor_ctx.dataflow.record, &record->node);
	}
	uv_rwlock_wrunlock(&g_monitor_ctx2.dataflow.rwlock);

	/* Must success */
	assert(ret == 0);
	EAF_SUPPRESS_UNUSED_VARIABLE(ret); // maybe unused
}

static void _monitor_update_message_path(uint32_t from, uint32_t to)
{
	monitor_dataflow_record_t tmp_key;
	tmp_key.data.from = from;
	tmp_key.data.to = to;

	int flag_need_insert = 0;
	uv_rwlock_rdlock(&g_monitor_ctx2.dataflow.rwlock);
	do
	{
		eaf_map_node_t* it = eaf_map_find(&g_monitor_ctx.dataflow.record, &tmp_key.node);
		if (it == NULL)
		{
			flag_need_insert = 1;
			break;
		}

		/* It is OK to modify `count' directly because it won't change rbtree's layout */
		monitor_dataflow_record_t* record = EAF_CONTAINER_OF(it, monitor_dataflow_record_t, node);
		record->data.count++;
	} EAF_MSVC_WARNING_GUARD(4127, while (0));
	uv_rwlock_rdunlock(&g_monitor_ctx2.dataflow.rwlock);

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
	const float gate = 100.0f;

	return ret > gate ? gate : ret;
}

static const char* _monitor_state_2_string(eaf_service_state_t state)
{
	switch (state)
	{
	case eaf_service_state_init0:
		return "init0";

	case eaf_service_state_init1:
		return "init1";

	case eaf_service_state_init2:
		return "init2";

	case eaf_service_state_idle:
		return "idle";

	case eaf_service_state_busy:
		return "busy";

	case eaf_service_state_yield:
		return "yield";

	case eaf_service_state_exit0:
		return "exit0";

	case eaf_service_state_exit1:
		return "exit1";

	default:
		break;
	}

	return "unknown";
}

static int _monitor_on_message_handle_before(uint32_t from, uint32_t to, eaf_msg_t* msg)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(from, msg);

	monitor_service_record_t* record = _monitor_find_serivce(to);
	if (record == NULL)
	{
		return 0;
	}

	record->temp.use_time_start = uv_hrtime();
	return 0;
}

static void _monitor_on_message_handle_after(uint32_t from, uint32_t to, eaf_msg_t* msg)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(from, msg);

	monitor_service_record_t* record = _monitor_find_serivce(to);
	if (record == NULL)
	{
		return;
	}

	uint64_t use_time = uv_hrtime() - record->temp.use_time_start;

	uv_mutex_lock(&record->objlock);
	{
		record->counter.flush_recv++;
		record->counter.flush_use_time += use_time;
	}
	uv_mutex_unlock(&record->objlock);

	/* no race condition, so it is ok outside of mutex protection */
	record->counter.total_recv++;

	monitor_group_record_t* g_record = &g_monitor_ctx.group.table[record->data.gid];
	uv_mutex_lock(&g_record->objlock);
	{
		g_record->counter.flush_use_time += use_time;
	}
	uv_mutex_unlock(&g_record->objlock);
}

static void _monitor_on_load_before(void)
{
	g_monitor_ctx.group.size = eaf_group_size();
	g_monitor_ctx.group.table = malloc(sizeof(monitor_group_record_t) * g_monitor_ctx.group.size);
	assert(g_monitor_ctx.group.table != NULL);

	size_t idx = 0;
	eaf_group_local_t* gls;
	for (gls = eaf_group_begin(); gls != NULL; gls = eaf_group_next(gls), idx++)
	{
		assert(idx < g_monitor_ctx.group.size);

		g_monitor_ctx.group.table[idx].gls = gls;
		g_monitor_ctx.group.table[idx].counter.flush_use_time = 0;
		if (uv_mutex_init(&g_monitor_ctx.group.table[idx].objlock) < 0)
		{
			goto err_init_mutex;
		}

		eaf_service_local_t* sls = eaf_service_begin(gls);
		for (; sls != NULL; sls = eaf_service_next(gls, sls))
		{
			monitor_service_record_t* record = malloc(sizeof(*record));
			assert(record != NULL);

			record->data.gid = (uint32_t)idx;
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

			if (eaf_map_insert(&g_monitor_ctx.serivce.record_split, &record->node_split) < 0)
			{
				uv_mutex_destroy(&record->objlock);
				free(record);
				continue;
			}
			eaf_map_insert(&g_monitor_ctx.serivce.record_group, &record->node_group);
		}
	}

	return;

err_init_mutex:
	assert(0);
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

static void _monitor_on_req_stringify_error(uint32_t from, eaf_msg_t* req)
{
	eaf_msg_t* rsp = eaf_msg_create_rsp(req, sizeof(eaf_monitor_stringify_rsp_t));
	((eaf_monitor_stringify_rsp_t*)eaf_msg_get_data(rsp, NULL))->size = 0;

	eaf_send_rsp(EAF_MONITOR_ID, from, rsp);
	eaf_msg_dec_ref(rsp);
}

static size_t _monitor_stringify_normal_fill_nolock(char* buffer, size_t size)
{
	size_t token = 0;
	size_t write_size = 0;

	write_size += eaf_string_apply(buffer, size, &token, "[SERVICE]    [STATE]   [RECV]   [SEND] [MSGQ] [TIME] [CPU%%]\n");

	uint32_t last_gid = (uint32_t)-1;
	monitor_group_record_t* g_record = NULL;

	eaf_map_node_t* it = eaf_map_begin(&g_monitor_ctx.serivce.record_group);
	for (; it != NULL; it = eaf_map_next(it))
	{
		monitor_service_record_t* record = EAF_CONTAINER_OF(it, monitor_service_record_t, node_group);

		if (last_gid != record->data.gid)
		{
			last_gid = record->data.gid;
			g_record = &g_monitor_ctx.group.table[last_gid];
			write_size += eaf_string_apply(buffer, size, &token, "%u:%lu\n", (unsigned)record->data.gid, g_record->gls->tid);
		}

		write_size += eaf_string_apply(buffer, size, &token, "|-0x%08"PRIx32" %-7s %8"PRIu32" %8"PRIu32" %6u %6u  %5.1f\n",
			record->data.sid,
			_monitor_state_2_string(record->data.sls->state),
			record->counter.flush_recv,
			record->counter.flush_send,
			(unsigned)eaf_message_queue_size(record->data.sls),
			(unsigned)(record->counter.flush_use_time / 1000 / 1000),
			_monitor_calculate_cpu(record->counter.flush_use_time, g_record->counter.flush_use_time));
	}

	return write_size;
}

static void _monitor_on_req_stringify_normal(uint32_t from, eaf_msg_t* req)
{
	/**
	 * The template is:
	 * 
	 * [SERVICE]    [STATE]   [RECV]   [SEND] [MSGQ] [TIME] [CPU%]      --> 59 + 1
	 * [uint32_t]:[uint64_t]                                            --> 31 + 1
	 * |-0x[    8C] [ 7C  ] [    8C] [    8C] [  6C] [  6C]  [ 5C]      --> 59 + 1
	 * |-0x[    8C] [ 7C  ] [    8C] [    8C] [  6C] [  6C]  [ 5C]
	 * |-0x[    8C] [ 7C  ] [    8C] [    8C] [  6C] [  6C]  [ 5C]
	 * [uint32_t]:[uint64_t]
	 * |-0xe0010000 [ 7C  ] [    8C] [    8C] [  6C] [  6C]  [ 5C]
	 * |-0xe0020000 [ 7C  ] [    8C] [    8C] [  6C] [  6C]  [ 5C]
	 * |-0xe0030000 [ 7C  ] [    8C] [    8C] [  6C] [  6C]  [ 5C]
	 *
	 * Which means the maximum length (include NULL terminator) is:
	 * N = 60 + 32 * G + 60 * S + 1
	 */

	size_t string_size = 60 + 32 * g_monitor_ctx.group.size + 60 * eaf_map_size(&g_monitor_ctx.serivce.record_split) + 1;
	eaf_msg_t* rsp = eaf_msg_create_rsp(req, sizeof(eaf_monitor_stringify_rsp_t) + string_size);
	eaf_monitor_stringify_rsp_t* monitor_rsp = eaf_msg_get_data(rsp, NULL);

	/**
	 * The refresh process will reset necessary fields.
	 *
	 * For performance consideration, the refresh process is separated into
	 * multiple steps, which means during refresh process, the report might odd.
	 *
	 * So it is better wait for refresh to finish.
	 */
	uv_mutex_lock(&g_monitor_ctx2.refresh.objlock);
	{
		monitor_rsp->size =
			_monitor_stringify_normal_fill_nolock(monitor_rsp->data, string_size);
	}
	uv_mutex_unlock(&g_monitor_ctx2.refresh.objlock);

	eaf_send_rsp(EAF_MONITOR_ID, from, rsp);
	eaf_msg_dec_ref(rsp);
}

static size_t _monitor_stringify_json_fill_nolock(char* buffer, size_t size)
{
	size_t token = 0;
	size_t write_size = 0;
	int flag_write_service_end = 0;

	/* The begin of json string */
	write_size += eaf_string_apply(buffer, size, &token, "{\"group\":[");

	uint32_t last_gid = (uint32_t)-1;
	monitor_group_record_t* g_record = NULL;

	eaf_map_node_t* it = eaf_map_begin(&g_monitor_ctx.serivce.record_group);
	for (; it != NULL; it = eaf_map_next(it))
	{
		monitor_service_record_t* record = EAF_CONTAINER_OF(it, monitor_service_record_t, node_group);

		if (last_gid != record->data.gid)
		{
			if (g_record != NULL)
			{
				write_size += eaf_string_apply(buffer, size, &token, "]},");
			}

			last_gid = record->data.gid;
			g_record = &g_monitor_ctx.group.table[last_gid];
			write_size += eaf_string_apply(buffer, size, &token,
				"{\"gid\":%u,\"tid\":%lu,\"service\":[", (unsigned)record->data.gid, g_record->gls->tid);

			flag_write_service_end = 0;
		}

		if (flag_write_service_end)
		{
			write_size += eaf_string_apply(buffer, size, &token, ",");
		}
		flag_write_service_end = 1;

		write_size += eaf_string_apply(buffer, size, &token,
			"{\"id\":%"PRIu32",\"state\":\"%s\",\"recv\":%u,\"send\":%u,\"msgq\":%u,\"time\":%u,\"cpu\":%.1f}",
			record->data.sid,
			_monitor_state_2_string(record->data.sls->state),
			(unsigned)record->counter.flush_recv,
			(unsigned)record->counter.flush_send,
			(unsigned)eaf_message_queue_size(record->data.sls),
			(unsigned)(record->counter.flush_use_time / 1000 / 1000),
			_monitor_calculate_cpu(record->counter.flush_use_time, g_record->counter.flush_use_time));
	}

	/* close group and finish json */
	write_size += eaf_string_apply(buffer, size, &token, "]}]}");

	return write_size;
}

static void _monitor_on_req_stringify_json(uint32_t from, eaf_msg_t* req)
{
	/**
	 * The template is:
	 * {
	 * 	"group" : [
	 * 		{
	 * 			"gid" : <uint32_t>,
	 * 			"tid" : <uint64_t>,
	 * 			"service": [
	 * 				{
	 * 					"id": <uint32_t>,
	 * 					"state": "<7C>",
	 * 					"recv": <uint32_t>,
	 * 					"send": <uint32_t>,
	 * 					"msgq": <uint32_t>,
	 * 					"time": <uint32_t>,
	 * 					"cpu": <float>
	 * 				},
	 * 				{
	 * 					"id": <uint32_t>,
	 * 					"state": "<7C>",
	 * 					"recv": <uint32_t>,
	 * 					"send": <uint32_t>,
	 * 					"msgq": <uint32_t>,
	 * 					"time": <uint32_t>,
	 * 					"cpu": <float>
	 * 				}
	 * 			]
	 * 		},
	 * 		{
	 * 			"gid" : <uint32_t>,
	 * 			"tid" : <uint64_t>,
	 * 			"service": [
	 * 				{
	 * 					"id": <uint32_t>,
	 * 					"state": "<7C>",
	 * 					"recv": <uint32_t>,
	 * 					"send": <uint32_t>,
	 * 					"msgq": <uint32_t>,
	 * 					"time": <uint32_t>,
	 * 					"cpu": <float>
	 * 				},
	 * 				{
	 * 					...
	 * 				}
	 * 			]
	 * 		}
	 * 	]
	 * }
	 *
	 * Which means the maximum length (include NULL terminator) is:
	 * N = 12 + 59 * G + 120 * S + 1
	 */

	size_t string_size = 12 + 59 * g_monitor_ctx.group.size + 120 * eaf_map_size(&g_monitor_ctx.serivce.record_split) + 1;
	eaf_msg_t* rsp = eaf_msg_create_rsp(req, sizeof(eaf_monitor_stringify_rsp_t) + string_size);
	eaf_monitor_stringify_rsp_t* monitor_rsp = eaf_msg_get_data(rsp, NULL);

	uv_mutex_lock(&g_monitor_ctx2.refresh.objlock);
	{
		monitor_rsp->size =
			_monitor_stringify_json_fill_nolock(monitor_rsp->data, string_size);
	}
	uv_mutex_unlock(&g_monitor_ctx2.refresh.objlock);

	eaf_send_rsp(EAF_MONITOR_ID, from, rsp);
	eaf_msg_dec_ref(rsp);
}

static void _monitor_on_req_stringify(uint32_t from, uint32_t to, eaf_msg_t* msg)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(to);

	eaf_monitor_stringify_req_t* req = eaf_msg_get_data(msg, NULL);
	switch (req->type)
	{
	case eaf_monitor_stringify_type_normal:
		_monitor_on_req_stringify_normal(from, msg);
		break;

	case eaf_monitor_stringify_type_json:
		_monitor_on_req_stringify_json(from, msg);
		break;

	default:
		_monitor_on_req_stringify_error(from, msg);
		break;
	}
}

static void _monitor_on_timer(uint32_t from, uint32_t to, eaf_msg_t* msg)
{
    int ret;
	EAF_SUPPRESS_UNUSED_VARIABLE(from, to, msg);

	/* We cannot block on this lock because it might affect libuv loop */
	if (uv_mutex_trylock(&g_monitor_ctx2.refresh.objlock) < 0)
	{
		return;
	}

	/* reset all necessary fields */
	_monitor_reset_flush_nolock();

	/* unlock refresh lock */
	uv_mutex_unlock(&g_monitor_ctx2.refresh.objlock);

	/* restart timer */
	EAF_TIMER_DELAY(ret, EAF_MONITOR_ID, _monitor_on_timer, g_monitor_ctx.config.timeout_sec * 1000);
    EAF_SUPPRESS_UNUSED_VARIABLE(ret);
}

static void _monitor_on_service_init(void)
{
	int ret;
	EAF_TIMER_DELAY(ret, EAF_MONITOR_ID, _monitor_on_timer, g_monitor_ctx.config.timeout_sec * 1000);

	if (ret < 0)
	{
		eaf_exit(ret);
	}
}

static void _monitor_on_service_exit(void)
{
	// do nothing
}

static void _monitor_on_req_flush(uint32_t from, uint32_t to, eaf_msg_t* msg)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(to);

	uv_mutex_lock(&g_monitor_ctx2.refresh.objlock);
	_monitor_reset_flush_nolock();
	uv_mutex_unlock(&g_monitor_ctx2.refresh.objlock);

	eaf_msg_t* rsp = eaf_msg_create_rsp(msg, sizeof(eaf_monitor_flush_rsp_t));
	((eaf_monitor_flush_rsp_t*)eaf_msg_get_data(rsp, NULL))->ret = eaf_errno_success;
	eaf_send_rsp(EAF_MONITOR_ID, from, rsp);
	eaf_msg_dec_ref(rsp);
}

EAF_API
int eaf_monitor_init(unsigned sec)
{
	int ret = eaf_errno_success;
	g_monitor_ctx.config.timeout_sec = sec;

	memset(&g_monitor_ctx2, 0, sizeof(g_monitor_ctx2));

	if (uv_rwlock_init(&g_monitor_ctx2.dataflow.rwlock) < 0)
	{
		return eaf_errno_unknown;
	}
	if (uv_mutex_init(&g_monitor_ctx2.refresh.objlock) < 0)
	{
		ret = eaf_errno_unknown;
		goto err_init_refresh_lock;
	}

	if ((ret = eaf_powerpack_hook_register(&g_monitor_ctx.hook, sizeof(g_monitor_ctx.hook))) < 0)
	{
		goto err_init_timer;
	}

	static eaf_message_table_t msg_table[] = {
		{ EAF_MINITOR_MSG_STRINGIFY_REQ,	_monitor_on_req_stringify },
		{ EAF_MONITOR_MSG_FLUSH_REQ,		_monitor_on_req_flush },
	};

	static eaf_entrypoint_t entry = {
		EAF_ARRAY_SIZE(msg_table), msg_table,
		_monitor_on_service_init,
		_monitor_on_service_exit,
	};

	if ((ret = eaf_register(EAF_MONITOR_ID, &entry)) < 0)
	{
		goto err_register_service;
	}

	return eaf_errno_success;

err_register_service:
	eaf_powerpack_hook_unregister(&g_monitor_ctx.hook);
err_init_timer:
	uv_mutex_destroy(&g_monitor_ctx2.refresh.objlock);
err_init_refresh_lock:
	uv_rwlock_destroy(&g_monitor_ctx2.dataflow.rwlock);
	return ret;
}

EAF_API
void eaf_monitor_exit(void)
{
	eaf_map_node_t* it;

	/* wait until timer stop */
	while (g_monitor_ctx2.mask.refresh_timer_running)
	{
		eaf_thread_sleep(10);
	}

	it = eaf_map_begin(&g_monitor_ctx.serivce.record_split);
	while (it != NULL)
	{
		eaf_map_node_t* tmp = it;
		it = eaf_map_next(it);
		eaf_map_erase(&g_monitor_ctx.serivce.record_split, tmp);

		monitor_service_record_t* record = EAF_CONTAINER_OF(tmp, monitor_service_record_t, node_split);
		eaf_map_erase(&g_monitor_ctx.serivce.record_group, &record->node_group);
		uv_mutex_destroy(&record->objlock);
		free(record);
	}

	it = eaf_map_begin(&g_monitor_ctx.dataflow.record);
	while (it != NULL)
	{
		eaf_map_node_t* tmp = it;
		it = eaf_map_next(it);
		eaf_map_erase(&g_monitor_ctx.dataflow.record, tmp);

		monitor_dataflow_record_t* record = EAF_CONTAINER_OF(tmp, monitor_dataflow_record_t, node);
		free(record);
	}

	uv_rwlock_destroy(&g_monitor_ctx2.dataflow.rwlock);
	uv_mutex_destroy(&g_monitor_ctx2.refresh.objlock);

	size_t i;
	for (i = 0; i < g_monitor_ctx.group.size; i++)
	{
		uv_mutex_destroy(&g_monitor_ctx.group.table[i].objlock);
	}
	free(g_monitor_ctx.group.table);
	g_monitor_ctx.group.table = NULL;
}
