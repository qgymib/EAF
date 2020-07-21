#include <string.h>
#include "quick2.h"

#define TEST_SERVICE_S1			0x00010000
#define TEST_SERVICE_S1_EVT		(TEST_SERVICE_S1 + 0x0001)

#define TEST_SERVICE_S2			0x00020000
#define TEST_SERVICE_S2_EVT		(TEST_SERVICE_S2 + 0x0001)

typedef struct test_filber_record
{
	eaf_list_node_t			node;
	int						ret;
}test_filber_record_t;

typedef struct test_serice_filter_ctx
{
	eaf_sem_t*				_s_ret_sem;
	test_filber_record_t	_s_nodes[4];
	eaf_list_t				_s_ret_list;
}test_serice_filter_ctx_t;

static test_serice_filter_ctx_t s_test_serice_filter_ctx;

static void _test_filber_s1_on_evt(uint32_t from, uint32_t to, struct eaf_msg* msg)
{
	(void)from; (void)to;
	eaf_reenter
	{
		s_test_serice_filter_ctx._s_nodes[0].ret = *(int*)eaf_msg_get_data(msg, NULL);
		eaf_list_push_back(&s_test_serice_filter_ctx._s_ret_list, &s_test_serice_filter_ctx._s_nodes[0].node);

		eaf_yield;

		s_test_serice_filter_ctx._s_nodes[1].ret = -1;
		eaf_list_push_back(&s_test_serice_filter_ctx._s_ret_list, &s_test_serice_filter_ctx._s_nodes[1].node);

		ASSERT(eaf_sem_post(s_test_serice_filter_ctx._s_ret_sem) == 0);
	};
}

static void _test_filber_s2_on_evt(uint32_t from, uint32_t to, struct eaf_msg* msg)
{
	(void)from; (void)to;
	s_test_serice_filter_ctx._s_nodes[2].ret = *(int*)eaf_msg_get_data(msg, NULL);
	eaf_list_push_back(&s_test_serice_filter_ctx._s_ret_list, &s_test_serice_filter_ctx._s_nodes[2].node);

	s_test_serice_filter_ctx._s_nodes[3].ret = -2;
	eaf_list_push_back(&s_test_serice_filter_ctx._s_ret_list, &s_test_serice_filter_ctx._s_nodes[3].node);

	ASSERT(eaf_resume(TEST_SERVICE_S1) == 0);
}

TEST_FIXTURE_SETUP(eaf_filber)
{
	memset(&s_test_serice_filter_ctx, 0, sizeof(s_test_serice_filter_ctx));
	eaf_list_init(&s_test_serice_filter_ctx._s_ret_list);
	ASSERT_NE_PTR(s_test_serice_filter_ctx._s_ret_sem = eaf_sem_create(0), NULL);

	QUICK_DEPLOY_SERVICE(0, TEST_SERVICE_S1, NULL, NULL, {
		{ TEST_SERVICE_S1_EVT, _test_filber_s1_on_evt }
	});
	QUICK_DEPLOY_SERVICE(0, TEST_SERVICE_S2, NULL, NULL, {
		{ TEST_SERVICE_S2_EVT, _test_filber_s2_on_evt }
	});
}

TEST_FIXTURE_TEAREDOWN(eaf_filber)
{
	eaf_sem_destroy(s_test_serice_filter_ctx._s_ret_sem);
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
	ASSERT_EQ_D32(eaf_sem_pend(s_test_serice_filter_ctx._s_ret_sem, 8 * 1000), 0);

	ASSERT_EQ_PTR(eaf_list_pop_front(&s_test_serice_filter_ctx._s_ret_list), &s_test_serice_filter_ctx._s_nodes[0].node);
	ASSERT_EQ_D32(s_test_serice_filter_ctx._s_nodes[0].ret, 99);

	ASSERT_EQ_PTR(eaf_list_pop_front(&s_test_serice_filter_ctx._s_ret_list), &s_test_serice_filter_ctx._s_nodes[2].node);
	ASSERT_EQ_D32(s_test_serice_filter_ctx._s_nodes[2].ret, 88);

	ASSERT_EQ_PTR(eaf_list_pop_front(&s_test_serice_filter_ctx._s_ret_list), &s_test_serice_filter_ctx._s_nodes[3].node);
	ASSERT_EQ_D32(s_test_serice_filter_ctx._s_nodes[3].ret, -2);

	ASSERT_EQ_PTR(eaf_list_pop_front(&s_test_serice_filter_ctx._s_ret_list), &s_test_serice_filter_ctx._s_nodes[1].node);
	ASSERT_EQ_D32(s_test_serice_filter_ctx._s_nodes[1].ret, -1);
}
