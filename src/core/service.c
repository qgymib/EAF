#include <assert.h>
#include "EAF/core/service.h"
#include "EAF/utils/list.h"
#include "EAF/utils/errno.h"
#include "EAF/utils/define.h"
#include "EAF/utils/map.h"
#include "EAF/utils/log.h"
#include "compat/mutex.h"
#include "compat/thread.h"
#include "compat/semaphore.h"
#include "utils/memory.h"
#include "message.h"

#define JMP_STATE_DIRECT			0			/** 自然流程 */
#define JMP_STATE_SWITCH			1			/** 上下文切换 */
#define JMP_STATE_RETURN			2			/** 用户调用返回 */

/**
* 对比模板
* @param a		值a
* @param b		值b
*/
#define COMPARE_TEMPLATE(a, b)	\
	do {\
		if ((a) < (b)) {\
			return -1;\
		} else if ((a) > (b)) {\
			return 1;\
		}\
	} while (0)

#define CUR_GROUP								(_eaf_get_current_group())

/**
* 服务状态
*                      |--------|
*                     \|/       |
* INIT0 --> INIT1 --> IDLE --> BUSY --> PEND
*             |        |       /|\       |
*             |       \|/       |--------|
*             |-----> EXIT
*/
typedef enum eaf_service_state
{
	eaf_service_state_init0,					/** 初始态 */
	eaf_service_state_init1,					/** 初始态 */
	eaf_service_state_idle,						/** 空闲 */
	eaf_service_state_busy,						/** 忙碌 */
	eaf_service_state_pend,						/** 等待resume */
	eaf_service_state_exit,						/** 退出 */
}eaf_service_state_t;

typedef enum eaf_ctx_state
{
	eaf_ctx_state_init,							/** 初始状态 */
	eaf_ctx_state_busy,							/** 运行状态 */
	eaf_ctx_state_exit,							/** 退出状态 */
}eaf_ctx_state_t;

typedef struct eaf_msgq_record
{
	eaf_list_node_t					node;		/** 侵入式节点 */

	union
	{
		struct
		{
			eaf_req_handle_fn		req_fn;		/** 请求处理函数 */
		}req;
	}info;

	struct
	{
		struct eaf_service*			service;	/** 服务句柄 */
		eaf_msg_full_t*				msg;		/** 消息 */
	}data;
}eaf_msgq_record_t;

typedef struct eaf_subscribe_record
{
	eaf_map_node_t					node;		/** 侵入式节点 */
	struct
	{
		uint32_t					evt_id;		/** 事件ID */
		struct eaf_service*			service;	/** 服务句柄 */
		eaf_evt_handle_fn			proc;		/** 回调函数 */
		void*						priv;		/** 自定义参数 */
	}data;
}eaf_subscribe_record_t;

typedef struct eaf_service
{
	uint32_t						service_id;	/** 服务ID */
	const eaf_service_info_t*		load;		/** 加载信息 */
	eaf_service_state_t				state;		/** 状态 */

	struct
	{
		eaf_list_node_t				node;		/** 侵入式节点。在ready_list或wait_list中 */
		eaf_jmp_buf_t				jmpbuf;		/** 跳转上下文 */
	}filber;

	struct
	{
		eaf_map_node_t*				cbiter;		/** 回调游标，eaf_subscribe_record_t */
	}subscribe;

	struct
	{
		unsigned					continue_evt : 1;	/** 处理事件返回 */
	}flag;

	struct
	{
		eaf_msgq_record_t*			cur_msg;	/** 正在处理的消息 */
		eaf_list_t					queue;		/** 缓存的消息 */
		size_t						capacity;	/** 消息队列容量 */
	}msgq;
}eaf_service_t;

typedef struct eaf_service_group
{
	eaf_mutex_t						objlock;	/** 线程锁 */
	eaf_thread_t					working;	/** 承载线程 */

	struct
	{
		size_t						init;		/** 初始化游标 */
		size_t						init_cnt;	/** 初始化计数器 */
	}idx;

	struct 
	{
		eaf_service_t*				cur_run;	/** 当前正在处理的服务 */

		eaf_list_t					busy_list;	/** 执行列表 */
		eaf_list_t					wait_list;	/** 等待列表 */

		eaf_jmp_buf_t				jmpbuf;		/** 跳转上下文 */
	}filber;

	struct
	{
		eaf_map_t					table;		/** 订阅表，eaf_subscribe_record_t */
	}subscribe;

	struct
	{
		eaf_sem_t					sem;		/** 信号量 */
		eaf_list_t					msg_gc;		/** GC队列 */
		unsigned					wait_time;	/** 等待时间 */
	}msgq;

	struct
	{
		size_t						size;		/** 服务表长度 */
		eaf_service_t				table[];	/** 服务表 */
	}service;
}eaf_service_group_t;

