#include "EAF/eaf.h"
#include "compat/semaphore.h"
#include "etest/etest.h"

#define TEST_SERVICE_S1			0x00010000
#define TEST_SERVICE_S1_MSG		(TEST_SERVICE_S1 + 0x0001)

#define TEST_SERVICE_S2			0x00020000
#define TEST_SERVICE_S2_MSG		(TEST_SERVICE_S2 + 0x0001)

static int			s_plugin_msg_retval;
static eaf_sem_t	s_plugin_msg_sem;

static int _test_plugin_msg_s1_on_init(void)
{
	eaf_reenter
	{
		int req = 10;

		eaf_msg_t* rsp;
		EAF_PLUGIN_MSG_SEND_REQ(rsp, TEST_SERVICE_S1, TEST_SERVICE_S2, TEST_SERVICE_S2_MSG, req);
		ASSERT(rsp != NULL);

		s_plugin_msg_retval = EAF_PLUGIN_MSG_ACCESS(int, rsp);
		eaf_sem_post(&s_plugin_msg_sem);

		eaf_msg_dec_ref(rsp);
	};

	return 0;
}

static void _test_plugin_msg_s1_on_exit(void)
{
	// do nothing
}

static int _test_plugin_msg_s2_on_init(void)
{
	return 0;
}

static void _test_plugin_msg_s2_on_exit(void)
{
	// do nothing
}

static void _test_plugin_msg_s1_on_req(eaf_msg_t* msg)
{
	// do nothing
}

static void _test_plugin_msg_s2_on_req(eaf_msg_t* req)
{
	int rsp = EAF_PLUGIN_MSG_ACCESS(int, req) * 2;
	int ret;
	EAF_PLUGIN_MSG_SEND_RSP(ret, TEST_SERVICE_S2, req, rsp);
	ASSERT(ret == 0);
}

TEST_CLASS_SETUP(plugin_msg)
{
	s_plugin_msg_retval = 0;
	eaf_sem_init(&s_plugin_msg_sem, 0);

	/* 配置EAF */
	static eaf_service_table_t service_table_1[] = {
		{ TEST_SERVICE_S1,			8 },
		{ TEST_SERVICE_S2,			8 },
		{ EAF_PLUGIN_SERVICE,		8 },
	};
	static eaf_group_table_t load_table[] = {
		{ { 0, -1, 0 }, { EAF_ARRAY_SIZE(service_table_1), service_table_1 } },
	};
	ASSERT_NUM_EQ(eaf_setup(load_table, EAF_ARRAY_SIZE(load_table)), 0);

	/* 加载所有插件 */
	eaf_thread_attr_t plugin_cfg = { 0, 0, 0 };
	ASSERT_NUM_EQ(eaf_plugin_load(&plugin_cfg), 0);

	/* 部署服务S1 */
	static eaf_message_table_t s1_msg_table[] = {
		{ TEST_SERVICE_S1_MSG, _test_plugin_msg_s1_on_req },
	};
	static eaf_service_info_t s1_info = {
		EAF_ARRAY_SIZE(s1_msg_table), s1_msg_table,
		_test_plugin_msg_s1_on_init,
		_test_plugin_msg_s1_on_exit,
	};
	ASSERT_NUM_EQ(eaf_register(TEST_SERVICE_S1, &s1_info), 0);

	/* 部署服务S2*/
	static eaf_message_table_t s2_msg_table[] = {
		{ TEST_SERVICE_S2_MSG, _test_plugin_msg_s2_on_req },
	};
	static eaf_service_info_t s2_info = {
		EAF_ARRAY_SIZE(s2_msg_table), s2_msg_table,
		_test_plugin_msg_s2_on_init,
		_test_plugin_msg_s2_on_exit,
	};
	ASSERT_NUM_EQ(eaf_register(TEST_SERVICE_S2, &s2_info), 0);

	/* 加载EAF */
	ASSERT_NUM_EQ(eaf_load(), 0);
}

TEST_CLASS_TEAREDOWN(plugin_msg)
{
	/* 退出并清理 */
	ASSERT_NUM_EQ(eaf_cleanup(), 0);
	eaf_sem_exit(&s_plugin_msg_sem);
}

TEST_F(plugin_msg, check)
{
	ASSERT_NUM_EQ(eaf_sem_pend(&s_plugin_msg_sem, 8 * 1000), 0);
	ASSERT_NUM_EQ(s_plugin_msg_retval, 20);
}
