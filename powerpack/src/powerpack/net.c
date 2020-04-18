#include <stdlib.h>
#include "EAF/eaf.h"
#include "powerpack.h"
#include "net.h"

/*
* Before Visual Studio 2015, there is a bug that a `do { } while (0)` will triger C4127 warning
* https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4127
*/
#if defined(_MSC_VER) && _MSC_VER <= 1900
#	pragma warning(disable : 4127)
#endif

typedef struct powerpack_poll_record
{
	union
	{
		eaf_map_node_t			table;
		eaf_list_node_t			queue;
	}node;

	struct
	{
		uv_poll_t				uv_poll;
		uv_timer_t				uv_timer;
		unsigned				refcnt;
	}uv;

	struct
	{
		eaf_service_local_t*	local;
		uint32_t				id;
		eaf_socket_t			sock;
		unsigned				evts;
		unsigned				timeout;
	}data;
}powerpack_poll_record_t;

typedef struct powerpack_net_ctx
{
	eaf_lock_t*					objlock;	/** global lock */
	eaf_map_t					table_poll;	/** poll record table */
	eaf_list_t					queue_gc;	/** gc queue */
}powerpack_net_ctx_t;

static powerpack_net_ctx_t* g_powerpack_net_ctx = NULL;

static int _powerpack_net_on_cmp_record(const eaf_map_node_t* key1, const eaf_map_node_t* key2, void* arg)
{
	(void)arg;
	powerpack_poll_record_t* rec_1 = EAF_CONTAINER_OF(key1, powerpack_poll_record_t, node.table);
	powerpack_poll_record_t* rec_2 = EAF_CONTAINER_OF(key2, powerpack_poll_record_t, node.table);

	if (rec_1->data.id == rec_2->data.id)
	{
		return 0;
	}
	return rec_1->data.id < rec_2->data.id ? -1 : 1;
}

static void _powerpack_net_append_to_gc_lock(powerpack_poll_record_t* rec)
{
	eaf_lock_enter(g_powerpack_net_ctx->objlock);
	do
	{
		eaf_list_push_back(&g_powerpack_net_ctx->queue_gc, &rec->node.queue);
	} while (0);
	eaf_lock_leave(g_powerpack_net_ctx->objlock);
}

static void _powerpack_net_on_poll_close(uv_handle_t* handle)
{
	powerpack_poll_record_t* rec = EAF_CONTAINER_OF(handle, powerpack_poll_record_t, uv.uv_poll);

	/* wait for both uv_pool and uv_timer close */
	if ((--rec->uv.refcnt) != 0)
	{
		return;
	}

	eaf_lock_enter(g_powerpack_net_ctx->objlock);
	do
	{
		eaf_list_erase(&g_powerpack_net_ctx->queue_gc, &rec->node.queue);
	} while (0);
	eaf_lock_leave(g_powerpack_net_ctx->objlock);

	free(rec);
}

static void _powerpack_net_move_to_gc_queue_lock(powerpack_poll_record_t* rec)
{
	eaf_lock_enter(g_powerpack_net_ctx->objlock);
	do
	{
		eaf_map_erase(&g_powerpack_net_ctx->table_poll, &rec->node.table);
		eaf_list_push_back(&g_powerpack_net_ctx->queue_gc, &rec->node.queue);
	} while (0);
	eaf_lock_leave(g_powerpack_net_ctx->objlock);
}

static void _powerpack_net_cleanup(powerpack_poll_record_t* rec)
{
	/* append to gc queue */
	_powerpack_net_move_to_gc_queue_lock(rec);

	/* stop uv */
	uv_poll_stop(&rec->uv.uv_poll);
	uv_timer_stop(&rec->uv.uv_timer);

	/* close */
	uv_close((uv_handle_t*)&rec->uv.uv_poll, _powerpack_net_on_poll_close);
	uv_close((uv_handle_t*)&rec->uv.uv_timer, _powerpack_net_on_poll_close);
}

static void _powerpack_net_on_uv_poll(uv_poll_t* handle, int status, int events)
{
	powerpack_poll_record_t* rec = EAF_CONTAINER_OF(handle, powerpack_poll_record_t, uv.uv_poll);

	int ret = (status < 0) ? eaf_socket_event_error : 0;
	ret |= (events & UV_READABLE) ? eaf_socket_event_in : 0;
	ret |= (events & UV_WRITABLE) ? eaf_socket_event_out : 0;

	/* resume */
	rec->data.local->unsafe.v_int = ret;
	eaf_resume(rec->data.id);

	_powerpack_net_cleanup(rec);
}