typedef struct eaf_ctx
{
	eaf_ctx_state_t					state;		/** 状态 */
	eaf_sem_t						ready;		/** 退出信号 */
	eaf_thread_storage_t			tls;		/** 线程私有变量 */

	struct
	{
		size_t						size;		/** 服务组长度 */
		eaf_service_group_t**		table;		/** 服务组 */
	}group;
}eaf_ctx_t;

static eaf_ctx_t* g_eaf_ctx			= NULL;		/** 全局运行环境 */

static eaf_service_t* _eaf_service_find_service(uint32_t service_id, eaf_service_group_t** group)
{
	size_t i;
	for (i = 0; i < g_eaf_ctx->group.size; i++)
	{
		size_t j;
		for (j = 0; j < g_eaf_ctx->group.table[i]->service.size; j++)
		{
			eaf_service_t* service = &g_eaf_ctx->group.table[i]->service.table[j];
			if (service->service_id != service_id)
			{
				continue;
			}

			if (group != NULL)
			{
				*group = g_eaf_ctx->group.table[i];
			}
			return service;
		}
	}

	return NULL;
}

static void _eaf_service_destroy_msg_record(eaf_msgq_record_t* record)
{
	eaf_msg_dec_ref(EAF_MSG_C2I(record->data.msg));
	EAF_FREE(record);
}

static void _eaf_service_msgq_gc(eaf_service_group_t* group)
{
	eaf_list_node_t* it;
	while (1)
	{
		eaf_mutex_enter(&group->objlock);
		it = eaf_list_pop_front(&group->msgq.msg_gc);
		eaf_mutex_leave(&group->objlock);
		if (it == NULL)
		{
			break;
		}

		eaf_msgq_record_t* record = EAF_CONTAINER_OF(it, eaf_msgq_record_t, node);
		_eaf_service_destroy_msg_record(record);
	}
}

static eaf_service_t* _eaf_get_first_busy_service_lock(eaf_service_group_t* group)
{
	eaf_service_t* service;
	eaf_mutex_enter(&group->objlock);
	do 
	{
		eaf_list_node_t* it = eaf_list_begin(&group->filber.busy_list);
		service = it != NULL ? EAF_CONTAINER_OF(it, eaf_service_t, filber.node) : NULL;
	} while (0);
	eaf_mutex_leave(&group->objlock);

	return service;
}

/**
* 设置服务状态
* @param group		组
* @param service	服务
* @param state		状态
*/
static void _eaf_service_set_state_nolock(eaf_service_group_t* group, eaf_service_t* service, eaf_service_state_t state)
{
	if (service->state == state)
	{
		return;
	}

	switch (service->state)
	{
	case eaf_service_state_init1:
	case eaf_service_state_idle:
	case eaf_service_state_pend:
	case eaf_service_state_exit:
		eaf_list_erase(&group->filber.wait_list, &service->filber.node);
		break;

	case eaf_service_state_init0:
	case eaf_service_state_busy:
		eaf_list_erase(&group->filber.busy_list, &service->filber.node);
		break;
	}

	switch (state)
	{
	case eaf_service_state_init1:
	case eaf_service_state_idle:
	case eaf_service_state_pend:
	case eaf_service_state_exit:
		eaf_list_push_back(&group->filber.wait_list, &service->filber.node);
		break;

	case eaf_service_state_init0:
	case eaf_service_state_busy:
		eaf_list_push_back(&group->filber.busy_list, &service->filber.node);
		break;
	}
	service->state = state;
}

static void _eaf_service_set_state_lock(eaf_service_group_t* group, eaf_service_t* service, eaf_service_state_t state)
{
	eaf_mutex_enter(&group->objlock);
	do 
	{
		_eaf_service_set_state_nolock(group, service, state);
	} while (0);
	eaf_mutex_leave(&group->objlock);
}

static void _eaf_handle_new_message_event(eaf_service_group_t* group, eaf_service_t* service)
{
	eaf_evt_handle_fn proc;
	void* priv;

	eaf_subscribe_record_t tmp_record;
	tmp_record.data.evt_id = service->msgq.cur_msg->data.msg->msg.id;
	tmp_record.data.service = service;
	tmp_record.data.proc = NULL;
	tmp_record.data.priv = NULL;

	eaf_mutex_enter(&group->objlock);
	for (service->subscribe.cbiter = eaf_map_find_upper(&group->subscribe.table, &tmp_record.node);
		service->subscribe.cbiter != NULL;
		service->subscribe.cbiter = eaf_map_next(&group->subscribe.table, service->subscribe.cbiter))
	{
		eaf_subscribe_record_t* orig = EAF_CONTAINER_OF(service->subscribe.cbiter, eaf_subscribe_record_t, node);
		if (orig->data.evt_id != service->msgq.cur_msg->data.msg->msg.id || orig->data.service != service->msgq.cur_msg->data.service)
		{
			break;
		}
		proc = orig->data.proc;
		priv = orig->data.priv;

		eaf_mutex_leave(&group->objlock);
		proc(EAF_MSG_C2I(service->msgq.cur_msg->data.msg), priv);
		eaf_mutex_enter(&group->objlock);
	}
	eaf_mutex_leave(&group->objlock);
}

