#include <stdlib.h>
#include <assert.h>
#include "time.h"
#include "timer.h"
#include "powerpack.h"

#define MODULE					"timer"
#define LOG_TRACE(fmt, ...)		EAF_LOG_TRACE(MODULE, fmt, ##__VA_ARGS__)

#define MALLOC(size)			malloc(size)
#define FREE(ptr)				free(ptr)

static int _timer_on_libuv_loop_init(void);
static void _timer_on_libuv_loop_exit(void);

static timer_uv_ctx_t g_timer_uv_ctx;
static timer_ctx_t	g_timer_ctx = {
	{ 0, 0, 0 },
	EAF_LIST_INITIALIZER,
	EAF_LIST_INITIALIZER,
	EAF_LIST_INITIALIZER,
	{
		EAF_LIST_NODE_INITIALIZER,
		EAF_HOOK_INITIALIZER,
		_timer_on_libuv_loop_init,
		_timer_on_libuv_loop_exit,
	}
};

static void _timer_send_delay_rsp(uint32_t to, eaf_msg_t* req, int32_t ret)
{
	eaf_msg_t* rsp = eaf_msg_create_rsp(req, sizeof(eaf_timer_delay_rsp_t));
	assert(rsp != NULL);

	((eaf_timer_delay_rsp_t*)eaf_msg_get_data(rsp, NULL))->ret = ret;

	eaf_send_rsp(EAF_TIMER_ID, to, rsp);
	eaf_msg_dec_ref(rsp);
}

static void _timer_on_timer_close(uv_handle_t* handle)
{
	uv_timer_t* timer = (uv_timer_t*)handle;
	timer_record_t* record = EAF_CONTAINER_OF(timer, timer_record_t, uv.timer);

	eaf_list_erase(&g_timer_ctx.dead_queue, &record->node);
	FREE(record);
}

static void _timer_on_active(uv_timer_t* handle)
{
	timer_record_t* record = EAF_CONTAINER_OF(handle, timer_record_t, uv.timer);
	_timer_send_delay_rsp(record->data.req.from, &record->data.req, eaf_errno_success);

	uv_mutex_lock(&g_timer_uv_ctx.glock);
	{
		eaf_list_erase(&g_timer_ctx.busy_queue, &record->node);
		eaf_list_push_back(&g_timer_ctx.dead_queue, &record->node);
	}
	uv_mutex_unlock(&g_timer_uv_ctx.glock);

	/* Send response and close */
	uv_close((uv_handle_t*)&record->uv.timer, _timer_on_timer_close);
}

static void _timer_handle_idle_record(timer_record_t* record)
{
	/* Initialize timer */
	if (uv_timer_init(eaf_uv_get(), &record->uv.timer) < 0)
	{
		goto err_init_timer;
	}

	/* Push to busy_queue */
	uv_mutex_lock(&g_timer_uv_ctx.glock);
	{
		eaf_list_push_back(&g_timer_ctx.busy_queue, &record->node);
	}
	uv_mutex_unlock(&g_timer_uv_ctx.glock);

	if (uv_timer_start(&record->uv.timer, _timer_on_active, record->data.msec, 0) < 0)
	{
		/* Operation rollback */
		goto err_start_timer;
	}

	return;

err_start_timer:
	/* Erase from busy_queue and push to deaed_queue */
	uv_mutex_lock(&g_timer_uv_ctx.glock);
	{
		eaf_list_erase(&g_timer_ctx.busy_queue, &record->node);
		eaf_list_push_back(&g_timer_ctx.dead_queue, &record->node);
	}
	uv_mutex_unlock(&g_timer_uv_ctx.glock);
	/* Send response and close */
	_timer_send_delay_rsp(record->data.req.from, &record->data.req, eaf_errno_unknown);
	uv_close((uv_handle_t*)&record->uv.timer, _timer_on_timer_close);
	return;

err_init_timer:
	_timer_send_delay_rsp(record->data.req.from, &record->data.req, eaf_errno_unknown);
	FREE(record);
	return;
}

static void _timer_close_all_timer(void)
{
	eaf_list_node_t* it;
	uv_mutex_lock(&g_timer_uv_ctx.glock);
	while ((it = eaf_list_pop_front(&g_timer_ctx.busy_queue)) != NULL)
	{
		eaf_list_push_back(&g_timer_ctx.dead_queue, it);
		uv_mutex_unlock(&g_timer_uv_ctx.glock);
		{
			timer_record_t* record = EAF_CONTAINER_OF(it, timer_record_t, node);
			uv_close((uv_handle_t*)&record->uv.timer, _timer_on_timer_close);
		}
		uv_mutex_lock(&g_timer_uv_ctx.glock);
	}
	while ((it = eaf_list_pop_front(&g_timer_ctx.idle_queue)) != NULL)
	{
		uv_mutex_unlock(&g_timer_uv_ctx.glock);
		{
			timer_record_t* record = EAF_CONTAINER_OF(it, timer_record_t, node);
			FREE(record);
		}
		uv_mutex_lock(&g_timer_uv_ctx.glock);
	}
	uv_mutex_unlock(&g_timer_uv_ctx.glock);
}

