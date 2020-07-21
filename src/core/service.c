#include <assert.h>
#include <limits.h>
#include "eaf/core/service.h"
#include "eaf/utils/list.h"
#include "eaf/utils/errno.h"
#include "eaf/utils/define.h"
#include "eaf/utils/map.h"
#include "compat/lock.h"
#include "compat/thread.h"
#include "compat/semaphore.h"
#include "utils/memory.h"
#include "message.h"

/*
 * Before Visual Studio 2015, there is a bug that a `do { } while (0)` will triger C4127 warning
 * https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4127
 */
#if defined(_MSC_VER) && _MSC_VER < 1900
#	pragma warning(disable : 4127)
#endif

#define PUSH_FLAG_LOCK			(0x01 << 0x00)
#define PUSH_FLAG_FORCE			(0x01 << 0x01)
#define HAS_FLAG(flag, bit)		((flag) & (bit))

/**
 * @brief Compare template
 * @param[in] a		value a
 * @param[in] b		value b
 */
#define COMPARE_TEMPLATE(a, b)	\
	do {\
		if ((a) < (b)) {\
			return -1;\
		} else if ((a) > (b)) {\
			return 1;\
		}\
	} while (0)

typedef enum eaf_ctx_state
{
	eaf_ctx_state_init,							/**< 初始状态 */
	eaf_ctx_state_busy,							/**< 运行状态 */
	eaf_ctx_state_exit,							/**< 退出状态 */
}eaf_ctx_state_t;

typedef struct eaf_msgq_record
{
	eaf_list_node_t					node;		/**< 侵入式节点 */

	union
	{
		struct
		{
			eaf_msg_handle_fn		req_fn;		/**< 请求处理函数 */
		}req;
	}info;

	struct
	{
		uint32_t					from;
		uint32_t					to;
		struct eaf_service*			service;	/**< 服务句柄 */
		eaf_msg_full_t*				msg;		/**< 消息 */
	}data;
}eaf_msgq_record_t;

typedef struct eaf_service
{
	const eaf_entrypoint_t*			entry;		/**< 加载信息 */

	struct
	{
		eaf_service_local_t			local;		/**< Service Local Information */
		eaf_list_node_t				node;		/**< 侵入式节点。在ready_list或wait_list中 */
	}runtime;

	struct
	{
		eaf_msgq_record_t*			cur_msg;	/**< 正在处理的消息 */
		eaf_list_t					queue;		/**< 缓存的消息 */
		size_t						capacity;	/**< 消息队列容量 */
	}msgq;
}eaf_service_t;

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4200)
#endif
typedef struct eaf_group
{
	eaf_compat_lock_t				objlock;	/**< 线程锁 */
	eaf_compat_thread_t				working;	/**< 承载线程 */
	size_t							index;		/**< Group index */

	struct 
	{
		eaf_group_local_t			local;		/**< local storage */
		eaf_service_t*				cur_run;	/**< 当前正在处理的服务 */
		eaf_list_t					busy_list;	/**< INIT/BUSY */
		eaf_list_t					wait_list;	/**< INIT_YIELD/IDLE/PEND */
	}coroutine;

	struct
	{
		eaf_compat_sem_t			sem;		/**< 信号量 */
	}msgq;

	struct
	{
		size_t						size;		/**< 服务表长度 */
		eaf_service_t				table[];	/**< 服务表 */
	}service;
}eaf_group_t;
#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

typedef struct eaf_ctx
{
	eaf_ctx_state_t					state;		/**< 状态 */
	eaf_compat_sem_t				ready;		/**< 退出信号 */
	eaf_thread_storage_t			tls;		/**< 线程私有变量 */

	struct
	{
		unsigned					init_failure : 1;
	}mask;

	struct
	{
		size_t						size;		/**< 服务组长度 */
		eaf_group_t**				table;		/**< 服务组 */
	}group;

	const eaf_hook_t*				hook;		/**< Hook */
}eaf_ctx_t;

static eaf_ctx_t* g_eaf_ctx			= NULL;		/**< Global runtime */

static int _eaf_hook_message_handle_before(eaf_msgq_record_t* msg)
{
	if (g_eaf_ctx->hook == NULL || g_eaf_ctx->hook->on_message_handle_before == NULL)
	{
		return 0;
	}

	int ret;
	if ((ret = g_eaf_ctx->hook->on_message_handle_before(msg->data.from,
		msg->data.to, &msg->data.msg->msg)) == 0)
	{
		return 0;
	}

	return ret;
}

static void _eaf_hook_message_handle_after(eaf_msgq_record_t* msg)
{
	if (g_eaf_ctx->hook == NULL || g_eaf_ctx->hook->on_message_handle_after == NULL)
	{
		return;
	}

	g_eaf_ctx->hook->on_message_handle_after(msg->data.from, msg->data.to, &msg->data.msg->msg);
}

