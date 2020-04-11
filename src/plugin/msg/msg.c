#include <assert.h>
#include "EAF/core/service.h"
#include "EAF/utils/errno.h"
#include "EAF/utils/map_low.h"
#include "EAF/plugin/plugin.h"
#include "compat/mutex.h"
#include "compat/thread.h"
#include "utils/memory.h"
#include "msg.h"

typedef struct eaf_plugin_msg_record
{
	eaf_map_low_node_t	node;	/** 侵入式节点 */

	struct
	{
		eaf_msg_t*		orig;	/** 原始请求地址 */
		eaf_msg_t*		rsp;	/** 响应数据 */
		uint32_t		from;	/** 发送方服务ID */
	}data;
}eaf_plugin_msg_record_t;

typedef struct eaf_plugin_msg_ctx
{
	eaf_mutex_t			objlock;
	eaf_map_low_t		wait_table;
}eaf_plugin_msg_ctx_t;

static eaf_plugin_msg_ctx_t* g_eaf_plugin_msg_ctx = NULL;

static int _eaf_plugin_msg_save_record(eaf_plugin_msg_record_t* rec)
{
	int ret;
	EAF_MAP_LOW_INSERT_HELPER(ret, &g_eaf_plugin_msg_ctx->wait_table, eaf_plugin_msg_record_t, rec,
		rec->data.orig < orig->data.orig ? -1 : (rec->data.orig > orig->data.orig ? 1 : 0));
	return ret;
}

static eaf_plugin_msg_record_t* _eaf_plugin_msg_find_by_req(eaf_msg_t* req)
{
	eaf_plugin_msg_record_t* ret;
	EAF_MAP_LOW_FIND_HELPER(ret, &g_eaf_plugin_msg_ctx->wait_table, eaf_plugin_msg_record_t, req,
		req < orig->data.orig ? -1 : (req > orig->data.orig ? 1 : 0));
	return ret;
}

static eaf_plugin_msg_record_t* _eaf_plugin_msg_find_by_service_id(uint32_t service_id)
{
	eaf_plugin_msg_record_t* ret;
	EAF_MAP_LOW_FIND_HELPER(ret, &g_eaf_plugin_msg_ctx->wait_table, eaf_plugin_msg_record_t, service_id,
		service_id < orig->data.from ? -1 : (service_id > orig->data.from ? 1 : 0));
	return ret;
}

int eaf_plugin_msg_init(void)
{
	if (g_eaf_plugin_msg_ctx != NULL)
	{
		return eaf_errno_duplicate;
	}

	g_eaf_plugin_msg_ctx = EAF_MALLOC(sizeof(eaf_plugin_msg_ctx_t));
	if (g_eaf_plugin_msg_ctx == NULL)
	{
		return eaf_errno_memory;
	}
	g_eaf_plugin_msg_ctx->wait_table = EAF_MAP_LOW_INIT;

	if (eaf_mutex_init(&g_eaf_plugin_msg_ctx->objlock, eaf_mutex_attr_normal) < 0)
	{
		EAF_FREE(g_eaf_plugin_msg_ctx);
		g_eaf_plugin_msg_ctx = NULL;
		return eaf_errno_unknown;
	}

	return eaf_errno_success;
}

void eaf_plugin_msg_exit(void)
{
	eaf_map_low_node_t* it;
	if (g_eaf_plugin_msg_ctx == NULL)
	{
		return;
	}

	it = eaf_map_low_first(&g_eaf_plugin_msg_ctx->wait_table);
	while (it != NULL)
	{
		eaf_map_low_node_t* tmp = it;
		it = eaf_map_low_next(it);
		eaf_map_low_erase(&g_eaf_plugin_msg_ctx->wait_table, tmp);

		eaf_plugin_msg_record_t* record = EAF_CONTAINER_OF(tmp, eaf_plugin_msg_record_t, node);
		eaf_msg_dec_ref(record->data.rsp);
		EAF_FREE(record);
	}

	eaf_mutex_exit(&g_eaf_plugin_msg_ctx->objlock);
	EAF_FREE(g_eaf_plugin_msg_ctx);
	g_eaf_plugin_msg_ctx = NULL;
}

int eaf_plugin_msg_send_req(uint32_t from, uint32_t to, eaf_msg_t* msg)
{
	int ret = eaf_errno_success;
	eaf_plugin_msg_record_t* rec = EAF_MALLOC(sizeof(eaf_plugin_msg_record_t));
	if (rec == NULL)
	{
		ret = eaf_errno_memory;
		goto fin;
	}

	rec->data.orig = msg;
	rec->data.from = from;
	rec->data.rsp = NULL;

	/* 保存数据。这一步不应该失败 */
	eaf_mutex_enter(&g_eaf_plugin_msg_ctx->objlock);
	do 
	{
		ret = _eaf_plugin_msg_save_record(rec);
	} while (0);
	eaf_mutex_leave(&g_eaf_plugin_msg_ctx->objlock);

	assert(ret == 0);
	if (ret < 0)
	{
		ret = eaf_errno_duplicate;
		goto err_free;
	}

	/* 发送数据 */
	if ((ret = eaf_send_req(EAF_PLUGIN_SERVICE, to, msg)) < 0)
	{
		goto err_del;
	}
	goto fin;

err_del:
	eaf_mutex_enter(&g_eaf_plugin_msg_ctx->objlock);
	do 
	{
		eaf_map_low_erase(&g_eaf_plugin_msg_ctx->wait_table, &rec->node);
	} while (0);
	eaf_mutex_leave(&g_eaf_plugin_msg_ctx->objlock);
err_free:
	EAF_FREE(rec);
fin:
	eaf_msg_dec_ref(msg);
	return ret;
}

void eaf_plugin_msg_proxy_handle(eaf_msg_t* msg)
{
	eaf_plugin_msg_record_t* rec = NULL;
	eaf_mutex_enter(&g_eaf_plugin_msg_ctx->objlock);
	do 
	{
		rec = _eaf_plugin_msg_find_by_req(msg->info.rsp.orig);
	} while (0);
	eaf_mutex_leave(&g_eaf_plugin_msg_ctx->objlock);

	/* 操作必须成功 */
	assert(rec != NULL);

	rec->data.rsp = msg;
	eaf_msg_add_ref(msg);

	/* 操作失败时，说明响应过快到达，需要等待目标协程yield */
	int ret;
	while ((ret = eaf_resume(rec->data.from)) != eaf_errno_success)
	{
		eaf_thread_sleep(1);
	}
}

eaf_msg_t* eaf_plugin_msg_get_rsp(uint32_t from)
{
	eaf_plugin_msg_record_t* rec = NULL;
	eaf_mutex_enter(&g_eaf_plugin_msg_ctx->objlock);
	do 
	{
		rec = _eaf_plugin_msg_find_by_service_id(from);
		if (rec != NULL)
		{
			eaf_map_low_erase(&g_eaf_plugin_msg_ctx->wait_table, &rec->node);
		}
	} while (0);
	eaf_mutex_leave(&g_eaf_plugin_msg_ctx->objlock);

	if (rec == NULL)
	{
		return NULL;
	}

	eaf_msg_t* rsp = rec->data.rsp;
	EAF_FREE(rec);
	return rsp;
}
