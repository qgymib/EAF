#include "eaf/eaf.h"
#include "ctest/ctest.h"

typedef struct test_eaf_map_node
{
	eaf_map_node_t		node;	/** node */
	struct
	{
		size_t			value;	/** value */
	}data;
}test_eaf_map_node_t;

static test_eaf_map_node_t	s_eaf_map_node[8];
static eaf_map_t			s_eaf_map_table;

static int _test_eaf_map_on_cmp(const eaf_map_node_t* key1, const eaf_map_node_t* key2, void* arg)
{
	(void)arg;
	const test_eaf_map_node_t* n1 = EAF_CONTAINER_OF(key1, test_eaf_map_node_t, node);
	const test_eaf_map_node_t* n2 = EAF_CONTAINER_OF(key2, test_eaf_map_node_t, node);

	if (n1->data.value == n2->data.value)
	{
		return 0;
	}
	return n1->data.value < n2->data.value ? -1 : 1;
}

TEST_FIXTURE_SETUP(eaf_map)
{
	eaf_map_init(&s_eaf_map_table, _test_eaf_map_on_cmp, NULL);

	size_t i;
	for (i = 0; i < EAF_ARRAY_SIZE(s_eaf_map_node); i++)
	{
		s_eaf_map_node[i].data.value = i;
		ASSERT_EQ_D32(eaf_map_insert(&s_eaf_map_table, &s_eaf_map_node[i].node), 0);
	}

	ASSERT_EQ_SIZE(eaf_map_size(&s_eaf_map_table), EAF_ARRAY_SIZE(s_eaf_map_node));
}

TEST_FIXTURE_TEAREDOWN(eaf_map)
{
	// do nothing
}

TEST_F(eaf_map, insert_r)
{
	/* reset */
	eaf_map_init(&s_eaf_map_table, _test_eaf_map_on_cmp, NULL);

	int i;
	for (i = (int)EAF_ARRAY_SIZE(s_eaf_map_node) - 1; i >= 0; i--)
	{
		ASSERT_EQ_D32(eaf_map_insert(&s_eaf_map_table, &s_eaf_map_node[i].node), 0);
	}

	ASSERT_EQ_SIZE(eaf_map_size(&s_eaf_map_table), EAF_ARRAY_SIZE(s_eaf_map_node));
}

TEST_F(eaf_map, insert_duplicate)
{
	test_eaf_map_node_t tmp_node = s_eaf_map_node[EAF_ARRAY_SIZE(s_eaf_map_node) / 2];
	ASSERT_LT_D32(eaf_map_insert(&s_eaf_map_table, &tmp_node.node), 0);

	ASSERT_EQ_SIZE(eaf_map_size(&s_eaf_map_table), EAF_ARRAY_SIZE(s_eaf_map_node));
}

TEST_F(eaf_map, erase)
{
	eaf_map_erase(&s_eaf_map_table, &s_eaf_map_node[EAF_ARRAY_SIZE(s_eaf_map_node) / 2].node);

	ASSERT_EQ_SIZE(eaf_map_size(&s_eaf_map_table), EAF_ARRAY_SIZE(s_eaf_map_node) - 1);

	eaf_map_node_t* it = eaf_map_begin(&s_eaf_map_table);
	for (; it != NULL; it = eaf_map_next(it))
	{
		test_eaf_map_node_t* rec = EAF_CONTAINER_OF(it, test_eaf_map_node_t, node);
		ASSERT_NE_PTR(rec, &s_eaf_map_node[EAF_ARRAY_SIZE(s_eaf_map_node) / 2]);
	}
}

TEST_F(eaf_map, erase_l)
{
	size_t i;
	for (i = 0; i < EAF_ARRAY_SIZE(s_eaf_map_node); i++)
	{
		eaf_map_erase(&s_eaf_map_table, &s_eaf_map_node[i].node);
	}
	ASSERT_EQ_SIZE(eaf_map_size(&s_eaf_map_table), 0);
}

TEST_F(eaf_map, erase_r)
{
	int i;
	for (i = EAF_ARRAY_SIZE(s_eaf_map_node) - 1; i >= 0; i--)
	{
		eaf_map_erase(&s_eaf_map_table, &s_eaf_map_node[i].node);
	}
	ASSERT_EQ_SIZE(eaf_map_size(&s_eaf_map_table), 0);
}

