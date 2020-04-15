#ifndef __EAF_POWERPACK_H__
#define __EAF_POWERPACK_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "EAF/eaf.h"
#include "EAF/powerpack/timer.h"

typedef struct eaf_powerpack_cfg
{
	eaf_thread_attr_t	unistd;	/** thread configure for unistd */
}eaf_powerpack_cfg_t;

/**
* Setup powerpack
* @param cfg	configure
* @return		eaf_errno
*/
int eaf_powerpack_init(const eaf_powerpack_cfg_t* cfg);

/**
* must be called after `eaf_cleanup`
*/
void eaf_powerpack_exit(void);

#ifdef __cplusplus
}
#endif
#endif
