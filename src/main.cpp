#define WIN32_LEAN_AND_MEAN
#include <stdlib.h>
#include <windows.h>
#include "Program.h"
#include <iostream>
#include <string>
#include "Tools.h"

int main(int argc, char** argv)
{
	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
	// Suppress the abort message
	_set_abort_behavior(0, _WRITE_ABORT_MSG);

	if(argc != 2){
		std::cout << "usage: " << argv[0] << " [shmname]" << std::endl;
		return 1;
	}

	return Program::Create()->Run(argv[1]);
}
