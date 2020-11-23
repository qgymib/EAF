#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "eaf/eaf.h"
#include "eaf/powerpack/time.h"
#include "eaf/powerpack/timer.h"
#include "eaf/powerpack/log.h"
#include "watchdog.h"

#define WATCHDOG_TIMER_INTERVAL		100

#define MODULE						"watchdog"
#define LOG_WARN(fmt, ...)			EAF_LOG_WARN(MODULE, fmt, ##__VA_ARGS__)

static int _watchdog_on_cmp_begin(const eaf_map_node_t* key1, const eaf_map_node_t* key2, void* arg);
static int _watchdog_on_cmp_end(const eaf_map_node_t* key1, const eaf_map_node_t* key2, void* arg);
static int _watchdog_on_cmp_token(const eaf_map_node_t* key1, const eaf_map_node_t* key2, void* arg);

typedef struct eaf_watchdog_record
{
	eaf_map_node_t							n_stack;
	eaf_map_node_t							n_table;

	struct
	{
		eaf_clock_time_t					begin;		/**< The timestamp when heartbeat request is send */
		eaf_clock_time_t					end;		/**< The timestamp when heartbeat response should received */
		size_t								cerrc;		/**< Continuous error count */
		int									token;		/**< UUID */
		const eaf_watchdog_watch_list_t*	rec;		/**< Watch record */
	}data;
}eaf_watchdog_record_t;

typedef struct eaf_watchdog_ctx
{
	/**
	 * Heartbeat request is going to send
	 * Sort by following field:
	 * + data.start
	 * + data.rec.sid
	 */
	eaf_map_t								idle_stack;

	/**
	 * Heartbeat request was sent but not receive response yet.
	 * Sort by following field:
	 * + data.start
	 * + data.rec
	 */
	eaf_map_t								busy_stack;

	/**
	 * Heartbeat request was sent but not receive response yet.
	 * Sort by following field:
	 * + data.token
	 */
	eaf_map_t								busy_table;

	struct
	{
		eaf_watchdog_record_t*				records;
		size_t								size;
	}data;

	struct
	{
		eaf_watchdog_on_error_fn			fn;
		void*								arg;
	}cb;
}eaf_watchdog_ctx_t;

static eaf_watchdog_ctx_t g_watchdog_ctx = {
	EAF_MAP_INITIALIZER(_watchdog_on_cmp_begin, NULL),
	EAF_MAP_INITIALIZER(_watchdog_on_cmp_end, NULL),
	EAF_MAP_INITIALIZER(_watchdog_on_cmp_token, NULL),
	{ NULL, 0 }, { NULL, NULL },
};

static int _watchdog_on_cmp_begin(const eaf_map_node_t* key1, const eaf_map_node_t* key2, void* arg)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(arg);
	eaf_watchdog_record_t* rec_1 = EAF_CONTAINER_OF(key1, eaf_watchdog_record_t, n_stack);
	eaf_watchdog_record_t* rec_2 = EAF_CONTAINER_OF(key2, eaf_watchdog_record_t, n_stack);

	/* compare timestamp */
	int ret;
	if ((ret = eaf_time_diffclock(&rec_1->data.begin, &rec_2->data.begin, NULL)) != 0)
	{
		return ret;
	}

	/* compare sid */
	if (rec_1->data.rec->sid == rec_2->data.rec->sid)
	{
		return 0;
	}
	return rec_1->data.rec->sid < rec_2->data.rec->sid ? -1 : 1;
}

static int _watchdog_on_cmp_end(const eaf_map_node_t* key1, const eaf_map_node_t* key2, void* arg)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(arg);
	eaf_watchdog_record_t* rec_1 = EAF_CONTAINER_OF(key1, eaf_watchdog_record_t, n_stack);
	eaf_watchdog_record_t* rec_2 = EAF_CONTAINER_OF(key2, eaf_watchdog_record_t, n_stack);

	/* compare timestamp */
	int ret;
	if ((ret = eaf_time_diffclock(&rec_1->data.end, &rec_2->data.end, NULL)) != 0)
	{
		return ret;
	}

	/* compare rec */
	if (rec_1->data.rec == rec_2->data.rec)
	{
		return 0;
	}
	return rec_1->data.rec < rec_2->data.rec ? -1 : 1;
}

static int _watchdog_on_cmp_token(const eaf_map_node_t* key1, const eaf_map_node_t* key2, void* arg)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(arg);
	eaf_watchdog_record_t* rec_1 = EAF_CONTAINER_OF(key1, eaf_watchdog_record_t, n_table);
	eaf_watchdog_record_t* rec_2 = EAF_CONTAINER_OF(key2, eaf_watchdog_record_t, n_table);

	return rec_1->data.token - rec_2->data.token;
}

