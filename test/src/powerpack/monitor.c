#include <string.h>
#include "quick.h"
#include "quick2.h"

typedef struct test_monitor_ctx
{
	eaf_sem_t*		wait_point;
	eaf_sem_t*		check_point;

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

static void _test_monitor_on_rsp(uint32_t from, uint32_t to, eaf_msg_t* msg)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(from, to, msg);

	if (!s_test_monitor_ctx.mask.pend_once)
	{
		ASSERT_EQ_D32(eaf_sem_pend(s_test_monitor_ctx.wait_point, 8 * 1000), 0);
		s_test_monitor_ctx.mask.pend_once = 1;
	}

	/* only test `max_count' round */
	if (s_test_monitor_ctx.counter >= s_test_monitor_ctx.max_count)
	{
		return;
	}

	/* at certain point we need to wait for check */
	if (s_test_monitor_ctx.counter == s_test_monitor_ctx.wait_round)
	{
		ASSERT_EQ_D32(eaf_sem_post(s_test_monitor_ctx.check_point), 0);
		ASSERT_EQ_D32(eaf_sem_pend(s_test_monitor_ctx.wait_point, 8 * 1000), 0);
	}

	int ret;
	EAF_MESSAGE_SEND_REQUEST(ret, TEST_QUICK_S3_REQ1, 0, _test_monitor_on_rsp, TEST_QUICK_S0, TEST_QUICK_S3, );
	ASSERT_EQ_D32(ret, 0);

	s_test_monitor_ctx.counter++;
}

static int _test_monitor_on_init(void)
{
	int ret;
	EAF_MESSAGE_SEND_REQUEST(ret, TEST_QUICK_S3_REQ1, 0, _test_monitor_on_rsp, TEST_QUICK_S0, TEST_QUICK_S3, );
	return 0;
}

TEST_FIXTURE_SETUP(powerpack_monitor)
{
	memset(&s_test_monitor_ctx, 0, sizeof(s_test_monitor_ctx));
	ASSERT_NE_PTR(s_test_monitor_ctx.wait_point = eaf_sem_create(0), NULL);
	ASSERT_NE_PTR(s_test_monitor_ctx.check_point = eaf_sem_create(0), NULL);
	s_test_monitor_ctx.max_count = 1000;
	s_test_monitor_ctx.wait_round = 500;

	QUICK_DEPLOY_SERVICE(0, TEST_QUICK_S0, _test_monitor_on_init, NULL, QUICK_DEPLOY_NO_MSG);
	QUICK_DEPLOY_SERVICE(0, TEST_QUICK_S1, NULL, NULL, QUICK_DEPLOY_NO_MSG);
	QUICK_DEPLOY_SERVICE(1, TEST_QUICK_S2, NULL, NULL, QUICK_DEPLOY_NO_MSG);
	QUICK_DEPLOY_SERVICE(1, TEST_QUICK_S3, NULL, NULL, {
		{ TEST_QUICK_S3_REQ1, test_quick_request_template_empty }
	});
	QUICK_FORCE_INIT_EAF();

	eaf_powerpack_cfg_t pp_cfg = { EAF_THREAD_ATTR_INITIALIZER };
	ASSERT_EQ_D32(eaf_powerpack_init(&pp_cfg), 0);

	ASSERT_EQ_D32(eaf_monitor_init(5), 0);
}

TEST_FIXTURE_TEAREDOWN(powerpack_monitor)
{
	eaf_monitor_exit();
	eaf_powerpack_exit();

	eaf_sem_destroy(s_test_monitor_ctx.wait_point);
	eaf_sem_destroy(s_test_monitor_ctx.check_point);
}

TEST_F(powerpack_monitor, DISABLED_print_tree)
{
	eaf_sem_post(s_test_monitor_ctx.wait_point);
	ASSERT_EQ_D32(eaf_sem_pend(s_test_monitor_ctx.check_point, 16 * 1000), 0);

	eaf_monitor_print_tree(s_test_monitor_ctx.buffer, sizeof(s_test_monitor_ctx.buffer));
	printf("%s", s_test_monitor_ctx.buffer);

	eaf_sem_post(s_test_monitor_ctx.wait_point);
}

TEST_F(powerpack_monitor, print_tree)
{
	eaf_monitor_print_tree(s_test_monitor_ctx.buffer, sizeof(s_test_monitor_ctx.buffer));
}