static void _eaf_hook_service_yield(uint32_t service)
{
	if (g_eaf_ctx->hook == NULL || g_eaf_ctx->hook->on_service_yield == NULL)
	{
		return;
	}
	g_eaf_ctx->hook->on_service_yield(service);
}

static void _eaf_hook_service_resume(uint32_t service)
{
	if (g_eaf_ctx->hook == NULL || g_eaf_ctx->hook->on_service_resume == NULL)
	{
		return;
	}
	g_eaf_ctx->hook->on_service_resume(service);
}

static int _eaf_hook_service_register(uint32_t id)
{
	if (g_eaf_ctx->hook == NULL || g_eaf_ctx->hook->on_service_register == NULL)
	{
		return 0;
	}

	return g_eaf_ctx->hook->on_service_register(id);
}

static void _eaf_hook_cleanup_before(void)
{
	if (g_eaf_ctx->hook == NULL || g_eaf_ctx->hook->on_exit_before == NULL)
	{
		return;
	}
	g_eaf_ctx->hook->on_exit_before();
}

static void _eaf_hook_cleanup_after(const eaf_hook_t* hook)
{
	if (hook == NULL || hook->on_exit_after == NULL)
	{
		return;
	}
	hook->on_exit_after();
}

static void _eaf_hook_service_init_before(uint32_t id)
{
	if (g_eaf_ctx->hook != NULL && g_eaf_ctx->hook->on_service_init_before != NULL)
	{
		g_eaf_ctx->hook->on_service_init_before(id);
	}
}

static void _eaf_hook_service_init_after(uint32_t id, int ret)
{
	if (g_eaf_ctx->hook != NULL && g_eaf_ctx->hook->on_service_init_after != NULL)
	{
		g_eaf_ctx->hook->on_service_init_after(id, ret);
	}
}

static void _eaf_hook_service_exit_before(uint32_t id)
{
	if (g_eaf_ctx->hook != NULL && g_eaf_ctx->hook->on_service_exit_before != NULL)
	{
		g_eaf_ctx->hook->on_service_exit_before(id);
	}
}

static void _eaf_hook_service_exit_after(uint32_t id)
{
	if (g_eaf_ctx->hook != NULL && g_eaf_ctx->hook->on_service_exit_after != NULL)
	{
		g_eaf_ctx->hook->on_service_exit_after(id);
	}
}

static int _eaf_hook_message_send_before(uint32_t from, uint32_t to, struct eaf_msg* msg)
{
	if (g_eaf_ctx->hook == NULL || g_eaf_ctx->hook->on_message_send_before == NULL)
	{
		return 0;
	}
	return g_eaf_ctx->hook->on_message_send_before(from, to, msg);
}

static void _eaf_hook_message_send_after(uint32_t from, uint32_t to, eaf_msg_t* msg, int ret)
{
	if (g_eaf_ctx->hook == NULL || g_eaf_ctx->hook->on_message_send_after == NULL)
	{
		return;
	}
	g_eaf_ctx->hook->on_message_send_after(from, to, msg, ret);
}

static void _eaf_hook_load_before(void)
{
	if (g_eaf_ctx->hook == NULL || g_eaf_ctx->hook->on_load_before == NULL)
	{
		return;
	}
	g_eaf_ctx->hook->on_load_before();
}

static void _eaf_hook_load_after(int ret)
{
	if (g_eaf_ctx->hook == NULL || g_eaf_ctx->hook->on_load_after == NULL)
	{
		return;
	}
	g_eaf_ctx->hook->on_load_after(ret);
}

static eaf_service_t* _eaf_service_find_service(uint32_t service_id, eaf_group_t** group)
{
	size_t i;
	for (i = 0; i < g_eaf_ctx->group.size; i++)
	{
		size_t j;
		for (j = 0; j < g_eaf_ctx->group.table[i]->service.size; j++)
		{
			eaf_service_t* service = &g_eaf_ctx->group.table[i]->service.table[j];
			if (service->runtime.local.id != service_id)
			{
				continue;
			}

			if (group != NULL)
			{
				*group = g_eaf_ctx->group.table[i];
			}
			return service->entry != NULL ? service : NULL;
		}
	}

	return NULL;
}

static void _eaf_service_destroy_msg_record(eaf_msgq_record_t* record)
{
	eaf_msg_dec_ref(EAF_MSG_C2I(record->data.msg));
	EAF_FREE(record);
}

static eaf_service_t* _eaf_get_first_busy_service_lock(eaf_group_t* group)
{
	eaf_service_t* service;
	eaf_compat_lock_enter(&group->objlock);
	{
		eaf_list_node_t* it = eaf_list_begin(&group->coroutine.busy_list);
		service = it != NULL ? EAF_CONTAINER_OF(it, eaf_service_t, runtime.node) : NULL;
	}
	eaf_compat_lock_leave(&group->objlock);

	return service;
}

