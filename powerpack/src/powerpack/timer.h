#ifndef __EAF_POWERPACK_TIMER_INTERNAL_H__
#define __EAF_POWERPACK_TIMER_INTERNAL_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "eaf/powerpack/timer.h"

/**
* Initialize timer
* @return	eaf_errno
*/
int eaf_powerpack_timer_init(void);

/**
* Cleanup timer
*/
void eaf_powerpack_timer_exit(void);

#ifdef __cplusplus
}
#endif
#endif