static void _eaf_handle_new_message(eaf_service_group_t* group, eaf_service_t* service, int jmpcode)
{
	/* 现在service的状态只可能是BUSY */
	assert(service->state == eaf_service_state_busy);

	/* 取出消息。若消息队列为空，则将服务置于IDLE状态 */
	eaf_mutex_enter(&group->objlock);
	do 
	{
		eaf_list_node_t* it = eaf_list_pop_front(&service->msgq.queue);
		if (it == NULL)
		{
			_eaf_service_set_state_nolock(group, service, eaf_service_state_idle);
			break;
		}
		service->msgq.cur_msg = EAF_CONTAINER_OF(it, eaf_msgq_record_t, node);
	} while (0);
	eaf_mutex_leave(&group->objlock);

	if (service->msgq.cur_msg == NULL)
	{
		return;
	}

	switch (service->msgq.cur_msg->data.msg->msg.type)
	{
	case eaf_msg_type_req:
		service->msgq.cur_msg->info.req.req_fn(EAF_MSG_C2I(service->msgq.cur_msg->data.msg));
		goto fin;

	case eaf_msg_type_rsp:
		service->msgq.cur_msg->data.msg->info.rsp.rsp_fn(EAF_MSG_C2I(service->msgq.cur_msg->data.msg));
		goto fin;

	case eaf_msg_type_evt:
		goto handle_evt;
	}

handle_evt:
	_eaf_handle_new_message_event(group, service);

fin:
	_eaf_service_destroy_msg_record(service->msgq.cur_msg);
	service->msgq.cur_msg = NULL;
}

static void _eaf_service_handle_code_switch(eaf_service_group_t* group, int jmpcode)
{
	if (group->filber.cur_run->state == eaf_service_state_init0)
	{
		_eaf_service_set_state_lock(group, group->filber.cur_run, eaf_service_state_init1);
	}
	else
	{
		_eaf_service_set_state_lock(group, group->filber.cur_run, eaf_service_state_pend);
	}
	return;
}

static void _eaf_service_handle_code_direct(eaf_service_group_t* group, int jmpcode)
{
	eaf_service_t* service = _eaf_get_first_busy_service_lock(group);
	if (service == NULL)
	{
		eaf_sem_pend(&group->msgq.sem, -1);
		return;
	}
	eaf_sem_pend(&group->msgq.sem, 0);
	group->filber.cur_run = service;

	/* 必要时恢复上下文 */
	if (service->state == eaf_service_state_init0 || service->msgq.cur_msg != NULL)
	{
		longjmp(service->filber.jmpbuf.env, JMP_STATE_SWITCH);
		return;
	}

	_eaf_handle_new_message(group, service, jmpcode);
}

/**
* 继续执行上下文
* @param group		组
* @param service	服务
*/
static void _eaf_service_continue_context(eaf_service_group_t* group, eaf_service_t* service)
{
	/* 若消息类型非事件，则可以销毁 */
	if (service->msgq.cur_msg->data.msg->msg.type != eaf_msg_type_evt)
	{
		goto cleanup;
	}

	eaf_evt_handle_fn proc;
	void* priv;

	/* 继续处理下一个事件 */
	eaf_mutex_enter(&group->objlock);
	for (service->subscribe.cbiter = eaf_map_next(&group->subscribe.table, service->subscribe.cbiter);
		service->subscribe.cbiter != NULL;
		service->subscribe.cbiter = eaf_map_next(&group->subscribe.table, service->subscribe.cbiter))
	{
		eaf_subscribe_record_t* orig = EAF_CONTAINER_OF(service->subscribe.cbiter, eaf_subscribe_record_t, node);
		if (orig->data.evt_id != service->msgq.cur_msg->data.msg->msg.id || orig->data.service != service->msgq.cur_msg->data.service)
		{
			break;
		}
		proc = orig->data.proc;
		priv = orig->data.priv;

		eaf_mutex_leave(&group->objlock);
		proc(EAF_MSG_C2I(service->msgq.cur_msg->data.msg), priv);
		eaf_mutex_enter(&group->objlock);
	}
	eaf_mutex_leave(&group->objlock);

cleanup:
	_eaf_service_destroy_msg_record(service->msgq.cur_msg);
	service->msgq.cur_msg = NULL;

	return;
}

