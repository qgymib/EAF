#ifndef __EAF_UTILS_MAP_H__
#define __EAF_UTILS_MAP_H__
#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

struct eaf_map;
typedef struct eaf_map eaf_map_t;

struct eaf_map_node;
typedef struct eaf_map_node eaf_map_node_t;

/**
* KEY�ԱȺ���
* @param key1	��һ��key
* @param key2	�ڶ���key
* @param arg	�Զ������
* @return		��Key_1С��Key_2ʱ������С��0��ֵ����Key_1����Key_2ʱ������0����Key_1����Key_2ʱ�����ش���0��ֵ
*/
typedef int (*eaf_map_cmp_fn)(const eaf_map_node_t* key1, const eaf_map_node_t* key2, void* arg);

/**
* ������󣬺�����ڵ㡣
* �˽ṹ����Ҫ��Ϊ�ⲿ����ĳ�Ա������
*/
struct eaf_map_node
{
	unsigned long		color;	/** ��ɫ */
	eaf_map_node_t*		parent;	/** ���ڵ� */
	eaf_map_node_t*		left;	/** ���ӽڵ� */
	eaf_map_node_t*		right;	/** ���ӽڵ� */
};

struct eaf_map
{
	eaf_map_node_t		root;	/** ���ڵ� */
	eaf_map_node_t		nil;	/** �սڵ� */

	struct
	{
		eaf_map_cmp_fn	cmp;	/** �ԱȺ��� */
		void*			arg;	/** �Զ������ */
	}cmp;

	size_t				size;	/** ��ǰԪ������ */
};

/**
* ��ʼ�������
* @param handler	�����
* @param cmp		�ԱȺ���
* @param arg		�Զ������
*/
void eaf_map_init(eaf_map_t* handler, eaf_map_cmp_fn cmp, void* arg);

/**
* ����Ԫ��
* @param handler	�����
* @param node		������ڵ�
* @return			0���ɹ���<0��ʧ��
*/
int eaf_map_insert(eaf_map_t* handler, eaf_map_node_t* node);

/**
* ɾ��Ԫ��
* @param handler	�����
* @param node		��ɾ���ڵ�
* @return			0���ɹ���<0��ʧ��
*/
void eaf_map_erase(eaf_map_t* handler, eaf_map_node_t* node);

/**
* ȡ�õ�ǰ��������
* @param handler	�����
* @return			������
*/
size_t eaf_map_size(const eaf_map_t* handler);

/**
* ���ҽڵ�
* @param handler	�����
* @param key		���ҽڵ�
* @return			����������Ϣ����ʵ�ڵ�
*/
eaf_map_node_t* eaf_map_find(const eaf_map_t* handler, const eaf_map_node_t* key);

/**
* ����С�ڵ���key�Ľڵ�
* @param handler	�����
* @param key		����
* @return			lower�ڵ�
*/
eaf_map_node_t* eaf_map_find_lower(const eaf_map_t* handler, const eaf_map_node_t* key);

/**
* ���Ҵ���key�Ľڵ�
* @param handler	�����
* @param key		����
* @return			upper�ڵ�
*/
eaf_map_node_t* eaf_map_find_upper(const eaf_map_t* handler, const eaf_map_node_t* key);

/**
* ȡ�ô������α�
* �α괦�ڿ�ʼλ��
* @param handler	�����
* @return			�α�
*/
eaf_map_node_t* eaf_map_begin(const eaf_map_t* handler);

/**
* ȡ����һ���ڵ�
* @param handler	�����
* @param node		��ǰ�ڵ�
* @return			��һ���ڵ�
*/
eaf_map_node_t* eaf_map_next(const eaf_map_t* handler, const eaf_map_node_t* node);

#ifdef __cplusplus
};
#endif	/* __cplusplus */
#endif
