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
* ��ָ��ջ�е��ú���
* @param addr	�ڴ��ַ
* @param size	�ڴ��С������Ϊsizeof(void*)��������
* @param proc	�û�����
* @param priv	�û�����
*/
#define eaf_stack_call(addr, size, fn, arg)	\
	do {\
		eaf_jmp_buf_t* p_buf = eaf_stack_calculate_jmpbuf(addr, size);\
		if (setjmp(p_buf->env) != 0) {\
			break;\
		}\
		eaf_asm_stackcall(p_buf, fn, arg);\
	} while (0)

/**
* �ö�ִ��Ȩ��ֱ����ʽresume
* @note	��ջЭ�̣�ֻ���ڶ������
*/
#define eaf_yield	\
	do {\
		if (setjmp(eaf_service_get_jmpbuf()->env) != 0) {\
			break;\
		}\
		eaf_filber_context_switch();\
	} while (0)

/**
* ��Э���з���
*/
#define eaf_return	\
		eaf_filber_context_return(); return

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
	uint8_t							proprity;		/** �߳����ȼ� */
	uint8_t							cpuno;			/** CPU�����׺��� */
	uint16_t						stacksize;		/** �߳�ջ��С����ʵջ��С = stacksize << 4 */

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