/**
 * @brief Set service state
 * @param[in,out] group		Service group
 * @param[in,out] service	Service
 * @param[in] state		State
 */
static void _eaf_service_set_state_nolock(eaf_group_t* group,
	eaf_service_t* service, eaf_service_state_t state)
{
	switch (service->runtime.local.state)
	{
	case eaf_service_state_init_yield:
	case eaf_service_state_idle:
	case eaf_service_state_yield:
	case eaf_service_state_exit:
		eaf_list_erase(&group->coroutine.wait_list, &service->runtime.node);
		break;

	case eaf_service_state_init:
	case eaf_service_state_busy:
		eaf_list_erase(&group->coroutine.busy_list, &service->runtime.node);
		break;
	}

	switch (state)
	{
	case eaf_service_state_init_yield:
	case eaf_service_state_idle:
	case eaf_service_state_yield:
	case eaf_service_state_exit:
		eaf_list_push_back(&group->coroutine.wait_list, &service->runtime.node);
		break;

	case eaf_service_state_init:
	case eaf_service_state_busy:
		eaf_list_push_back(&group->coroutine.busy_list, &service->runtime.node);
		break;
	}
	service->runtime.local.state = state;
}

static void _eaf_service_set_state_lock(eaf_group_t* group, eaf_service_t* service, eaf_service_state_t state)
{
	eaf_compat_lock_enter(&group->objlock);
	{
		_eaf_service_set_state_nolock(group, service, state);
	}
	eaf_compat_lock_leave(&group->objlock);
}

/**
 * @brief Reset yield branch
 */
static void _eaf_service_reset_yield_branch(eaf_service_t* service)
{
	service->runtime.local.branch = 0;
}

static void _eaf_service_reset(eaf_service_t* service)
{
	_eaf_service_reset_yield_branch(service);
	_eaf_service_destroy_msg_record(service->msgq.cur_msg);
	service->msgq.cur_msg = NULL;
}

/**
 * @brief Get current running service
 */
static eaf_service_t* _eaf_group_get_cur_run(eaf_group_t* group)
{
	return group->coroutine.cur_run;
}

/**
 * @brief Set current running service
 */
static void _eaf_group_set_cur_run(eaf_group_t* group, eaf_service_t* service)
{
	group->coroutine.cur_run = service;
}

/**
 * @brief Call user defined yield hook
 */
static void _eaf_group_call_yield_hook(eaf_group_t* group)
{
	if (group->coroutine.local.yield.hook == NULL)
	{
		return;
	}

	eaf_service_t* service = _eaf_group_get_cur_run(group);
	group->coroutine.local.yield.hook(&service->runtime.local, group->coroutine.local.yield.arg);
}

/**
 * @brief Check control bit
 */
static int _eaf_group_check_cc0(eaf_group_t* group, uint32_t mask)
{
	return group->coroutine.local.cc[0] & mask;
}

static void _eaf_service_resume_message(eaf_group_t* group, eaf_service_t* service)
{
	eaf_msg_handle_fn rsp_fn;
	eaf_msgq_record_t* msg = service->msgq.cur_msg;

	switch (eaf_msg_get_type(&msg->data.msg->msg))
	{
	case eaf_msg_type_req:
		msg->info.req.req_fn(msg->data.from, msg->data.to, EAF_MSG_C2I(msg->data.msg));
		break;

	case eaf_msg_type_rsp:
		rsp_fn = eaf_msg_get_rsp_fn(&msg->data.msg->msg);
		if (rsp_fn != NULL)
		{
			rsp_fn(msg->data.from, msg->data.to, EAF_MSG_C2I(msg->data.msg));
		}
		break;
	}

	/* yield */
	if (_eaf_group_check_cc0(group, EAF_SERVICE_CC0_YIELD))
	{
		_eaf_service_set_state_lock(group, service, eaf_service_state_yield);
		_eaf_group_call_yield_hook(group);
		_eaf_hook_service_yield(service->runtime.local.id);
		return;
	}

	_eaf_hook_message_handle_after(msg);

	/* normal end, reset branch */
	_eaf_service_reset(service);
}

static void _eaf_handle_new_message(eaf_group_t* group, eaf_service_t* service)
{
	/* the state of service must be BUSY */
	assert(service->runtime.local.state == eaf_service_state_busy);

	/* If cur_msg is not NULL, context need to be restore */
	if (service->msgq.cur_msg != NULL)
	{
		_eaf_service_resume_message(group, service);
		return;
	}

	/* 取出消息。若消息队列为空，则将服务置于IDLE状态 */
	eaf_compat_lock_enter(&group->objlock);
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
	eaf_compat_lock_leave(&group->objlock);

	if (service->msgq.cur_msg == NULL)
	{
		return;
	}

	/* hook: on_pre_msg_process */
	if (_eaf_hook_message_handle_before(service->msgq.cur_msg) < 0)
	{
		_eaf_service_reset(service);
		return;
	}

	_eaf_service_resume_message(group, service);
}

