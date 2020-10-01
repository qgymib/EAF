#include <string.h>
#include "quick2.h"

#define TEST_SERVICE_S1			0x00010000
#define TEST_SERVICE_S1_REQ		(TEST_SERVICE_S1 + 0x0001)

#define TEST_SERVICE_S2			0x00020000

typedef struct test_service_lazyload_ctx
{
	struct
	{
		unsigned	init_s1 : 1;
		unsigned	init_s2 : 1;
	}mask;
}test_service_lazyload_ctx_t;

static test_service_lazyload_ctx_t s_test_service_lazyload_ctx;

static void _test_lazyload_s1_on_req(uint32_t from, uint32_t to, eaf_msg_t* msg)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(from, to, msg);
}

static void _test_lazyload_s1_on_init(void)
{
	s_test_service_lazyload_ctx.mask.init_s1 = 1;
}

static void _test_lazyload_s2_on_init(void)
{
	s_test_service_lazyload_ctx.mask.init_s2 = 1;
}

static void _test_lazyload_on_exit(void)
{
}

TEST_FIXTURE_SETUP(eaf_lazyload)
{
	memset(&s_test_service_lazyload_ctx, 0, sizeof(s_test_service_lazyload_ctx));

	QUICK_SKIP_INIT();
	QUICK_SKIP_LOAD();

	static eaf_service_table_t service[] = {
		{ TEST_SERVICE_S1, 16, eaf_service_attribute_lazyload },
		{ TEST_SERVICE_S2, 16, 0 },
	};

	static eaf_group_table_t group[] = {
		{ EAF_THREAD_ATTR_INITIALIZER, { EAF_ARRAY_SIZE(service), service } }
	};

	ASSERT_EQ_D32(eaf_init(group, EAF_ARRAY_SIZE(group)), 0);

	static eaf_message_table_t msg_table[] = {
		{ TEST_SERVICE_S1_REQ, _test_lazyload_s1_on_req }
	};
	static eaf_entrypoint_t s1_entry = {
		EAF_ARRAY_SIZE(msg_table), msg_table,
		_test_lazyload_s1_on_init, _test_lazyload_on_exit
	};
	ASSERT_EQ_D32(eaf_register(TEST_SERVICE_S1, &s1_entry), 0);

	static eaf_entrypoint_t s2_entry = {
		0, NULL, _test_lazyload_s2_on_init, _test_lazyload_on_exit
	};
	ASSERT_EQ_D32(eaf_register(TEST_SERVICE_S2, &s2_entry), 0);

	ASSERT_EQ_D32(eaf_load(), 0);
}

TEST_FIXTURE_TEAREDOWN(eaf_lazyload)
{

}

TEST_F(eaf_lazyload, lazyload)
{
	/**
	 * S1: not initialized due to #eaf_service_attribute_lazyload
	 * S1: initialized
	 */
	ASSERT_EQ_D32(!!s_test_service_lazyload_ctx.mask.init_s1, 0);
	ASSERT_EQ_D32(!!s_test_service_lazyload_ctx.mask.init_s2, 1);

	/* Send request to S1 */
	int ret;
	EAF_MESSAGE_SEND_REQUEST(ret, TEST_SERVICE_S1_REQ, 0, NULL, TEST_SERVICE_S2, TEST_SERVICE_S1, );
	ASSERT_EQ_D32(ret, 0);

	eaf_thread_sleep(500);

	ASSERT_EQ_D32(!!s_test_service_lazyload_ctx.mask.init_s1, 1);
	ASSERT_EQ_D32(!!s_test_service_lazyload_ctx.mask.init_s2, 1);
}
