#include <stdlib.h>
#include "eaf/eaf.h"
#include "timer.h"
#include "powerpack.h"

/*
* Before Visual Studio 2015, there is a bug that a `do { } while (0)` will triger C4127 warning
* https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4127
*/
#if defined(_MSC_VER) && _MSC_VER < 1900
#	pragma warning(disable : 4127)
#endif

#define USEC_IN_SEC		(1 * 1000 * 1000)

typedef struct powerpack_timer_record
{
	union
	{
		eaf_map_node_t			table;			/** table node */
		eaf_list_node_t			queue;			/** list node */
	}node;

	struct
	{
		uv_timer_t				uv_timer;		/** timer */
		uint32_t				service_id;		/** service id */
	}data;
}powerpack_timer_record_t;

typedef struct powerpack_timer_ctx
{
	eaf_lock_t*					objlock;		/** global lock */
	eaf_map_t					table;			/** record table */
	eaf_list_t					gc_queue;		/** wait for free */
}powerpack_timer_ctx_t;

static powerpack_timer_ctx_t*	g_powerpack_timer_ctx = NULL;

static int _powerpack_timer_on_cmp(const eaf_map_node_t* key1, const eaf_map_node_t* key2, void* arg)
{
	(void)arg;
	const powerpack_timer_record_t* rec_1 = EAF_CONTAINER_OF(key1, powerpack_timer_record_t, node.table);
	const powerpack_timer_record_t* rec_2 = EAF_CONTAINER_OF(key2, powerpack_timer_record_t, node.table);

	if (rec_1->data.service_id == rec_2->data.service_id)
	{
		return 0;
	}
	return rec_1->data.service_id < rec_2->data.service_id ? -1 : 1;
}

int eaf_powerpack_timer_init(void)
{
	if (g_powerpack_timer_ctx != NULL)
	{
		return -1;
	}

	if ((g_powerpack_timer_ctx = malloc(sizeof(powerpack_timer_ctx_t))) == NULL)
	{
		return -1;
	}

	if ((g_powerpack_timer_ctx->objlock = eaf_lock_create(eaf_lock_attr_normal)) == NULL)
	{
		eaf_powerpack_timer_exit();
		return -1;
	}

	eaf_list_init(&g_powerpack_timer_ctx->gc_queue);
	eaf_map_init(&g_powerpack_timer_ctx->table, _powerpack_timer_on_cmp, NULL);

	return 0;
}

void eaf_powerpack_timer_exit(void)
{
	if (g_powerpack_timer_ctx == NULL)
	{
		return;
	}

	eaf_map_node_t* it = eaf_map_begin(&g_powerpack_timer_ctx->table);
	while (it != NULL)
	{
		eaf_map_node_t* tmp = it;
		it = eaf_map_next(&g_powerpack_timer_ctx->table, it);
		eaf_map_erase(&g_powerpack_timer_ctx->table, tmp);

		powerpack_timer_record_t* rec = EAF_CONTAINER_OF(it, powerpack_timer_record_t, node.table);
		free(rec);
	}

	if (g_powerpack_timer_ctx->objlock != NULL)
	{
		eaf_lock_destroy(g_powerpack_timer_ctx->objlock);
		g_powerpack_timer_ctx->objlock = NULL;
	}

	free(g_powerpack_timer_ctx);
	g_powerpack_timer_ctx = NULL;
}

static void _powerpack_timer_erase_lock(powerpack_timer_record_t* record)
{
	eaf_lock_enter(g_powerpack_timer_ctx->objlock);
	do
	{
		eaf_map_erase(&g_powerpack_timer_ctx->table, &record->node.table);
	} while (0);
	eaf_lock_leave(g_powerpack_timer_ctx->objlock);
}

static void _powerpack_timer_on_close(uv_handle_t* handle)
{
	powerpack_timer_record_t* rec = EAF_CONTAINER_OF(handle, powerpack_timer_record_t, data.uv_timer);

	eaf_lock_enter(g_powerpack_timer_ctx->objlock);
	do 
	{
		eaf_list_erase(&g_powerpack_timer_ctx->gc_queue, &rec->node.queue);
	} while (0);
	eaf_lock_leave(g_powerpack_timer_ctx->objlock);

	free(rec);
}

static void _powerpack_timer_on_timer(uv_timer_t* handle)
{
	powerpack_timer_record_t* rec = EAF_CONTAINER_OF(handle, powerpack_timer_record_t, data.uv_timer);
	uv_timer_stop(handle);

	eaf_resume(rec->data.service_id);

	/* append to gc_queue */
	eaf_lock_enter(g_powerpack_timer_ctx->objlock);
	do
	{
		eaf_map_erase(&g_powerpack_timer_ctx->table, &rec->node.table);
		eaf_list_push_back(&g_powerpack_timer_ctx->gc_queue, &rec->node.queue);
	} while (0);
	eaf_lock_leave(g_powerpack_timer_ctx->objlock);

	uv_close((uv_handle_t*)handle, _powerpack_timer_on_close);
}

void eaf_powerpack_sleep_commit(_Inout_ eaf_service_local_t* local, _Inout_opt_ void* arg)
{
	(void)arg;
	powerpack_timer_record_t* record = NULL;
	if (g_powerpack_timer_ctx == NULL)
	{
		goto err;
	}

	unsigned sleep_timeout = (unsigned)(uintptr_t)arg;
	if ((record = malloc(sizeof(powerpack_timer_record_t))) == NULL)
	{
		goto err;
	}
	record->data.service_id = local->id;

	/* initialize timer */
	if (uv_timer_init(eaf_uv_get(), &record->data.uv_timer) < 0)
	{
		goto err;
	}

	int res;
	eaf_lock_enter(g_powerpack_timer_ctx->objlock);
	do 
	{
		res = eaf_map_insert(&g_powerpack_timer_ctx->table, &record->node.table);
	} while (0);
	eaf_lock_leave(g_powerpack_timer_ctx->objlock);

	if (res < 0)
	{
		goto err;
	}

	/* start timer */
	if (uv_timer_start(&record->data.uv_timer, _powerpack_timer_on_timer, sleep_timeout, 0) < 0)
	{
		goto err_erase_record;
	}

	eaf_uv_mod();
	return;

err_erase_record:
	_powerpack_timer_erase_lock(record);

err:
	if (record != NULL)
	{
		free(record);
	}
	eaf_resume(local->id);
	return;
}
