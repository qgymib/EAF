/**
 * @file
 * EAF Monitor.
 */
#ifndef __EAF_POWERPACK_MONITOR_H__
#define __EAF_POWERPACK_MONITOR_H__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup PowerPack
 * @defgroup PowerPack-Monitor Monitor
 * @{
 */

#include "eaf/eaf.h"

/**
 * @brief Initialize monitor
 * @param[in] sec	Refresh time interval
 * @return		#eaf_errno
 */
int eaf_monitor_init(unsigned sec);

/**
 * @brief Exit monitor
 */
void eaf_monitor_exit(void);

/**
 * @brief Print service summary information into buffer.
 *
 * Buffer always NULL terminated.
 *
 * @param[in] buffer	Buffer
 * @param[in] size		Buffer size
 */
void eaf_monitor_print_tree(char* buffer, size_t size);

/**
 * @brief Reset all necessary counters.
 */
void eaf_monitor_flush(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
#endif
