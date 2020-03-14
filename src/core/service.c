#include <assert.h>
#include "EAF/core/load.h"
#include "EAF/filber/filber.h"
#include "EAF/utils/list.h"
#include "EAF/utils/errno.h"
#include "EAF/utils/define.h"
#include "EAF/utils/map.h"
#include "arch/eaf_setjmp.h"
#include "compat/mutex.h"
#include "compat/thread.h"
#include "compat/semaphore.h"
#include "utils/memory.h"
#include "service.h"
#include "message.h"

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

#define EAF_SERVICE_HANDLE_REQ(_group, rec_req)	\
	do {\
		(rec_req)->info.req.req_fn(EAF_MSG_C2I((rec_req)->data.msg));\
	} while (0)

#define EAF_SERVICE_HANDLE_RSP(_group, rec_rsp)	\
	do {\
		(rec_rsp)->data.msg->info.rsp.rsp_fn(EAF_MSG_C2I((rec_rsp)->data.msg));\
	} while (0)

#define EAF_SERVICE_HANDLE_EVT(_group, _rec_evt)	\
	do {\
		eaf_subscribe_record_t tmp_record;\
		tmp_record.data.evt_id = (_rec_evt)->data.msg->msg.id;\
		tmp_record.data.service = (_rec_evt)->data.service;\
		tmp_record.data.proc = NULL;\
		tmp_record.data.priv = NULL;\
		\
		eaf_mutex_enter(&(_group)->objlock);\
		for ((_rec_evt)->data.service->subscribe.cbiter = eaf_map_find_upper(&(_group)->subscribe.table, &tmp_record.node);\
			(_rec_evt)->data.service->subscribe.cbiter != NULL;\
			(_rec_evt)->data.service->subscribe.cbiter = eaf_map_next(&(_group)->subscribe.table, (_rec_evt)->data.service->subscribe.cbiter)) {\
			eaf_subscribe_record_t* orig = EAF_CONTAINER_OF((_rec_evt)->data.service->subscribe.cbiter, eaf_subscribe_record_t, node);\
			if (orig->data.evt_id != (_rec_evt)->data.msg->msg.id || orig->data.service != (_rec_evt)->data.service){\
				break;\
			}\
			\
			eaf_mutex_leave(&(_group)->objlock);\
			orig->data.proc(EAF_MSG_C2I((_rec_evt)->data.msg), orig->data.priv);\
			eaf_mutex_enter(&(_group)->objlock);\
		}\
		(_rec_evt)->data.service->subscribe.cbiter = NULL;\
		eaf_mutex_leave(&(_group)->objlock);\
	} while (0)

#define JMP_STATE_DIRECT			0			/** ��Ȼ���� */
#define JMP_STATE_SWITCH			1			/** ��ת���� */

typedef enum eaf_service_state
{
	eaf_service_state_idle,						/** ���� */
	eaf_service_state_wait,						/** �ȴ� */
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
	uint32_t						service_id;	/** ����ID */
	const eaf_service_info_t*		load;		/** ������Ϣ */
	eaf_service_state_t				state;		/** ״̬ */

	struct
	{
		eaf_list_t					msg_cache;	/** �������Ϣ */
		eaf_list_node_t				node;		/** ����ʽ�ڵ� */
		eaf_jmpbuf_t				jmpbuf;		/** ��ת������ */
	}filber;

	struct
	{
		eaf_map_node_t*				cbiter;		/** �ص��α꣬eaf_subscribe_record_t */
	}subscribe;

	struct
	{
		size_t						capacity;	/** ��Ϣ�������� */
		size_t						size;		/** ��Ϣ�������� */
	}msgq;
}eaf_service_t;

typedef struct eaf_service_group
{
	eaf_mutex_t						objlock;	/** �߳��� */
	eaf_thread_t					working;	/** �����߳� */

	struct 
	{
		eaf_service_t*				cur_run;	/** ��ǰ���ڴ���ķ��� */
		eaf_msgq_record_t*			cur_msg;	/** ���ڴ������Ϣ */

		eaf_list_t					ready_list;	/** ִ���б� */
		eaf_jmpbuf_t				jmpbuf;		/** ��ת������ */
	}filber;

	struct
	{
		eaf_map_t					table;		/** ���ı�eaf_subscribe_record_t */
	}subscribe;

	struct
	{
		eaf_sem_t					sem;		/** �ź��� */
		eaf_list_t					que;		/** ��Ϣ���� */
	}msgq;

	struct
	{
		size_t						size;		/** ������� */
		eaf_service_t				table[];	/** ����� */
	}service;
}eaf_service_group_t;

