#include <string.h>
#include "quick2.h"

#define TEST_SERVICE_S1			0xF0010000

#define TEST_SERVICE_S2			0xF0020000
#define TEST_SERVICE_S2_REQ		(TEST_SERVICE_S2 + 0x0001)

typedef struct test_send_req_ctx
{
	eaf_sem_t*	s_test_pp_message_sem;
	int			s_test_pp_message_rsp;
}test_send_req_ctx_t;

static test_send_req_ctx_t s_test_send_req_ctx;

static int _test_powerpack_message_s1_on_init(void)
{
	eaf_reenter
	{
		int ret;
		EAF_MESSAGE_CALL_FILBER(ret, TEST_SERVICE_S2, TEST_SERVICE_S2_REQ, sizeof(int),
			{
				*(int*)eaf_msg_get_data(_0, NULL) = 100;
			},
			{
				s_test_send_req_ctx.s_test_pp_message_rsp = *(int*)eaf_msg_get_data(_1, NULL);
			}
		);

		ASSERT_EQ_D32(ret, 0, "error:%s(%d)", eaf_strerror(_a), _a);
		ASSERT_EQ_D32(eaf_sem_post(s_test_send_req_ctx.s_test_pp_message_sem), 0);
	};

	return 0;
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
	memset(&s_test_send_req_ctx, 0, sizeof(s_test_send_req_ctx));
	ASSERT_NE_PTR(s_test_send_req_ctx.s_test_pp_message_sem = eaf_sem_create(0), NULL);

	QUICK_DEPLOY_SERVICE(0, TEST_SERVICE_S1, _test_powerpack_message_s1_on_init, NULL, QUICK_DEPLOY_NO_MSG);
	QUICK_DEPLOY_SERVICE(0, TEST_SERVICE_S2, NULL, NULL, {
		{ TEST_SERVICE_S2_REQ, _test_powerpack_message_s2_on_req }
	});
	QUICK_RESERVE_SERVICE(0, EAF_MESSAGE_ID);

	QUICK_FORCE_INIT_EAF();

	eaf_powerpack_cfg_t powerpack_cfg = { EAF_THREAD_ATTR_INITIALIZER };
	ASSERT_EQ_D32(eaf_powerpack_init(&powerpack_cfg), 0);
	ASSERT_EQ_D32(eaf_message_init(), 0);
}

TEST_FIXTURE_TEAREDOWN(powerpack_message)
{
	eaf_message_exit();
	eaf_powerpack_exit();

	eaf_sem_destroy(s_test_send_req_ctx.s_test_pp_message_sem);
}

TEST_F(powerpack_message, send_req)
{
	ASSERT_EQ_D32(eaf_sem_pend(s_test_send_req_ctx.s_test_pp_message_sem, 1000), 0);
	ASSERT_EQ_D32(s_test_send_req_ctx.s_test_pp_message_rsp, 200);
}