static void _eaf_group_finish_service_init_lock(eaf_group_t* group, eaf_service_t* service)
{
	eaf_compat_lock_enter(&group->objlock);
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
	eaf_compat_lock_leave(&group->objlock);
}

/**
 * 继续进行初始化
 */
static int _eaf_service_resume_init(eaf_group_t* group, eaf_service_t* service)
{
	/* No need to call #_eaf_hook_service_init_before() because it's alyready done */
	int ret = service->entry->on_init();

	/* 检查是否执行yield */
	if (_eaf_group_check_cc0(group, EAF_SERVICE_CC0_YIELD))
	{/* 若init阶段进行了yield */
		_eaf_service_set_state_lock(group, service, eaf_service_state_init_yield);
		_eaf_group_call_yield_hook(group);
		return 0;
	}
	_eaf_service_reset_yield_branch(service);
	_eaf_hook_service_init_after(service->runtime.local.id, ret);

	/* 初始化失败时返回错误 */
	if (ret < 0)
	{
		return -1;
	}

	/* 标记完成初始化 */
	_eaf_group_finish_service_init_lock(group, service);
	return 0;
}

/**
 * @brief Clear control bits
 */
static void _eaf_group_clear_cc0(eaf_group_t* group)
{
	group->coroutine.local.cc[0] = 0;
}

static int _eaf_service_thread_loop(eaf_group_t* group)
{
	eaf_service_t* service = _eaf_get_first_busy_service_lock(group);
	if (service == NULL)
	{
		eaf_compat_sem_pend(&group->msgq.sem, (unsigned)-1);
		return 0;
	}
	eaf_compat_sem_pend(&group->msgq.sem, 0);

	_eaf_group_set_cur_run(group, service);
	_eaf_group_clear_cc0(group);

	if (service->runtime.local.state == eaf_service_state_init)
	{
		return _eaf_service_resume_init(group, service);
	}

	_eaf_handle_new_message(group, service);
	return 0;
}

/**
 * @brief 获取当前线程对应的服务组
 * @return		服务组
 */
static eaf_group_t* _eaf_get_current_group(void)
{
	return eaf_thread_storage_get(&g_eaf_ctx->tls);
}

/**
 * Init this service group.
 * @param [in] group	Service Group
 * @param [out] idx		The index if failure
 * @return				The number of service init, or -1 if failed
 */
static int _eaf_group_init(eaf_group_t* group, size_t* idx)
{
	int counter = 0;
	for (*idx = 0; *idx < group->service.size; *idx += 1)
	{
		eaf_service_t* service = &group->service.table[*idx];
		_eaf_group_set_cur_run(group, service);

		/*
		 * If service is not available, remove it from ready list and set state
		 * to #eaf_service_state_exit.
		 */
		if (service->entry == NULL || service->entry->on_init == NULL)
		{
			service->runtime.local.state = eaf_service_state_exit;
			eaf_list_erase(&group->coroutine.busy_list, &service->runtime.node);

			continue;
		}

		_eaf_group_clear_cc0(group);

		_eaf_hook_service_init_before(service->runtime.local.id);
		int ret = service->entry->on_init();

		/* 检查是否执行yield */
		if (_eaf_group_check_cc0(group, EAF_SERVICE_CC0_YIELD))
		{/* 若init阶段进行了yield */
			_eaf_service_set_state_lock(group, service, eaf_service_state_init_yield);

			/* call user hook */
			_eaf_group_call_yield_hook(group);

			/*
			 * need to consider it as init success,
			 * because if no service finish init process here, this thread will be exited.
			 */
			counter++;

			continue;
		}
		_eaf_service_reset_yield_branch(service);
		_eaf_hook_service_init_after(service->runtime.local.id, ret);

		/* 若初始化失败则返回错误 */
		if (ret < 0)
		{
			return -1;
		}

		_eaf_group_finish_service_init_lock(group, service);
		counter++;
	}

	return counter;
}

/**
 * @brief Exit group in reverse
 * @param[in] group		Group
 * @param[in] max_idx	The max idx. This index it self should not be exit
 */
static void _eaf_group_exit(eaf_group_t* group, size_t max_idx)
{
	assert(max_idx < INT_MAX);

	int i;
	for (i = (int)max_idx - 1; i >= 0; i--)
	{
		eaf_service_t* service = &group->service.table[i];
		if (service->runtime.local.state == eaf_service_state_exit)
		{
			continue;
		}

		_eaf_group_set_cur_run(group, service);
		_eaf_service_set_state_lock(group, service, eaf_service_state_exit);

		if (service->entry != NULL && service->entry->on_exit != NULL)
		{
			_eaf_hook_service_exit_before(service->runtime.local.id);
			service->entry->on_exit();
			_eaf_hook_service_exit_after(service->runtime.local.id);
		}
	}
}