static void _powerpack_net_on_timer(uv_timer_t* handle)
{
	powerpack_poll_record_t* rec = EAF_CONTAINER_OF(handle, powerpack_poll_record_t, uv.uv_timer);

	rec->data.local->unsafe.v_int = eaf_socket_event_timeout;
	eaf_resume(rec->data.id);

	_powerpack_net_cleanup(rec);
}

int eaf_powerpack_net_init(void)
{
	if (g_powerpack_net_ctx != NULL)
	{
		return eaf_errno_duplicate;
	}

	if ((g_powerpack_net_ctx = malloc(sizeof(powerpack_net_ctx_t))) == NULL)
	{
		return eaf_errno_memory;
	}

	if ((g_powerpack_net_ctx->objlock = eaf_lock_create(eaf_lock_attr_normal)) == NULL)
	{
		goto err_lock;
	}
	eaf_map_init(&g_powerpack_net_ctx->table_poll, _powerpack_net_on_cmp_record, NULL);
	eaf_list_init(&g_powerpack_net_ctx->queue_gc);

	return eaf_errno_success;

err_lock:
	free(g_powerpack_net_ctx);
	g_powerpack_net_ctx = NULL;
	return eaf_errno_memory;
}

void eaf_powerpack_net_exit(void)
{
	if (g_powerpack_net_ctx == NULL)
	{
		return;
	}

	eaf_lock_destroy(g_powerpack_net_ctx->objlock);
	g_powerpack_net_ctx->objlock = NULL;

	free(g_powerpack_net_ctx);
	g_powerpack_net_ctx = NULL;
}

int eaf_powerpack_net_socket_wait_setup(uint32_t id, eaf_socket_t sock, unsigned evts, unsigned timeout)
{
	powerpack_poll_record_t* rec = malloc(sizeof(powerpack_poll_record_t));
	if (rec == NULL)
	{
		return eaf_errno_memory;
	}
	rec->data.local = NULL;
	rec->data.id = id;
	rec->data.sock = sock;
	rec->data.evts = evts;
	rec->data.timeout = timeout;
	rec->uv.refcnt = 2;

	if (uv_timer_init(powerpack_get_uv(), &rec->uv.uv_timer) < 0)
	{
		free(rec);
		return eaf_errno_unknown;
	}
	if (uv_poll_init_socket(powerpack_get_uv(), &rec->uv.uv_poll, sock) < 0)
	{
		rec->uv.refcnt = 1;	/* poll not success, so refnct should be 1 */
		goto err_uv_poll;
	}

	int res;
	eaf_lock_enter(g_powerpack_net_ctx->objlock);
	do
	{
		res = eaf_map_insert(&g_powerpack_net_ctx->table_poll, &rec->node.table);
	} while (0);
	eaf_lock_leave(g_powerpack_net_ctx->objlock);

	if (res < 0)
	{
		goto err_table;
	}

	return eaf_errno_success;

err_uv_poll:
	_powerpack_net_append_to_gc_lock(rec);
	uv_close((uv_handle_t*)&rec->uv.uv_poll, _powerpack_net_on_poll_close);
	return eaf_errno_unknown;

err_table:
	_powerpack_net_append_to_gc_lock(rec);
	uv_close((uv_handle_t*)&rec->uv.uv_poll, _powerpack_net_on_poll_close);
	uv_close((uv_handle_t*)&rec->uv.uv_timer, _powerpack_net_on_poll_close);
	return eaf_errno_duplicate;
}

void eaf_powerpack_net_socket_wait_commit(eaf_service_local_t* local, void* arg)
{
	(void)arg;
	powerpack_poll_record_t tmp_key;
	tmp_key.data.id = local->id;

	powerpack_poll_record_t* rec;
	eaf_lock_enter(g_powerpack_net_ctx->objlock);
	do 
	{
		eaf_map_node_t* it = eaf_map_find(&g_powerpack_net_ctx->table_poll, &tmp_key.node.table);
		rec = it != NULL ? EAF_CONTAINER_OF(it, powerpack_poll_record_t, node.table) : NULL;
	} while (0);
	eaf_lock_leave(g_powerpack_net_ctx->objlock);

	if (rec == NULL)
	{
		eaf_resume(local->id);
		return;
	}
	rec->data.local = local;

	int uv_events = 0;
	uv_events |= (rec->data.evts & eaf_socket_event_in) ? UV_READABLE : 0;
	uv_events |= (rec->data.evts & eaf_socket_event_out) ? UV_WRITABLE : 0;

	if (uv_poll_start(&rec->uv.uv_poll, uv_events, _powerpack_net_on_uv_poll) < 0)
	{
		goto cleanup;
	}
	if (uv_timer_start(&rec->uv.uv_timer, _powerpack_net_on_timer, rec->data.timeout, 0) < 0)
	{
		goto cleanup;
	}

	return;

cleanup:
	_powerpack_net_cleanup(rec);
	eaf_resume(local->id);
}
