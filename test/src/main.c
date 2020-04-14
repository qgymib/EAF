#include "etest/etest.h"

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

	return ret;
}
