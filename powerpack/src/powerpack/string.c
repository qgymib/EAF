#include <stdarg.h>
#include <stdio.h>
#include "string.h"

size_t eaf_string_apply(char* buffer, size_t size, size_t* token, const char* fmt, ...)
{
	char* start_pos = buffer + *token;
	size_t left_size = size - *token;

	va_list ap;
	va_start(ap, fmt);
	size_t write_size =
#if defined(_MSC_VER)
		vsnprintf_s(start_pos, left_size, _TRUNCATE, fmt, ap)
#else
		vsnprintf(start_pos, left_size, fmt, ap)
#endif
	;
	va_end(ap);

	/* check if output was trunked */
	if (write_size >= left_size)
	{
		*token = size;
	}
	else
	{
		*token += write_size;
	}

	return write_size;
}
