#include "eaf/eaf.h"
#include "time.h"
#include "uv.h"

#if defined(_MSC_VER)
#include <Windows.h>
#else
#include <sys/time.h>
#include <time.h>
#endif

#define USEC_IN_SEC		(1 * 1000 * 1000)

#if defined(_MSC_VER)

typedef struct time_getclocktime_static_ctx
{
	LARGE_INTEGER	offset;
	double			frequencyToMicroseconds;
	BOOL			usePerformanceCounter;
}time_getclocktime_static_ctx_t;

static time_getclocktime_static_ctx_t	g_getclocktime_ctx;

static LARGE_INTEGER _time_get_file_time_offset(void)
{
	SYSTEMTIME s;
	FILETIME f;
	LARGE_INTEGER t;

	s.wYear = 1970;
	s.wMonth = 1;
	s.wDay = 1;
	s.wHour = 0;
	s.wMinute = 0;
	s.wSecond = 0;
	s.wMilliseconds = 0;
	SystemTimeToFileTime(&s, &f);
	t.QuadPart = f.dwHighDateTime;
	t.QuadPart <<= 32;
	t.QuadPart |= f.dwLowDateTime;

	return t;
}

static void _time_getclocktime_init_once(void)
{
	LARGE_INTEGER performanceFrequency;
	g_getclocktime_ctx.usePerformanceCounter = QueryPerformanceFrequency(&performanceFrequency);
	if (g_getclocktime_ctx.usePerformanceCounter)
	{
		QueryPerformanceCounter(&g_getclocktime_ctx.offset);
		g_getclocktime_ctx.frequencyToMicroseconds = (double)performanceFrequency.QuadPart / 1000000.;
	}
	else
	{
		g_getclocktime_ctx.offset = _time_get_file_time_offset();
		g_getclocktime_ctx.frequencyToMicroseconds = 10.;
	}
}

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

int eaf_getclocktime(_Out_ eaf_clock_time_t* ts)
{
#if defined(_MSC_VER)

	LARGE_INTEGER t;
	FILETIME f;
	double microseconds;
	
	static uv_once_t get_clocktime_once = UV_ONCE_INIT;
	uv_once(&get_clocktime_once, _time_getclocktime_init_once);

	if (g_getclocktime_ctx.usePerformanceCounter)
	{
		QueryPerformanceCounter(&t);
	}
	else
	{
		GetSystemTimeAsFileTime(&f);
		t.QuadPart = f.dwHighDateTime;
		t.QuadPart <<= 32;
		t.QuadPart |= f.dwLowDateTime;
	}

	t.QuadPart -= g_getclocktime_ctx.offset.QuadPart;
	microseconds = (double)t.QuadPart / g_getclocktime_ctx.frequencyToMicroseconds;
	t.QuadPart = (LONGLONG)microseconds;
	ts->tv_sec = t.QuadPart / 1000000;
	ts->tv_usec = t.QuadPart % 1000000;
	return 0;

#else
	struct timespec tmp_ts;
	if (clock_gettime(CLOCK_MONOTONIC, &tmp_ts) < 0)
	{
		return -1;
	}

	ts->sec = tmp_ts.tv_sec;
	ts->usec = tmp_ts.tv_nsec / 1000;
	return 0;
#endif
}

int eaf_clocktime_diff(_In_ const eaf_clock_time_t* t1,
	_In_ const eaf_clock_time_t* t2, _Out_opt_ eaf_clock_time_t* diff)
{
	eaf_clock_time_t tmp_dif;
	const eaf_clock_time_t* large_t = (t1->tv_sec > t2->tv_sec) ?
	t1 : (t1->tv_sec < t2->tv_sec ? t2 : (t1->tv_usec > t2->tv_usec ? t1 : t2));
	const eaf_clock_time_t* little_t = large_t == t1 ? t2 : t1;

	tmp_dif.tv_sec = large_t->tv_sec - little_t->tv_sec;
	if (large_t->tv_usec < little_t->tv_usec)
	{
		tmp_dif.tv_usec = little_t->tv_usec - large_t->tv_usec;
		tmp_dif.tv_sec--;
	}
	else
	{
		tmp_dif.tv_usec = large_t->tv_usec - little_t->tv_usec;
	}

	if (diff != NULL)
	{
		*diff = tmp_dif;
	}

	if (tmp_dif.tv_sec == 0 && tmp_dif.tv_usec == 0)
	{
		return 0;
	}
	return t1 == little_t ? -1 : 1;
}

int eaf_clocktime_add(_Out_ eaf_clock_time_t* dst, _In_ const eaf_clock_time_t* src)
{
	/**
	 * How to check overflow:
	 * Consider two unsigned 32bits value: v1 and v2.
	 * If (v1 + v2 < v1 || v1 + v2 < v2), then overflow happen.
	 */

	/* store result temporary */
	eaf_clock_time_t tmp = *dst;

	/* add seconds first */
	tmp.tv_sec += src->tv_sec;
	if (tmp.tv_sec < dst->tv_sec || tmp.tv_sec < src->tv_sec)
	{// check overflow
		return -1;
	}

	uint64_t extra_sec = 0;
	/* format result microseconds */
	while (tmp.tv_usec >= USEC_IN_SEC)
	{
		tmp.tv_usec -= USEC_IN_SEC;
		extra_sec++;
	}
	/* format source microseconds */
	uint32_t orig_usec = src->tv_usec;
	while (orig_usec >= USEC_IN_SEC)
	{
		orig_usec -= USEC_IN_SEC;
		extra_sec++;
	}

	/* now we can add microseconds */
	tmp.tv_usec += orig_usec;
	if (tmp.tv_usec >= USEC_IN_SEC)
	{
		extra_sec++;
		tmp.tv_usec -= USEC_IN_SEC;
	}

	uint64_t orig_sec = tmp.tv_sec;
	tmp.tv_sec += extra_sec;
	if (tmp.tv_sec < orig_sec || tmp.tv_sec < extra_sec)
	{// check overflow
		return -1;
	}

	*dst = tmp;
	return 0;
}
