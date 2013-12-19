#ifndef PROGRAM_H
#define PROGRAM_H

#include <memory>
#include <string>

class Program;
typedef std::shared_ptr<Program> ProgramPtr; 

class Program 
{
	public:
	virtual int Run(std::string shmName) = 0;
	static ProgramPtr Create();
};

#endif
