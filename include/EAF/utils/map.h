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
* KEY对比函数
* @param key1	第一个key
* @param key2	第二个key
* @param arg	自定义参数
* @return		当Key_1小于Key_2时，返回小于0的值；当Key_1等于Key_2时，返回0；当Key_1大于Key_2时，返回大于0的值
*/
typedef int (*eaf_map_cmp_fn)(const eaf_map_node_t* key1, const eaf_map_node_t* key2, void* arg);

/**
* 侵入对象，红黑树节点。
* 此结构体需要作为外部对象的成员变量。
*/
struct eaf_map_node
{
	unsigned long		color;	/** 颜色 */
	eaf_map_node_t*		parent;	/** 父节点 */
	eaf_map_node_t*		left;	/** 左子节点 */
	eaf_map_node_t*		right;	/** 右子节点 */
};

struct eaf_map
{
	eaf_map_node_t		root;	/** 根节点 */
	eaf_map_node_t		nil;	/** 空节点 */

	struct
	{
		eaf_map_cmp_fn	cmp;	/** 对比函数 */
		void*			arg;	/** 自定义参数 */
	}cmp;

	size_t				size;	/** 当前元素容量 */
};

/**
* 初始化红黑树
* @param handler	红黑树
* @param cmp		对比函数
* @param arg		自定义参数
*/
void eaf_map_init(eaf_map_t* handler, eaf_map_cmp_fn cmp, void* arg);

/**
* 插入元素
* @param handler	红黑树
* @param node		待插入节点
* @return			0：成功；<0：失败
*/
int eaf_map_insert(eaf_map_t* handler, eaf_map_node_t* node);

/**
* 删除元素
* @param handler	红黑树
* @param node		待删除节点
* @return			0：成功；<0：失败
*/
void eaf_map_erase(eaf_map_t* handler, eaf_map_node_t* node);

/**
* 取得当前中数据量
* @param handler	红黑树
* @return			数据量
*/
size_t eaf_map_size(const eaf_map_t* handler);

/**
* 查找节点
* @param handler	红黑树
* @param key		查找节点
* @return			包含所需信息的真实节点
*/
eaf_map_node_t* eaf_map_find(const eaf_map_t* handler, const eaf_map_node_t* key);

/**
* 查找小于等于key的节点
* @param handler	红黑树
* @param key		索引
* @return			lower节点
*/
eaf_map_node_t* eaf_map_find_lower(const eaf_map_t* handler, const eaf_map_node_t* key);

/**
* 查找大于key的节点
* @param handler	红黑树
* @param key		索引
* @return			upper节点
*/
eaf_map_node_t* eaf_map_find_upper(const eaf_map_t* handler, const eaf_map_node_t* key);

/**
* 取得此树的游标
* 游标处于开始位置
* @param handler	红黑树
* @return			游标
*/
eaf_map_node_t* eaf_map_begin(const eaf_map_t* handler);

/**
* 取得下一个节点
* @param handler	红黑树
* @param node		当前节点
* @return			下一个节点
*/
eaf_map_node_t* eaf_map_next(const eaf_map_t* handler, const eaf_map_node_t* node);

#ifdef __cplusplus
};
#endif	/* __cplusplus */
#endif
