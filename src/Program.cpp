#include "Program.h"
#include <vector>

class CProgram : public Program {
	public:

	int Run(int argc, char** argv){
		return 0;
	}
};

ProgramPtr Program::Create()
{
	return ProgramPtr(new CProgram);
}
