#ifndef PLUGINHANDLER_H
#define PLUGINHANDLER_H

#include <memory>
#include <string>

#include "Platform.h"
#include "IpcMessageQueue.h"

class PluginHandler;
typedef std::shared_ptr<PluginHandler> PluginHandlerPtr; 

DefEx(PluginHandlerEx);

class PluginHandler
{
	public:
	enum SignalType {
		SignalNewFrame,
		SignalQuit
	};

	virtual void AddPlugin(const std::string& name, const std::string& executable, const std::string& directory) = 0;
	virtual void StartSession(const std::string& shmName, PlatformPtr platform) = 0;
	virtual void EndSession() = 0;
	virtual void Signal(SignalType signal) = 0;
	virtual void WaitAndRelayResults(PlatformPtr platform, IpcMessageQueuePtr hostQueue, int timeout = 1000) = 0;
	virtual void SetArgument(const std::string& pluginName, const std::string& key, const std::string& value) = 0;

	static PluginHandlerPtr Create();
};

#endif