static void _eaf_group_finish_service_init_lock(eaf_service_group_t* group, eaf_service_t* service)
{
	eaf_mutex_enter(&group->objlock);
	do
	{
		/* 切换至IDLE态 */
		_eaf_service_set_state_nolock(group, service, eaf_service_state_idle);
		/* 消息队列非空时，切换至BUSY */
		if (eaf_list_size(&service->msgq.queue) > 0)
		{
			_eaf_service_set_state_nolock(group, service, eaf_service_state_busy);
		}
	} while (0);
	eaf_mutex_leave(&group->objlock);
}

static void _eaf_service_handle_code_return(eaf_service_group_t* group, int jmpcode)
{
	eaf_service_t* service = group->filber.cur_run;
	if (service->state == eaf_service_state_init0)
	{
		_eaf_group_finish_service_init_lock(group, service);
		return;
	}

	_eaf_service_continue_context(group, service);
}

EAF_NOINLINE
static void _eaf_service_thread_loop(eaf_service_group_t* group, int jmpcode)
{
	_eaf_service_msgq_gc(group);

	switch (jmpcode)
	{
	case JMP_STATE_DIRECT:
		_eaf_service_handle_code_direct(group, jmpcode);
		break;

	case JMP_STATE_SWITCH:
		_eaf_service_handle_code_switch(group, jmpcode);
		break;

	case JMP_STATE_RETURN:
		_eaf_service_handle_code_return(group, jmpcode);
		break;
	}
	
	group->filber.cur_run = NULL;
	return;
}

/**
* 获取当前线程对应的服务组
* @return		服务组
*/
static eaf_service_group_t* _eaf_get_current_group(void)
{
	return eaf_thread_storage_get(&g_eaf_ctx->tls);
}

/**
* 事件循环。此处会处理本线程中所有服务的数据。
*/
EAF_NOINLINE
static void _eaf_service_thread_body(void)
{
	/* 由于编译器优化，此函数体内的栈不可信，因此不能通过参数传递给出线程组 */
	int jmp_code = setjmp(CUR_GROUP->filber.jmpbuf.env);

	/* 事件循环 */
	while (g_eaf_ctx->state == eaf_ctx_state_busy)
	{
		_eaf_service_thread_loop(CUR_GROUP, jmp_code);
		jmp_code = JMP_STATE_DIRECT;
	}
}

/**
* 工作线程
* @param arg	eaf_service_group_t
*/
static void _eaf_service_thread(void* arg)
{
	size_t i;
	/* 设置线程私有变量 */
	if (eaf_thread_storage_set(&g_eaf_ctx->tls, arg) < 0)
	{
		return;
	}

	/* 等待就绪 */
	while (EAF_ACCESS(eaf_ctx_state_t, g_eaf_ctx->state) == eaf_ctx_state_init)
	{
		eaf_sem_pend(&CUR_GROUP->msgq.sem, -1);
	}
	if (g_eaf_ctx->state != eaf_ctx_state_busy)
	{
		return;
	}

	/*
	* 若在此处返回，则模块初始化处发生协程切换
	* 当在初始化阶段发生协程切换时，需要继续继续其他服务的初始化
	*/
	CUR_GROUP->idx.init = 0;
	CUR_GROUP->idx.init_cnt = 0;
	switch (setjmp(CUR_GROUP->filber.jmpbuf.env))
	{
	case JMP_STATE_DIRECT:
		break;

		/* init阶段没有进行yield，直接使用了eaf_return */
	case JMP_STATE_RETURN:
		_eaf_group_finish_service_init_lock(CUR_GROUP, CUR_GROUP->filber.cur_run);
		CUR_GROUP->idx.init++;
		CUR_GROUP->idx.init_cnt++;
		break;

		/* init阶段进行了yield */
	case JMP_STATE_SWITCH:
		_eaf_service_set_state_nolock(CUR_GROUP, CUR_GROUP->filber.cur_run, eaf_service_state_init1);
		CUR_GROUP->idx.init++;
		CUR_GROUP->idx.init_cnt++;
		break;
	}

	/* 初始化当前线程包含的服务 */
	for (; CUR_GROUP->idx.init < CUR_GROUP->service.size; CUR_GROUP->idx.init++)
	{
		if (CUR_GROUP->service.table[CUR_GROUP->idx.init].load == NULL
			|| CUR_GROUP->service.table[CUR_GROUP->idx.init].load->on_init == NULL)
		{
			continue;
		}

		/* 标记当前正在运行的服务 */
		CUR_GROUP->filber.cur_run = &CUR_GROUP->service.table[CUR_GROUP->idx.init];
		if (CUR_GROUP->filber.cur_run->load->on_init() < 0)
		{
			goto cleanup;
		}
		_eaf_group_finish_service_init_lock(CUR_GROUP, CUR_GROUP->filber.cur_run);

		CUR_GROUP->idx.init_cnt++;
	}

	/* 通告初始化完毕 */
	eaf_sem_post(&g_eaf_ctx->ready);

	/* 若本线程未部署任何服务，则退出 */
	if (CUR_GROUP->idx.init_cnt == 0)
	{
		return;
	}

	/* 处理消息 */
	_eaf_service_thread_body();

cleanup:
	for (i = 0; i < CUR_GROUP->idx.init; i++)
	{
		if (CUR_GROUP->service.table[i].load != NULL
			&& CUR_GROUP->service.table[i].load->on_exit != NULL)
		{
			CUR_GROUP->filber.cur_run = &CUR_GROUP->service.table[i];

			_eaf_service_set_state_lock(CUR_GROUP, CUR_GROUP->filber.cur_run, eaf_service_state_exit);
			CUR_GROUP->filber.cur_run->load->on_exit();
		}
	}
}

