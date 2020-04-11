#ifndef __EAF_PLUGIN_PLUGIN_H__
#define __EAF_PLUGIN_PLUGIN_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "EAF/plugin/detail/msg.h"

/**
* 插件服务ID
*/
#define EAF_PLUGIN_SERVICE		0XF0000000

/**
* 加载插件系统。需要在setup之后、load之前调用此函数
* @param cfg	插件线程配置。仅需要其中的proprity/cpuno/stacksize字段
*/
int eaf_plugin_load(const eaf_thread_table_t* cfg);

#ifdef __cplusplus
}
#endif
#endif
