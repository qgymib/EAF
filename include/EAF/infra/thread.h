#ifndef __EAF_INFRA_THREAD_H__
#define __EAF_INFRA_THREAD_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

struct eaf_thread;
typedef struct eaf_thread eaf_thread_t;

#define EAF_THREAD_VALID_PRIORITY		(0x01 << 0x00)
#define EAF_THREAD_VALID_AFFINITY		(0x01 << 0x01)
#define EAF_THREAD_VALID_STACKSIZE		(0x01 << 0x02)

typedef struct eaf_thread_attr
{
	unsigned long				valid;			/** mark which field will be valid */

	struct
	{
		unsigned long			priority;		/** priority, platform related. */
		size_t					affinity;		/** cpu affinity. only one core can be selected */
		size_t					stacksize;		/** stack size */
	}field;
}eaf_thread_attr_t;

/**
* 线程回调
* @param arg		自定义参数
*/
typedef void(*eaf_thread_fn)(void* arg);

/**
* Create thread
* @param cfg		thread configure
* @param fn			thread body
* @param arg		user defined arg
* @return			thread handler
*/
eaf_thread_t* eaf_thread_create(const eaf_thread_attr_t* cfg, eaf_thread_fn fn, void* arg);

/**
* Wait for thread exit and destroy it
* @param handler	thread handler
*/
void eaf_thread_destroy(eaf_thread_t* handler);

/**
* Get current thread id.
* @return			thread id
*/
unsigned long eaf_thread_id(void);

#ifdef __cplusplus
}
#endif
#endif