/**
* 工作线程
* @param arg	eaf_service_group_t
*/
static void _eaf_service_thread(void* arg)
{
	size_t init_idx = 0;
	eaf_group_t* group = arg;

	/* 设置线程私有变量 */
	if (eaf_thread_storage_set(&g_eaf_ctx->tls, arg) < 0)
	{
		return;
	}

	/* 等待就绪 */
	while (EAF_ACCESS(eaf_ctx_state_t, g_eaf_ctx->state) == eaf_ctx_state_init)
	{
		eaf_compat_sem_pend(&group->msgq.sem, (unsigned long)-1);
	}
	if (g_eaf_ctx->state != eaf_ctx_state_busy)
	{
		return;
	}

	/* 进行初始化 */
	int init_count = _eaf_group_init(group, &init_idx);

	/* Set failure flag if initialize failed */
	if (init_count < 0)
	{
		g_eaf_ctx->mask.init_failure = 1;
	}

	/* 通告初始化完毕 */
	eaf_compat_sem_post(&g_eaf_ctx->ready);

	/* 失败时清理 */
	if (init_count <= 0)
	{
		goto cleanup;
	}

	/* 事件循环 */
	while (g_eaf_ctx->state == eaf_ctx_state_busy
		&& _eaf_service_thread_loop(group) == 0)
	{
	}

cleanup:
	_eaf_group_exit(group, init_idx);
}

static void _eaf_cleanup_service(eaf_service_t* service)
{
	eaf_list_node_t* it;
	while ((it = eaf_list_pop_front(&service->msgq.queue)) != NULL)
	{
		eaf_msgq_record_t* msg = EAF_CONTAINER_OF(it, eaf_msgq_record_t, node);
		_eaf_service_destroy_msg_record(msg);
	}

	if (service->msgq.cur_msg != NULL)
	{
		_eaf_service_destroy_msg_record(service->msgq.cur_msg);
		service->msgq.cur_msg = NULL;
	}
}

static void _eaf_cleanup_group(eaf_group_t* group)
{
	/* 向队列推送以保证线程感知到状态改变 */
	eaf_compat_sem_post(&group->msgq.sem);

	/* 等待线程退出 */
	eaf_compat_thread_exit(&group->working);

	size_t i;
	for (i = 0; i < group->service.size; i++)
	{
		_eaf_cleanup_service(&group->service.table[i]);
	}

	/* 清理资源 */
	eaf_compat_lock_exit(&group->objlock);
	eaf_compat_sem_exit(&group->msgq.sem);
}

static int _eaf_service_push_msg(eaf_group_t* group, eaf_service_t* service, eaf_msg_full_t* msg, uint32_t from, uint32_t to,
	void(*on_create)(eaf_msgq_record_t* record, void* arg), void* arg, int flag)
{
	/* 检查消息队列容量 */
	if (!HAS_FLAG(flag, PUSH_FLAG_FORCE) &&
		eaf_list_size(&service->msgq.queue) >= service->msgq.capacity)
	{
		return eaf_errno_overflow;
	}

	eaf_msgq_record_t* record = EAF_MALLOC(sizeof(eaf_msgq_record_t));
	if (record == NULL)
	{
		return eaf_errno_memory;
	}

	record->data.from = from;
	record->data.to = to;
	record->data.service = service;
	record->data.msg = msg;
	eaf_msg_add_ref(EAF_MSG_C2I(msg));

	if (on_create != NULL)
	{
		on_create(record, arg);
	}

	if (HAS_FLAG(flag, PUSH_FLAG_LOCK)) { eaf_compat_lock_enter(&group->objlock); }
	do
	{
		if (service->runtime.local.state == eaf_service_state_idle)
		{
			_eaf_service_set_state_nolock(group, service, eaf_service_state_busy);
		}
		eaf_list_push_back(&service->msgq.queue, &record->node);
	} while (0);
	if (HAS_FLAG(flag, PUSH_FLAG_LOCK)) { eaf_compat_lock_leave(&group->objlock); }

	eaf_compat_sem_post(&group->msgq.sem);
	return eaf_errno_success;
}

static void _eaf_service_on_req_fix(eaf_msgq_record_t* record, void* arg)
{
#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4055)
#endif
	record->info.req.req_fn = (eaf_msg_handle_fn)arg;
#if defined(_MSC_VER)
#	pragma warning(pop)
#endif
}

static eaf_service_t* _eaf_get_current_service(eaf_group_t** group)
{
	eaf_group_t* ret = _eaf_get_current_group();

	if (group != NULL)
	{
		*group = ret;
	}
	return ret != NULL ? _eaf_group_get_cur_run(ret) : NULL;
}