typedef struct eaf_ctx
{
	eaf_ctx_state_t					state;		/** ״̬ */
	eaf_sem_t						ready;		/** �˳��ź� */
	eaf_thread_storage_t			tls;		/** �߳�˽�б��� */

	struct
	{
		size_t						size;		/** �����鳤�� */
		eaf_service_group_t**		table;		/** ������ */
	}group;
}eaf_ctx_t;

static eaf_ctx_t* g_eaf_ctx			= NULL;		/** ȫ�����л��� */

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

static eaf_msgq_record_t* _eaf_service_pop_record(eaf_service_group_t* group)
{
	eaf_list_node_t* it = NULL;
	eaf_mutex_enter(&group->objlock);
	do
	{
		/* ������һ�����е�����������ִ��״̬������Ҫ���������Ϣ���� */
		if (group->filber.cur_run != NULL && group->filber.cur_run->state == eaf_service_state_idle)
		{
			it = eaf_list_pop_front(&group->filber.cur_run->filber.msg_cache);
		}
		if (it == NULL)
		{
			it = eaf_list_pop_front(&group->msgq.que);
		}
	} while (0);
	eaf_mutex_leave(&group->objlock);

	if (it == NULL)
	{
		return NULL;
	}

	eaf_msgq_record_t* record = EAF_CONTAINER_OF(it, eaf_msgq_record_t, node);
	if (record->data.service->state != eaf_service_state_idle)
	{
		eaf_list_push_back(&group->filber.cur_run->filber.msg_cache, &record->node);
		return NULL;
	}

	return record;
}

static unsigned _eaf_service_thread_process_normal_resume(eaf_service_group_t* group)
{
	/*
	* �жϾ����������Ƿ���ڷ���
	* ���������д��ڵķ���������һ��δ�������Ϣ
	*/
	eaf_service_t* service;
	eaf_mutex_enter(&group->objlock);
	do 
	{
		eaf_list_node_t* it = eaf_list_pop_front(&group->filber.ready_list);
		service = it != NULL ? EAF_CONTAINER_OF(it, eaf_service_t, filber.node) : NULL;
	} while (0);
	eaf_mutex_leave(&group->objlock);

	if (service == NULL)
	{
		return -1;
	}

	/* �ָ������� */
	group->filber.cur_run = service;
	group->filber.cur_msg = EAF_CONTAINER_OF(eaf_list_pop_front(&service->filber.msg_cache), eaf_msgq_record_t, node);

	/* ��ת */
	eaf_longjmp(&service->filber.jmpbuf, JMP_STATE_SWITCH);
	assert(0);

	return 0;
}

/**
* �¼�ѭ�����˴��ᴦ���߳������з�������ݡ�
* ����Э���л���������setjmp��������ǰջ֡��LR�������setjmp��longjmp֮�����ջ��δ���1����ᵼ���쳣
* �����Ϣ��������ڱ�������ֱ��ִ��
* @parm group	������
*/
static void _eaf_service_thread_looping(eaf_service_group_t* group)
{
	eaf_setjmp(&group->filber.jmpbuf);

	while (g_eaf_ctx->state == eaf_ctx_state_busy)
	{
		unsigned timeout_1 = _eaf_service_thread_process_normal_resume(group);

		unsigned timeout_2 = 0;
		do 
		{
			group->filber.cur_msg = _eaf_service_pop_record(group);
			if (group->filber.cur_msg == NULL)
			{
				timeout_2 = -1;
				break;
			}
			group->filber.cur_run = group->filber.cur_msg->data.service;

			/* �������ʱֱ�Ӵ��� */
			switch (group->filber.cur_msg->data.msg->msg.type)
			{
			case eaf_msg_type_req:
				EAF_SERVICE_HANDLE_REQ(group, group->filber.cur_msg);
				break;

			case eaf_msg_type_rsp:
				EAF_SERVICE_HANDLE_RSP(group, group->filber.cur_msg);
				break;

			case eaf_msg_type_evt:
				EAF_SERVICE_HANDLE_EVT(group, group->filber.cur_msg);
				break;
			}

			eaf_msg_dec_ref(EAF_MSG_C2I(group->filber.cur_msg->data.msg));
			EAF_FREE(group->filber.cur_msg);
			group->filber.cur_msg = NULL;
		} while (0);

		eaf_sem_pend(&group->msgq.sem, timeout_1 < timeout_2 ? timeout_1 : timeout_2);
	}
}

