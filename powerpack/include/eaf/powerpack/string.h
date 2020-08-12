/**
 * @file
 */
#ifndef __EAF_POWERPACK_STRING_H__
#define __EAF_POWERPACK_STRING_H__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup PowerPack
 * @defgroup PowerPack-String String
 * @{
 */

#include "eaf/eaf.h"

/**
 * @brief Apply string into exist buffer.
 *
 * This function always add trailing null byte to the buffer.
 *
 * @param[in,out] buffer	Buffer, can not be NULL.
 * @param[in] size			Buffer size, must large than 0
 * @param[in,out] token		A token for internal use, must initialize to 0.
 *							If token is NULL, it write to the begin of buffer.
 * @param[in] fmt			User format
 * @param[in] ...			Argument list
 * @return					The number of characters (not including the trailing '\0')
 *							which  would  have  been written to the final string if
 *							enough space had been available
 */
EAF_API int eaf_string_apply(_Inout_ char* buffer, _In_ size_t size, _Inout_opt_ size_t* token,
	_Printf_format_string_ const char* fmt, ...)
	EAF_ATTRIBUTE_ACCESS(write_only, 1)
	EAF_ATTRIBUTE_ACCESS(read_only, 4)
	EAF_ATTRIBUTE_FORMAT_PRINTF(4, 5);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
#endif
