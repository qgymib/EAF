#include "EAF/eaf.h"
#include "etest/etest.h"

TEST(eaf_errno, exist)
{
	ASSERT_PTR_NE(eaf_strerror(eaf_errno_success), NULL);
}

TEST(eaf_errno, nonexist)
{
	ASSERT_PTR_EQ(eaf_strerror(10000), NULL);
}
