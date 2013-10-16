#include "PluginHandler.h"

#include <vector>

#include "IpcMessageQueue.h"
#include "UuidGenerator.h"
#include "RandChar.h"
#include "flog.h"

struct Plugin
{
	std::string executable;
	std::string directory;
	IpcMessageQueuePtr messageQueue;
	std::string messageQueueName;
	bool started;
};

class CPluginHandler : public PluginHandler 
{
	public:
	std::vector<Plugin> plugins;

	void AddPlugin(const std::string& executable, const std::string& directory){
		Plugin plugin;

		plugin.executable = executable;
		plugin.directory = directory;
		plugin.messageQueueName = UuidGenerator::Create()->GenerateUuid(RandChar::Create());
		FlogExpD(plugin.messageQueueName);
		plugin.messageQueue = IpcMessageQueue::Create(plugin.messageQueueName, true);

		plugins.push_back(plugin);
	}

	void StartSession(const std::string& shmName, PlatformPtr platform){
		for(auto& plugin : plugins){
			try { 
				platform->StartProcess(plugin.executable, {plugin.messageQueueName}, plugin.directory);
				plugin.started = true;
			} catch (PlatformEx e) {
				FlogE("could not start plugin: " << plugin.executable << " because: " << e.GetMsg());
				plugin.started = false;
			}
		}
	}

	void Signal(SignalType signal){
		// TODO timeout
		static std::string sigs[] = {"newframe", "quit"};
		for(auto plugin : plugins){
			FlogD("signaling");
			plugin.messageQueue->WriteMessage("cmd", sigs[signal]);
		}
	}

	void WaitReady(PlatformPtr platform){
		for(auto plugin : plugins){
			std::string type, message;

			plugin.messageQueue->ReadMessage(type, message);
			AssertEx(type == "status", PluginHandlerEx, "unexpected message type: " << type);
			AssertEx(message == "ready", PluginHandlerEx, "unexpected message: '" << message << "'");
		}
	}
};

PluginHandlerPtr PluginHandler::Create(){
	return PluginHandlerPtr(new CPluginHandler);
}
