#include <string.h>
#include "EAF/powerpack.h"
#include "etest/etest.h"


#define TEST_SERVICE_S1			0x00010000

#define TEST_SERVICE_S2			0x00020000
#define TEST_SERVICE_S2_REQ		(TEST_SERVICE_S2 + 0x0001)

#define TEST_SERVICE_SS			0x11110000

static int			s_powerpack_message_val_req_exp;
static int			s_powerpack_message_val_rsp_exp;
static int			s_powerpack_message_val_rsp_rel;
static eaf_sem_t*	s_powerpack_message_sem;

static int _test_powerpack_message_s1_on_init(void)
{
	eaf_reenter
	{
		int ret;
		eaf_msg_call(ret, &s_powerpack_message_val_rsp_rel,
			TEST_SERVICE_S2, TEST_SERVICE_S2_REQ, int, s_powerpack_message_val_req_exp);

		ASSERT(ret == 0);
		ASSERT(eaf_sem_post(s_powerpack_message_sem) == 0);
	};

	return 0;
}

static void _test_powerpack_message_s2_on_req(struct eaf_msg* msg)
{
	int val_req = *(int*)eaf_msg_get_data(msg, NULL);
	ASSERT(val_req == s_powerpack_message_val_req_exp);

	eaf_msg_t* rsp = eaf_msg_create_rsp(msg, sizeof(int));
	ASSERT(rsp != NULL);

	*(int*)eaf_msg_get_data(rsp, NULL) = s_powerpack_message_val_rsp_exp;

	int ret = eaf_send_rsp(TEST_SERVICE_S2, rsp);
	ASSERT(ret == 0);
	eaf_msg_dec_ref(rsp);
}

static int _test_powerpack_message_s2_on_init(void)
{
	return 0;
}

static void _test_powerpack_message_on_exit(void)
{
}

TEST_CLASS_SETUP(powerpack_message)
{
	s_powerpack_message_val_req_exp = 33;
	s_powerpack_message_val_rsp_exp = 66;
	s_powerpack_message_val_rsp_rel = 99;
	ASSERT_PTR_NE(s_powerpack_message_sem = eaf_sem_create(0), NULL);

	/* 配置EAF */
	static eaf_service_table_t service_table_1[] = {
		{ TEST_SERVICE_S1, 8 },
		{ TEST_SERVICE_S2, 8 },
		{ TEST_SERVICE_SS, 8 },
	};
	static eaf_group_table_t load_table[] = {
		{ { 0, -1, 0 }, { EAF_ARRAY_SIZE(service_table_1), service_table_1 } },
	};
	ASSERT_NUM_EQ(eaf_setup(load_table, EAF_ARRAY_SIZE(load_table)), 0);

	/* 部署服务S1 */
	static eaf_service_info_t s1_info = {
		0, NULL,
		_test_powerpack_message_s1_on_init,
		_test_powerpack_message_on_exit,
	};
	ASSERT_NUM_EQ(eaf_register(TEST_SERVICE_S1, &s1_info), 0);

	/* 部署服务S2 */
	static eaf_message_table_t s2_msg[] = {
		{ TEST_SERVICE_S2_REQ, _test_powerpack_message_s2_on_req }
	};
	static eaf_service_info_t s2_info = {
		EAF_ARRAY_SIZE(s2_msg), s2_msg,
		_test_powerpack_message_s2_on_init,
		_test_powerpack_message_on_exit,
	};
	ASSERT_NUM_EQ(eaf_register(TEST_SERVICE_S2, &s2_info), 0);

	eaf_powerpack_cfg_t powerpack_cfg;
	memset(&powerpack_cfg, 0, sizeof(powerpack_cfg));
	powerpack_cfg.service_id = TEST_SERVICE_SS;
	ASSERT_NUM_EQ(eaf_powerpack_init(&powerpack_cfg), 0);

	/* 加载EAF */
	ASSERT_NUM_EQ(eaf_load(), 0);
}

TEST_CLASS_TEAREDOWN(powerpack_message)
{
	/* 退出并清理 */
	ASSERT_NUM_EQ(eaf_cleanup(), 0);
	eaf_powerpack_exit();

	eaf_sem_destroy(s_powerpack_message_sem);
}

TEST_F(powerpack_message, send_req)
{
	ASSERT_NUM_EQ(eaf_sem_pend(s_powerpack_message_sem, -1), 0);
	ASSERT_NUM_EQ(s_powerpack_message_val_rsp_rel, s_powerpack_message_val_rsp_exp);
}
