#include "eaf/eaf.h"
#include "uv.h"
#include "time.h"

#if defined(_MSC_VER)
#include <minwinbase.h>
#else
#include <time.h>
#endif

int eaf_gettimeofday(_Inout_ eaf_clock_time_t* tv)
{
	uv_timeval64_t uv_tv;
	if (uv_gettimeofday(&uv_tv) < 0)
	{
		return eaf_errno_unknown;
	}

	tv->tv_sec = uv_tv.tv_sec;
	tv->tv_usec = uv_tv.tv_usec;

	return eaf_errno_success;
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
