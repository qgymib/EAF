#include <string.h>
#include "etest/etest.h"
#include "EAF/eaf.h"

#define TEST_SERVICE_S1			0x00010000
#define TEST_SERVICE_S1_REQ		(TEST_SERVICE_S1 + 0x0001)

#define TEST_SERVICE_S2			0x00020000
#define TEST_SERVICE_S2_REQ		(TEST_SERVICE_S2 + 0x0001)

static eaf_sem_t*				s_benchmark_yield_sem_s1;
static eaf_sem_t*				s_benchmark_yield_sem_s2;
static size_t					s_benchmark_yield_count_s1;
static size_t					s_benchmark_yield_count_s2;
static size_t					s_benchmark_yield_total;

static void _test_benchmark_yield_on_rsp(eaf_msg_t* msg)
{
	(void)msg;
}

static int _test_benchmark_yield_on_init(void)
{
	return 0;
}

static void _test_benchmark_yield_on_exit(void)
{
}

static void _test_benchmark_yield_s1_on_req(struct eaf_msg* msg)
{
	(void)msg;
	eaf_reenter
	{
		/* first we need s2 continue run */
		eaf_resume(TEST_SERVICE_S2);

		for (; s_benchmark_yield_count_s1 < s_benchmark_yield_total;
			s_benchmark_yield_count_s1++)
		{
			eaf_yield eaf_resume(TEST_SERVICE_S2);
		}

		eaf_sem_post(s_benchmark_yield_sem_s1);
	};
}

static void _test_benchmark_yield_s2_on_req(struct eaf_msg* msg)
{
	(void)msg;
	eaf_reenter
	{
		/* first we need s1 continue run */
		eaf_resume(TEST_SERVICE_S1);

		for (; s_benchmark_yield_count_s2 < s_benchmark_yield_total;
			s_benchmark_yield_count_s2++)
		{
			eaf_yield eaf_resume(TEST_SERVICE_S1);
		}

		eaf_sem_post(s_benchmark_yield_sem_s2);
	};
}

static void _benchmark_yield_setup(size_t count)
{
	ASSERT_PTR_NE(s_benchmark_yield_sem_s1 = eaf_sem_create(0), NULL);
	ASSERT_PTR_NE(s_benchmark_yield_sem_s2 = eaf_sem_create(0), NULL);

	s_benchmark_yield_total = count;
	s_benchmark_yield_count_s1 = 0;
	s_benchmark_yield_count_s2 = 0;

	/* 配置EAF */
	static eaf_service_table_t service_table_1[] = {
		{ TEST_SERVICE_S1, 8 },
		{ TEST_SERVICE_S2, 8 },
	};
	static eaf_group_table_t load_table[] = {
		{ { 0, { 0, 0, 0 } }, { EAF_ARRAY_SIZE(service_table_1), service_table_1 } },
	};
	ASSERT_NUM_EQ(eaf_setup(load_table, EAF_ARRAY_SIZE(load_table)), 0);

	/* 部署服务S1 */
	static eaf_message_table_t s1_msg_table[] = {
		{ TEST_SERVICE_S1_REQ, _test_benchmark_yield_s1_on_req },
	};
	static eaf_service_info_t s1_info = {
		EAF_ARRAY_SIZE(s1_msg_table), s1_msg_table,
		_test_benchmark_yield_on_init,
		_test_benchmark_yield_on_exit,
	};
	ASSERT_NUM_EQ(eaf_register(TEST_SERVICE_S1, &s1_info), 0);

	/* 部署服务S2*/
	static eaf_message_table_t s2_msg_table[] = {
		{ TEST_SERVICE_S2_REQ, _test_benchmark_yield_s2_on_req },
	};
	static eaf_service_info_t s2_info = {
		EAF_ARRAY_SIZE(s2_msg_table), s2_msg_table,
		_test_benchmark_yield_on_init,
		_test_benchmark_yield_on_exit,
	};
	ASSERT_NUM_EQ(eaf_register(TEST_SERVICE_S2, &s2_info), 0);

	/* 加载EAF */
	ASSERT_NUM_EQ(eaf_load(), 0);
}

static void _benchmark_yield_teardown(void)
{
	/* 退出并清理 */
	ASSERT_NUM_EQ(eaf_cleanup(), 0);
	eaf_sem_destroy(s_benchmark_yield_sem_s1);
	eaf_sem_destroy(s_benchmark_yield_sem_s2);
}

TEST_CLASS_SETUP(benchmark)
{
	_benchmark_yield_setup(1000000);
}

TEST_CLASS_TEAREDOWN(benchmark)
{
	_benchmark_yield_teardown();
}

TEST_F(benchmark, DISABLED_yield_1000000)
{
	/* send to s1 */
	{
		eaf_msg_t* msg = eaf_msg_create_req(TEST_SERVICE_S1_REQ, sizeof(int), _test_benchmark_yield_on_rsp);
		ASSERT_PTR_NE(msg, NULL);
		ASSERT_NUM_EQ(eaf_send_req((uint32_t)-1, TEST_SERVICE_S1, msg), 0);
		eaf_msg_dec_ref(msg);
	}
	/* send to s2 */
	{
		eaf_msg_t* msg = eaf_msg_create_req(TEST_SERVICE_S2_REQ, sizeof(int), _test_benchmark_yield_on_rsp);
		ASSERT_PTR_NE(msg, NULL);
		ASSERT_NUM_EQ(eaf_send_req((uint32_t)-1, TEST_SERVICE_S2, msg), 0);
		eaf_msg_dec_ref(msg);
	}
	/* wait for test complete */
	{
		eaf_sem_pend(s_benchmark_yield_sem_s1, (unsigned long)-1);
		eaf_sem_pend(s_benchmark_yield_sem_s2, (unsigned long)-1);
	}
}
