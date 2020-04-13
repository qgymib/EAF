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
#	define EAF_DEFINE_ERRNO(err, describe)
#endif

EAF_DEFINE_ERRNO(eaf_errno_success,		"success")
EAF_DEFINE_ERRNO(eaf_errno_unknown,		"unknown or system error")
EAF_DEFINE_ERRNO(eaf_errno_duplicate,	"duplicated operation or request")
EAF_DEFINE_ERRNO(eaf_errno_memory,		"memory lack")
EAF_DEFINE_ERRNO(eaf_errno_state,		"wrong state")
EAF_DEFINE_ERRNO(eaf_errno_notfound,	"not found")
EAF_DEFINE_ERRNO(eaf_errno_overflow,	"overflow")
EAF_DEFINE_ERRNO(eaf_errno_timeout,		"timeout")
EAF_DEFINE_ERRNO(eaf_errno_invalid,		"invalid parameters")
EAF_DEFINE_ERRNO(eaf_errno_rpc_failure, "RPC operation failure")

#ifdef __cplusplus
}
#endif
#endif
