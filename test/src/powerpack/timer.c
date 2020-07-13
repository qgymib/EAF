#include <string.h>
#include "eaf/powerpack.h"
#include "ctest/ctest.h"
#include "quick.h"

static uint32_t s_delay_timeout;
static eaf_clock_time_t s_timer_start;
static eaf_clock_time_t s_timer_stop;
static eaf_sem_t*	s_timer_sem;

static void _test_timer_on_rsp(_In_ uint32_t from, _In_ uint32_t to, _Inout_ struct eaf_msg* msg)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(from, to, msg);

	eaf_getclocktime(&s_timer_stop);
	eaf_sem_post(s_timer_sem);
}

static int _test_timer_on_init(void)
{
	eaf_getclocktime(&s_timer_start);

	int ret;
	EAF_MESSAGE_SEND_REQUEST(ret, EAF_TIMER_MSG_DELAY_REQ, sizeof(eaf_timer_delay_req_t),
		_test_timer_on_rsp, TEST_QUICK_S0, EAF_TIMER_ID, {
			((eaf_timer_delay_req_t*)eaf_msg_get_data(_0, NULL))->msec = s_delay_timeout;
		}
	);
	ASSERT_EQ_D32(ret, 0);

	return 0;
}

static void _test_timer_on_exit(void)
{

}

TEST_FIXTURE_SETUP(powerpack_timer)
{
	s_delay_timeout = 50;
	memset(&s_timer_start, 0, sizeof(s_timer_start));
	memset(&s_timer_stop, 0, sizeof(s_timer_stop));
	ASSERT_NE_PTR(s_timer_sem = eaf_sem_create(0), NULL);

	static eaf_service_table_t service_table[] = {
		{ EAF_TIMER_ID, 8 },
		{ TEST_QUICK_S0, 8 },
		{ TEST_QUICK_S1, 8 },
	};

	static eaf_group_table_t group_table[] = {
		{ EAF_THREAD_ATTR_INITIALIZER, { EAF_ARRAY_SIZE(service_table), service_table } },
	};

	ASSERT_EQ_D32(eaf_init(group_table, EAF_ARRAY_SIZE(group_table)), 0);

	eaf_powerpack_cfg_t pp_cfg = { EAF_THREAD_ATTR_INITIALIZER };
	ASSERT_EQ_D32(eaf_powerpack_init(&pp_cfg), 0);
	ASSERT_EQ_D32(eaf_timer_init(), 0);

	static eaf_entrypoint_t entry = {
		0, NULL, _test_timer_on_init, _test_timer_on_exit,
	};
	ASSERT_EQ_D32(eaf_register(TEST_QUICK_S0, &entry), 0);

	ASSERT_EQ_D32(eaf_load(), 0);
}

TEST_FIXTURE_TEAREDOWN(powerpack_timer)
{
	eaf_exit();
	eaf_powerpack_exit();
	eaf_timer_exit();

	eaf_sem_destroy(s_timer_sem);
	s_timer_sem = NULL;
}

TEST_F(powerpack_timer, delay)
{
	ASSERT_EQ_D32(eaf_sem_pend(s_timer_sem, 100000), 0);

	eaf_clock_time_t diff;
	ASSERT_LT_D32(eaf_clocktime_diff(&s_timer_start, &s_timer_stop, &diff), 0);

	uint32_t msec = (uint32_t)diff.tv_sec * 1000 + diff.tv_usec / 1000;
	ASSERT_GE_D32(msec, s_delay_timeout * 0.9);
}
