#ifndef __EAF_CORE_LOAD_H__
#define __EAF_CORE_LOAD_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

typedef struct eaf_service_table
{
	uint32_t				srv_id;			/** 服务ID */
	uint32_t				msgq_size;		/** 消息队列大小 */
}eaf_service_table_t;

typedef struct eaf_thread_table
{
	uint8_t					proprity;		/** 线程优先级 */
	uint8_t					cpuno;			/** CPU核心亲和性 */
	uint16_t				stacksize;		/** 线程栈大小。真实栈大小 = stacksize << 4 */

	struct
	{
		size_t				size;			/** 配置表大小 */
		eaf_service_table_t*	table;			/** 配置表 */
	}service;
}eaf_thread_table_t;

/**
* 配置EAF平台
* @param info	信息列表。必须为全局变量
* @param size	列表长度
* @return		eaf_errno
*/
int eaf_setup(const eaf_thread_table_t* info, size_t size);

/**
* 开启EAF平台
* 函数返回时，所有服务均已初始化完毕
* @return		eaf_errno
*/
int eaf_load(void);

/**
* 清理EAF平台
* @return		eaf_errno
*/
int eaf_cleanup(void);

#ifdef __cplusplus
}
#endif
#endif
