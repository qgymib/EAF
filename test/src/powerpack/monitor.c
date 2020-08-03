#include <string.h>
#include "quick.h"
#include "quick2.h"

typedef struct test_monitor_ctx
{
	test_dial_t		check_point;

	size_t			counter;
	size_t			max_count;
	size_t			wait_round;

	struct
	{
		unsigned	pend_once : 1;
	}mask;

	char			buffer[1024];
}test_monitor_ctx_t;

static test_monitor_ctx_t s_test_monitor_ctx;

static void _test_monitor_on_stringify_rsp(uint32_t from, uint32_t to, eaf_msg_t* msg)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(from, to);

	eaf_monitor_stringify_rsp_t* rsp = eaf_msg_get_data(msg, NULL);

	ASSERT_EQ_U32(rsp->size, strlen(rsp->data));
	//printf("%s", rsp->data);

	test_dial_call(&s_test_monitor_ctx.check_point, 0);
}

static void _test_monitor_on_rsp(uint32_t from, uint32_t to, eaf_msg_t* msg)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(from, to, msg);

	/* only test `max_count' round */
	if (s_test_monitor_ctx.counter >= s_test_monitor_ctx.max_count)
	{
		test_dial_call(&s_test_monitor_ctx.check_point, 0);
		return;
	}

	/* at certain point we need to wait for check */
	if (s_test_monitor_ctx.counter == s_test_monitor_ctx.wait_round)
	{
		int ret;
		EAF_MESSAGE_SEND_REQUEST(ret, EAF_MINITOR_MSG_STRINGIFY_REQ, sizeof(eaf_monitor_stringify_req_t), _test_monitor_on_stringify_rsp, TEST_QUICK_S0, EAF_MONITOR_ID, {
			((eaf_monitor_stringify_req_t*)eaf_msg_get_data(_0, NULL))->type = eaf_monitor_stringify_type_normal;
		});
		ASSERT_EQ_D32(ret, 0);
	}

	/* Here we still need service is running */
	int ret;
	EAF_MESSAGE_SEND_REQUEST(ret, TEST_QUICK_S1_REQ1, 0, _test_monitor_on_rsp, TEST_QUICK_S0, TEST_QUICK_S1, );
	ASSERT_EQ_D32(ret, 0);

	s_test_monitor_ctx.counter++;
}

static int _test_monitor_on_init(void)
{
	int ret;
	EAF_MESSAGE_SEND_REQUEST(ret, TEST_QUICK_S1_REQ1, 0, _test_monitor_on_rsp, TEST_QUICK_S0, TEST_QUICK_S1, );
	ASSERT_EQ_D32(ret, 0);
	return 0;
}

TEST_FIXTURE_SETUP(powerpack_monitor)
{
	memset(&s_test_monitor_ctx, 0, sizeof(s_test_monitor_ctx));
	ASSERT_EQ_D32(test_dial_init(&s_test_monitor_ctx.check_point), 0);
	s_test_monitor_ctx.max_count = 1000;
	s_test_monitor_ctx.wait_round = 500;

	QUICK_DEPLOY_SERVICE(0, TEST_QUICK_S0, _test_monitor_on_init, NULL, QUICK_DEPLOY_NO_MSG);
	QUICK_DEPLOY_SERVICE(1, TEST_QUICK_S1, NULL, NULL, {
		{ TEST_QUICK_S1_REQ1, test_quick_request_template_empty }
	});
	QUICK_DEPLOY_SERVICE(1, TEST_QUICK_S2, NULL, NULL, QUICK_DEPLOY_NO_MSG);
	QUICK_DEPLOY_SERVICE(1, TEST_QUICK_S3, NULL, NULL, QUICK_DEPLOY_NO_MSG);
	QUICK_RESERVE_SERVICE(0, EAF_MONITOR_ID);
	QUICK_FORCE_INIT_EAF();

	eaf_powerpack_cfg_t pp_cfg = { EAF_THREAD_ATTR_INITIALIZER };
	ASSERT_EQ_D32(eaf_powerpack_init(&pp_cfg), 0);

	ASSERT_EQ_D32(eaf_monitor_init(5), 0);
}

TEST_FIXTURE_TEAREDOWN(powerpack_monitor)
{
	eaf_monitor_exit();
	eaf_powerpack_exit();

	test_dial_exit(&s_test_monitor_ctx.check_point);
}

TEST_F(powerpack_monitor, stringify_normal)
{
	void* token;

	/* Wait for check point */
	{
		test_dial_wait(&s_test_monitor_ctx.check_point, &token);
		test_dial_answer(&s_test_monitor_ctx.check_point, token, 0);
	}

	/* Wait for check point */
	{
		test_dial_wait(&s_test_monitor_ctx.check_point, &token);
		test_dial_answer(&s_test_monitor_ctx.check_point, token, 0);
	}
}

