#include "EAF/eaf.h"
#include "TEST.h"

static size_t _test_ret;

static void _test_stack_call(void* arg)
{
	_test_ret = *(size_t*)arg;
}

TEST(stack_call_normal)
{
	_test_ret = 0;

	size_t size = 1024;
	void* data = malloc(size);
	ASSERT_PTR_NE(data, NULL);

	eaf_stack_call(data, size, _test_stack_call, &size);

	ASSERT_NUM_EQ(size, _test_ret);
	free(data);
}
