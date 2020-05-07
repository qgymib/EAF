#include <string.h>
#include "eaf/eaf.h"
#include "etest/etest.h"
#include "quick.h"

#define TEST_QUICK_S4		0xE0040000
#define TEST_QUICK_S4_REQ1	(TEST_QUICK_S4 + 0x0001)
#define TEST_QUICK_S4_REQ2	(TEST_QUICK_S4 + 0x0002)
#define TEST_QUICK_S4_REQ3	(TEST_QUICK_S4 + 0x0003)
#define TEST_QUICK_S4_REQ4	(TEST_QUICK_S4 + 0x0004)

typedef struct test_eaf_hook
{
	struct
	{
		unsigned		on_msg_send : 1;
		unsigned		on_dst_not_found : 1;
		unsigned		on_pre_msg_process : 1;
		unsigned		on_post_msg_process : 1;
		unsigned		on_post_yield : 1;
		unsigned		on_post_resume : 1;
	}mask;

	struct
	{
		uint32_t		last_from;
		uint32_t		last_to;
		eaf_msg_t*		last_msg;
		void*			last_hook;
		int				last_val;
	}info[2];

	size_t				counter;
	eaf_sem_t*			sem;
}test_eaf_hook_t;

static test_eaf_hook_t	s_eaf_hook;

static int _test_eaf_hook_on_msg_send(uint32_t from, uint32_t to, struct eaf_msg* msg)
{
	if (!s_eaf_hook.mask.on_msg_send)
	{
		return eaf_errno_success;
	}

	s_eaf_hook.info[0].last_hook = (void*)(uintptr_t)_test_eaf_hook_on_msg_send;
	s_eaf_hook.info[0].last_from = from;
	s_eaf_hook.info[0].last_to = to;
	s_eaf_hook.info[0].last_msg = msg;
	s_eaf_hook.counter++;

	return eaf_errno_transfer;
}

static int _test_eaf_hook_on_dst_not_found(uint32_t from, uint32_t to, struct eaf_msg* msg)
{
	if (!s_eaf_hook.mask.on_dst_not_found)
	{
		return eaf_errno_notfound;
	}

	s_eaf_hook.info[0].last_hook = (void*)(uintptr_t)_test_eaf_hook_on_dst_not_found;
	s_eaf_hook.info[0].last_from = from;
	s_eaf_hook.info[0].last_to = to;
	s_eaf_hook.info[0].last_msg = msg;
	s_eaf_hook.counter++;

	return eaf_errno_success;
}

static int _test_eaf_hook_on_pre_msg_process(_In_ uint32_t from, _In_ uint32_t to, _Inout_ struct eaf_msg* msg)
{
	if (!s_eaf_hook.mask.on_pre_msg_process)
	{
		return eaf_errno_success;
	}

	/* ignore request */
	if (eaf_msg_get_type(msg) != eaf_msg_type_req)
	{
		return eaf_errno_success;
	}

	s_eaf_hook.info[0].last_hook = (void*)(uintptr_t)_test_eaf_hook_on_pre_msg_process;
	s_eaf_hook.info[0].last_from = from;
	s_eaf_hook.info[0].last_to = to;
	s_eaf_hook.info[0].last_msg = msg;
	s_eaf_hook.counter++;

	eaf_msg_t* rsp = eaf_msg_create_rsp(msg, sizeof(int));
	ASSERT(rsp != NULL);

	*(int*)eaf_msg_get_data(rsp, NULL) = (*(int*)eaf_msg_get_data(msg, NULL)) + 1;
	ASSERT(eaf_send_rsp(to, from, rsp) == 0);
	eaf_msg_dec_ref(rsp);

	return eaf_errno_notfound;
}

static void _test_eaf_hook_on_post_msg_process(_In_ uint32_t from, _In_ uint32_t to, _Inout_ struct eaf_msg* msg)
{
	if (!s_eaf_hook.mask.on_post_msg_process)
	{
		return;
	}

	int idx = (eaf_msg_get_type(msg) == eaf_msg_type_req) ? 0 : 1;

	s_eaf_hook.info[idx].last_from = from;
	s_eaf_hook.info[idx].last_to = to;
	s_eaf_hook.info[idx].last_val = *(int*)eaf_msg_get_data(msg, NULL);
	s_eaf_hook.counter++;

	eaf_sem_post(s_eaf_hook.sem);
}

static void _test_eaf_hook_on_post_yield(_In_ uint32_t srv_id)
{
	if (!s_eaf_hook.mask.on_post_yield)
	{
		return;
	}

	s_eaf_hook.info[0].last_from = srv_id;
	s_eaf_hook.counter++;

	eaf_sem_post(s_eaf_hook.sem);
}

