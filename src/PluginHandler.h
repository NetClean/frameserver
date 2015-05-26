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
		SignalEndSession,
		SignalQuit
	};

	virtual void AddPlugin(const std::string& name, const std::string& executable, const std::string& directory, bool debug = false) = 0;
	virtual void StartSession(const std::string& shmName, PlatformPtr platform, IpcMessageQueuePtr hostQueue) = 0;
	virtual void EndSession(int timeout = 10000) = 0;
	virtual void Signal(SignalType signal) = 0;
	virtual void ProcessMessages(PlatformPtr platform, IpcMessageQueuePtr hostQueue, bool expectFinished, int timeout = 10000) = 0;
	virtual void SetArgument(const std::string& pluginName, const std::string& key, const std::string& value) = 0;
	virtual void SetDebugger(const std::string& pluginName, const std::string& debugger) = 0;
	virtual void SetDebuggerArgs(const std::string& pluginName, const std::string& debuggerArgs) = 0;
	virtual void SetStartSuspended(const std::string& pluginName, bool value) = 0;
	virtual void SetShowWindow(const std::string& pluginName, bool value) = 0;

	static PluginHandlerPtr Create();
};

#endif

