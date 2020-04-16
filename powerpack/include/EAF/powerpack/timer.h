#ifndef __EAF_POWERPACK_TIMER_H__
#define __EAF_POWERPACK_TIMER_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "EAF/core/service.h"

#define eaf_sleep(ms)	\
	do {\
		_eaf_local->unsafe.v_ulong = ms;\
		eaf_yield_ext(eaf_powerpack_sleep_commit, NULL);\
	} while (0)

/**
* sleep
*/
void eaf_powerpack_sleep_commit(eaf_service_local_t* local, void* arg);

#ifdef __cplusplus
}
#endif
#endif