static void _test_eaf_hook_on_post_resume(_In_ uint32_t srv_id)
{
	if (!s_eaf_hook.mask.on_post_resume)
	{
		return;
	}

	s_eaf_hook.info[1].last_from = srv_id;
	s_eaf_hook.counter++;

	eaf_sem_post(s_eaf_hook.sem);
}

static void _on_pre_msg_process_on_rsp(uint32_t from, uint32_t to, struct eaf_msg* msg)
{
	s_eaf_hook.info[1].last_hook = (void*)(uintptr_t)_on_pre_msg_process_on_rsp;
	s_eaf_hook.info[1].last_from = from;
	s_eaf_hook.info[1].last_to = to;
	s_eaf_hook.info[1].last_msg = msg;
	s_eaf_hook.info[1].last_val = *(int*)eaf_msg_get_data(msg, NULL);

	s_eaf_hook.counter++;
	eaf_sem_post(s_eaf_hook.sem);
}

static void _test_eaf_hook_before_load(void* arg)
{
	(void)arg;

	static eaf_hook_t hook;
	memset(&hook, 0, sizeof(hook));
	hook.on_msg_send = _test_eaf_hook_on_msg_send;
	hook.on_dst_not_found = _test_eaf_hook_on_dst_not_found;
	hook.on_pre_msg_process = _test_eaf_hook_on_pre_msg_process;
	hook.on_post_msg_process = _test_eaf_hook_on_post_msg_process;
	hook.on_post_yield = _test_eaf_hook_on_post_yield;
	hook.on_post_resume = _test_eaf_hook_on_post_resume;

	ASSERT_NUM_EQ(eaf_inject(&hook, sizeof(hook)), 0);
}

static void _test_eaf_hook_on_req(uint32_t from, uint32_t to, struct eaf_msg* msg)
{
	(void)from; (void)to; (void)msg;

	eaf_reenter
	{
		if (!s_eaf_hook.mask.on_post_yield)
		{
			return;
		}
		eaf_yield;
	};
}

TEST_CLASS_SETUP(eaf_hook)
{
	memset(&s_eaf_hook, 0, sizeof(s_eaf_hook));
	s_eaf_hook.sem = eaf_sem_create(0);

	static test_quick_cfg_t quick_cfg;
	memset(&quick_cfg, 0, sizeof(quick_cfg));

	quick_cfg.before_load.fn = _test_eaf_hook_before_load;
	quick_cfg.entry[3].msg_map[0].fn = _test_eaf_hook_on_req;
	ASSERT_NUM_EQ(test_eaf_quick_setup(&quick_cfg), 0);
}

TEST_CLASS_TEAREDOWN(eaf_hook)
{
	test_eaf_quick_cleanup();
	eaf_sem_destroy(s_eaf_hook.sem);
}

TEST_F(eaf_hook, on_msg_send)
{
	s_eaf_hook.mask.on_msg_send = 1;

	eaf_msg_t* msg = eaf_msg_create_req(TEST_QUICK_S1_REQ1, 0, NULL);
	ASSERT_PTR_NE(msg, NULL);

	ASSERT_NUM_EQ(eaf_send_req(TEST_QUICK_S0, TEST_QUICK_S1, msg), eaf_errno_transfer);

	ASSERT_NUM_EQ(s_eaf_hook.counter, 1);
	ASSERT_NUM_EQ(s_eaf_hook.info[0].last_from, TEST_QUICK_S0);
	ASSERT_NUM_EQ(s_eaf_hook.info[0].last_to, TEST_QUICK_S1);
	ASSERT_PTR_EQ(s_eaf_hook.info[0].last_msg, msg);
	ASSERT_PTR_EQ(s_eaf_hook.info[0].last_hook, (void*)(uintptr_t)_test_eaf_hook_on_msg_send);

	eaf_msg_dec_ref(msg);
}

TEST_F(eaf_hook, on_dst_not_found)
{
	s_eaf_hook.mask.on_dst_not_found = 1;

	eaf_msg_t* msg = eaf_msg_create_req(TEST_QUICK_S4_REQ1, 0, NULL);
	ASSERT_PTR_NE(msg, NULL);

	ASSERT_NUM_EQ(eaf_send_req(TEST_QUICK_S0, TEST_QUICK_S4, msg), eaf_errno_success);

	ASSERT_NUM_EQ(s_eaf_hook.counter, 1);
	ASSERT_NUM_EQ(s_eaf_hook.info[0].last_from, TEST_QUICK_S0);
	ASSERT_NUM_EQ(s_eaf_hook.info[0].last_to, TEST_QUICK_S4);
	ASSERT_PTR_EQ(s_eaf_hook.info[0].last_msg, msg);
	ASSERT_PTR_EQ(s_eaf_hook.info[0].last_hook, (void*)(uintptr_t)_test_eaf_hook_on_dst_not_found);

	eaf_msg_dec_ref(msg);
}

