#include <assert.h>
#include "EAF/core/service.h"
#include "EAF/core/rpc.h"
#include "EAF/utils/list.h"
#include "EAF/utils/errno.h"
#include "EAF/utils/define.h"
#include "EAF/utils/map.h"
#include "EAF/utils/log.h"
#include "compat/lock.h"
#include "compat/thread.h"
#include "compat/semaphore.h"
#include "utils/memory.h"
#include "message.h"

/*
* Before Visual Studio 2015, there is a bug that a `do { } while (0)` will triger C4127 warning
* https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4127
*/
#if defined(_MSC_VER) && _MSC_VER <= 1900
#	pragma warning(disable : 4127)
#endif

#define PUSH_FLAG_LOCK			(0x01 << 0x00)
#define PUSH_FLAG_FORCE			(0x01 << 0x01)
#define HAS_FLAG(flag, bit)		((flag) & (bit))

/**
* �Ա�ģ��
* @param a		ֵa
* @param b		ֵb
*/
#define COMPARE_TEMPLATE(a, b)	\
	do {\
		if ((a) < (b)) {\
			return -1;\
		} else if ((a) > (b)) {\
			return 1;\
		}\
	} while (0)

/**
* �������λ
*/
#define CLEAR_CC0(group)	\
	do {\
		(group)->coroutine.local.cc[0] = 0;\
	} while (0)

/**
* ������λ
*/
#define CHECK_CC0(group, bmask)	\
	((group)->coroutine.local.cc[0] & (bmask))

/**
* ��ȡ��ǰ�������е�����
*/
#define CUR_RUN(group)	\
	((group)->coroutine.cur_run)

/**
* ���õ�ǰ�������е�����
*/
#define CUR_RUN_SET_BY_IDX(group, idx)	\
	do {\
		eaf_group_t* _group = group;\
		CUR_RUN(_group) = &_group->service.table[idx];\
	} while (0)

/**
* ���÷�֧
*/
#define RESET_BRANCH(service)	\
	do {\
		(service)->coroutine.local.branch = 0;\
	} while (0)

/**
* ���÷�֧
*/
#define CUR_RUN_RESET_BRANCH(group)	\
	RESET_BRANCH(CUR_RUN(group))

/**
* call user defined yield hook
*/
#define CALL_YIELD_HOOK(group)	\
	do {\
		eaf_group_t* _group = group;\
		if (_group->coroutine.local.yield.hook != NULL) {\
			_group->coroutine.local.yield.hook(&(CUR_RUN(_group)->coroutine.local),\
				_group->coroutine.local.yield.arg);\
		}\
	} while (0)

/**
* ����״̬
*
* INIT1
*  /|\       |--------|
*  \|/      \|/       |
* INIT0 --> IDLE --> BUSY --> PEND
*   |       \|/      /|\       |
*   | ----> EXIT      |--------|
*/
typedef enum eaf_service_state
{
	eaf_service_state_init0,					/** ��ʼ̬ */
	eaf_service_state_init1,					/** ��ʼ̬ */
	eaf_service_state_idle,						/** ���� */
	eaf_service_state_busy,						/** æµ */
	eaf_service_state_pend,						/** �ȴ�resume */
	eaf_service_state_exit,						/** �˳� */
}eaf_service_state_t;

typedef enum eaf_ctx_state
{
	eaf_ctx_state_init,							/** ��ʼ״̬ */
	eaf_ctx_state_busy,							/** ����״̬ */
	eaf_ctx_state_exit,							/** �˳�״̬ */
}eaf_ctx_state_t;

typedef struct eaf_msgq_record
{
	eaf_list_node_t					node;		/** ����ʽ�ڵ� */

	union
	{
		struct
		{
			eaf_req_handle_fn		req_fn;		/** �������� */
		}req;
	}info;

	struct
	{
		struct eaf_service*			service;	/** ������ */
		eaf_msg_full_t*				msg;		/** ��Ϣ */
	}data;
}eaf_msgq_record_t;

