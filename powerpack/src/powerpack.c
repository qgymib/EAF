#include <stdlib.h>
#include "eaf/eaf.h"
#include "powerpack/message.h"
#include "powerpack/net.h"
#include "powerpack/timer.h"
#include "powerpack.h"

typedef struct powerpack_init_item
{
	int(*on_init)(void);
	void(*on_exit)(void);
}powerpack_init_item_t;

typedef struct powerpack_ctx
{
	uint32_t		service_id;		/** working service id */
	uv_loop_t		uv_loop;		/** uv loop */
	eaf_thread_t*	working;		/** working thread */
	eaf_sem_t*		sem_loop;		/** loop sem */

	struct
	{
		unsigned	looping : 1;
	}mask;
}powerpack_ctx_t;

static powerpack_ctx_t* g_powerpack_ctx = NULL;
static powerpack_init_item_t g_powerpack_table[] = {
	{ eaf_powerpack_timer_init,		eaf_powerpack_timer_exit },
	{ eaf_powerpack_message_init,	eaf_powerpack_message_exit },
	{ eaf_powerpack_net_init,		eaf_powerpack_net_exit },
};

static void _powerpack_thread(void* arg)
{
	(void)arg;
	while (g_powerpack_ctx->mask.looping)
	{
		uv_run(&g_powerpack_ctx->uv_loop, UV_RUN_DEFAULT);
		eaf_sem_pend(g_powerpack_ctx->sem_loop, (unsigned long)-1);
	}
}

static int _powerpack_on_init(void)
{
	return 0;
}

static void _powerpack_on_exit(void)
{
	/* stop thread */
	g_powerpack_ctx->mask.looping = 0;
	uv_stop(&g_powerpack_ctx->uv_loop);

	eaf_sem_post(g_powerpack_ctx->sem_loop);

	/* wait for thread exit */
	eaf_thread_destroy(g_powerpack_ctx->working);
	g_powerpack_ctx->working = NULL;
}

int eaf_powerpack_init(_In_ const eaf_powerpack_cfg_t* cfg)
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
	g_powerpack_ctx->service_id = cfg->service_id;
	if ((g_powerpack_ctx->sem_loop = eaf_sem_create(0)) == NULL)
	{
		goto err_free;
	}

	/* initialize libuv */
	if (uv_loop_init(&g_powerpack_ctx->uv_loop) < 0)
	{
		ret = eaf_errno_unknown;
		goto err_free;
	}

	static eaf_entrypoint_t service_info = { 0, NULL, _powerpack_on_init, _powerpack_on_exit };
	if ((ret = eaf_register(cfg->service_id, &service_info)) < 0)
	{
		goto err_close;
	}

	size_t i;
	for (i = 0; i < EAF_ARRAY_SIZE(g_powerpack_table); i++)
	{
		if (g_powerpack_table[i].on_init() < 0)
		{
			goto err_init;
		}
	}

	/* create working thread */
	g_powerpack_ctx->working = eaf_thread_create(&cfg->unistd, _powerpack_thread, NULL);
	if (g_powerpack_ctx->working == NULL)
	{
		goto err_init;
	}

	return eaf_errno_success;

err_init:
	for (i = 0; i < EAF_ARRAY_SIZE(g_powerpack_table); i++)
	{
		g_powerpack_table[i].on_exit();
	}
err_close:
	uv_loop_close(&g_powerpack_ctx->uv_loop);
err_free:
	if (g_powerpack_ctx->sem_loop != NULL)
	{
		eaf_sem_destroy(g_powerpack_ctx->sem_loop);
		g_powerpack_ctx->sem_loop = NULL;
	}
	free(g_powerpack_ctx);
	g_powerpack_ctx = NULL;
	return ret;
}

void eaf_powerpack_exit(void)
{
	if (g_powerpack_ctx == NULL)
	{
		return;
	}

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

	if (g_powerpack_ctx->sem_loop != NULL)
	{
		eaf_sem_destroy(g_powerpack_ctx->sem_loop);
		g_powerpack_ctx->sem_loop = NULL;
	}

	free(g_powerpack_ctx);
	g_powerpack_ctx = NULL;
}

uv_loop_t* eaf_uv_get(void)
{
	return &g_powerpack_ctx->uv_loop;
}

void eaf_uv_mod(void)
{
	eaf_sem_post(g_powerpack_ctx->sem_loop);
}

uint32_t powerpack_get_service_id(void)
{
	return g_powerpack_ctx->service_id;
}
