#include <string.h>
#include <strings.h>
#include "ctest/ctest.h"
#include "eaf/powerpack/log.h"
#include "quick2.h"
#if defined(_MSC_VER)
#	define _CRTDBG_MAP_ALLOC
#	include <stdlib.h>
#	include <crtdbg.h>
#	include <string.h>
#else
#	include <stdlib.h>
#endif

/*
* Before Visual Studio 2015, there is a bug that a `do { } while (0)` will triger C4127 warning
* https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4127
*/
#if defined(_MSC_VER) && _MSC_VER < 1900
#	pragma warning(disable : 4127)
#endif

/**
 * @brief Parse command line argument
 * @param[in] argc		The number of arguments
 * @param[in] argv		Argument list
 * @param[in] opt		The argument to be parsed
 * @param[in] val		Does it need a value
 * @param[in] buffer	A buffer to store value
 * @param[in] size		The size of the buffer
 * @return				<0: option not found;
 *						0: option found and not value given;
 *						>0: the length of value should be written
 */
static int ctest_parse_option(int argc, char* argv[], const char* opt, int val, char* buffer, size_t size)
{
	int idx;
	size_t opt_len = strlen(opt);
	for (idx = 0; idx < argc; idx++)
	{
		if (strncmp(argv[idx], opt, opt_len) != 0)
		{
			continue;
		}

		goto parse_opt;
	}

	return -1;

parse_opt:
	if (!val)
	{
		return 0;
	}

	if (strlen(argv[idx]) == opt_len)
	{
		goto parse_next;
	}

	return snprintf(buffer, size, "%s", argv[idx] + opt_len + 1);

parse_next:
	if (++idx > argc)
	{
		return 0;
	}

	if (*argv[idx] != '-')
	{
		return snprintf(buffer, size, "%s", argv[idx]);
	}
	return 0;
}

int main(int argc, char* argv[])
{
#if defined(_MSC_VER)
	int flag_leack_check;
	if ((flag_leack_check = ctest_parse_option(argc, argv, "--ctest_check_leak", 0, NULL, 0)) == 0)
	{
		_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
		_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
		_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
		_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
		_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
		_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);
	}
#endif

	if (ctest_parse_option(argc, argv, "--log_level", 1, quick_buffer, sizeof(quick_buffer)) > 0)
	{
		if (strcasecmp(quick_buffer, "trace") == 0)
		{
			quick_config.log_level = eaf_log_level_trace;
		}
		else if (strcasecmp(quick_buffer, "debug") == 0)
		{
			quick_config.log_level = eaf_log_level_debug;
		}
		else if (strcasecmp(quick_buffer, "info") == 0)
		{
			quick_config.log_level = eaf_log_level_info;
		}
	}
	else
	{
		quick_config.log_level = eaf_log_level_error;
	}

	int ret = ctest_run_tests(argc, argv, &quick_hook);

#if defined(_MSC_VER)
	if (flag_leack_check == 0)
	{
		_CrtDumpMemoryLeaks();
	}

	if (ctest_parse_option(argc, argv, "--ctest_pause_on_exit", 0, NULL, 0) == 0)
	{
		system("pause");
	}
#endif

	return ret;
}
