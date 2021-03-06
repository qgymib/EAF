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
#include "eaf/eaf.h"

#if defined(WITHOUT_LOG)
#	define EAF_LOG_INTERNAL_WRAPPER(level, file, func, line, mode, fmt, ...)	\
		do {} EAF_MSVC_WARNING_GUARD(4217, while (0))
#else
#	define EAF_LOG_INTERNAL_WRAPPER(level, file, func, line, mode, fmt, ...)	\
		eaf_log(level, file, func, line, mode, fmt, ##__VA_ARGS__)
#endif

/**
 * @def EAF_LOG_TRACE
 * @brief Log: Trace
 *
 * Designates finer-grained informational events than the DEBUG.
 *
 * @param[in] mod	Module name
 * @param[in] fmt	Format string
 * @param[in] ...	details
 */
#if defined(WITHOUT_LOG_TRACE)
#	define EAF_LOG_TRACE(mod, fmt, ...) \
		do {} EAF_MSVC_WARNING_GUARD(4217, while (0))
#else
#	define EAF_LOG_TRACE(mod, fmt, ...) \
		EAF_LOG_INTERNAL_WRAPPER(eaf_log_level_trace, __FILE__, __FUNCTION__, __LINE__, mod, fmt, ##__VA_ARGS__)
#endif

/**
 * @def EAF_LOG_DEBUG
 * @brief Log: Debug
 *
 * Designates fine-grained informational events that are most useful to debug
 * an application.
 *
 * @param[in] mod	Module name
 * @param[in] fmt	Format string
 * @param[in] ...	details
 */
#if defined(WITHOUT_LOG_DEBUG)
#	define EAF_LOG_DEBUG(mod, fmt, ...) \
		do {} EAF_MSVC_WARNING_GUARD(4217, while (0))
#else
#	define EAF_LOG_DEBUG(mod, fmt, ...) \
		EAF_LOG_INTERNAL_WRAPPER(eaf_log_level_debug, __FILE__, __FUNCTION__, __LINE__, mod, fmt, ##__VA_ARGS__)
#endif

/**
 * @def EAF_LOG_INFO
 * @brief Log: Info
 *
 * Designates informational messages that highlight the progress of the
 * application at coarse-grained level.
 *
 * @param[in] mod	Module name
 * @param[in] fmt	Format string
 * @param[in] ...	details
 */
#if defined(WITHOUT_LOG_INFO)
#	define EAF_LOG_INFO(mod, fmt, ...)  \
		do {} EAF_MSVC_WARNING_GUARD(4217, while (0))
#else
#	define EAF_LOG_INFO(mod, fmt, ...)  \
		EAF_LOG_INTERNAL_WRAPPER(eaf_log_level_info, __FILE__, __FUNCTION__, __LINE__, mod, fmt, ##__VA_ARGS__)
#endif

/**
 * @def EAF_LOG_WARN
 * @brief Log: Warn
 *
 * Designates potentially harmful situations.
 *
 * @param[in] mod	Module name
 * @param[in] fmt	Format string
 * @param[in] ...	details
 */
#if defined(WITHOUT_LOG_WARN)
#	define EAF_LOG_WARN(mod, fmt, ...)  \
		do {} EAF_MSVC_WARNING_GUARD(4217, while (0))
#else
#	define EAF_LOG_WARN(mod, fmt, ...)  \
		EAF_LOG_INTERNAL_WRAPPER(eaf_log_level_warn, __FILE__, __FUNCTION__, __LINE__, mod, fmt, ##__VA_ARGS__)
#endif

/**
 * @def EAF_LOG_ERROR
 * @brief Log: Error
 *
 * Designates error events that might still allow the application to continue
 * running.
 *
 * @param[in] mod	Module name
 * @param[in] fmt	Format string
 * @param[in] ...	details
 */
#if defined(WITHOUT_LOG_ERROR)
#	define EAF_LOG_ERROR(mod, fmt, ...) \
		do {} EAF_MSVC_WARNING_GUARD(4217, while (0))
#else
#	define EAF_LOG_ERROR(mod, fmt, ...) \
		EAF_LOG_INTERNAL_WRAPPER(eaf_log_level_error, __FILE__, __FUNCTION__, __LINE__, mod, fmt, ##__VA_ARGS__)
#endif

/**
 * @def EAF_LOG_FATAL
 * @brief Log: Fatal
 *
 * Designates very severe error events that will presumably lead the application to abort.
 *
 * @param[in] mod	Module name
 * @param[in] fmt	Format string
 * @param[in] ...	details
 */
#if defined(WITHOUT_LOG_FATAL)
#	define EAF_LOG_FATAL(mod, fmt, ...) \
		do {} EAF_MSVC_WARNING_GUARD(4217, while (0))
#else
#	define EAF_LOG_FATAL(mod, fmt, ...) \
		EAF_LOG_INTERNAL_WRAPPER(eaf_log_level_fatal, __FILE__, __FUNCTION__, __LINE__, mod, fmt, ##__VA_ARGS__)
#endif

/**
 * @brief Dump data as hex
 * @param[in] data	A pointer to data
 * @param[in] size	The size of data
 */
#define EAF_DUMP(data, size)	\
	eaf_dump_data_pretty(#data, __FILE__, __FUNCTION__, __LINE__, data, size)

/**
 * @brief Log level
 */
typedef enum eaf_log_level
{
	eaf_log_level_trace,		/**< Trace */
	eaf_log_level_debug,		/**< Debug */
	eaf_log_level_info,			/**< Info. This is the default log level. */
	eaf_log_level_warn,			/**< Warn */
	eaf_log_level_error,		/**< Error */
	eaf_log_level_fatal,		/**< Fatal */
}eaf_log_level_t;

/**
 * @brief Log information
 */
typedef struct eaf_log_info
{
	const char*			mode;	/**< Module name */
	const char*			file;	/**< File name */
	const char*			func;	/**< Function name */
	int					line;	/**< Line */
	eaf_log_level_t		level;	/**< Log level */
}eaf_log_info_t;

/**
 * @brief Log callback
 * @param[in] info	Log information
 * @param[in] fmt	Format
 * @param[in] ap	Argument list
 * @param[in] arg	User defined argument
 */
typedef void(*eaf_log_callback_fn)(_In_ const eaf_log_info_t* info,
	_In_ const char* fmt, _In_ va_list ap, _Inout_opt_ void* arg);

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
	_In_ _Printf_format_string_ const char* fmt, ...)
	EAF_ATTRIBUTE_FORMAT_PRINTF(6, 7);

/**
 * @brief Set log level
 * @param[in] level		Log level
 */
void eaf_log_set_level(_In_ eaf_log_level_t level);

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
 *
 * @note The callback does not affected by log level filter
 * @note You cannot set more than one callback, otherwise the later one will
 *   replace the earlier one.
 * @param[in] fn	Callback function
 * @param[in] arg	User defined argument
 */
void eaf_log_set_callback(_In_opt_ eaf_log_callback_fn fn, _Inout_opt_ void* arg);

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
