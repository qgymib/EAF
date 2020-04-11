#ifndef __EAF_PLUGIN_PLUGIN_H__
#define __EAF_PLUGIN_PLUGIN_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "EAF/plugin/detail/msg.h"

#define EAF_PLUGIN_SERVICE_MSG		0XF0000000

/**
* EAF不会主动加载插件系统。若想要加载插件，则需要在setup之后、load之前调用此函数
*/
void eaf_plugin_load(void);

/**
* 卸载插件。需要在cleanup阶段之后调用此函数
*/
void eaf_plugin_unload(void);

#ifdef __cplusplus
}
#endif
#endif