typedef struct eaf_subscribe_record
{
	eaf_map_node_t					node;		/** ����ʽ�ڵ� */
	struct
	{
		uint32_t					evt_id;		/** �¼�ID */
		struct eaf_service*			service;	/** ������ */
		eaf_evt_handle_fn			proc;		/** �ص����� */
		void*						priv;		/** �Զ������ */
	}data;
}eaf_subscribe_record_t;

typedef struct eaf_service
{
	eaf_service_state_t				state;		/** ״̬ */
	const eaf_service_info_t*		load;		/** ������Ϣ */

	struct
	{
		eaf_service_local_t			local;		/** ���ش洢 */
		eaf_list_node_t				node;		/** ����ʽ�ڵ㡣��ready_list��wait_list�� */
	}coroutine;

	struct
	{
		eaf_map_node_t*				cbiter;		/** �ص��α꣬eaf_subscribe_record_t */
	}subscribe;

	struct
	{
		eaf_msgq_record_t*			cur_msg;	/** ���ڴ������Ϣ */
		eaf_list_t					queue;		/** �������Ϣ */
		size_t						capacity;	/** ��Ϣ�������� */
	}msgq;
}eaf_service_t;

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4200)
#endif
typedef struct eaf_group
{
	eaf_compat_lock_t				objlock;	/** �߳��� */
	eaf_compat_thread_t				working;	/** �����߳� */

	struct 
	{
		eaf_group_local_t			local;		/** local storage */
		eaf_service_t*				cur_run;	/** ��ǰ���ڴ���ķ��� */
		eaf_list_t					busy_list;	/** INIT0/BUSY */
		eaf_list_t					wait_list;	/** INIT1/IDLE/PEND */
	}coroutine;

	struct
	{
		eaf_map_t					table;		/** ���ı�eaf_subscribe_record_t */
	}subscribe;

	struct
	{
		eaf_compat_sem_t			sem;		/** �ź��� */
	}msgq;

	struct
	{
		size_t						size;		/** ������� */
		eaf_service_t				table[];	/** ����� */
	}service;
}eaf_group_t;
#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

typedef struct eaf_ctx
{
	eaf_ctx_state_t					state;		/** ״̬ */
	eaf_compat_sem_t				ready;		/** �˳��ź� */
	eaf_thread_storage_t			tls;		/** �߳�˽�б��� */

	struct
	{
		size_t						size;		/** �����鳤�� */
		eaf_group_t**				table;		/** ������ */
	}group;

	const eaf_rpc_cfg_t*			rpc;		/** RPC */
}eaf_ctx_t;

static eaf_ctx_t* g_eaf_ctx			= NULL;		/** ȫ�����л��� */

