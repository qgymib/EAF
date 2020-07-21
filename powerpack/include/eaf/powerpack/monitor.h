#ifndef __EAF_POWERPACK_MONITOR_H__
#define __EAF_POWERPACK_MONITOR_H__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize monitor
 * @return		#eaf_errno
 */
int eaf_monitor_init(void);

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

#ifdef __cplusplus
}
#endif
#endif
