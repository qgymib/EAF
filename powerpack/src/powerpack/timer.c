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

	struct
	{
		unsigned				dead : 1;
	}mask;
}timer_record_t;

typedef struct timer_ctx
{
	eaf_lock_t*					objlock;		/**< global lock */
	eaf_list_t					delay_queue;	/**< Delay queue */
	eaf_list_t					recycle_queue;	/**< Recycle queue*/
}timer_ctx_t;

static timer_ctx_t	g_timer_ctx = {
	NULL,
	EAF_LIST_INITIALIZER,
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

	/* remove recycle record */
	eaf_lock_enter(g_timer_ctx.objlock);
	{
		eaf_list_erase(&g_timer_ctx.recycle_queue, &record->node);
	}
	eaf_lock_leave(g_timer_ctx.objlock);

	free(record);
}

static void _timer_on_uv_timer_active(uv_timer_t* handle)
{
	timer_record_t* record = EAF_CONTAINER_OF(handle, timer_record_t, data.uv_timer);
	uv_timer_stop(handle);

	int ret = 0;
	eaf_lock_enter(g_timer_ctx.objlock);
	do 
	{
		/* avoid multi-thread destroy */
		if (record->mask.dead)
		{
			ret = -1;
			break;
		}
		record->mask.dead = 1;

		eaf_list_erase(&g_timer_ctx.delay_queue, &record->node);
		eaf_list_push_back(&g_timer_ctx.recycle_queue, &record->node);
	} EAF_MSVC_WARNING_GUARD(4127, while(0));
	eaf_lock_leave(g_timer_ctx.objlock);

	if (ret < 0)
	{
		return;
	}

	_timer_send_delay_rsp(record->data.from, record->data.req, eaf_errno_success);
	uv_close((uv_handle_t*)&record->data.uv_timer, _timer_on_close_uv_timer);
}

static void _timer_on_req_delay(_In_ uint32_t from, _In_ uint32_t to, _Inout_ struct eaf_msg* msg)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(to);
	uint32_t msec = ((eaf_timer_delay_req_t*)eaf_msg_get_data(msg, NULL))->msec;

	timer_record_t* record = malloc(sizeof(*record));
	assert(record != NULL);

	/* initialize */
	if (uv_timer_init(eaf_uv_get(), &record->data.uv_timer) < 0)
	{
		goto err_init_timer;
	}
	record->mask.dead = 0;
	record->data.from = from;
	record->data.req = msg;
	eaf_msg_add_ref(msg);

	/* save record */
	eaf_list_push_back(&g_timer_ctx.delay_queue, &record->node);

	/* start timer */
	if (uv_timer_start(&record->data.uv_timer, _timer_on_uv_timer_active, msec, 0) < 0)
	{
		goto err_start_timer;
	}
	eaf_uv_mod();

	return;

err_start_timer:
	/* remove record */
	_timer_send_delay_rsp(from, msg, eaf_errno_unknown);

	eaf_lock_enter(g_timer_ctx.objlock);
	{
		eaf_list_erase(&g_timer_ctx.delay_queue, &record->node);
		eaf_list_push_back(&g_timer_ctx.recycle_queue, &record->node);
		record->mask.dead = 1;
	}
	eaf_lock_leave(g_timer_ctx.objlock);

	uv_close((uv_handle_t*)&record->data.uv_timer, _timer_on_close_uv_timer);
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

	/* close pending request */
	eaf_lock_enter(g_timer_ctx.objlock);
	while ((it = eaf_list_pop_front(&g_timer_ctx.delay_queue)) != NULL)
	{
		timer_record_t* record = EAF_CONTAINER_OF(it, timer_record_t, node);

		eaf_list_push_back(&g_timer_ctx.recycle_queue, it);
		record->mask.dead = 1;

		_timer_send_delay_rsp(record->data.from, record->data.req, eaf_errno_state);
		uv_close((uv_handle_t*)&record->data.uv_timer, _timer_on_close_uv_timer);
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

	/* wait for all record closed */
	eaf_lock_enter(g_timer_ctx.objlock);
	while (eaf_list_size(&g_timer_ctx.recycle_queue) != 0)
	{
		eaf_lock_leave(g_timer_ctx.objlock);
		{
			eaf_thread_sleep(10);
		}
		eaf_lock_enter(g_timer_ctx.objlock);
	}
	eaf_lock_leave(g_timer_ctx.objlock);

	/* cleanup resource */
	eaf_lock_destroy(g_timer_ctx.objlock);
	g_timer_ctx.objlock = NULL;
}