TEST_F(eaf_map, find)
{
	/* find a exists value */
	{
		test_eaf_map_node_t tmp_node = s_eaf_map_node[EAF_ARRAY_SIZE(s_eaf_map_node) / 2];
		eaf_map_node_t* it = eaf_map_find(&s_eaf_map_table, &tmp_node.node);
		ASSERT_NE_PTR(it, NULL);

		test_eaf_map_node_t* real_node = EAF_CONTAINER_OF(it, test_eaf_map_node_t, node);
		ASSERT_EQ_PTR(real_node, &s_eaf_map_node[EAF_ARRAY_SIZE(s_eaf_map_node) / 2]);
	}

	/* find nonexists value */
	{
		test_eaf_map_node_t tmp_node;
		tmp_node.data.value = EAF_ARRAY_SIZE(s_eaf_map_node) + 1;

		ASSERT_EQ_PTR(eaf_map_find(&s_eaf_map_table, &tmp_node.node), NULL);
	}
}

TEST_F(eaf_map, find_lower)
{
	/* reset */
	eaf_map_init(&s_eaf_map_table, _test_eaf_map_on_cmp, NULL);

	/* value: 2, 4, 6, 8, 10, 12, 14, 16 */
	size_t i;
	for (i = 0; i < EAF_ARRAY_SIZE(s_eaf_map_node); i++)
	{
		s_eaf_map_node[i].data.value = (i + 1) * 2;
		ASSERT_EQ_D32(eaf_map_insert(&s_eaf_map_table, &s_eaf_map_node[i].node), 0);
	}

	/* find lower 1 */
	{
		const unsigned target_value = 0;
		test_eaf_map_node_t tmp_node;
		tmp_node.data.value = target_value;

		eaf_map_node_t* it = eaf_map_find_lower(&s_eaf_map_table, &tmp_node.node);
		ASSERT_EQ_PTR(it, NULL);
	}

	/* find lower 2 */
	{
		const unsigned target_value = 2;
		test_eaf_map_node_t tmp_node;
		tmp_node.data.value = target_value;

		eaf_map_node_t* it = eaf_map_find_lower(&s_eaf_map_table, &tmp_node.node);
		ASSERT_NE_PTR(it, NULL);

		test_eaf_map_node_t* rec = EAF_CONTAINER_OF(it, test_eaf_map_node_t, node);
		ASSERT_EQ_SIZE(rec->data.value, target_value);
		ASSERT_EQ_PTR(rec, &s_eaf_map_node[0]);
	}

	/* find lower 8 */
	{
		const unsigned target_value = 8;
		test_eaf_map_node_t tmp_node;
		tmp_node.data.value = target_value;

		eaf_map_node_t* it = eaf_map_find_lower(&s_eaf_map_table, &tmp_node.node);
		ASSERT_NE_PTR(it, NULL);

		test_eaf_map_node_t* rec = EAF_CONTAINER_OF(it, test_eaf_map_node_t, node);
		ASSERT_EQ_SIZE(rec->data.value, target_value);
		ASSERT_EQ_PTR(rec, &s_eaf_map_node[3]);
	}

	/* find lower 11 */
	{
		const unsigned target_value = 11;
		test_eaf_map_node_t tmp_node;
		tmp_node.data.value = target_value;

		eaf_map_node_t* it = eaf_map_find_lower(&s_eaf_map_table, &tmp_node.node);
		ASSERT_NE_PTR(it, NULL);

		test_eaf_map_node_t* rec = EAF_CONTAINER_OF(it, test_eaf_map_node_t, node);
		ASSERT_EQ_SIZE(rec->data.value, target_value - 1);
		ASSERT_EQ_PTR(rec, &s_eaf_map_node[4]);
	}

	/* find lower 16 */
	{
		const unsigned target_value = 16;
		test_eaf_map_node_t tmp_node;
		tmp_node.data.value = target_value;

		eaf_map_node_t* it = eaf_map_find_lower(&s_eaf_map_table, &tmp_node.node);
		ASSERT_NE_PTR(it, NULL);

		test_eaf_map_node_t* rec = EAF_CONTAINER_OF(it, test_eaf_map_node_t, node);
		ASSERT_EQ_SIZE(rec->data.value, target_value);
		ASSERT_EQ_PTR(rec, &s_eaf_map_node[7]);
	}

	/* find lower 17 */
	{
		const unsigned target_value = 17;
		test_eaf_map_node_t tmp_node;
		tmp_node.data.value = target_value;

		eaf_map_node_t* it = eaf_map_find_lower(&s_eaf_map_table, &tmp_node.node);
		ASSERT_NE_PTR(it, NULL);

		test_eaf_map_node_t* rec = EAF_CONTAINER_OF(it, test_eaf_map_node_t, node);
		ASSERT_EQ_SIZE(rec->data.value, 16);
		ASSERT_EQ_PTR(rec, &s_eaf_map_node[7]);
	}
}

