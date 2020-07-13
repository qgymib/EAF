#include <string.h>
#include "eaf/powerpack.h"
#include "ctest/ctest.h"
#include "quick.h"

typedef struct test_watchdog_ctx
{
	struct
	{
		eaf_sem_t*	sem;
		int			has;
	}on_error;

	struct
	{
		eaf_sem_t*	sem;
		int			has;
	}on_register_rsp;

	struct
	{
		eaf_sem_t*	sem;
		int			has;
	}on_unregister_rsp;
}test_watchdog_ctx_t;

static test_watchdog_ctx_t s_test_watchdog_ctx;

static void _test_watchdog_on_error_fn(uint32_t id, void* arg)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(id, arg);

	s_test_watchdog_ctx.on_error.has = 1;
	eaf_sem_post(s_test_watchdog_ctx.on_error.sem);
}

static void _test_watchdog_on_register_rsp(uint32_t from, uint32_t to, struct eaf_msg* msg)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(from, to, msg);

	s_test_watchdog_ctx.on_register_rsp.has = 1;
	eaf_sem_post(s_test_watchdog_ctx.on_register_rsp.sem);
}

static void _test_watchdog_on_unregister_rsp(uint32_t from, uint32_t to, struct eaf_msg* msg)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(from, to, msg);

	s_test_watchdog_ctx.on_unregister_rsp.has = 1;
	eaf_sem_post(s_test_watchdog_ctx.on_unregister_rsp.sem);
}

TEST_FIXTURE_SETUP(powerpack_watchdog)
{
	memset(&s_test_watchdog_ctx, 0, sizeof(s_test_watchdog_ctx));
	ASSERT_NE_PTR(s_test_watchdog_ctx.on_error.sem = eaf_sem_create(0), NULL);
	ASSERT_NE_PTR(s_test_watchdog_ctx.on_register_rsp.sem = eaf_sem_create(0), NULL);
	ASSERT_NE_PTR(s_test_watchdog_ctx.on_unregister_rsp.sem = eaf_sem_create(0), NULL);

	static eaf_service_table_t service_table[] = {
		{ EAF_WATCHDOG_ID, 8 },
		{ EAF_TIMER_ID, 8 },
		{ TEST_QUICK_S1, 8 },
	};
	static eaf_group_table_t group_table[] = {
		{ EAF_THREAD_ATTR_INITIALIZER, { EAF_ARRAY_SIZE(service_table), service_table } },
	};
	ASSERT_EQ_D32(eaf_setup(group_table, EAF_ARRAY_SIZE(group_table)), 0);

	eaf_powerpack_cfg_t pp_cfg = { EAF_THREAD_ATTR_INITIALIZER };
	ASSERT_EQ_D32(eaf_powerpack_init(&pp_cfg), 0);

	ASSERT_EQ_D32(eaf_timer_init(), 0, "error:%s(%d)", eaf_strerror(_a), _a);
	ASSERT_EQ_D32(eaf_watchdog_init(_test_watchdog_on_error_fn, NULL), 0);

	ASSERT_EQ_D32(eaf_load(), 0, "error:%s(%d)", eaf_strerror(_a), _a);
}

TEST_FIXTURE_TEAREDOWN(powerpack_watchdog)
{
	eaf_cleanup();
	eaf_watchdog_exit();
	eaf_timer_exit();
	eaf_powerpack_exit();

	eaf_sem_destroy(s_test_watchdog_ctx.on_error.sem);
	eaf_sem_destroy(s_test_watchdog_ctx.on_register_rsp.sem);
	eaf_sem_destroy(s_test_watchdog_ctx.on_unregister_rsp.sem);
}

TEST_F(powerpack_watchdog, triger)
{
	int ret;
	EAF_SEND_REQUEST(ret, EAF_WATCHDOG_MSG_REGISTER_REQ, sizeof(eaf_watchdog_register_req_t),
		_test_watchdog_on_register_rsp, TEST_QUICK_S1, EAF_WATCHDOG_ID, {
			eaf_watchdog_register_req_t * req = eaf_msg_get_data(_0, NULL);
			req->id = 1;
			req->timeout = 100;
		}
	);
	ASSERT_EQ_D32(ret, 0, "error:%s(%d)", eaf_strerror(ret), ret);

	ASSERT_EQ_D32(eaf_sem_pend(s_test_watchdog_ctx.on_register_rsp.sem, 1000), 0);
	ASSERT_EQ_D32(s_test_watchdog_ctx.on_register_rsp.has, 1);

	ASSERT_EQ_D32(eaf_sem_pend(s_test_watchdog_ctx.on_error.sem, 1000), 0);
	ASSERT_EQ_D32(s_test_watchdog_ctx.on_error.has, 1);
}

TEST_F(powerpack_watchdog, register_and_unregister)
{
	int ret;
	EAF_SEND_REQUEST(ret, EAF_WATCHDOG_MSG_REGISTER_REQ, sizeof(eaf_watchdog_register_req_t),
		_test_watchdog_on_register_rsp, TEST_QUICK_S1, EAF_WATCHDOG_ID, {
			eaf_watchdog_register_req_t * req = eaf_msg_get_data(_0, NULL);
			req->id = 1;
			req->timeout = 100;
		}
	);
	ASSERT_EQ_D32(ret, 0);

	EAF_SEND_REQUEST(ret, EAF_WATCHDOG_MSG_UNREGISTER_REQ, sizeof(eaf_watchdog_unregister_req_t),
		_test_watchdog_on_unregister_rsp, TEST_QUICK_S1, EAF_WATCHDOG_ID, {
			((eaf_watchdog_unregister_req_t*)eaf_msg_get_data(_0, NULL))->id = 1;
		}
	);
	ASSERT_EQ_D32(ret, 0);

	/* wait for register response */
	ASSERT_EQ_D32(eaf_sem_pend(s_test_watchdog_ctx.on_register_rsp.sem, 1000), 0);
	ASSERT_EQ_D32(s_test_watchdog_ctx.on_register_rsp.has, 1);

	/* wait for unregister response */
	ASSERT_EQ_D32(eaf_sem_pend(s_test_watchdog_ctx.on_unregister_rsp.sem, 1000), 0);
	ASSERT_EQ_D32(s_test_watchdog_ctx.on_unregister_rsp.has, 1);

	/* ensure error callback is not called */
	ASSERT_EQ_D32(eaf_sem_pend(s_test_watchdog_ctx.on_error.sem, 500), eaf_errno_timeout);
	ASSERT_EQ_D32(s_test_watchdog_ctx.on_error.has, 0);
}
