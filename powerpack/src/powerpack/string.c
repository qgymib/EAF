#include <stdarg.h>
#include <stdio.h>
#include "string.h"

static int _string_snprintf(char* buffer, size_t size, const char* fmt, va_list ap)
{
#if defined(_MSC_VER)

	int count = -1;
	if (size != 0)
	{
		count = vsnprintf_s(buffer, size, _TRUNCATE, fmt, ap);
	}
	if (count == -1)
	{
		count = _vscprintf(fmt, ap);
	}

	return count;
#else
	return vsnprintf(buffer, size, fmt, ap);
#endif
}

EAF_API
int eaf_string_apply(_Inout_ char* buffer, _In_ size_t size, _Inout_opt_ size_t* token,
	_Printf_format_string_ const char* fmt, ...)
{
	int write_size;
	char* start_pos = token != NULL ? buffer + *token : buffer;
	size_t left_size = token != NULL ? size - *token : size;

	va_list ap;
	va_start(ap, fmt);
	write_size = _string_snprintf(start_pos, left_size, fmt, ap);
	va_end(ap);

	if (write_size < 0)
	{
		return write_size;
	}

	if (token != NULL)
	{
		/* check if output was trunked */
		if ((size_t)write_size >= left_size)
		{
			*token = size;
		}
		else
		{
			*token += write_size;
		}
	}

	return write_size;
}
