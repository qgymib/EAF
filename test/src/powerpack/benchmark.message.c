#include <string.h>
#include "EAF/powerpack.h"
#include "etest/etest.h"

/*
* Before Visual Studio 2015, there is a bug that a `do { } while (0)` will triger C4127 warning
* https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4127
*/
#if defined(_MSC_VER) && _MSC_VER <= 1900
#	pragma warning(disable : 4127)
#endif

#define TEST_SERVICE_S1			0x00010000
#define TEST_SERVICE_S1_REQ		(TEST_SERVICE_S1 + 0x0001)

#define TEST_SERVICE_S2			0x00020000
#define TEST_SERVICE_S2_REQ		(TEST_SERVICE_S2 + 0x0001)

#define TEST_SERVICE_SS			0x11110000

static size_t		s_powerpack_benchmark_message_count;
static size_t		s_powerpack_benchmark_message_total;
static eaf_sem_t*	s_powerpack_benchmark_message_sem;

static void _test_powerpack_benchmark_message_s1_on_req(struct eaf_msg* req)
{
	(void)req;
	eaf_reenter
	{
		for (;s_powerpack_benchmark_message_count < s_powerpack_benchmark_message_total;
			s_powerpack_benchmark_message_count++)
		{
			eaf_msg_t* tmp_req = eaf_msg_create_req(TEST_SERVICE_S2_REQ, 0, NULL);
			ASSERT(tmp_req != NULL);

			eaf_msg_t* rsp;
			eaf_send_req_sync(rsp, TEST_SERVICE_S1, TEST_SERVICE_S2, tmp_req, 1);

			ASSERT(rsp != NULL);
			eaf_msg_dec_ref(rsp);
		}

		eaf_sem_post(s_powerpack_benchmark_message_sem);
	};
}

static void _test_powerpack_benchmark_message_s2_on_req(struct eaf_msg* req)
{
	eaf_msg_t* rsp = eaf_msg_create_rsp(req, 0);
	ASSERT(eaf_send_rsp(TEST_SERVICE_S2, rsp) == 0);
	eaf_msg_dec_ref(rsp);
}

static int _test_powerpack_benchmark_message_on_init(void)
{
	return 0;
}

static void _test_powerpack_benchmark_message_on_exit(void)
{
}

TEST_CLASS_SETUP(benchmark)
{
	s_powerpack_benchmark_message_total = 1000000;
	s_powerpack_benchmark_message_count = 0;
	ASSERT_PTR_NE(s_powerpack_benchmark_message_sem = eaf_sem_create(0), NULL);

	/* 配置EAF */
	static eaf_service_table_t service_table_1[] = {
		{ TEST_SERVICE_S1, 8 },
		{ TEST_SERVICE_S2, 8 },
		{ TEST_SERVICE_SS, 8 },
	};
	static eaf_group_table_t load_table[] = {
		{ { 0, { 0, 0, 0 } }, { EAF_ARRAY_SIZE(service_table_1), service_table_1 } },
	};
	ASSERT_NUM_EQ(eaf_setup(load_table, EAF_ARRAY_SIZE(load_table)), 0);

	/* 部署服务S1 */
	static eaf_message_table_t s1_msg[] = {
		{ TEST_SERVICE_S1_REQ, _test_powerpack_benchmark_message_s1_on_req }
	};
	static eaf_service_info_t s1_info = {
		EAF_ARRAY_SIZE(s1_msg), s1_msg,
		_test_powerpack_benchmark_message_on_init,
		_test_powerpack_benchmark_message_on_exit,
	};
	ASSERT_NUM_EQ(eaf_register(TEST_SERVICE_S1, &s1_info), 0);

	/* 部署服务S2 */
	static eaf_message_table_t s2_msg[] = {
		{ TEST_SERVICE_S2_REQ, _test_powerpack_benchmark_message_s2_on_req }
	};
	static eaf_service_info_t s2_info = {
		EAF_ARRAY_SIZE(s2_msg), s2_msg,
		_test_powerpack_benchmark_message_on_init,
		_test_powerpack_benchmark_message_on_exit,
	};
	ASSERT_NUM_EQ(eaf_register(TEST_SERVICE_S2, &s2_info), 0);

	eaf_powerpack_cfg_t powerpack_cfg;
	memset(&powerpack_cfg, 0, sizeof(powerpack_cfg));
	powerpack_cfg.service_id = TEST_SERVICE_SS;
	ASSERT_NUM_EQ(eaf_powerpack_init(&powerpack_cfg), 0);

	/* 加载EAF */
	ASSERT_NUM_EQ(eaf_load(), 0);
}

TEST_CLASS_TEAREDOWN(benchmark)
{
	/* 退出并清理 */
	ASSERT_NUM_EQ(eaf_cleanup(), 0);
	eaf_powerpack_exit();

	eaf_sem_destroy(s_powerpack_benchmark_message_sem);
}

TEST_F(benchmark, DISABLED_powerpack_message_1000000)
{
	{
		eaf_msg_t* req = eaf_msg_create_req(TEST_SERVICE_S1_REQ, 0, NULL);
		ASSERT_PTR_NE(req, NULL);

		ASSERT_NUM_EQ(eaf_send_req(TEST_SERVICE_S1, TEST_SERVICE_S1, req), 0);
		eaf_msg_dec_ref(req);
	}

	ASSERT_NUM_EQ(eaf_sem_pend(s_powerpack_benchmark_message_sem, (unsigned long)-1), 0);
}
