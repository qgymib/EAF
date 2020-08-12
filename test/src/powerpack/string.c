#include <string.h>
#include "quick2.h"

static const char* source_string = "abcdefghijklmnopqrstuvwxyz";

TEST(powerpack_string, apply)
{
	/* normal apply */
	{
		size_t token = 0;
		int ret = eaf_string_apply(quick_buffer, sizeof(quick_buffer), &token, "%s", source_string);
		ASSERT_EQ_D32(ret, strlen(source_string));
		ASSERT_EQ_U32(strlen(quick_buffer), strlen(source_string));
	}
	/* trunked */
	{
		size_t token = 0;
		int ret = eaf_string_apply(quick_buffer, 1, &token, "%s", source_string);
		ASSERT_EQ_D32(ret, strlen(source_string));
		ASSERT_EQ_U32(strlen(quick_buffer), 0);
	}
	{
		quick_buffer[0] = '.';

		size_t token = 0;
		int ret = eaf_string_apply(quick_buffer, 0, &token, "%s", source_string);
		ASSERT_EQ_D32(ret, strlen(source_string));
		ASSERT_EQ_U32(quick_buffer[0], '.');
	}
}