static void _eaf_service_cleanup_group(eaf_service_group_t* group)
{
	/* 向队列推送以保证线程感知到状态改变 */
	eaf_sem_post(&group->msgq.sem);

	eaf_map_node_t* it = eaf_map_begin(&group->subscribe.table);
	while (it != NULL)
	{
		eaf_map_node_t* tmp = it;
		it = eaf_map_next(&group->subscribe.table, it);
		eaf_map_erase(&group->subscribe.table, tmp);

		eaf_subscribe_record_t* record = EAF_CONTAINER_OF(tmp, eaf_subscribe_record_t, node);
		EAF_FREE(record);
	}

	/* 清理资源 */
	eaf_thread_exit(&group->working);
	eaf_mutex_exit(&group->objlock);
	eaf_sem_exit(&group->msgq.sem);
}

static int _eaf_service_push_msg(eaf_service_group_t* group, eaf_service_t* service, eaf_msg_full_t* msg,
	int lock, void(*on_create)(eaf_msgq_record_t* record, void* arg), void* arg)
{
	/* 检查消息队列容量 */
	if (eaf_list_size(&service->msgq.queue) >= service->msgq.capacity)
	{
		return eaf_errno_overflow;
	}

	eaf_msgq_record_t* record = EAF_MALLOC(sizeof(eaf_msgq_record_t));
	if (record == NULL)
	{
		return eaf_errno_memory;
	}

	record->data.service = service;
	record->data.msg = msg;
	eaf_msg_add_ref(EAF_MSG_C2I(msg));

	if (on_create != NULL)
	{
		on_create(record, arg);
	}

	if (lock) { eaf_mutex_enter(&group->objlock); }
	do
	{
		if (service->state == eaf_service_state_idle)
		{
			_eaf_service_set_state_nolock(group, service, eaf_service_state_busy);
		}
		eaf_list_push_back(&service->msgq.queue, &record->node);
	} while (0);
	if (lock) { eaf_mutex_leave(&group->objlock); }

	eaf_sem_post(&group->msgq.sem);
	return eaf_errno_success;
}

static void _eaf_service_on_req_record_create(eaf_msgq_record_t* record, void* arg)
{
	record->info.req.req_fn = (eaf_req_handle_fn)arg;
}

static int _eaf_service_on_cmp_subscribe_record(const eaf_map_node_t* key1, const eaf_map_node_t* key2, void* arg)
{
	eaf_subscribe_record_t* record1 = EAF_CONTAINER_OF(key1, eaf_subscribe_record_t, node);
	eaf_subscribe_record_t* record2 = EAF_CONTAINER_OF(key2, eaf_subscribe_record_t, node);

	COMPARE_TEMPLATE(record1->data.evt_id, record2->data.evt_id);
	COMPARE_TEMPLATE(record1->data.service, record2->data.service);
	COMPARE_TEMPLATE(record1->data.proc, record2->data.proc);
	COMPARE_TEMPLATE(record1->data.priv, record2->data.priv);

	return 0;
}

static eaf_service_t* _eaf_get_current_service(eaf_service_group_t** group)
{
	eaf_service_group_t* ret = _eaf_get_current_group();

	if (group != NULL)
	{
		*group = ret;
	}
	return ret != NULL ? ret->filber.cur_run : NULL;
}

