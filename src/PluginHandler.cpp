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
	ProcessPtr process;
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

	void StartSession(const std::string& shmName, PlatformPtr platform, IpcMessageQueuePtr hostQueue){
		for(auto& plugin : plugins){
			try { 
				std::string messageQueueName = UuidGenerator::Create()->GenerateUuid(RandChar::Create(platform));
				FlogExpD(messageQueueName);
				plugin.messageQueue = IpcMessageQueue::Create(messageQueueName, 2, 1024 * 1024 * 32, 4, 1024 * 16);

				//std::string gdb = "z:\\opt\\toolchains\\mingw32-dwarf-posix\\bin\\gdb.exe";

				//plugin.process = platform->StartProcess(gdb, {"--args", plugin.executable, messageQueueName, shmName}, plugin.directory);
				plugin.process = platform->StartProcess(plugin.executable, {messageQueueName, shmName}, plugin.directory, true);
				plugin.process->InjectDll(platform->CombinePath({platform->GetWorkingDirectory(), "plugins", "videosdk.dll"}));
				plugin.process->Resume();
				plugin.started = true;

				SendArguments(plugin);
			} catch (PlatformEx e) {
				FlogE("could not start plugin: " << plugin.executable << " because: " << e.GetMsg());
				plugin.started = false;
				hostQueue->WriteMessage(Str("error -1 " << plugin.name), "could not start process");
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

	void EndSession(int timeout){
		for(auto& plugin : plugins){
			// Wait [timeout] ms for processes to exit. If not, kill it.
			if(plugin.process->Wait(timeout)){
				plugin.process->Kill();
			}
			
			plugin.started = false;
			plugin.messageQueue = 0;
		}
	}

	void Signal(SignalType signal){
		// TODO timeout
		static std::string sigs[] = {"newframe", "quit"};
		for(auto plugin : plugins){
			if(!plugin.started || !plugin.process->IsRunning())
				continue;

			//FlogD("signaling");
			plugin.messageQueue->WriteMessage("cmd", sigs[signal]);
		}
	}

	void ProcessMessages(PlatformPtr platform, IpcMessageQueuePtr hostQueue, bool waitReady, int timeout){
		for(auto& plugin : plugins){
			if(!plugin.started)
				continue;
		
			bool done = false;

			while(!done){
				bool ret = false;
				bool isRunning = true; // = false;
				const int period = 100;
				int deadSincePeriods = 0;
				
				for(int i = 0; i < timeout / period; i++){
					if(i > 10)
						FlogW("waiting for slow plugin: " << plugin.executable);

					ret = plugin.messageQueue->GetReadBuffer([&](const std::string& type, const char* buffer, size_t size){
						if(Tools::StartsWith(type, "results")){
							// results message, relay to host
							FlogD("relaying results from: " << plugin.executable);

							if(size > 0){
								FlogD("message starts with: " << (unsigned)buffer[0]);
							}

							if(hostQueue->GetWriteQueueSize() >= (int)size){
								char* outBuffer = hostQueue->GetWriteBuffer();
								memcpy(outBuffer, buffer, size);
								hostQueue->ReturnWriteBuffer(Str(type << " " << plugin.name), &outBuffer, size);
							}else{
								hostQueue->WriteMessage(Str("error 0 " << plugin.name), "result too big for frameserver/host message queue");
								FlogW("result too big for frameserver/host message queue (" << size << "), plugin: " << plugin.executable << " " << plugin.name);
							}
						}

						else if(type == "status"){
							// plugin is ready for next frame
							// TODO actually check if the message says "ready"
							if(waitReady)
								done = true;
						}

						else if(Tools::StartsWith(type, "error")){
							// error
							FlogD("relaying error from: " << plugin.executable);
							done = true;
							plugin.started = false;
							hostQueue->WriteMessage(Str(type << " " << plugin.name), buffer);
						}
						
						else{
							// unknown
							FlogW("unknown message type: " << type << " from plugin: " << plugin.name);
							done = true;
							plugin.started = false;
						}
					}, period);

					if(!ret){
						// The deadSincePeriods counter exists to prevent a race condition where the frameserver queue read times out
						// because the plugin has acquired a buffer for writing a result. If the plugin then manages to terminate
						// before the process->IsRunning check in the frameserver, the reported result (or other message) would be ignored.

						// If the process has been dead the last 10 periods (1 is probably enough) we can safely assume that there are no
						// result messages in the queue waiting for us, and since the plugin process is dead, we can exit the loop.

						deadSincePeriods += plugin.process->IsRunning() ? 0 : 1;
						isRunning = deadSincePeriods < 10;

						if(!isRunning){
							break;
						}

						// message queue is empty and we don't wait for ready state -> done and break
						if(!waitReady){
							done = true;
							break;
						}
					}

					// success -> exit retry loop
					if(ret)
						break;
				}

				if(!isRunning){
					// process exited
					hostQueue->WriteMessage(Str("error 0 " << plugin.name), "process exited");
					FlogW("process exited, plugin: " << plugin.executable << " " << plugin.name);

					plugin.started = false;
					break;
				}

				if(!done && !ret){
					// timeout occured
					hostQueue->WriteMessage(Str("error 0 " << plugin.name), "timeout while processing frames");
					FlogW("timeout, plugin: " << plugin.executable << " " << plugin.name);

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
