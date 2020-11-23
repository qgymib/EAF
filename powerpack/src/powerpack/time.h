/**
 * @file
 * Internal common defines for time.h
 */
#ifndef __EAF_POWERPACK_TIME_INTERNAL_H__
#define __EAF_POWERPACK_TIME_INTERNAL_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "eaf/powerpack/time.h"

/**
 * @berif The number of milliseconds in one second
 */
#define MSEC_IN_SEC		(1 * 1000)

/**
 * @berif The number of microseconds in one second
 */
#define USEC_IN_SEC		(MSEC_IN_SEC * 1000)

/**
 * @berif The number of nanoseconds in one second
 */
#define NSEC_IN_SEC		(USEC_IN_SEC * 1000)

#ifdef __cplusplus
}
#endif
#endif
