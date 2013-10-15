#include "PluginHandler.h"

#include <vector>

#include "IpcMessageQueue.h"
#include "UuidGenerator.h"
#include "RandChar.h"
#include "flog.h"

struct Plugin
{
	std::string executable;
	IpcMessageQueuePtr messageQueue;
	std::string messageQueueName;
};

class CPluginHandler : public PluginHandler 
{
	public:
	std::vector<Plugin> plugins;

	void AddPlugin(const std::string& executable, const std::string& directory){
		Plugin plugin;

		plugin.executable = executable;
		plugin.messageQueueName = UuidGenerator::Create()->GenerateUuid(RandChar::Create());
		FlogExpD(plugin.messageQueueName);
		plugin.messageQueue = IpcMessageQueue::Create(plugin.messageQueueName, true);
	}

	void StartSession(const std::string& shmName){
	}

	void Signal(SignalType signal){
	}

	void Wait(){
	}
};

PluginHandlerPtr PluginHandler::Create(){
	return PluginHandlerPtr(new CPluginHandler);
}
