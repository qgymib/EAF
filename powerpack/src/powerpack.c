#include "EAF/eaf.h"
#include "powerpack/timer.h"
#include "powerpack.h"

typedef struct powerpack_init_item
{
	int(*on_init)(void);
	void(*on_exit)(void);
}powerpack_init_item_t;

typedef struct powerpack_ctx
{
	uv_loop_t		uv_loop;
	eaf_thread_t*	working;		/** working thread */

	struct
	{
		unsigned	looping : 1;
	}mask;
}powerpack_ctx_t;

static powerpack_ctx_t* g_powerpack_ctx = NULL;
static powerpack_init_item_t g_powerpack_table[] = {
	{ eaf_powerpack_timer_init, eaf_powerpack_timer_exit },
};

static void _powerpack_thread(void* arg)
{
	(void)arg;
	while (g_powerpack_ctx->mask.looping)
	{
		uv_run(&g_powerpack_ctx->uv_loop, UV_RUN_DEFAULT);
	}
}

int eaf_powerpack_init(const eaf_powerpack_cfg_t* cfg)
{
	int ret = eaf_errno_success;
	if (g_powerpack_ctx != NULL)
	{
		return eaf_errno_duplicate;
	}

	if ((g_powerpack_ctx = calloc(1, sizeof(*g_powerpack_ctx))) == NULL)
	{
		return eaf_errno_memory;
	}
	g_powerpack_ctx->mask.looping = 1;

	/* initialize libuv */
	if (uv_loop_init(&g_powerpack_ctx->uv_loop) < 0)
	{
		free(g_powerpack_ctx);
		g_powerpack_ctx = NULL;
		return eaf_errno_unknown;
	}

	/* create working thread */
	g_powerpack_ctx->working = eaf_thread_create(&cfg->unistd, _powerpack_thread, NULL);
	if (g_powerpack_ctx->working == NULL)
	{
		goto err;
	}

	size_t i;
	for (i = 0; i < EAF_ARRAY_SIZE(g_powerpack_table); i++)
	{
		if (g_powerpack_table[i].on_init() < 0)
		{
			goto err;
		}
	}

	return eaf_errno_success;

err:
	eaf_powerpack_exit();
	return ret;
}

void eaf_powerpack_exit(void)
{
	if (g_powerpack_ctx == NULL)
	{
		return;
	}

	g_powerpack_ctx->mask.looping = 0;
	if (g_powerpack_ctx->working != NULL)
	{
		eaf_thread_destroy(g_powerpack_ctx->working);
		g_powerpack_ctx->working = NULL;
	}

	size_t i;
	for (i = 0; i < EAF_ARRAY_SIZE(g_powerpack_table); i++)
	{
		g_powerpack_table[i].on_exit();
	}

	uv_loop_close(&g_powerpack_ctx->uv_loop);

	free(g_powerpack_ctx);
	g_powerpack_ctx = NULL;
}

uv_loop_t* powerpack_get_uv(void)
{
	if (g_powerpack_ctx == NULL)
	{
		return NULL;
	}

	return &g_powerpack_ctx->uv_loop;
}
