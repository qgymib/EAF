#include <stdlib.h>
#include <string.h>
#include "eaf/eaf.h"
#include "powerpack/message.h"
#include "powerpack/net.h"
#include "powerpack/timer.h"
#include "powerpack.h"

#define MODULE	"powerpack"
#define LOG_TRACE(fmt, ...)	EAF_LOG_TRACE(MODULE, fmt, ##__VA_ARGS__)

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
		unsigned	have_thread : 1;
	}mask;

	struct
	{
		eaf_list_t	table;
		eaf_hook_t	inject;
	}hook;
}pp_ctx_t;

typedef struct uv_ctx
{
	struct
	{
		unsigned	init_failed : 1;
	}mask;

	uv_sem_t		init_wait_point;
	uv_loop_t		uv_loop;			/**< libuv loop */
	uv_async_t		uv_async_mode;		/**< libuv async notifier */
}uv_ctx_t;

static uv_ctx_t g_uv_ctx;
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
	/* callback */
	FOREACH_VOID_HOOK(on_load_after, ret);

	/* start libuv loop */
	eaf_sem_post(g_pp_ctx.sem_loop);
}

static void _pp_notify_exit_thread(void)
{
	if (!g_pp_ctx.mask.have_thread)
	{
		return;
	}

	g_pp_ctx.mask.looping = 0;
	eaf_sem_post(g_pp_ctx.sem_loop);

	/* notify libuv loop we need to exit */
	uv_async_send(&g_uv_ctx.uv_async_mode);
}

static void _pp_hook_on_exit_before(void)
{
	_pp_notify_exit_thread();

	/* callback */
	FOREACH_VOID_HOOK(on_exit_before);
}

static void _pp_hook_on_exit_after(void)
{
	FOREACH_VOID_HOOK(on_exit_after);
}

static int _pp_call_init(size_t* idx)
{
	eaf_list_node_t* it = eaf_list_begin(&g_pp_ctx.hook.table);
	for (*idx = 0; it != NULL; it = eaf_list_next(it), *idx += 1)
	{
		eaf_powerpack_hook_t* record = EAF_CONTAINER_OF(it, eaf_powerpack_hook_t, node);
		if (record->on_loop_init == NULL)
		{
			continue;
		}
		if (record->on_loop_init() < 0)
		{
			return -1;
		}
	}
	return 0;
}

static void _pp_call_exit(size_t idx)
{
	size_t i;
	eaf_list_node_t* it = eaf_list_begin(&g_pp_ctx.hook.table);
	for (i = 0; it != NULL && i < idx; it = eaf_list_next(it), i++)
	{
		eaf_powerpack_hook_t* record = EAF_CONTAINER_OF(it, eaf_powerpack_hook_t, node);
		if (record->on_loop_exit == NULL)
		{
			continue;
		}
		record->on_loop_exit();
	}
}

static void _pp_loop(void)
{
	while (g_pp_ctx.mask.looping)
	{
		while (uv_run(&g_uv_ctx.uv_loop, UV_RUN_DEFAULT) != 0)
		{
		}
	}
}

static void _pp_on_uv_async_mode(uv_async_t* handle)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(handle);

	if (!g_pp_ctx.mask.looping)
	{
		_pp_call_exit((size_t)-1);
		uv_close((uv_handle_t*)&g_uv_ctx.uv_async_mode, NULL);
		return;
	}
}

static void _pp_thread(void* arg)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(arg);

	g_pp_ctx.mask.have_thread = 1;

	/* initialize libuv */
	if (uv_loop_init(&g_uv_ctx.uv_loop) < 0)
	{
		goto err_init_uv_loop;
	}

	if (uv_async_init(&g_uv_ctx.uv_loop, &g_uv_ctx.uv_async_mode, _pp_on_uv_async_mode) < 0)
	{
		goto err_init_async;
	}

	uv_sem_post(&g_uv_ctx.init_wait_point);

	/* wait for initialize */
	while (g_pp_ctx.mask.looping)
	{
		if (eaf_sem_pend(g_pp_ctx.sem_loop, EAF_SEM_INFINITY) == 0)
		{
			break;
		}
	}

	if (!g_pp_ctx.mask.looping)
	{
		return;
	}

	size_t idx;
	if (_pp_call_init(&idx) < 0)
	{
		goto err_exit;
	}

	_pp_loop();

	/* we cannot call #_pp_call_exit() here */

	/* close loop */
	uv_loop_close(&g_uv_ctx.uv_loop);
	g_pp_ctx.mask.have_thread = 0;

	return;

