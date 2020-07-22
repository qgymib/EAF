#include <string.h>
#include "eaf/powerpack.h"
#include "ctest/ctest.h"
#include "quick.h"
#include "quick2.h"

typedef struct test_timer_delay_ctx
{
	uint32_t			s_delay_timeout;
	eaf_clock_time_t	s_timer_start;
	eaf_clock_time_t	s_timer_stop;
	eaf_sem_t*			s_timer_sem;
}test_timer_delay_ctx_t;

static test_timer_delay_ctx_t s_test_timer_delay_ctx;

static void _test_timer_on_rsp(_In_ uint32_t from, _In_ uint32_t to, _Inout_ struct eaf_msg* msg)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(from, to, msg);

	eaf_time_getclock(&s_test_timer_delay_ctx.s_timer_stop);
	eaf_sem_post(s_test_timer_delay_ctx.s_timer_sem);
}

static int _test_timer_on_init(void)
{
	eaf_time_getclock(&s_test_timer_delay_ctx.s_timer_start);

	int ret;
	EAF_MESSAGE_SEND_REQUEST(ret, EAF_TIMER_MSG_DELAY_REQ, sizeof(eaf_timer_delay_req_t),
		_test_timer_on_rsp, TEST_QUICK_S0, EAF_TIMER_ID, {
			((eaf_timer_delay_req_t*)eaf_msg_get_data(_0, NULL))->msec = s_test_timer_delay_ctx.s_delay_timeout;
		}
	);
	ASSERT_EQ_D32(ret, 0);

	return 0;
}

TEST_FIXTURE_SETUP(powerpack_timer)
{
	memset(&s_test_timer_delay_ctx, 0, sizeof(s_test_timer_delay_ctx));

	s_test_timer_delay_ctx.s_delay_timeout = 50;
	ASSERT_NE_PTR(s_test_timer_delay_ctx.s_timer_sem = eaf_sem_create(0), NULL);

	QUICK_RESERVE_SERVICE(0, EAF_TIMER_ID);
	QUICK_DEPLOY_SERVICE(0, TEST_QUICK_S0, _test_timer_on_init, NULL, QUICK_DEPLOY_NO_MSG);

	QUICK_FORCE_INIT_EAF();

	eaf_powerpack_cfg_t pp_cfg = { EAF_THREAD_ATTR_INITIALIZER };
	ASSERT_EQ_D32(eaf_powerpack_init(&pp_cfg), 0);
	ASSERT_EQ_D32(eaf_timer_init(), 0);
}

TEST_FIXTURE_TEAREDOWN(powerpack_timer)
{
	eaf_powerpack_exit();
	eaf_timer_exit();

	eaf_sem_destroy(s_test_timer_delay_ctx.s_timer_sem);
}

TEST_F(powerpack_timer, delay)
{
	ASSERT_EQ_D32(eaf_sem_pend(s_test_timer_delay_ctx.s_timer_sem, 100000), 0);

	eaf_clock_time_t diff;
	ASSERT_LT_D32(eaf_time_diffclock(&s_test_timer_delay_ctx.s_timer_start, &s_test_timer_delay_ctx.s_timer_stop, &diff), 0);

	uint32_t msec = (uint32_t)diff.tv_sec * 1000 + diff.tv_usec / 1000;
	ASSERT_GE_D32(msec, s_test_timer_delay_ctx.s_delay_timeout * 0.8);
}
