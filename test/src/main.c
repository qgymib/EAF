#include "ctest/ctest.h"
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
 * Check whether `exp` is in argv
 * @param ret	result (bool)
 * @param argc	argc
 * @param argv	argv
 * @param exp	string
 */
#define FIND_ARG(ret, argc, argv, exp)	\
	do {\
		int i = 0;\
		for (ret = 0; i < argc; i++) {\
			if (strstr(argv[i], exp) != NULL) {\
				ret = 1;\
				break;\
			}\
		}\
	} while (0)

int main(int argc, char* argv[])
{
#if defined(_MSC_VER)
	int flag_leack_check = 0;
	FIND_ARG(flag_leack_check, argc, argv, "ctest_check_leak");

	if (flag_leack_check)
	{
		_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
		_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
		_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
		_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
		_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
		_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);
	}
#endif

	int ret = ctest_run_tests(argc, argv, NULL);

#if defined(_MSC_VER)
	if (flag_leack_check)
	{
		_CrtDumpMemoryLeaks();
	}

	int flag_pause_on_exit;
	FIND_ARG(flag_pause_on_exit, argc, argv, "ctest_pause_on_exit");
	if (flag_pause_on_exit)
	{
		system("pause");
	}
#endif

	return ret;
}
