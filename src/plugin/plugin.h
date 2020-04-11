#ifndef __EAF_PLUGIN_PLUGIN_INTERNAL_H__
#define __EAF_PLUGIN_PLUGIN_INTERNAL_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "EAF/plugin/plugin.h"

/**
* 卸载插件。需要在cleanup阶段之后调用此函数
*/
void eaf_plugin_unload(void);

#ifdef __cplusplus
}
#endif
#endif
