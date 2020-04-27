#include <stdlib.h>
#include <string.h>
#include "eaf/powerpack.h"
#include "etest/etest.h"

TEST_CLASS_SETUP(powerpack_ringbuffer)
{
}

TEST_CLASS_TEAREDOWN(powerpack_ringbuffer)
{
}

TEST_F(powerpack_ringbuffer, init_exit)
{
	const size_t cache_size = 1024;
	uint8_t* cache = malloc(cache_size);
	ASSERT_PTR_NE(cache, NULL);

	eaf_ringbuffer_t* rb = eaf_ringbuffer_init(cache, cache_size);
	ASSERT_PTR_NE(rb, (void*)NULL);

	eaf_ringbuffer_counter_t counter;
	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &counter), 0);
	ASSERT_NUM_EQ(counter.committed, 0);
	ASSERT_NUM_EQ(counter.reading, 0);
	ASSERT_NUM_EQ(counter.writing, 0);

	free(cache);
}

TEST_F(powerpack_ringbuffer, consume_empty)
{
	/* initialized ring buffer */
	const size_t cache_size = 1024;
	uint8_t* cache = malloc(cache_size);
	ASSERT_PTR_NE(cache, NULL);
	eaf_ringbuffer_t* rb = eaf_ringbuffer_init(cache, cache_size);
	ASSERT_PTR_NE(rb, NULL);

	eaf_ringbuffer_counter_t counter;
	/* empty ring buffer must not contains any data */
	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &counter), 0);

	/* empty ring buffer must consume failed */
	ASSERT_PTR_EQ(eaf_ringbuffer_consume(rb), (void*)NULL);

	/* ring buffer must keep empty */
	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &counter), (size_t)0);

	free(cache);
}

/**
 * step: 
 * 1. ring buffer is empty
 * 2. acquire write token (success)
 * 3. acquire read token (failure)
 * 4. commit write token (success)
 */
TEST_F(powerpack_ringbuffer, consume_when_reserve_not_committed)
{
	const size_t cache_size = 1024;
	uint8_t* cache = malloc(cache_size);
	ASSERT_PTR_NE(cache, NULL);
	eaf_ringbuffer_t* rb = eaf_ringbuffer_init(cache, cache_size);
	ASSERT_PTR_NE(rb, NULL);

	eaf_ringbuffer_counter_t counter;
	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &counter), (size_t)0);

	/* acquire write token */
	eaf_ringbuffer_token_t* write_token = eaf_ringbuffer_reserve(rb, 1, 0);
	ASSERT_PTR_NE(write_token, NULL);
	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &counter), 1);
	ASSERT_NUM_EQ(counter.writing, 1);

	/* before `write_token` was committed, consume must failed */
	ASSERT_PTR_EQ(eaf_ringbuffer_consume(rb), NULL);
	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &counter), 1);
	ASSERT_NUM_EQ(counter.writing, 1);

	/* commit write token */
	ASSERT_NUM_EQ(eaf_ringbuffer_commit(rb, write_token, 0), 0);
	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &counter), (size_t)1);
	ASSERT_NUM_EQ(counter.committed, (size_t)1);

	/* acquire read token must success */
	eaf_ringbuffer_token_t* read_token = eaf_ringbuffer_consume(rb);
	ASSERT_PTR_NE(read_token, NULL);
	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &counter), 1);
	ASSERT_NUM_EQ(counter.reading, 1);

	ASSERT_NUM_EQ(eaf_ringbuffer_commit(rb, read_token, 0), 0);
	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &counter), 0);

	free(cache);
}

/**
 * step:
 * 1. ring buffer is empty
 * 2. acquire write token_1 (success)
 * 3. acquire write token_2 (success)
 * 4. acquire read token (failure)
 * 5. commit write token_2 (success)
 * 6. acquire read token (failure)
 * 7. commit write token_1 (success)
 * 8. acquire read token (success)
 */
TEST_F(powerpack_ringbuffer, consume_when_reserve_not_committed2)
{
	/* make ring buffer */
	const size_t cache_size = 1024;
	uint8_t* cache = malloc(cache_size);
	ASSERT_PTR_NE(cache, NULL);
	eaf_ringbuffer_t* rb = eaf_ringbuffer_init(cache, cache_size);
	ASSERT_PTR_NE(rb, NULL);

	eaf_ringbuffer_counter_t counter;
	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &counter), 0);

	/* acquire 2 write token */
	eaf_ringbuffer_token_t* write_token_1 = eaf_ringbuffer_reserve(rb, 1, 0);
	ASSERT_PTR_NE(write_token_1, NULL);
	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &counter), 1);
	ASSERT_NUM_EQ(counter.writing, 1);

	eaf_ringbuffer_token_t* write_token_2 = eaf_ringbuffer_reserve(rb, 1, 0);
	ASSERT_PTR_NE(write_token_2, NULL);
	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &counter), 2);
	ASSERT_NUM_EQ(counter.writing, 2);

	/* before write token was committed, consume must failed */
	ASSERT_PTR_EQ(eaf_ringbuffer_consume(rb), NULL);
	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &counter), 2);
	ASSERT_NUM_EQ(counter.writing, 2);

	/* commit 2st write token */
	ASSERT_NUM_EQ(eaf_ringbuffer_commit(rb, write_token_2, 0), 0);
	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &counter), 2);
	ASSERT_NUM_EQ(counter.writing, 1);
	ASSERT_NUM_EQ(counter.committed, 1);

	/* because 1st write token not committed, consume must failed */
	ASSERT_PTR_EQ(eaf_ringbuffer_consume(rb), NULL);
	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &counter), 2);
	ASSERT_NUM_EQ(counter.writing, 1);
	ASSERT_NUM_EQ(counter.committed, 1);

	/* commit 1st write token */
	ASSERT_NUM_EQ(eaf_ringbuffer_commit(rb, write_token_1, 0), 0);
	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &counter), 2);
	ASSERT_NUM_EQ(counter.committed, 2);

	/* because 1st write token was committed, `consume` is success */
	eaf_ringbuffer_token_t* consume_token = eaf_ringbuffer_consume(rb);
	ASSERT_PTR_NE(consume_token, NULL);
	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &counter), 2);
	ASSERT_NUM_EQ(counter.committed, 1);
	ASSERT_NUM_EQ(counter.reading, 1);

	ASSERT_NUM_EQ(eaf_ringbuffer_commit(rb, consume_token, 0), 0);
	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &counter), 1);
	ASSERT_NUM_EQ(counter.committed, 1);

	/* now 2st write token is available */
	eaf_ringbuffer_token_t* consume_token_2 = eaf_ringbuffer_consume(rb);
	ASSERT_PTR_NE(consume_token_2, NULL);
	ASSERT_NUM_EQ(eaf_ringbuffer_commit(rb, consume_token_2, 0), 0);
	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &counter), 0);

	free(cache);
}