static void _timer_on_uv_notify(uv_async_t* handle)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(handle);

	if (!g_timer_ctx.mask.looping)
	{
		_timer_close_all_timer();
		return;
	}

	eaf_list_node_t* it;
	uv_mutex_lock(&g_timer_uv_ctx.glock);
	while ((it = eaf_list_pop_front(&g_timer_ctx.idle_queue)) != NULL)
	{
		uv_mutex_unlock(&g_timer_uv_ctx.glock);
		{
			_timer_handle_idle_record(EAF_CONTAINER_OF(it, timer_record_t, node));
		}
		uv_mutex_lock(&g_timer_uv_ctx.glock);
	}
	uv_mutex_unlock(&g_timer_uv_ctx.glock);
}

static int _timer_on_libuv_loop_init(void)
{
	if (uv_async_init(eaf_uv_get(), &g_timer_uv_ctx.gnotifier, _timer_on_uv_notify) < 0)
	{
		return -1;
	}
	g_timer_ctx.mask.inited_notifier = 1;
	return 0;
}

static void _timer_on_libuv_loop_exit(void)
{
	assert(eaf_list_size(&g_timer_ctx.busy_queue) == 0);
	assert(eaf_list_size(&g_timer_ctx.dead_queue) == 0);
	assert(eaf_list_size(&g_timer_ctx.idle_queue) == 0);

	/* Close notifier */
	uv_close((uv_handle_t*)&g_timer_uv_ctx.gnotifier, NULL);
}

static void _timer_on_req_delay(uint32_t from, uint32_t to, eaf_msg_t* msg)
{
	int ret;
	EAF_SUPPRESS_UNUSED_VARIABLE(to);

	if (!g_timer_ctx.mask.looping)
	{
		_timer_send_delay_rsp(from, msg, eaf_errno_state);
		return;
	}

	timer_record_t* record = MALLOC(sizeof(*record));
	assert(record != NULL);

	record->data.req = *msg;
	record->data.msec = ((eaf_timer_delay_req_t*)eaf_msg_get_data(msg, NULL))->msec;

	/* store in queue */
	uv_mutex_lock(&g_timer_uv_ctx.glock);
	{
		eaf_list_push_back(&g_timer_ctx.idle_queue, &record->node);
		if ((ret = uv_async_send(&g_timer_uv_ctx.gnotifier)) < 0)
		{
			eaf_list_erase(&g_timer_ctx.idle_queue, &record->node);
		}
	}
	uv_mutex_unlock(&g_timer_uv_ctx.glock);

	if (ret >= 0)
	{
		return;
	}

	_timer_send_delay_rsp(from, msg, eaf_errno_unknown);
	FREE(record);

	return;
}

static void _timer_on_init(void)
{
	g_timer_ctx.mask.looping = 1;
}

static void _timer_on_exit(void)
{
	g_timer_ctx.mask.looping = 0;
	uv_async_send(&g_timer_uv_ctx.gnotifier);

	/* Here we need to wait until all timer is closed */
	while ((
		eaf_list_size(&g_timer_ctx.busy_queue) +
		eaf_list_size(&g_timer_ctx.idle_queue) +
		eaf_list_size(&g_timer_ctx.dead_queue)) != 0)
	{
		eaf_thread_sleep(10);
	}
}

int eaf_timer_init(void)
{
	int ret;
	if (g_timer_ctx.mask.inited)
	{
		return eaf_errno_duplicate;
	}

	if (uv_mutex_init(&g_timer_uv_ctx.glock) < 0)
	{
		return eaf_errno_unknown;
	}

	if ((ret = eaf_powerpack_hook_register(&g_timer_ctx.hook, sizeof(g_timer_ctx.hook))) < 0)
	{
		goto err_hook_register;
	}

	static eaf_message_table_t msg_table[] = {
		{ EAF_TIMER_MSG_DELAY_REQ, _timer_on_req_delay },
	};

	static eaf_entrypoint_t entry = {
		EAF_ARRAY_SIZE(msg_table), msg_table,
		_timer_on_init, _timer_on_exit,
	};

	if ((ret = eaf_register(EAF_TIMER_ID, &entry)) < 0)
	{
		goto err_register;
	}

	g_timer_ctx.mask.inited = 1;
	return eaf_errno_success;

err_register:
	eaf_powerpack_hook_unregister(&g_timer_ctx.hook);
err_hook_register:
	uv_mutex_destroy(&g_timer_uv_ctx.glock);
	return ret;
}

void eaf_timer_exit(void)
{
	if (!g_timer_ctx.mask.inited)
	{
		return;
	}

	uv_mutex_destroy(&g_timer_uv_ctx.glock);
	g_timer_ctx.mask.inited = 0;
}
