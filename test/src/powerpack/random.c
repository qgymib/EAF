/**
 * @file I cannot find a good way to measure random algorithm quality,
 * so this test suit is only for coverage purpose.
 */

#include "quick2.h"

TEST_FIXTURE_SETUP(powerpack_random)
{
	ASSERT_EQ_D32(eaf_random_init(0), 0);
}

TEST_FIXTURE_TEAREDOWN(powerpack_random)
{
	eaf_random_exit();
}

TEST_F(powerpack_random, random)
{
	static char buffer[5];
	eaf_random(buffer, sizeof(buffer));
	(void)eaf_random32();
	(void)eaf_random64();
}