/**
 * step:
 * 1. reserve (success)
 * 2. commit (success)
 * 3. reserve (success)
 */
TEST_F(powerpack_ringbuffer, consume_after_reserve_10K)
{
	const size_t cache_size = 1024;
	uint8_t* cache = malloc(cache_size);
	ASSERT_PTR_NE(cache, NULL);
	eaf_ringbuffer_t* rb = eaf_ringbuffer_init(cache, cache_size);
	ASSERT_PTR_NE(rb, NULL);

	eaf_ringbuffer_counter_t counter;
	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &counter), 0);

	size_t i;
	for (i = 0; i < 10000; i++)
	{
		uint64_t data = i;
		{
			eaf_ringbuffer_token_t* token = eaf_ringbuffer_reserve(rb, sizeof(uint64_t), 0);
			ASSERT_PTR_NE(token, NULL);
			ASSERT_NUM_EQ(token->size.size, sizeof(uint64_t));

			ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &counter), 1);
			ASSERT_NUM_EQ(counter.writing, 1);

			memcpy(token->data, &data, sizeof(uint64_t));
			ASSERT_NUM_EQ(eaf_ringbuffer_commit(rb, token, 0), 0);

			ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &counter), 1);
			ASSERT_NUM_EQ(counter.committed, 1);
		}

		{
			eaf_ringbuffer_token_t* token = eaf_ringbuffer_consume(rb);
			ASSERT_PTR_NE(token, NULL);
			ASSERT_NUM_EQ(token->size.size, sizeof(uint64_t));
			ASSERT_NUM_EQ(memcmp(token->data, &data, sizeof(uint64_t)), 0);

			ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &counter), 1);
			ASSERT_NUM_EQ(counter.reading, 1);

			ASSERT_NUM_EQ(eaf_ringbuffer_commit(rb, token, 0), 0);

			ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &counter), 0);
		}
	}

	free(cache);
}

/**
 * step:
 * 1. full fill ring buffer
 * 2. reserve data (check: failure)
 */
TEST_F(powerpack_ringbuffer, max_element_no_overwrite)
{
	const size_t number_of_elements = 128;
	const size_t cache_size = eaf_ringbuffer_heap_cost() + number_of_elements * eaf_ringbuffer_node_cost(sizeof(uint64_t));
	uint8_t* cache = malloc(cache_size);
	ASSERT_PTR_NE(cache, NULL);
	eaf_ringbuffer_t* rb = eaf_ringbuffer_init(cache, cache_size);
	ASSERT_PTR_NE(rb, NULL);

	eaf_ringbuffer_counter_t counter;
	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &counter), 0);

	/* full fill ring buffer */
	size_t i;
	for (i = 0; i < number_of_elements; i++)
	{
		eaf_ringbuffer_token_t* token = eaf_ringbuffer_reserve(rb, sizeof(uint64_t), 0);
		ASSERT_PTR_NE(token, NULL);
		ASSERT_NUM_EQ(token->size.size, sizeof(uint64_t));

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &counter), i + 1);
		ASSERT_NUM_EQ(counter.writing, (size_t)1);
		ASSERT_NUM_EQ(counter.committed, (size_t)i);

		memset(token->data, 0, sizeof(uint64_t));
		ASSERT_NUM_EQ(eaf_ringbuffer_commit(rb, token, 0), 0);

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &counter), i + 1);
		ASSERT_NUM_EQ(counter.committed, i + 1);
	}

	/* because ring buffer is full, reserve must failed */
	{
		eaf_ringbuffer_token_t* token = eaf_ringbuffer_reserve(rb, sizeof(uint64_t), 0);
		ASSERT_PTR_EQ(token, NULL);

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &counter), number_of_elements);
		ASSERT_NUM_EQ(counter.committed, number_of_elements);
	}

	free(cache);
}

/**
 * 8 byte full fill test
 */
