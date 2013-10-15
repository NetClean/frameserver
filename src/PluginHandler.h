#ifndef PLUGINHANDLER_H
#define PLUGINHANDLER_H

#include <memory>
#include <string>

#include "Platform.h"

class PluginHandler;
typedef std::shared_ptr<PluginHandler> PluginHandlerPtr; 

class PluginHandler
{
	public:
	enum SignalType {
		SignalNewFrame,
		SignalFinish
	};

	virtual void AddPlugin(const std::string& executable, const std::string& directory) = 0;
	virtual void StartSession(const std::string& shmName, PlatformPtr platform) = 0;
	virtual void Signal(SignalType signal) = 0;
	virtual void Wait() = 0;

	static PluginHandlerPtr Create();
};

#endif

