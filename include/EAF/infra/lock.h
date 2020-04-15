#ifndef __EAF_INFRA_LOCK_H__
#define __EAF_INFRA_LOCK_H__
#ifdef __cplusplus
extern "C" {
#endif

struct eaf_lock;
typedef struct eaf_lock eaf_lock_t;

typedef enum eaf_lock_attr
{
	eaf_lock_attr_normal,		/** normal lock */
	eaf_lock_attr_recursive,	/** recursive lock */
}eaf_lock_attr_t;

/**
* create a lock
* @param attr	attribute
* @return		lock handler
*/
eaf_lock_t* eaf_lock_create(eaf_lock_attr_t attr);

/**
* destroy a lock
* @param handler	the lock you want to destroy
*/
void eaf_lock_destroy(eaf_lock_t* handler);

/**
* Enter critical section
* @param handler	the lock
*/
void eaf_lock_enter(eaf_lock_t* handler);

/**
* Leave critical section
* @param handler	the lock
*/
void eaf_lock_leave(eaf_lock_t* handler);

#ifdef __cplusplus
}
#endif
#endif
