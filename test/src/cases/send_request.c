#include "TEST.h"
#include "EAF/eaf.h"
#include "compat/semaphore.h"

#define TEST_SERVICE_S1			0x00010000
#define TEST_SERVICE_S1_MSG		(TEST_SERVICE_S1 + 0x0001)

#define TEST_SERVICE_S2			0x00020000
#define TEST_SERVICE_S2_MSG		(TEST_SERVICE_S2 + 0x0001)

static int			_s_ret_val;
static eaf_sem_t	_s_ret_sem;

static void _test_send_request_s1_on_rsp(eaf_msg_t* msg)
{
	_s_ret_val = *(int*)eaf_msg_get_data(msg, NULL);
	eaf_sem_post(&_s_ret_sem);
}

static int _test_send_request_s1_on_init(void)
{
	eaf_msg_t* req = eaf_msg_create_req(TEST_SERVICE_S2_MSG, sizeof(int), _test_send_request_s1_on_rsp);
	ASSERT(req != NULL);
	EAF_MSG_ACCESS(int, req) = 99;

	ASSERT(eaf_service_send_req(TEST_SERVICE_S1, TEST_SERVICE_S2, req) == 0);
	eaf_msg_dec_ref(req);

	return 0;
}

static void _test_send_request_s1_on_exit(void)
{
	// do nothing
}

static int _test_send_request_s2_on_init(void)
{
	return 0;
}

static void _test_send_request_s2_on_exit(void)
{
	// do nothing
}

static void _test_send_request_s1_on_req(eaf_msg_t* msg)
{
	// do nothing
}

static void _test_send_request_s2_on_req(eaf_msg_t* req)
{
	eaf_msg_t* rsp = eaf_msg_create_rsp(req, sizeof(int));
	ASSERT(rsp != NULL);

	EAF_MSG_ACCESS(int, rsp) = EAF_MSG_ACCESS(int, req) * 2;

	ASSERT(eaf_service_send_rsp(TEST_SERVICE_S2, rsp) == 0);
	eaf_msg_dec_ref(rsp);
}

TEST_CLASS_SETUP(send_request)
{
	_s_ret_val = 0;
	eaf_sem_init(&_s_ret_sem, 0);

	/* 配置EAF */
	static eaf_service_table_t service_table_1[] = {
		{ TEST_SERVICE_S1, 8 },
		{ TEST_SERVICE_S2, 8 },
	};
	static eaf_thread_table_t load_table[] = {
		{ 0, -1, 0, { EAF_ARRAY_SIZE(service_table_1), service_table_1 } },
	};
	ASSERT_NUM_EQ(eaf_setup(load_table, EAF_ARRAY_SIZE(load_table)), 0);

	/* 部署服务S1 */
	static eaf_service_msgmap_t s1_msg_table[] = {
		{ TEST_SERVICE_S1_MSG, _test_send_request_s1_on_req },
	};
	static eaf_service_info_t s1_info = {
		EAF_ARRAY_SIZE(s1_msg_table), s1_msg_table,
		_test_send_request_s1_on_init,
		_test_send_request_s1_on_exit,
	};
	ASSERT_NUM_EQ(eaf_service_register(TEST_SERVICE_S1, &s1_info), 0);

	/* 部署服务S2*/
	static eaf_service_msgmap_t s2_msg_table[] = {
		{ TEST_SERVICE_S2_MSG, _test_send_request_s2_on_req },
	};
	static eaf_service_info_t s2_info = {
		EAF_ARRAY_SIZE(s2_msg_table), s2_msg_table,
		_test_send_request_s2_on_init,
		_test_send_request_s2_on_exit,
	};
	ASSERT_NUM_EQ(eaf_service_register(TEST_SERVICE_S2, &s2_info), 0);

	/* 加载EAF */
	ASSERT_NUM_EQ(eaf_load(), 0);
}

TEST_CLASS_TEAREDOWN(send_request)
{
	/* 退出并清理 */
	ASSERT_NUM_EQ(eaf_cleanup(), 0);

	eaf_sem_exit(&_s_ret_sem);
}

TEST_F(send_request, check)
{
	/* 等待结果 */
	ASSERT_NUM_EQ(eaf_sem_pend(&_s_ret_sem, 8 * 1000), 0);

	/* 检查结果 */
	ASSERT_NUM_EQ(_s_ret_val, 99 * 2);
}
