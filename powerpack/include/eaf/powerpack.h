/** @file
 * Include this file to use all powerpack feature.
 */
#ifndef __EAF_POWERPACK_H__
#define __EAF_POWERPACK_H__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup PowerPack PowerPack
 * Some of PowerPack's modules are actually services, they do have Service ID.
 * These Service IDs are listed here:
 *
 * Module          | Service ID
 * --------------- | ----------
 * EAF_TIMER_ID    | 0x00010000
 * EAF_WATCHDOG_ID | 0x00020000
 * EAF_MESSAGE_ID  | 0x00030000
 * EAF_MONITOR_ID  | 0x00040000
 *
 * @{
 */
#include "eaf/powerpack/define.h"
#include "eaf/powerpack/hash.h"
#include "eaf/powerpack/log.h"
#include "eaf/powerpack/message.h"
#include "eaf/powerpack/monitor.h"
#include "eaf/powerpack/net.h"
#include "eaf/powerpack/random.h"
#include "eaf/powerpack/ringbuffer.h"
#include "eaf/powerpack/string.h"
#include "eaf/powerpack/time.h"
#include "eaf/powerpack/timer.h"
#include "eaf/powerpack/watchdog.h"
/**
 * @}
 */

/**
 * @brief A default static initializer for #eaf_powerpack_hook_t
 */
#define EAF_POWERPACK_HOOK_INITIALIZER	{ EAF_LIST_NODE_INITIALIZER, EAF_HOOK_INITIALIZER, NULL, NULL }

/**
 * @brief PowerPack hook
 */
typedef struct eaf_powerpack_hook
{
	eaf_list_node_t		node;
	eaf_hook_t			hook;

	/**
	 * @brief Initialize function, called in loop thread
	 *
	 * PowerPack guarantee this callback is called before every service is going
	 * to initialize.
	 *
	 * @return			Non-zero if initialize failure, zero if success.
	 */
	int (*on_loop_init)(void);

	/**
	 * @brief Exit function, called in loop thread
	 *
	 * PowerPack guarantee is callback is called after every service is done exit.
	 */
	void(*on_loop_exit)(void);
}eaf_powerpack_hook_t;

/**
 * @brief PowerPack configuration
 */
typedef struct eaf_powerpack_cfg
{
	eaf_thread_attr_t	thread_attr;		/**< thread configure for unistd */
}eaf_powerpack_cfg_t;

/**
 * @brief Setup PowerPack
 * @param[in] cfg	Configuration
 * @return			#eaf_errno
 */
int eaf_powerpack_init(_In_ const eaf_powerpack_cfg_t* cfg);

/**
 * @brief Must be called after #eaf_exit()
 */
void eaf_powerpack_exit(void);

/**
 * @brief Register global hook.
 * @param[in] hook	Hook
 * @param[in] size	sizeof(*hook)
 * @return			#eaf_errno
 */
int eaf_powerpack_hook_register(eaf_powerpack_hook_t* hook, size_t size);

/**
 * @brief Unregister global hook.
 * @param[in] hook	A registered hook
 */
void eaf_powerpack_hook_unregister(eaf_powerpack_hook_t* hook);

#ifdef __cplusplus
}
#endif
#endif
