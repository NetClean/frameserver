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
	std::vector<std::tuple<std::string, std::string>> args;
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

	void SendArguments(Plugin& plugin){
		FlogExpD(Str(plugin.args.size()));
		plugin.messageQueue->WriteMessage("arguments", Str(plugin.args.size()));
		for(auto& arg : plugin.args){
			FlogExpD(std::get<0>(arg));
			FlogExpD(std::get<1>(arg));
			plugin.messageQueue->WriteMessage(std::get<0>(arg), std::get<1>(arg));
		}
	}

	void StartSession(const std::string& shmName, PlatformPtr platform){
		for(auto& plugin : plugins){
			try { 
				std::string messageQueueName = UuidGenerator::Create()->GenerateUuid(RandChar::Create());
				FlogExpD(messageQueueName);
				plugin.messageQueue = IpcMessageQueue::Create(messageQueueName, 2, 1024 * 1024 * 10, 4, 1024 * 16);

				platform->StartProcess(plugin.executable, {messageQueueName, shmName}, plugin.directory);
				plugin.started = true;

				SendArguments(plugin);
			} catch (PlatformEx e) {
				FlogE("could not start plugin: " << plugin.executable << " because: " << e.GetMsg());
				plugin.started = false;
			}
		}
	}

	void SetArgument(const std::string& pluginName, const std::string& key, const std::string& value){
		bool found = false;
		for(auto& plugin : plugins){
			if(plugin.name == pluginName){
				plugin.args.push_back(std::tuple<std::string, std::string>{key, value});
				found = true;
			}
		}
		if(!found)
			FlogE("no such plugin enabled: " << pluginName);
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
			if(!plugin.started)
				continue;

			FlogD("signaling");
			plugin.messageQueue->WriteMessage("cmd", sigs[signal]);
		}
	}

	void WaitAndRelayResults(PlatformPtr platform, IpcMessageQueuePtr hostQueue, int timeout){
		FlogD("waiting for ready");
		for(auto& plugin : plugins){
			if(!plugin.started)
				continue;

			bool done = false;

			while(!done){
				bool ret = plugin.messageQueue->GetReadBuffer([&](const std::string& type, const char* buffer, size_t size){
					if(Tools::StartsWith(type, "results")){
						// results message, relay to host
						FlogD("relaying results from: " << plugin.executable);

						char* outBuffer = hostQueue->GetWriteBuffer();
						memcpy(outBuffer, buffer, size);
						hostQueue->ReturnWriteBuffer(Str(type << " " << plugin.name), &outBuffer, size);
					}

					else if(type == "status"){
						// TODO actually check if the message says "ready"
						// plugin is ready for next frame
						done = true;
					}

					else{
						// unexpected message
						plugin.started = false;

						if(Tools::StartsWith(type, "error")){
							FlogD("relaying error from: " << plugin.executable);
							hostQueue->WriteMessage(Str(type << " " << plugin.name), buffer);
						}else{
							FlogW("unknown message type: " << type << " from plugin: " << plugin.name);
						}
						done = true;
					}
				}, 30000);
					
				if(!ret){
					// timeout occured
					hostQueue->WriteMessage(Str("error 0 " << plugin.name), "timeout while processing frames");
					FlogW("timeout for plugin: " << plugin.name);
					plugin.started = false;
					break;
				}
			}
		}
	}
};

PluginHandlerPtr PluginHandler::Create(){
	return PluginHandlerPtr(new CPluginHandler);
}
