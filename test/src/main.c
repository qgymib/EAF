#include "etest/etest.h"

#if defined(_MSC_VER)
#	define _CRTDBG_MAP_ALLOC
#	include <stdlib.h>
#	include <crtdbg.h>
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
	// Send all reports to STDOUT
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);
#endif

	int ret = etest_run_tests(argc, argv);

#if defined(_MSC_VER)
	_CrtDumpMemoryLeaks();
#endif

#if defined(_MSC_VER) && _MSC_VER < 1900
//	system("pause");
#endif

	return ret;
}