TEST_F(powerpack_ringbuffer, reserve_consume_8bit_full)
{
	const size_t cache_size = 8 * 1024;
	uint8_t* cache = malloc(cache_size);
	ASSERT_PTR_NE(cache, NULL);
	eaf_ringbuffer_t* rb = eaf_ringbuffer_init(cache, cache_size);
	ASSERT_PTR_NE(rb, NULL);

	eaf_ringbuffer_counter_t rb_counter;
	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rb_counter), 0);

	/* full fill ring buffer with 8 bite */
	uint64_t token_counter = 0;
	for (;; token_counter++)
	{
		eaf_ringbuffer_token_t* token = eaf_ringbuffer_reserve(rb, sizeof(uint64_t), 0);
		if (token == NULL)
		{
			break;
		}

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rb_counter), token_counter + 1);
		ASSERT_NUM_EQ(rb_counter.writing, 1);
		ASSERT_NUM_EQ(rb_counter.committed, token_counter);

		memcpy(token->data, &token_counter, sizeof(uint64_t));
		ASSERT_NUM_EQ(eaf_ringbuffer_commit(rb, token, 0), 0);

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rb_counter), token_counter + 1);
		ASSERT_NUM_EQ(rb_counter.committed, token_counter + 1);
	}

	/* data check */
	uint64_t i;
	for (i = 0; i < token_counter; i++)
	{
		eaf_ringbuffer_token_t* token = eaf_ringbuffer_consume(rb);
		ASSERT_PTR_NE(token, NULL);
		ASSERT_NUM_EQ(token->size.size, sizeof(uint64_t));
		ASSERT_NUM_EQ(memcmp(token->data, &i, sizeof(uint64_t)), 0);

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rb_counter), token_counter - i);
		ASSERT_NUM_EQ(rb_counter.committed, token_counter - i - 1);
		ASSERT_NUM_EQ(rb_counter.reading, 1);

		ASSERT_NUM_EQ(eaf_ringbuffer_commit(rb, token, 0), 0);

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rb_counter), token_counter - i - 1);
		ASSERT_NUM_EQ(rb_counter.committed, token_counter - i - 1);
	}

	free(cache);
}

/**
 * generic overwrite test
 */
TEST_F(powerpack_ringbuffer, overwrite_generic)
{
	const size_t node_number = 5;
	const size_t cache_size = eaf_ringbuffer_heap_cost() + node_number * eaf_ringbuffer_node_cost(sizeof(uint64_t));
	uint8_t* cache = malloc(cache_size);
	ASSERT_PTR_NE(cache, NULL);
	eaf_ringbuffer_t* rb = eaf_ringbuffer_init(cache, cache_size);
	ASSERT_PTR_NE(rb, NULL);

	eaf_ringbuffer_counter_t rb_counter;
	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rb_counter), 0);

	/* full fill ring buffer */
	uint64_t i;
	for (i = 0; i < node_number; i++)
	{
		eaf_ringbuffer_token_t* token = eaf_ringbuffer_reserve(rb, sizeof(uint64_t), 0);
		ASSERT_PTR_NE(token, NULL);

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rb_counter), i + 1);
		ASSERT_NUM_EQ(rb_counter.writing, 1);
		ASSERT_NUM_EQ(rb_counter.committed, i);

		memcpy(token->data, &i, sizeof(uint64_t));
		ASSERT_NUM_EQ(eaf_ringbuffer_commit(rb, token, 0), 0);

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rb_counter), i + 1);
		ASSERT_NUM_EQ(rb_counter.committed, i + 1);
	}

	/* check oldest data */
	{
		eaf_ringbuffer_token_t* token = eaf_ringbuffer_consume(rb);
		ASSERT_PTR_NE(token, NULL);

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rb_counter), node_number);
		ASSERT_NUM_EQ(rb_counter.committed, node_number - 1);
		ASSERT_NUM_EQ(rb_counter.reading, 1);

		/* the first data should be 0 */
		uint64_t tmpval = 0;
		ASSERT_NUM_EQ(token->size.size, sizeof(tmpval));
		ASSERT_NUM_EQ(memcmp(token->data, &tmpval, sizeof(tmpval)), 0);

		/* cancel this operation */
		ASSERT_NUM_EQ(eaf_ringbuffer_commit(rb, token, eaf_ringbuffer_flag_discard), 0);

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rb_counter), node_number);
		ASSERT_NUM_EQ(rb_counter.committed, node_number);
	}

	/* overwrite oldest data */
	{
		uint64_t tmp_val = node_number;

		eaf_ringbuffer_token_t* token = eaf_ringbuffer_reserve(rb, sizeof(uint64_t), eaf_ringbuffer_flag_overwrite);
		ASSERT_PTR_NE(token, NULL);

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rb_counter), node_number);
		ASSERT_NUM_EQ(rb_counter.committed, node_number - 1);
		ASSERT_NUM_EQ(rb_counter.writing, 1);

		memcpy(token->data, &tmp_val, sizeof(tmp_val));
		ASSERT_NUM_EQ(eaf_ringbuffer_commit(rb, token, 0), 0);

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rb_counter), node_number);
		ASSERT_NUM_EQ(rb_counter.committed, node_number);
	}

	/* check oldest data */
	{
		eaf_ringbuffer_token_t* token = eaf_ringbuffer_consume(rb);
		ASSERT_PTR_NE(token, NULL);

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rb_counter), node_number);
		ASSERT_NUM_EQ(rb_counter.committed, node_number - 1);
		ASSERT_NUM_EQ(rb_counter.reading, 1);

		uint64_t tmpval = 1;
		ASSERT_NUM_EQ(token->size.size, sizeof(tmpval));
		memcpy(&tmpval, token->data, sizeof(uint64_t));
		ASSERT_NUM_EQ(tmpval, 1);

		/* cancel this operation */
		ASSERT_NUM_EQ(eaf_ringbuffer_commit(rb, token, eaf_ringbuffer_flag_discard), 0);

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rb_counter), node_number);
		ASSERT_NUM_EQ(rb_counter.committed, node_number);
	}

	/* overwrite oldest data */
	{
		uint64_t tmp_val = node_number + 1;

		eaf_ringbuffer_token_t* token = eaf_ringbuffer_reserve(rb, sizeof(uint64_t) * 2, eaf_ringbuffer_flag_overwrite);
		ASSERT_PTR_NE(token, NULL);

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rb_counter), node_number - 1);
		ASSERT_NUM_EQ(rb_counter.committed, node_number - 2);
		ASSERT_NUM_EQ(rb_counter.writing, 1);

		memcpy(token->data, &tmp_val, sizeof(tmp_val));
		memcpy(token->data + sizeof(uint64_t), &tmp_val, sizeof(tmp_val));
		ASSERT_NUM_EQ(eaf_ringbuffer_commit(rb, token, 0), 0);

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rb_counter), node_number - 1);
		ASSERT_NUM_EQ(rb_counter.committed, node_number - 1);
	}

	/* check oldest data */
	{
		eaf_ringbuffer_token_t* token = eaf_ringbuffer_consume(rb);
		ASSERT_PTR_NE(token, NULL);

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rb_counter), node_number - 1);
		ASSERT_NUM_EQ(rb_counter.committed, node_number - 2);
		ASSERT_NUM_EQ(rb_counter.reading, (size_t)1);

		uint64_t tmpval = 3;
		ASSERT_NUM_EQ(token->size.size, sizeof(tmpval));
		ASSERT_NUM_EQ(memcmp(token->data, &tmpval, sizeof(tmpval)), 0);

		/* cancel this operation */
		ASSERT_NUM_EQ(eaf_ringbuffer_commit(rb, token, eaf_ringbuffer_flag_discard), 0);

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rb_counter), node_number - 1);
		ASSERT_NUM_EQ(rb_counter.committed, node_number - 1);
	}

	free(cache);
}

