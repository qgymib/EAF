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
 *
 * @{
 */
#include "eaf/eaf.h"
#include "eaf/powerpack/define.h"
#include "eaf/powerpack/hash.h"
#include "eaf/powerpack/log.h"
#include "eaf/powerpack/message.h"
#include "eaf/powerpack/net.h"
#include "eaf/powerpack/ringbuffer.h"
#include "eaf/powerpack/time.h"
#include "eaf/powerpack/timer.h"
#include "eaf/powerpack/watchdog.h"
/**
 * @}
 */

/**
 * @brief PowerPack configuration
 */
typedef struct eaf_powerpack_cfg
{
	eaf_thread_attr_t	unistd;		/**< thread configure for unistd */
}eaf_powerpack_cfg_t;

/**
 * @brief Setup PowerPack
 * @param[in] cfg	Configuration
 * @return			#eaf_errno
 */
int eaf_powerpack_init(_In_ const eaf_powerpack_cfg_t* cfg);

/**
 * @brief must be called after #eaf_cleanup
 */
void eaf_powerpack_exit(void);

#ifdef __cplusplus
}
#endif
#endif
