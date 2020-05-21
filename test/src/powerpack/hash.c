#include <string.h>
#include "eaf/powerpack.h"
#include "etest/etest.h"

TEST_CLASS_SETUP(powerpack_hash)
{
}

TEST_CLASS_TEAREDOWN(powerpack_hash)
{
}

TEST_F(powerpack_hash, bkdr_32)
{
	const char* str_a = __FUNCTION__;
	const char* str_b = __FUNCTION__;
	uint32_t seed = __LINE__;

	uint32_t hval_a = eaf_hash32_bkdr(str_a, strlen(str_a), seed);
	uint32_t hval_b = eaf_hash32_bkdr(str_b, strlen(str_b), seed);

	ASSERT_EQ_STR(str_a, str_b);
	ASSERT_EQ_U32(hval_a, hval_b);
}
