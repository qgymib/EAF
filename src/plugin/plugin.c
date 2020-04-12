#include "EAF/utils/errno.h"
#include "msg/msg.h"
#include "compat/thread.h"
#include "utils/memory.h"
#include "plugin.h"

#if defined(__linux__)
#include <sys/epoll.h>
#include <unistd.h>
#endif

typedef struct eaf_plugin_loader
{
	int(*init)(void);
	void(*exit)(void);
}eaf_plugin_loader_t;

typedef struct eaf_plugin_ctx
{
	eaf_thread_t	driver;			/** 工作线程 */

#if defined(_MSC_VER)
#else
	int				epfd;			/** epoll套接字 */
#endif

	struct
	{
		unsigned	looping : 1;
	}mask;
}eaf_plugin_ctx_t;

static eaf_plugin_loader_t _g_plugin_loader[] = {
	{ eaf_plugin_msg_init, eaf_plugin_msg_exit },
};

static eaf_plugin_ctx_t* g_eaf_plugin_ctx = NULL;

static int _eaf_plugin_on_init(void)
{
	return 0;
}

static void _eaf_plugin_on_exit(void)
{
	// do nothing
}

static void _eaf_plugin_thread(void* arg)
{
	(void)arg;
	while (g_eaf_plugin_ctx->mask.looping)
	{

	}
}

int eaf_plugin_load(const eaf_thread_table_t* cfg)
{
	int ret;
	if (g_eaf_plugin_ctx != NULL)
	{
		return eaf_errno_duplicate;
	}

	if ((g_eaf_plugin_ctx = EAF_MALLOC(sizeof(eaf_plugin_ctx_t))) == NULL)
	{
		return eaf_errno_memory;
	}
	g_eaf_plugin_ctx->mask.looping = 1;

#if defined(_MSC_VER)
#else
	if ((g_eaf_plugin_ctx->epfd = epoll_create(128)) < 0)
	{
		goto err_free;
	}
#endif

	/* 启动线程 */
	eaf_thread_attr_t thread_attr;
	thread_attr.priority = cfg->proprity;
	thread_attr.stack_size = cfg->stacksize;
	thread_attr.cpuno = cfg->cpuno;
	if (eaf_thread_init(&g_eaf_plugin_ctx->driver, &thread_attr, _eaf_plugin_thread, NULL) < 0)
	{
#if defined(_MSC_VER)
#else
		close(g_eaf_plugin_ctx->epfd);
#endif
		goto err_free;
	}

	/* 注册服务 */
	static eaf_service_info_t serivce_info = { 0, NULL, _eaf_plugin_on_init, _eaf_plugin_on_exit };
	if ((ret = eaf_register(EAF_PLUGIN_SERVICE, &serivce_info)) != 0)
	{
		return ret;
	}

	/* 服务初始化 */
	size_t i;
	for (i = 0; i < EAF_ARRAY_SIZE(_g_plugin_loader); i++)
	{
		if (_g_plugin_loader[i].init() < 0)
		{
			goto err;
		}
	}

	return 0;

err_free:
	EAF_FREE(g_eaf_plugin_ctx);
	g_eaf_plugin_ctx = NULL;
	return eaf_errno_unknown;

err:
	eaf_plugin_unload();
	return eaf_errno_unknown;
}

void eaf_plugin_unload(void)
{
	if (g_eaf_plugin_ctx == NULL)
	{
		return;
	}

	/* 停止线程 */
	g_eaf_plugin_ctx->mask.looping = 0;

	size_t i;
	for (i = EAF_ARRAY_SIZE(_g_plugin_loader); i > 0; i--)
	{
		_g_plugin_loader[i - 1].exit();
	}

#if defined(_MSC_VER)
#else
	close(g_eaf_plugin_ctx->epfd);
#endif

	eaf_thread_exit(&g_eaf_plugin_ctx->driver);
	EAF_FREE(g_eaf_plugin_ctx);
	g_eaf_plugin_ctx = NULL;
}
