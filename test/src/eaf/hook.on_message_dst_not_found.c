#include <string.h>
#include "quick.h"
#include "quick2.h"

#define TEST_QUICK_S4		0xE0040000
#define TEST_QUICK_S4_REQ1	(TEST_QUICK_S4 + 0x0001)
#define TEST_QUICK_S4_REQ2	(TEST_QUICK_S4 + 0x0002)
#define TEST_QUICK_S4_REQ3	(TEST_QUICK_S4 + 0x0003)
#define TEST_QUICK_S4_REQ4	(TEST_QUICK_S4 + 0x0004)

typedef struct test_hook_on_message_dst_not_found_ctx
{
	eaf_sem_t*			sem;
}test_hook_on_message_dst_not_found_ctx_t;

static test_hook_on_message_dst_not_found_ctx_t s_test_hook_on_message_dst_not_found_ctx;

static int _test_hook_on_message_dst_not_found(uint32_t from, uint32_t to, eaf_msg_t* msg)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(from, to, msg);
	eaf_sem_post(s_test_hook_on_message_dst_not_found_ctx.sem);
	return 0;
}

TEST_FIXTURE_SETUP(eaf_hook)
{
	ASSERT_NE_PTR(s_test_hook_on_message_dst_not_found_ctx.sem = eaf_sem_create(0), NULL);

	QUICK_DEPLOY_SERVICE(0, TEST_QUICK_S0, NULL, NULL, QUICK_DEPLOY_NO_MSG);

	QUICK_FORCE_INIT_EAF();

	static eaf_hook_t hook;
	memset(&hook, 0, sizeof(hook));
	hook.on_message_dst_not_found = _test_hook_on_message_dst_not_found;
	ASSERT_EQ_D32(eaf_inject(&hook, sizeof(hook)), 0);
}

TEST_FIXTURE_TEAREDOWN(eaf_hook)
{
	eaf_sem_destroy(s_test_hook_on_message_dst_not_found_ctx.sem);
}

TEST_F(eaf_hook, on_message_dst_not_found)
{
	eaf_msg_t* msg = eaf_msg_create_req(TEST_QUICK_S4_REQ1, 0, NULL);
	ASSERT_NE_PTR(msg, NULL);

	ASSERT_EQ_D32(eaf_send_req(TEST_QUICK_S0, TEST_QUICK_S4, msg), eaf_errno_success);
	ASSERT_EQ_D32(eaf_sem_pend(s_test_hook_on_message_dst_not_found_ctx.sem, 1 * 1000), 0);

	eaf_msg_dec_ref(msg);
}
