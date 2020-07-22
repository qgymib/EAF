#include <stdlib.h>
#include <assert.h>
#include "message.h"

static int _message_on_cmp_record(const eaf_map_node_t* key1, const eaf_map_node_t* key2, void* arg);

typedef struct pp_message_record
{
	eaf_map_node_t				node;
	struct
	{
		uint64_t				uuid;
		eaf_msg_t*				orig_req;
		eaf_msg_t*				orig_rsp;
		uint32_t				orig_from;
		int						ret;
	}data;
}pp_message_record_t;

typedef struct pp_message_ctx
{
	eaf_lock_t*					objlock;
	eaf_map_t					pend_table;
	eaf_map_t					ready_table;
}pp_message_ctx_t;

static pp_message_ctx_t g_pp_message_ctx = {
	NULL,
	EAF_MAP_INITIALIZER(_message_on_cmp_record, NULL),
	EAF_MAP_INITIALIZER(_message_on_cmp_record, NULL),
};

static int _message_on_cmp_record(const eaf_map_node_t* key1, const eaf_map_node_t* key2, void* arg)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(arg);
	pp_message_record_t* rec_1 = EAF_CONTAINER_OF(key1, pp_message_record_t, node);
	pp_message_record_t* rec_2 = EAF_CONTAINER_OF(key2, pp_message_record_t, node);

	if (rec_1->data.uuid == rec_2->data.uuid)
	{
		return 0;
	}
	return rec_1->data.uuid < rec_2->data.uuid ? -1 : 1;
}

static int _message_on_init(void)
{
	return eaf_errno_success;
}

static void _message_on_exit(void)
{
}

int eaf_message_init(void)
{
	if (g_pp_message_ctx.objlock != NULL)
	{
		return eaf_errno_state;
	}

	if ((g_pp_message_ctx.objlock = eaf_lock_create(eaf_lock_attr_normal)) == NULL)
	{
		return eaf_errno_memory;
	}

	static eaf_entrypoint_t entry = {
		0, NULL,
		_message_on_init,
		_message_on_exit,
	};
	return eaf_register(EAF_MESSAGE_ID, &entry);
}

void eaf_message_exit(void)
{
	if (g_pp_message_ctx.objlock == NULL)
	{
		return;
	}

	eaf_lock_destroy(g_pp_message_ctx.objlock);
	g_pp_message_ctx.objlock = NULL;
}

void eaf_message_internal_proxy(_Inout_ eaf_service_local_t* local, _Inout_opt_ void* arg)
{
	int ret;
	uint32_t id_to = local->unsafe[0].ww.w1;
	eaf_msg_t* orig_req = (eaf_msg_t*)arg;

	/* save uuid as Service Local Information */
	local->unsafe[0].v_u64 = orig_req->info.constant.uuid;

	pp_message_record_t* record = malloc(sizeof(pp_message_record_t));
	assert(record != NULL);
	record->data.uuid = orig_req->info.constant.uuid;
	record->data.orig_req = orig_req;
	record->data.orig_rsp = NULL;
	record->data.orig_from = local->id;
	record->data.ret = eaf_errno_success;

	eaf_lock_enter(g_pp_message_ctx.objlock);
	{
		ret = eaf_map_insert(&g_pp_message_ctx.pend_table, &record->node);
	}
	eaf_lock_leave(g_pp_message_ctx.objlock);
	assert(ret == 0);

	if ((ret = eaf_send_req(EAF_MESSAGE_ID, id_to, orig_req)) < 0)
	{
		goto err_send_req;
	}

	return;

err_send_req:
	/* save error code */
	eaf_lock_enter(g_pp_message_ctx.objlock);
	{
		eaf_map_erase(&g_pp_message_ctx.pend_table, &record->node);
		eaf_map_insert(&g_pp_message_ctx.ready_table, &record->node);
		record->data.ret = ret;
	}
	eaf_lock_leave(g_pp_message_ctx.objlock);
	/* wake up service */
	eaf_resume(local->id);
}

void eaf_message_internal_response_handler(_In_ uint32_t from, _In_ uint32_t to, _Inout_ eaf_msg_t* msg)
{
	int ret;
	EAF_SUPPRESS_UNUSED_VARIABLE(from, to, ret);

	pp_message_record_t tmp;
	tmp.data.uuid = msg->info.constant.uuid;
	eaf_msg_add_ref(msg);

	uint32_t orig_from;
	eaf_lock_enter(g_pp_message_ctx.objlock);
	{
		eaf_map_node_t* it = eaf_map_find(&g_pp_message_ctx.pend_table, &tmp.node);
		assert(it != NULL);

		eaf_map_erase(&g_pp_message_ctx.pend_table, it);
		ret = eaf_map_insert(&g_pp_message_ctx.ready_table, it);
		assert(ret == 0);

		pp_message_record_t* record = EAF_CONTAINER_OF(it, pp_message_record_t, node);
		record->data.orig_rsp = msg;
		orig_from = record->data.orig_from;
	}
	eaf_lock_leave(g_pp_message_ctx.objlock);

	ret = eaf_resume(orig_from);
	assert(ret == 0);
}

int eaf_message_internal_finalize(_In_ uint64_t uuid, _Out_ eaf_msg_t** rsp)
{
	pp_message_record_t tmp;
	tmp.data.uuid = uuid;

	pp_message_record_t* record = NULL;
	eaf_lock_enter(g_pp_message_ctx.objlock);
	do 
	{
		eaf_map_node_t* it = eaf_map_find(&g_pp_message_ctx.ready_table, &tmp.node);
		if (it == NULL)
		{
			break;
		}
		record = EAF_CONTAINER_OF(it, pp_message_record_t, node);
		eaf_map_erase(&g_pp_message_ctx.ready_table, it);

	} EAF_MSVC_WARNING_GUARD(4127, while(0));
	eaf_lock_leave(g_pp_message_ctx.objlock);

	if (record == NULL)
	{
		return eaf_errno_notfound;
	}

	*rsp = record->data.orig_rsp;
	eaf_msg_dec_ref(record->data.orig_req);
	free(record);

	return eaf_errno_success;
}
