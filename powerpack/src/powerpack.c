#include <stdlib.h>
#include "eaf/eaf.h"
#include "powerpack/message.h"
#include "powerpack/net.h"
#include "powerpack/timer.h"
#include "powerpack.h"

#define FOREACH_VOID_HOOK(fn, ...)	\
	do {\
		eaf_list_node_t* it = eaf_list_begin(&g_pp_ctx.hook.table);\
		for (; it != NULL; it = eaf_list_next(it)) {\
			eaf_powerpack_hook_t* hook = EAF_CONTAINER_OF(it, eaf_powerpack_hook_t, node);\
			if (hook->hook.fn != NULL) {\
				hook->hook.fn(__VA_ARGS__);\
			}\
		}\
	} EAF_MSVC_WARNING_GUARD(4127, while (0))

#define FOREACH_INT_HOOK(fn, ...)	\
	int ret = 0;\
	int flag_first = 1;\
	eaf_list_node_t* it = eaf_list_begin(&g_pp_ctx.hook.table);\
	for (; it != NULL; it = eaf_list_next(it)) {\
		eaf_powerpack_hook_t* hook = EAF_CONTAINER_OF(it, eaf_powerpack_hook_t, node);\
		if (hook->hook.fn != NULL) {\
			int tmp_ret = hook->hook.fn(__VA_ARGS__);\
			if (flag_first) {\
				ret = tmp_ret;\
				flag_first = 0;\
			}\
		}\
	}\
	return ret;

static void _pp_hook_on_service_init_before(uint32_t id);
static void _pp_hook_on_service_init_after(uint32_t id, int ret);
static void _pp_hook_on_service_exit_before(uint32_t id);
static void _pp_hook_on_service_exit_after(uint32_t id);
static void _pp_hook_on_service_yield(uint32_t id);
static void _pp_hook_on_service_resume(uint32_t id);
static int _pp_hook_on_service_register(uint32_t id);
static int _pp_hook_on_message_send_before(uint32_t from, uint32_t to, eaf_msg_t* msg);
static void _pp_hook_on_message_send_after(uint32_t from, uint32_t to, eaf_msg_t* msg, int ret);
static int _pp_hook_on_message_handle_before(uint32_t from, uint32_t to, eaf_msg_t* msg);
static void _pp_hook_on_message_handle_after(uint32_t from, uint32_t to, eaf_msg_t* msg);
static void _pp_hook_on_load_before(void);
static void _pp_hook_on_load_after(int ret);
static void _pp_hook_on_exit_before(void);
static void _pp_hook_on_exit_after(void);

typedef struct powerpack_init_item
{
	int(*on_init)(void);
	void(*on_exit)(void);
}powerpack_init_item_t;

typedef struct powerpack_ctx
{
	eaf_thread_t*	working;		/** working thread */
	eaf_sem_t*		sem_loop;		/** loop sem */

	struct
	{
		unsigned	looping : 1;
	}mask;

	struct
	{
		eaf_list_t	table;
		eaf_hook_t	inject;
	}hook;
}pp_ctx_t;

typedef struct pp_uv_ctx
{
	uv_loop_t		uv_loop;		/** uv loop */
}pp_uv_ctx_t;

static pp_uv_ctx_t g_pp_uv_ctx;
static pp_ctx_t g_pp_ctx = {
	NULL, NULL, { 0 },
	{
		EAF_LIST_INITIALIZER,
		{
			_pp_hook_on_service_init_before,	// .on_service_init_before
			_pp_hook_on_service_init_after,		// .on_service_init_after
			_pp_hook_on_service_exit_before,	// .on_service_exit_before
			_pp_hook_on_service_exit_after,		// .on_service_exit_after
			_pp_hook_on_service_yield,			// .on_service_yield
			_pp_hook_on_service_resume,			// .on_service_resume
			_pp_hook_on_service_register,		// .on_service_register
			_pp_hook_on_message_send_before,	// .on_message_send_before
			_pp_hook_on_message_send_after,		// .on_message_send_after
			_pp_hook_on_message_handle_before,	// .on_message_handle_before
			_pp_hook_on_message_handle_after,	// .on_message_handle_after
			_pp_hook_on_load_before,			// .on_load_before
			_pp_hook_on_load_after,				// .on_load_after
			_pp_hook_on_exit_before,			// .on_exit_before
			_pp_hook_on_exit_after,				// .on_exit_after
		}
	},
};

static powerpack_init_item_t g_powerpack_table[] = {
	{ eaf_powerpack_net_init,		eaf_powerpack_net_exit },
};

static void _pp_hook_on_service_init_before(uint32_t id)
{
	FOREACH_VOID_HOOK(on_service_init_before, id);
}

static void _pp_hook_on_service_init_after(uint32_t id, int ret)
{
	FOREACH_VOID_HOOK(on_service_init_after, id, ret);
}

static void _pp_hook_on_service_exit_before(uint32_t id)
{
	FOREACH_VOID_HOOK(on_service_exit_before, id);
}

