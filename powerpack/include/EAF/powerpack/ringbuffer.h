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
	eaf_ringbuffer_flag_overwrite	= 0x01 << 0x00,		/** 没有空闲空间时，覆盖现有数据 */
	eaf_ringbuffer_flag_discard		= 0x01 << 0x01,		/** 撤销操作 */
	eaf_ringbuffer_flag_abandon		= 0x01 << 0x02,		/** 废弃token */
}eaf_ringbuffer_flag_t;

typedef struct eaf_ringbuffer_counter
{
	size_t				committed;						/** 处于committed状态节点数 */
	size_t				writing;						/** 处于writing状态节点数 */
	size_t				reading;						/** 处于reading状态节点数 */
}eaf_ringbuffer_counter_t;

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4200)
#endif
typedef struct eaf_ringbuffer_token
{
	union
	{
		size_t			size;							/** 数据长度 */
		void*			_pad;							/** 占位符，保证data起始地址与sizeof(void*)对齐 */
	}size;

	uint8_t				data[];							/** 数据体 */
}eaf_ringbuffer_token_t;
#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

/**
* 在给定的内存区块上初始化RingBuffer
* @param buffer		内存区域
* @param size		内存大小
* @return			句柄
*/
eaf_ringbuffer_t* eaf_ringbuffer_init(void* buffer, size_t size);

/**
* 请求一个用于写入的token
* @param handler	句柄
* @param size		待写入数据长度
* @param flags		ring_buffer_flag_t
* @return			token。稍后需要进行commit
*/
eaf_ringbuffer_token_t* eaf_ringbuffer_reserve(eaf_ringbuffer_t* handler, size_t size, int flags);

/**
* 请求一个用于读取的token
* @param handler	句柄
* @return			已经写入好数据的token。在使用完成之后，需要进行commit
*/
eaf_ringbuffer_token_t* eaf_ringbuffer_consume(eaf_ringbuffer_t* handler);

/**
* 提交一个token
*
* 当token处于写入状态时，此操作将导致token确认写入完成，随后此token将能够被consume。
* 如果flag被置为`ring_buffer_flag_discard`，则此token作废。
*
* 当token处于读取状态时，此操作将导致token确认读取完成，随后此token将作废。
* 如果flag被置为`ring_buffer_flag_discard`，则此token被重新标记为可被读取，consume将能够再次获取到此token。
* 
* 注意：当存在多个消费者时，对于读取状态的token进行discard操作不一定能成功（想象存在3个消费者。在时间顺序上发生了如下状况：
* 消费者A获取了token `a`，消费者B获取了token `b`，消费者A回退了token `a`。此时如果消费者C尝试获取token，则此token会是`a`（比token `b`更老），
* 而RingBuffer需要保证后来的消费者一定拿到了较新的数据）。
* 若要进行强制discard，则需要携带`ring_buffer_flag_abandon`。当携带此flag时，此token将被强制作废。
*
* @param handler	句柄
* @param token		待提交的token
* @param flags		ring_buffer_flag_t
* @return			0：成功；<0：失败
*/
int eaf_ringbuffer_commit(eaf_ringbuffer_t* handler, eaf_ringbuffer_token_t* token, int flags);

/**
* 获取计数信息
* @param handler	句柄
* @param counter	计数器
* @return			所有节点数量总和
*/
size_t eaf_ringbuffer_count(eaf_ringbuffer_t* handler, eaf_ringbuffer_counter_t* counter);

/**
* 环形缓冲区头部大小
* @return			头部大小
*/
size_t eaf_ringbuffer_heap_cost(void);

/**
* 计算指定数据所占用的真实空间大小
* @param size		数据大小
* @return			真实空间占用大小
*/
size_t eaf_ringbuffer_node_cost(size_t size);

/**
* 遍历ringbuffer
* @param handler	句柄
* @param cb			用户回调，返回小于0时终止
* @param arg		自定义参数
* @return			用户成功遍历的token数量
*/
size_t eaf_ringbuffer_foreach(eaf_ringbuffer_t* handler,
	int(*cb)(const eaf_ringbuffer_token_t* token, void* arg), void* arg);

#ifdef __cplusplus
}
#endif
#endif
