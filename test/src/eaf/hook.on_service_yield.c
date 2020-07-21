#include <string.h>
#include "quick.h"
#include "quick2.h"

typedef struct test_hook_on_service_yield_ctx
{
	eaf_sem_t*			sem;

	struct
	{
		unsigned		called_yield : 1;
		unsigned		called_resume : 1;

		unsigned		called_before_yield : 1;
		unsigned		called_after_yield : 1;
	}mask;
}test_hook_on_service_yield_ctx_t;

static test_hook_on_service_yield_ctx_t s_test_hook_on_service_yield_ctx;

static void _test_hook_on_service_yield(uint32_t id)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(id);

	s_test_hook_on_service_yield_ctx.mask.called_yield = 1;
	eaf_sem_post(s_test_hook_on_service_yield_ctx.sem);
}

static void _test_hook_on_service_resume(uint32_t id)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(id);

	s_test_hook_on_service_yield_ctx.mask.called_resume = 1;
	eaf_sem_post(s_test_hook_on_service_yield_ctx.sem);
}

static void _test_hook_on_service_yield_hook(eaf_service_local_t* local, void* arg)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(local, arg);

	eaf_sem_post(s_test_hook_on_service_yield_ctx.sem);
}

static void _test_hook_on_service_yield_req(uint32_t from, uint32_t to, eaf_msg_t* req)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(from, to, req);

	eaf_reenter
	{
		s_test_hook_on_service_yield_ctx.mask.called_before_yield = 1;
		eaf_yield_ext(_test_hook_on_service_yield_hook, NULL);

		s_test_hook_on_service_yield_ctx.mask.called_after_yield = 1;
		eaf_sem_post(s_test_hook_on_service_yield_ctx.sem);
	};
}

TEST_FIXTURE_SETUP(eaf_hook)
{
	memset(&s_test_hook_on_service_yield_ctx, 0, sizeof(s_test_hook_on_service_yield_ctx));
	ASSERT_NE_PTR(s_test_hook_on_service_yield_ctx.sem = eaf_sem_create(0), NULL);

	QUICK_DEPLOY_SERVICE(0, TEST_QUICK_S0, NULL, NULL, QUICK_DEPLOY_NO_MSG);
	QUICK_DEPLOY_SERVICE(0, TEST_QUICK_S3, NULL, NULL, {
		{ TEST_QUICK_S3_REQ1, _test_hook_on_service_yield_req }
	});

	QUICK_FORCE_INIT_EAF();

	static eaf_hook_t hook;
	memset(&hook, 0, sizeof(hook));
	hook.on_service_yield = _test_hook_on_service_yield;
	hook.on_service_resume = _test_hook_on_service_resume;
	ASSERT_EQ_D32(eaf_inject(&hook, sizeof(hook)), 0);
}

TEST_FIXTURE_TEAREDOWN(eaf_hook)
{
	eaf_sem_destroy(s_test_hook_on_service_yield_ctx.sem);
}

TEST_F(eaf_hook, on_service_yield)
{
	{
		eaf_msg_t* msg = eaf_msg_create_req(TEST_QUICK_S3_REQ1, 0, NULL);
		ASSERT_NE_PTR(msg, NULL);

		ASSERT_EQ_D32(eaf_send_req(TEST_QUICK_S0, TEST_QUICK_S3, msg), eaf_errno_success);
		eaf_msg_dec_ref(msg);
	}

	/* Step 1. service yield */
	ASSERT_EQ_D32(eaf_sem_pend(s_test_hook_on_service_yield_ctx.sem, 1 * 1000), 0);
	ASSERT_EQ_D32(!!s_test_hook_on_service_yield_ctx.mask.called_before_yield, 1);
	ASSERT_EQ_D32(!!s_test_hook_on_service_yield_ctx.mask.called_after_yield, 0);

	/* Step 2. hook yield */
	ASSERT_EQ_D32(eaf_sem_pend(s_test_hook_on_service_yield_ctx.sem, 1 * 1000), 0);
	ASSERT_EQ_D32(!!s_test_hook_on_service_yield_ctx.mask.called_yield, 1);

	/* Step 3. resume service */
	ASSERT_EQ_D32(eaf_resume(TEST_QUICK_S3), 0);

	/* Step 4. hook resume */
	ASSERT_EQ_D32(eaf_sem_pend(s_test_hook_on_service_yield_ctx.sem, 1 * 1000), 0);
	ASSERT_EQ_D32(!!s_test_hook_on_service_yield_ctx.mask.called_resume, 1);

	/* Step 5. service resume */
	ASSERT_EQ_D32(eaf_sem_pend(s_test_hook_on_service_yield_ctx.sem, 1 * 1000), 0);
	ASSERT_EQ_D32(!!s_test_hook_on_service_yield_ctx.mask.called_after_yield, 1);
}
