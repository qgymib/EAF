#ifndef __EAF_CORE_SERVICE_H__
#define __EAF_CORE_SERVICE_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "EAF/core/internal/service.h"
#include "EAF/core/message.h"

#define eaf_reenter		EAF_FILBER_REENTER()
#define eaf_yield		EAF_FILBER_YIELD(EAF_FILBER_YIELD_TOKEN)

typedef struct eaf_service_msgmap
{
	uint32_t						msg_id;			/** ������ϢID */
	eaf_req_handle_fn				fn;				/** ��Ϣ������ */
}eaf_service_msgmap_t;

typedef struct eaf_service_info
{
	size_t							msg_table_size;	/** ����ӳ����С */
	const eaf_service_msgmap_t*		msg_table;		/** ����ӳ��� */

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
	uint32_t						srv_id;			/** ����ID */
	uint32_t						msgq_size;		/** ��Ϣ���д�С */
}eaf_service_table_t;

typedef struct eaf_thread_table
{
	uint16_t						proprity;		/** �߳����ȼ� */
	uint16_t						cpuno;			/** CPU�����׺��� */
	uint32_t						stacksize;		/** �߳�ջ��С */

	struct
	{
		size_t						size;			/** ���ñ��С */
		eaf_service_table_t*		table;			/** ���ñ� */
	}service;
}eaf_thread_table_t;

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
int eaf_setup(const eaf_thread_table_t* info, size_t size);

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
int eaf_register(uint32_t srv_id, const eaf_service_info_t* info);

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
* @param to	���շ�����ID
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

#ifdef __cplusplus
}
#endif
#endif
