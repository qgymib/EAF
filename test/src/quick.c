#include <stdlib.h>
#include <string.h>
#include "eaf/eaf.h"
#include "quick.h"

typedef struct test_quick_entry
{
	struct
	{
		unsigned			enable : 1;
	}ability;
	uint32_t				id;
	eaf_message_table_t		msg_map[4];
	eaf_entrypoint_t		entry;
}test_quick_entry_t;

typedef struct test_template_ctx
{
	int						running;
	test_quick_entry_t		entry[4];				/**< total 4 services */

	eaf_group_table_t		group[2];				/**< 2 thread */
	eaf_service_table_t		service_table[2][2];	/**< each thread has 2 services */
}test_template_ctx_t;

static test_template_ctx_t	g_test_quick_ctx = {
	0,	/* running */
	{	/* entry */
		{
			{ 0 }, TEST_QUICK_S0,
			{ { TEST_QUICK_S0_REQ1, NULL }, { TEST_QUICK_S0_REQ2, NULL }, { TEST_QUICK_S0_REQ3, NULL }, { TEST_QUICK_S0_REQ4, NULL } },
			{ EAF_ARRAY_SIZE(g_test_quick_ctx.entry[0].msg_map), g_test_quick_ctx.entry[0].msg_map, NULL, NULL }
		},
		{
			{ 0 }, TEST_QUICK_S1,
			{ { TEST_QUICK_S1_REQ1, NULL }, { TEST_QUICK_S1_REQ2, NULL }, { TEST_QUICK_S1_REQ3, NULL }, { TEST_QUICK_S1_REQ4, NULL } },
			{ EAF_ARRAY_SIZE(g_test_quick_ctx.entry[1].msg_map), g_test_quick_ctx.entry[1].msg_map, NULL, NULL }
		},
		{
			{ 0 }, TEST_QUICK_S2,
			{ { TEST_QUICK_S2_REQ1, NULL }, { TEST_QUICK_S2_REQ2, NULL }, { TEST_QUICK_S2_REQ3, NULL }, { TEST_QUICK_S2_REQ4, NULL } },
			{ EAF_ARRAY_SIZE(g_test_quick_ctx.entry[2].msg_map), g_test_quick_ctx.entry[2].msg_map, NULL, NULL }
		},
		{
			{ 0 }, TEST_QUICK_S3,
			{ { TEST_QUICK_S3_REQ1, NULL }, { TEST_QUICK_S3_REQ2, NULL }, { TEST_QUICK_S3_REQ3, NULL }, { TEST_QUICK_S3_REQ4, NULL } },
			{ EAF_ARRAY_SIZE(g_test_quick_ctx.entry[3].msg_map), g_test_quick_ctx.entry[3].msg_map, NULL, NULL }
		},
	},	/* entry */
	{	/* group */
		{ EAF_THREAD_ATTR_INITIALIZER, { 2, g_test_quick_ctx.service_table[0] } },
		{ EAF_THREAD_ATTR_INITIALIZER, { 2, g_test_quick_ctx.service_table[1] } },
	},	/* group */
	{	/* service_table */
		{ { TEST_QUICK_S0, 8 }, { TEST_QUICK_S1, 8 } },
		{ { TEST_QUICK_S2, 8 }, { TEST_QUICK_S3, 8 } },
	},	/* service_table */
};
test_quick_cfg_t g_quick_cfg;

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
	// do nothing
}

static void _test_template_custom(const test_quick_cfg_t* cfg)
{
	size_t i;
	for (i = 0; i < EAF_ARRAY_SIZE(cfg->entry); i++)
	{
		if (!cfg->entry[i].ability.enable)
		{
			continue;
		}

		g_test_quick_ctx.entry[i].ability.enable = 1;
		if (cfg->entry[i].on_init != NULL)
		{
			g_test_quick_ctx.entry[i].entry.on_init = cfg->entry[i].on_init;
		}
		if (cfg->entry[i].on_exit != NULL)
		{
			g_test_quick_ctx.entry[i].entry.on_exit = cfg->entry[i].on_exit;
		}

		size_t j;
		for (j = 0; j < EAF_ARRAY_SIZE(cfg->entry[i].msg_map); j++)
		{
			if (cfg->entry[i].msg_map[j] != NULL)
			{
				g_test_quick_ctx.entry[i].msg_map[j].fn = cfg->entry[i].msg_map[j];
			}
		}
	}
}

static void _test_quick_reset_default_config(void)
{
	size_t i;
	for (i = 0; i < EAF_ARRAY_SIZE(g_test_quick_ctx.entry); i++)
	{
		size_t j;
		for (j = 0; j < EAF_ARRAY_SIZE(g_test_quick_ctx.entry[j].msg_map); j++)
		{
			g_test_quick_ctx.entry[i].msg_map[j].fn = _test_template_default_request;
		}

		g_test_quick_ctx.entry[i].ability.enable = 0;
		g_test_quick_ctx.entry[i].entry.on_init = _test_template_default_init;
		g_test_quick_ctx.entry[i].entry.on_exit = _test_template_default_exit;
	}
}

int test_eaf_quick_setup(const test_quick_cfg_t* cfg)
{
	int ret;
	if (g_test_quick_ctx.running)
	{
		return eaf_errno_duplicate;
	}

	_test_quick_reset_default_config();
	if (cfg != NULL)
	{
		_test_template_custom(cfg);
	}

	if ((ret = eaf_init(g_test_quick_ctx.group, EAF_ARRAY_SIZE(g_test_quick_ctx.group))) < 0)
	{
		return ret;
	}

	if (cfg != NULL && cfg->before_load.fn != NULL)
	{
		cfg->before_load.fn(cfg->before_load.arg);
	}

	size_t i;
	for (i = 0; i < EAF_ARRAY_SIZE(g_test_quick_ctx.entry); i++)
	{
		if (!g_test_quick_ctx.entry[i].ability.enable)
		{
			continue;
		}
		if (eaf_register(g_test_quick_ctx.entry[i].id, &g_test_quick_ctx.entry[i].entry) < 0)
		{
			return -1;
		}
	}

	if ((ret = eaf_load()) < 0)
	{
		return ret;
	}

	g_test_quick_ctx.running = 1;
	return 0;
}

void test_eaf_quick_cleanup(void)
{
	if (!g_test_quick_ctx.running)
	{
		return;
	}

	eaf_exit();
	g_test_quick_ctx.running = 0;
}
