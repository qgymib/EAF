#ifndef __EAF_POWERPACK_STRING_INTERNAL_H__
#define __EAF_POWERPACK_STRING_INTERNAL_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>

/**
 * @brief Apply string into exist buffer.
 * @param[in] buffer	Buffer, can not be NULL.
 * @param[in] size		Buffer size, must large than 0
 * @param[in] token		A token for internal use, must initialize to 0
 * @param[in] fmt		User format
 * @param[in] ...		Argument list
 * @return				The number of characters (not including the trailing '\0')
 *						which  would  have  been written to the final string if
 *						enough space had been available
 */
size_t eaf_string_apply(char* buffer, size_t size, size_t* token, const char* fmt, ...);

#ifdef __cplusplus
}
#endif
#endif
