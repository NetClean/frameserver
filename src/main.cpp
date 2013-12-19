#include "Program.h"
#include <iostream>
#include <string>
#include "Tools.h"

#ifdef DEBUG
int main(int argc, char** argv)
{
	if(argc != 2){
		std::cout << "usage: " << argv[0] << " [shmname]" << std::endl;
		return 1;
	}

	return Program::Create()->Run(argv[1]);
}
#else

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	return Program::Create()->Run(Tools::Split(lpCmdLine).back());
}
#endif
