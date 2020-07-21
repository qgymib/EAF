#include <string.h>
#include "ctest/ctest.h"
#include "eaf/eaf.h"
#include "quick2.h"

#define TEST_SERVICE_S1			0x00010000
#define TEST_SERVICE_S1_REQ		(TEST_SERVICE_S1 + 0x0001)

#define TEST_SERVICE_S2			0x00020000
#define TEST_SERVICE_S2_REQ		(TEST_SERVICE_S2 + 0x0001)

typedef struct test_benchmark_yield_ctx
{
	eaf_sem_t*	sem_s1;
	eaf_sem_t*	sem_s2;
	size_t		cnt_s1;
	size_t		cnt_s2;
	size_t		total;
}test_benchmark_yield_ctx_t;

static test_benchmark_yield_ctx_t s_test_benchmark_yield_ctx;

static void _test_benchmark_yield_on_rsp(uint32_t from, uint32_t to, struct eaf_msg* msg)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(from, to, msg);
}

static void _test_benchmark_yield_s1_on_req(uint32_t from, uint32_t to, struct eaf_msg* msg)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(from, to, msg);
	eaf_reenter
	{
		/* first we need s2 continue run */
		eaf_resume(TEST_SERVICE_S2);

		for (; s_test_benchmark_yield_ctx.cnt_s1 < s_test_benchmark_yield_ctx.total;
			s_test_benchmark_yield_ctx.cnt_s1++)
		{
			eaf_yield eaf_resume(TEST_SERVICE_S2);
		}

		eaf_sem_post(s_test_benchmark_yield_ctx.sem_s1);
	};
}

static void _test_benchmark_yield_s2_on_req(uint32_t from, uint32_t to, struct eaf_msg* msg)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(from, to, msg);
	eaf_reenter
	{
		/* first we need s1 continue run */
		eaf_resume(TEST_SERVICE_S1);

		for (; s_test_benchmark_yield_ctx.cnt_s2 < s_test_benchmark_yield_ctx.total;
			s_test_benchmark_yield_ctx.cnt_s2++)
		{
			eaf_yield eaf_resume(TEST_SERVICE_S1);
		}

		eaf_sem_post(s_test_benchmark_yield_ctx.sem_s2);
	};
}

TEST_FIXTURE_SETUP(benchmark_yield)
{
	memset(&s_test_benchmark_yield_ctx, 0, sizeof(s_test_benchmark_yield_ctx));
	ASSERT_NE_PTR(s_test_benchmark_yield_ctx.sem_s1 = eaf_sem_create(0), NULL);
	ASSERT_NE_PTR(s_test_benchmark_yield_ctx.sem_s2 = eaf_sem_create(0), NULL);
	s_test_benchmark_yield_ctx.total = 1000000;

	QUICK_DEPLOY_SERVICE(0, TEST_SERVICE_S1, NULL, NULL, {
		{ TEST_SERVICE_S1_REQ, _test_benchmark_yield_s1_on_req }
	});
	QUICK_DEPLOY_SERVICE(0, TEST_SERVICE_S2, NULL, NULL, {
		{ TEST_SERVICE_S2_REQ, _test_benchmark_yield_s2_on_req }
	});
}

TEST_FIXTURE_TEAREDOWN(benchmark_yield)
{
	eaf_sem_destroy(s_test_benchmark_yield_ctx.sem_s1);
	eaf_sem_destroy(s_test_benchmark_yield_ctx.sem_s2);
}

TEST_F(benchmark_yield, DISABLED_yield_1000000)
{
	/* send to s1 */
	{
		eaf_msg_t* msg = eaf_msg_create_req(TEST_SERVICE_S1_REQ, sizeof(int), _test_benchmark_yield_on_rsp);
		ASSERT_NE_PTR(msg, NULL);
		ASSERT_EQ_D32(eaf_send_req((uint32_t)-1, TEST_SERVICE_S1, msg), 0);
		eaf_msg_dec_ref(msg);
	}
	/* send to s2 */
	{
		eaf_msg_t* msg = eaf_msg_create_req(TEST_SERVICE_S2_REQ, sizeof(int), _test_benchmark_yield_on_rsp);
		ASSERT_NE_PTR(msg, NULL);
		ASSERT_EQ_D32(eaf_send_req((uint32_t)-1, TEST_SERVICE_S2, msg), 0);
		eaf_msg_dec_ref(msg);
	}
	/* wait for test complete */
	{
		eaf_sem_pend(s_test_benchmark_yield_ctx.sem_s1, EAF_SEM_INFINITY);
		eaf_sem_pend(s_test_benchmark_yield_ctx.sem_s2, EAF_SEM_INFINITY);
	}
}
