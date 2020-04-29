#include <string.h>
#include "eaf/eaf.h"
#include "etest/etest.h"
#include "quick.h"

#define TEST_SERIVCE_S1			0x00010000
#define TEST_SERIVCE_S1_REQ		(TEST_SERIVCE_S1 + 0x0001)

typedef struct test_rpc_ctx
{
	test_quick_cfg_t		template_cfg;
	eaf_sem_t*				sem_ret;

	struct
	{
		eaf_msg_t*			orig_req;
		uint32_t			to;
		int					value;
	}last_req;

	struct
	{
		int					receipt;
		int					value;
	}last_rsp;
}test_rpc_ctx_t;

static test_rpc_ctx_t	s_test_rpc_ctx;

static void _test_rpc_on_service_register(_In_ const eaf_rpc_service_info_t* info)
{
	(void)info;
}

static int _test_rpc_on_init_done(void)
{
	return 0;
}

static void _test_rpc_on_exit(void)
{

}

static int _test_rpc_output_req(_In_ uint32_t from, _In_ uint32_t to, _Inout_ eaf_msg_t* req)
{
	(void)from;

	/**
	 * Record necessary message
	 */
	s_test_rpc_ctx.last_req.orig_req = req;
	eaf_msg_add_ref(req);

	s_test_rpc_ctx.last_req.to = to;
	s_test_rpc_ctx.last_req.value = *(int*)eaf_msg_get_data(req, NULL);

	eaf_sem_post(s_test_rpc_ctx.sem_ret);
	return 0;
}

static void _test_rpc_on_test1_rsp(_In_ int receipt, _Inout_ struct eaf_msg* rsp)
{
	/**
	 * Record necessary message
	 */
	s_test_rpc_ctx.last_rsp.receipt = receipt;
	s_test_rpc_ctx.last_rsp.value = *(int*)eaf_msg_get_data(rsp, NULL);

	eaf_sem_post(s_test_rpc_ctx.sem_ret);
}

static int _test_rpc_output_rsp(_In_ int receipt, _In_ uint32_t from, _In_ uint32_t to, _Inout_ eaf_msg_t* rsp)
{
	(void)from;
	(void)to;
    _test_rpc_on_test1_rsp(receipt, rsp);
	return 0;
}

static eaf_rpc_cfg_t s_eaf_rpc_cfg = {
	_test_rpc_on_service_register,
	_test_rpc_on_init_done,
	_test_rpc_on_exit,
	_test_rpc_output_req,
	_test_rpc_output_rsp,
};

static void _test_rpc_before_load(void* arg)
{
	(void)arg;
	ASSERT_NUM_EQ(eaf_rpc_init(&s_eaf_rpc_cfg), 0);
}

TEST_CLASS_SETUP(eaf_rpc)
{
	memset(&s_test_rpc_ctx, 0, sizeof(s_test_rpc_ctx));
	ASSERT_PTR_NE(s_test_rpc_ctx.sem_ret = eaf_sem_create(0), NULL);

	s_test_rpc_ctx.template_cfg.before_load.fn = _test_rpc_before_load;
	ASSERT_NUM_EQ(test_eaf_quick_setup(&s_test_rpc_ctx.template_cfg), 0);
}

TEST_CLASS_TEAREDOWN(eaf_rpc)
{
	test_eaf_quick_cleanup();

	eaf_sem_destroy(s_test_rpc_ctx.sem_ret);

	if (s_test_rpc_ctx.last_req.orig_req != NULL)
	{
		eaf_msg_dec_ref(s_test_rpc_ctx.last_req.orig_req);
	}
}

TEST_F(eaf_rpc, output_req)
{
	const int req_val = 99;

	/* send request to RPC */
	{
		eaf_msg_t* req = eaf_msg_create_req(TEST_SERIVCE_S1_REQ, sizeof(int), _test_rpc_on_test1_rsp);
		ASSERT_PTR_NE(req, NULL);
		*(int*)eaf_msg_get_data(req, NULL) = req_val;

		ASSERT_NUM_EQ(eaf_send_req(TEST_QUICK_S1, TEST_SERIVCE_S1, req), 0);
		eaf_msg_dec_ref(req);
	}
	/* verify */
	{
		ASSERT_NUM_EQ(eaf_sem_pend(s_test_rpc_ctx.sem_ret, 8 * 1000), 0);
		ASSERT_NUM_EQ(s_test_rpc_ctx.last_req.value, req_val);
		ASSERT_NUM_EQ(s_test_rpc_ctx.last_req.orig_req->from, TEST_QUICK_S1);
		ASSERT_NUM_EQ(s_test_rpc_ctx.last_req.to, TEST_SERIVCE_S1);
	}

	/* send response */
	const int rsp_val = req_val * 3;
	{
		eaf_msg_t* rsp = eaf_msg_create_rsp(s_test_rpc_ctx.last_req.orig_req, sizeof(int));
		ASSERT_PTR_NE(rsp, NULL);
		*(int*)eaf_msg_get_data(rsp, NULL) = rsp_val;

		ASSERT_NUM_EQ(eaf_rpc_input_rsp(eaf_errno_success, TEST_SERIVCE_S1, TEST_QUICK_S1, rsp), 0);
		eaf_msg_dec_ref(rsp);
	}
	/* verify */
	{
		ASSERT_NUM_EQ(eaf_sem_pend(s_test_rpc_ctx.sem_ret, 8 * 1000), 0);
		ASSERT_NUM_EQ(s_test_rpc_ctx.last_rsp.value, rsp_val);
		ASSERT_NUM_EQ(s_test_rpc_ctx.last_rsp.receipt, eaf_errno_success);
	}
}

TEST_F(eaf_rpc, output_rsp)
{
	/* send request to EAF */
	const int req_val = 88;
	{
		eaf_msg_t* req = eaf_msg_create_req(TEST_QUICK_S0_REQ1, sizeof(int), NULL);
		ASSERT_PTR_NE(req, NULL);
		*(int*)eaf_msg_get_data(req, NULL) = req_val;

		ASSERT_NUM_EQ(eaf_rpc_input_req(TEST_SERIVCE_S1, TEST_QUICK_S0, req), 0);
		eaf_msg_dec_ref(req);
	}
	/* verify */
	{
		ASSERT_NUM_EQ(eaf_sem_pend(s_test_rpc_ctx.sem_ret, 8 * 1000), 0);
		ASSERT_NUM_EQ(s_test_rpc_ctx.last_rsp.value, (int)(~req_val));
	}
}
