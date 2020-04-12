#include "EAF/eaf.h"
#include "etest/etest.h"
#include "compat/semaphore.h"

#define TEST_SERVICE_S1			0x00010000

#define TEST_SERVICE_S2			0x00020000
#define TEST_SERVICE_S2_EVT		(TEST_SERVICE_S2 + 0x0001)

static int			_s1_ret_val;
static int			_s2_ret_val;
static eaf_sem_t	_s1_ret_sem;
static eaf_sem_t	_s2_ret_sem;

static void _test_send_event_s1_on_evt(eaf_msg_t* msg, void* arg)
{
	_s2_ret_val = EAF_PLUGIN_MSG_ACCESS(int, msg);
	eaf_sem_post(&_s2_ret_sem);
}

static int _test_send_event_s1_on_init(void)
{
	ASSERT(eaf_subscribe(TEST_SERVICE_S1, TEST_SERVICE_S2_EVT, _test_send_event_s1_on_evt, NULL) == 0);

	eaf_msg_t* evt = eaf_msg_create_evt(TEST_SERVICE_S2_EVT, sizeof(int));
	ASSERT(evt != NULL);

	EAF_PLUGIN_MSG_ACCESS(int, evt) = 199;
	ASSERT(eaf_send_evt(TEST_SERVICE_S1, evt) == 0);
	eaf_msg_dec_ref(evt);

	return 0;
}

static void _test_send_event_s1_on_exit(void)
{
	// do nothing
}

static void _test_send_event_s2_on_evt(eaf_msg_t* msg, void* arg)
{
	_s1_ret_val = EAF_PLUGIN_MSG_ACCESS(int, msg);
	eaf_sem_post(&_s1_ret_sem);
}

static int _test_send_event_s2_on_init(void)
{
	ASSERT(eaf_subscribe(TEST_SERVICE_S2, TEST_SERVICE_S2_EVT, _test_send_event_s2_on_evt, NULL) == 0);

	return 0;
}

static void _test_send_event_s2_on_exit(void)
{
	// do nothing
}

TEST_CLASS_SETUP(send_event)
{
	_s1_ret_val = 0;
	eaf_sem_init(&_s1_ret_sem, 0);

	_s2_ret_val = 0;
	eaf_sem_init(&_s2_ret_sem, 0);

	/* 配置EAF */
	static eaf_service_table_t service_table_1[] = {
		{ TEST_SERVICE_S2, 8 },	/* S2先于S1部署，保证S2先初始化 */
		{ TEST_SERVICE_S1, 8 },
	};
	static eaf_thread_table_t load_table[] = {
		{ 0, -1, 0, { EAF_ARRAY_SIZE(service_table_1), service_table_1 } },
	};
	ASSERT_NUM_EQ(eaf_setup(load_table, EAF_ARRAY_SIZE(load_table)), 0);

	/* 部署服务S1 */
	static eaf_service_info_t s1_info = {
		0, NULL,
		_test_send_event_s1_on_init,
		_test_send_event_s1_on_exit,
	};
	ASSERT_NUM_EQ(eaf_register(TEST_SERVICE_S1, &s1_info), 0);

	/* 部署服务S2*/
	static eaf_service_info_t s2_info = {
		0, NULL,
		_test_send_event_s2_on_init,
		_test_send_event_s2_on_exit,
	};
	ASSERT_NUM_EQ(eaf_register(TEST_SERVICE_S2, &s2_info), 0);

	/* 加载EAF */
	ASSERT_NUM_EQ(eaf_load(), 0);
}

TEST_CLASS_TEAREDOWN(send_event)
{
	/* 退出并清理 */
	ASSERT_NUM_EQ(eaf_cleanup(), 0);

	eaf_sem_exit(&_s1_ret_sem);
	eaf_sem_exit(&_s2_ret_sem);
}

TEST_F(send_event, check)
{
	ASSERT_NUM_EQ(eaf_sem_pend(&_s1_ret_sem, 8 * 1000), 0);
	ASSERT_NUM_EQ(eaf_sem_pend(&_s2_ret_sem, 8 * 1000), 0);

	ASSERT_NUM_EQ(_s1_ret_val, 199);
	ASSERT_NUM_EQ(_s2_ret_val, 199);
}
