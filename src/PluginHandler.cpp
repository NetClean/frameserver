#include "PluginHandler.h"

#include <vector>
#include <map>

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
	std::string debugger;
	std::string debuggerArgs;
	bool started = false;
	bool finished = false;
	bool debug = false;
	bool startSuspended = false;
	bool showWindow = false;
};

class CPluginHandler : public PluginHandler 
{
	public:
	std::vector<Plugin> plugins;

	void AddPlugin(const std::string& name, const std::string& executable, const std::string& directory, bool debug){
		Plugin plugin;

		plugin.name = name;
		plugin.executable = executable;
		plugin.directory = directory;
		plugin.debug = debug;

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

				// start with a debugger
				if(plugin.debug && plugin.debugger != ""){
					FlogD("debugging enabled for: " << plugin.executable);

					std::vector<std::string> args;

					std::map<std::string, std::string> vars = {
						{"$plugin", plugin.executable},
						{"\"$plugin\"", Str("\"" << plugin.executable << "\"")},
						{"$frame_name", shmName},
						{"$cmd_queue_name", messageQueueName},
					};

					for(auto arg : Tools::Split(plugin.debuggerArgs)){
						auto var = vars.find(arg);

						if(var != vars.end())
							args.push_back(var->second);
						else 
							args.push_back(arg);
					}

					plugin.process = platform->StartProcess(plugin.debugger, args, plugin.directory, plugin.startSuspended, plugin.showWindow);
				}

				// start without a debugger
				else {
					plugin.process = platform->StartProcess(plugin.executable, 
						{messageQueueName, shmName}, plugin.directory, plugin.startSuspended, plugin.showWindow);
				}

				if(plugin.startSuspended)
				{
					platform->WaitMessageBox("Debug", "Plugin suspended. Attach debugger and press OK to resume plugin.");
					plugin.process->Resume();
				}

				plugin.started = true;
				plugin.finished = false;

				SendArguments(plugin);
			} catch (PlatformEx e) {
				FlogE("could not start plugin: " << plugin.executable << " because: " << e.GetMsg());
				plugin.started = false;
				hostQueue->WriteMessage(Str("error -1 " << plugin.name), "could not start process");
			}
		}
	}

	std::vector<Plugin>::iterator FindPlugin(const std::string& name)
	{
		for(auto plugin = plugins.begin(); plugin != plugins.end(); plugin++){
			if(plugin->name == name){
				return plugin;
			}
		}
		
		FlogE("no plugin with name: " << name << " is enabled");

		return plugins.end();
	}

	void SetArgument(const std::string& pluginName, const std::string& key, const std::string& value){
		auto plugin = FindPlugin(pluginName);
		if(plugin != plugins.end()){
			plugin->args.push_back(std::tuple<std::string, std::string>{key, value});
		}
	}
	
	void SetDebugger(const std::string& pluginName, const std::string& debugger)
	{
		auto plugin = FindPlugin(pluginName);
		if(plugin != plugins.end()){
			plugin->debugger = debugger;
		}
	}

	void SetDebuggerArgs(const std::string& pluginName, const std::string& debuggerArgs)
	{
		auto plugin = FindPlugin(pluginName);
		if(plugin != plugins.end()){
			plugin->debuggerArgs = debuggerArgs;
		}
	}
	
	void SetStartSuspended(const std::string& pluginName, bool value)
	{
		auto plugin = FindPlugin(pluginName);
		if(plugin != plugins.end()){
			plugin->startSuspended = value;
		}
	}
	
	void SetShowWindow(const std::string& pluginName, bool value)
	{
		auto plugin = FindPlugin(pluginName);
		if(plugin != plugins.end()){
			plugin->showWindow = value;
		}
	}

	void EndSession(int timeout){
		for(auto& plugin : plugins){
			// Wait [timeout] ms for processes to exit. If not, kill it.
			if(plugin.started && plugin.process->Wait(plugin.debug ? -1 : timeout)){
				plugin.process->Kill();
			}
			
			plugin.started = false;
			plugin.messageQueue = 0;
		}
	}

	void Signal(SignalType signal){
		// TODO timeout
		static std::string sigs[] = {"newframe", "endsession", "quit"};
		for(auto plugin : plugins){
			if(!plugin.started || !plugin.process->IsRunning())
				continue;

			//FlogD("signaling");
			plugin.messageQueue->WriteMessage("cmd", sigs[signal]);
		}
	}

	void ProcessMessages(PlatformPtr platform, IpcMessageQueuePtr hostQueue, bool expectFinished, int timeout){
		for(auto& plugin : plugins){
			if(!plugin.started || plugin.finished)
				continue;
				
			if(!plugin.process->IsRunning()){
				// process exited
				int exitCode = plugin.process->GetExitCode();

				std::string msg = Str("process exited unexpectedly, plugin: " 
					<< plugin.executable << " " << plugin.name << ", exit code: " << exitCode);

				hostQueue->WriteMessage(Str("error 0 " << plugin.name), msg);
				FlogW(msg);

				plugin.started = false;
				break;
			}
		
			bool done = false;

			while(!done){
				bool ret = plugin.messageQueue->GetReadBuffer([&](const std::string& type, const char* buffer, size_t size){
					//FlogD("waiting");
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

					else if(type == "status" && std::string(buffer) == "ready"){
						// plugin is ready for next frame

						if(!expectFinished){
							done = true;
							//FlogD("ready");
						}
						else{
							FlogD("(ignoring ready)");
						}
					}

					else if(type == "status" && std::string(buffer) == "finished"){
						// plugin is finished with file/session

						if(expectFinished){
							plugin.finished = true;
							done = true;
							FlogD("finished");
						}else{
							FlogD("(ignoring finished)");
						}
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
				}, plugin.debug ? -1 : timeout);

				if(!ret){
					// timeout occured
					hostQueue->WriteMessage(Str("error 0 " << plugin.name), "timeout while processing frames/waiting for result");
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
