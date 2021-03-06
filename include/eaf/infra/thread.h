/** @file
 * Thread management.
 */
#ifndef __EAF_INFRA_THREAD_H__
#define __EAF_INFRA_THREAD_H__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup EAF-Infra
 * @defgroup EAF-Thread Thread
 * @{
 */

#include "eaf/utils/define.h"

/**
 * @brief Thread instance
 */
typedef struct eaf_thread eaf_thread_t;

#define EAF_THREAD_VALID_PRIORITY		(0x01 << 0x00)	/**< make field `priority` valid */
#define EAF_THREAD_VALID_AFFINITY		(0x01 << 0x01)	/**< make field `affinity` valid */
#define EAF_THREAD_VALID_STACKSIZE		(0x01 << 0x02)	/**< make field `stacksize` valid */

/**
 * @brief Static initializer helper for #eaf_thread_attr_t
 * @see eaf_thread_attr_t
 */
#define EAF_THREAD_ATTR_INITIALIZER		{ 0, { 0, 0, 0 } }

/**
 * @brief Thread attribute
 * @see EAF_THREAD_VALID_PRIORITY
 * @see EAF_THREAD_VALID_AFFINITY
 * @see EAF_THREAD_VALID_STACKSIZE
 */
typedef struct eaf_thread_attr
{
	unsigned long				valid;			/**< mark which field will be valid */

	struct
	{
		unsigned long			priority;		/**< priority, platform related. */
		size_t					affinity;		/**< cpu affinity. only one core can be selected */
		size_t					stacksize;		/**< stack size */
	}field;										/**< attribute field */
}eaf_thread_attr_t;

/**
 * @brief Thread body
 * @param[in,out] arg	User defined argument
 */
typedef void(*eaf_thread_fn)(_Inout_opt_ void* arg);

/**
 * @brief Create thread
 * @param[in] cfg		Thread configure
 * @param[in] fn		Thread body
 * @param[in,out] arg	User defined arg
 * @return				Thread handler
 */
EAF_API eaf_thread_t* eaf_thread_create(_In_opt_ const eaf_thread_attr_t* cfg,
	_In_ eaf_thread_fn fn, _Inout_opt_ void* arg)
	EAF_ATTRIBUTE_ACCESS(read_only, 1)
	EAF_ATTRIBUTE_NONNULL(2)
	EAF_ATTRIBUTE_NOTHROW;

/**
 * @brief Wait for thread exit and destroy it
 * @param[in] handler	Thread handler
 */
EAF_API void eaf_thread_destroy(_Post_invalid_ eaf_thread_t* handler)
	EAF_ATTRIBUTE_NONNULL(1)
	EAF_ATTRIBUTE_NOTHROW;

/**
 * @brief Get current thread id.
 * @return			thread id
 */
EAF_API unsigned long eaf_thread_id(void)
	EAF_ATTRIBUTE_NOTHROW;

/**
 * @brief Causes the calling thread to sleep for msec milliseconds.
 * @param[in] msec	milliseconds
 */
EAF_API void eaf_thread_sleep(unsigned msec)
	EAF_ATTRIBUTE_NOTHROW;

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
#endif