/**
 * overwrite non-committed state node
 */
TEST_F(powerpack_ringbuffer, overwrite_non_reserve)
{
	const size_t node_num = 5;
	const size_t cache_size = eaf_ringbuffer_heap_cost() + node_num * eaf_ringbuffer_node_cost(sizeof(uint64_t));
	uint8_t* cache = malloc(cache_size);
	ASSERT_PTR_NE(cache, NULL);
	eaf_ringbuffer_t* rb = eaf_ringbuffer_init(cache, cache_size);
	ASSERT_PTR_NE(rb, NULL);

	eaf_ringbuffer_counter_t rb_counter;
	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rb_counter), (size_t)0);

	/* ensure all nodes is in writing state */
	size_t i;
	for (i = 0; i < node_num; i++)
	{
		eaf_ringbuffer_token_t* token = eaf_ringbuffer_reserve(rb, sizeof(uint64_t), 0);
		ASSERT_PTR_NE(token, NULL);

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rb_counter), i + 1);
		ASSERT_NUM_EQ(rb_counter.writing, i + 1);
	}

	/* try to overwrite a node should failed */
	{
		eaf_ringbuffer_token_t* token = eaf_ringbuffer_reserve(rb, sizeof(uint64_t), eaf_ringbuffer_flag_overwrite);
		ASSERT_PTR_EQ(token, (void*)NULL);

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rb_counter), node_num);
		ASSERT_NUM_EQ(rb_counter.writing, node_num);
	}

	free(cache);
}

/**
 * overwrite single node
 */
TEST_F(powerpack_ringbuffer, overwrite_only_one)
{
	const size_t cache_size = eaf_ringbuffer_heap_cost() + eaf_ringbuffer_node_cost(100);
	uint8_t* cache = malloc(cache_size);
	ASSERT_PTR_NE(cache, NULL);
	eaf_ringbuffer_t* rb = eaf_ringbuffer_init(cache, cache_size);
	ASSERT_PTR_NE(rb, NULL);

	eaf_ringbuffer_counter_t rb_counter;
	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rb_counter), 0);

	/* reserve 100 byte */
	{
		eaf_ringbuffer_token_t* token = eaf_ringbuffer_reserve(rb, 100, 0);
		ASSERT_PTR_NE(token, NULL);

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rb_counter), 1);
		ASSERT_NUM_EQ(rb_counter.writing, 1);

		ASSERT_NUM_EQ(eaf_ringbuffer_commit(rb, token, 0), 0);

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rb_counter), 1);
		ASSERT_NUM_EQ(rb_counter.committed, 1);
	}

	/* reserve 1 byte more should failed */
	{
		ASSERT_PTR_EQ(eaf_ringbuffer_reserve(rb, 1, 0), NULL);

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rb_counter), 1);
		ASSERT_NUM_EQ(rb_counter.committed, 1);
	}

	/* overwrite */
	{
		eaf_ringbuffer_token_t* token = eaf_ringbuffer_reserve(rb, 1, eaf_ringbuffer_flag_overwrite);
		ASSERT_PTR_NE(token, NULL);
		ASSERT_NUM_EQ(token->size.size, 1);

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rb_counter), 1);
		ASSERT_NUM_EQ(rb_counter.writing, 1);

		ASSERT_NUM_EQ(eaf_ringbuffer_commit(rb, token, 0), 0);

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rb_counter), 1);
		ASSERT_NUM_EQ(rb_counter.committed, 1);
	}

	/* check data */
	{
		eaf_ringbuffer_token_t* token = eaf_ringbuffer_consume(rb);
		ASSERT_PTR_NE(token, NULL);
		ASSERT_NUM_EQ(token->size.size, 1);

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rb_counter), 1);
		ASSERT_NUM_EQ(rb_counter.reading, 1);

		ASSERT_NUM_EQ(eaf_ringbuffer_commit(rb, token, 0), 0);
		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rb_counter), 0);
	}

	/* consume empty ring buffer should failed */
	{
		ASSERT_PTR_EQ(eaf_ringbuffer_consume(rb), NULL);
		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rb_counter), 0);
	}

	free(cache);
}

/**
 * cancel write operation
 */
TEST_F(powerpack_ringbuffer, write_discard_only_one)
{
	const size_t cache_size = 1 * 1024;
	uint8_t* cache = malloc(cache_size);
	ASSERT_PTR_NE(cache, NULL);
	eaf_ringbuffer_t* rb = eaf_ringbuffer_init(cache, cache_size);
	ASSERT_PTR_NE(rb, NULL);

	eaf_ringbuffer_counter_t rbc;
	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), 0);

	/* acquire a write token and discard it */
	{
		uint64_t val = 1;

		eaf_ringbuffer_token_t* token = eaf_ringbuffer_reserve(rb, sizeof(uint64_t), 0);
		ASSERT_PTR_NE(token, NULL);
		ASSERT_NUM_EQ(token->size.size, sizeof(val));
		memcpy(token->data, &val, sizeof(val));

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), 1);
		ASSERT_NUM_EQ(rbc.writing, (size_t)1);

		ASSERT_NUM_EQ(eaf_ringbuffer_commit(rb, token, eaf_ringbuffer_flag_discard), 0);
		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), 0);
	}

	{
		eaf_ringbuffer_token_t* token = eaf_ringbuffer_consume(rb);
		ASSERT_PTR_EQ(token,NULL);
		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), 0);
	}

	free(cache);
}

