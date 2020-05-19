#include "eaf/powerpack.h"
#include "etest/etest.h"

TEST_CLASS_SETUP(powerpack_time)
{
}

TEST_CLASS_TEAREDOWN(powerpack_time)
{
}

TEST_F(powerpack_time, gettimeofday)
{
	eaf_clock_time_t tv_1;
	ASSERT_NUM_EQ(eaf_gettimeofday(&tv_1), 0);

	eaf_clock_time_t tv_2;
	ASSERT_NUM_EQ(eaf_gettimeofday(&tv_2), 0);

	ASSERT_NUM_GE(tv_2.tv_sec, tv_1.tv_sec);
}

TEST_F(powerpack_time, getsystemtime)
{
	eaf_calendar_time_t tv;
	ASSERT_NUM_EQ(eaf_getsystemtime(&tv), 0);
}