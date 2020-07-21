#include <string.h>
#include "quick.h"
#include "quick2.h"

typedef struct test_hook_on_message_send_before_ctx
{
	eaf_sem_t*			sem;
}test_hook_on_message_send_before_ctx_t;

static test_hook_on_message_send_before_ctx_t s_test_hook_on_message_send_before_ctx;

static int _test_hook_on_message_send(uint32_t from, uint32_t to, eaf_msg_t* msg)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(from, to, msg);

	eaf_sem_post(s_test_hook_on_message_send_before_ctx.sem);
	return 0;
}

TEST_FIXTURE_SETUP(eaf_hook)
{
	ASSERT_NE_PTR(s_test_hook_on_message_send_before_ctx.sem = eaf_sem_create(0), NULL);

	QUICK_DEPLOY_SERVICE(0, TEST_QUICK_S0, NULL, NULL, QUICK_DEPLOY_NO_MSG);
	QUICK_DEPLOY_SERVICE(0, TEST_QUICK_S1, NULL, NULL, {
		{ TEST_QUICK_S1_REQ1, test_quick_request_template_empty }
	});

	QUICK_FORCE_INIT_EAF();

	static eaf_hook_t hook;
	memset(&hook, 0, sizeof(hook));
	hook.on_message_send_before = _test_hook_on_message_send;
	ASSERT_EQ_D32(eaf_inject(&hook, sizeof(hook)), 0);
}

TEST_FIXTURE_TEAREDOWN(eaf_hook)
{
	eaf_sem_destroy(s_test_hook_on_message_send_before_ctx.sem);
}

TEST_F(eaf_hook, on_message_send_before)
{
	eaf_msg_t* msg = eaf_msg_create_req(TEST_QUICK_S1_REQ1, 0, NULL);
	ASSERT_NE_PTR(msg, NULL);

	ASSERT_EQ_D32(eaf_send_req(TEST_QUICK_S0, TEST_QUICK_S1, msg), 0);
	ASSERT_EQ_D32(eaf_sem_pend(s_test_hook_on_message_send_before_ctx.sem, 1 * 1000), 0);

	eaf_msg_dec_ref(msg);
}
