/** @file
 * EAF errno
 */
#ifndef __EAF_ERRNO_H__
#define __EAF_ERRNO_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "eaf/utils/define.h"

/**
 * @brief All known EAF error codes.
 */
typedef enum eaf_errno
{
	eaf_errno_success		=  0x00,		/**< Success */
	eaf_errno_unknown		= -0x01,		/**< Unknown error */
	eaf_errno_duplicate		= -0x02,		/**< Duplicated operation */
	eaf_errno_memory		= -0x03,		/**< Out of memory */
	eaf_errno_state			= -0x04,		/**< Wrong state */
	eaf_errno_notfound		= -0x05,		/**< Resource not found */
	eaf_errno_overflow		= -0x06,		/**< Overflow */
	eaf_errno_timeout		= -0x07,		/**< Operation timeout */
	eaf_errno_invalid		= -0x08,		/**< Invalid Parameter */
	eaf_errno_transfer		= -0x09,		/**< Transmission protocol error */
}eaf_errno_t;

/**
 * @brief Convert a error code into string.
 * 
 * This function returns a pointer to a string that describes the error code
 * passed in the argument `err`. If error code not defined, `NULL` will be
 * returned.
 * 
 * @param err	error code
 * @return		description
 */
EAF_API const char* eaf_strerror(_In_ int err);

#ifdef __cplusplus
}
#endif
#endif
