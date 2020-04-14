#ifndef __EAF_CORE_SERVICE_H__
#define __EAF_CORE_SERVICE_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "EAF/core/internal/service.h"
#include "EAF/core/message.h"

/**
* coroutine body
*/
#define eaf_reenter					EAF_COROUTINE_REENTER()

/**
* Suspend service until `eaf_resume` is called
*/
#define eaf_yield					eaf_yield_ext(NULL, NULL)

/**
* Like `eaf_yield`, service will suspend until `eaf_resume` is called,
* and when service was suspended, `fn` will be called immediately.
* `fn` is a function with proto `void(*)(uint32_t service_id, void* arg)`.
*/
#define eaf_yield_ext(fn, arg)		EAF_COROUTINE_YIELD(fn, arg, EAF_COROUTINE_YIELD_TOKEN)

typedef struct eaf_message_table
{
	uint32_t						msg_id;			/** ������ϢID */
	eaf_req_handle_fn				fn;				/** ��Ϣ������ */
}eaf_message_table_t;

typedef struct eaf_service_info
{
	size_t							msg_table_size;	/** ����ӳ����С */
	const eaf_message_table_t*		msg_table;		/** ����ӳ��� */

	/**
	* ��ʼ���ص�
	* @param arg	�Զ������
	* @return		0���ɹ���<0��ʧ��
	*/
	int (*on_init)(void);

	/**
	* ȥ��ʼ���ص�
	* @param arg	�Զ������
	*/
	void (*on_exit)(void);
}eaf_service_info_t;

typedef struct eaf_service_table
{
	uint32_t						srv_id;			/** service id */
	uint32_t						msgq_size;		/** message queue capacity */
}eaf_service_table_t;

typedef struct eaf_thread_attr
{
	unsigned long					priority;		/** priority, platform related. */
	unsigned long					cpuno;			/** cpuno attached. if let system select, set to `-1` */
	size_t							stacksize;		/** stack size */
}eaf_thread_attr_t;

typedef struct eaf_group_table
{
	eaf_thread_attr_t				attr;			/** thread attribute */

	struct
	{
		size_t						size;			/** configure table size */
		eaf_service_table_t*		table;			/** configure table */
	}service;
}eaf_group_table_t;

/**
* ����ָ���������ִ��
* @param srv_id	����ID
* @return		eaf_errno
*/
int eaf_resume(uint32_t srv_id);

/**
* ����EAFƽ̨
* @param info	��Ϣ�б�����Ϊȫ�ֱ���
* @param size	�б���
* @return		eaf_errno
*/
int eaf_setup(const eaf_group_table_t* info /*static*/, size_t size);

/**
* ����EAFƽ̨
* ��������ʱ�����з�����ѳ�ʼ�����
* @return		eaf_errno
*/
int eaf_load(void);

/**
* ����EAFƽ̨
* @return		eaf_errno
*/
int eaf_cleanup(void);

/**
* ע�����
* @param srv_id	����ID
* @param info	������Ϣ������Ϊȫ�ֱ���
* @return		eaf_errno
*/
int eaf_register(uint32_t srv_id, const eaf_service_info_t* info /*static*/);

/**
* �¼�����
* @param srv_id	����ID
* @param evt_id	�¼�ID
* @param fn		������
* @param arg	�Զ������
* @return		eaf_errno
*/
int eaf_subscribe(uint32_t srv_id, uint32_t evt_id, eaf_evt_handle_fn fn, void* arg);

/**
* ȡ���¼�����
* @param srv_id	����ID
* @param evt_id	�¼�ID
* @param fn		������
* @param arg	�Զ������
* @return		eaf_errno
*/
int eaf_unsubscribe(uint32_t srv_id, uint32_t evt_id, eaf_evt_handle_fn fn, void* arg);

/**
* ������������
* @param from	���ͷ�����ID
* @param to		���շ�����ID
* @param req	��������
* @return		eaf_errno
*/
int eaf_send_req(uint32_t from, uint32_t to, eaf_msg_t* req);

/**
* ������Ӧ����
* @param from	���ͷ�����ID
* @param rsp	��Ӧ����
* @return		eaf_errno
*/
int eaf_send_rsp(uint32_t from, eaf_msg_t* rsp);

/**
* ���͹㲥����
* @param from	���ͷ�����ID
* @param evt	�㲥����
* @return		eaf_errno
*/
int eaf_send_evt(uint32_t from, eaf_msg_t* evt);

/**
* Get self's ID
* @return		service id
*/
uint32_t eaf_service_self(void);

#ifdef __cplusplus
}
#endif
#endif