/**
 * consume a single node ring buffer and discard operation
 */
TEST_F(powerpack_ringbuffer, consume_discard_only_one)
{
	const size_t cache_size = 1 * 1024;
	uint8_t* cache = malloc(cache_size);
	ASSERT_PTR_NE(cache, NULL);
	eaf_ringbuffer_t* rb = eaf_ringbuffer_init(cache, cache_size);
	ASSERT_PTR_NE(rb, NULL);

	eaf_ringbuffer_counter_t rbc;
	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), 0);

	/* write data */
	const uint64_t tmpval = __LINE__;
	{
		eaf_ringbuffer_token_t* token = eaf_ringbuffer_reserve(rb, sizeof(tmpval), 0);
		ASSERT_PTR_NE(token, NULL);
		ASSERT_NUM_EQ(token->size.size, sizeof(tmpval));

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), 1);
		ASSERT_NUM_EQ(rbc.writing, 1);

		memcpy(token->data, &tmpval, sizeof(tmpval));
		ASSERT_NUM_EQ(eaf_ringbuffer_commit(rb, token, 0), 0);

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), 1);
		ASSERT_NUM_EQ(rbc.committed, 1);
	}

	/* consume and cancel */
	{
		eaf_ringbuffer_token_t* token = eaf_ringbuffer_consume(rb);
		ASSERT_PTR_NE(token, NULL);
		ASSERT_NUM_EQ(token->size.size, sizeof(tmpval));
		ASSERT_NUM_EQ(memcmp(token->data, &tmpval, sizeof(tmpval)), 0);

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), 1);
		ASSERT_NUM_EQ(rbc.reading, 1);

		ASSERT_NUM_EQ(eaf_ringbuffer_commit(rb, token, eaf_ringbuffer_flag_discard), 0);
		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), 1);
		ASSERT_NUM_EQ(rbc.committed, 1);
	}

	/* consume and cancel again */
	{
		eaf_ringbuffer_token_t* token = eaf_ringbuffer_consume(rb);
		ASSERT_PTR_NE(token, NULL);
		ASSERT_NUM_EQ(token->size.size, sizeof(tmpval));
		ASSERT_NUM_EQ(memcmp(token->data, &tmpval, sizeof(tmpval)), 0);

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), 1);
		ASSERT_NUM_EQ(rbc.reading, 1);

		ASSERT_NUM_EQ(eaf_ringbuffer_commit(rb, token, eaf_ringbuffer_flag_discard), 0);
		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), 1);
		ASSERT_NUM_EQ(rbc.committed, 1);
	}

	/* consume */
	{
		eaf_ringbuffer_token_t* token = eaf_ringbuffer_consume(rb);
		ASSERT_PTR_NE(token, NULL);
		ASSERT_NUM_EQ(token->size.size, sizeof(tmpval));
		ASSERT_NUM_EQ(memcmp(token->data, &tmpval, sizeof(tmpval)), 0);

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), 1);
		ASSERT_NUM_EQ(rbc.reading, 1);

		ASSERT_NUM_EQ(eaf_ringbuffer_commit(rb, token, 0), 0);
		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), 0);
	}

	/* consume again */
	{
		ASSERT_PTR_EQ(eaf_ringbuffer_consume(rb), NULL);
		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), 0);
	}

	free(cache);
}

/**
 * step:
 * 1. 1st consume
 * 2. 2st consume
 * 3. discard 1st consume token
 */
TEST_F(powerpack_ringbuffer, discard_older_consume)
{
	const size_t cache_size = 1 * 1024;
	uint8_t* cache = malloc(cache_size);
	ASSERT_PTR_NE(cache, (void*)NULL);
	eaf_ringbuffer_t* rb = eaf_ringbuffer_init(cache, cache_size);
	ASSERT_PTR_NE(rb, (void*)NULL);

	eaf_ringbuffer_counter_t rbc;
	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), 0);

	/* full fill ring buffer */
	size_t number_of_ele = 0;
	for (;;)
	{
		eaf_ringbuffer_token_t* token = eaf_ringbuffer_reserve(rb, sizeof(uint64_t), 0);
		if (token == NULL)
		{
			break;
		}

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), number_of_ele + 1);
		ASSERT_NUM_EQ(rbc.committed, number_of_ele);
		ASSERT_NUM_EQ(rbc.writing, 1);

		ASSERT_NUM_EQ(eaf_ringbuffer_commit(rb, token, 0), 0);

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), number_of_ele + 1);
		ASSERT_NUM_EQ(rbc.committed, number_of_ele + 1);

		number_of_ele++;
	}

	/* acquire consume token */
	eaf_ringbuffer_token_t* token_1 = eaf_ringbuffer_consume(rb);
	ASSERT_PTR_NE(token_1, NULL);

	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), number_of_ele);
	ASSERT_NUM_EQ(rbc.committed, number_of_ele - 1);
	ASSERT_NUM_EQ(rbc.reading, (size_t)1);

	eaf_ringbuffer_token_t* token_2 = eaf_ringbuffer_consume(rb);
	ASSERT_PTR_NE(token_2, NULL);

	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), number_of_ele);
	ASSERT_NUM_EQ(rbc.committed, number_of_ele - 2);
	ASSERT_NUM_EQ(rbc.reading, (size_t)2);

	/* discard 1st token should failed */
	ASSERT_NUM_LT(eaf_ringbuffer_commit(rb, token_1, eaf_ringbuffer_flag_discard), 0);
	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), number_of_ele);
	ASSERT_NUM_EQ(rbc.committed, number_of_ele - 2);
	ASSERT_NUM_EQ(rbc.reading, (size_t)2);

	/* discard 2st token should success */
	ASSERT_NUM_EQ(eaf_ringbuffer_commit(rb, token_2, eaf_ringbuffer_flag_discard), 0);
	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), number_of_ele);
	ASSERT_NUM_EQ(rbc.committed, number_of_ele - 1);
	ASSERT_NUM_EQ(rbc.reading, (size_t)1);

	/* now discard 1st token should success */
	ASSERT_NUM_EQ(eaf_ringbuffer_commit(rb, token_1, eaf_ringbuffer_flag_discard), 0);
	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), number_of_ele);
	ASSERT_NUM_EQ(rbc.committed, number_of_ele);

	free(cache);
}

