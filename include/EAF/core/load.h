#ifndef __EAF_CORE_LOAD_H__
#define __EAF_CORE_LOAD_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

typedef struct eaf_service_table
{
	uint32_t				srv_id;			/** ����ID */
	uint32_t				msgq_size;		/** ��Ϣ���д�С */
}eaf_service_table_t;

typedef struct eaf_thread_table
{
	uint8_t					proprity;		/** �߳����ȼ� */
	uint8_t					cpuno;			/** CPU�����׺��� */
	uint16_t				stacksize;		/** �߳�ջ��С����ʵջ��С = stacksize << 4 */

	struct
	{
		size_t				size;			/** ���ñ��С */
		eaf_service_table_t*	table;			/** ���ñ� */
	}service;
}eaf_thread_table_t;

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

#ifdef __cplusplus
}
#endif
#endif