static int _eaf_send_req(uint32_t from, uint32_t to, eaf_msg_t* req)
{
	eaf_msg_full_t* real_msg = EAF_MSG_I2C(req);
	req->from = from;

	/* 查询接收服务 */
	eaf_group_t* group;
	eaf_service_t* service = _eaf_service_find_service(to, &group);

	/* if service not found, send to rpc */
	if (service == NULL)
	{
		return eaf_errno_notfound;
	}

	/* 查找消息处理函数 */
	size_t i;
	eaf_msg_handle_fn msg_proc = NULL;
	for (i = 0; i < service->entry->msg_table_size; i++)
	{
		if (service->entry->msg_table[i].msg_id == req->id)
		{
			msg_proc = service->entry->msg_table[i].fn;
			break;
		}
	}

	if (msg_proc == NULL)
	{
		return eaf_errno_notfound;
	}

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4054)
#endif
	/* 推送消息 */
	return _eaf_service_push_msg(group, service, real_msg, from, to,
		_eaf_service_on_req_fix, (void*)msg_proc, PUSH_FLAG_LOCK);
#if defined(_MSC_VER)
#	pragma warning(pop)
#endif
}

/**
 * Send response
 * @note rsp->from shows who send this response
 * @param[in] to	Who will receive this response
 * @param[in] rsp	Response message
 */
static int _eaf_send_rsp(uint32_t from, uint32_t to, eaf_msg_t* rsp)
{
	eaf_msg_full_t* real_msg = EAF_MSG_I2C(rsp);

	/* find service */
	eaf_group_t* group;
	eaf_service_t* service = _eaf_service_find_service(to, &group);

	/* report dst_not_found */
	if (service == NULL)
	{
		return eaf_errno_notfound;
	}

	/* 推送消息 */
	return _eaf_service_push_msg(group, service, real_msg, from, to,
		NULL, NULL, PUSH_FLAG_LOCK | PUSH_FLAG_FORCE);
}

int eaf_init(_In_ const eaf_group_table_t* info, _In_ size_t size)
{
	size_t i;
	if (g_eaf_ctx != NULL)
	{
		return eaf_errno_duplicate;
	}

	/* calculate memory size */
	size_t malloc_size = sizeof(eaf_ctx_t) + sizeof(eaf_group_t*) * size;
	for (i = 0; i < size; i++)
	{
		malloc_size += sizeof(eaf_group_t) + sizeof(eaf_service_t) * info[i].service.size;
	}

	g_eaf_ctx = EAF_CALLOC(1, malloc_size);
	if (g_eaf_ctx == NULL)
	{
		return eaf_errno_memory;
	}
	g_eaf_ctx->state = eaf_ctx_state_init;
	if (eaf_compat_sem_init(&g_eaf_ctx->ready, 0) < 0)
	{
		EAF_FREE(g_eaf_ctx);
		g_eaf_ctx = NULL;
		return eaf_errno_unknown;
	}
	if (eaf_thread_storage_init(&g_eaf_ctx->tls) < 0)
	{
		eaf_compat_sem_exit(&g_eaf_ctx->ready);
		EAF_FREE(g_eaf_ctx);
		g_eaf_ctx = NULL;
		return eaf_errno_unknown;
	}

	/* fix address */
	g_eaf_ctx->group.size = size;
	g_eaf_ctx->group.table = (eaf_group_t**)(g_eaf_ctx + 1);
	g_eaf_ctx->group.table[0] = (eaf_group_t*)((char*)g_eaf_ctx->group.table + sizeof(eaf_group_t*) * size);
	for (i = 1; i < size; i++)
	{
		g_eaf_ctx->group.table[i] = (eaf_group_t*)
			((char*)g_eaf_ctx->group.table[i - 1] + sizeof(eaf_group_t) + sizeof(eaf_service_t) * info[i - 1].service.size);
	}

	/* initialize resource */
	size_t init_idx;	// Initialize index
	for (init_idx = 0; init_idx < size; init_idx++)
	{
		g_eaf_ctx->group.table[init_idx]->index = init_idx;
		g_eaf_ctx->group.table[init_idx]->service.size = info[init_idx].service.size;
		eaf_list_init(&g_eaf_ctx->group.table[init_idx]->coroutine.busy_list);
		eaf_list_init(&g_eaf_ctx->group.table[init_idx]->coroutine.wait_list);
		if (eaf_compat_lock_init(&g_eaf_ctx->group.table[init_idx]->objlock, eaf_lock_attr_normal) < 0)
		{
			goto err;
		}
		if (eaf_compat_sem_init(&g_eaf_ctx->group.table[init_idx]->msgq.sem, 0) < 0)
		{
			eaf_compat_lock_exit(&g_eaf_ctx->group.table[init_idx]->objlock);
			goto err;
		}

		if (eaf_compat_thread_init(&g_eaf_ctx->group.table[init_idx]->working, &info[init_idx].attr,
			_eaf_service_thread, g_eaf_ctx->group.table[init_idx]) < 0)
		{
			eaf_compat_lock_exit(&g_eaf_ctx->group.table[init_idx]->objlock);
			eaf_compat_sem_exit(&g_eaf_ctx->group.table[init_idx]->msgq.sem);
			goto err;
		}

		size_t idx;
		for (idx = 0; idx < info[init_idx].service.size; idx++)
		{
			g_eaf_ctx->group.table[init_idx]->service.table[idx].runtime.local.id =
				info[init_idx].service.table[idx].srv_id;
			g_eaf_ctx->group.table[init_idx]->service.table[idx].msgq.capacity =
				info[init_idx].service.table[idx].msgq_size;
			eaf_list_init(&g_eaf_ctx->group.table[init_idx]->service.table[idx].msgq.queue);

			/* by default, service should in init0 state */
			g_eaf_ctx->group.table[init_idx]->service.table[idx].runtime.local.state = eaf_service_state_init;
			eaf_list_push_back(&g_eaf_ctx->group.table[init_idx]->coroutine.busy_list,
				&g_eaf_ctx->group.table[init_idx]->service.table[idx].runtime.node);
		}
	}

	return eaf_errno_success;

err:
	for (i = 0; i < init_idx; i++)
	{
		eaf_compat_sem_post(&g_eaf_ctx->group.table[init_idx]->msgq.sem);

		eaf_compat_thread_exit(&g_eaf_ctx->group.table[init_idx]->working);
		eaf_compat_sem_exit(&g_eaf_ctx->group.table[init_idx]->msgq.sem);
		eaf_compat_lock_exit(&g_eaf_ctx->group.table[init_idx]->objlock);
	}

	eaf_thread_storage_exit(&g_eaf_ctx->tls);
	eaf_compat_sem_exit(&g_eaf_ctx->ready);

	EAF_FREE(g_eaf_ctx);
	g_eaf_ctx = NULL;

	return eaf_errno_unknown;
}