static void _watchdog_send_response_heartbeat(uint32_t to, eaf_msg_t* req, int32_t ret)
{
	eaf_msg_t* rsp = eaf_msg_create_rsp(req, sizeof(eaf_watchdog_heartbeat_rsp_t));
	assert(rsp != NULL);

	((eaf_watchdog_heartbeat_rsp_t*)eaf_msg_get_data(rsp, NULL))->ret = ret;
	eaf_send_rsp(EAF_WATCHDOG_ID, to, rsp);
	eaf_msg_dec_ref(rsp);
}

static void _watchdog_check_busy(const eaf_clock_time_t* current_timestamp)
{
	eaf_map_node_t* it = eaf_map_begin(&g_watchdog_ctx.busy_stack);
	for (; it != NULL; it = eaf_map_next(it))
	{
		eaf_watchdog_record_t* record = EAF_CONTAINER_OF(it, eaf_watchdog_record_t, n_stack);
		if (eaf_time_diffclock(current_timestamp, &record->data.end, NULL) < 0)
		{
			return;
		}

		eaf_clock_time_t cmp = record->data.end;
		eaf_time_addclock_msec(&cmp, record->data.rec->timeout * record->data.rec->jitter);
		if (eaf_time_diffclock(&record->data.end, &cmp, NULL) > 0)
		{
			g_watchdog_ctx.cb.fn(record->data.rec->sid, g_watchdog_ctx.cb.arg);
		}
	}
}

static eaf_watchdog_record_t* _watchdog_find_record_by_token(int token)
{
	eaf_watchdog_record_t tmp_key; tmp_key.data.token = token;
	eaf_map_node_t* it = eaf_map_find(&g_watchdog_ctx.busy_table, &tmp_key.n_table);
	return it != NULL ? EAF_CONTAINER_OF(it, eaf_watchdog_record_t, n_table) : NULL;
}

static void _watchdog_on_heartbeat_rsp(_In_ uint32_t from, _In_ uint32_t to, _Inout_ eaf_msg_t* msg)
{
	int ret; EAF_SUPPRESS_UNUSED_VARIABLE(ret, from, to);

	int token = eaf_msg_get_token(msg);
	eaf_watchdog_record_t* record = _watchdog_find_record_by_token(token);
	assert(record != NULL);

	/* Remove from busy table */
	eaf_map_erase(&g_watchdog_ctx.busy_table, &record->n_table);
	eaf_map_erase(&g_watchdog_ctx.busy_stack, &record->n_stack);

	/* Check whether timeout */
	ret = eaf_time_getclock(&record->data.begin);
	assert(ret == 0);

	if (eaf_time_diffclock(&record->data.begin, &record->data.end, NULL) > 0)
	{/* If current time is larger than end time, it means timeout */
		record->data.cerrc++;
	}
	else
	{/* Other wise clear error counter */
		record->data.cerrc = 0;
	}

	/* If there is too many error, report */
	if (record->data.cerrc > record->data.rec->jitter)
	{
		g_watchdog_ctx.cb.fn(record->data.rec->sid, g_watchdog_ctx.cb.arg);
	}

	/* Re-calculate start time */
	eaf_time_addclock_msec(&record->data.begin, record->data.rec->interval);

	/* Insert into idle table */
	ret = eaf_map_insert(&g_watchdog_ctx.idle_stack, &record->n_stack);
	assert(ret == 0);
}

static int _watchdog_send_request(eaf_watchdog_record_t* record)
{
	eaf_msg_t* req = eaf_msg_create_req(EAF_WATCHDOG_MSG_HEARTBEAT_REQ, sizeof(eaf_watchdog_heartbeat_req_t), _watchdog_on_heartbeat_rsp);
	assert(req != NULL);

	((eaf_watchdog_heartbeat_req_t*)eaf_msg_get_data(req, NULL))->threshold = record->data.end;
	eaf_msg_set_token(req, record->data.token);

	int ret = eaf_send_req(EAF_WATCHDOG_ID, record->data.rec->sid, req);
	eaf_msg_dec_ref(req);

	return ret;
}

static void _watchdog_check_idle(const eaf_clock_time_t* current_timestamp)
{
	int ret; EAF_SUPPRESS_UNUSED_VARIABLE(ret);

	eaf_map_node_t* it = eaf_map_begin(&g_watchdog_ctx.idle_stack);
	while (it != NULL)
	{
		eaf_watchdog_record_t* record = EAF_CONTAINER_OF(it, eaf_watchdog_record_t, n_stack);
		if (eaf_time_diffclock(current_timestamp, &record->data.begin, NULL) < 0)
		{
			return;
		}

		/* Remove record from idle_map */
		eaf_map_node_t* tmp = it;
		it = eaf_map_next(it);
		eaf_map_erase(&g_watchdog_ctx.idle_stack, tmp);

		/* Re-calculate end time */
		eaf_clock_time_t dif = { 0,0 };
		eaf_time_addclock_msec(&dif, record->data.rec->timeout);
		record->data.begin = *current_timestamp;
		eaf_time_addclock_ext(&record->data.end, &record->data.begin, &dif, EAF_TIME_IGNORE_OVERFLOW);

		/* If request send failed, no need to watch this service */
		if (_watchdog_send_request(record) < 0)
		{
			LOG_WARN("send heartbeat to sid(0x%08" PRIx32 ") failed", record->data.rec->sid);
			continue;
		}

		ret = eaf_map_insert(&g_watchdog_ctx.busy_stack, &record->n_stack);
		assert(ret == 0);
		ret = eaf_map_insert(&g_watchdog_ctx.busy_table, &record->n_table);
		assert(ret == 0);
	}
}

