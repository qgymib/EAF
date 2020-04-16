#include "etest/etest.h"

#if defined(_MSC_VER)
#	define _CRTDBG_MAP_ALLOC
#	include <stdlib.h>
#	include <crtdbg.h>
#	include <string.h>
#else
#	include <stdlib.h>
#endif

#if defined(_MSC_VER)
#	pragma comment(lib, "ws2_32.lib")
#	pragma comment(lib, "IPHLPAPI.lib")
#	pragma comment(lib, "Psapi.lib")
#	pragma comment(lib, "Userenv.lib")
#endif

int main(int argc, char* argv[])
{
#if defined(_MSC_VER)
	int flag_need_crt_leack_check = 0;
	int i;
	for (i = 0; i < argc; i++)
	{
		if (strstr(argv[i], "check-leak") != NULL)
		{
			flag_need_crt_leack_check = 1;
			break;
		}
	}

	if (flag_need_crt_leack_check)
	{
		_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
		_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
		_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
		_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
		_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
		_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);
	}
#endif

	int ret = etest_run_tests(argc, argv);

#if defined(_MSC_VER)
	if (flag_need_crt_leack_check)
	{
		_CrtDumpMemoryLeaks();
	}
#if _MSC_VER < 1900
	system("pause");
#endif
#endif

	return ret;
}
