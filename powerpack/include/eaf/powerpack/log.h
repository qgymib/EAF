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

#include <stdarg.h>
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
 * @brief Dump data as hex
 * @param[in] data	A pointer to data
 * @param[in] size	The size of data
 */
#define EAF_DUMP(data, size)	\
	eaf_dump_data_pretty(#data, __FILE__, __FUNCTION__, __LINE__, data, size)

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

typedef struct eaf_log_info
{
	eaf_log_level_t	level;
	int				line;
	const char*		mode;
	const char*		file;
	const char*		func;
}eaf_log_info_t;

/**
 * @brief Log callback
 * @param[in] info	Log information
 * @param[in] fmt	Format
 * @param[in] ap	Argument list
 * @param[in] arg	User defined argument
 */
typedef void(*eaf_log_callback_fn)(const eaf_log_info_t* info, const char* fmt, va_list ap, void* arg);

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
 * @brief Set log level
 * @param[in] level		Log level
 */
void eaf_log_set_level(eaf_log_level_t level);

/**
 * @brief Get current filter level
 * @return				Log level
 */
eaf_log_level_t eaf_log_get_level(void);

/**
 * @brief Set log callback
 *
 * Once set, all logs will be redirect to this callback. So if you still need
 * output log on your screen, you need to do it manually.
 * @note The callback does not affected by log level filter
 * @param[in] fn	Callback function
 * @param[in] arg	User defined argument
 */
void eaf_log_set_callback(eaf_log_callback_fn fn, void* arg);

/**
 * @brief Dump hex data
 * @param[in] data	The data pointer
 * @param[in] size	The data size
 * @param[in] width	The amount of bytes one line contains
 */
void eaf_dump_data(_In_ const void* data, _In_ size_t size, _In_ size_t width);

/**
 * @private
 * @brief Dump hex data with extra info
 * @param[in] name	The name of data
 * @param[in] file	The file name
 * @param[in] func	The function name
 * @param[in] line	The line
 * @param[in] data	The data
 * @param[in] size	The data size
 */
void eaf_dump_data_pretty(_In_ const char* name, _In_ const char* file,
	_In_ const char* func, _In_ int line, _In_ const void* data, _In_ size_t size);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
#endif