static void _watchdog_on_timer(uint32_t from, uint32_t to, struct eaf_msg* msg)
{
	int ret;
	EAF_SUPPRESS_UNUSED_VARIABLE(ret, from, to, msg);

	/* Get current time */
	eaf_clock_time_t current_timestamp;
	ret = eaf_time_getclock(&current_timestamp);
	assert(ret == 0);

	_watchdog_check_idle(&current_timestamp);
	_watchdog_check_busy(&current_timestamp);

	/* Require timer delay */
	EAF_TIMER_DELAY(ret, EAF_WATCHDOG_ID, _watchdog_on_timer, WATCHDOG_TIMER_INTERVAL);
}

static void _watchdog_on_init(void)
{
	int ret;
	EAF_TIMER_DELAY(ret, EAF_WATCHDOG_ID, _watchdog_on_timer, WATCHDOG_TIMER_INTERVAL);

	if (ret < 0)
	{
		eaf_exit(ret);
	}
}

static void _watchdog_on_exit(void)
{
	// do nothing
}

static void _watchdog_reset_callback(void)
{
	g_watchdog_ctx.cb.fn = NULL;
	g_watchdog_ctx.cb.arg = NULL;
}

static void _watchdog_free_records(void)
{
	free(g_watchdog_ctx.data.records);
	g_watchdog_ctx.data.records = NULL;
	g_watchdog_ctx.data.size = 0;
}

static void _watchdog_cleanup_idle_stack(void)
{
	eaf_map_node_t* node = eaf_map_begin(&g_watchdog_ctx.idle_stack);
	while (node != NULL)
	{
		eaf_map_node_t* tmp = node;
		node = eaf_map_next(node);
		eaf_map_erase(&g_watchdog_ctx.idle_stack, tmp);
	}
}

static void _watchdog_cleanup_busy_stack(void)
{
	eaf_map_node_t* node = eaf_map_begin(&g_watchdog_ctx.busy_stack);
	while (node != NULL)
	{
		eaf_map_node_t* tmp = node;
		node = eaf_map_next(node);
		eaf_map_erase(&g_watchdog_ctx.busy_stack, tmp);
	}
}

int eaf_watchdog_init(_In_ const eaf_watchdog_watch_list_t* table, _In_ size_t size,
	_In_ eaf_watchdog_on_error_fn fn, _Inout_opt_ void* arg)
{
	int i, ret;
	if (g_watchdog_ctx.cb.fn != NULL)
	{
		return eaf_errno_duplicate;
	}

	g_watchdog_ctx.data.records = calloc(size, sizeof(eaf_watchdog_record_t));
	if (g_watchdog_ctx.data.records == NULL)
	{
		return eaf_errno_memory;
	}
	g_watchdog_ctx.data.size = size;

	for (i = 0; i < (int)size; i++)
	{
		g_watchdog_ctx.data.records[i].data.rec = &table[i];
		g_watchdog_ctx.data.records[i].data.token = i;
		if (eaf_map_insert(&g_watchdog_ctx.idle_stack, &g_watchdog_ctx.data.records[i].n_stack) < 0)
		{
			ret = eaf_errno_duplicate;
			goto err_register;
		}
	}

	static const eaf_entrypoint_t entry = {
		0, NULL,
		_watchdog_on_init,
		_watchdog_on_exit,
	};

	g_watchdog_ctx.cb.fn = fn;
	g_watchdog_ctx.cb.arg = arg;
	if ((ret = eaf_register(EAF_WATCHDOG_ID, &entry)) != eaf_errno_success)
	{
		goto err_register;
	}

	return ret;

err_register:
	_watchdog_cleanup_idle_stack();
	_watchdog_free_records();
	_watchdog_reset_callback();
	return ret;
}

void eaf_watchdog_exit(void)
{
	if (g_watchdog_ctx.cb.fn == NULL)
	{
		return;
	}

	_watchdog_cleanup_idle_stack();
	_watchdog_cleanup_busy_stack();

	_watchdog_free_records();
	_watchdog_reset_callback();

	return;
}
