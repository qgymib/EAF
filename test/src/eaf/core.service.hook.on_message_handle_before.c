#include <string.h>
#include "quick.h"
#include "quick2.h"

typedef struct test_hook_on_message_before_ctx
{
	eaf_sem_t*			check_point;
	eaf_sem_t*			wait_point;
	unsigned			hook_cnt;

	struct
	{
		unsigned		called_rsp : 1;
	}mask;
}test_hook_on_message_before_ctx_t;

static test_hook_on_message_before_ctx_t s_test_hook_on_message_before_ctx;

static void _test_hook_on_message_before_rsp(uint32_t from, uint32_t to, struct eaf_msg* msg)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(from, to, msg);
	s_test_hook_on_message_before_ctx.mask.called_rsp = 1;
	eaf_sem_post(s_test_hook_on_message_before_ctx.check_point);
}

static int _test_hook_on_message_handle_before(uint32_t from, uint32_t to, eaf_msg_t* msg)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(from, to, msg);
	s_test_hook_on_message_before_ctx.hook_cnt++;
	eaf_sem_post(s_test_hook_on_message_before_ctx.check_point);
	ASSERT_EQ_D32(eaf_sem_pend(s_test_hook_on_message_before_ctx.wait_point, 8 * 1000), 0);

	return 0;
}

TEST_FIXTURE_SETUP(eaf_hook)
{
	memset(&s_test_hook_on_message_before_ctx, 0, sizeof(s_test_hook_on_message_before_ctx));
	ASSERT_NE_PTR(s_test_hook_on_message_before_ctx.check_point = eaf_sem_create(0), NULL);
	ASSERT_NE_PTR(s_test_hook_on_message_before_ctx.wait_point = eaf_sem_create(0), NULL);

	QUICK_DEPLOY_SERVICE(0, TEST_QUICK_S0, NULL, NULL, QUICK_DEPLOY_NO_MSG);
	QUICK_DEPLOY_SERVICE(0, TEST_QUICK_S1, NULL, NULL, {
		{ TEST_QUICK_S1_REQ1, test_quick_request_template_empty }
	});

	QUICK_FORCE_INIT_EAF();

	static eaf_hook_t hook;
	memset(&hook, 0, sizeof(hook));
	hook.on_message_handle_before = _test_hook_on_message_handle_before;
	ASSERT_EQ_D32(eaf_inject(&hook, sizeof(hook)), 0);
}

TEST_FIXTURE_TEAREDOWN(eaf_hook)
{
	eaf_sem_destroy(s_test_hook_on_message_before_ctx.check_point);
	eaf_sem_destroy(s_test_hook_on_message_before_ctx.wait_point);
}

TEST_F(eaf_hook, on_message_handle_before)
{
	{
		eaf_msg_t* msg = eaf_msg_create_req(TEST_QUICK_S1_REQ1, 0, _test_hook_on_message_before_rsp);
		ASSERT_NE_PTR(msg, NULL);

		ASSERT_EQ_D32(eaf_send_req(TEST_QUICK_S0, TEST_QUICK_S1, msg), eaf_errno_success);
		eaf_msg_dec_ref(msg);
	}

	/* Step 1. hook for request */
	ASSERT_EQ_D32(eaf_sem_pend(s_test_hook_on_message_before_ctx.check_point, 1 * 1000), 0);
	ASSERT_EQ_U32(s_test_hook_on_message_before_ctx.hook_cnt, 1);
	ASSERT_EQ_D32(eaf_sem_post(s_test_hook_on_message_before_ctx.wait_point), 0);

	/* Step 2. callback for request and send response */
	/* Step 3. hook for response */
	ASSERT_EQ_D32(eaf_sem_pend(s_test_hook_on_message_before_ctx.check_point, 1 * 1000), 0);
	ASSERT_EQ_U32(s_test_hook_on_message_before_ctx.hook_cnt, 2);
	ASSERT_EQ_D32(eaf_sem_post(s_test_hook_on_message_before_ctx.wait_point), 0);

	/* Step 4. callback for response */
	ASSERT_EQ_D32(eaf_sem_pend(s_test_hook_on_message_before_ctx.check_point, 1 * 1000), 0);
	ASSERT_EQ_D32(!!s_test_hook_on_message_before_ctx.mask.called_rsp, 1);
}
