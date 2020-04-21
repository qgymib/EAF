#include <string.h>
#include "EAF/powerpack.h"
#include "etest/etest.h"

/*
* Before Visual Studio 2015, there is a bug that a `do { } while (0)` will triger C4127 warning
* https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4127
*/
#if defined(_MSC_VER) && _MSC_VER < 1900
#	pragma warning(disable : 4127)
#endif

#define TEST_SERVICE_S1			0x00010000
#define TEST_SERVICE_S1_EVT		(TEST_SERVICE_S1 + 0x0001)

#define TEST_SERVICE_SS			0x11110000

static etest_timestamp_t	s_powerpack_timer_start;
static etest_timestamp_t	s_powerpack_timer_end;
static eaf_sem_t*			s_powerpack_timer_sem;
static unsigned long		s_sleep_time;

static int _test_powerpack_timer_on_init(void)
{
	eaf_reenter
	{
		etest_timestamp_get(&s_powerpack_timer_start);
		eaf_sleep(s_sleep_time);
		etest_timestamp_get(&s_powerpack_timer_end);

		eaf_sem_post(s_powerpack_timer_sem);
	};

	return 0;
}

static void _test_powerpack_timer_on_exit(void)
{
}

TEST_CLASS_SETUP(powerpack_timer)
{
	s_sleep_time = 50;
	ASSERT_PTR_NE(s_powerpack_timer_sem = eaf_sem_create(0), NULL);

	/* 配置EAF */
	static eaf_service_table_t service_table_1[] = {
		{ TEST_SERVICE_S1, 8 },
		{ TEST_SERVICE_SS, 8 },
	};
	static eaf_group_table_t load_table[] = {
		{ { 0, { 0, 0, 0 } }, { EAF_ARRAY_SIZE(service_table_1), service_table_1 } },
	};
	ASSERT_NUM_EQ(eaf_setup(load_table, EAF_ARRAY_SIZE(load_table)), 0);

	/* 部署服务S1 */
	static eaf_service_info_t s1_info = {
		0, NULL,
		_test_powerpack_timer_on_init,
		_test_powerpack_timer_on_exit,
	};
	ASSERT_NUM_EQ(eaf_register(TEST_SERVICE_S1, &s1_info), 0);

	eaf_powerpack_cfg_t powerpack_cfg;
	memset(&powerpack_cfg, 0, sizeof(powerpack_cfg));
	powerpack_cfg.service_id = TEST_SERVICE_SS;
	ASSERT_NUM_EQ(eaf_powerpack_init(&powerpack_cfg), 0);

	/* 加载EAF */
	ASSERT_NUM_EQ(eaf_load(), 0);
}

TEST_CLASS_TEAREDOWN(powerpack_timer)
{
	/* 退出并清理 */
	ASSERT_NUM_EQ(eaf_cleanup(), 0);
	eaf_powerpack_exit();

	eaf_sem_destroy(s_powerpack_timer_sem);
}

TEST_F(powerpack_timer, sleep)
{
	ASSERT_NUM_EQ(eaf_sem_pend(s_powerpack_timer_sem, (unsigned long)-1), 0);

	etest_timestamp_t dif;
	ASSERT_NUM_LT(etest_timestamp_dif(&s_powerpack_timer_start, &s_powerpack_timer_end, &dif), 0);

	uint64_t diff_time = dif.sec * 1000 + dif.usec / 1000;
	ASSERT_NUM_GE(diff_time, (long long)(s_sleep_time * 0.8));
}
