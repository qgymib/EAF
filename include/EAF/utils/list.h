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
	struct eaf_list_node*	p_after;	/** 下一节点 */
	struct eaf_list_node*	p_before;	/** 上一节点 */
}eaf_list_node_t;

typedef struct eaf_list
{
	eaf_list_node_t*		head;		/** 头结点 */
	eaf_list_node_t*		tail;		/** 尾节点 */
	size_t					size;		/** 节点数量 */
}eaf_list_t;

/**
* 静态list_lite_node初始化工具
*/
#define EAF_LIST_NODE_INITIALIZER	{ NULL, NULL }

/**
* 静态list_lite初始化工具
*/
#define EAF_LIST_INITIALIZER		{ NULL, NULL, 0 }

/**
* 初始化链表
* @param handler	链表
*/
void eaf_list_init(eaf_list_t* handler);

/**
* 将节点添加到链表头
* @param handler	链表
* @param node		目标节点
*/
void eaf_list_push_front(eaf_list_t* handler, eaf_list_node_t* node);

/**
* 将节点添加到链表尾
* @param handler	链表
* @param node		目标节点
*/
void eaf_list_push_back(eaf_list_t* handler, eaf_list_node_t* node);

/**
* 插入到指定节点之前
* @param handler	链表
* @param pos		基准位置
* @param node		待插入节点
*/
void eaf_list_insert_before(eaf_list_t* handler, eaf_list_node_t* pos, eaf_list_node_t* node);

/**
* 插入到指定节点之后
* @param handler	链表
* @param pos		基准位置
* @param node		待插入节点
*/
void eaf_list_insert_after(eaf_list_t* handler, eaf_list_node_t* pos, eaf_list_node_t* node);

/**
* 删除节点
* @param handler	链表
* @param node		待删除节点
*/
void eaf_list_erase(eaf_list_t* handler, eaf_list_node_t* node);

/**
* 获取节点数量
* @return			节点数量
*/
size_t eaf_list_size(const eaf_list_t* handler);

/**
* 移除首节点
* @param handler	链表
* @return			首节点
*/
eaf_list_node_t* eaf_list_pop_front(eaf_list_t* handler);

/**
* 移除尾节点
* @param handler	链表
* @return			首节点
*/
eaf_list_node_t* eaf_list_pop_back(eaf_list_t* handler);

/**
* 获取首节点
* @param handler	链表
* @return			首节点
*/
eaf_list_node_t* eaf_list_begin(const eaf_list_t* handler);

/**
 * Get the last node
 * @param handler	the handler of list
 * @return			the last node
 */
eaf_list_node_t* eaf_list_end(const eaf_list_t* handler);

/**
* 获取下一个节点
* @param handler	链表
* @param node		当前节点
* @return			下一个节点
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