err_exit:
	_pp_call_exit(idx);

err_init_async:
	uv_loop_close(&g_uv_ctx.uv_loop);
err_init_uv_loop:
	g_pp_ctx.mask.have_thread = 0;
	g_uv_ctx.mask.init_failed = 1;
	uv_sem_post(&g_uv_ctx.init_wait_point);
	return;
}

int eaf_powerpack_init(_In_ const eaf_powerpack_cfg_t* cfg)
{
	int ret = eaf_errno_success;

	if (g_pp_ctx.sem_loop != NULL)
	{
		return eaf_errno_duplicate;
	}
	memset(&g_uv_ctx, 0, sizeof(g_uv_ctx));

	if ((ret = eaf_inject(&g_pp_ctx.hook.inject, sizeof(g_pp_ctx.hook.inject))) < 0)
	{
		return ret;
	}

	if (uv_sem_init(&g_uv_ctx.init_wait_point, 0) < 0)
	{
		goto err_sem_create_1;
	}

	g_pp_ctx.mask.looping = 1;
	if ((g_pp_ctx.sem_loop = eaf_sem_create(0)) == NULL)
	{
		goto err_sem_create;
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
	g_pp_ctx.working = eaf_thread_create(&cfg->thread_attr, _pp_thread, NULL);
	if (g_pp_ctx.working == NULL)
	{
		goto err_init;
	}

	uv_sem_wait(&g_uv_ctx.init_wait_point);
	if (g_uv_ctx.mask.init_failed)
	{
		goto err_uv_init;
	}

	return eaf_errno_success;

err_uv_init:
	eaf_thread_destroy(g_pp_ctx.working);
	g_pp_ctx.working = NULL;
err_init:
	for (i = 0; i < EAF_ARRAY_SIZE(g_powerpack_table); i++)
	{
		g_powerpack_table[i].on_exit();
	}
	eaf_sem_destroy(g_pp_ctx.sem_loop);
	g_pp_ctx.sem_loop = NULL;
err_sem_create:
	uv_sem_destroy(&g_uv_ctx.init_wait_point);
err_sem_create_1:
	eaf_uninject(&g_pp_ctx.hook.inject);
	g_pp_ctx.mask.looping = 0;
	return ret;
}

void eaf_powerpack_exit(void)
{
	if (g_pp_ctx.sem_loop == NULL)
	{
		return;
	}

	/* stop thread */
	_pp_notify_exit_thread();

	eaf_thread_destroy(g_pp_ctx.working);
	g_pp_ctx.working = NULL;

	size_t i;
	for (i = 0; i < EAF_ARRAY_SIZE(g_powerpack_table); i++)
	{
		g_powerpack_table[i].on_exit();
	}

	uv_loop_close(&g_uv_ctx.uv_loop);
	eaf_sem_destroy(g_pp_ctx.sem_loop);
	g_pp_ctx.sem_loop = NULL;

	uv_sem_destroy(&g_uv_ctx.init_wait_point);

	/* clear hook table */
	while (eaf_list_pop_front(&g_pp_ctx.hook.table) != NULL)
	{
	}
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

void eaf_powerpack_hook_unregister(eaf_powerpack_hook_t* hook)
{
	eaf_list_node_t* it = eaf_list_begin(&g_pp_ctx.hook.table);
	for (; it != NULL; it = eaf_list_next(it))
	{
		eaf_powerpack_hook_t* record = EAF_CONTAINER_OF(it, eaf_powerpack_hook_t, node);
		if (record == hook)
		{
			eaf_list_erase(&g_pp_ctx.hook.table, it);
			break;
		}
	}
}

uv_loop_t* eaf_uv_get(void)
{
	return &g_uv_ctx.uv_loop;
}

void eaf_uv_mod(void)
{
	eaf_sem_post(g_pp_ctx.sem_loop);
}
