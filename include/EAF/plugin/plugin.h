#ifndef __EAF_PLUGIN_PLUGIN_H__
#define __EAF_PLUGIN_PLUGIN_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "EAF/plugin/detail/msg.h"

#define EAF_PLUGIN_SERVICE_MSG		0XF0000000

void eaf_plugin_load(void);
void eaf_plugin_unload(void);

#ifdef __cplusplus
}
#endif
#endif
