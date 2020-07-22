#include "eaf/powerpack.h"
#include "ctest/ctest.h"

TEST_FIXTURE_SETUP(powerpack_time)
{
}

TEST_FIXTURE_TEAREDOWN(powerpack_time)
{
}

TEST_F(powerpack_time, gettimeofday)
{
	eaf_clock_time_t tv_1;
	ASSERT_EQ_D32(eaf_time_get(&tv_1), 0);

	eaf_clock_time_t tv_2;
	ASSERT_EQ_D32(eaf_time_get(&tv_2), 0);

	ASSERT_GE_U64(tv_2.tv_sec, tv_1.tv_sec);
}

TEST_F(powerpack_time, getsystemtime)
{
	eaf_calendar_time_t tv;
	ASSERT_EQ_D32(eaf_time_getsystem(&tv), 0);
}

TEST_F(powerpack_time, getclocktime)
{
	eaf_clock_time_t t1;
	ASSERT_EQ_D32(eaf_time_getclock(&t1), 0);
}

TEST_F(powerpack_time, clocktime_diff)
{
	eaf_clock_time_t t1;
	ASSERT_EQ_D32(eaf_time_getclock(&t1), 0);

	ASSERT_EQ_D32(eaf_time_diffclock(&t1, &t1, NULL), 0);

	eaf_thread_sleep(10);

	eaf_clock_time_t t2;
	ASSERT_EQ_D32(eaf_time_getclock(&t2), 0);

	eaf_clock_time_t dif;
	ASSERT_LT_D32(eaf_time_diffclock(&t1, &t2, &dif), 0);
	ASSERT_GT_D32(eaf_time_diffclock(&t2, &t1, &dif), 0);

	ASSERT_GT_U64(dif.tv_sec * 1000 * 1000 + dif.tv_usec, 0);
}

TEST_F(powerpack_time, clocktime_add)
{
	eaf_clock_time_t t1 = { 1, 0 };
	eaf_clock_time_t t2 = { 1, 0 };

	ASSERT_EQ_D32(eaf_time_addclock(&t1, &t2), 0);
	ASSERT_EQ_U64(t1.tv_sec, 2);
	ASSERT_EQ_U32(t1.tv_usec, 0);
}
