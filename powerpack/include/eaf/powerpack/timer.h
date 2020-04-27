/** @file
 * Enhance timer
 */
#ifndef __EAF_POWERPACK_TIMER_H__
#define __EAF_POWERPACK_TIMER_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "eaf/eaf.h"

/**
 * @brief Suspends execution of the calling service for (at least) ms milliseconds.
 * @param[in] ms	Sleep timeout in milliseconds
 */
#define eaf_sleep(ms)	\
	do {\
		eaf_yield_ext(eaf_powerpack_sleep_commit,\
			(void*)(uintptr_t)(unsigned)(ms));\
	} while (0)

/**
 * @brief (Internal) Make service sleep
 * @note This function should only for internal usage.
 * @param[in,out] local	Service local storage
 * @param[in,out] arg	Sleep timeout
 */
void eaf_powerpack_sleep_commit(_Inout_ eaf_service_local_t* local, _Inout_opt_ void* arg);

#ifdef __cplusplus
}
#endif
#endif
