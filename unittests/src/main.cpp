#include <flog.h>
#include <vector>

#include "Tests.h"

int main(int argc, char** argv)
{
	std::vector<ITest*> tests;

	tests.push_back(new Test_IpcMessageQueue_SendRecvMessage());
	tests.push_back(new Test_SharedMem_ReadWrite());

	int ret = 0;
	for(auto test : tests){
		try {
			test->RunTest();
			FlogI(test->GetSection() << "/" << test->GetSubsection() << ": ok");
		} catch (TestFailedEx ex) {
			FlogE(ex.section << "/" << ex.subsection << ": failed, '" << ex.msg << "'");
			ret = 1;
		}
	}

	return ret;
}
