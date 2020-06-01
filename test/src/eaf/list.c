#include "eaf/eaf.h"
#include "ctest/ctest.h"

typedef struct test_eaf_list_node
{
	eaf_list_node_t			node;
	struct
	{
		unsigned			value;
	}data;
}test_eaf_list_node_t;

static test_eaf_list_node_t s_eaf_list_node[8];
static eaf_list_t			s_eaf_list_queue;

TEST_FIXTURE_SETUP(eaf_list)
{
	eaf_list_init(&s_eaf_list_queue);

	size_t i;
	for (i = 0; i < EAF_ARRAY_SIZE(s_eaf_list_node); i++)
	{
		eaf_list_push_back(&s_eaf_list_queue, &s_eaf_list_node[i].node);
	}
	ASSERT_EQ_SIZE(eaf_list_size(&s_eaf_list_queue), EAF_ARRAY_SIZE(s_eaf_list_node));
}

TEST_FIXTURE_TEAREDOWN(eaf_list)
{
	// do nothing
}

TEST_F(eaf_list, push_front)
{
	test_eaf_list_node_t tmp_node;
	tmp_node.data.value = 99;

	eaf_list_push_front(&s_eaf_list_queue, &tmp_node.node);

	/* check size */
	ASSERT_EQ_SIZE(eaf_list_size(&s_eaf_list_queue), EAF_ARRAY_SIZE(s_eaf_list_node) + 1);

	/* check front node */
	{
		eaf_list_node_t* it = eaf_list_begin(&s_eaf_list_queue);
		ASSERT_NE_PTR(it, NULL);

		test_eaf_list_node_t* data = EAF_CONTAINER_OF(it, test_eaf_list_node_t, node);
		ASSERT_EQ_PTR(data, &tmp_node);
	}
}

TEST_F(eaf_list, empty_push_front)
{
	eaf_list_init(&s_eaf_list_queue);
	eaf_list_push_front(&s_eaf_list_queue, &s_eaf_list_node[0].node);

	ASSERT_EQ_SIZE(eaf_list_size(&s_eaf_list_queue), 1);
	ASSERT_EQ_PTR(EAF_CONTAINER_OF(eaf_list_begin(&s_eaf_list_queue), test_eaf_list_node_t, node), &s_eaf_list_node[0]);
	ASSERT_EQ_PTR(EAF_CONTAINER_OF(eaf_list_end(&s_eaf_list_queue), test_eaf_list_node_t, node), &s_eaf_list_node[0]);
}

TEST_F(eaf_list, push_back)
{
	test_eaf_list_node_t tmp_node;
	tmp_node.data.value = 99;

	eaf_list_push_back(&s_eaf_list_queue, &tmp_node.node);

	/* check size */
	ASSERT_EQ_SIZE(eaf_list_size(&s_eaf_list_queue), EAF_ARRAY_SIZE(s_eaf_list_node) + 1);

	/* check back node */
	{
		eaf_list_node_t* it = eaf_list_end(&s_eaf_list_queue);
		ASSERT_NE_PTR(it, NULL);

		test_eaf_list_node_t* data = EAF_CONTAINER_OF(it, test_eaf_list_node_t, node);
		ASSERT_EQ_PTR(data, &tmp_node);
	}
}

TEST_F(eaf_list, insert_before)
{
	eaf_list_node_t* it;
	test_eaf_list_node_t* data;

	test_eaf_list_node_t tmp_node_1;
	tmp_node_1.data.value = 11;
	test_eaf_list_node_t tmp_node_2;
	tmp_node_2.data.value = 22;

	/* insert before HEAD */
	{
		eaf_list_insert_before(&s_eaf_list_queue, &s_eaf_list_node[0].node, &tmp_node_1.node);
		ASSERT_EQ_SIZE(eaf_list_size(&s_eaf_list_queue), EAF_ARRAY_SIZE(s_eaf_list_node) + 1);

		it = eaf_list_begin(&s_eaf_list_queue);
		ASSERT_NE_PTR(it, NULL);

		data = EAF_CONTAINER_OF(it, test_eaf_list_node_t, node);
		ASSERT_EQ_PTR(data, &tmp_node_1);
	}

	/* insert before TAIL */
	{
		eaf_list_insert_before(&s_eaf_list_queue, &s_eaf_list_node[EAF_ARRAY_SIZE(s_eaf_list_node) - 1].node, &tmp_node_2.node);
		ASSERT_EQ_SIZE(eaf_list_size(&s_eaf_list_queue), EAF_ARRAY_SIZE(s_eaf_list_node) + 2);

		it = eaf_list_end(&s_eaf_list_queue);
		ASSERT_NE_PTR(it, NULL);
		data = EAF_CONTAINER_OF(it, test_eaf_list_node_t, node);
		ASSERT_EQ_PTR(data, &s_eaf_list_node[EAF_ARRAY_SIZE(s_eaf_list_node) - 1]);

		it = eaf_list_prev(it);
		ASSERT_NE_PTR(it, NULL);
		data = EAF_CONTAINER_OF(it, test_eaf_list_node_t, node);
		ASSERT_EQ_PTR(data, &tmp_node_2);
	}
}

