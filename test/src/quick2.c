#include "quick2.h"

#define TEST_QUICK_GROUP_NUM	4
#define TEST_QUICK_SERVICE_NUM	8

static void _quick_hook_before_fixture_setup(const char* fixture_name);
static void _quick_hook_after_fixture_setup(const char* fixture_name, int ret);
static void _quick_hook_before_fixture_teardown(const char* fixture_name);

typedef struct test_quick2_ctx
{
	struct
	{
		unsigned			inited : 1;
		unsigned			loaded : 1;
	}mask;
}test_quick2_ctx_t;

typedef struct test_quick2_ctx2
{
	eaf_service_table_t		service_table[TEST_QUICK_GROUP_NUM][TEST_QUICK_SERVICE_NUM];
	eaf_group_table_t		group_table[TEST_QUICK_GROUP_NUM];
	eaf_entrypoint_t*		entry_table[TEST_QUICK_GROUP_NUM * TEST_QUICK_SERVICE_NUM];
}test_quick2_ctx2_t;

static test_quick2_ctx2_t	g_test_quick2_ctx2;
static test_quick2_ctx_t	g_test_quick2_ctx = {
	{ 0, 0 },								// .mask
};
const ctest_hook_t			quick_hook = {
	NULL,									// .before_all_test
	NULL,									// .after_all_test
	_quick_hook_before_fixture_setup,		// .before_fixture_setup
	_quick_hook_after_fixture_setup,		// .after_fixture_setup
	_quick_hook_before_fixture_teardown,	// .before_fixture_teardown
	NULL,									// .after_fixture_teardown
	NULL,									// .before_fixture_test
	NULL,									// .after_fixture_test
	NULL,									// .before_parameterized_test
	NULL,									// .after_parameterized_test
	NULL,									// .before_simple_test
	NULL,									// .after_simple_test
};

static void _quick_hook_before_fixture_setup(const char* fixture_name)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(fixture_name);

	size_t i;
	for (i = 0; i < TEST_QUICK_GROUP_NUM; i++)
	{
		size_t j;
		for (j = 0; j < TEST_QUICK_SERVICE_NUM; j++)
		{
			g_test_quick2_ctx2.service_table[i][j].srv_id = 0;
			g_test_quick2_ctx2.service_table[i][j].msgq_size = 64;
		}
		g_test_quick2_ctx2.group_table[i].attr = (eaf_thread_attr_t)EAF_THREAD_ATTR_INITIALIZER;
		g_test_quick2_ctx2.group_table[i].service.size = TEST_QUICK_SERVICE_NUM;
		g_test_quick2_ctx2.group_table[i].service.table = g_test_quick2_ctx2.service_table[i];
	}

	for (i = 0; i < EAF_ARRAY_SIZE(g_test_quick2_ctx2.entry_table); i++)
	{
		g_test_quick2_ctx2.entry_table[i] = NULL;
	}

	g_test_quick2_ctx.mask.inited = 0;
	g_test_quick2_ctx.mask.loaded = 0;
}

static void _quick_hook_after_fixture_setup(const char* fixture_name, int ret)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(fixture_name);

	/* If setup failure, do nothing */
	if (ret)
	{
		return;
	}

	/* If not loaded, load eaf */
	if (!g_test_quick2_ctx.mask.inited)
	{
		ASSERT_EQ_D32(test_quick2_internal_force_init_eaf(), eaf_errno_success, "%s(%d)", eaf_strerror(_a), _a);
	}

	/* load EAF */
	if (!g_test_quick2_ctx.mask.loaded)
	{
		ASSERT_EQ_D32(test_quick2_internal_force_load_eaf(), eaf_errno_success, "%s(%d)", eaf_strerror(_a), _a);
	}
}

static void _quick_hook_before_fixture_teardown(const char* fixture_name)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(fixture_name);

	eaf_exit();
	g_test_quick2_ctx.mask.inited = 0;
	g_test_quick2_ctx.mask.loaded = 0;
}

static int _test_default_init(void)
{
	return 0;
}

static void _test_default_exit(void)
{
}

int test_quick2_internal_deploy(uint32_t gid, uint32_t sid, eaf_entrypoint_t* entry)
{
	/* avoid overflow */
	if (gid >= TEST_QUICK_GROUP_NUM)
	{
		return eaf_errno_overflow;
	}

	/* Fix entry point */
	entry->on_init = entry->on_init == NULL ? _test_default_init : entry->on_init;
	entry->on_exit = entry->on_exit == NULL ? _test_default_exit : entry->on_exit;

	/* save record */
	size_t index_service;
	eaf_service_table_t* service_table = g_test_quick2_ctx2.service_table[gid];
	for (index_service = 0; index_service < TEST_QUICK_SERVICE_NUM; index_service++)
	{
		if (service_table[index_service].srv_id != 0)
		{
			continue;
		}

		service_table[index_service].srv_id = sid;
		g_test_quick2_ctx2.entry_table[gid * TEST_QUICK_GROUP_NUM + index_service] = entry;

		break;
	}

	return index_service >= TEST_QUICK_SERVICE_NUM ? eaf_errno_overflow : eaf_errno_success;
}

int test_quick2_internal_reserve(uint32_t gid, uint32_t sid)
{
	if (gid >= EAF_ARRAY_SIZE(g_test_quick2_ctx2.group_table))
	{
		return eaf_errno_overflow;
	}

	size_t index_service;
	eaf_service_table_t* service_table = g_test_quick2_ctx2.service_table[gid];
	for (index_service = 0; index_service < TEST_QUICK_SERVICE_NUM; index_service++)
	{
		if (service_table[index_service].srv_id != 0)
		{
			continue;
		}

		service_table[index_service].srv_id = sid;
		g_test_quick2_ctx2.entry_table[gid * TEST_QUICK_GROUP_NUM + index_service] = NULL;
		break;
	}

	return index_service >= TEST_QUICK_SERVICE_NUM ? eaf_errno_overflow : eaf_errno_success;
}

int test_quick2_internal_force_init_eaf(void)
{
	if (g_test_quick2_ctx.mask.inited)
	{
		return eaf_errno_duplicate;
	}

	int ret;
	if ((ret = eaf_init(g_test_quick2_ctx2.group_table, TEST_QUICK_GROUP_NUM)) < 0)
	{
		return ret;
	}

	g_test_quick2_ctx.mask.inited = 1;
	return eaf_errno_success;
}

int test_quick2_internal_force_load_eaf(void)
{
	int ret;
	if (g_test_quick2_ctx.mask.loaded)
	{
		return eaf_errno_duplicate;
	}

	size_t i;
	for (i = 0; i < EAF_ARRAY_SIZE(g_test_quick2_ctx2.group_table); i++)
	{
		size_t j;
		for (j = 0; j < EAF_ARRAY_SIZE(g_test_quick2_ctx2.service_table[i]); j++)
		{
			size_t entry_index = i * TEST_QUICK_GROUP_NUM + j;
			if (g_test_quick2_ctx2.service_table[i][j].srv_id == 0
				|| g_test_quick2_ctx2.entry_table[entry_index] == NULL)
			{
				continue;
			}

			if ((ret = eaf_register(g_test_quick2_ctx2.service_table[i][j].srv_id,
				g_test_quick2_ctx2.entry_table[entry_index])) < 0)
			{
				return ret;
			}
		}
	}

	if ((ret = eaf_load()) < 0)
	{
		return ret;
	}

	g_test_quick2_ctx.mask.loaded = 1;
	return eaf_errno_success;
}
