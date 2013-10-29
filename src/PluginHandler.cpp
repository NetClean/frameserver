#include "PluginHandler.h"

#include <vector>

#include "IpcMessageQueue.h"
#include "UuidGenerator.h"
#include "RandChar.h"
#include "flog.h"

struct Plugin
{
	std::string name;
	std::string executable;
	std::string directory;
	IpcMessageQueuePtr messageQueue;
	bool started;
};

class CPluginHandler : public PluginHandler 
{
	public:
	std::vector<Plugin> plugins;

	void AddPlugin(const std::string& name, const std::string& executable, const std::string& directory){
		Plugin plugin;

		plugin.name = name;
		plugin.executable = executable;
		plugin.directory = directory;

		plugins.push_back(plugin);
	}

	void StartSession(const std::string& shmName, PlatformPtr platform){
		for(auto& plugin : plugins){
			try { 
				std::string messageQueueName = UuidGenerator::Create()->GenerateUuid(RandChar::Create());
				FlogExpD(messageQueueName);
				plugin.messageQueue = IpcMessageQueue::Create(messageQueueName, true);

				platform->StartProcess(plugin.executable, {messageQueueName, shmName}, plugin.directory);
				plugin.started = true;
			} catch (PlatformEx e) {
				FlogE("could not start plugin: " << plugin.executable << " because: " << e.GetMsg());
				plugin.started = false;
			}
		}
	}

	void EndSession(){
		for(auto& plugin : plugins){
			plugin.started = false;
			plugin.messageQueue = 0;
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
		FlogD("waiting for ready");
		for(auto plugin : plugins){
			std::string type, message;

			plugin.messageQueue->ReadMessage(type, message);
			AssertEx(type == "status", PluginHandlerEx, "unexpected message type: " << type);
			AssertEx(message == "ready", PluginHandlerEx, "unexpected message: '" << message << "'");
		}
	}

	void RelayResults(PlatformPtr platform, IpcMessageQueuePtr hostQueue){
		FlogD("waiting for results");
		for(auto plugin : plugins){
			std::string type;
			const char* buffer;
			size_t size;

			plugin.messageQueue->GetReadBuffer(type, &buffer, &size);
			
			AssertEx(type == "results", PluginHandlerEx, "expected message 'results', but got: " << type);
			FlogD("relaying results from: " << plugin.executable);

			char* outBuffer = hostQueue->GetWriteBuffer();
			memcpy(outBuffer, buffer, size);
			hostQueue->ReturnWriteBuffer(Str("results " << plugin.name), &outBuffer, size);
		}
	}
};

PluginHandlerPtr PluginHandler::Create(){
	return PluginHandlerPtr(new CPluginHandler);
}
