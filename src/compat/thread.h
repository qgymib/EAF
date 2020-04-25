#ifndef __EAF_COMPAT_THREAD_INTERNAL_H__
#define __EAF_COMPAT_THREAD_INTERNAL_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "eaf/infra/thread.h"
#include "eaf/core/service.h"
#include "c_thread.h"

struct eaf_compat_thread;
typedef struct eaf_compat_thread eaf_compat_thread_t;

struct eaf_thread_storage;
typedef struct eaf_thread_storage eaf_thread_storage_t;

/**
* �����߳�
* @param handler	���
* @param cfg		����
* @param fn			�̻߳ص�
* @param arg		�Զ������
* @return			eaf_errno
*/
int eaf_compat_thread_init(eaf_compat_thread_t* handler, const eaf_thread_attr_t* cfg, eaf_thread_fn fn, void* arg);

/**
* �����߳�
* @param handler	���
*/
void eaf_compat_thread_exit(eaf_compat_thread_t* handler);

/**
* ��ʼ���߳�˽�б���
* @param handler	���
* @return			eaf_errno
*/
int eaf_thread_storage_init(eaf_thread_storage_t* handler);

/**
* �����߳�˽�б���
* @param handler	���
*/
void eaf_thread_storage_exit(eaf_thread_storage_t* handler);

/**
* �����߳�˽�б���
* @param handler	���
* @param val		ֵ
* @return			eaf_errno
*/
int eaf_thread_storage_set(eaf_thread_storage_t* handler, void* val);

/**
* ��ȡ�߳�˽�б���
* @param handler	���
* @return			ֵ
*/
void* eaf_thread_storage_get(eaf_thread_storage_t* handler);

/**
* ��ȡ�߳�ID
* @return			�߳�ID
*/
unsigned long eaf_compat_thread_id(void);

/**
* �߳�˯��
* @param timeout	��ʱʱ�䣬����
*/
void eaf_compat_thread_sleep(unsigned timeout);

#ifdef __cplusplus
}
#endif
#endif	/* __EAF_COMPAT_THREAD_INTERNAL_H__ */