TEST_F(eaf_list, insert_after)
{
	eaf_list_node_t* it;
	test_eaf_list_node_t* data;

	test_eaf_list_node_t tmp_node_1;
	tmp_node_1.data.value = 11;
	test_eaf_list_node_t tmp_node_2;
	tmp_node_2.data.value = 22;

	/* insert after HEAD */
	{
		eaf_list_insert_after(&s_eaf_list_queue, &s_eaf_list_node[0].node, &tmp_node_1.node);
		ASSERT_EQ_SIZE(eaf_list_size(&s_eaf_list_queue), EAF_ARRAY_SIZE(s_eaf_list_node) + 1);

		it = eaf_list_begin(&s_eaf_list_queue);
		ASSERT_NE_PTR(it, NULL);
		data = EAF_CONTAINER_OF(it, test_eaf_list_node_t, node);
		ASSERT_EQ_PTR(data, &s_eaf_list_node[0]);

		it = eaf_list_next(it);
		ASSERT_NE_PTR(it, NULL);
		data = EAF_CONTAINER_OF(it, test_eaf_list_node_t, node);
		ASSERT_EQ_PTR(data, &tmp_node_1);
	}

	/* insert after TAIL */
	{
		eaf_list_insert_after(&s_eaf_list_queue, &s_eaf_list_node[EAF_ARRAY_SIZE(s_eaf_list_node) - 1].node, &tmp_node_2.node);
		ASSERT_EQ_SIZE(eaf_list_size(&s_eaf_list_queue), EAF_ARRAY_SIZE(s_eaf_list_node) + 2);

		it = eaf_list_end(&s_eaf_list_queue);
		ASSERT_NE_PTR(it, NULL);
		data = EAF_CONTAINER_OF(it, test_eaf_list_node_t, node);
		ASSERT_EQ_PTR(data, &tmp_node_2);
	}
}

TEST_F(eaf_list, erase)
{
	eaf_list_erase(&s_eaf_list_queue, &s_eaf_list_node[EAF_ARRAY_SIZE(s_eaf_list_node) / 2].node);
	ASSERT_EQ_SIZE(eaf_list_size(&s_eaf_list_queue), EAF_ARRAY_SIZE(s_eaf_list_node) - 1);

	eaf_list_node_t* it = eaf_list_begin(&s_eaf_list_queue);
	for (; it != NULL; it = eaf_list_next(it))
	{
		test_eaf_list_node_t* data = EAF_CONTAINER_OF(it, test_eaf_list_node_t, node);
		ASSERT_NE_PTR(data, &s_eaf_list_node[EAF_ARRAY_SIZE(s_eaf_list_node) / 2]);
	}
}

TEST_F(eaf_list, pop_front)
{
	eaf_list_node_t* node = eaf_list_pop_front(&s_eaf_list_queue);
	ASSERT_NE_PTR(node, NULL);
	ASSERT_EQ_SIZE(eaf_list_size(&s_eaf_list_queue), EAF_ARRAY_SIZE(s_eaf_list_node) - 1);

	test_eaf_list_node_t* data = EAF_CONTAINER_OF(node, test_eaf_list_node_t, node);
	ASSERT_EQ_PTR(data, &s_eaf_list_node[0]);

	eaf_list_node_t* it = eaf_list_begin(&s_eaf_list_queue);
	for (; it != NULL; it = eaf_list_next(it))
	{
		data = EAF_CONTAINER_OF(it, test_eaf_list_node_t, node);
		ASSERT_NE_PTR(data, &s_eaf_list_node[0]);
	}
}

TEST_F(eaf_list, pop_front_empty)
{
	eaf_list_init(&s_eaf_list_queue);
	ASSERT_EQ_PTR(eaf_list_pop_front(&s_eaf_list_queue), NULL);
}

TEST_F(eaf_list, pop_back)
{
	eaf_list_node_t* node = eaf_list_pop_back(&s_eaf_list_queue);
	ASSERT_NE_PTR(node, NULL);
	ASSERT_EQ_SIZE(eaf_list_size(&s_eaf_list_queue), EAF_ARRAY_SIZE(s_eaf_list_node) - 1);

	test_eaf_list_node_t* data = EAF_CONTAINER_OF(node, test_eaf_list_node_t, node);
	ASSERT_EQ_PTR(data, &s_eaf_list_node[EAF_ARRAY_SIZE(s_eaf_list_node) - 1]);

	eaf_list_node_t* it = eaf_list_begin(&s_eaf_list_queue);
	for (; it != NULL; it = eaf_list_next(it))
	{
		data = EAF_CONTAINER_OF(it, test_eaf_list_node_t, node);
		ASSERT_NE_PTR(data, &s_eaf_list_node[EAF_ARRAY_SIZE(s_eaf_list_node) - 1]);
	}
}

TEST_F(eaf_list, pop_back_empty)
{
	eaf_list_init(&s_eaf_list_queue);
	ASSERT_EQ_PTR(eaf_list_pop_back(&s_eaf_list_queue), NULL);
}

TEST_F(eaf_list, next)
{
	size_t i = 0;
	eaf_list_node_t* it = eaf_list_begin(&s_eaf_list_queue);
	for (; it != NULL; it = eaf_list_next(it), i++)
	{
		test_eaf_list_node_t* data = EAF_CONTAINER_OF(it, test_eaf_list_node_t, node);
		ASSERT_EQ_PTR(data, &s_eaf_list_node[i]);
	}
	ASSERT_EQ_SIZE(i, EAF_ARRAY_SIZE(s_eaf_list_node));
}

TEST_F(eaf_list, prev)
{
	int i = EAF_ARRAY_SIZE(s_eaf_list_node) - 1;
	eaf_list_node_t* it = eaf_list_end(&s_eaf_list_queue);
	for (; it != NULL; it = eaf_list_prev(it), i--)
	{
		test_eaf_list_node_t* data = EAF_CONTAINER_OF(it, test_eaf_list_node_t, node);
		ASSERT_EQ_PTR(data, &s_eaf_list_node[i]);
	}
	ASSERT_EQ_D32(i, -1);
}
