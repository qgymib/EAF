#ifndef __EAF_ERRNO_H__
#define __EAF_ERRNO_H__
#ifdef __cplusplus
extern "C" {
#endif

typedef enum eaf_errno
{
	eaf_errno_success		=  0x00,	/** �ɹ� */
	eaf_errno_unknown		= -0x01,	/** δ֪���� */
	eaf_errno_duplicate		= -0x02,	/** �ظ����� */
	eaf_errno_memory		= -0x03,	/** �ڴ��쳣 */
	eaf_errno_state			= -0x04,	/** ״̬���� */
	eaf_errno_notfound		= -0x05,	/** ��Դδ�ҵ� */
	eaf_errno_overflow		= -0x06,	/** ��� */
	eaf_errno_timeout		= -0x07,	/** ��ʱ */
}eaf_errno_t;

/**
* ��ȡ�����Զ���������ַ���
* @param err	������
* @return		����
*/
const char* eaf_strerror(eaf_errno_t err);

#ifdef __cplusplus
}
#endif
#endif
