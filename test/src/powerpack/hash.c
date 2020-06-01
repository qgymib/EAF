#include <string.h>
#include "eaf/powerpack.h"
#include "ctest/ctest.h"

TEST_FIXTURE_SETUP(powerpack_hash)
{
}

TEST_FIXTURE_TEAREDOWN(powerpack_hash)
{
}

TEST_F(powerpack_hash, bkdr_32)
{
	const char* str_a = __FUNCTION__;
	const char* str_b = __FUNCTION__;
	uint32_t seed = __LINE__;

	ASSERT_EQ_STR(str_a, str_b);
	ASSERT_EQ_U32(eaf_hash32_bkdr(str_a, strlen(str_a), seed), eaf_hash32_bkdr(str_b, strlen(str_b), seed));
}
