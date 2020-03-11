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
* �̻߳ص�
* @param arg		�Զ������
*/
typedef void(*eaf_thread_fn)(void* arg);

/**
* �����߳�
* @param handler	���
* @param attr		����
* @param fn			�̻߳ص�
* @param arg		�Զ������
* @return			eaf_errno
*/
int eaf_thread_init(eaf_thread_t* handler, const eaf_thread_attr_t* attr, eaf_thread_fn fn, void* arg);

/**
* �����߳�
* @param handler	���
*/
void eaf_thread_exit(eaf_thread_t* handler);

/**
* ��ȡ�߳�ID
* @return			�߳�ID
*/
unsigned long eaf_thread_id(void);

/**
* �߳�˯��
* @param timeout	��ʱʱ�䣬����
*/
void eaf_thread_sleep(unsigned timeout);

#ifdef __cplusplus
}
#endif
#endif	/* __EAF_COMPAT_THREAD_INTERNAL_H__ */
