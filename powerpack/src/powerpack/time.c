#include "eaf/eaf.h"
#include "time.h"
#include "uv.h"

#if defined(_MSC_VER)
#include <Windows.h>
#else
#include <sys/time.h>
#include <time.h>
#endif

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

int eaf_time_get(_Out_ eaf_clock_time_t* tv)
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
	tv->tv_nsec = uv_tv.tv_usec * 1000;

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
	tv->tv_nsec = (int32_t)(((ularge.QuadPart - epoch) % 10000000L) * 100);
	return 0;
#else
	struct timeval time;
	if (gettimeofday(&time, NULL) != 0)
	{
		return eaf_errno_unknown;
	}

	tv->tv_sec = (uint64_t)time.tv_sec;
	tv->tv_nsec = (uint32_t)time.tv_usec * 1000;
	return 0;
#endif
}

int eaf_time_getsystem(_Out_ eaf_calendar_time_t* tv)
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
	if (eaf_time_get(&tmp_clock) < 0)
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

int eaf_time_getclock(_Out_ eaf_clock_time_t* ts)
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
	ts->tv_nsec = (t.QuadPart % 1000000) * 1000;
	return 0;

#else
	struct timespec tmp_ts;
	if (clock_gettime(CLOCK_MONOTONIC, &tmp_ts) < 0)
	{
		return -1;
	}

	ts->tv_sec = tmp_ts.tv_sec;
	ts->tv_nsec = tmp_ts.tv_nsec;
	return 0;
#endif
}

int eaf_time_diffclock(_In_ const eaf_clock_time_t* t1,
	_In_ const eaf_clock_time_t* t2, _Out_opt_ eaf_clock_time_t* diff)
{
	eaf_clock_time_t tmp_dif;
	const eaf_clock_time_t* large_t = (t1->tv_sec > t2->tv_sec) ?
	t1 : (t1->tv_sec < t2->tv_sec ? t2 : (t1->tv_nsec > t2->tv_nsec ? t1 : t2));
	const eaf_clock_time_t* little_t = large_t == t1 ? t2 : t1;

	tmp_dif.tv_sec = large_t->tv_sec - little_t->tv_sec;
	if (large_t->tv_nsec < little_t->tv_nsec)
	{
		tmp_dif.tv_nsec = little_t->tv_nsec - large_t->tv_nsec;
		tmp_dif.tv_sec--;
	}
	else
	{
		tmp_dif.tv_nsec = large_t->tv_nsec - little_t->tv_nsec;
	}

	if (diff != NULL)
	{
		*diff = tmp_dif;
	}

	if (tmp_dif.tv_sec == 0 && tmp_dif.tv_nsec == 0)
	{
		return 0;
	}
	return t1 == little_t ? -1 : 1;
}

int eaf_time_addclock(_Inout_ eaf_clock_time_t* dst, _In_ const eaf_clock_time_t* dif)
{
	return eaf_time_addclock_ext(dst, dst, dif, 0);
}

int eaf_time_fmtclock_ext(_Out_ eaf_clock_time_t* dst, _In_ const eaf_clock_time_t* src, int flags)
{
	eaf_clock_time_t tmp = *src;
	while (tmp.tv_nsec >= NSEC_IN_SEC)
	{
		tmp.tv_sec++;
		tmp.tv_nsec -= NSEC_IN_SEC;
	}

	/* Check overflow */
	int flag_overflow = !!(tmp.tv_sec < src->tv_sec);
	if (flag_overflow && !(flags & EAF_TIME_IGNORE_OVERFLOW))
	{
		goto fin;
	}

	*dst = tmp;

fin:
	return 0 - flag_overflow;
}

int eaf_time_addclock_ext(_Out_ eaf_clock_time_t* dst,
	_In_ const eaf_clock_time_t* src, _In_ const eaf_clock_time_t* dif, int flags)
{
	int flag_overflow = 0;
	eaf_clock_time_t tmp;

	/* Copy src to tmp */
	{
		int step_overflow = eaf_time_fmtclock_ext(&tmp, src, flags);
		flag_overflow += step_overflow;
		if (step_overflow && !(flags & EAF_TIME_IGNORE_OVERFLOW))
		{
			goto fin;
		}
	}

	/* Add seconds */
	{
		int step_overflow;

		uint64_t tmp_sec = tmp.tv_sec;
		tmp.tv_sec += dif->tv_sec;
		step_overflow = tmp.tv_sec < tmp_sec;	/* Check overflow */
		flag_overflow += step_overflow;
		if (step_overflow && !(flags & EAF_TIME_IGNORE_OVERFLOW))
		{
			goto fin;
		}
	}

	/* Convert microseconds to seconds */
	{
		int step_overflow;

		uint64_t tmp_sec = tmp.tv_sec;
		uint32_t tmp_nsec = dif->tv_nsec;
		while (tmp_nsec >= NSEC_IN_SEC)
		{
			tmp_nsec -= NSEC_IN_SEC;
			tmp.tv_sec++;
		}

		step_overflow = tmp.tv_sec < tmp_sec;
		flag_overflow += step_overflow;
		if (step_overflow && !(flags & EAF_TIME_IGNORE_OVERFLOW))
		{
			goto fin;
		}
		tmp.tv_nsec += tmp_nsec;
	}

	/* Fix microseconds */
	{
		int step_overflow;

		uint64_t tmp_sec = tmp.tv_sec;
		while (tmp.tv_nsec >= NSEC_IN_SEC)
		{
			tmp.tv_sec++;
			tmp.tv_nsec -= NSEC_IN_SEC;
		}

		step_overflow = tmp.tv_sec < tmp_sec;
		flag_overflow += step_overflow;
		if (step_overflow && !(flags & EAF_TIME_IGNORE_OVERFLOW))
		{
			goto fin;
		}
	}

	*dst = tmp;

fin:
	return 0 - !!flag_overflow;
}

int eaf_time_addclock_msec(_Inout_ eaf_clock_time_t* dst, _In_ uint64_t msec)
{
	eaf_clock_time_t tmp = { 0, 0 };
	while (msec >= MSEC_IN_SEC)
	{
		msec -= MSEC_IN_SEC;
		tmp.tv_sec++;
	}
	tmp.tv_nsec = (uint32_t)(msec * 1000 * 1000);
	return eaf_time_addclock(dst, &tmp);
}
