/** @file
 * A multi-producers multi-consumers ring buffer.
 */
#ifndef __EAF_POWERPACK_RINGBUFFER_H__
#define __EAF_POWERPACK_RINGBUFFER_H__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup PowerPack
 * @defgroup PowerPack-RingBuffer RingBuffer
 * @{
 */

#include <stddef.h>
#include <stdint.h>
#include "eaf/eaf.h"

/**
 * @brief Ring buffer
 */
typedef struct eaf_ringbuffer eaf_ringbuffer_t;

/**
 * @brief Ring buffer flags
 * @see eaf_ringbuffer_reserve
 * @see eaf_ringbuffer_commit
 */
typedef enum eaf_ringbuffer_flag
{
	eaf_ringbuffer_flag_overwrite	= 0x01 << 0x00,		/**< Overwrite data if no enought free space */
	eaf_ringbuffer_flag_discard		= 0x01 << 0x01,		/**< Discard operation */
	eaf_ringbuffer_flag_abandon		= 0x01 << 0x02,		/**< Abandon token */
}eaf_ringbuffer_flag_t;

/**
 * @brief Ring buffer counter
 */
typedef struct eaf_ringbuffer_counter
{
	size_t				committed;						/**< The number of committed nodes */
	size_t				writing;						/**< The number of writing nodes */
	size_t				reading;						/**< The number of reading nodes */
}eaf_ringbuffer_counter_t;

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4200)
#endif
/**
 * @brief Ring buffer data token
 */
typedef struct eaf_ringbuffer_token
{
	union
	{
		size_t			size;							/**< Data length */
		void*			_pad;							/**< Padding field used for make sure address align */
	}size;												/**< Data size */

	uint8_t				data[];							/**< Data body */
}eaf_ringbuffer_token_t;
#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

/**
 * @brief Initialize ring buffer on the give memory.
 * @param[in,out] buffer	Memory area
 * @param[in] size			The size of memory area
 * @return					Initialized ring buffer
 */
eaf_ringbuffer_t* eaf_ringbuffer_init(_Inout_ void* buffer, _In_ size_t size);

/**
 * @brief Acquire a token for write.
 * @see eaf_ringbuffer_commit
 * @param[in,out] handler	The pointer to the ring buffer
 * @param[in] size			The size of aera you want
 * @param[in] flags			#eaf_ringbuffer_flag
 * @return					A token for write. This token must be commited by #eaf_ringbuffer_commit
 */
eaf_ringbuffer_token_t* eaf_ringbuffer_reserve(_Inout_ eaf_ringbuffer_t* handler,
	_In_ size_t size, _In_ int flags);

/**
 * @brief Acquire a token for read.
 * @see eaf_ringbuffer_commit
 * @param[in,out] handler	The pointer to the ring buffer
 * @return					A token for read. This token must be commited by #eaf_ringbuffer_commit
 */
eaf_ringbuffer_token_t* eaf_ringbuffer_consume(_Inout_ eaf_ringbuffer_t* handler);

/**
 * @brief Commit a token.
 *
 * If the token was created by #eaf_ringbuffer_reserve, then this token can be
 * consumed by #eaf_ringbuffer_consume. If flag contains
 * #eaf_ringbuffer_flag_discard, then this token will be
 * destroyed.
 *
 * If the token was created by #eaf_ringbuffer_consume, then this token will be
 * destroyed. If flag contains
 * #eaf_ringbuffer_flag_discard, then this token will be
 * marked as readable, and #eaf_ringbuffer_consume will be able to get this
 * token.
 *
 * If there are two or more consumers, discard a reading token may be failed.
 * Consider the following condition:
 * 1. `CONSUMER_A` acquire a reading token `READ_A` (success)
 * 2. `CONSUMER_B` acquire a reading token `READ_B` (success)
 * 3. `CONSUMER_A` discard `READ_A` (failure)
 * > This happens because ring buffer must guarantee data order is FIFO. If
 * > `CONSUMER_A` is able to discard `READ_A`, then next consumer will get
 * > `READ_A` which is older than `READ_B`. This condition must not happen.
 *
 * @param[in,out] handler	The pointer to the ring buffer
 * @param[in,out] token		The token going to be committed
 * @param[in] flags			#eaf_ringbuffer_flag
 * @return					0 if success, otherwise failure
 */
int eaf_ringbuffer_commit(_Inout_ eaf_ringbuffer_t* handler,
	_Inout_ eaf_ringbuffer_token_t* token, _In_ int flags);

/**
 * @brief Get counter
 * @param[in] handler	The ring buffer
 * @param[out] counter	The pointer to counter
 * @return				The sum of all counter
 */
size_t eaf_ringbuffer_count(_In_ eaf_ringbuffer_t* handler,
	_Out_opt_ eaf_ringbuffer_counter_t* counter);

/**
 * @brief Get how much bytes the ring buffer structure cost
 * @return			size of eaf_ringbuffer
 */
size_t eaf_ringbuffer_heap_cost(void);

/**
 * @brief Calculate the how much space of given size data will take place.
 * @param[in] size		The size of data
 * @return				The space of data will take place
 */
size_t eaf_ringbuffer_node_cost(_In_ size_t size);

/**
 * @brief Traverse ring buffer
 * @param[in] handler	The ring buffer
 * @param[in] cb		User callback
 * @param[in,out] arg	User defined argument
 * @return				The amount of successful callback
 */
size_t eaf_ringbuffer_foreach(_In_ eaf_ringbuffer_t* handler,
	_In_ int(*cb)(const eaf_ringbuffer_token_t* token, void* arg), _Inout_opt_ void* arg);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
#endif