TEST_F(eaf_hook, on_pre_msg_process)
{
	s_eaf_hook.mask.on_pre_msg_process = 1;

	const int val = 100;

	eaf_msg_t* msg = eaf_msg_create_req(TEST_QUICK_S1_REQ1, sizeof(int), _on_pre_msg_process_on_rsp);
	ASSERT_PTR_NE(msg, NULL);

	*(int*)eaf_msg_get_data(msg, NULL) = val;
	ASSERT_NUM_EQ(eaf_send_req(TEST_QUICK_S0, TEST_QUICK_S1, msg), eaf_errno_success);

	/* wait for response */
	ASSERT_NUM_EQ(eaf_sem_pend(s_eaf_hook.sem, 8 * 1000), 0);
	ASSERT_NUM_EQ(s_eaf_hook.counter, 2);
	ASSERT_NUM_EQ(s_eaf_hook.info[1].last_val, val + 1);
	ASSERT_NUM_EQ(s_eaf_hook.info[1].last_from, TEST_QUICK_S1);
	ASSERT_NUM_EQ(s_eaf_hook.info[1].last_to, TEST_QUICK_S0);
	ASSERT_PTR_EQ(s_eaf_hook.info[1].last_hook, (void*)(uintptr_t)_on_pre_msg_process_on_rsp);

	eaf_msg_dec_ref(msg);
}

TEST_F(eaf_hook, on_post_msg_process)
{
	s_eaf_hook.mask.on_post_msg_process = 1;

	const int val = 200;
	eaf_msg_t* msg = eaf_msg_create_req(TEST_QUICK_S1_REQ1, sizeof(int), NULL);
	ASSERT_PTR_NE(msg, NULL);

	*(int*)eaf_msg_get_data(msg, NULL) = val;
	ASSERT_NUM_EQ(eaf_send_req(TEST_QUICK_S0, TEST_QUICK_S1, msg), eaf_errno_success);

	/* verify request */
	{
		ASSERT_NUM_EQ(eaf_sem_pend(s_eaf_hook.sem, 8 * 1000), 0);
		ASSERT_NUM_EQ(s_eaf_hook.info[0].last_from, TEST_QUICK_S0);
		ASSERT_NUM_EQ(s_eaf_hook.info[0].last_to, TEST_QUICK_S1);
		ASSERT_NUM_EQ(s_eaf_hook.info[0].last_val, val);
	}

	/* verify response */
	{
		ASSERT_NUM_EQ(eaf_sem_pend(s_eaf_hook.sem, 8 * 1000), 0);
		ASSERT_NUM_EQ(s_eaf_hook.info[1].last_from, TEST_QUICK_S1);
		ASSERT_NUM_EQ(s_eaf_hook.info[1].last_to, TEST_QUICK_S0);
		ASSERT_NUM_EQ(s_eaf_hook.info[1].last_val, ~val);
	}

	eaf_msg_dec_ref(msg);
}

TEST_F(eaf_hook, on_post_yield)
{
	s_eaf_hook.mask.on_post_yield = 1;
	s_eaf_hook.mask.on_post_resume = 1;

	eaf_msg_t* msg = eaf_msg_create_req(TEST_QUICK_S3_REQ1, 0, NULL);
	ASSERT_PTR_NE(msg, NULL);

	ASSERT_NUM_EQ(eaf_send_req(TEST_QUICK_S0, TEST_QUICK_S3, msg), eaf_errno_success);

	ASSERT_NUM_EQ(eaf_sem_pend(s_eaf_hook.sem, 8 * 1000), 0);
	ASSERT_NUM_EQ(s_eaf_hook.info[0].last_from, TEST_QUICK_S3);
	ASSERT_NUM_EQ(s_eaf_hook.counter, 1);

	ASSERT_NUM_EQ(eaf_resume(TEST_QUICK_S3), 0);
	ASSERT_NUM_EQ(eaf_sem_pend(s_eaf_hook.sem, 8 * 1000), 0);
	ASSERT_NUM_EQ(s_eaf_hook.info[1].last_from, TEST_QUICK_S3);

	eaf_msg_dec_ref(msg);
}