static eaf_service_t* _eaf_service_find_service(uint32_t service_id, eaf_group_t** group)
{
	size_t i;
	for (i = 0; i < g_eaf_ctx->group.size; i++)
	{
		size_t j;
		for (j = 0; j < g_eaf_ctx->group.table[i]->service.size; j++)
		{
			eaf_service_t* service = &g_eaf_ctx->group.table[i]->service.table[j];
			if (service->coroutine.local.id != service_id)
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

static eaf_service_t* _eaf_get_first_busy_service_lock(eaf_group_t* group)
{
	eaf_service_t* service;
	eaf_compat_lock_enter(&group->objlock);
	{
		eaf_list_node_t* it = eaf_list_begin(&group->coroutine.busy_list);
		service = it != NULL ? EAF_CONTAINER_OF(it, eaf_service_t, coroutine.node) : NULL;
	}
	eaf_compat_lock_leave(&group->objlock);

	return service;
}

/**
* ���÷���״̬
* @param group		��
* @param service	����
* @param state		״̬
*/
static void _eaf_service_set_state_nolock(eaf_group_t* group, eaf_service_t* service,
	eaf_service_state_t state)
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
		eaf_list_erase(&group->coroutine.wait_list, &service->coroutine.node);
		break;

	case eaf_service_state_init0:
	case eaf_service_state_busy:
		eaf_list_erase(&group->coroutine.busy_list, &service->coroutine.node);
		break;
	}

	switch (state)
	{
	case eaf_service_state_init1:
	case eaf_service_state_idle:
	case eaf_service_state_pend:
	case eaf_service_state_exit:
		eaf_list_push_back(&group->coroutine.wait_list, &service->coroutine.node);
		break;

	case eaf_service_state_init0:
	case eaf_service_state_busy:
		eaf_list_push_back(&group->coroutine.busy_list, &service->coroutine.node);
		break;
	}
	service->state = state;
}

static void _eaf_service_set_state_lock(eaf_group_t* group, eaf_service_t* service, eaf_service_state_t state)
{
	eaf_compat_lock_enter(&group->objlock);
	{
		_eaf_service_set_state_nolock(group, service, state);
	}
	eaf_compat_lock_leave(&group->objlock);
}

static void _eaf_service_resume_message_event(eaf_group_t* group, eaf_service_t* service)
{
	eaf_evt_handle_fn proc;
	void* priv;

	eaf_subscribe_record_t tmp_record;
	tmp_record.data.evt_id = service->msgq.cur_msg->data.msg->msg.id;
	tmp_record.data.service = service;
	tmp_record.data.proc = NULL;
	tmp_record.data.priv = NULL;

	eaf_compat_lock_enter(&group->objlock);
	{
		if (service->subscribe.cbiter == NULL)
		{
			service->subscribe.cbiter = eaf_map_find_upper(&group->subscribe.table, &tmp_record.node);
		}

		while (service->subscribe.cbiter != NULL)
		{
			/* ȷ�����ڵ�ǰ����Χ */
			eaf_subscribe_record_t* orig = EAF_CONTAINER_OF(service->subscribe.cbiter, eaf_subscribe_record_t, node);
			if (orig->data.evt_id != service->msgq.cur_msg->data.msg->msg.id || orig->data.service != service->msgq.cur_msg->data.service)
			{
				break;
			}
			proc = orig->data.proc;
			priv = orig->data.priv;

			/* ִ�лص� */
			eaf_compat_lock_leave(&group->objlock);
			{
				proc(EAF_MSG_C2I(service->msgq.cur_msg->data.msg), priv);
			}
			eaf_compat_lock_enter(&group->objlock);

			/* ������yield����ֹ */
			if (CHECK_CC0(group, EAF_SERVICE_CC0_YIELD))
			{
				break;
			}
			service->subscribe.cbiter = eaf_map_next(&group->subscribe.table, service->subscribe.cbiter);
		}
	}
	eaf_compat_lock_leave(&group->objlock);
}

static void _eaf_service_resume_message(eaf_group_t* group, eaf_service_t* service)
{
	switch (service->msgq.cur_msg->data.msg->msg.type)
	{
	case eaf_msg_type_req:
		service->msgq.cur_msg->info.req.req_fn(EAF_MSG_C2I(service->msgq.cur_msg->data.msg));
		goto fin;

	case eaf_msg_type_rsp:
		service->msgq.cur_msg->data.msg->msg.info.rr.rfn(EAF_MSG_C2I(service->msgq.cur_msg->data.msg));
		goto fin;

	case eaf_msg_type_evt:
		_eaf_service_resume_message_event(group, service);
		goto fin;
	}

fin:
	/* ����yield */
	if (CHECK_CC0(group, EAF_SERVICE_CC0_YIELD))
	{
		_eaf_service_set_state_lock(group, service, eaf_service_state_pend);
		CALL_YIELD_HOOK(group);
		return;
	}

	/* �������������÷�֧ */
	RESET_BRANCH(service);
	_eaf_service_destroy_msg_record(service->msgq.cur_msg);
	service->msgq.cur_msg = NULL;
}

static void _eaf_handle_new_message(eaf_group_t* group, eaf_service_t* service)
{
	/* ����service��״ֻ̬������BUSY */
	assert(service->state == eaf_service_state_busy);

	/* ��ǰ��Ϣ��ΪNULLʱ��˵����Ҫ�ָ������� */
	if (service->msgq.cur_msg != NULL)
	{
		_eaf_service_resume_message(group, service);
		return;
	}

	/* ȡ����Ϣ������Ϣ����Ϊ�գ��򽫷�������IDLE״̬ */
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

	_eaf_service_resume_message(group, service);
}

static void _eaf_group_finish_service_init_lock(eaf_group_t* group, eaf_service_t* service)
{
	eaf_compat_lock_enter(&group->objlock);
	do
	{
		/* �л���IDLE̬ */
		_eaf_service_set_state_nolock(group, service, eaf_service_state_idle);
		/* ��Ϣ���зǿ�ʱ���л���BUSY */
		if (eaf_list_size(&service->msgq.queue) > 0)
		{
			_eaf_service_set_state_nolock(group, service, eaf_service_state_busy);
		}
	} while (0);
	eaf_compat_lock_leave(&group->objlock);
}

/**
* �������г�ʼ��
*/
static int _eaf_service_resume_init(eaf_group_t* group, eaf_service_t* service)
{
	int ret = service->load->on_init();

	/* ����Ƿ�ִ��yield */
	if (CHECK_CC0(group, EAF_SERVICE_CC0_YIELD))
	{/* ��init�׶ν�����yield */
		_eaf_service_set_state_lock(group, service, eaf_service_state_init1);
		CALL_YIELD_HOOK(group);
		return 0;
	}
	RESET_BRANCH(service);

	/* ��ʼ��ʧ��ʱ���ش��� */
	if (ret < 0)
	{
		return -1;
	}

	/* �����ɳ�ʼ�� */
	_eaf_group_finish_service_init_lock(group, service);
	return 0;
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

	CUR_RUN(group) = service;
	CLEAR_CC0(group);

	if (service->state == eaf_service_state_init0)
	{
		return _eaf_service_resume_init(group, service);
	}

	_eaf_handle_new_message(group, service);
	return 0;
}

/**
* ��ȡ��ǰ�̶߳�Ӧ�ķ�����
* @return		������
*/
static eaf_group_t* _eaf_get_current_group(void)
{
	return eaf_thread_storage_get(&g_eaf_ctx->tls);
}

static int _eaf_group_init(eaf_group_t* group, size_t* idx)
{
	int counter = 0;
	for (*idx = 0; *idx < group->service.size; *idx += 1)
	{
		CUR_RUN_SET_BY_IDX(group, *idx);

		if (CUR_RUN(group)->load == NULL || CUR_RUN(group)->load->on_init == NULL)
		{
			continue;
		}

		CLEAR_CC0(group);
		int ret = CUR_RUN(group)->load->on_init();

		/* ����Ƿ�ִ��yield */
		if (CHECK_CC0(group, EAF_SERVICE_CC0_YIELD))
		{/* ��init�׶ν�����yield */
			_eaf_service_set_state_lock(group, CUR_RUN(group), eaf_service_state_init1);

			/* call user hook */
			CALL_YIELD_HOOK(group);

			/*
			* need to consider it as init success,
			* because if no service finish init process here, this thread will be exited.
			*/
			counter++;

			continue;
		}
		CUR_RUN_RESET_BRANCH(group);

		/* ����ʼ��ʧ���򷵻ش��� */
		if (ret < 0)
		{
			return -1;
		}

		_eaf_group_finish_service_init_lock(group, CUR_RUN(group));
		counter++;
	}

	return counter;
}

/**
* �����߳�
* @param arg	eaf_service_group_t
*/
static void _eaf_service_thread(void* arg)
{
	size_t i;
	size_t init_idx = 0;
	eaf_group_t* group = arg;

	/* �����߳�˽�б��� */
	if (eaf_thread_storage_set(&g_eaf_ctx->tls, arg) < 0)
	{
		return;
	}

	/* �ȴ����� */
	while (EAF_ACCESS(eaf_ctx_state_t, g_eaf_ctx->state) == eaf_ctx_state_init)
	{
		eaf_compat_sem_pend(&group->msgq.sem, (unsigned)-1);
	}
	if (g_eaf_ctx->state != eaf_ctx_state_busy)
	{
		return;
	}

	/* ���г�ʼ�� */
	int init_count = _eaf_group_init(group, &init_idx);

	/* ͨ���ʼ����� */
	eaf_compat_sem_post(&g_eaf_ctx->ready);

	/* ʧ��ʱ���� */
	if (init_count <= 0)
	{
		goto cleanup;
	}

	/* �¼�ѭ�� */
	while (g_eaf_ctx->state == eaf_ctx_state_busy
		&& _eaf_service_thread_loop(group) == 0)
	{
	}

cleanup:
	for (i = 0; i < init_idx; i++)
	{
		CUR_RUN_SET_BY_IDX(group, i);
		_eaf_service_set_state_lock(group, CUR_RUN(group), eaf_service_state_exit);

		if (group->service.table[i].load != NULL
			&& group->service.table[i].load->on_exit != NULL)
		{
			CUR_RUN(group)->load->on_exit();
		}
	}
}

static void _eaf_service_cleanup_group(eaf_group_t* group)
{
	/* ����������Ա�֤�̸߳�֪��״̬�ı� */
	eaf_compat_sem_post(&group->msgq.sem);

	/* �ȴ��߳��˳� */
	eaf_compat_thread_exit(&group->working);

	eaf_map_node_t* it = eaf_map_begin(&group->subscribe.table);
	while (it != NULL)
	{
		eaf_map_node_t* tmp = it;
		it = eaf_map_next(&group->subscribe.table, it);
		eaf_map_erase(&group->subscribe.table, tmp);

		eaf_subscribe_record_t* record = EAF_CONTAINER_OF(tmp, eaf_subscribe_record_t, node);
		EAF_FREE(record);
	}

	/* ������Դ */
	eaf_compat_lock_exit(&group->objlock);
	eaf_compat_sem_exit(&group->msgq.sem);
}

static int _eaf_service_push_msg(eaf_group_t* group, eaf_service_t* service, eaf_msg_full_t* msg,
	void(*on_create)(eaf_msgq_record_t* record, void* arg), void* arg, int flag)
{
	/* �����Ϣ�������� */
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
		if (service->state == eaf_service_state_idle)
		{
			_eaf_service_set_state_nolock(group, service, eaf_service_state_busy);
		}
		eaf_list_push_back(&service->msgq.queue, &record->node);
	} while (0);
	if (HAS_FLAG(flag, PUSH_FLAG_LOCK)) { eaf_compat_lock_leave(&group->objlock); }

	eaf_compat_sem_post(&group->msgq.sem);
	return eaf_errno_success;
}

