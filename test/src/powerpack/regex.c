#include "eaf/powerpack.h"
#include "etest/etest.h"

TEST(powerpack_regex, sample1)
{
	const char* origin = "pipe:s=1234567,c=7654321";
	const char* pattern = "abcd";

	ASSERT_NUM_LT(eaf_regex(pattern, origin, NULL, 0, 0), 0);
}

TEST(powerpack_regex, sample2)
{
	const char* origin = "pipe:s=1234567,c=7654321";
	const char* pattern = "s=";

	ASSERT_NUM_EQ(eaf_regex(pattern, origin, NULL, 0, 0), 0);
}

TEST(powerpack_regex, sample3)
{
	const char* origin = "pipe:s=1234567,c=7654321";
	const char* pattern = "s=(\\d+)";

	eaf_regex_match_t match[3];
	ASSERT_NUM_EQ(eaf_regex(pattern, origin, match, EAF_ARRAY_SIZE(match), eaf_regex_flag_extened), 2);

	ASSERT_NUM_EQ(match[0].rm_so, 5);
	ASSERT_NUM_EQ(match[0].rm_eo, 14);

	ASSERT_NUM_EQ(match[1].rm_so, 7);
	ASSERT_NUM_EQ(match[1].rm_eo, 14);
}
