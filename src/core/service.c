#include <assert.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include "eaf/utils/list.h"
#include "eaf/utils/errno.h"
#include "eaf/utils/define.h"
#include "eaf/utils/map.h"
#include "compat/lock.h"
#include "compat/thread.h"
#include "compat/semaphore.h"
#include "utils/memory.h"
#include "message.h"
#include "service.h"

/*
 * Before Visual Studio 2015, there is a bug that a `do { } while (0)` will triger C4127 warning
 * https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4127
 */
#if defined(_MSC_VER) && _MSC_VER < 1900
#	pragma warning(disable : 4127)
#endif

#define PUSH_FLAG_FORCE			(0x01 << 0x00)
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

static void _eaf_hook_service_init_after(uint32_t id)
{
	if (g_eaf_ctx->hook != NULL && g_eaf_ctx->hook->on_service_init_after != NULL)
	{
		g_eaf_ctx->hook->on_service_init_after(id);
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
	case eaf_service_state_init0:
	case eaf_service_state_init2:
	case eaf_service_state_idle:
	case eaf_service_state_yield:
	case eaf_service_state_exit1:
		eaf_list_erase(&group->coroutine.wait_list, &service->runtime.node);
		break;

	case eaf_service_state_init1:
	case eaf_service_state_busy:
	case eaf_service_state_exit0:
		eaf_list_erase(&group->coroutine.busy_list, &service->runtime.node);
		break;
	}

	switch (state)
	{
	case eaf_service_state_init0:
	case eaf_service_state_init2:
	case eaf_service_state_idle:
	case eaf_service_state_yield:
	case eaf_service_state_exit1:
		eaf_list_push_back(&group->coroutine.wait_list, &service->runtime.node);
		break;

	case eaf_service_state_init1:
	case eaf_service_state_busy:
	case eaf_service_state_exit0:
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

/**
 * @brief Clear control bits
 */
static void _eaf_group_clear_cc0(eaf_group_t* group)
{
	group->coroutine.local.cc[0] = 0;
}

static void _eaf_group_finish_service_init_lock(eaf_group_t* group, eaf_service_t* service)
{
	eaf_compat_lock_enter(&group->objlock);
	do
	{
		/* Switch to IDLE */
		_eaf_service_set_state_nolock(group, service, eaf_service_state_idle);
		/* If message queue is not empty, switch state to BUSY */
		if (eaf_list_size(&service->msgq.queue) > 0)
		{
			_eaf_service_set_state_nolock(group, service, eaf_service_state_busy);
		}
	} while (0);
	eaf_compat_lock_leave(&group->objlock);
}

static void _eaf_do_service_init(eaf_group_t* group, eaf_service_t* service)
{
	_eaf_group_clear_cc0(group);
	_eaf_hook_service_init_before(service->runtime.local.id);
	service->entry->on_init();

	/**
	 * yield can not be called during init stage.
	 */
	if (_eaf_group_check_cc0(group, EAF_SERVICE_CC0_YIELD))
	{
		printf("coroutine is not support during initialize stage. service:%#10x\n", service->runtime.local.id);
		assert(0);
	}

	_eaf_service_reset_yield_branch(service);
	_eaf_hook_service_init_after(service->runtime.local.id);

	_eaf_group_finish_service_init_lock(group, service);
}

static void _eaf_handle_new_message(eaf_group_t* group, eaf_service_t* service)
{
	/* If initialize is delay, do it right now */
	if (service->runtime.local.state == eaf_service_state_init1)
	{
		_eaf_do_service_init(group, service);
	}

	/* the state of service must be BUSY */
	assert(service->runtime.local.state == eaf_service_state_busy);

	/* If cur_msg is not NULL, context need to be restore */
	if (service->msgq.cur_msg != NULL)
	{
		_eaf_service_resume_message(group, service);
		return;
	}

	/* Pop message. If the message queue is empty, the set service state to IDLE */
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

static int _eaf_service_thread_loop(eaf_group_t* group)
{
	eaf_service_t* service = _eaf_get_first_busy_service_lock(group);
	if (service == NULL)
	{
		eaf_compat_sem_pend(&group->msgq.sem, EAF_COMPAT_SEM_INFINITY);
		return 0;
	}
	eaf_compat_sem_pend(&group->msgq.sem, 0);

	_eaf_group_set_cur_run(group, service);
	_eaf_group_clear_cc0(group);

	_eaf_handle_new_message(group, service);
	return 0;
}

/**
 * @brief Get group for current thread
 * @return		Group
 */
static eaf_group_t* _eaf_get_current_group(void)
{
	return eaf_thread_storage_get(&g_eaf_ctx->tls);
}

/**
 * @brief Exit group in reverse
 * @param[in] group		Group
 * @param[in] max_idx	The max idx. This index it self should not be exit
 */
static void _eaf_group_exit(eaf_group_t* group, size_t max_idx)
{
	int i;

	if (max_idx > group->service.size)
	{
		max_idx = group->service.size;
	}

	for (i = (int)max_idx - 1; i >= 0; i--)
	{
		eaf_service_t* service = &group->service.table[i];
		if (service->runtime.local.state == eaf_service_state_exit1 || service->entry == NULL)
		{
			continue;
		}

		_eaf_group_set_cur_run(group, service);
		_eaf_service_set_state_lock(group, service, eaf_service_state_exit0);

		_eaf_hook_service_exit_before(service->runtime.local.id);
		service->entry->on_exit();
		_eaf_hook_service_exit_after(service->runtime.local.id);

		_eaf_service_set_state_lock(group, service, eaf_service_state_exit1);
	}
}

/**
 * Init this service group.
 * @param [in] group	Service Group
 * @param [out] idx		The index if failure
 * @return				The number of service init, or -1 if failed
 */
static int _eaf_group_init_necessary(eaf_group_t* group)
{
	int counter = 0;
	size_t idx = 0;

	for (idx = 0; idx < group->service.size; idx++)
	{
		eaf_service_t* service = &group->service.table[idx];
		_eaf_group_set_cur_run(group, service);

		if (service->runtime.local.state != eaf_service_state_init1)
		{
			continue;
		}

		_eaf_do_service_init(group, service);
		counter++;
	}

	return counter;
}

static void _eaf_service_teardown_hanle_message_nolock(eaf_service_t* service, eaf_msgq_record_t* msg)
{
	if (eaf_msg_get_type(&msg->data.msg->msg) == eaf_msg_type_req)
	{
		/* Send default response */
		eaf_msg_t* rsp = eaf_msg_create_rsp(&msg->data.msg->msg, 0);
		assert(rsp != NULL);

		eaf_msg_set_receipt(rsp, eaf_errno_state);
		eaf_send_rsp(service->runtime.local.id, msg->data.from, rsp);
		eaf_msg_dec_ref(rsp);
	}

	_eaf_service_destroy_msg_record(msg);
}

static void _eaf_service_teardown_cleanup_msgq(eaf_group_t* group, eaf_service_t* service)
{
	eaf_list_node_t* node;
	if (service->msgq.cur_msg != NULL)
	{
		_eaf_service_teardown_hanle_message_nolock(service, service->msgq.cur_msg);
		service->msgq.cur_msg = NULL;
	}

	eaf_compat_lock_enter(&group->objlock);
	while ((node = eaf_list_pop_front(&service->msgq.queue)) != NULL)
	{
		eaf_compat_lock_leave(&group->objlock);
		{
			_eaf_service_teardown_hanle_message_nolock(service,
					EAF_CONTAINER_OF(node, eaf_msgq_record_t, node));
		}
		eaf_compat_lock_enter(&group->objlock);
	}
	eaf_compat_lock_leave(&group->objlock);
}

static void _eaf_service_teardown(eaf_group_t* group)
{
	size_t i;
	for (i = 0; i < group->service.size; i++)
	{
		eaf_service_t* service = &group->service.table[i];
		if (service->mask.alive || service->entry == NULL)
		{
			continue;
		}

		/* Set state to exit */
		_eaf_service_set_state_lock(group, service, eaf_service_state_exit0);

		/* Cleanup resource */
		service->entry->on_exit();
		_eaf_service_teardown_cleanup_msgq(group, service);

		_eaf_service_set_state_lock(group, service, eaf_service_state_exit1);
	}
}

/**
 * @brief Working thread
 * @param[in] arg	eaf_service_group_t
 */
static void _eaf_thread(void* arg)
{
	eaf_group_t* group = arg;

	/* Set thread id */
	group->coroutine.local.tid = eaf_compat_thread_id();

	/* Set Thread Local Storage */
	if (eaf_thread_storage_set(&g_eaf_ctx->tls, arg) < 0)
	{
		return;
	}

	/* Wait for start */
	while (g_eaf_ctx->state == eaf_ctx_state_init)
	{
		eaf_compat_sem_pend(&group->msgq.sem, EAF_COMPAT_SEM_INFINITY);
	}
	if (g_eaf_ctx->state != eaf_ctx_state_busy)
	{
		return;
	}

	/* Do initialize */
	int init_count = _eaf_group_init_necessary(group);

	/* Notify we are done initialize */
	eaf_compat_sem_post(&g_eaf_ctx->ready);

	/* Set failure flag if initialize failed */
	if (init_count < 0)
	{
		g_eaf_ctx->mask.failure = 1;
	}
	/* If failure or no available service, exit thread */
	if (init_count <= 0)
	{
		return;
	}

	/* Event looping */
	while (g_eaf_ctx->state == eaf_ctx_state_busy
		&& _eaf_service_thread_loop(group) == 0)
	{
	}

	/* Teardown */
	if (g_eaf_ctx->state == eaf_ctx_state_teardown)
	{
		_eaf_service_teardown(group);
	}
	while (g_eaf_ctx->state == eaf_ctx_state_teardown
		&& _eaf_service_thread_loop(group) == 0)
	{
	}

	_eaf_group_exit(group, (size_t)-1);
}

static void _eaf_cleanup_msgq(eaf_service_t* service)
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
	/* Wait for thread exit */
	eaf_compat_thread_exit(&group->working);

	size_t i;
	for (i = 0; i < group->service.size; i++)
	{
		_eaf_cleanup_msgq(&group->service.table[i]);
	}

	/* Cleanup resources */
	eaf_compat_lock_exit(&group->objlock);
	eaf_compat_sem_exit(&group->msgq.sem);
}

static int _eaf_service_push_msg(eaf_group_t* group, eaf_service_t* service,
	eaf_msg_full_t* msg, uint32_t from, uint32_t to,
	void(*on_create)(eaf_msgq_record_t* record, void* arg), void* arg, int flag)
{
	int ret = eaf_errno_success;
	/* Check message queue size */
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

	/* Push to queue */
	eaf_compat_lock_enter(&group->objlock);
	do
	{
		if (service->runtime.local.state == eaf_service_state_exit0)
		{
			ret = eaf_errno_state;
			break;
		}

		switch (service->runtime.local.state)
		{
			/* For lazyload */
		case eaf_service_state_init2:
			_eaf_service_set_state_nolock(group, service, eaf_service_state_init1);
			break;

		case eaf_service_state_idle:
			_eaf_service_set_state_nolock(group, service, eaf_service_state_busy);
			break;

		default:
			break;
		}

		eaf_list_push_back(&service->msgq.queue, &record->node);
	} while (0);
	eaf_compat_lock_leave(&group->objlock);

	/* Revert if failure */
	if (ret != eaf_errno_success)
	{
		eaf_msg_dec_ref(EAF_MSG_C2I(msg));
		EAF_FREE(record);
		return ret;
	}

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

	/* Query service */
	eaf_group_t* group;
	eaf_service_t* service = _eaf_service_find_service(to, &group);

	if (service == NULL)
	{
		return eaf_errno_notfound;
	}

	/* Find message handler */
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
	/* Push message */
	return _eaf_service_push_msg(group, service, real_msg, from, to,
		_eaf_service_on_req_fix, (void*)msg_proc, 0);
#if defined(_MSC_VER)
#	pragma warning(pop)
#endif
}

/**
 * @brief Send response
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

	/* Push message */
	return _eaf_service_push_msg(group, service, real_msg, from, to,
		NULL, NULL, PUSH_FLAG_FORCE);
}

/**
 * @brief Do exit
 * @param[in] teardown	Is it a teardown procedure
 * @return				#eaf_errno
 */
static int _eaf_exit(eaf_exit_executor_t executor, int reason, int teardown)
{
	size_t i;
	if (g_eaf_ctx->state != eaf_ctx_state_teardown
		&& g_eaf_ctx->state != eaf_ctx_state_exit)
	{
		_eaf_hook_cleanup_before();
	}

	if (!teardown)
	{
		g_eaf_ctx->summary.executor = executor;
		g_eaf_ctx->summary.reason = reason;
	}

	g_eaf_ctx->state = teardown ? eaf_ctx_state_teardown : eaf_ctx_state_exit;

	for (i = 0; i < g_eaf_ctx->group.size; i++)
	{
		eaf_compat_sem_post(&g_eaf_ctx->group.table[i]->msgq.sem);
	}

	return eaf_errno_success;
}

static void _eaf_init_fix_address(const eaf_group_table_t* info, size_t size)
{
	g_eaf_ctx->group.size = size;
	g_eaf_ctx->group.table = (eaf_group_t**)(g_eaf_ctx + 1);
	g_eaf_ctx->group.table[0] = (eaf_group_t*)((char*)g_eaf_ctx->group.table + sizeof(eaf_group_t*) * size);

	size_t i;
	for (i = 1; i < size; i++)
	{
		g_eaf_ctx->group.table[i] = (eaf_group_t*)
			((char*)g_eaf_ctx->group.table[i - 1] + sizeof(eaf_group_t) + sizeof(eaf_service_t) * info[i - 1].service.size);
	}
}

static size_t _eaf_init_calculate_malloc_size(const eaf_group_table_t* info, size_t size)
{
	size_t malloc_size = sizeof(eaf_ctx_t) + sizeof(eaf_group_t*) * size;

	size_t i;
	for (i = 0; i < size; i++)
	{
		malloc_size += sizeof(eaf_group_t) + sizeof(eaf_service_t) * info[i].service.size;
	}

	return malloc_size;
}

static void _eaf_init_service(eaf_group_t* group, eaf_service_t* service, eaf_service_table_t* info)
{
	/* Convert mask */
	service->mask.alive = !!(info->attribute & eaf_service_attribute_alive);
	service->mask.lazyload = !!(info->attribute & eaf_service_attribute_lazyload);

	service->runtime.local.id = info->srv_id;
	service->msgq.capacity = info->msgq_size;
	eaf_list_init(&service->msgq.queue);

	/* by default, service should in init0 state */
	service->runtime.local.state = eaf_service_state_init0;
	eaf_list_push_back(&group->coroutine.wait_list, &service->runtime.node);
}

static int _eaf_init_group(const eaf_group_table_t* info, size_t size)
{
	size_t i, idx;
	for (idx = 0; idx < size; idx++)
	{
		g_eaf_ctx->group.table[idx]->index = idx;
		g_eaf_ctx->group.table[idx]->service.size = info[idx].service.size;
		eaf_list_init(&g_eaf_ctx->group.table[idx]->coroutine.busy_list);
		eaf_list_init(&g_eaf_ctx->group.table[idx]->coroutine.wait_list);
		if (eaf_compat_lock_init(&g_eaf_ctx->group.table[idx]->objlock, eaf_lock_attr_normal) < 0)
		{
			goto err;
		}
		if (eaf_compat_sem_init(&g_eaf_ctx->group.table[idx]->msgq.sem, 0) < 0)
		{
			eaf_compat_lock_exit(&g_eaf_ctx->group.table[idx]->objlock);
			goto err;
		}

		for (i = 0; i < info[idx].service.size; i++)
		{
			_eaf_init_service(g_eaf_ctx->group.table[idx],
				&g_eaf_ctx->group.table[idx]->service.table[i],
				&info[idx].service.table[i]);
		}

		if (eaf_compat_thread_init(&g_eaf_ctx->group.table[idx]->working, &info[idx].attr,
			_eaf_thread, g_eaf_ctx->group.table[idx]) < 0)
		{
			eaf_compat_sem_exit(&g_eaf_ctx->group.table[idx]->msgq.sem);
			eaf_compat_lock_exit(&g_eaf_ctx->group.table[idx]->objlock);
			goto err;
		}
	}

	return 0;

err:
	for (i = 0; i < idx; i++)
	{
		eaf_compat_sem_post(&g_eaf_ctx->group.table[i]->msgq.sem);

		eaf_compat_thread_exit(&g_eaf_ctx->group.table[i]->working);
		eaf_compat_sem_exit(&g_eaf_ctx->group.table[i]->msgq.sem);
		eaf_compat_lock_exit(&g_eaf_ctx->group.table[i]->objlock);
	}

	return -1;
}

static void _eaf_load_fix_state(void)
{
	size_t i, j;
	for (i = 0; i < g_eaf_ctx->group.size; i++)
	{
		eaf_group_t* group = g_eaf_ctx->group.table[i];

		eaf_compat_lock_enter(&group->objlock);
		for (j = 0; j < group->service.size; j++)
		{
			eaf_service_t* service = &group->service.table[j];

			/* If no entry, service is dead */
			if (service->entry == NULL)
			{
				_eaf_service_set_state_nolock(group, service, eaf_service_state_exit1);
				continue;
			}

			/* If lazyload was set, do not initialize */
			if (service->mask.lazyload)
			{
				_eaf_service_set_state_nolock(group, service, eaf_service_state_init2);
				continue;
			}

			/* In normal condition, service will do initialize */
			_eaf_service_set_state_nolock(group, service, eaf_service_state_init1);
		}
		eaf_compat_lock_leave(&group->objlock);
	}
}

int eaf_init(_In_ const eaf_group_table_t* info, _In_ size_t size)
{
	if (g_eaf_ctx != NULL)
	{
		return eaf_errno_duplicate;
	}

	g_eaf_ctx = EAF_CALLOC(1, _eaf_init_calculate_malloc_size(info, size));
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

	/* Fix address */
	_eaf_init_fix_address(info, size);

	/* Initialize */
	if (_eaf_init_group(info, size) < 0)
	{
		goto err;
	}

	return eaf_errno_success;

err:
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

	/* Fix initialize state */
	_eaf_load_fix_state();

	/* Start all thread */
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

	int ret = g_eaf_ctx->mask.failure ? eaf_errno_state : eaf_errno_success;
	_eaf_hook_load_after(ret);

	return ret;
}

int eaf_teardown(void)
{
	if (g_eaf_ctx == NULL || g_eaf_ctx->state == eaf_ctx_state_teardown)
	{
		return eaf_errno_state;
	}

	return _eaf_exit(eaf_exit_executor_user, 0, 1);
}

int eaf_exit(_In_ int reason)
{
	if (g_eaf_ctx == NULL || g_eaf_ctx->state == eaf_ctx_state_exit)
	{
		return eaf_errno_state;
	}

	return _eaf_exit(eaf_exit_executor_user, reason, 0);
}

int eaf_cleanup(_Out_opt_ eaf_cleanup_summary_t* summary)
{
	size_t i;
	const eaf_hook_t* hook = g_eaf_ctx->hook;

	/* Wait for group exit */
	for (i = 0; i < g_eaf_ctx->group.size; i++)
	{
		_eaf_cleanup_group(g_eaf_ctx->group.table[i]);
	}

	eaf_compat_sem_exit(&g_eaf_ctx->ready);
	eaf_thread_storage_exit(&g_eaf_ctx->tls);

	if (summary != NULL)
	{
		memcpy(summary, &g_eaf_ctx->summary, sizeof(*summary));
	}

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
	if (entry->on_init == NULL || entry->on_exit == NULL)
	{
		return eaf_errno_invalid;
	}

	size_t i, idx;
	eaf_service_t* service = NULL;
	for (i = 0; i < g_eaf_ctx->group.size; i++)
	{
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

		default:
			ret = eaf_errno_state;
			break;
		}
	} while (0);
	eaf_compat_lock_leave(&group->objlock);

	if (ret == eaf_errno_success)
	{
		eaf_compat_sem_post(&group->msgq.sem);
	}

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

EAF_API size_t eaf_group_size(void)
{
	return g_eaf_ctx->group.size;
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
