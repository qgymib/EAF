#ifndef __EAF_UTILS_LOG_H__
#define __EAF_UTILS_LOG_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#define EAF_LOG_TRACE(fmt, ...)	\
	eaf_log(eaf_log_level_trace, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)
#define EAF_LOG_INFO(fmt, ...)	\
	eaf_log(eaf_log_level_info, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)
#define EAF_LOG_WARN(fmt, ...)	\
	eaf_log(eaf_log_level_warn, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)
#define EAF_LOG_ERROR(fmt, ...)	\
	eaf_log(eaf_log_level_error, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

typedef enum eaf_log_level
{
	eaf_log_level_trace,
	eaf_log_level_info,
	eaf_log_level_warn,
	eaf_log_level_error,
}eaf_log_level_t;

void eaf_log(eaf_log_level_t level, const char* file, const char* func, int line, const char* fmt, ...);

#ifdef __cplusplus
}
#endif
#endif
