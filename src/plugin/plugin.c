#include "EAF/plugin/plugin.h"
#include "msg/msg.h"

typedef struct eaf_plugin_loader
{
	int(*init)(void);
	int(*exit)(void);
}eaf_plugin_loader_t;

static eaf_plugin_loader_t _g_plugin_loader[] = {
	{ eaf_plugin_msg_init, eaf_plugin_msg_exit },
};

void eaf_plugin_load(void)
{
	size_t i;
	for (i = 0; i < EAF_ARRAY_SIZE(_g_plugin_loader); i++)
	{
		_g_plugin_loader[i].init();
	}
}

void eaf_plugin_unload(void)
{
	size_t i;
	for (i = EAF_ARRAY_SIZE(_g_plugin_loader); i > 0; i--)
	{
		_g_plugin_loader[i - 1].exit();
	}
}
