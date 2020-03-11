#ifndef __EAF_UTILS_DEFINE_H__
#define __EAF_UTILS_DEFINE_H__
#ifdef __cplusplus
extern "C" {
#endif

/**
* ���ݽṹ���Ա������ַ�ҵ��ṹ���׵�ַ
* @param ptr	��ַ
* @param TYPE	�ṹ������
* @param member	��ַ��Ӧ��Ա
* @return		�ṹ���ַ
*/
#define EAF_CONTAINER_OF(ptr, TYPE, member)	\
	((TYPE*)((char*)(ptr) - (size_t)&((TYPE*)0)->member))

/**
* ǿ�Ʒ�������
* @param TYPE	��������
* @param x		����
* @return		����ֵ
*/
#define EAF_ACCESS(TYPE, x)		(*(volatile TYPE*)&(x))

/**
* ��ȡ���鳤��
* @param arr	����
* @return		����
*/
#define EAF_ARRAY_SIZE(arr)		(sizeof(arr) / sizeof(arr[0]))

#ifdef __cplusplus
}
#endif
#endif
