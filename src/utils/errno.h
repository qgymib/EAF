/**
* @file
* ∂®“Â¥ÌŒÛ¬Î
*/
#if !defined(__EAF_ERRNO_INTERNAL_H__) || defined(__EAF_HEADER_MULTI_READ__)
#define __EAF_ERRNO_INTERNAL_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "EAF/utils/errno.h"

#ifndef EAF_DEFINE_ERRNO
#	define EAF_DEFINE_ERRNO(err, destribe)
#endif

EAF_DEFINE_ERRNO(eaf_errno_success,		"operation success")
EAF_DEFINE_ERRNO(eaf_errno_unknown,		"unknown or system error")
EAF_DEFINE_ERRNO(eaf_errno_duplicate,	"duplicated operation or resource")
EAF_DEFINE_ERRNO(eaf_errno_memory,		"no memory")
EAF_DEFINE_ERRNO(eaf_errno_state,		"resource is in wrong state")
EAF_DEFINE_ERRNO(eaf_errno_notfound,	"resource not found")
EAF_DEFINE_ERRNO(eaf_errno_overflow,	"msgq overflow")

#ifdef __cplusplus
}
#endif
#endif
