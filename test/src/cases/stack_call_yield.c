#include <string.h>
#include "TEST.h"
#include "EAF/eaf.h"
#include "compat/semaphore.h"

#define TEST_SERVICE_S1			0x00010000
#define TEST_SERVICE_S1_EVT		(TEST_SERVICE_S1 + 0x0001)

#define TEST_SERVICE_S2			0x00020000
#define TEST_SERVICE_S2_EVT		(TEST_SERVICE_S2 + 0x0001)

typedef struct test_stackcall_record
{
	eaf_list_node_t		node;
	int					ret;
}test_stackcall_record_t;

static eaf_sem_t			_s_ret_sem;
static test_stackcall_record_t	_s_nodes[4];
static eaf_list_t			_s_ret_list;
void*	_stack_mem;
size_t	_stack_mem_size;

TEST_NOINLINE
static void _test_stackcall_body_2(eaf_msg_t* msg)
{
	_s_nodes[0].ret = EAF_MSG_ACCESS(int, msg);
	eaf_list_push_back(&_s_ret_list, &_s_nodes[0].node);

	eaf_yield;

	_s_nodes[1].ret = -1;
	eaf_list_push_back(&_s_ret_list, &_s_nodes[1].node);

	ASSERT(eaf_sem_post(&_s_ret_sem) == 0);
}

TEST_NOINLINE
static void _test_stackcall_body_1(void* arg)
{
	/**
	* �ٴ�Ƕ��
	*/
	_test_stackcall_body_2((eaf_msg_t*)arg);

	/* �˴�����Ƕ�׺������أ���eaf_stack_call��Ч���򷵻ص�ַ���ƻ����˴���core dump */
}

TEST_NOINLINE
static void _test_stackcall_s1_on_evt(eaf_msg_t* msg, void* arg)
{
	/**
	* ͨ��ָ��ջ���ã��������ջ�н���yield
	*/
	eaf_stack_call(_stack_mem, _stack_mem_size, _test_stackcall_body_1, msg);
	eaf_return;
}

static void _test_stackcall_s2_on_evt(eaf_msg_t* msg, void* arg)
{
	_s_nodes[2].ret = EAF_MSG_ACCESS(int, msg);
	eaf_list_push_back(&_s_ret_list, &_s_nodes[2].node);

	_s_nodes[3].ret = -2;
	eaf_list_push_back(&_s_ret_list, &_s_nodes[3].node);

	ASSERT(eaf_resume(TEST_SERVICE_S1) == 0);
}

static int _test_stackcall_s1_on_init(void)
{
	ASSERT(eaf_service_subscribe(TEST_SERVICE_S1, TEST_SERVICE_S1_EVT, _test_stackcall_s1_on_evt, NULL) == 0);
	return 0;
}

static void _test_stackcall_s1_on_exit(void)
{
}

static int _test_stackcall_s2_on_init(void)
{
	ASSERT(eaf_service_subscribe(TEST_SERVICE_S2, TEST_SERVICE_S2_EVT, _test_stackcall_s2_on_evt, NULL) == 0);
	return 0;
}

static void _test_stackcall_s2_on_exit(void)
{
}

TEST_CLASS_SETUP(stack_call)
{
	memset(_s_nodes, 0, sizeof(_s_nodes));
	eaf_list_init(&_s_ret_list);
	eaf_sem_init(&_s_ret_sem, 0);

	_stack_mem_size = 64 * 1024;
	_stack_mem = malloc(_stack_mem_size);

	/* ����EAF */
	static eaf_service_table_t service_table_1[] = {
		{ TEST_SERVICE_S1, 8 },
		{ TEST_SERVICE_S2, 8 },
	};
	static eaf_thread_table_t load_table[] = {
		{ 0, -1, 0, { EAF_ARRAY_SIZE(service_table_1), service_table_1 } },
	};
	ASSERT_NUM_EQ(eaf_setup(load_table, EAF_ARRAY_SIZE(load_table)), 0);

	/* �������S1 */
	static eaf_service_info_t s1_info = {
		0, NULL,
		_test_stackcall_s1_on_init,
		_test_stackcall_s1_on_exit,
	};
	ASSERT_NUM_EQ(eaf_service_register(TEST_SERVICE_S1, &s1_info), 0);

	/* �������S2*/
	static eaf_service_info_t s2_info = {
		0, NULL,
		_test_stackcall_s2_on_init,
		_test_stackcall_s2_on_exit,
	};
	ASSERT_NUM_EQ(eaf_service_register(TEST_SERVICE_S2, &s2_info), 0);

	/* ����EAF */
	ASSERT_NUM_EQ(eaf_load(), 0);
}

TEST_CLASS_TEAREDOWN(stack_call)
{
	/* �˳������� */
	ASSERT_NUM_EQ(eaf_cleanup(), 0);

	eaf_sem_exit(&_s_ret_sem);

	free(_stack_mem);
	_stack_mem_size = 0;
}

TEST_F(stack_call, yield_in_call)
{
	/* ����EVT_1 */
	{
		eaf_msg_t* s1_evt = eaf_msg_create_evt(TEST_SERVICE_S1_EVT, sizeof(int));
		ASSERT_PTR_NE(s1_evt, NULL);
		EAF_MSG_ACCESS(int, s1_evt) = 99;
		ASSERT_NUM_EQ(eaf_service_send_evt(-1, s1_evt), 0);
		eaf_msg_dec_ref(s1_evt);
	}
	/* ����EVT_2 */
	{
		eaf_msg_t* s2_evt = eaf_msg_create_evt(TEST_SERVICE_S2_EVT, sizeof(int));
		ASSERT_PTR_NE(s2_evt, NULL);
		EAF_MSG_ACCESS(int, s2_evt) = 88;
		ASSERT_NUM_EQ(eaf_service_send_evt(-1, s2_evt), 0);
		eaf_msg_dec_ref(s2_evt);
	}

	/* �ȴ�������� */
	ASSERT_NUM_EQ(eaf_sem_pend(&_s_ret_sem, 8 * 1000), 0);

	ASSERT_PTR_EQ(eaf_list_pop_front(&_s_ret_list), &_s_nodes[0].node);
	ASSERT_NUM_EQ(_s_nodes[0].ret, 99);

	ASSERT_PTR_EQ(eaf_list_pop_front(&_s_ret_list), &_s_nodes[2].node);
	ASSERT_NUM_EQ(_s_nodes[2].ret, 88);

	ASSERT_PTR_EQ(eaf_list_pop_front(&_s_ret_list), &_s_nodes[3].node);
	ASSERT_NUM_EQ(_s_nodes[3].ret, -2);

	ASSERT_PTR_EQ(eaf_list_pop_front(&_s_ret_list), &_s_nodes[1].node);
	ASSERT_NUM_EQ(_s_nodes[1].ret, -1);
}
