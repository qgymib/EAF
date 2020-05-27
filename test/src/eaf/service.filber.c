#include <string.h>
#include "ctest/ctest.h"
#include "eaf/eaf.h"

#define TEST_SERVICE_S1			0x00010000
#define TEST_SERVICE_S1_EVT		(TEST_SERVICE_S1 + 0x0001)

#define TEST_SERVICE_S2			0x00020000
#define TEST_SERVICE_S2_EVT		(TEST_SERVICE_S2 + 0x0001)

typedef struct test_filber_record
{
	eaf_list_node_t		node;
	int					ret;
}test_filber_record_t;

static eaf_sem_t*			_s_ret_sem;
static test_filber_record_t	_s_nodes[4];
static eaf_list_t			_s_ret_list;

static void _test_filber_s1_on_evt(uint32_t from, uint32_t to, struct eaf_msg* msg)
{
	(void)from; (void)to;
	eaf_reenter
	{
		_s_nodes[0].ret = *(int*)eaf_msg_get_data(msg, NULL);
		eaf_list_push_back(&_s_ret_list, &_s_nodes[0].node);

		eaf_yield;

		_s_nodes[1].ret = -1;
		eaf_list_push_back(&_s_ret_list, &_s_nodes[1].node);

		ASSERT(eaf_sem_post(_s_ret_sem) == 0);
	};
}

static void _test_filber_s2_on_evt(uint32_t from, uint32_t to, struct eaf_msg* msg)
{
	(void)from; (void)to;
	_s_nodes[2].ret = *(int*)eaf_msg_get_data(msg, NULL);
	eaf_list_push_back(&_s_ret_list, &_s_nodes[2].node);

	_s_nodes[3].ret = -2;
	eaf_list_push_back(&_s_ret_list, &_s_nodes[3].node);

	ASSERT(eaf_resume(TEST_SERVICE_S1) == 0);
}

static int _test_filber_s1_on_init(void)
{
	return 0;
}

static void _test_filber_s1_on_exit(void)
{
}

static int _test_filber_s2_on_init(void)
{
	return 0;
}

static void _test_filber_s2_on_exit(void)
{
}

TEST_CLASS_SETUP(eaf_filber)
{
	memset(_s_nodes, 0, sizeof(_s_nodes));
	eaf_list_init(&_s_ret_list);
	ASSERT_NE_PTR(_s_ret_sem = eaf_sem_create(0), NULL);

	/* 配置EAF */
	static eaf_service_table_t service_table_1[] = {
		{ TEST_SERVICE_S1, 8 },
		{ TEST_SERVICE_S2, 8 },
	};
	static eaf_group_table_t load_table[] = {
		{ { 0, { 0, 0, 0 } }, { EAF_ARRAY_SIZE(service_table_1), service_table_1 } },
	};
	ASSERT_EQ_D32(eaf_setup(load_table, EAF_ARRAY_SIZE(load_table)), 0);

	/* 部署服务S1 */
	static eaf_message_table_t s1_msg[] = {
		{ TEST_SERVICE_S1_EVT, _test_filber_s1_on_evt },
	};
	static eaf_entrypoint_t s1_info = {
		EAF_ARRAY_SIZE(s1_msg), s1_msg,
		_test_filber_s1_on_init,
		_test_filber_s1_on_exit,
	};
	ASSERT_EQ_D32(eaf_register(TEST_SERVICE_S1, &s1_info), 0);

	/* 部署服务S2*/
	static eaf_message_table_t s2_msg[] = {
		{ TEST_SERVICE_S2_EVT, _test_filber_s2_on_evt },
	};
	static eaf_entrypoint_t s2_info = {
		EAF_ARRAY_SIZE(s2_msg), s2_msg,
		_test_filber_s2_on_init,
		_test_filber_s2_on_exit,
	};
	ASSERT_EQ_D32(eaf_register(TEST_SERVICE_S2, &s2_info), 0);

	/* 加载EAF */
	ASSERT_EQ_D32(eaf_load(), 0);
}

TEST_CLASS_TEAREDOWN(eaf_filber)
{
	/* 退出并清理 */
	ASSERT_EQ_D32(eaf_cleanup(), 0);

	eaf_sem_destroy(_s_ret_sem);
}

TEST_F(eaf_filber, yield_in_event)
{
	/* 发送EVT_1 */
	{
		eaf_msg_t* s1_evt = eaf_msg_create_req(TEST_SERVICE_S1_EVT, sizeof(int), NULL);
		ASSERT_NE_PTR(s1_evt, NULL);
		*(int*)eaf_msg_get_data(s1_evt, NULL) = 99;

		ASSERT_EQ_D32(eaf_send_req((uint32_t)-1, TEST_SERVICE_S1, s1_evt), 0);
		eaf_msg_dec_ref(s1_evt);
	}
	/* 发送EVT_2 */
	{
		eaf_msg_t* s2_evt = eaf_msg_create_req(TEST_SERVICE_S2_EVT, sizeof(int), NULL);
		ASSERT_NE_PTR(s2_evt, NULL);
		*(int*)eaf_msg_get_data(s2_evt, NULL) = 88;
		ASSERT_EQ_D32(eaf_send_req((uint32_t)-1, TEST_SERVICE_S2, s2_evt), 0);
		eaf_msg_dec_ref(s2_evt);
	}

	/* 等待处理完成 */
	ASSERT_EQ_D32(eaf_sem_pend(_s_ret_sem, 8 * 1000), 0);

	ASSERT_EQ_PTR(eaf_list_pop_front(&_s_ret_list), &_s_nodes[0].node);
	ASSERT_EQ_D32(_s_nodes[0].ret, 99);

	ASSERT_EQ_PTR(eaf_list_pop_front(&_s_ret_list), &_s_nodes[2].node);
	ASSERT_EQ_D32(_s_nodes[2].ret, 88);

	ASSERT_EQ_PTR(eaf_list_pop_front(&_s_ret_list), &_s_nodes[3].node);
	ASSERT_EQ_D32(_s_nodes[3].ret, -2);

	ASSERT_EQ_PTR(eaf_list_pop_front(&_s_ret_list), &_s_nodes[1].node);
	ASSERT_EQ_D32(_s_nodes[1].ret, -1);
}
