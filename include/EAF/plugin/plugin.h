#ifndef __EAF_PLUGIN_PLUGIN_H__
#define __EAF_PLUGIN_PLUGIN_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "EAF/plugin/detail/msg.h"

/**
* �������ID
*/
#define EAF_PLUGIN_SERVICE		0XF0000000

/**
* ���ز��ϵͳ����Ҫ��setup֮��load֮ǰ���ô˺���
* @param cfg	����߳�����
* @return		eaf_errno
*/
int eaf_plugin_load(const eaf_thread_attr_t* cfg);

#ifdef __cplusplus
}
#endif
#endif
