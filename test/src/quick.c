#include <stdlib.h>
#include <string.h>
#include "eaf/eaf.h"
#include "quick.h"

typedef struct test_template_ctx
{
	test_quick_cfg_t		cfg;					/** User define configure */
	eaf_entrypoint_t		entry[4];				/**< total 4 services */

	eaf_group_table_t		group[2];				/**< 2 thread */
	eaf_service_table_t		service_table[2][2];	/**< each thread has 2 services */
}test_template_ctx_t;

static test_template_ctx_t* g_test_template_ctx = NULL;
static uint32_t				g_msgid_list[][4] = {
	{ TEST_QUICK_S0_REQ1, TEST_QUICK_S0_REQ2, TEST_QUICK_S0_REQ3, TEST_QUICK_S0_REQ4 },
	{ TEST_QUICK_S1_REQ1, TEST_QUICK_S1_REQ2, TEST_QUICK_S1_REQ3, TEST_QUICK_S1_REQ4 },
	{ TEST_QUICK_S2_REQ1, TEST_QUICK_S2_REQ2, TEST_QUICK_S2_REQ3, TEST_QUICK_S2_REQ4 },
	{ TEST_QUICK_S3_REQ1, TEST_QUICK_S3_REQ2, TEST_QUICK_S3_REQ3, TEST_QUICK_S3_REQ4 },
};

static void _test_template_default_request(_In_ uint32_t from, _In_ uint32_t to, _Inout_ struct eaf_msg* req)
{
	(void)from; (void)to;
	int value = *(int*)eaf_msg_get_data(req, NULL);

	eaf_msg_t* rsp = eaf_msg_create_rsp(req, sizeof(int));
	*(int*)eaf_msg_get_data(rsp, NULL) = ~value;

	eaf_send_rsp(eaf_service_self(), req->from, rsp);
	eaf_msg_dec_ref(rsp);
}

static int _test_template_default_init(void)
{
	return 0;
}

static void _test_template_default_exit(void)
{
}

static void _test_template_custom(const test_quick_cfg_t* cfg)
{
	memcpy(&g_test_template_ctx->cfg, cfg, sizeof(*cfg));

	size_t i, j;
	for (i = 0; i < EAF_ARRAY_SIZE(g_test_template_ctx->cfg.entry); i++)
	{
		for (j = 0; j < EAF_ARRAY_SIZE(g_test_template_ctx->cfg.entry[0].msg_map); j++)
		{
			if (g_test_template_ctx->cfg.entry[i].msg_map[j].msg_id == 0)
			{
				g_test_template_ctx->cfg.entry[i].msg_map[j].msg_id = g_msgid_list[i][j];
			}
			if (g_test_template_ctx->cfg.entry[i].msg_map[j].fn == NULL)
			{
				g_test_template_ctx->cfg.entry[i].msg_map[j].fn = _test_template_default_request;
			}
		}

		if (g_test_template_ctx->cfg.entry[i].on_init == NULL)
		{
			g_test_template_ctx->cfg.entry[i].on_init = _test_template_default_init;
		}
		if (g_test_template_ctx->cfg.entry[i].on_exit == NULL)
		{
			g_test_template_ctx->cfg.entry[i].on_exit = _test_template_default_exit;
		}
	}
}

int test_eaf_quick_setup(const test_quick_cfg_t* cfg)
{
#define SETUP_SERVICE_ENTRY(idx)	\
	g_test_template_ctx->entry[idx].on_init = g_test_template_ctx->cfg.entry[idx].on_init;\
	g_test_template_ctx->entry[idx].on_exit = g_test_template_ctx->cfg.entry[idx].on_exit;\
	g_test_template_ctx->entry[idx].msg_table = g_test_template_ctx->cfg.entry[idx].msg_map;\
	g_test_template_ctx->entry[idx].msg_table_size = EAF_ARRAY_SIZE(g_test_template_ctx->cfg.entry[idx].msg_map);\
	if ((ret = eaf_register(TEST_QUICK_S##idx, &g_test_template_ctx->entry[idx])) < 0) {\
		return ret;\
	}

	int ret;
	if (g_test_template_ctx != NULL)
	{
		return eaf_errno_duplicate;
	}

	if ((g_test_template_ctx = calloc(1, sizeof(test_template_ctx_t))) == NULL)
	{
		return eaf_errno_memory;
	}

	if (cfg != NULL)
	{
		_test_template_custom(cfg);
	}

	// config thread 1 {{{
	g_test_template_ctx->service_table[0][0].srv_id = TEST_QUICK_S0;
	g_test_template_ctx->service_table[0][0].msgq_size = 8;
	g_test_template_ctx->service_table[0][1].srv_id = TEST_QUICK_S1;
	g_test_template_ctx->service_table[0][1].msgq_size = 8;
	g_test_template_ctx->group[0].service.size = 2;
	g_test_template_ctx->group[0].service.table = g_test_template_ctx->service_table[0];
	// }}}

	// config thread 2 {{{
	g_test_template_ctx->service_table[1][0].srv_id = TEST_QUICK_S2;
	g_test_template_ctx->service_table[1][0].msgq_size = 8;
	g_test_template_ctx->service_table[1][1].srv_id = TEST_QUICK_S3;
	g_test_template_ctx->service_table[1][1].msgq_size = 8;
	g_test_template_ctx->group[1].service.size = 2;
	g_test_template_ctx->group[1].service.table = g_test_template_ctx->service_table[1];
	// }}}

	if ((ret = eaf_setup(g_test_template_ctx->group, EAF_ARRAY_SIZE(g_test_template_ctx->group))) < 0)
	{
		return ret;
	}

	SETUP_SERVICE_ENTRY(0);
	SETUP_SERVICE_ENTRY(1);
	SETUP_SERVICE_ENTRY(2);
	SETUP_SERVICE_ENTRY(3);

	if (g_test_template_ctx->cfg.before_load.fn != NULL)
	{
		g_test_template_ctx->cfg.before_load.fn(g_test_template_ctx->cfg.before_load.arg);
	}

	return eaf_load();

#undef SETUP_SERVICE_ENTRY
}

void test_eaf_quick_cleanup(void)
{
	if (g_test_template_ctx == NULL)
	{
		return;
	}

	eaf_cleanup();
	free(g_test_template_ctx);
	g_test_template_ctx = NULL;
}