static void _pp_hook_on_service_exit_after(uint32_t id)
{
	FOREACH_VOID_HOOK(on_service_exit_after, id);
}

static void _pp_hook_on_service_yield(uint32_t id)
{
	FOREACH_VOID_HOOK(on_service_yield, id);
}

static void _pp_hook_on_service_resume(uint32_t id)
{
	FOREACH_VOID_HOOK(on_service_resume, id);
}

static int _pp_hook_on_service_register(uint32_t id)
{
	FOREACH_INT_HOOK(on_service_register, id);
}

static int _pp_hook_on_message_send_before(uint32_t from, uint32_t to, eaf_msg_t* msg)
{
	FOREACH_INT_HOOK(on_message_send_before, from, to, msg);
}

static void _pp_hook_on_message_send_after(uint32_t from, uint32_t to, eaf_msg_t* msg, int ret)
{
	FOREACH_VOID_HOOK(on_message_send_after, from, to, msg, ret);
}

static int _pp_hook_on_message_handle_before(uint32_t from, uint32_t to, eaf_msg_t* msg)
{
	FOREACH_INT_HOOK(on_message_handle_before, from, to, msg);
}

static void _pp_hook_on_message_handle_after(uint32_t from, uint32_t to, eaf_msg_t* msg)
{
	FOREACH_VOID_HOOK(on_message_handle_after, from, to, msg);
}

static void _pp_hook_on_load_before(void)
{
	FOREACH_VOID_HOOK(on_load_before);
}

static void _pp_hook_on_load_after(int ret)
{
	FOREACH_VOID_HOOK(on_load_after, ret);
}

static void _pp_hook_on_exit_before(void)
{
	FOREACH_VOID_HOOK(on_exit_before);
}

static void _pp_hook_on_exit_after(void)
{
	FOREACH_VOID_HOOK(on_exit_after);
}

static void _powerpack_thread(void* arg)
{
	(void)arg;
	while (g_pp_ctx.mask.looping)
	{
		while (uv_run(&g_pp_uv_ctx.uv_loop, UV_RUN_DEFAULT) != 0)
		{
		}

		eaf_sem_pend(g_pp_ctx.sem_loop, EAF_SEM_INFINITY);
	}
}

int eaf_powerpack_init(_In_ const eaf_powerpack_cfg_t* cfg)
{
	int ret = eaf_errno_success;

	if (g_pp_ctx.sem_loop != NULL)
	{
		return eaf_errno_duplicate;
	}

	if ((ret = eaf_inject(&g_pp_ctx.hook.inject, sizeof(g_pp_ctx.hook.inject))) < 0)
	{
		return ret;
	}

	g_pp_ctx.mask.looping = 1;
	if ((g_pp_ctx.sem_loop = eaf_sem_create(0)) == NULL)
	{
		return eaf_errno_memory;
	}

	/* initialize libuv */
	if (uv_loop_init(&g_pp_uv_ctx.uv_loop) < 0)
	{
		ret = eaf_errno_unknown;
		goto err_free;
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
	g_pp_ctx.working = eaf_thread_create(&cfg->thread_attr, _powerpack_thread, NULL);
	if (g_pp_ctx.working == NULL)
	{
		goto err_init;
	}

	return eaf_errno_success;

err_init:
	for (i = 0; i < EAF_ARRAY_SIZE(g_powerpack_table); i++)
	{
		g_powerpack_table[i].on_exit();
	}
	uv_loop_close(&g_pp_uv_ctx.uv_loop);
err_free:
	eaf_sem_destroy(g_pp_ctx.sem_loop);
	g_pp_ctx.sem_loop = NULL;
	eaf_uninject(&g_pp_ctx.hook.inject);
	return ret;
}

void eaf_powerpack_exit(void)
{
	if (g_pp_ctx.sem_loop == NULL)
	{
		return;
	}

	/* stop thread */
	g_pp_ctx.mask.looping = 0;
	eaf_sem_post(g_pp_ctx.sem_loop);
	eaf_thread_destroy(g_pp_ctx.working);
	g_pp_ctx.working = NULL;

	size_t i;
	for (i = 0; i < EAF_ARRAY_SIZE(g_powerpack_table); i++)
	{
		g_powerpack_table[i].on_exit();
	}

	uv_loop_close(&g_pp_uv_ctx.uv_loop);
	eaf_sem_destroy(g_pp_ctx.sem_loop);
	g_pp_ctx.sem_loop = NULL;
}

int eaf_powerpack_hook_register(eaf_powerpack_hook_t* hook, size_t size)
{
	if (size != sizeof(*hook))
	{
		return eaf_errno_invalid;
	}

	eaf_list_push_back(&g_pp_ctx.hook.table, &hook->node);

	return eaf_errno_success;
}

uv_loop_t* eaf_uv_get(void)
{
	return &g_pp_uv_ctx.uv_loop;
}

void eaf_uv_mod(void)
{
	eaf_sem_post(g_pp_ctx.sem_loop);
}
