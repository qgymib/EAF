#include <stdio.h>
#include <stdarg.h>
#include "eaf/utils/log.h"

static const char* _log_getfilename(const char* file)
{
	const char* pos = file;

	for (; *file; ++file)
	{
		if (*file == '\\' || *file == '/')
		{
			pos = file + 1;
		}
	}
	return pos;
}

void eaf_log(eaf_log_level_t level, const char* file, const char* func, int line, const char* fmt, ...)
{
	(void)level;

	va_list ap;
	va_start(ap, fmt);

	printf("[%s:%d %s]: ", _log_getfilename(file), line, func);
	vprintf(fmt, ap);
	printf("\n");

	va_end(ap);
}
