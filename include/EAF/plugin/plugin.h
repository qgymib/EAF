#ifndef __EAF_PLUGIN_PLUGIN_H__
#define __EAF_PLUGIN_PLUGIN_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "EAF/plugin/detail/msg.h"

#define EAF_PLUGIN_SERVICE_MSG		0XF0000000

/**
* EAF�����������ز��ϵͳ������Ҫ���ز��������Ҫ��setup֮��load֮ǰ���ô˺���
*/
void eaf_plugin_load(void);

/**
* ж�ز������Ҫ��cleanup�׶�֮����ô˺���
*/
void eaf_plugin_unload(void);

#ifdef __cplusplus
}
#endif
#endif
