#include "etest/etest.h"

int main(int argc, char* argv[])
{
	malloc(1);
	int ret = test_run_tests(argc, argv);

#if defined(_MSC_VER)
	_CrtDumpMemoryLeaks();
#endif

	return ret;
}