/**
* �����߳�
* @param arg	eaf_service_group_t
*/
static void _eaf_service_thread(void* arg)
{
	size_t i;
	eaf_service_group_t* group = arg;

	/* �����߳�˽�б��� */
	assert(eaf_thread_storage_set(&g_eaf_ctx->tls, group) == 0);

	/* �ȴ����� */
	while (g_eaf_ctx->state == eaf_ctx_state_init)
	{
		eaf_sem_pend(&group->msgq.sem, -1);
	}

	if (g_eaf_ctx->state != eaf_ctx_state_busy)
	{
		return;
	}

	/* ��ʼ����ǰ�̰߳����ķ��� */
	size_t init_idx;
	size_t counter = 0;
	for (init_idx = 0; init_idx < group->service.size; init_idx++)
	{
		if (group->service.table[init_idx].load == NULL
			|| group->service.table[init_idx].load->on_init == NULL)
		{
			continue;
		}

		/* ��ǵ�ǰ�������еķ��� */
		group->filber.cur_run = &group->service.table[init_idx];
		if (group->filber.cur_run->load->on_init() < 0)
		{
			goto cleanup;
		}

		counter++;
	}

	/* ͨ���ʼ����� */
	eaf_sem_post(&g_eaf_ctx->ready);

	/* �����߳�δ�����κη������˳� */
	if (counter == 0)
	{
		return;
	}

	/* ������Ϣ */
	_eaf_service_thread_looping(group);

cleanup:
	for (i = 0; i < init_idx; i++)
	{
		if (group->service.table[i].load != NULL
			&& group->service.table[i].load->on_exit != NULL)
		{
			group->filber.cur_run = &group->service.table[i];
			group->service.table[i].load->on_exit();
		}
	}
}

static void _eaf_service_cleanup_group(eaf_service_group_t* group)
{
	/* ����������Ա�֤�̸߳�֪��״̬�ı� */
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

	/* ������Դ */
	eaf_thread_exit(&group->working);
	eaf_mutex_exit(&group->objlock);
	eaf_sem_exit(&group->msgq.sem);
}

