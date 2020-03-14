#ifndef __EAF_FILBER_INTERNAL_SERVICE_H__
#define __EAF_FILBER_INTERNAL_SERVICE_H__
#ifdef __cplusplus
extern "C" {
#endif

struct eaf_jmpbuf;

/**
* 获取跳转上下文
* @return		上下文
*/
struct eaf_jmpbuf* eaf_service_get_jmpbuf(void);

/**
* 进行上下文切换
*/
void eaf_service_context_switch(void);

#ifdef __cplusplus
}
#endif
#endif
