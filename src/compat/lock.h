#ifndef __EAF_COMPAT_MUTEX_INTERNAL_H__
#define __EAF_COMPAT_MUTEX_INTERNAL_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "eaf/infra/lock.h"
#include "c_lock.h"

struct eaf_compat_lock;
typedef struct eaf_compat_lock eaf_compat_lock_t;

/**
* ´´½¨Ëø
* @param handler	¾ä±ú
* @return			eaf_errno
*/
int eaf_compat_lock_init(eaf_compat_lock_t* handler, eaf_lock_attr_t attr);

/**
* Ïú»ÙËø
* @param handler	¾ä±ú
*/
void eaf_compat_lock_exit(eaf_compat_lock_t* handler);

/**
* ¼ÓËø
* @param handler	¾ä±ú
* @return			eaf_errno
*/
int eaf_compat_lock_enter(eaf_compat_lock_t* handler);

/**
* ½âËø
* @param handler	¾ä±ú
* @return			eaf_errno
*/
int eaf_compat_lock_leave(eaf_compat_lock_t* handler);

#ifdef __cplusplus
}
#endif
#endif	/* __EAF_COMPAT_MUTEX_INTERNAL_H__ */
