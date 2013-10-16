#include "Program.h"
#include <vector>

#include "flog.h"

#include "PluginHandler.h"
#include "Platform.h"

class CProgram : public Program {
	public:

	int Run(int argc, char** argv){
		try { 
			PluginHandlerPtr pluginHandler = PluginHandler::Create();
			PlatformPtr platform = Platform::Create();

			std::string dir = platform->GetWorkingDirectory();
			pluginHandler->AddPlugin("test.exe", platform->CombinePath({dir, "VideoPlugins"}));

			pluginHandler->StartSession("test", platform);

			for(int i = 0; i < 100; i++){
				pluginHandler->WaitReady(platform);
				pluginHandler->Signal(PluginHandler::SignalNewFrame);
			}

			pluginHandler->Signal(PluginHandler::SignalQuit);
		}

		catch (ExBase ex) {
			FlogF("unhandled exception: " << ex.GetMsg());
			return 1;
		}

		return 0;
	}
};

ProgramPtr Program::Create()
{
	return ProgramPtr(new CProgram);
}
