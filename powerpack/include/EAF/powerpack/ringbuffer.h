#ifndef __EAF_POWERPACK_RINGBUFFER_H__
#define __EAF_POWERPACK_RINGBUFFER_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

struct eaf_ringbuffer;
typedef struct eaf_ringbuffer eaf_ringbuffer_t;

typedef enum eaf_ringbuffer_flag
{
	eaf_ringbuffer_flag_overwrite	= 0x01 << 0x00,		/** û�п��пռ�ʱ�������������� */
	eaf_ringbuffer_flag_discard		= 0x01 << 0x01,		/** �������� */
	eaf_ringbuffer_flag_abandon		= 0x01 << 0x02,		/** ����token */
}eaf_ringbuffer_flag_t;

typedef struct eaf_ringbuffer_counter
{
	size_t				committed;						/** ����committed״̬�ڵ��� */
	size_t				writing;						/** ����writing״̬�ڵ��� */
	size_t				reading;						/** ����reading״̬�ڵ��� */
}eaf_ringbuffer_counter_t;

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4200)
#endif
typedef struct eaf_ringbuffer_token
{
	union
	{
		size_t			size;							/** ���ݳ��� */
		void*			_pad;							/** ռλ������֤data��ʼ��ַ��sizeof(void*)���� */
	}size;

	uint8_t				data[];							/** ������ */
}eaf_ringbuffer_token_t;
#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

/**
* �ڸ������ڴ������ϳ�ʼ��RingBuffer
* @param buffer		�ڴ�����
* @param size		�ڴ��С
* @return			���
*/
eaf_ringbuffer_t* eaf_ringbuffer_init(void* buffer, size_t size);

/**
* ����һ������д���token
* @param handler	���
* @param size		��д�����ݳ���
* @param flags		ring_buffer_flag_t
* @return			token���Ժ���Ҫ����commit
*/
eaf_ringbuffer_token_t* eaf_ringbuffer_reserve(eaf_ringbuffer_t* handler, size_t size, int flags);

/**
* ����һ�����ڶ�ȡ��token
* @param handler	���
* @return			�Ѿ�д������ݵ�token����ʹ�����֮����Ҫ����commit
*/
eaf_ringbuffer_token_t* eaf_ringbuffer_consume(eaf_ringbuffer_t* handler);

/**
* �ύһ��token
*
* ��token����д��״̬ʱ���˲���������tokenȷ��д����ɣ�����token���ܹ���consume��
* ���flag����Ϊ`ring_buffer_flag_discard`�����token���ϡ�
*
* ��token���ڶ�ȡ״̬ʱ���˲���������tokenȷ�϶�ȡ��ɣ�����token�����ϡ�
* ���flag����Ϊ`ring_buffer_flag_discard`�����token�����±��Ϊ�ɱ���ȡ��consume���ܹ��ٴλ�ȡ����token��
* 
* ע�⣺�����ڶ��������ʱ�����ڶ�ȡ״̬��token����discard������һ���ܳɹ����������3�������ߡ���ʱ��˳���Ϸ���������״����
* ������A��ȡ��token `a`��������B��ȡ��token `b`��������A������token `a`����ʱ���������C���Ի�ȡtoken�����token����`a`����token `b`���ϣ���
* ��RingBuffer��Ҫ��֤������������һ���õ��˽��µ����ݣ���
* ��Ҫ����ǿ��discard������ҪЯ��`ring_buffer_flag_abandon`����Я����flagʱ����token����ǿ�����ϡ�
*
* @param handler	���
* @param token		���ύ��token
* @param flags		ring_buffer_flag_t
* @return			0���ɹ���<0��ʧ��
*/
int eaf_ringbuffer_commit(eaf_ringbuffer_t* handler, eaf_ringbuffer_token_t* token, int flags);

/**
* ��ȡ������Ϣ
* @param handler	���
* @param counter	������
* @return			���нڵ������ܺ�
*/
size_t eaf_ringbuffer_count(eaf_ringbuffer_t* handler, eaf_ringbuffer_counter_t* counter);

/**
* ���λ�����ͷ����С
* @return			ͷ����С
*/
size_t eaf_ringbuffer_heap_cost(void);

/**
* ����ָ��������ռ�õ���ʵ�ռ��С
* @param size		���ݴ�С
* @return			��ʵ�ռ�ռ�ô�С
*/
size_t eaf_ringbuffer_node_cost(size_t size);

/**
* ����ringbuffer
* @param handler	���
* @param cb			�û��ص�������С��0ʱ��ֹ
* @param arg		�Զ������
* @return			�û��ɹ�������token����
*/
size_t eaf_ringbuffer_foreach(eaf_ringbuffer_t* handler,
	int(*cb)(const eaf_ringbuffer_token_t* token, void* arg), void* arg);

#ifdef __cplusplus
}
#endif
#endif