static void _eaf_service_on_req_record_create(eaf_msgq_record_t* record, void* arg)
{
#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4055)
#endif
	record->info.req.req_fn = (eaf_req_handle_fn)arg;
#if defined(_MSC_VER)
#	pragma warning(pop)
#endif
}

static int _eaf_service_on_cmp_subscribe_record(const eaf_map_node_t* key1, const eaf_map_node_t* key2, void* arg)
{
	(void)arg;
	eaf_subscribe_record_t* record1 = EAF_CONTAINER_OF(key1, eaf_subscribe_record_t, node);
	eaf_subscribe_record_t* record2 = EAF_CONTAINER_OF(key2, eaf_subscribe_record_t, node);

	COMPARE_TEMPLATE(record1->data.evt_id, record2->data.evt_id);
	COMPARE_TEMPLATE(record1->data.service, record2->data.service);
	COMPARE_TEMPLATE((uintptr_t)record1->data.proc, (uintptr_t)record2->data.proc);
	COMPARE_TEMPLATE((uintptr_t)record1->data.priv, (uintptr_t)record2->data.priv);

	return 0;
}

static eaf_service_t* _eaf_get_current_service(eaf_group_t** group)
{
	eaf_group_t* ret = _eaf_get_current_group();

	if (group != NULL)
	{
		*group = ret;
	}
	return ret != NULL ? CUR_RUN(ret) : NULL;
}

