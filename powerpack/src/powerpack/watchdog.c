#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "eaf/eaf.h"
#include "eaf/powerpack/time.h"
#include "eaf/powerpack/timer.h"
#include "watchdog.h"

#define WATCHDOG_TIMER_INTERVAL		100

static int _watchdog_on_cmp_record(const eaf_map_node_t*, const eaf_map_node_t*, void*);
static int _watchdog_on_cmp_timeout(const eaf_map_node_t*, const eaf_map_node_t*, void*);

typedef struct eaf_watchdog_record
{
	eaf_map_node_t					node_timeout;
	eaf_map_node_t					node_record;
	struct
	{
		uint32_t					id;			/**< ID */
		uint32_t					timeout;	/**< Timeout in milliseconds */
		eaf_clock_time_t			timestamp;	/**< Active time */
	}data;
}eaf_watchdog_record_t;

typedef struct eaf_watchdog_ctx
{
	eaf_map_t						record_table;
	eaf_map_t						timeout_table;

	struct
	{
		eaf_watchdog_on_error_fn	fn;
		void*						arg;
	}cb;
}eaf_watchdog_ctx_t;

static eaf_watchdog_ctx_t g_watchdog_ctx = {
	EAF_MAP_INITIALIZER(_watchdog_on_cmp_record, NULL),
	EAF_MAP_INITIALIZER(_watchdog_on_cmp_timeout, NULL),
	{ NULL, NULL },
};

static int _watchdog_on_cmp_record(const eaf_map_node_t* key1, const eaf_map_node_t* key2, void* arg)
{
	(void)arg;
	eaf_watchdog_record_t* rec_1 = EAF_CONTAINER_OF(key1, eaf_watchdog_record_t, node_record);
	eaf_watchdog_record_t* rec_2 = EAF_CONTAINER_OF(key2, eaf_watchdog_record_t, node_record);
	if (rec_1->data.id == rec_2->data.id)
	{
		return 0;
	}
	return rec_1->data.id < rec_2->data.id ? -1 : 1;
}

static int _watchdog_on_cmp_timeout(const eaf_map_node_t* key1, const eaf_map_node_t* key2, void* arg)
{
	(void)arg;
	eaf_watchdog_record_t* rec_1 = EAF_CONTAINER_OF(key1, eaf_watchdog_record_t, node_timeout);
	eaf_watchdog_record_t* rec_2 = EAF_CONTAINER_OF(key2, eaf_watchdog_record_t, node_timeout);

	/* compare timestamp */
	int ret;
	if ((ret = eaf_time_diffclock(&rec_1->data.timestamp, &rec_2->data.timestamp, NULL)) != 0)
	{
		return ret;
	}

	/* compare id */
	if (rec_1->data.id == rec_2->data.id)
	{
		return 0;
	}
	return rec_1->data.id < rec_2->data.id ? -1 : 1;
}

static void _watchdog_send_response_register(uint32_t to, eaf_msg_t* req, int32_t ret)
{
	eaf_msg_t* rsp = eaf_msg_create_rsp(req, sizeof(eaf_watchdog_register_rsp_t));
	assert(rsp != NULL);
	((eaf_watchdog_register_rsp_t*)eaf_msg_get_data(rsp, NULL))->ret = ret;
	eaf_send_rsp(EAF_WATCHDOG_ID, to, rsp);
	eaf_msg_dec_ref(rsp);
}

static void _watchdog_send_response_unregister(uint32_t to, eaf_msg_t* req, int32_t ret)
{
	eaf_msg_t* rsp = eaf_msg_create_rsp(req, sizeof(eaf_watchdog_unregister_rsp_t));
	assert(rsp != NULL);

	((eaf_watchdog_unregister_rsp_t*)eaf_msg_get_data(rsp, NULL))->ret = ret;
	eaf_send_rsp(EAF_WATCHDOG_ID, to, rsp);
	eaf_msg_dec_ref(rsp);
}