/**
 * step:
 * 1. 1st consume
 * 2. 2st consume
 * 3. force discard 1st consume
 */
TEST_F(powerpack_ringbuffer, force_discard_older_consume)
{
	const size_t number_of_elements = 2;
	const size_t cache_size = eaf_ringbuffer_heap_cost() + number_of_elements * eaf_ringbuffer_node_cost(sizeof(uint32_t));
	uint8_t* cache = malloc(cache_size);
	ASSERT_PTR_NE(cache, NULL);
	eaf_ringbuffer_t* rb = eaf_ringbuffer_init(cache, cache_size);
	ASSERT_PTR_NE(rb, NULL);

	eaf_ringbuffer_counter_t rbc;
	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), (size_t)0);

	/* full fill ring buffer */
	uint32_t i;
	for (i = 0; i < number_of_elements; i++)
	{
		eaf_ringbuffer_token_t* token = eaf_ringbuffer_reserve(rb, sizeof(i), 0);
		ASSERT_PTR_NE(token, NULL);
		memcpy(token->data, &i, sizeof(i));

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), i + 1);
		ASSERT_NUM_EQ(rbc.committed, i);
		ASSERT_NUM_EQ(rbc.writing, (size_t)1);

		ASSERT_NUM_EQ(eaf_ringbuffer_commit(rb, token, 0), 0);
		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), i + 1);
		ASSERT_NUM_EQ(rbc.committed, i + 1);
	}

	/* acquire consume token */
	eaf_ringbuffer_token_t* token_1 = eaf_ringbuffer_consume(rb);
	ASSERT_PTR_NE(token_1, NULL);

	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), number_of_elements);
	ASSERT_NUM_EQ(rbc.reading, (size_t)1);
	ASSERT_NUM_EQ(rbc.committed, number_of_elements - 1);

	eaf_ringbuffer_token_t* token_2 = eaf_ringbuffer_consume(rb);
	ASSERT_PTR_NE(token_2, NULL);

	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), number_of_elements);
	ASSERT_NUM_EQ(rbc.reading, (size_t)2);
	ASSERT_NUM_EQ(rbc.committed, number_of_elements - 2);

	/* discard 1st token */
	{
		/* by default discard should failed because 2st token exists */
		ASSERT_NUM_LT(eaf_ringbuffer_commit(rb, token_1, eaf_ringbuffer_flag_discard), 0);
		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), number_of_elements);
		ASSERT_NUM_EQ(rbc.reading, (size_t)2);
		ASSERT_NUM_EQ(rbc.committed, number_of_elements - 2);

		/* force discard 1st can be success */
		ASSERT_NUM_EQ(eaf_ringbuffer_commit(rb, token_1, eaf_ringbuffer_flag_discard | eaf_ringbuffer_flag_abandon), 0);
		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), number_of_elements - 1);
		ASSERT_NUM_EQ(rbc.reading, (size_t)1);
		ASSERT_NUM_EQ(rbc.committed, number_of_elements - 2);
	}

	/* commit 2st token */
	{
		ASSERT_NUM_EQ(eaf_ringbuffer_commit(rb, token_2, 0), 0);
		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), number_of_elements - 2);
		ASSERT_NUM_EQ(rbc.reading, (size_t)0);
		ASSERT_NUM_EQ(rbc.committed, number_of_elements - 2);
	}

	/* consume should failed because ring buffer is empty */
	{
		ASSERT_PTR_EQ(eaf_ringbuffer_consume(rb), NULL);
		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), number_of_elements - 2);
		ASSERT_NUM_EQ(rbc.committed, number_of_elements - 2);
	}

	free(cache);
}

/**
 * create ring buffer with 1 byte buffer
 */
TEST_F(powerpack_ringbuffer, buffer_too_small)
{
	const size_t cache_size = 1;
	uint8_t* cache = malloc(cache_size);
	ASSERT_PTR_NE(cache, NULL);

	/* should initialize failed */
	ASSERT_PTR_EQ(eaf_ringbuffer_init(cache, cache_size), NULL);

	free(cache);
}

/**
 * try to reserve a too large block in ring buffer
 */
TEST_F(powerpack_ringbuffer, reserve_empty_but_too_large)
{
	const size_t cache_size = 1 * 1024;
	uint8_t* cache = malloc(cache_size);
	ASSERT_PTR_NE(cache, NULL);
	eaf_ringbuffer_t* rb = eaf_ringbuffer_init(cache, cache_size);
	ASSERT_PTR_NE(rb, NULL);

	eaf_ringbuffer_counter_t rbc;
	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), (size_t)0);

	eaf_ringbuffer_token_t* token = eaf_ringbuffer_reserve(rb, cache_size + 1, 0);
	ASSERT_PTR_EQ(token, NULL);

	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), (size_t)0);

	free(cache);
}

/**
 * discard middle node write operation
 */
