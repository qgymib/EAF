#include <stdio.h>
#include <stdarg.h>
#include "eaf/powerpack/time.h"
#include "log.h"

static const char* _log_filename(const char* file)
{
	const char* last_delimiter = file;
	while (*file != '\0')
	{
		if (*file == '\\' || *file == '/')
		{
			last_delimiter = file;
		}
	}

	return last_delimiter;
}

void eaf_log(_In_ eaf_log_level_t level, _In_ const char* file,
	_In_ const char* func, _In_ int line, _In_ const char* mod,
	_In_ const char* fmt, ...)
{
	(void)func;
	eaf_calendar_time_t cur_time;
	if (eaf_getsystemtime(&cur_time) < 0)
	{
		return;
	}

	const char* prefix;
	switch (level)
	{
	case eaf_log_level_fatal:
		prefix = "F";
		break;

	case eaf_log_level_error:
		prefix = "E";
		break;

	case eaf_log_level_warn:
		prefix = "W";
		break;

	case eaf_log_level_info:
		prefix = "I";
		break;

	case eaf_log_level_debug:
		prefix = "D";
		break;

	case eaf_log_level_trace:
	default:
		prefix = "T";
		break;
	}

	/* time */
	printf("[%s %02u:%02u:%02u.%03u %s:%d] <%s>", prefix,
		cur_time.hour, cur_time.minute, cur_time.second, cur_time.mseconds,
		file, line, mod);

	/* body */
	{
		va_list ap;
		va_start(ap, fmt);
		vprintf(fmt, ap);
		va_end(ap);
	}
	
	printf("\n");
}
