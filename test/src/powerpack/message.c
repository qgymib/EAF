#include <string.h>
#include "eaf/powerpack.h"
#include "ctest/ctest.h"

#define TEST_SERVICE_S1			0xF0010000

#define TEST_SERVICE_S2			0xF0020000
#define TEST_SERVICE_S2_REQ		(TEST_SERVICE_S2 + 0x0001)

static eaf_sem_t* s_test_pp_message_sem;
static int s_test_pp_message_rsp;

static int _test_powerpack_message_s1_on_init(void)
{
	eaf_reenter
	{
		int ret;
		EAF_MESSAGE_CALL_FILBER(ret, TEST_SERVICE_S2_REQ, sizeof(int), TEST_SERVICE_S1, TEST_SERVICE_S2,
			{
				*(int*)eaf_msg_get_data(_0, NULL) = 100;
			},
			{
				s_test_pp_message_rsp = *(int*)eaf_msg_get_data(_1, NULL);
			}
		);

		ASSERT_EQ_D32(ret, 0, "error:%s(%d)", eaf_strerror(_a), _a);
		ASSERT_EQ_D32(eaf_sem_post(s_test_pp_message_sem), 0);
	};

	return 0;
}

static void _test_powerpack_message_s1_on_exit(void)
{
}

static int _test_powerpack_message_s2_on_init(void)
{
	return 0;
}

static void _test_powerpack_message_s2_on_exit(void)
{
}

static void _test_powerpack_message_s2_on_req(uint32_t from, uint32_t to, eaf_msg_t* msg)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(to);
	int val = *(int*)eaf_msg_get_data(msg, NULL);

	eaf_msg_t* rsp = eaf_msg_create_rsp(msg, sizeof(int));
	ASSERT_NE_PTR(rsp, NULL);

	*(int*)eaf_msg_get_data(rsp, NULL) = val * 2;
	ASSERT_EQ_D32(eaf_send_rsp(TEST_SERVICE_S2, from, rsp), 0);
	eaf_msg_dec_ref(rsp);
}

TEST_FIXTURE_SETUP(powerpack_message)
{
	s_test_pp_message_rsp = 0;
	ASSERT_NE_PTR(s_test_pp_message_sem = eaf_sem_create(0), NULL);

	/* 配置EAF */
	static eaf_service_table_t service_table_1[] = {
		{ TEST_SERVICE_S1, 8 },
		{ TEST_SERVICE_S2, 8 },
		{ EAF_MESSAGE_ID, 8 },
	};
	static eaf_group_table_t load_table[] = {
		{ { 0, { 0, 0, 0 } }, { EAF_ARRAY_SIZE(service_table_1), service_table_1 } },
	};
	ASSERT_EQ_D32(eaf_setup(load_table, EAF_ARRAY_SIZE(load_table)), 0);

	/* 部署服务S1 */
	static eaf_entrypoint_t s1_info = {
		0, NULL,
		_test_powerpack_message_s1_on_init,
		_test_powerpack_message_s1_on_exit,
	};
	ASSERT_EQ_D32(eaf_register(TEST_SERVICE_S1, &s1_info), 0);

	/* 部署服务S2 */
	static eaf_message_table_t s2_msg[] = {
		{ TEST_SERVICE_S2_REQ, _test_powerpack_message_s2_on_req }
	};
	static eaf_entrypoint_t s2_info = {
		EAF_ARRAY_SIZE(s2_msg), s2_msg,
		_test_powerpack_message_s2_on_init,
		_test_powerpack_message_s2_on_exit,
	};
	ASSERT_EQ_D32(eaf_register(TEST_SERVICE_S2, &s2_info), 0);

	eaf_powerpack_cfg_t powerpack_cfg = { EAF_THREAD_ATTR_INITIALIZER };
	ASSERT_EQ_D32(eaf_powerpack_init(&powerpack_cfg), 0);
	ASSERT_EQ_D32(eaf_message_init(), 0);

	/* 加载EAF */
	ASSERT_EQ_D32(eaf_load(), 0);
}

TEST_FIXTURE_TEAREDOWN(powerpack_message)
{
	eaf_cleanup();
	eaf_message_exit();
	eaf_powerpack_exit();

	eaf_sem_destroy(s_test_pp_message_sem);
}

TEST_F(powerpack_message, send_req)
{
	ASSERT_EQ_D32(eaf_sem_pend(s_test_pp_message_sem, 1000), 0);
	ASSERT_EQ_D32(s_test_pp_message_rsp, 200);
}