TEST_F(powerpack_ringbuffer, discard_middle)
{
	const size_t cache_size = eaf_ringbuffer_heap_cost() + 3 * eaf_ringbuffer_node_cost(sizeof(uint64_t));
	uint8_t* cache = malloc(cache_size);
	ASSERT_PTR_NE(cache, NULL);
	eaf_ringbuffer_t* rb = eaf_ringbuffer_init(cache, cache_size);
	ASSERT_PTR_NE(rb, (void*)NULL);

	eaf_ringbuffer_counter_t rbc;
	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), (size_t)0);

	/* acquire 3 tokens */
	eaf_ringbuffer_token_t* token_1 = eaf_ringbuffer_reserve(rb, sizeof(uint64_t), 0);
	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), (size_t)1);
	ASSERT_NUM_EQ(rbc.writing, (size_t)1);
	ASSERT_PTR_NE(token_1, (void*)NULL);

	eaf_ringbuffer_token_t* token_2 = eaf_ringbuffer_reserve(rb, sizeof(uint64_t), 0);
	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), (size_t)2);
	ASSERT_NUM_EQ(rbc.writing, (size_t)2);
	ASSERT_PTR_NE(token_2, (void*)NULL);

	eaf_ringbuffer_token_t* token_3 = eaf_ringbuffer_reserve(rb, sizeof(uint64_t), 0);
	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), (size_t)3);
	ASSERT_NUM_EQ(rbc.writing, (size_t)3);
	ASSERT_PTR_NE(token_3, (void*)NULL);

	/* the 4st token should not be reserved */
	ASSERT_PTR_EQ(eaf_ringbuffer_reserve(rb, sizeof(uint64_t), 0), NULL);
	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), (size_t)3);
	ASSERT_NUM_EQ(rbc.writing, (size_t)3);

	/* write to 1st token */
	{
		uint64_t tmpval = 100;
		memcpy(token_1->data, &tmpval, token_1->size.size);
		ASSERT_NUM_EQ(eaf_ringbuffer_commit(rb, token_1, 0), 0);

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), (size_t)3);
		ASSERT_NUM_EQ(rbc.committed, (size_t)1);
		ASSERT_NUM_EQ(rbc.writing, (size_t)2);
	}

	/* write to 3st token */
	{
		uint64_t tmpval = 107;
		memcpy(token_3->data, &tmpval, token_3->size.size);
		ASSERT_NUM_EQ(eaf_ringbuffer_commit(rb, token_3, 0), 0);

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), (size_t)3);
		ASSERT_NUM_EQ(rbc.committed, (size_t)2);
		ASSERT_NUM_EQ(rbc.writing, (size_t)1);
	}

	/* discard 2st token */
	{
		ASSERT_NUM_EQ(eaf_ringbuffer_commit(rb, token_2, eaf_ringbuffer_flag_discard), 0);

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), (size_t)2);
		ASSERT_NUM_EQ(rbc.committed, (size_t)2);
	}

	/* check 1st data */
	eaf_ringbuffer_token_t* r_token_1 = NULL;
	{
		r_token_1 = eaf_ringbuffer_consume(rb);
		ASSERT_PTR_NE(r_token_1, NULL);
		ASSERT_NUM_EQ(r_token_1->size.size, sizeof(uint64_t));

		uint64_t tmpval = 100;
		ASSERT_NUM_EQ(memcmp(r_token_1->data, &tmpval, sizeof(tmpval)), 0);

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), (size_t)2);
		ASSERT_NUM_EQ(rbc.committed, (size_t)1);
		ASSERT_NUM_EQ(rbc.reading, (size_t)1);
	}

	/* check 2st data */
	eaf_ringbuffer_token_t* r_token_2 = NULL;
	{
		r_token_2 = eaf_ringbuffer_consume(rb);
		ASSERT_PTR_NE(r_token_2, NULL);
		ASSERT_NUM_EQ(r_token_2->size.size, sizeof(uint64_t));

		uint64_t tmpval = 107;
		ASSERT_NUM_EQ(memcmp(r_token_2->data, &tmpval, sizeof(tmpval)), 0);

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), (size_t)2);
		ASSERT_NUM_EQ(rbc.reading, (size_t)2);
	}

	/* commit consume */
	{
		ASSERT_NUM_EQ(eaf_ringbuffer_commit(rb, r_token_2, 0), 0);
		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), (size_t)1);
		ASSERT_NUM_EQ(rbc.reading, (size_t)1);

		ASSERT_NUM_EQ(eaf_ringbuffer_commit(rb, r_token_1, 0), 0);
		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), (size_t)0);
	}

	/* ring buffer should be empty */
	{
		ASSERT_PTR_EQ(eaf_ringbuffer_consume(rb), NULL);
		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), (size_t)0);
	}

	free(cache);
}

/**
 * discard newest write token
 */
