#include <assert.h>
#include <stdlib.h>
#include "powerpack.h"
#include "message.h"

/*
* Before Visual Studio 2015, there is a bug that a `do { } while (0)` will triger C4127 warning
* https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4127
*/
#if defined(_MSC_VER) && _MSC_VER < 1900
#	pragma warning(disable : 4127)
#endif

typedef struct powerpack_message_record
{
	eaf_map_node_t				node;		/**< table node */

	struct
	{
		uint64_t				uuid;		/**< uuid */
		eaf_msg_t*				req;		/**< raw request */
		eaf_service_local_t*	local;		/**< service local storage */
		uint32_t				from;		/**< raw sender */
		unsigned long			defcnt;		/**< the number of defcnt */
	}data;
}powerpack_message_record_t;

typedef struct powerpack_message_ctx
{
	eaf_lock_t*					objlock;	/**< global lock */
	eaf_map_t					table;		/**< global table */
}powerpack_message_ctx_t;

static powerpack_message_ctx_t* g_powerpack_message_ctx = NULL;

/**
 * Decrease reference count.
 * @param[in] msg	Message
 * @param[in] cnt	Count
 */
static void _powerpack_message_decref(eaf_msg_t* msg, unsigned cnt)
{
	size_t i;
	for (i = 0; i < cnt; i++)
	{
		/**
		 * coverity false positive:
		 * we decrease reference by user request, so it is user's
		 * responsibility to make sure the count is right.
		 */
		/* coverity[double_free : FALSE] */
		eaf_msg_dec_ref(msg);
	}
}

static void _powerpack_message_on_rsp_proxy(_In_ uint32_t from, _In_ uint32_t to, _Inout_ struct eaf_msg* rsp)
{
	(void)from; (void)to;
	powerpack_message_record_t tmp_key;
	tmp_key.data.uuid = rsp->info.constant.uuid;

	powerpack_message_record_t* rec = NULL;
	eaf_lock_enter(g_powerpack_message_ctx->objlock);
	do 
	{
		eaf_map_node_t* it = eaf_map_find(&g_powerpack_message_ctx->table, &tmp_key.node);
		if (it == NULL)
		{
			break;
		}
		eaf_map_erase(&g_powerpack_message_ctx->table, it);
		rec = EAF_CONTAINER_OF(it, powerpack_message_record_t, node);
	} while (0);
	eaf_lock_leave(g_powerpack_message_ctx->objlock);

	/* must found */
	assert(rec != NULL);

	/* resume */
	eaf_msg_add_ref(rsp);
	rec->data.local->unsafe[0].v_ptr = rsp;
	eaf_resume(rec->data.from);

	_powerpack_message_decref(rec->data.req, rec->data.defcnt);
	free(rec);
}

static int _powerpack_message_on_cmp_record(const eaf_map_node_t* key1, const eaf_map_node_t* key2, void* arg)
{
	(void)arg;
	powerpack_message_record_t* rec_1 = EAF_CONTAINER_OF(key1, powerpack_message_record_t, node);
	powerpack_message_record_t* rec_2 = EAF_CONTAINER_OF(key2, powerpack_message_record_t, node);

	if (rec_1->data.uuid == rec_2->data.uuid)
	{
		return 0;
	}
	return rec_1->data.uuid < rec_2->data.uuid ? -1 : 1;
}

int eaf_powerpack_message_init(void)
{
	if (g_powerpack_message_ctx != NULL)
	{
		return -1;
	}

	if ((g_powerpack_message_ctx = calloc(1, sizeof(*g_powerpack_message_ctx))) == NULL)
	{
		return -1;
	}
	eaf_map_init(&g_powerpack_message_ctx->table, _powerpack_message_on_cmp_record, NULL);

	if ((g_powerpack_message_ctx->objlock = eaf_lock_create(eaf_lock_attr_normal)) == NULL)
	{
		eaf_powerpack_message_exit();
		return -1;
	}

	return 0;
}

void eaf_powerpack_message_exit(void)
{
	if (g_powerpack_message_ctx == NULL)
	{
		return;
	}

	eaf_map_node_t* it = eaf_map_begin(&g_powerpack_message_ctx->table);
	while (it)
	{
		eaf_map_node_t* tmp = it;
		it = eaf_map_next(it);
		eaf_map_erase(&g_powerpack_message_ctx->table, tmp);

		powerpack_message_record_t* rec = EAF_CONTAINER_OF(tmp, powerpack_message_record_t, node);
		free(rec);
	}

	if (g_powerpack_message_ctx->objlock != NULL)
	{
		eaf_lock_destroy(g_powerpack_message_ctx->objlock);
		g_powerpack_message_ctx->objlock = NULL;
	}

	free(g_powerpack_message_ctx);
	g_powerpack_message_ctx = NULL;
}

void eaf_powerpack_message_commit(_Inout_ eaf_service_local_t* local, _Inout_opt_ void* arg)
{
	eaf_msg_t* req = (eaf_msg_t*)arg;
	assert(eaf_msg_get_type(req) == eaf_msg_type_req);

	powerpack_message_record_t* record = malloc(sizeof(powerpack_message_record_t));
	if (record == NULL)
	{
		goto err_malloc;
	}
	record->data.uuid = req->info.constant.uuid;
	record->data.from = req->from;
	record->data.local = local;
	record->data.req = req;
	record->data.defcnt = local->unsafe[0].ww.w2;

	int ret;
	eaf_lock_enter(g_powerpack_message_ctx->objlock);
	do 
	{
		ret = eaf_map_insert(&g_powerpack_message_ctx->table, &record->node);
	} while (0);
	eaf_lock_leave(g_powerpack_message_ctx->objlock);
	if (ret < 0)
	{
		goto err_table;
	}

	/* replace response handler */
	eaf_msg_set_rsp_fn(req, _powerpack_message_on_rsp_proxy);

	/* send request */
	if (eaf_send_req(powerpack_get_service_id(), local->unsafe[0].ww.w1, req) < 0)
	{
		goto err_send;
	}

	/* here just wait for response */
	eaf_uv_mod();
	return;

err_send:
	eaf_lock_enter(g_powerpack_message_ctx->objlock);
	do 
	{
		eaf_map_erase(&g_powerpack_message_ctx->table, &record->node);
	} while (0);
	eaf_lock_leave(g_powerpack_message_ctx->objlock);
err_table:
	free(record);
err_malloc:
	_powerpack_message_decref(req, local->unsafe[0].ww.w2);
	local->unsafe[0].v_ptr = NULL;
	eaf_resume(local->id);
}