static int _eaf_service_push_msg(eaf_service_group_t* group, eaf_service_t* service, eaf_msg_full_t* msg,
	int lock, void(*on_create)(eaf_msgq_record_t* record, void* arg), void* arg)
{
	/* �����Ϣ�������� */
	if (service->msgq.size >= service->msgq.capacity)
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
		eaf_list_push_back(&group->msgq.que, &record->node);
		service->msgq.size++;
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

static eaf_service_group_t* _eaf_get_current_group(void)
{
	return eaf_thread_storage_get(&g_eaf_ctx->tls);
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

	/* ���������ڴ� */
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
	assert(eaf_sem_init(&g_eaf_ctx->ready, 0) == 0);
	assert(eaf_thread_storage_init(&g_eaf_ctx->tls) == 0);

	/* �޸������ַ */
	g_eaf_ctx->group.size = size;
	g_eaf_ctx->group.table = (eaf_service_group_t**)(g_eaf_ctx + 1);
	g_eaf_ctx->group.table[0] = (eaf_service_group_t*)((char*)g_eaf_ctx->group.table + sizeof(eaf_service_group_t*) * size);
	for (i = 1; i < size; i++)
	{
		g_eaf_ctx->group.table[i] = (eaf_service_group_t*)
			((char*)g_eaf_ctx->group.table[i - 1] + sizeof(eaf_service_group_t) + sizeof(eaf_service_t) * info[i - 1].service.size);
	}

	/* ��Դ��ʼ�� */
	for (i = 0; i < size; i++)
	{
		g_eaf_ctx->group.table[i]->service.size = info[i].service.size;
		eaf_list_init(&g_eaf_ctx->group.table[i]->msgq.que);
		eaf_list_init(&g_eaf_ctx->group.table[i]->filber.ready_list);
		eaf_map_init(&g_eaf_ctx->group.table[i]->subscribe.table, _eaf_service_on_cmp_subscribe_record, NULL);
		assert(eaf_mutex_init(&g_eaf_ctx->group.table[i]->objlock, eaf_mutex_attr_normal) == 0);
		assert(eaf_sem_init(&g_eaf_ctx->group.table[i]->msgq.sem, 0) == 0);
		assert(eaf_thread_init(&g_eaf_ctx->group.table[i]->working, NULL, _eaf_service_thread, g_eaf_ctx->group.table[i]) == 0);

		size_t idx;
		for (idx = 0; idx < info[i].service.size; idx++)
		{
			g_eaf_ctx->group.table[i]->service.table[idx].service_id = info[i].service.table[idx].srv_id;
			g_eaf_ctx->group.table[i]->service.table[idx].msgq.capacity = info[i].service.table[idx].msgq_size;
			eaf_list_init(&g_eaf_ctx->group.table[i]->service.table[idx].filber.msg_cache);
		}
	}

	return eaf_errno_success;
}

int eaf_load(void)
{
	size_t i;
	if (g_eaf_ctx == NULL)
	{
		return eaf_errno_state;
	}

	/* ���������߳� */
	g_eaf_ctx->state = eaf_ctx_state_busy;
	for (i = 0; i < g_eaf_ctx->group.size; i++)
	{
		eaf_sem_post(&g_eaf_ctx->group.table[i]->msgq.sem);
	}

	/* �ȴ����з������ */
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

int eaf_service_register(uint32_t id, const eaf_service_info_t* info)
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

int eaf_service_subscribe(uint32_t srv_id, uint32_t evt_id, eaf_evt_handle_fn fn, void* arg)
{
	/* �ǹ���״̬��ֹ���� */
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

int eaf_service_unsubscribe(uint32_t srv_id, uint32_t evt_id, eaf_evt_handle_fn fn, void* arg)
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

		/* ��ɾ����¼����������������򽫷��������ǰ�� */
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

int eaf_service_send_req(uint32_t from, uint32_t to, eaf_msg_t* req)
{
	eaf_msg_full_t* real_msg = EAF_MSG_I2C(req);
	req->from = from;
	req->to = to;

	/* ��ѯ���շ��� */
	eaf_service_group_t* group;
	eaf_service_t* service = _eaf_service_find_service(to, &group);
	if (service == NULL)
	{
		return eaf_errno_notfound;
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

	/* ������Ϣ */
	return _eaf_service_push_msg(group, service, real_msg, 1, _eaf_service_on_req_record_create, msg_proc);
}

int eaf_service_send_rsp(uint32_t from, eaf_msg_t* rsp)
{
	eaf_msg_full_t* real_msg = EAF_MSG_I2C(rsp);
	rsp->from = from;

	/* ��ѯ���շ��� */
	eaf_service_group_t* group;
	eaf_service_t* service = _eaf_service_find_service(rsp->to, &group);
	if (service == NULL)
	{
		return eaf_errno_notfound;
	}

	/* ������Ϣ */
	return _eaf_service_push_msg(group, service, real_msg, 1, NULL, NULL);
}

int eaf_service_send_evt(uint32_t from, eaf_msg_t* evt)
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

struct eaf_jmpbuf* eaf_service_get_jmpbuf(void)
{
	eaf_service_t* service = _eaf_get_current_service(NULL);
	return service != NULL ? &service->filber.jmpbuf : NULL;
}

void eaf_service_context_switch(void)
{
	eaf_service_group_t* group;
	eaf_service_t* service = _eaf_get_current_service(&group);
	if (service == NULL)
	{
		return;
	}

	eaf_mutex_enter(&group->objlock);
	do
	{
		/* �л�״̬ */
		service->state = eaf_service_state_wait;

		/* ���浱ǰ���ڴ������Ϣ */
		eaf_list_push_back(&group->filber.cur_run->filber.msg_cache, &group->filber.cur_msg->node);
		group->filber.cur_msg = NULL;
	} while (0);
	eaf_mutex_leave(&group->objlock);

	eaf_longjmp(&group->filber.jmpbuf, JMP_STATE_SWITCH);
}

int eaf_filber_resume(uint32_t srv_id)
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
		if (service->state != eaf_service_state_wait)
		{
			ret = eaf_errno_state;
			break;
		}

		/* �л�����״̬������������� */
		service->state = eaf_service_state_idle;
		eaf_list_push_back(&group->filber.ready_list, &service->filber.node);
	} while (0);
	eaf_mutex_leave(&group->objlock);

	return ret;
}
