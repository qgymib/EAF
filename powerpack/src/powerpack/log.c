#include <stdio.h>
#include <stdarg.h>
#include "eaf/powerpack/time.h"
#include "log.h"

static const char* _log_filename(const char* file)
{
	const char* last_delimiter = file;
	for (; *file != '\0'; file++)
	{
		if (*file == '\\' || *file == '/')
		{
			last_delimiter = file + 1;
		}
	}

	return last_delimiter;
}

static char _log_ascii_to_char(unsigned char c)
{
	if (c >= 32 && c <= 126)
	{
		return c;
	}
	return '.';
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
	printf("[%s %02u:%02u:%02u.%03u %s:%d]<%s> ", prefix,
		cur_time.hour, cur_time.minute, cur_time.second, cur_time.mseconds,
		_log_filename(file), line, mod);

	/* body */
	{
		va_list ap;
		va_start(ap, fmt);
		vprintf(fmt, ap);
		va_end(ap);
	}
	
	printf("\n");
}

void eaf_dump_data(_In_ const void* data, _In_ size_t size, _In_ size_t width)
{
	const unsigned char* pdat = data;

	size_t idx_line;
	for (idx_line = 0; idx_line < size; idx_line += width)
	{
		printf("%p | ", &pdat[idx_line]);

		size_t idx_colume;
		/* printf hex */
		for (idx_colume = 0; idx_colume < width; idx_colume++)
		{
			const char* postfix = (idx_colume < width - 1) ? "" : "|";

			if (idx_colume + idx_line < size)
			{
				printf("%02x %s", pdat[idx_colume + idx_line], postfix);
			}
			else
			{
				printf("   %s", postfix);
			}
		}
		printf(" ");
		/* printf char */
		for (idx_colume = 0; (idx_colume < width) && (idx_colume + idx_line < size); idx_colume++)
		{
			printf("%c", _log_ascii_to_char(pdat[idx_colume + idx_line]));
			if (idx_colume < width - 1)
			{
				printf(" ");
			}
		}
		printf("\n");
	}
}

void eaf_dump_data_pretty(_In_ const char* name, _In_ const char* file,
	_In_ const char* func, _In_ int line, _In_ const void* data, _In_ size_t size)
{
	printf("dump `%s' with size `%zu' -> %s:%d:%s:\n", name, size, _log_filename(file), line, func);
	eaf_dump_data(data, size, 16);
}
