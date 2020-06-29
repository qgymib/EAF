/** @file
 * Time operations.
 */
#ifndef __EAF_POWERPACK_TIME_H__
#define __EAF_POWERPACK_TIME_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "eaf/utils/define.h"
#include <stdint.h>

/**
 * @brief Clock time
 */
typedef struct eaf_clock_time
{
	uint64_t	tv_sec;		/**< second */
	uint32_t	tv_usec;	/**< microsecond */
}eaf_clock_time_t;

/**
 * @brief Calendar time
 */
typedef struct eaf_calendar_time
{
	unsigned	year;		/**< The year*/
	unsigned	month;		/**< The month. The valid values for this member are 1 through 12. */
	unsigned	day;		/**< The day of the month. The valid values for this member are 1 through 31. */
	unsigned	hour;		/**< The hour. The valid values for this member are 0 through 23. */
	unsigned	minute;		/**< The minute. The valid values for this member are 0 through 59. */
	unsigned	second;		/**< The second. The valid values for this member are 0 through 59. */
	unsigned	mseconds;	/**< The millisecond. The valid values for this member are 0 through 999. */
}eaf_calendar_time_t;

/**
 * @brief Get current time
 * @param[out] tv	Time stamp
 * @return			#eaf_errno
 */
int eaf_gettimeofday(_Out_ eaf_clock_time_t* tv);

/**
 * @brief Retrieves the current system date and time. The system time is
 *   expressed in Coordinated Universal Time (UTC).
 * @param[out] tv	A pointer to a #eaf_calendar_time_t structure to receive
 *   the current system date and time.
 * @return			#eaf_errno
 */
int eaf_getsystemtime(_Out_ eaf_calendar_time_t* tv);

#ifdef __cplusplus
}
#endif
#endif
