#include <stdio.h>
#include <stdarg.h>
#include "eaf/powerpack/time.h"
#include "log.h"

#define LOG_DEFAULT_FILTER_LEVEL	eaf_log_level_info

static eaf_log_ctx_t g_eaf_log_ctx = {
	LOG_DEFAULT_FILTER_LEVEL,
	{
		NULL,
		NULL,
	}
};

static void _log_reset_default_config(void)
{
	g_eaf_log_ctx.filter_level = LOG_DEFAULT_FILTER_LEVEL;
	eaf_log_set_callback(NULL, NULL);
}

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

static void _log_redirect(const eaf_log_info_t* info, const char* fmt, va_list ap)
{
	g_eaf_log_ctx.cb.fn(info, fmt, ap, g_eaf_log_ctx.cb.arg);
}

static void _log_direct(const eaf_log_info_t* info, const char* fmt, va_list ap)
{
	/* Direct output must respect global log level */
	if ((int)info->level < (int)g_eaf_log_ctx.filter_level)
	{
		return;
	}

	eaf_calendar_time_t cur_time;
	if (eaf_time_getsystem(&cur_time) < 0)
	{
		return;
	}

	const char* prefix;
	switch (info->level)
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
	printf("[%s %02u:%02u:%02u.%03u %s:%d %s]<%s> ", prefix,
		cur_time.hour, cur_time.minute, cur_time.second, cur_time.mseconds,
		_log_filename(info->file), info->line, info->func, info->mode);

	vprintf(fmt, ap);
	printf("\n");
}

void eaf_log(_In_ eaf_log_level_t level, _In_ const char* file,
	_In_ const char* func, _In_ int line, _In_ const char* mod,
	_In_ const char* fmt, ...)
{
	eaf_log_info_t info;
	info.file = file;
	info.func = func;
	info.line = line;
	info.mode = mod;
	info.level = level;

	va_list ap;
	va_start(ap, fmt);
	if (g_eaf_log_ctx.cb.fn != NULL)
	{
		_log_redirect(&info, fmt, ap);
	}
	else
	{
		_log_direct(&info, fmt, ap);
	}
	va_end(ap);
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

int eaf_log_init(void)
{
	_log_reset_default_config();
	return 0;
}

void eaf_log_exit(void)
{
	_log_reset_default_config();
}

void eaf_log_set_level(eaf_log_level_t level)
{
	g_eaf_log_ctx.filter_level = level;
}

eaf_log_level_t eaf_log_get_level(void)
{
	return g_eaf_log_ctx.filter_level;
}

void eaf_log_set_callback(_In_opt_ eaf_log_callback_fn fn, _Inout_opt_ void* arg)
{
	g_eaf_log_ctx.cb.fn = fn;
	g_eaf_log_ctx.cb.arg = arg;
}
