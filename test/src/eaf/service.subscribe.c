#include "etest/etest.h"
#include "eaf/eaf.h"

#define TEST_SERVICE_S1			0x00010000
#define TEST_SERVICE_S1_EVT		(TEST_SERVICE_S1 + 0x0001)

#define TEST_SERVICE_S2			0x00020000

static int _test_subscribe_on_init(void)
{
	return 0;
}

static void _test_subscribe_on_exit(void)
{
}

static void _test_subscribe_on_evt(eaf_msg_t* msg, void* arg)
{
	(void)msg;
	(void)arg;
}

TEST_CLASS_SETUP(eaf_service)
{
	/* 配置EAF */
	static eaf_service_table_t service_table_1[] = {
		{ TEST_SERVICE_S1, 8 },
	};
	static eaf_group_table_t load_table[] = {
		{ { 0, { 0, 0, 0 } }, { EAF_ARRAY_SIZE(service_table_1), service_table_1 } },
	};
	ASSERT_NUM_EQ(eaf_setup(load_table, EAF_ARRAY_SIZE(load_table)), 0);

	/* 部署服务S1 */
	static eaf_entrypoint_t s1_info = {
		0, NULL,
		_test_subscribe_on_init,
		_test_subscribe_on_exit,
	};
	ASSERT_NUM_EQ(eaf_register(TEST_SERVICE_S1, &s1_info), 0);

	/* 加载EAF */
	ASSERT_NUM_EQ(eaf_load(), 0);
}

TEST_CLASS_TEAREDOWN(eaf_service)
{
	/* 退出并清理 */
	ASSERT_NUM_EQ(eaf_cleanup(), 0);
}

TEST_F(eaf_service, subscribe_twice)
{
	/* 首次订阅应该成功 */
	ASSERT_NUM_EQ(eaf_subscribe(TEST_SERVICE_S1, TEST_SERVICE_S1_EVT, _test_subscribe_on_evt, NULL), eaf_errno_success);
	/* 二次订阅应该失败 */
	ASSERT_NUM_EQ(eaf_subscribe(TEST_SERVICE_S1, TEST_SERVICE_S1_EVT, _test_subscribe_on_evt, NULL), eaf_errno_duplicate);

	/* 首次取消订阅应该成功 */
	ASSERT_NUM_EQ(eaf_unsubscribe(TEST_SERVICE_S1, TEST_SERVICE_S1_EVT, _test_subscribe_on_evt, NULL), eaf_errno_success);
	/* 二次取消订阅应该失败 */
	ASSERT_NUM_EQ(eaf_unsubscribe(TEST_SERVICE_S1, TEST_SERVICE_S1_EVT, _test_subscribe_on_evt, NULL), eaf_errno_notfound);
}

TEST_F(eaf_service, subscribe_nonexist)
{
	ASSERT_NUM_LT(eaf_subscribe(TEST_SERVICE_S2, TEST_SERVICE_S1_EVT, _test_subscribe_on_evt, NULL), 0);
}

TEST_F(eaf_service, unsubscribe_nonexist)
{
	ASSERT_NUM_LT(eaf_unsubscribe(TEST_SERVICE_S2, TEST_SERVICE_S1_EVT, _test_subscribe_on_evt, NULL), 0);
}
