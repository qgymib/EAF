#ifndef __EAF_COMPAT_THREAD_INTERNAL_H__
#define __EAF_COMPAT_THREAD_INTERNAL_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "s_thread.h"

struct eaf_thread;
typedef struct eaf_thread eaf_thread_t;

typedef struct eaf_thread_attr
{
	unsigned	priority;
	unsigned	stack_size;
	unsigned	cpuno;
}eaf_thread_attr_t;

/**
* 线程回调
* @param arg		自定义参数
*/
typedef void(*eaf_thread_fn)(void* arg);

/**
* 创建线程
* @param handler	句柄
* @param attr		属性
* @param fn			线程回调
* @param arg		自定义参数
* @return			eaf_errno
*/
int eaf_thread_init(eaf_thread_t* handler, const eaf_thread_attr_t* attr, eaf_thread_fn fn, void* arg);

/**
* 销毁线程
* @param handler	句柄
*/
void eaf_thread_exit(eaf_thread_t* handler);

/**
* 获取线程ID
* @return			线程ID
*/
unsigned long eaf_thread_id(void);

/**
* 线程睡眠
* @param timeout	超时时间，毫秒
*/
void eaf_thread_sleep(unsigned timeout);

#ifdef __cplusplus
}
#endif
#endif	/* __EAF_COMPAT_THREAD_INTERNAL_H__ */