int eaf_load(void)
{
	size_t i;
	if (g_eaf_ctx == NULL || g_eaf_ctx->state != eaf_ctx_state_init)
	{
		return eaf_errno_state;
	}

	_eaf_hook_load_before();

	/* start all thread */
	g_eaf_ctx->state = eaf_ctx_state_busy;
	for (i = 0; i < g_eaf_ctx->group.size; i++)
	{
		eaf_compat_sem_post(&g_eaf_ctx->group.table[i]->msgq.sem);
	}

	/* wait for all thread to be ready */
	for (i = 0; i < g_eaf_ctx->group.size; i++)
	{
		eaf_compat_sem_pend(&g_eaf_ctx->ready, EAF_COMPAT_SEM_INFINITY);
	}

	int ret = g_eaf_ctx->mask.init_failure ? eaf_errno_state : eaf_errno_success;
	_eaf_hook_load_after(ret);

	return ret;
}

int eaf_exit(void)
{
	size_t i;
	if (g_eaf_ctx == NULL)
	{
		return eaf_errno_state;
	}

	_eaf_hook_cleanup_before();

	/* change state */
	g_eaf_ctx->state = eaf_ctx_state_exit;

	/* exit thread */
	for (i = 0; i < g_eaf_ctx->group.size; i++)
	{
		_eaf_cleanup_group(g_eaf_ctx->group.table[i]);
	}

	eaf_compat_sem_exit(&g_eaf_ctx->ready);
	eaf_thread_storage_exit(&g_eaf_ctx->tls);

	const eaf_hook_t* hook = g_eaf_ctx->hook;

	/* resource cleanup */
	EAF_FREE(g_eaf_ctx);
	g_eaf_ctx = NULL;

	_eaf_hook_cleanup_after(hook);
	return eaf_errno_success;
}

int eaf_register(_In_ uint32_t id, _In_ const eaf_entrypoint_t* entry)
{
	if (g_eaf_ctx == NULL || g_eaf_ctx->state != eaf_ctx_state_init)
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
			if (g_eaf_ctx->group.table[i]->service.table[idx].runtime.local.id == id)
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

	if (service->entry != NULL)
	{
		return eaf_errno_duplicate;
	}

	int ret;
	if ((ret = _eaf_hook_service_register(id)) < 0)
	{
		return ret;
	}

	service->entry = entry;
	return eaf_errno_success;
}

int eaf_send_req(_In_ uint32_t from, _In_ uint32_t to, _Inout_ eaf_msg_t* req)
{
	int ret;
	if (g_eaf_ctx == NULL || g_eaf_ctx->state != eaf_ctx_state_busy)
	{
		return eaf_errno_state;
	}

	/* hook callback */
	if ((ret = _eaf_hook_message_send_before(from, to, req)) < 0)
	{
		return ret;
	}

	ret = _eaf_send_req(from, to, req);
	_eaf_hook_message_send_after(from, to, req, ret);

	return ret;
}

