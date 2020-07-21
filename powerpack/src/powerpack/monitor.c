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

static int _monitor_on_cmp_dataflow_record(const eaf_map_node_t* key1, const eaf_map_node_t* key2, void* arg);
static int _monitor_on_cmp_service_record_group(const eaf_map_node_t* key1, const eaf_map_node_t* key2, void* arg);
static int _monitor_on_cmp_service_record_split(const eaf_map_node_t* key1, const eaf_map_node_t* key2, void* arg);
static void _monitor_on_load_before(void);
static void _monitor_on_message_send_after(uint32_t from, uint32_t to, eaf_msg_t* msg, int ret);
static void _monitor_on_message_handle_after(uint32_t from, uint32_t to, eaf_msg_t* msg);

typedef struct monitor_dataflow_record
{
	eaf_map_node_t				node;			/**< node for eaf_monitor_ctx_t::dataflow::record */
	struct
	{
		uint32_t				from;			/** Who send this message */
		uint32_t				to;				/** Who will receive this message */
		size_t					count;			/** Message count */
	}data;
}monitor_dataflow_record_t;

typedef struct monitor_service_record
{
	eaf_map_node_t				node_group;		/**< node for eaf_monitor_ctx_t::serivce::record_group */
	eaf_map_node_t				node_split;		/**< node for eaf_monitor_ctx_t::serivce::record_split */

	struct
	{
		uint32_t				gid;			/**< Group ID */
		uint32_t				sid;			/**< Service ID. It must be unique */
		eaf_service_local_t*	sls;			/**< Service Local Storage */
	}data;

	struct
	{
		uint64_t				cnt_send;		/** The number of message send */
		uint64_t				cnt_recv;		/** The number of message recv */
	}counter;
}monitor_service_record_t;

typedef struct eaf_monitor_ctx
{
	struct
	{
		eaf_map_t				record_group;	/**< Group by GID, search by `gid' and `sid' */
		eaf_map_t				record_split;	/**< Independent record, search by `sid' */
	}serivce;

	struct
	{
		eaf_map_t				record;			/**< A insert-only table for record message flow */
	}dataflow;
}eaf_monitor_ctx_t;

typedef struct eaf_monitor_ctx2
{
	struct
	{
		uv_rwlock_t				rwlock;
	}dataflow;
}eaf_monitor_ctx2_t;

static eaf_monitor_ctx2_t g_eaf_monitor_ctx2;
static eaf_monitor_ctx_t g_eaf_monitor_ctx = {
	{
		EAF_MAP_INITIALIZER(_monitor_on_cmp_service_record_group, NULL),
		EAF_MAP_INITIALIZER(_monitor_on_cmp_service_record_split, NULL),
	},																		// .serivce
	{
		EAF_MAP_INITIALIZER(_monitor_on_cmp_dataflow_record, NULL),
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
		NULL,								// .on_message_send_before
		_monitor_on_message_send_after,		// .on_message_send_after
		NULL,								// .on_message_handle_before
		_monitor_on_message_handle_after,	// .on_message_handle_after
		_monitor_on_load_before,			// .on_load_before
		NULL,								// .on_load_after
		NULL,								// .on_exit_before
		NULL,								// .on_exit_after
	}
};

static int _monitor_on_cmp_service_record_group(const eaf_map_node_t* key1, const eaf_map_node_t* key2, void* arg)
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

static int _monitor_on_cmp_service_record_split(const eaf_map_node_t* key1, const eaf_map_node_t* key2, void* arg)
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

static int _monitor_on_cmp_dataflow_record(const eaf_map_node_t* key1, const eaf_map_node_t* key2, void* arg)
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

static void _monitor_update_counter_recv(uint32_t sid)
{
	monitor_service_record_t* record = _monitor_find_serivce(sid);
	record->counter.cnt_recv++;
}

static void _monitor_on_message_handle_after(uint32_t from, uint32_t to, eaf_msg_t* msg)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(from, msg);

	_monitor_update_counter_recv(to);
}

static void _monitor_on_load_before(void)
{
	uint32_t idx_gls;
	eaf_group_local_t* gls = eaf_group_begin();
	for (idx_gls = 0; gls != NULL; gls = eaf_group_next(gls), idx_gls++)
	{
		eaf_service_local_t* sls = eaf_service_begin(gls);
		for (; sls != NULL; sls = eaf_service_next(gls, sls))
		{
			monitor_service_record_t* record = malloc(sizeof(*record));
			assert(record != NULL);

			record->data.gid = idx_gls;
			record->data.sid = sls->id;
			record->data.sls = sls;
			record->counter.cnt_send = 0;
			record->counter.cnt_recv = 0;

			if (eaf_map_insert(&g_eaf_monitor_ctx.serivce.record_split, &record->node_split) < 0)
			{
				free(record);
				continue;
			}
			eaf_map_insert(&g_eaf_monitor_ctx.serivce.record_group, &record->node_group);
		}
	}
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

static void _monitor_update_counter_send(uint32_t sid)
{
	monitor_service_record_t* record = _monitor_find_serivce(sid);
	record->counter.cnt_send++;
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

int eaf_monitor_init(void)
{
	if (uv_rwlock_init(&g_eaf_monitor_ctx2.dataflow.rwlock) < 0)
	{
		return eaf_errno_unknown;
	}

	return eaf_powerpack_hook_register(&g_pp_hook, sizeof(g_pp_hook));
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

void eaf_monitor_print_tree(char* buffer, size_t size)
{
	buffer[0] = '\0';

	APPEND_LINE(buffer, size, "[SERVICE]        [STATE] [RECV]     [SEND]     [S/C]\n");

	uint32_t last_gid = (uint32_t)-1;
	eaf_map_node_t* it = eaf_map_begin(&g_eaf_monitor_ctx.serivce.record_group);
	for (; it != NULL; it = eaf_map_next(it))
	{
		monitor_service_record_t* record = EAF_CONTAINER_OF(it, monitor_service_record_t, node_group);

		if (last_gid != record->data.gid)
		{
			APPEND_LINE(buffer, size, "|- %u\n", (unsigned)record->data.gid);
			last_gid = record->data.gid;
		}

		APPEND_LINE(buffer, size, "|  |- %#010"PRIx32" %-7s %-10"PRIu64" %-10"PRIu64" %u/%u\n",
			record->data.sid,
			_monitor_state_2_string(record->data.sls->state),
			record->counter.cnt_recv,
			record->counter.cnt_send,
			(unsigned)eaf_message_queue_size(record->data.sls),
			(unsigned)eaf_message_queue_capacity(record->data.sls));
	}
}