static void _watchdog_send_response_heartbeat(uint32_t to, eaf_msg_t* req, int32_t ret)
{
	eaf_msg_t* rsp = eaf_msg_create_rsp(req, sizeof(eaf_watchdog_heartbeat_rsp_t));
	assert(rsp != NULL);

	((eaf_watchdog_heartbeat_rsp_t*)eaf_msg_get_data(rsp, NULL))->ret = ret;
	eaf_send_rsp(EAF_WATCHDOG_ID, to, rsp);
	eaf_msg_dec_ref(rsp);
}

static void _watchdog_set_timestamp(eaf_clock_time_t* t, uint32_t ms)
{
	t->tv_sec = ms / 1000;
	t->tv_usec = (ms - (uint32_t)t->tv_sec * 1000) * 1000;
}

static void _watchdog_calculate_timeout(eaf_watchdog_record_t* record)
{
	int ret; EAF_SUPPRESS_UNUSED_VARIABLE(ret);

	ret = eaf_time_getclock(&record->data.timestamp);
	assert(ret == 0);

	eaf_clock_time_t timeout;
	_watchdog_set_timestamp(&timeout, record->data.timeout);

	/* calculate dead time */
	eaf_time_addclock(&record->data.timestamp, &timeout);
}

static void _watchdog_on_timer(uint32_t from, uint32_t to, struct eaf_msg* msg)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(from, to, msg);

	/* Require timer delay */
	int ret; EAF_SUPPRESS_UNUSED_VARIABLE(ret);
	EAF_TIMER_DELAY(ret, EAF_WATCHDOG_ID, _watchdog_on_timer, WATCHDOG_TIMER_INTERVAL);

	/* Get the oldest timer in timer stack */
	eaf_map_node_t* it = eaf_map_begin(&g_watchdog_ctx.timeout_table);
	if (it == NULL)
	{
		return;
	}

	/* Get current time */
	eaf_clock_time_t current_time;
	ret = eaf_time_getclock(&current_time);
	assert(ret == 0);

	/* Check timestamp */
	eaf_watchdog_record_t* record = EAF_CONTAINER_OF(it, eaf_watchdog_record_t, node_timeout);
	if (eaf_time_diffclock(&record->data.timestamp, &current_time, NULL) > 0)
	{
		return;
	}

	g_watchdog_ctx.cb.fn(record->data.id, g_watchdog_ctx.cb.arg);
}

static void _watchdog_register_heartbeat(eaf_watchdog_record_t* record)
{
	_watchdog_calculate_timeout(record);

	if (eaf_map_insert(&g_watchdog_ctx.timeout_table, &record->node_timeout) < 0)
	{// should not fail
		assert(0);
	}
}

static void _watchdog_on_req_register(uint32_t from, uint32_t to, struct eaf_msg* msg)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(to);

	int ret;
	eaf_watchdog_register_req_t* req = eaf_msg_get_data(msg, NULL);

	eaf_watchdog_record_t* record = malloc(sizeof(eaf_watchdog_record_t));
	memset(&record->data, 0, sizeof(record->data));
	record->data.id = req->id;
	record->data.timeout = req->timeout;

	/* save record */
	if ((ret = eaf_map_insert(&g_watchdog_ctx.record_table, &record->node_record)) < 0)
	{
		ret = eaf_errno_duplicate;
		goto err_duplicate;
	}

	if (record->data.timeout != 0)
	{
		_watchdog_register_heartbeat(record);
	}

	_watchdog_send_response_register(from, msg, eaf_errno_success);
	return;

err_duplicate:
	_watchdog_send_response_register(from, msg, ret);
	free(record);
	return;
}

static eaf_watchdog_record_t* _watchdog_find_record(uint32_t id)
{
	eaf_watchdog_record_t tmp;
	tmp.data.id = id;

	eaf_map_node_t* it = eaf_map_find(&g_watchdog_ctx.record_table, &tmp.node_record);
	if (it == NULL)
	{
		return NULL;
	}

	return EAF_CONTAINER_OF(it, eaf_watchdog_record_t, node_record);
}

