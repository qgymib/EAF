/** @file
 * Time operations.
 */
#ifndef __EAF_POWERPACK_TIME_H__
#define __EAF_POWERPACK_TIME_H__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup PowerPack
 * @defgroup PowerPack-Time Time
 * @{
 */

#include "eaf/eaf.h"

#define EAF_TIME_IGNORE_OVERFLOW	(0x01 << 0x00)

/**
 * @brief Clock time
 */
typedef struct eaf_clock_time
{
	uint64_t	tv_sec;		/**< second */
	uint32_t	tv_nsec;	/**< nanosecond */
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
int eaf_time_get(_Out_ eaf_clock_time_t* tv);

/**
 * @brief Retrieves the current system date and time. The system time is
 *   expressed in Coordinated Universal Time (UTC).
 * @param[out] tv	A pointer to a #eaf_calendar_time_t structure to receive
 *   the current system date and time.
 * @return			#eaf_errno
 */
int eaf_time_getsystem(_Out_ eaf_calendar_time_t* tv);

/**
 * @brief Get high resolution time since some unspecified starting point.
 * @param[out] ts	Time stamp
 * @return			#eaf_errno
 */
int eaf_time_getclock(_Out_ eaf_clock_time_t* ts);

/**
 * @brief Compare `t1' with `t2', and Difference diff into `diff'
 * @param[in] t1	Time stamp 1
 * @param[in] t2	Time stamp 2
 * @param[out] diff	Difference
 * @return			-1 if t1 < t2; 1 if t1 > t2; 0 if t1 == t2
 */
int eaf_time_diffclock(_In_ const eaf_clock_time_t* t1,
	_In_ const eaf_clock_time_t* t2, _Out_opt_ eaf_clock_time_t* diff);

/**
 * @brief Add `src' to `dst'.
 * @param[in,out] dst	Destination
 * @param[in] dif		Source
 * @return				0: success; -1: overflow, and dst is not modified
 */
int eaf_time_addclock(_Inout_ eaf_clock_time_t* dst, _In_ const eaf_clock_time_t* dif);

/**
 * @brief Add `src' with `dif' and store the result into `dst'.
 *
 * By default, the `dst' is left untouched if overflow will happen. But if
 * `flags' contains #EAF_TIME_IGNORE_OVERFLOW, the `dst' still store the result,
 * and -1 is returned.
 *
 * @note It is safe if `dst' and `src' point to the same location.
 * @param[in] dst	Destination
 * @param[in] src	Source
 * @param[in] dif	The time need to add
 * @param[in] flags	zero or following values:
 *					+ #EAF_TIME_IGNORE_OVERFLOW
 * @return			0 if overflow not happen, -1 if overflow occur
 *
 */
int eaf_time_addclock_ext(_Out_ eaf_clock_time_t* dst,
	_In_ const eaf_clock_time_t* src, _In_ const eaf_clock_time_t* dif, int flags);

/**
 * @brief Format `src' and store the result into `dst'
 *
 * By default, the `dst' is left untouched if overflow will happen. But if
 * `flags' contains #EAF_TIME_IGNORE_OVERFLOW, `dst' still sotre the result,
 * and -1 is returned.
 *
 * @note It is safe if `dst' and `src' point to the same location.
 * @param[in] dst	Destination
 * @param[in] src	Source
 * @param[in] flags	zero or following values:
 *					+ #EAF_TIME_IGNORE_OVERFLOW
 * @return			0 if overflow not happen, -1 if overflow occur
 */
int eaf_time_fmtclock_ext(_Out_ eaf_clock_time_t* dst, _In_ const eaf_clock_time_t* src, int flags);

/**
 * @brief Add `dst' by `msec' millisecond(s)
 * @param[in,out] dst	Destination
 * @param[in] msec		millisecond(s)
 * @return				0: success; -1: overflow, and dst is not modified
 */
int eaf_time_addclock_msec(_Inout_ eaf_clock_time_t* dst, _In_ uint64_t msec);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
#endif
