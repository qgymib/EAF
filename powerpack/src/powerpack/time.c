#include "eaf/eaf.h"
#include "time.h"

#if defined(_MSC_VER)
#include <Windows.h>
#else
#include <time.h>
#endif

int eaf_gettimeofday(_Inout_ eaf_clock_time_t* tv)
{
	/**
	 * uv_gettimeofday() was added in libuv-1.28.0, which was release in
	 * 2019/04/15.
	 * As this version is not old enough, we copy implement from libuv.
	 */
#if 0
	uv_timeval64_t uv_tv;
	if (uv_gettimeofday(&uv_tv) < 0)
	{
		return eaf_errno_unknown;
	}

	tv->tv_sec = uv_tv.tv_sec;
	tv->tv_usec = uv_tv.tv_usec;

	return eaf_errno_success;
#endif

#if defined(_MSC_VER)
	/* Based on https://doxygen.postgresql.org/gettimeofday_8c_source.html */
	const uint64_t epoch = (uint64_t)116444736000000000ULL;
	FILETIME file_time;
	ULARGE_INTEGER ularge;

	GetSystemTimeAsFileTime(&file_time);
	ularge.LowPart = file_time.dwLowDateTime;
	ularge.HighPart = file_time.dwHighDateTime;
	tv->tv_sec = (int64_t)((ularge.QuadPart - epoch) / 10000000L);
	tv->tv_usec = (int32_t)(((ularge.QuadPart - epoch) % 10000000L) / 10);
	return 0;
#else
	struct timeval time;
	if (gettimeofday(&time, NULL) != 0)
	{
		return eaf_errno_unknown;
	}

	tv->tv_sec = (uint64_t)time.tv_sec;
	tv->tv_usec = (uint32_t)time.tv_usec;
	return 0;
#endif
}

int eaf_getsystemtime(_Inout_ eaf_calendar_time_t* tv)
{
#if defined(_MSC_VER)
	SYSTEMTIME cur_time;
	GetSystemTime(&cur_time);

	tv->year = cur_time.wYear;
	tv->month = cur_time.wMonth;
	tv->day = cur_time.wDay;
	tv->hour = cur_time.wHour;
	tv->minute = cur_time.wMinute;
	tv->second = cur_time.wSecond;
	tv->mseconds = cur_time.wMilliseconds;

	return eaf_errno_success;
#else
	time_t cur_time = time(NULL);
	if (cur_time == (time_t)-1)
	{
		return eaf_errno_unknown;
	}

	struct tm tmp_tm;
	if (gmtime_r(&cur_time, &tmp_tm) == NULL)
	{
		return eaf_errno_unknown;
	}

	eaf_clock_time_t tmp_clock;
	if (eaf_gettimeofday(&tmp_clock) < 0)
	{
		return eaf_errno_unknown;
	}

	tv->year = tmp_tm.tm_yday + 1900;
	tv->month = tmp_tm.tm_mon + 1;
	tv->day = tmp_tm.tm_mday;
	tv->hour = tmp_tm.tm_hour;
	tv->minute = tmp_tm.tm_min;
	tv->second = tmp_tm.tm_sec;
	tv->mseconds = tmp_clock.tv_usec / 1000;

	return eaf_errno_success;
#endif
}