/**
* check if the service subscribe the same event
* @return	bool
*/
static int _eaf_is_subscribe_near_nolock(eaf_group_t* group, eaf_subscribe_record_t* record)
{
	eaf_map_node_t* it;
	eaf_subscribe_record_t* orig;

	it = eaf_map_prev(&group->subscribe.table, &record->node);
	if (it != NULL)
	{
		orig = EAF_CONTAINER_OF(it, eaf_subscribe_record_t, node);
		if (orig->data.evt_id == record->data.evt_id && orig->data.service == record->data.service)
		{
			return 1;
		}
	}

	it = eaf_map_next(&group->subscribe.table, &record->node);
	if (it != NULL)
	{
		orig = EAF_CONTAINER_OF(it, eaf_subscribe_record_t, node);
		if (orig->data.evt_id == record->data.evt_id && orig->data.service == record->data.service)
		{
			return 1;
		}
	}

	return 0;
}

static int _eaf_send_req(uint32_t from, uint32_t to, eaf_msg_t* req, int rpc)
{
	if (g_eaf_ctx == NULL || g_eaf_ctx->state != eaf_ctx_state_busy)
	{
		return eaf_errno_state;
	}

	eaf_msg_full_t* real_msg = EAF_MSG_I2C(req);
	req->from = from;
	req->to = to;

	/* ��ѯ���շ��� */
	eaf_group_t* group;
	eaf_service_t* service = _eaf_service_find_service(to, &group);

	/* if service not found, send to rpc */
	if (service == NULL)
	{
		return rpc && g_eaf_ctx->rpc != NULL ?
			g_eaf_ctx->rpc->send_msg(req) : eaf_errno_notfound;
	}

	/* ������Ϣ������ */
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

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4054)
#endif
	/* ������Ϣ */
	return _eaf_service_push_msg(group, service, real_msg,
		_eaf_service_on_req_record_create, (void*)msg_proc, PUSH_FLAG_LOCK);
