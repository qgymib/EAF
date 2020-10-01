#include <string.h>
#include "quick2.h"

#define TEST_SERVICE_S1			0x00010000

#define TEST_SERVICE_S2			0x00020000

typedef struct test_service_teardown_ctx
{
	struct
	{
		unsigned	s1_exit : 1;
		unsigned	s2_exit : 1;
	}mask;
}test_service_teardown_ctx_t;

static test_service_teardown_ctx_t s_test_service_teardown_ctx;

static void _test_teardown_on_init(void)
{
}

static void _test_lazyload_s1_on_exit(void)
{
	s_test_service_teardown_ctx.mask.s1_exit = 1;
}

static void _test_lazyload_s2_on_exit(void)
{
	s_test_service_teardown_ctx.mask.s2_exit = 1;
}

TEST_FIXTURE_SETUP(eaf_teardown)
{
	QUICK_SKIP_INIT();
	QUICK_SKIP_LOAD();

	memset(&s_test_service_teardown_ctx, 0, sizeof(s_test_service_teardown_ctx));

	static eaf_service_table_t service[] = {
		{ TEST_SERVICE_S1, 16, eaf_service_attribute_alive },
		{ TEST_SERVICE_S2, 16, 0 },
	};

	static eaf_group_table_t group[] = {
		{ EAF_THREAD_ATTR_INITIALIZER, { EAF_ARRAY_SIZE(service), service } }
	};

	ASSERT_EQ_D32(eaf_init(group, EAF_ARRAY_SIZE(group)), 0);

	static eaf_entrypoint_t s1_entry = {
		0, NULL, _test_teardown_on_init, _test_lazyload_s1_on_exit
	};
	ASSERT_EQ_D32(eaf_register(TEST_SERVICE_S1, &s1_entry), 0);

	static eaf_entrypoint_t s2_entry = {
		0, NULL, _test_teardown_on_init, _test_lazyload_s2_on_exit
	};
	ASSERT_EQ_D32(eaf_register(TEST_SERVICE_S2, &s2_entry), 0);

	ASSERT_EQ_D32(eaf_load(), 0);
}

TEST_FIXTURE_TEAREDOWN(eaf_teardown)
{
}

TEST_F(eaf_teardown, teardown)
{
	ASSERT_EQ_D32(!!s_test_service_teardown_ctx.mask.s1_exit, 0);
	ASSERT_EQ_D32(!!s_test_service_teardown_ctx.mask.s2_exit, 0);

	ASSERT_EQ_D32(eaf_teardown(), 0);
	eaf_thread_sleep(500);

	ASSERT_EQ_D32(!!s_test_service_teardown_ctx.mask.s1_exit, 0);
	ASSERT_EQ_D32(!!s_test_service_teardown_ctx.mask.s2_exit, 1);

	eaf_exit(0);
	eaf_cleanup(NULL);

	ASSERT_EQ_D32(!!s_test_service_teardown_ctx.mask.s1_exit, 1);
	ASSERT_EQ_D32(!!s_test_service_teardown_ctx.mask.s2_exit, 1);
}
