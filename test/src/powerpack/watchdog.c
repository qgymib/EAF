#include <string.h>
#include "eaf/powerpack.h"
#include "ctest/ctest.h"
#include "quick.h"
#include "quick2.h"

typedef struct test_watchdog_ctx
{
	int useless;
}test_watchdog_ctx_t;

static test_watchdog_ctx_t s_test_watchdog_ctx;

static void _test_watchdog_on_error_fn(uint32_t id, void* arg)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(id, arg);
}

TEST_FIXTURE_SETUP(powerpack_watchdog)
{
	memset(&s_test_watchdog_ctx, 0, sizeof(s_test_watchdog_ctx));

	QUICK_RESERVE_SERVICE(0, EAF_WATCHDOG_ID);
	QUICK_RESERVE_SERVICE(0, EAF_TIMER_ID);
	QUICK_FORCE_INIT_EAF();

	eaf_powerpack_cfg_t pp_cfg = { EAF_THREAD_ATTR_INITIALIZER };
	ASSERT_EQ_D32(eaf_powerpack_init(&pp_cfg), 0);

	ASSERT_EQ_D32(eaf_timer_init(), 0, "error:%s(%d)", eaf_strerror(_a), _a);
	ASSERT_EQ_D32(eaf_watchdog_init(NULL, 0, _test_watchdog_on_error_fn, NULL), 0);
}

TEST_FIXTURE_TEAREDOWN(powerpack_watchdog)
{
	eaf_watchdog_exit();
	eaf_timer_exit();
	eaf_powerpack_exit();
}

TEST_F(powerpack_watchdog, heartbeat)
{
}