#if defined(_MSC_VER)
#	pragma warning(pop)
#endif
}

static int _eaf_send_rsp(uint32_t from, eaf_msg_t* rsp, int rpc)
{
	if (g_eaf_ctx == NULL || g_eaf_ctx->state != eaf_ctx_state_busy)
	{
		return eaf_errno_state;
	}

	eaf_msg_full_t* real_msg = EAF_MSG_I2C(rsp);
	rsp->from = from;

	/* ��ѯ���շ��� */
	eaf_group_t* group;
	eaf_service_t* service = _eaf_service_find_service(rsp->to, &group);

	/* if service not found, send to rpc */
	if (service == NULL)
	{
		return rpc && g_eaf_ctx->rpc != NULL ?
			g_eaf_ctx->rpc->send_msg(rsp) : eaf_errno_notfound;
	}

	/* ������Ϣ */
	return _eaf_service_push_msg(group, service, real_msg, NULL, NULL, PUSH_FLAG_LOCK | PUSH_FLAG_FORCE);
}

static int _eaf_send_evt(uint32_t from, eaf_msg_t* evt, int rpc)
{
	if (g_eaf_ctx == NULL || g_eaf_ctx->state != eaf_ctx_state_busy)
	{
		return eaf_errno_state;
	}

	eaf_msg_full_t* real_msg = EAF_MSG_I2C(evt);
	evt->from = from;

	eaf_subscribe_record_t tmp_key;
	tmp_key.data.evt_id = evt->id;
	tmp_key.data.service = NULL;
	tmp_key.data.proc = NULL;
	tmp_key.data.priv = NULL;

	/* TODO: optimization this lock */
	size_t i;
	for (i = 0; i < g_eaf_ctx->group.size; i++)
	{
		eaf_compat_lock_enter(&g_eaf_ctx->group.table[i]->objlock);
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
				_eaf_service_push_msg(g_eaf_ctx->group.table[i], record->data.service, real_msg, NULL, NULL, 0);
			}
		} while (0);
		eaf_compat_lock_leave(&g_eaf_ctx->group.table[i]->objlock);
	}

	/* send to rpc */
	if (rpc && g_eaf_ctx->rpc != NULL)
	{
		g_eaf_ctx->rpc->send_msg(evt);
	}

	return eaf_errno_success;
}

