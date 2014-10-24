#ifndef PLATFORM_H
#define PLATFORM_H

#include <memory>
#include "Macros.h"
#include "Tools.h"

DefEx(PlatformEx);

class Platform;
typedef std::shared_ptr<Platform> PlatformPtr;

class Process {
	public:
	virtual bool IsRunning() = 0;
	virtual bool Wait(int msTimeout = -1) = 0;
	virtual void Kill() = 0;
	virtual void InjectDll(const std::string& path, int timeout = 10000) = 0;
	virtual void Resume() = 0;
};

typedef std::shared_ptr<Process> ProcessPtr;

class Platform {
	public:
	virtual void Sleep(int ms) = 0;
	virtual ProcessPtr StartProcess(const std::string& executable, const StrVec& args, const std::string& directory, bool startPaused = false) = 0;
	virtual std::string GetWorkingDirectory() = 0;
	virtual std::string CombinePath(const StrVec& paths) = 0;
	virtual void GetRandChars(char* buffer, int len) = 0;

	static PlatformPtr Create();
};

#endif
