#ifndef PLATFORM_H
#define PLATFORM_H

#include <memory>
#include "Macros.h"
#include "Tools.h"

DefEx(PlatformEx);

class Platform;
typedef std::shared_ptr<Platform> PlatformPtr;

class Platform {
	public:
	virtual void Sleep(int ms) = 0;
	virtual void StartProcess(const std::string& executable, const StrVec& args, const std::string& directory) = 0;
	virtual std::string GetWorkingDirectory() = 0;
	virtual std::string CombinePath(const StrVec& paths) = 0;

	static PlatformPtr Create();
};

#endif