TEST_F(eaf_map, find_upper)
{
	/* reset */
	eaf_map_init(&s_eaf_map_table, _test_eaf_map_on_cmp, NULL);

	/* value: 2, 4, 6, 8, 10, 12, 14, 16 */
	size_t i;
	for (i = 0; i < EAF_ARRAY_SIZE(s_eaf_map_node); i++)
	{
		s_eaf_map_node[i].data.value = (i + 1) * 2;
		ASSERT_EQ_D32(eaf_map_insert(&s_eaf_map_table, &s_eaf_map_node[i].node), 0);
	}

	/* find upper 1 */
	{
		const unsigned target_value = 1;
		test_eaf_map_node_t tmp_node;
		tmp_node.data.value = target_value;

		eaf_map_node_t* it = eaf_map_find_upper(&s_eaf_map_table, &tmp_node.node);
		ASSERT_NE_PTR(it, NULL);

		test_eaf_map_node_t* rec = EAF_CONTAINER_OF(it, test_eaf_map_node_t, node);
		ASSERT_EQ_SIZE(rec->data.value, 2);
		ASSERT_EQ_PTR(rec, &s_eaf_map_node[0]);
	}

	/* find upper 2 */
	{
		const unsigned target_value = 2;
		test_eaf_map_node_t tmp_node;
		tmp_node.data.value = target_value;

		eaf_map_node_t* it = eaf_map_find_upper(&s_eaf_map_table, &tmp_node.node);
		ASSERT_NE_PTR(it, NULL);

		test_eaf_map_node_t* rec = EAF_CONTAINER_OF(it, test_eaf_map_node_t, node);
		ASSERT_EQ_SIZE(rec->data.value, 4);
		ASSERT_EQ_PTR(rec, &s_eaf_map_node[1]);
	}

	/* find upper 3 */
	{
		const unsigned target_value = 3;
		test_eaf_map_node_t tmp_node;
		tmp_node.data.value = target_value;

		eaf_map_node_t* it = eaf_map_find_upper(&s_eaf_map_table, &tmp_node.node);
		ASSERT_NE_PTR(it, NULL);

		test_eaf_map_node_t* rec = EAF_CONTAINER_OF(it, test_eaf_map_node_t, node);
		ASSERT_EQ_SIZE(rec->data.value, 4);
		ASSERT_EQ_PTR(rec, &s_eaf_map_node[1]);
	}

	/* find upper 16 */
	{
		const unsigned target_value = 16;
		test_eaf_map_node_t tmp_node;
		tmp_node.data.value = target_value;

		eaf_map_node_t* it = eaf_map_find_upper(&s_eaf_map_table, &tmp_node.node);
		ASSERT_EQ_PTR(it, NULL);
	}

	/* find upper 17 */
	{
		const unsigned target_value = 17;
		test_eaf_map_node_t tmp_node;
		tmp_node.data.value = target_value;

		eaf_map_node_t* it = eaf_map_find_upper(&s_eaf_map_table, &tmp_node.node);
		ASSERT_EQ_PTR(it, NULL);
	}
}

TEST_F(eaf_map, begin)
{
	/* reset */
	eaf_map_init(&s_eaf_map_table, _test_eaf_map_on_cmp, NULL);

	ASSERT_EQ_PTR(eaf_map_begin(&s_eaf_map_table), NULL);
}

TEST_F(eaf_map, end)
{
	/* reset */
	eaf_map_init(&s_eaf_map_table, _test_eaf_map_on_cmp, NULL);

	ASSERT_EQ_PTR(eaf_map_end(&s_eaf_map_table), NULL);
}

TEST_F(eaf_map, next)
{
	size_t i = 0;
	eaf_map_node_t* it = eaf_map_begin(&s_eaf_map_table);
	for (; it != NULL; it = eaf_map_next(it), i++)
	{
		test_eaf_map_node_t* rec = EAF_CONTAINER_OF(it, test_eaf_map_node_t, node);
		ASSERT_EQ_PTR(rec, &s_eaf_map_node[i]);
	}
	ASSERT_EQ_SIZE(i, EAF_ARRAY_SIZE(s_eaf_map_node));
}

TEST_F(eaf_map, prev)
{
	int i = EAF_ARRAY_SIZE(s_eaf_map_node) - 1;
	eaf_map_node_t* it = eaf_map_end(&s_eaf_map_table);
	for (; it != NULL; it = eaf_map_prev(it), i--)
	{
		test_eaf_map_node_t* rec = EAF_CONTAINER_OF(it, test_eaf_map_node_t, node);
		ASSERT_EQ_PTR(rec, &s_eaf_map_node[i]);
	}
	ASSERT_EQ_D32(i, -1);
}
