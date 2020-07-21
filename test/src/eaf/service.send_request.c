#include <string.h>
#include "ctest/ctest.h"
#include "eaf/eaf.h"
#include "quick2.h"

#define TEST_SERVICE_S1			0x00010000
#define TEST_SERVICE_S1_MSG		(TEST_SERVICE_S1 + 0x0001)

#define TEST_SERVICE_S2			0x00020000
#define TEST_SERVICE_S2_MSG		(TEST_SERVICE_S2 + 0x0001)

typedef struct test_send_request_ctx
{
	int			_s_ret_val;
	eaf_sem_t*	s_service_send_request_sem;
}test_send_request_ctx_t;

static test_send_request_ctx_t s_test_send_request_ctx;

static void _test_send_request_s1_on_rsp(uint32_t from, uint32_t to, eaf_msg_t* msg)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(from, to);

	ASSERT_EQ_D32(eaf_msg_get_receipt(msg), eaf_errno_success);
	s_test_send_request_ctx._s_ret_val = *(int*)eaf_msg_get_data(msg, NULL);
	eaf_sem_post(s_test_send_request_ctx.s_service_send_request_sem);
}

static int _test_send_request_s1_on_init(void)
{
	eaf_msg_t* req = eaf_msg_create_req(TEST_SERVICE_S2_MSG, sizeof(int), _test_send_request_s1_on_rsp);
	ASSERT(req != NULL);
	*(int*)eaf_msg_get_data(req, NULL) = 99;

	ASSERT(eaf_send_req(TEST_SERVICE_S1, TEST_SERVICE_S2, req) == 0);
	eaf_msg_dec_ref(req);

	return 0;
}

static void _test_send_request_s2_on_req(uint32_t from, uint32_t to, eaf_msg_t* req)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(from, to);

	eaf_msg_t* rsp = eaf_msg_create_rsp(req, sizeof(int));
	ASSERT(rsp != NULL);

	*(int*)eaf_msg_get_data(rsp, NULL) = (*(int*)eaf_msg_get_data(req, NULL)) * 2;
	ASSERT(eaf_send_rsp(TEST_SERVICE_S2, req->from, rsp) == 0);
	eaf_msg_dec_ref(rsp);
}

TEST_FIXTURE_SETUP(eaf_service)
{
	memset(&s_test_send_request_ctx, 0, sizeof(s_test_send_request_ctx));
	ASSERT_NE_PTR(s_test_send_request_ctx.s_service_send_request_sem = eaf_sem_create(0), NULL);

	QUICK_DEPLOY_SERVICE(0, TEST_SERVICE_S1, _test_send_request_s1_on_init, NULL, QUICK_DEPLOY_NO_MSG);
	QUICK_DEPLOY_SERVICE(0, TEST_SERVICE_S2, NULL, NULL, {
		{ TEST_SERVICE_S2_MSG, _test_send_request_s2_on_req }
	});
}

TEST_FIXTURE_TEAREDOWN(eaf_service)
{
	eaf_sem_destroy(s_test_send_request_ctx.s_service_send_request_sem);
}

TEST_F(eaf_service, send_request)
{
	/* 等待结果 */
	ASSERT_EQ_D32(eaf_sem_pend(s_test_send_request_ctx.s_service_send_request_sem, 8 * 1000), 0);

	/* 检查结果 */
	ASSERT_EQ_D32(s_test_send_request_ctx._s_ret_val, 99 * 2);
}
