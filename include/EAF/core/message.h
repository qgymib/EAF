#ifndef __EAF_CORE_MESSAGE_H__
#define __EAF_CORE_MESSAGE_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

struct eaf_msg;

typedef enum eaf_msg_type
{
	eaf_msg_type_req,					/** ���� */
	eaf_msg_type_rsp,					/** ��Ӧ */
	eaf_msg_type_evt,					/** �¼� */
}eaf_msg_type_t;

/**
* ������Ϣ������
* @param msg		��Ϣ
*/
typedef void(*eaf_req_handle_fn)(struct eaf_msg* msg);

/**
* ��Ӧ��Ϣ������
* @param msg		��Ϣ
*/
typedef void(*eaf_rsp_handle_fn)(struct eaf_msg* msg);

/**
* �¼�������
* @param msg		�¼�
* @param arg		�Զ������
*/
typedef void(*eaf_evt_handle_fn)(struct eaf_msg* msg, void* arg);

typedef struct eaf_msg
{
	eaf_msg_type_t				type;	/** ��Ϣ���� */
	uint32_t					id;		/** ��ϢID */
	uint32_t					from;	/** �����߷���ID */
	uint32_t					to;		/** �����߷���ID */

	struct
	{
		struct
		{
			eaf_rsp_handle_fn	rfn;	/** response handle function, automatically filled by EAF. user should not modify this field. */
			uintptr_t			orig;	/** original request address, automatically filled by EAF. user should not modify this field. */
			uintptr_t			uid;	/** resource id, not initialized by default. use at your wish. */
		}rr;							/** information for request/response. */
	}info;
}eaf_msg_t;

/**
 * ��������
 * @param msg_id	��ϢID
 * @param size_t	��Ϣ��С
 * @param rsp_fn	������Ӧ��Ϣ����
 * @return			��Ϣ���
 */
eaf_msg_t* eaf_msg_create_req(uint32_t msg_id, size_t size, eaf_rsp_handle_fn rsp_fn);

/**
 * ������Ӧ
 * @param msg_id	��ϢID
 * @param size_t	��Ϣ��С
 * @return			��Ϣ���
 */
eaf_msg_t* eaf_msg_create_rsp(eaf_msg_t* req, size_t size);

/**
 * �����¼�
 * @param msg_id	��ϢID
 * @param size_t	��Ϣ��С
 * @return			��Ϣ���
 */
eaf_msg_t* eaf_msg_create_evt(uint32_t evt_id, size_t size);

/**
 * ��������
 * @param msg		��Ϣ���
 */
void eaf_msg_add_ref(eaf_msg_t* msg);

/**
 * ��������
 * @param msg		��Ϣ���
 */
void eaf_msg_dec_ref(eaf_msg_t* msg);

/**
 * ��ȡ���ݵ�ַ
 * @param msg		��Ϣ���
 * @param size		���ݴ�С
 * @return			���ݵ�ַ
 */
void* eaf_msg_get_data(eaf_msg_t* msg, size_t* size);

#ifdef __cplusplus
}
#endif
#endif  /* __EAF_CORE_MESSAGE_H__ */
