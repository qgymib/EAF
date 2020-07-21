#include "quick.h"
#include "quick2.h"

typedef struct test_monitor_ctx
{
	char	buffer[1024];
}test_monitor_ctx_t;

static test_monitor_ctx_t s_test_monitor_ctx;

TEST_FIXTURE_SETUP(powerpack_monitor)
{
	QUICK_DEPLOY_SERVICE(0, TEST_QUICK_S0, NULL, NULL, QUICK_DEPLOY_NO_MSG);
	QUICK_DEPLOY_SERVICE(0, TEST_QUICK_S1, NULL, NULL, QUICK_DEPLOY_NO_MSG);
	QUICK_DEPLOY_SERVICE(1, TEST_QUICK_S2, NULL, NULL, QUICK_DEPLOY_NO_MSG);
	QUICK_DEPLOY_SERVICE(1, TEST_QUICK_S3, NULL, NULL, QUICK_DEPLOY_NO_MSG);
	QUICK_FORCE_INIT_EAF();

	eaf_powerpack_cfg_t pp_cfg = { EAF_THREAD_ATTR_INITIALIZER };
	ASSERT_EQ_D32(eaf_powerpack_init(&pp_cfg), 0);

	ASSERT_EQ_D32(eaf_monitor_init(), 0);
}

TEST_FIXTURE_TEAREDOWN(powerpack_monitor)
{
	eaf_monitor_exit();
	eaf_powerpack_exit();
}

TEST_F(powerpack_monitor, DISABLED_print_tree)
{
	eaf_monitor_print_tree(s_test_monitor_ctx.buffer, sizeof(s_test_monitor_ctx.buffer));
	printf("%s", s_test_monitor_ctx.buffer);
}
