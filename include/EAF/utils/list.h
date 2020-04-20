/** @file
 * |--------|--------|--------|--------|--------|--------|--------|
 *   HEAD ------------------------------------------------> TAIL
 *     front -------------------------------------------> back
 *       before -------------------------------------> after
 */
#ifndef __EAF_UTILS_LIST_H__
#define __EAF_UTILS_LIST_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef struct eaf_list_node
{
	struct eaf_list_node*	p_after;	/** ��һ�ڵ� */
	struct eaf_list_node*	p_before;	/** ��һ�ڵ� */
}eaf_list_node_t;

typedef struct eaf_list
{
	eaf_list_node_t*		head;		/** ͷ��� */
	eaf_list_node_t*		tail;		/** β�ڵ� */
	size_t					size;		/** �ڵ����� */
}eaf_list_t;

/**
* ��̬list_lite_node��ʼ������
*/
#define EAF_LIST_NODE_INITIALIZER	{ NULL, NULL }

/**
* ��̬list_lite��ʼ������
*/
#define EAF_LIST_INITIALIZER		{ NULL, NULL, 0 }

/**
* ��ʼ������
* @param handler	����
*/
void eaf_list_init(eaf_list_t* handler);

/**
* ���ڵ���ӵ�����ͷ
* @param handler	����
* @param node		Ŀ��ڵ�
*/
void eaf_list_push_front(eaf_list_t* handler, eaf_list_node_t* node);

/**
* ���ڵ���ӵ�����β
* @param handler	����
* @param node		Ŀ��ڵ�
*/
void eaf_list_push_back(eaf_list_t* handler, eaf_list_node_t* node);

/**
* ���뵽ָ���ڵ�֮ǰ
* @param handler	����
* @param pos		��׼λ��
* @param node		������ڵ�
*/
void eaf_list_insert_before(eaf_list_t* handler, eaf_list_node_t* pos, eaf_list_node_t* node);

/**
* ���뵽ָ���ڵ�֮��
* @param handler	����
* @param pos		��׼λ��
* @param node		������ڵ�
*/
void eaf_list_insert_after(eaf_list_t* handler, eaf_list_node_t* pos, eaf_list_node_t* node);

/**
* ɾ���ڵ�
* @param handler	����
* @param node		��ɾ���ڵ�
*/
void eaf_list_erase(eaf_list_t* handler, eaf_list_node_t* node);

/**
* ��ȡ�ڵ�����
* @return			�ڵ�����
*/
size_t eaf_list_size(const eaf_list_t* handler);

/**
* �Ƴ��׽ڵ�
* @param handler	����
* @return			�׽ڵ�
*/
eaf_list_node_t* eaf_list_pop_front(eaf_list_t* handler);

/**
* �Ƴ�β�ڵ�
* @param handler	����
* @return			�׽ڵ�
*/
eaf_list_node_t* eaf_list_pop_back(eaf_list_t* handler);

/**
* ��ȡ�׽ڵ�
* @param handler	����
* @return			�׽ڵ�
*/
eaf_list_node_t* eaf_list_begin(const eaf_list_t* handler);

/**
 * Get the last node
 * @param handler	the handler of list
 * @return			the last node
 */
eaf_list_node_t* eaf_list_end(const eaf_list_t* handler);

/**
* ��ȡ��һ���ڵ�
* @param handler	����
* @param node		��ǰ�ڵ�
* @return			��һ���ڵ�
*/
eaf_list_node_t* eaf_list_next(const eaf_list_t* handler, const eaf_list_node_t* node);

/**
 * Get previous node
 * @param handler	the handler of list
 * @param node		current node
 * @return			previous node
 */
eaf_list_node_t* eaf_list_prev(const eaf_list_t* handler, const eaf_list_node_t* node);

#ifdef __cplusplus
}
#endif
#endif	/* __EAF_UTILS_LIST_INTERNAL_H__ */
