/** @file
 * Log operations.
 */
#ifndef __EAF_POWERPACK_LOG_H__
#define __EAF_POWERPACK_LOG_H__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup PowerPack
 * @defgroup PowerPack-Log Log
 * @{
 */

#include "eaf/utils/define.h"

/**
 * @brief Log: Trace
 *
 * Designates finer-grained informational events than the DEBUG.
 *
 * @param[in] mod	Module name
 * @param[in] fmt	Format string
 * @param[in] ...	details
 */
#define EAF_LOG_TRACE(mod, fmt, ...) \
	eaf_log(eaf_log_level_trace, __FILE__, __FUNCTION__, __LINE__, mod, fmt, ##__VA_ARGS__)

/**
 * @brief Log: Debug
 *
 * Designates fine-grained informational events that are most useful to debug
 * an application.
 *
 * @param[in] mod	Module name
 * @param[in] fmt	Format string
 * @param[in] ...	details
 */
#define EAF_LOG_DEBUG(mod, fmt, ...) \
	eaf_log(eaf_log_level_debug, __FILE__, __FUNCTION__, __LINE__, mod, fmt, ##__VA_ARGS__)

/**
 * @brief Log: Info
 *
 * Designates informational messages that highlight the progress of the
 * application at coarse-grained level.
 *
 * @param[in] mod	Module name
 * @param[in] fmt	Format string
 * @param[in] ...	details
 */
#define EAF_LOG_INFO(mod, fmt, ...)  \
	eaf_log(eaf_log_level_info, __FILE__, __FUNCTION__, __LINE__, mod, fmt, ##__VA_ARGS__)

/**
 * @brief Log: Warn
 *
 * Designates potentially harmful situations.
 *
 * @param[in] mod	Module name
 * @param[in] fmt	Format string
 * @param[in] ...	details
 */
#define EAF_LOG_WARN(mod, fmt, ...)  \
	eaf_log(eaf_log_level_warn, __FILE__, __FUNCTION__, __LINE__, mod, fmt, ##__VA_ARGS__)

/**
 * @brief Log: Error
 *
 * Designates error events that might still allow the application to continue
 * running.
 *
 * @param[in] mod	Module name
 * @param[in] fmt	Format string
 * @param[in] ...	details
 */
#define EAF_LOG_ERROR(mod, fmt, ...) \
	eaf_log(eaf_log_level_error, __FILE__, __FUNCTION__, __LINE__, mod, fmt, ##__VA_ARGS__)

/**
 * @brief Log: Fatal
 *
 * Designates very severe error events that will presumably lead the application to abort.
 *
 * @param[in] mod	Module name
 * @param[in] fmt	Format string
 * @param[in] ...	details
 */
#define EAF_LOG_FATAL(mod, fmt, ...) \
	eaf_log(eaf_log_level_fatal, __FILE__, __FUNCTION__, __LINE__, mod, fmt, ##__VA_ARGS__)

/**
 * @private
 * @brief Log level
 */
typedef enum eaf_log_level
{
	eaf_log_level_trace,	/**< Trace */
	eaf_log_level_debug,	/**< Debug */
	eaf_log_level_info,		/**< Info */
	eaf_log_level_warn,		/**< Warn */
	eaf_log_level_error,	/**< Error */
	eaf_log_level_fatal,	/**< Fatal */
}eaf_log_level_t;

/**
 * @private
 * @brief Log
 * @param[in] level		Log level
 * @param[in] file		The file
 * @param[in] func		The function
 * @param[in] line		The line
 * @param[in] mod   	Module name
 * @param[in] fmt		Syntax
 * @param[in] ...		Argument list
 */
void eaf_log(_In_ eaf_log_level_t level, _In_ const char* file,
	_In_ const char* func, _In_ int line, _In_ const char* mod,
	_In_ const char* fmt, ...);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
#endif
