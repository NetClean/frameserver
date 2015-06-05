#include "Program.h"
#include <iostream>
#include <string>
#include "Tools.h"

int main(int argc, char** argv)
{
	if(argc != 2){
		std::cout << "usage: " << argv[0] << " [shmname]" << std::endl;
		return 1;
	}

	return Program::Create()->Run(argv[1]);
}