int eaf_setup(const eaf_thread_table_t* info, size_t size)
{
	size_t i;
	if (g_eaf_ctx != NULL)
	{
		return eaf_errno_duplicate;
	}

	/* 计算所需内存 */
	size_t malloc_size = sizeof(eaf_ctx_t) + sizeof(eaf_service_group_t*) * size;
	for (i = 0; i < size; i++)
	{
		malloc_size += sizeof(eaf_service_group_t) + sizeof(eaf_service_t) * info[i].service.size;
	}

	g_eaf_ctx = EAF_CALLOC(1, malloc_size);
	if (g_eaf_ctx == NULL)
	{
		return eaf_errno_memory;
	}
	g_eaf_ctx->state = eaf_ctx_state_init;
	if (eaf_sem_init(&g_eaf_ctx->ready, 0) < 0)
	{
		EAF_FREE(g_eaf_ctx);
		g_eaf_ctx = NULL;
		return eaf_errno_unknown;
	}
	if (eaf_thread_storage_init(&g_eaf_ctx->tls) < 0)
	{
		eaf_sem_exit(&g_eaf_ctx->ready);
		EAF_FREE(g_eaf_ctx);
		g_eaf_ctx = NULL;
		return eaf_errno_unknown;
	}

	/* 修复数组地址 */
	g_eaf_ctx->group.size = size;
	g_eaf_ctx->group.table = (eaf_service_group_t**)(g_eaf_ctx + 1);
	g_eaf_ctx->group.table[0] = (eaf_service_group_t*)((char*)g_eaf_ctx->group.table + sizeof(eaf_service_group_t*) * size);
	for (i = 1; i < size; i++)
	{
		g_eaf_ctx->group.table[i] = (eaf_service_group_t*)
			((char*)g_eaf_ctx->group.table[i - 1] + sizeof(eaf_service_group_t) + sizeof(eaf_service_t) * info[i - 1].service.size);
	}

	/* 资源初始化 */
	size_t init_idx;
	for (init_idx = 0; init_idx < size; init_idx++)
	{
		g_eaf_ctx->group.table[init_idx]->service.size = info[init_idx].service.size;
		eaf_list_init(&g_eaf_ctx->group.table[init_idx]->msgq.msg_gc);
		eaf_list_init(&g_eaf_ctx->group.table[init_idx]->filber.busy_list);
		eaf_list_init(&g_eaf_ctx->group.table[init_idx]->filber.wait_list);
		eaf_map_init(&g_eaf_ctx->group.table[init_idx]->subscribe.table, _eaf_service_on_cmp_subscribe_record, NULL);
		if (eaf_mutex_init(&g_eaf_ctx->group.table[init_idx]->objlock, eaf_mutex_attr_normal) < 0)
		{
			goto err;
		}
		if (eaf_sem_init(&g_eaf_ctx->group.table[init_idx]->msgq.sem, 0) < 0)
		{
			eaf_mutex_exit(&g_eaf_ctx->group.table[init_idx]->objlock);
			goto err;
		}
		if (eaf_thread_init(&g_eaf_ctx->group.table[init_idx]->working, NULL, _eaf_service_thread, g_eaf_ctx->group.table[init_idx]) < 0)
		{
			eaf_mutex_exit(&g_eaf_ctx->group.table[init_idx]->objlock);
			eaf_sem_exit(&g_eaf_ctx->group.table[init_idx]->msgq.sem);
			goto err;
		}

		size_t idx;
		for (idx = 0; idx < info[init_idx].service.size; idx++)
		{
			g_eaf_ctx->group.table[init_idx]->service.table[idx].service_id = info[init_idx].service.table[idx].srv_id;
			g_eaf_ctx->group.table[init_idx]->service.table[idx].msgq.capacity = info[init_idx].service.table[idx].msgq_size;
			eaf_list_init(&g_eaf_ctx->group.table[init_idx]->service.table[idx].msgq.queue);

			/* 默认情况下加入wait表 */
			g_eaf_ctx->group.table[init_idx]->service.table[idx].state = eaf_service_state_init0;
			eaf_list_push_back(&g_eaf_ctx->group.table[init_idx]->filber.busy_list, &g_eaf_ctx->group.table[init_idx]->service.table[idx].filber.node);
		}
	}

	return eaf_errno_success;

err:
	for (i = 0; i < init_idx; i++)
	{
		eaf_sem_post(&g_eaf_ctx->group.table[init_idx]->msgq.sem);

		eaf_thread_exit(&g_eaf_ctx->group.table[init_idx]->working);
		eaf_sem_exit(&g_eaf_ctx->group.table[init_idx]->msgq.sem);
		eaf_mutex_exit(&g_eaf_ctx->group.table[init_idx]->objlock);
	}

	eaf_thread_storage_exit(&g_eaf_ctx->tls);
	eaf_sem_exit(&g_eaf_ctx->ready);

	EAF_FREE(g_eaf_ctx);
	g_eaf_ctx = NULL;

	return eaf_errno_unknown;
}

