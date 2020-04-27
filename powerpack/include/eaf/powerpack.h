/** @file
 * Include this file to use all powerpack feature.
 */
#ifndef __EAF_POWERPACK_H__
#define __EAF_POWERPACK_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "eaf/eaf.h"
#include "eaf/powerpack/define.h"
#include "eaf/powerpack/hash.h"
#include "eaf/powerpack/message.h"
#include "eaf/powerpack/net.h"
#include "eaf/powerpack/ringbuffer.h"
#include "eaf/powerpack/timer.h"

/**
 * @brief Powerpack configuration
 */
typedef struct eaf_powerpack_cfg
{
	uint32_t			service_id;	/**< powerpack need a empty service to attach */
	eaf_thread_attr_t	unistd;		/**< thread configure for unistd */
}eaf_powerpack_cfg_t;

/**
* @brief Setup powerpack
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
