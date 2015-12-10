#define WIN32_LEAN_AND_MEAN
#include <stdlib.h>
#include <windows.h>
#include "Program.h"
#include <iostream>
#include <string>
#include "Tools.h"
#include "crthack.h"

int main(int argc, char** argv)
{
	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
	// Suppress the abort message
	//_set_abort_behavior(0, _WRITE_ABORT_MSG);

	if(argc != 2){
		std::cout << "usage: " << argv[0] << " [shmname]" << std::endl;
		return 1;
	}

	// msvcrt shows a messagebox on abort() and waits indefinitely for user input, causing the program to never return
	// instead of just failing.

	// This can be remedied in newer versions of msvcrt with _set_abort_behavior().
	// The new msvcrt versions are however incompatible with mingw (at least c++), and cause
	// seemingly random crashes.

	// This hack instead patches MessageBoxA to simply return when called (if the caption isn't prepended with a '!'),
	// suppressing the error window and continuing with the abort() without user input.

	PatchMessageBox();

	return Program::Create()->Run(argv[1]);
}