TEST_F(powerpack_ringbuffer, discard_newest)
{
	const size_t number_of_elements = 3;
	const size_t cache_size = eaf_ringbuffer_heap_cost() + number_of_elements * eaf_ringbuffer_node_cost(sizeof(uint64_t));
	uint8_t* cache = malloc(cache_size);
	ASSERT_PTR_NE(cache, (void*)NULL);
	eaf_ringbuffer_t* rb = eaf_ringbuffer_init(cache, cache_size);
	ASSERT_PTR_NE(rb, (void*)NULL);

	eaf_ringbuffer_counter_t rbc;
	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), (size_t)0);

	/* write to first n-1 elements */
	uint64_t i;
	for (i = 0; i < number_of_elements - 1; i++)
	{
		eaf_ringbuffer_token_t* token = eaf_ringbuffer_reserve(rb, sizeof(uint64_t), 0);
		ASSERT_PTR_NE(token, (void*)NULL);
		memcpy(token->data, &i, sizeof(uint64_t));

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), i + 1);
		ASSERT_NUM_EQ(rbc.writing, (size_t)1);
		ASSERT_NUM_EQ(rbc.committed, (size_t)i);

		ASSERT_NUM_EQ(eaf_ringbuffer_commit(rb, token, 0), 0);
		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), i + 1);
		ASSERT_NUM_EQ(rbc.committed, i + 1);
	}

	/* acquire a write token and discard it */
	{
		eaf_ringbuffer_token_t* token = eaf_ringbuffer_reserve(rb, sizeof(uint64_t), 0);
		ASSERT_PTR_NE(token, (void*)NULL);

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), number_of_elements);
		ASSERT_NUM_EQ(rbc.committed, number_of_elements - 1);
		ASSERT_NUM_EQ(rbc.writing, (size_t)1);

		ASSERT_NUM_EQ(eaf_ringbuffer_commit(rb, token, eaf_ringbuffer_flag_discard), 0);
		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), number_of_elements - 1);
		ASSERT_NUM_EQ(rbc.committed, number_of_elements - 1);
	}

	/* the 1st token should not affected by last operation */
	{
		eaf_ringbuffer_token_t* token = eaf_ringbuffer_consume(rb);
		ASSERT_PTR_NE(token, (void*)NULL);
		ASSERT_NUM_EQ(token->size.size, sizeof(uint64_t));

		uint64_t tmpval = 0;
		ASSERT_NUM_EQ(memcmp(token->data, &tmpval, sizeof(uint64_t)), 0);

		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), number_of_elements - 1);
		ASSERT_NUM_EQ(rbc.committed, number_of_elements - 2);
		ASSERT_NUM_EQ(rbc.reading, (size_t)1);

		ASSERT_NUM_EQ(eaf_ringbuffer_commit(rb, token, eaf_ringbuffer_flag_discard), 0);
		ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &rbc), number_of_elements - 1);
		ASSERT_NUM_EQ(rbc.committed, number_of_elements - 1);
	}

	free(cache);
}

TEST_F(powerpack_ringbuffer, counter_empty)
{
	const size_t number_of_elements = 3;
	const size_t cache_size = eaf_ringbuffer_heap_cost() + number_of_elements * eaf_ringbuffer_node_cost(sizeof(uint64_t));
	uint8_t* cache = malloc(cache_size);
	ASSERT_PTR_NE(cache, (void*)NULL);
	eaf_ringbuffer_t* rb = eaf_ringbuffer_init(cache, cache_size);
	ASSERT_PTR_NE(rb, (void*)NULL);

	/* check counter */
	eaf_ringbuffer_counter_t counter;
	ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &counter), (size_t)0);

	free(cache);
}

TEST_F(powerpack_ringbuffer, counter_full)
{
	const size_t number_of_elements = 3;
	const size_t cache_size = eaf_ringbuffer_heap_cost() + number_of_elements * eaf_ringbuffer_node_cost(sizeof(uint64_t));
	uint8_t* cache = malloc(cache_size);
	ASSERT_PTR_NE(cache, (void*)NULL);
	eaf_ringbuffer_t* rb = eaf_ringbuffer_init(cache, cache_size);
	ASSERT_PTR_NE(rb, (void*)NULL);

	/* full fill ring buffer */
	size_t i;
	for (i = 0; i < number_of_elements; i++)
	{
		eaf_ringbuffer_token_t* token = eaf_ringbuffer_reserve(rb, sizeof(uint64_t), 0);
		ASSERT_PTR_NE(token, (void*)NULL);

		/* check data before commit */
		{
			eaf_ringbuffer_counter_t counter;
			ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &counter), i + 1);
			ASSERT_NUM_EQ(counter.committed, i);
			ASSERT_NUM_EQ(counter.reading, (size_t)0);
			ASSERT_NUM_EQ(counter.writing, (size_t)1);
		}

		ASSERT_NUM_EQ(eaf_ringbuffer_commit(rb, token, 0), 0);

		/* check data */
		{
			eaf_ringbuffer_counter_t counter;
			ASSERT_NUM_EQ(eaf_ringbuffer_count(rb, &counter), i + 1);
			ASSERT_NUM_EQ(counter.committed, i + 1);
			ASSERT_NUM_EQ(counter.reading, (size_t)0);
			ASSERT_NUM_EQ(counter.writing, (size_t)0);
		}
	}

	free(cache);
}

static int _powerpack_ringbuffer_foreach(const eaf_ringbuffer_token_t* token, void* arg)
{
	uint64_t* p_dat_ring = (uint64_t*)token->data;
	uint64_t* p_dat_key = (uint64_t*)arg;

	ASSERT_NUM_EQ(*p_dat_key, *p_dat_ring);
	*p_dat_key = *p_dat_key + 1;

	return 0;
}

TEST_F(powerpack_ringbuffer, foreach)
{
	const size_t number_of_elements = 3;
	const size_t cache_size = eaf_ringbuffer_heap_cost() + number_of_elements * eaf_ringbuffer_node_cost(sizeof(uint64_t));
	uint8_t* cache = malloc(cache_size);
	ASSERT_PTR_NE(cache, (void*)NULL);
	eaf_ringbuffer_t* rb = eaf_ringbuffer_init(cache, cache_size);
	ASSERT_PTR_NE(rb, (void*)NULL);

	/* full fill ring buffer */
	size_t i;
	for (i = 0; i < number_of_elements; i++)
	{
		eaf_ringbuffer_token_t* token = eaf_ringbuffer_reserve(rb, sizeof(uint64_t), 0);
		ASSERT_PTR_NE(token, (void*)NULL);

		uint64_t tmp_val = i;
		memcpy(token->data, &tmp_val, sizeof(uint64_t));

		ASSERT_NUM_EQ(eaf_ringbuffer_commit(rb, token, 0), 0);
	}

	/* work through */
	{
		uint64_t tmp = 0;
		ASSERT_NUM_EQ(eaf_ringbuffer_foreach(rb, _powerpack_ringbuffer_foreach, &tmp), number_of_elements);
	}

	free(cache);
}
