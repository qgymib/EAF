#include "TEST.h"

class MsvcConsoleGuard
{
public:
	MsvcConsoleGuard(){}
	virtual ~MsvcConsoleGuard()
	{
		system("pause");
	}
};

int main(int argc, char* argv[])
{
#ifdef _MSC_VER
	MsvcConsoleGuard s_console_guard;
#endif

	return test_run_tests(argc, argv);
}
