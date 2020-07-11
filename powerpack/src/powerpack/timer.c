#include <stdlib.h>
#include <assert.h>
#include "eaf/eaf.h"
#include "time.h"
#include "timer.h"
#include "powerpack.h"

typedef struct timer_record
{
	eaf_list_node_t				node;			/**< List node */

	struct
	{
		uv_timer_t				uv_timer;		/**< timer */
		eaf_msg_t*				req;			/**< original request */
		uint32_t				from;
	}data;
}timer_record_t;

typedef struct timer_ctx
{
	eaf_lock_t*					objlock;		/**< global lock */
	eaf_list_t					delay_queue;	/**< Delay queue */
}timer_ctx_t;

static timer_ctx_t	g_timer_ctx = {
	NULL,
	EAF_LIST_INITIALIZER,
};

static void _timer_send_delay_rsp(uint32_t to, eaf_msg_t* req, int32_t ret)
{
	eaf_msg_t* rsp = eaf_msg_create_rsp(req, sizeof(eaf_timer_delay_rsp_t));
	assert(rsp != NULL);

	((eaf_timer_delay_rsp_t*)eaf_msg_get_data(rsp, NULL))->ret = ret;

	eaf_send_rsp(EAF_TIMER_ID, to, rsp);
	eaf_msg_dec_ref(rsp);
}

static void _timer_on_close_uv_timer(uv_handle_t* handle)
{
	timer_record_t* record = EAF_CONTAINER_OF(handle, timer_record_t, data.uv_timer);
	eaf_msg_dec_ref(record->data.req);
	free(record);
}

static void _timer_close_record(timer_record_t* record, int erase)
{
	if (erase)
	{
		eaf_lock_enter(g_timer_ctx.objlock);
		{
			eaf_list_erase(&g_timer_ctx.delay_queue, &record->node);
		}
		eaf_lock_leave(g_timer_ctx.objlock);
	}

	uv_close((uv_handle_t*)&record->data.uv_timer, _timer_on_close_uv_timer);
}

static void _timer_on_uv_timer_delay(uv_timer_t* handle)
{
	uv_timer_stop(handle);

	timer_record_t* record = EAF_CONTAINER_OF(handle, timer_record_t, data.uv_timer);

	_timer_send_delay_rsp(record->data.from, record->data.req, eaf_errno_success);
	_timer_close_record(record, 1);
}

static void _timer_on_req_delay(_In_ uint32_t from, _In_ uint32_t to, _Inout_ struct eaf_msg* msg)
{
	(void)to;
	uint32_t msec = ((eaf_timer_delay_req_t*)eaf_msg_get_data(msg, NULL))->msec;

	timer_record_t* record = malloc(sizeof(*record));
	assert(record != NULL);

	/* initialize */
	if (uv_timer_init(eaf_uv_get(), &record->data.uv_timer) < 0)
	{
		goto err_init_timer;
	}
	record->data.from = from;
	record->data.req = msg;
	eaf_msg_add_ref(msg);

	/* save record */
	eaf_list_push_back(&g_timer_ctx.delay_queue, &record->node);

	/* start timer */
	if (uv_timer_start(&record->data.uv_timer, _timer_on_uv_timer_delay, msec, 0) < 0)
	{
		goto err_start_timer;
	}
	eaf_uv_mod();

	return;

err_start_timer:
	/* remove record */
	_timer_send_delay_rsp(from, msg, eaf_errno_unknown);
	_timer_close_record(record, 1);
	return;

err_init_timer:
	_timer_send_delay_rsp(from, msg, eaf_errno_unknown);
	free(record);
	return;
}

static int _timer_on_init(void)
{
	return 0;
}

static void _timer_on_exit(void)
{
	eaf_list_node_t* it;

	eaf_lock_enter(g_timer_ctx.objlock);
	while ((it = eaf_list_pop_front(&g_timer_ctx.delay_queue)) != NULL)
	{
		timer_record_t* record = EAF_CONTAINER_OF(it, timer_record_t, node);
		_timer_close_record(record, 0);
	}
	eaf_lock_leave(g_timer_ctx.objlock);
}

int eaf_timer_init(void)
{
	if (g_timer_ctx.objlock != NULL)
	{
		return eaf_errno_duplicate;
	}

	if ((g_timer_ctx.objlock = eaf_lock_create(eaf_lock_attr_normal)) == NULL)
	{
		return eaf_errno_memory;
	}

	static eaf_message_table_t msg_table[] = {
		{ EAF_TIMER_MSG_DELAY_REQ, _timer_on_req_delay },
	};

	static eaf_entrypoint_t entry = {
		EAF_ARRAY_SIZE(msg_table), msg_table,
		_timer_on_init, _timer_on_exit,
	};

	return eaf_register(EAF_TIMER_ID, &entry);
}

void eaf_timer_exit(void)
{
	if (g_timer_ctx.objlock == NULL)
	{
		return;
	}

	eaf_lock_destroy(g_timer_ctx.objlock);
	g_timer_ctx.objlock = NULL;
}