int eaf_send_rsp(_In_ uint32_t from, _In_ uint32_t to, _Inout_ eaf_msg_t* rsp)
{
	int ret;
	if (g_eaf_ctx == NULL || g_eaf_ctx->state != eaf_ctx_state_busy)
	{
		return eaf_errno_state;
	}

	/* hook callback */
	if ((ret = _eaf_hook_message_send_before(from, to, rsp)) < 0)
	{
		return ret;
	}

	ret = _eaf_send_rsp(from, to, rsp);
	_eaf_hook_message_send_after(from, to, rsp, ret);

	return ret;
}

int eaf_resume(_In_ uint32_t srv_id)
{
	eaf_group_t* group;
	eaf_service_t* service = _eaf_service_find_service(srv_id, &group);
	if (service == NULL)
	{
		return eaf_errno_notfound;
	}

	_eaf_hook_service_resume(srv_id);

	int ret = eaf_errno_success;
	eaf_compat_lock_enter(&group->objlock);
	do 
	{
		switch (service->runtime.local.state)
		{
		case eaf_service_state_yield:
			_eaf_service_set_state_nolock(group, service, eaf_service_state_busy);
			break;

		case eaf_service_state_init_yield:
			_eaf_service_set_state_nolock(group, service, eaf_service_state_init);
			break;

		default:
			ret = eaf_errno_state;
			break;
		}
	} while (0);
	eaf_compat_lock_leave(&group->objlock);

	eaf_compat_sem_post(&group->msgq.sem);

	return ret;
}

eaf_service_local_t* eaf_service_get_local(_Outptr_opt_result_maybenull_ eaf_group_local_t** local)
{
	if (g_eaf_ctx == NULL)
	{
		goto err;
	}

	eaf_group_t* group;
	eaf_service_t* service = _eaf_get_current_service(&group);
	if (service == NULL)
	{
		goto err;
	}

	if (local != NULL)
	{
		*local = &group->coroutine.local;
	}

	return &service->runtime.local;

err:
	if (local != NULL)
	{
		*local = NULL;
	}
	return NULL;
}

uint32_t eaf_service_self(void)
{
	eaf_service_local_t* local = eaf_service_get_local(NULL);
	return local != NULL ? local->id : (uint32_t)-1;
}

int eaf_inject(_In_ const eaf_hook_t* hook, _In_ size_t size)
{
	if (g_eaf_ctx == NULL)
	{
		return eaf_errno_state;
	}
	if (size != sizeof(*hook))
	{
		return eaf_errno_invalid;
	}
	if (g_eaf_ctx->hook != NULL)
	{
		return eaf_errno_duplicate;
	}

	g_eaf_ctx->hook = hook;
	return eaf_errno_success;
}

EAF_API int eaf_uninject(_In_ const eaf_hook_t* hook)
{
	if (g_eaf_ctx == NULL)
	{
		return eaf_errno_state;
	}
	if (g_eaf_ctx->hook != hook)
	{
		return eaf_errno_invalid;
	}

	g_eaf_ctx->hook = NULL;
	return eaf_errno_success;
}

EAF_API eaf_group_local_t* eaf_group_begin(void)
{
	return &g_eaf_ctx->group.table[0]->coroutine.local;
}

EAF_API eaf_group_local_t* eaf_group_next(eaf_group_local_t* gls)
{
	eaf_group_t* group = EAF_CONTAINER_OF(gls, eaf_group_t, coroutine.local);
	size_t index = group->index;

	if (index >= (g_eaf_ctx->group.size - 1))
	{
		return NULL;
	}

	return &g_eaf_ctx->group.table[index + 1]->coroutine.local;
}

EAF_API eaf_service_local_t* eaf_service_begin(eaf_group_local_t* gls)
{
	eaf_group_t* group = EAF_CONTAINER_OF(gls, eaf_group_t, coroutine.local);
	eaf_service_t* service = &group->service.table[0];
	return &service->runtime.local;
}

EAF_API eaf_service_local_t* eaf_service_next(eaf_group_local_t* gls, eaf_service_local_t* sls)
{
	eaf_group_t* group = EAF_CONTAINER_OF(gls, eaf_group_t, coroutine.local);
	eaf_service_t* service = EAF_CONTAINER_OF(sls, eaf_service_t, runtime.local);

	size_t index = service - &group->service.table[0];
	if (index >= (group->service.size - 1))
	{
		return NULL;
	}
	return &group->service.table[index + 1].runtime.local;
}

EAF_API size_t eaf_message_queue_size(_In_ const eaf_service_local_t* sls)
{
	eaf_service_t* service = EAF_CONTAINER_OF(sls, eaf_service_t, runtime.local);
	return eaf_list_size(&service->msgq.queue);
}

EAF_API size_t eaf_message_queue_capacity(_In_ const eaf_service_local_t* sls)
{
	eaf_service_t* service = EAF_CONTAINER_OF(sls, eaf_service_t, runtime.local);
	return service->msgq.capacity;
}