static void _watchdog_on_req_unregister(uint32_t from, uint32_t to, struct eaf_msg* msg)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(to);
	eaf_watchdog_record_t* record =
		_watchdog_find_record(((eaf_watchdog_unregister_req_t*)eaf_msg_get_data(msg, NULL))->id);
	if (record == NULL)
	{
		_watchdog_send_response_unregister(from, msg, eaf_errno_notfound);
		return;
	}

	eaf_map_erase(&g_watchdog_ctx.record_table, &record->node_record);
	if (record->data.timeout != 0)
	{
		eaf_map_erase(&g_watchdog_ctx.timeout_table, &record->node_timeout);
	}

	free(record);
	_watchdog_send_response_unregister(from, msg, eaf_errno_success);
}

static void _watchdog_on_req_heartbeat(uint32_t from, uint32_t to, struct eaf_msg* msg)
{
	(void)to;
	uint32_t id = ((eaf_watchdog_heartbeat_req_t*)eaf_msg_get_data(msg, NULL))->id;
	eaf_watchdog_record_t* record = _watchdog_find_record(id);
	if (record == NULL)
	{
		_watchdog_send_response_heartbeat(from, msg, eaf_errno_notfound);
		return;
	}

	if (record->data.timeout == 0)
	{
		g_watchdog_ctx.cb.fn(id, g_watchdog_ctx.cb.arg);
		return;
	}

	/* resort */
	eaf_map_erase(&g_watchdog_ctx.timeout_table, &record->node_timeout);
	_watchdog_calculate_timeout(record);
	eaf_map_insert(&g_watchdog_ctx.timeout_table, &record->node_timeout);

	_watchdog_send_response_heartbeat(from, msg, eaf_errno_success);
}

static int _watchdog_on_init(void)
{
	int ret;
	EAF_TIMER_DELAY(ret, EAF_WATCHDOG_ID, _watchdog_on_timer, WATCHDOG_TIMER_INTERVAL);
	return ret;
}

static void _watchdog_on_exit(void)
{
	// do nothing
}

int eaf_watchdog_init(_In_ eaf_watchdog_on_error_fn fn, _Inout_opt_ void* arg)
{
	if (g_watchdog_ctx.cb.fn != NULL)
	{
		return eaf_errno_duplicate;
	}

	static eaf_message_table_t msg_table[] = {
		{ EAF_WATCHDOG_MSG_REGISTER_REQ, _watchdog_on_req_register },
		{ EAF_WATCHDOG_MSG_UNREGISTER_REQ, _watchdog_on_req_unregister },
		{ EAF_WATCHDOG_MSG_HEARTBEAT_REQ, _watchdog_on_req_heartbeat },
	};

	static eaf_entrypoint_t entry = {
		EAF_ARRAY_SIZE(msg_table), msg_table,
		_watchdog_on_init,
		_watchdog_on_exit,
	};

	g_watchdog_ctx.cb.fn = fn;
	g_watchdog_ctx.cb.arg = arg;
	return eaf_register(EAF_WATCHDOG_ID, &entry);
}

void eaf_watchdog_exit(void)
{
	if (g_watchdog_ctx.cb.fn == NULL)
	{
		return;
	}

	eaf_map_node_t* it = eaf_map_begin(&g_watchdog_ctx.record_table);
	while (it != NULL)
	{
		eaf_map_node_t* tmp = it;
		it = eaf_map_next(it);
		eaf_map_erase(&g_watchdog_ctx.record_table, tmp);

		eaf_watchdog_record_t* record = EAF_CONTAINER_OF(tmp, eaf_watchdog_record_t, node_record);
		eaf_map_erase(&g_watchdog_ctx.record_table, &record->node_record);
		if (record->data.timeout != 0)
		{
			eaf_map_erase(&g_watchdog_ctx.timeout_table, &record->node_timeout);
		}
		free(record);
	}

	g_watchdog_ctx.cb.fn = NULL;
	g_watchdog_ctx.cb.arg = NULL;

	return;
}