int eaf_load(void)
{
	size_t i;
	if (g_eaf_ctx == NULL)
	{
		return eaf_errno_state;
	}

	/* 启动所有线程 */
	g_eaf_ctx->state = eaf_ctx_state_busy;
	for (i = 0; i < g_eaf_ctx->group.size; i++)
	{
		eaf_sem_post(&g_eaf_ctx->group.table[i]->msgq.sem);
	}

	/* 等待所有服务就绪 */
	for (i = 0; i < g_eaf_ctx->group.size; i++)
	{
		eaf_sem_pend(&g_eaf_ctx->ready, -1);
	}

	return eaf_errno_success;
}

int eaf_cleanup(void)
{
	size_t i;
	if (g_eaf_ctx == NULL)
	{
		return eaf_errno_state;
	}

	g_eaf_ctx->state = eaf_ctx_state_exit;
	for (i = 0; i < g_eaf_ctx->group.size; i++)
	{
		_eaf_service_cleanup_group(g_eaf_ctx->group.table[i]);
	}

	EAF_FREE(g_eaf_ctx);
	g_eaf_ctx = NULL;

	return eaf_errno_success;
}

int eaf_register(uint32_t id, const eaf_service_info_t* info)
{
	if (g_eaf_ctx == NULL)
	{
		return eaf_errno_state;
	}

	size_t i;
	eaf_service_t* service = NULL;
	for (i = 0; i < g_eaf_ctx->group.size; i++)
	{
		size_t idx;
		for (idx = 0; idx < g_eaf_ctx->group.table[i]->service.size; idx++)
		{
			if (g_eaf_ctx->group.table[i]->service.table[idx].service_id == id)
			{
				service = &g_eaf_ctx->group.table[i]->service.table[idx];
				break;
			}
		}
	}

	if (service == NULL)
	{
		return eaf_errno_notfound;
	}

	if (service->load != NULL)
	{
		return eaf_errno_duplicate;
	}

	service->load = info;
	return eaf_errno_success;
}

int eaf_subscribe(uint32_t srv_id, uint32_t evt_id, eaf_evt_handle_fn fn, void* arg)
{
	/* 非工作状态禁止订阅 */
	if (g_eaf_ctx->state != eaf_ctx_state_busy)
	{
		return eaf_errno_state;
	}

	eaf_service_group_t* group;
	eaf_service_t* service = _eaf_service_find_service(srv_id, &group);
	if (service == NULL)
	{
		return eaf_errno_notfound;
	}

	eaf_subscribe_record_t* record = EAF_MALLOC(sizeof(eaf_subscribe_record_t));
	if (record == NULL)
	{
		return eaf_errno_memory;
	}
	record->data.evt_id = evt_id;
	record->data.service = service;
	record->data.proc = fn;
	record->data.priv = arg;

	int ret;
	eaf_mutex_enter(&group->objlock);
	do 
	{
		ret = eaf_map_insert(&group->subscribe.table, &record->node);
	} while (0);
	eaf_mutex_leave(&group->objlock);

	if (ret < 0)
	{
		EAF_FREE(record);
		return eaf_errno_duplicate;
	}

	return eaf_errno_success;
}

int eaf_unsubscribe(uint32_t srv_id, uint32_t evt_id, eaf_evt_handle_fn fn, void* arg)
{
	eaf_service_group_t* group;
	eaf_service_t* service = _eaf_service_find_service(srv_id, &group);
	if (service == NULL)
	{
		return eaf_errno_notfound;
	}

	eaf_subscribe_record_t tmp_key;
	tmp_key.data.evt_id = evt_id;
	tmp_key.data.service = service;
	tmp_key.data.proc = fn;
	tmp_key.data.priv = arg;

	eaf_subscribe_record_t* record = NULL;
	eaf_mutex_enter(&group->objlock);
	do 
	{
		eaf_map_node_t* it = eaf_map_find(&group->subscribe.table, &tmp_key.node);
		if (it == NULL)
		{
			break;
		}
		record = EAF_CONTAINER_OF(it, eaf_subscribe_record_t, node);
		eaf_map_erase(&group->subscribe.table, it);

		/* 若删除记录与服务迭代器相符，则将服务迭代器前移 */
		if (it == service->subscribe.cbiter)
		{
			service->subscribe.cbiter = eaf_map_prev(&group->subscribe.table, it);
		}
	} while (0);
	eaf_mutex_leave(&group->objlock);

	if (record == NULL)
	{
		return eaf_errno_notfound;
	}

	EAF_FREE(record);
	return eaf_errno_success;
}