int eaf_setup(const eaf_group_table_t* info, size_t size)
{
	size_t i;
	if (g_eaf_ctx != NULL)
	{
		return eaf_errno_duplicate;
	}

	/* ���������ڴ� */
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

	/* �޸������ַ */
	g_eaf_ctx->group.size = size;
	g_eaf_ctx->group.table = (eaf_group_t**)(g_eaf_ctx + 1);
	g_eaf_ctx->group.table[0] = (eaf_group_t*)((char*)g_eaf_ctx->group.table + sizeof(eaf_group_t*) * size);
	for (i = 1; i < size; i++)
	{
		g_eaf_ctx->group.table[i] = (eaf_group_t*)
			((char*)g_eaf_ctx->group.table[i - 1] + sizeof(eaf_group_t) + sizeof(eaf_service_t) * info[i - 1].service.size);
	}

	/* ��Դ��ʼ�� */
	size_t init_idx;
	for (init_idx = 0; init_idx < size; init_idx++)
	{
		g_eaf_ctx->group.table[init_idx]->service.size = info[init_idx].service.size;
		eaf_list_init(&g_eaf_ctx->group.table[init_idx]->coroutine.busy_list);
		eaf_list_init(&g_eaf_ctx->group.table[init_idx]->coroutine.wait_list);
		eaf_map_init(&g_eaf_ctx->group.table[init_idx]->subscribe.table,
			_eaf_service_on_cmp_subscribe_record, NULL);
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
			g_eaf_ctx->group.table[init_idx]->service.table[idx].coroutine.local.id =
				info[init_idx].service.table[idx].srv_id;
			g_eaf_ctx->group.table[init_idx]->service.table[idx].msgq.capacity =
				info[init_idx].service.table[idx].msgq_size;
			eaf_list_init(&g_eaf_ctx->group.table[init_idx]->service.table[idx].msgq.queue);

			/* Ĭ������¼���wait�� */
			g_eaf_ctx->group.table[init_idx]->service.table[idx].state = eaf_service_state_init0;
			eaf_list_push_back(&g_eaf_ctx->group.table[init_idx]->coroutine.busy_list,
				&g_eaf_ctx->group.table[init_idx]->service.table[idx].coroutine.node);
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

	/* initialize rpc */
	if (g_eaf_ctx->rpc != NULL && g_eaf_ctx->rpc->on_init_done() != 0)
	{
		return eaf_errno_rpc_failure;
	}

	/* ���������߳� */
	g_eaf_ctx->state = eaf_ctx_state_busy;
	for (i = 0; i < g_eaf_ctx->group.size; i++)
	{
		eaf_compat_sem_post(&g_eaf_ctx->group.table[i]->msgq.sem);
	}

	/* �ȴ����з������ */
	for (i = 0; i < g_eaf_ctx->group.size; i++)
	{
		eaf_compat_sem_pend(&g_eaf_ctx->ready, (unsigned)-1);
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

	/* change state */
	g_eaf_ctx->state = eaf_ctx_state_exit;

	/* exit rpc */
	if (g_eaf_ctx->rpc != NULL)
	{
		g_eaf_ctx->rpc->on_exit();
	}

	/* exit thread */
	for (i = 0; i < g_eaf_ctx->group.size; i++)
	{
		_eaf_service_cleanup_group(g_eaf_ctx->group.table[i]);
	}

	/* resource cleanup */
	EAF_FREE(g_eaf_ctx);
	g_eaf_ctx = NULL;

	return eaf_errno_success;
}

int eaf_register(uint32_t id, const eaf_service_info_t* info)
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
			if (g_eaf_ctx->group.table[i]->service.table[idx].coroutine.local.id == id)
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

	/* report to rpc */
	if (g_eaf_ctx->rpc != NULL)
	{
		eaf_rpc_service_info_t service_info;
		service_info.service_id = id;
		service_info.capacity = (uint32_t)service->msgq.capacity;
		g_eaf_ctx->rpc->on_service_register(&service_info);
	}

	service->load = info;
	return eaf_errno_success;
}

int eaf_subscribe(uint32_t srv_id, uint32_t evt_id, eaf_evt_handle_fn fn, void* arg)
{
	/* �ǹ���״̬��ֹ���� */
	if (g_eaf_ctx->state != eaf_ctx_state_busy)
	{
		return eaf_errno_state;
	}

	eaf_group_t* group;
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
	int need_notify_rpc = 0;
	eaf_compat_lock_enter(&group->objlock);
	do 
	{
		ret = eaf_map_insert(&group->subscribe.table, &record->node);
		if (ret < 0)
		{
			break;
		}
		need_notify_rpc = g_eaf_ctx->rpc != NULL && _eaf_is_subscribe_near_nolock(group, record);
	} while (0);
	eaf_compat_lock_leave(&group->objlock);

	/* insert failed. duplicate subscribe */
	if (ret < 0)
	{
		EAF_FREE(record);
		return eaf_errno_duplicate;
	}

	/* notify RPC */
	if (need_notify_rpc)
	{
		g_eaf_ctx->rpc->on_event_subscribe(srv_id, evt_id);
	}

	return eaf_errno_success;
}

int eaf_unsubscribe(uint32_t srv_id, uint32_t evt_id, eaf_evt_handle_fn fn, void* arg)
{
	eaf_group_t* group;
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

	int need_notify_rpc = 0;
	eaf_subscribe_record_t* record = NULL;
	eaf_compat_lock_enter(&group->objlock);
	do 
	{
		eaf_map_node_t* it = eaf_map_find(&group->subscribe.table, &tmp_key.node);
		if (it == NULL)
		{
			break;
		}
		record = EAF_CONTAINER_OF(it, eaf_subscribe_record_t, node);
		need_notify_rpc = g_eaf_ctx->rpc != NULL && _eaf_is_subscribe_near_nolock(group, record);

		eaf_map_erase(&group->subscribe.table, it);

		/* ��ɾ����¼����������������򽫷��������ǰ�� */
		if (it == service->subscribe.cbiter)
		{
			service->subscribe.cbiter = eaf_map_prev(&group->subscribe.table, it);
		}
	} while (0);
	eaf_compat_lock_leave(&group->objlock);

	if (record == NULL)
	{
		return eaf_errno_notfound;
	}

	/* notify RPC */
	if (need_notify_rpc)
	{
		g_eaf_ctx->rpc->on_event_unsubscribe(srv_id, evt_id);
	}

	EAF_FREE(record);
	return eaf_errno_success;
}

int eaf_send_req(uint32_t from, uint32_t to, eaf_msg_t* req)
{
	return _eaf_send_req(from, to, req, 1);
}

int eaf_send_rsp(uint32_t from, eaf_msg_t* rsp)
{
	return _eaf_send_rsp(from, rsp, 1);
}

int eaf_send_evt(uint32_t from, eaf_msg_t* evt)
{
	return _eaf_send_evt(from, evt, 1);
}

int eaf_resume(uint32_t srv_id)
{
	eaf_group_t* group;
	eaf_service_t* service = _eaf_service_find_service(srv_id, &group);
	if (service == NULL)
	{
		return eaf_errno_notfound;
	}

	int ret = eaf_errno_success;
	eaf_compat_lock_enter(&group->objlock);
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
	eaf_compat_lock_leave(&group->objlock);

	eaf_compat_sem_post(&group->msgq.sem);

	return ret;
}

eaf_service_local_t* eaf_service_get_local(eaf_group_local_t** local)
{
	if (g_eaf_ctx == NULL)
	{
		return NULL;
	}

	eaf_group_t* group;
	eaf_service_t* service = _eaf_get_current_service(&group);
	if (service == NULL)
	{
		return NULL;
	}

	if (local != NULL)
	{
		*local = &group->coroutine.local;
	}

	return &service->coroutine.local;
}

int eaf_rpc_init(const eaf_rpc_cfg_t* cfg)
{
	if (g_eaf_ctx->rpc != NULL)
	{
		return eaf_errno_duplicate;
	}

	/* init */
	cfg->on_init();

	g_eaf_ctx->rpc = cfg;
	return eaf_errno_success;
}

int eaf_rpc_income(eaf_msg_t* msg)
{
	switch (msg->type)
	{
	case eaf_msg_type_req:
		return _eaf_send_req(msg->from, msg->to, msg, 0);

	case eaf_msg_type_rsp:
		return _eaf_send_rsp(msg->from, msg, 0);

	case eaf_msg_type_evt:
		return _eaf_send_evt(msg->from, msg, 0);

	default:
		break;
	}

	return eaf_errno_invalid;
}

uint32_t eaf_service_self(void)
{
	eaf_service_local_t* local = eaf_service_get_local(NULL);
	return local != NULL ? local->id : (uint32_t)-1;
}
