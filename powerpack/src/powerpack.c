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

static uv_ctx_t g_uv_ctx;
static pp_ctx_t g_pp_ctx = {
	NULL, { 0, 0 },
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

	if (pp_sync_dial_once(&g_uv_ctx.init_point, 0) < 0)
	{
		// TODO failure handle
	}
}

static void _pp_hook_on_load_after(int ret)
{
	/* callback */
	FOREACH_VOID_HOOK(on_load_after, ret);
}

static void _pp_notify_exit_thread(void)
{
	g_pp_ctx.mask.looping = 0;

	/* notify libuv loop we need to exit */
	if (uv_is_active((uv_handle_t*)&g_uv_ctx.uv_async_mode))
	{
		uv_async_send(&g_uv_ctx.uv_async_mode);
	}

	pp_sync_dial_once(&g_uv_ctx.exit_point, 0);
}

static void _pp_hook_on_exit_before(void)
{
	/* callback */
	FOREACH_VOID_HOOK(on_exit_before);
}

static void _pp_hook_on_exit_after(void)
{
	FOREACH_VOID_HOOK(on_exit_after);

	_pp_notify_exit_thread();
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
	while (uv_run(&g_uv_ctx.uv_loop, UV_RUN_DEFAULT) != 0)
	{
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

	/* wait for initialize */
	pp_sync_wait(&g_uv_ctx.init_point);

	size_t idx;
	if (_pp_call_init(&idx) < 0)
	{
		goto err_exit;
	}

	/* Answer with initialize success */
	pp_sync_answer(&g_uv_ctx.init_point, 0);

	_pp_loop();

	pp_sync_answer(&g_uv_ctx.exit_point, pp_sync_wait(&g_uv_ctx.exit_point));

	/* we cannot call #_pp_call_exit() here */
	return;

err_exit:
	_pp_call_exit(idx);
	/* Answer with initialize failure */
	pp_sync_answer(&g_uv_ctx.init_point, -1);
	return;
}

static int _pp_init_libuv_ctx(void)
{
	if (pp_sync_init(&g_uv_ctx.init_point) < 0)
	{
		goto err_init_point;
	}

	if (pp_sync_init(&g_uv_ctx.exit_point) < 0)
	{
		goto err_exit_point;
	}

	if (uv_loop_init(&g_uv_ctx.uv_loop) < 0)
	{
		goto err_init_loop;
	}

	if (uv_async_init(&g_uv_ctx.uv_loop, &g_uv_ctx.uv_async_mode, _pp_on_uv_async_mode) < 0)
	{
		goto err_init_async;
	}

	return 0;

err_init_async:
	uv_loop_close(&g_uv_ctx.uv_loop);
err_init_loop:
	pp_sync_exit(&g_uv_ctx.exit_point);
err_exit_point:
	pp_sync_exit(&g_uv_ctx.init_point);
err_init_point:
	return -1;
}

/**
 * @brief Cleanup libuv resource
 *
 * Make sure libuv is not in looping
 */
static void _pp_exit_libuv_ctx(void)
{
	/**
	 * http://docs.libuv.org/en/v1.x/handle.html
	 * A uv_async_t handle is always active and cannot be deactivated, except by closing it with uv_close()
	 */
	if (uv_is_active((uv_handle_t*)&g_uv_ctx.uv_async_mode))
	{
		uv_close((uv_handle_t*)&g_uv_ctx.uv_async_mode, NULL);
		uv_run(&g_uv_ctx.uv_loop, UV_RUN_DEFAULT);	/* active libuv loop to close uv_ctx_t::uv_async_mode */
	}

	uv_loop_close(&g_uv_ctx.uv_loop);
	pp_sync_exit(&g_uv_ctx.init_point);
	pp_sync_exit(&g_uv_ctx.exit_point);
}

static int _pp_init_pp_ctx(const eaf_powerpack_cfg_t* cfg)
{
	/* The initialize table will be removed soon */
	size_t i;
	for (i = 0; i < EAF_ARRAY_SIZE(g_powerpack_table); i++)
	{
		g_powerpack_table[i].on_init();
	}

	g_pp_ctx.working = eaf_thread_create(&cfg->thread_attr, _pp_thread, NULL);
	if (g_pp_ctx.working == NULL)
	{
		goto err_create_thread;
	}

	return 0;

err_create_thread:
	return -1;
}

static void _pp_exit_pp_ctx(void)
{
	/* stop thread */
	_pp_notify_exit_thread();

	eaf_thread_destroy(g_pp_ctx.working);
	g_pp_ctx.working = NULL;

	uv_loop_close(&g_uv_ctx.uv_loop);
}

int eaf_powerpack_init(_In_ const eaf_powerpack_cfg_t* cfg)
{
	int ret = eaf_errno_success;

	if (g_pp_ctx.mask.initialized)
	{
		return eaf_errno_duplicate;
	}

	if ((ret = eaf_inject(&g_pp_ctx.hook.inject, sizeof(g_pp_ctx.hook.inject))) < 0)
	{
		return ret;
	}

	/* Initialize libuv */
	if ((ret = _pp_init_libuv_ctx()) < 0)
	{
		goto err_init_uv;
	}

	if ((ret = _pp_init_pp_ctx(cfg)) < 0)
	{
		goto err_init_pp;
	}

	g_pp_ctx.mask.initialized = 1;
	return eaf_errno_success;

err_init_pp:
	_pp_exit_libuv_ctx();
err_init_uv:
	eaf_uninject(&g_pp_ctx.hook.inject);
	return ret;
}

void eaf_powerpack_exit(void)
{
	if (!g_pp_ctx.mask.initialized)
	{
		return;
	}

	_pp_exit_pp_ctx();

	size_t i;
	for (i = 0; i < EAF_ARRAY_SIZE(g_powerpack_table); i++)
	{
		g_powerpack_table[i].on_exit();
	}

	_pp_exit_libuv_ctx();

	/* clear hook table */
	while (eaf_list_pop_front(&g_pp_ctx.hook.table) != NULL)
	{
	}

	g_pp_ctx.mask.initialized = 0;
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
	uv_async_send(&g_uv_ctx.uv_async_mode);
}

int pp_sync_init(pp_sync_t* handle)
{
	if (uv_sem_init(&handle->sync.sem_req, 0) < 0)
	{
		goto err_init_sem_req;
	}
	if (uv_sem_init(&handle->sync.sem_rsp, 0) < 0)
	{
		goto err_init_sem_rsp;
	}
	if (uv_mutex_init(&handle->sync.objlock) < 0)
	{
		goto err_init_mutex;
	}
	handle->mask.have_req = 0;
	handle->mask.have_rsp = 0;

	return eaf_errno_success;

err_init_mutex:
	uv_sem_destroy(&handle->sync.sem_rsp);
err_init_sem_rsp:
	uv_sem_destroy(&handle->sync.sem_req);
err_init_sem_req:
	return eaf_errno_unknown;
}

void pp_sync_exit(pp_sync_t* handle)
{
	uv_sem_destroy(&handle->sync.sem_req);
	uv_sem_destroy(&handle->sync.sem_rsp);
	uv_mutex_destroy(&handle->sync.objlock);
}

int pp_sync_dial(pp_sync_t* handle, int req)
{
	uv_mutex_lock(&handle->sync.objlock);
	{
		handle->code.req = req;
		handle->mask.have_req = 1;
	}
	uv_mutex_unlock(&handle->sync.objlock);
	uv_sem_post(&handle->sync.sem_req);

	uv_sem_wait(&handle->sync.sem_rsp);
	return handle->code.rsp;
}

int pp_sync_dial_once(pp_sync_t* handle, int req)
{
	int flag_direct_return = 0;
	int rsp = 0;
	uv_mutex_lock(&handle->sync.objlock);
	{
		if (handle->mask.have_rsp)
		{
			flag_direct_return = 1;
			rsp = handle->code.rsp;
		}
	}
	uv_mutex_unlock(&handle->sync.objlock);

	if (flag_direct_return)
	{
		return rsp;
	}

	return pp_sync_dial(handle, req);
}

int pp_sync_wait(pp_sync_t* handle)
{
	uv_sem_wait(&handle->sync.sem_req);
	return handle->code.req;
}

void pp_sync_answer(pp_sync_t* handle, int rsp)
{
	uv_mutex_lock(&handle->sync.objlock);
	{
		handle->code.rsp = rsp;
		handle->mask.have_rsp = 1;
	}
	uv_mutex_unlock(&handle->sync.objlock);

	uv_sem_post(&handle->sync.sem_rsp);
}
