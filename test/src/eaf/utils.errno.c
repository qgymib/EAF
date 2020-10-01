#include "eaf/eaf.h"
#include "ctest/ctest.h"

TEST(eaf_errno, exist)
{
	ASSERT_NE_PTR(eaf_strerror(eaf_errno_success), NULL);
}

TEST(eaf_errno, nonexist)
{
	ASSERT_EQ_PTR(eaf_strerror(10000), NULL);
}
