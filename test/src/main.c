#include "TEST.h"

int main(int argc, char* argv[])
{
	int ret = test_run_tests(argc, argv);

#ifdef _MSC_VER
	system("pause");
#endif

	return ret;
}