int eaf_send_req(uint32_t from, uint32_t to, eaf_msg_t* req)
{
	eaf_msg_full_t* real_msg = EAF_MSG_I2C(req);
	req->from = from;
	req->to = to;

	/* 查询接收服务 */
	eaf_service_group_t* group;
	eaf_service_t* service = _eaf_service_find_service(to, &group);
	if (service == NULL)
	{
		return eaf_errno_notfound;
	}

	/* 查找消息处理函数 */
	size_t i;
	eaf_req_handle_fn msg_proc = NULL;
	for (i = 0; i < service->load->msg_table_size; i++)
	{
		if (service->load->msg_table[i].msg_id == req->id)
		{
			msg_proc = service->load->msg_table[i].fn;
			break;
		}
	}

	if (msg_proc == NULL)
	{
		return eaf_errno_notfound;
	}

	/* 推送消息 */
	return _eaf_service_push_msg(group, service, real_msg, 1, _eaf_service_on_req_record_create, msg_proc);
}

int eaf_send_rsp(uint32_t from, eaf_msg_t* rsp)
{
	eaf_msg_full_t* real_msg = EAF_MSG_I2C(rsp);
	rsp->from = from;

	/* 查询接收服务 */
	eaf_service_group_t* group;
	eaf_service_t* service = _eaf_service_find_service(rsp->to, &group);
	if (service == NULL)
	{
		return eaf_errno_notfound;
	}

	/* 推送消息 */
	return _eaf_service_push_msg(group, service, real_msg, 1, NULL, NULL);
}

int eaf_send_evt(uint32_t from, eaf_msg_t* evt)
{
	eaf_msg_full_t* real_msg = EAF_MSG_I2C(evt);
	evt->from = from;

	eaf_subscribe_record_t tmp_key;
	tmp_key.data.evt_id = evt->id;
	tmp_key.data.service = NULL;
	tmp_key.data.proc = NULL;
	tmp_key.data.priv = NULL;

	size_t i;
	for (i = 0; i < g_eaf_ctx->group.size; i++)
	{
		eaf_mutex_enter(&g_eaf_ctx->group.table[i]->objlock);
		do 
		{
			eaf_map_node_t* it = eaf_map_find_upper(&g_eaf_ctx->group.table[i]->subscribe.table, &tmp_key.node);
			for (; it != NULL; it = eaf_map_next(&g_eaf_ctx->group.table[i]->subscribe.table, it))
			{
				eaf_subscribe_record_t* record = EAF_CONTAINER_OF(it, eaf_subscribe_record_t, node);
				if (record->data.evt_id != evt->id)
				{
					break;
				}
				_eaf_service_push_msg(g_eaf_ctx->group.table[i], record->data.service, real_msg, 0, NULL, NULL);
			}
		} while (0);
		eaf_mutex_leave(&g_eaf_ctx->group.table[i]->objlock);
	}

	return eaf_errno_success;
}

eaf_jmp_buf_t* eaf_service_get_jmpbuf(void)
{
	return &_eaf_get_current_service(NULL)->filber.jmpbuf;
}

void eaf_filber_context_switch(void)
{
	eaf_service_group_t* group = _eaf_get_current_group();

	longjmp(group->filber.jmpbuf.env, JMP_STATE_SWITCH);
}

void eaf_filber_context_return(void)
{
	eaf_service_group_t* group = _eaf_get_current_group();

	longjmp(group->filber.jmpbuf.env, JMP_STATE_RETURN);
}

int eaf_resume(uint32_t srv_id)
{
	eaf_service_group_t* group;
	eaf_service_t* service = _eaf_service_find_service(srv_id, &group);
	if (service == NULL)
	{
		return eaf_errno_notfound;
	}

	int ret = eaf_errno_success;
	eaf_mutex_enter(&group->objlock);
	do 
	{
		switch (service->state)
		{
		case eaf_service_state_pend:
			_eaf_service_set_state_nolock(group, service, eaf_service_state_busy);
			break;

		case eaf_service_state_init1:
			_eaf_service_set_state_nolock(group, service, eaf_service_state_init0);
			break;

		default:
			ret = eaf_errno_state;
			break;
		}
	} while (0);
	eaf_mutex_leave(&group->objlock);

	eaf_sem_post(&group->msgq.sem);

	return ret;
}

eaf_jmp_buf_t* eaf_stack_calculate_jmpbuf(void* addr, size_t size)
{
	return (eaf_jmp_buf_t*)((char*)addr + size - sizeof(jmp_buf));
}
