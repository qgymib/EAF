#ifndef __EAF_UTILS_MAP_LOW_H__
#define __EAF_UTILS_MAP_LOW_H__
#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

#include <stddef.h>

#define EAF_MAP_LOW_INIT	((eaf_map_low_t){ NULL })

/**
* 辅助查找工具
* @param ret			返回结果
* @param p_table		eaf_map_low_t*
* @param USER_TYPE		用户类型
* @param user			用户数据
* @param user_vs_orig	对比规则
*/
#define EAF_MAP_LOW_FIND_HELPER(ret, p_table, USER_TYPE, user, user_vs_orig)	\
	do {\
		int flag_success = 0;\
		eaf_map_low_t* __table = p_table;\
		eaf_map_low_node_t* node = __table->rb_root;\
		ret = NULL;\
		while (node) {\
			USER_TYPE* orig = EAF_CONTAINER_OF(node, USER_TYPE, node);\
			int cmp_ret = user_vs_orig;\
			if (cmp_ret < 0) {\
				node = node->rb_left;\
			} else if (cmp_ret > 0) {\
				node = node->rb_right;\
			} else {\
				flag_success = 1;\
				ret = orig;\
				break;\
			}\
		}\
		if (flag_success) {\
			break;\
		}\
	} while (0)

/**
* 辅助插入工具
* @param ret				返回结果，eaf_errno
* @param p_table			eaf_map_low_t*
* @param DATA_TYPE			用户数据类型
* @param data				用户数据地址
* @param orig_vs_data		data与orig的对比结果
*/
#define EAF_MAP_LOW_INSERT_HELPER(ret, p_table, USER_TYPE, user, user_vs_orig)	\
	do {\
		int flag_failed = 0;\
		eaf_map_low_t* __table = p_table;\
		eaf_map_low_node_t **new_node = &(__table->rb_root), *parent = NULL;\
		while (*new_node) {\
			USER_TYPE* orig = EAF_CONTAINER_OF(*new_node, USER_TYPE, node);\
			int cmp_ret = user_vs_orig;\
			parent = *new_node;\
			if (cmp_ret < 0) {\
				new_node = &((*new_node)->rb_left);\
			} else if (cmp_ret > 0) {\
				new_node = &((*new_node)->rb_right);\
			} else {\
				flag_failed = 1;\
				ret = eaf_errno_duplicate;\
				break;\
			}\
		}\
		if (flag_failed) {\
			break;\
		}\
		eaf_map_low_link_node(&(user)->node, parent, new_node);\
		eaf_map_low_insert_color(&(user)->node, __table);\
		ret = eaf_errno_success;\
	} while (0)

typedef struct eaf_map_low_node
{
	unsigned long				__rb_parent_color;	/** 父节点|颜色 */
	struct eaf_map_low_node*	rb_right;			/** 右子节点 */
	struct eaf_map_low_node*	rb_left;			/** 左子节点 */
}eaf_map_low_node_t;

typedef struct eaf_map_low
{
	eaf_map_low_node_t*			rb_root;			/** 根元素节点 */
}eaf_map_low_t;

eaf_map_low_node_t* eaf_map_low_first(const eaf_map_low_t *root);
eaf_map_low_node_t* eaf_map_low_last(const eaf_map_low_t* root);

eaf_map_low_node_t* eaf_map_low_next(const eaf_map_low_node_t* node);
eaf_map_low_node_t* eaf_map_low_prev(const eaf_map_low_node_t* node);

void eaf_map_low_link_node(eaf_map_low_node_t* node, eaf_map_low_node_t* parent, eaf_map_low_node_t** rb_link);
void eaf_map_low_insert_color(eaf_map_low_node_t* node, eaf_map_low_t* root);
void eaf_map_low_erase(eaf_map_low_t* root, eaf_map_low_node_t* node);

#ifdef __cplusplus
};
#endif	/* __cplusplus */
#endif
