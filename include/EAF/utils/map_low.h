#ifndef __EAF_UTILS_MAP_LOW_H__
#define __EAF_UTILS_MAP_LOW_H__
#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

#include <stddef.h>

#define EAF_MAP_LOW_INIT	((eaf_map_low_t){ NULL })

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

eaf_map_low_node_t* rb_first(const eaf_map_low_t *root);
eaf_map_low_node_t* rb_last(const eaf_map_low_t* root);

eaf_map_low_node_t* rb_next(const eaf_map_low_node_t* node);
eaf_map_low_node_t* rb_prev(const eaf_map_low_node_t* node);

void rb_link_node(eaf_map_low_node_t* node, eaf_map_low_node_t* parent, eaf_map_low_node_t** rb_link);
void rb_insert_color(eaf_map_low_node_t* node, eaf_map_low_t* root);
void rb_erase(eaf_map_low_node_t* node, eaf_map_low_t* root);

#ifdef __cplusplus
};
#endif	/* __cplusplus */
#endif
