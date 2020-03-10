#ifndef __EAF_COMPAT_MUTEX_INTERNAL_H__
#define __EAF_COMPAT_MUTEX_INTERNAL_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "s_mutex.h"

struct eaf_mutex;
typedef struct eaf_mutex eaf_mutex_t;

typedef enum eaf_mutex_attr
{
	eaf_mutex_attr_normal,
	eaf_mutex_attr_recursive,
}eaf_mutex_attr_t;

/**
* ´´½¨Ëø
* @param handler	¾ä±ú
* @return			eaf_errno
*/
int eaf_mutex_init(eaf_mutex_t* handler, eaf_mutex_attr_t attr);

/**
* Ïú»ÙËø
* @param handler	¾ä±ú
*/
void eaf_mutex_exit(eaf_mutex_t* handler);

/**
* ¼ÓËø
* @param handler	¾ä±ú
* @return			eaf_errno
*/
int eaf_mutex_enter(eaf_mutex_t* handler);

/**
* ½âËø
* @param handler	¾ä±ú
* @return			eaf_errno
*/
int eaf_mutex_leave(eaf_mutex_t* handler);

#ifdef __cplusplus
}
#endif
#endif	/* __EAF_COMPAT_MUTEX_INTERNAL_H__ */
