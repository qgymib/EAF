#include "random.h"

#if defined(_MSC_VER)
#define _CRT_RAND_S
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#endif

/**
 * @brief Default random device fd
 */
#define RANDOM_DEFAULT_FD		((int)(-1))
#define RANDOM_DEFAULT_SEED		((uint64_t)(-1))

typedef struct random_ctx
{
	uint64_t	seed;			/**< Pseudo random seed */
#if !defined(_MSC_VER)
	int			random_fd;		/**< Random device file descriptor */
#endif
}random_ctx_t;

static random_ctx_t g_random_ctx = {
	RANDOM_DEFAULT_SEED,
#if !defined(_MSC_VER)
	RANDOM_DEFAULT_FD,
#endif
};

/**
* @brief Non-failure random algorithm.
*
* Linear Congruence Algorithm
*
* @return Random number
*/
static unsigned int _random_non_failure(void)
{
#if defined(_MSC_VER)
	unsigned int pad;
	if (rand_s(&pad) == 0)
	{
		return pad;
	}
#endif
	g_random_ctx.seed = 6364136223846793005ULL * g_random_ctx.seed + 1;
	return g_random_ctx.seed >> 33;
}

int eaf_random_init(uint32_t seed)
{
	g_random_ctx.seed = seed - 1;

#if defined(_MSC_VER)
	return eaf_errno_success;
#else
	if (g_random_ctx.random_fd >= 0)
	{
		return eaf_errno_duplicate;
	}

	g_random_ctx.random_fd = open("/dev/urandom", O_RDONLY);
	if (g_random_ctx.random_fd < 0)
	{
		return eaf_errno_unknown;
	}

	return eaf_errno_success;
#endif
}

void eaf_random_exit(void)
{
#if !defined(_MSC_VER)
	if (g_random_ctx.random_fd < 0)
	{
		return;
	}

	close(g_random_ctx.random_fd);
	g_random_ctx.random_fd = RANDOM_DEFAULT_FD;
#endif
	g_random_ctx.seed = RANDOM_DEFAULT_SEED;
}

void eaf_random(void* buffer, size_t size)
{
	size_t diff_size;
	void* start_pos;
	unsigned int pad;

#if defined(_MSC_VER)
	/**
	 * Because `buffer' might not be aligned to machine size, it is better to
	 * deal with unaligned part.
	 */
	start_pos = (void*)EAF_ALIGN(buffer, sizeof(pad));
	if ((diff_size = (uintptr_t)start_pos - (uintptr_t)buffer) != 0)
	{
		assert(diff_size < sizeof(unsigned int));
		pad = _random_non_failure();
		memcpy(buffer, &pad, diff_size);
	}
#else
	/**
	 * Under linux, we just read from `/dev/urandom'
	 */
	ssize_t read_size = read(g_random_ctx.random_fd, buffer, size);
	if (read_size >= size)
	{
		return;
	}

	start_pos = (void*)((uintptr_t)buffer + (read_size >= 0 ? read_size : 0));
#endif

	/* Fill random data */
	while ((uintptr_t)start_pos <= (uintptr_t)buffer + size - sizeof(unsigned int))
	{
		*(unsigned int*)start_pos = _random_non_failure();
		start_pos = (void*)((uintptr_t)start_pos + 4);
	}

	/* The tail part may not be filled, handle it here */
	if ((uintptr_t)start_pos >= (uintptr_t)buffer + size)
	{
		return;
	}

	diff_size = (uintptr_t)buffer + size - (uintptr_t)start_pos;
	assert(diff_size < sizeof(unsigned int));

	pad = _random_non_failure();
	memcpy(start_pos, &pad, diff_size);
	return;
}

uint32_t eaf_random32(void)
{
	uint32_t pad;
	eaf_random(&pad, sizeof(pad));

	return pad;
}

uint64_t eaf_random64(void)
{
	uint64_t pad;
	eaf_random(&pad, sizeof(pad));

	return pad;
}
