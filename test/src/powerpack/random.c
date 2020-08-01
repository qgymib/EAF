/**
 * @file I cannot find a good way to measure random algorithm quality,
 * so this test suit is only for coverage purpose.
 */
#include <stdlib.h>
#include "quick2.h"

typedef struct test_random_ctx
{
	char		pad;		/**< Padding byte */
	char		buffer[9];	/**< Array that start address is not align to sizeof(unsigned int) */
}test_random_ctx_t;

static test_random_ctx_t s_test_random_ctx;	/**< Must NOT initialized */

TEST_FIXTURE_SETUP(powerpack_random)
{
	ASSERT_EQ_D32(eaf_random_init(0), 0);
}

TEST_FIXTURE_TEAREDOWN(powerpack_random)
{
	eaf_random_exit();
}

TEST_F(powerpack_random, random)
{
	/* Ensure buffer address is not aligned to sizeof(unsigned int) */
	{
		uintptr_t addr = s_test_random_ctx.buffer;
		ASSERT_NE_U32(addr % sizeof(unsigned int), 0);
	}

	/* Fill buffer */
	eaf_random(s_test_random_ctx.buffer, sizeof(s_test_random_ctx.buffer));

	/* Access all elements to let Valgrind check whether all bytes are written */
	{
		uint32_t ret = eaf_hash32_bkdr(s_test_random_ctx.buffer, sizeof(s_test_random_ctx.buffer), 0);

		/*
		 * To prevent from compiler optimization, `ret' must be used.
		 *
		 * `ret' cannot be used by user codebase or any static-linked library
		 * because it may be erased by link-time optimization.
		 *
		 * So a non-return shared libc function is a better choice.
		 */
		srand(ret);
	}
}

TEST_F(powerpack_random, random32)
{
	(void)eaf_random32();
}

TEST_F(powerpack_random, random64)
{
	(void)eaf_random64();
}
