#include "Program.h"
#include <vector>
#include <cstdlib>

#include "flog.h"

#include "PluginHandler.h"
#include "Platform.h"
#include "Video.h"
#include "SharedMem.h"
#include "UuidGenerator.h"
#include "RandChar.h"
#include "IpcMessageQueue.h"

class CProgram : public Program {
	public:

	void RunPlugins(const std::string& videoFile, IpcMessageQueuePtr hostQueue, PluginHandlerPtr pluginHandler, PlatformPtr platform){
		VideoPtr video = Video::Create(videoFile);
		FramePtr frame = Video::CreateFrame(video->GetWidth(), video->GetHeight(), Video::PixelFormatRgb);

		std::string frameShmName = UuidGenerator::Create()->GenerateUuid(RandChar::Create());
		SharedMemPtr frameShm = SharedMem::Create(frameShmName, frame->width * frame->height * frame->bytesPerPixel + 4096);

		pluginHandler->StartSession(frameShmName, platform);

		int nFrames = 0;

		while(video->GetFrame(frame->width, frame->height, Video::PixelFormatRgb, frame)){
			*((uint32_t*)frameShm->GetPtrRw()) = frame->width;
			*((uint32_t*)frameShm->GetPtrRw() + 1) = frame->height;
			memcpy((void*)((char*)frameShm->GetPtrRw() + 4096), frame->buffer, frame->width * frame->height * frame->bytesPerPixel);

			pluginHandler->WaitReady(platform);
			pluginHandler->Signal(PluginHandler::SignalNewFrame);

			nFrames++;
		}
		
		pluginHandler->Signal(PluginHandler::SignalQuit);
		pluginHandler->WaitReady(platform);
		pluginHandler->RelayResults(platform, hostQueue);

		FlogD("decoded: " << nFrames << " frames");
	}

	int Run(int argc, char** argv){
		FlogAssert(argc == 2, "usage: " << argv[0] << " [shm name]");

		try { 
			bool done = false;
			IpcMessageQueuePtr hostQueue = IpcMessageQueue::Create(argv[1], false);
			PluginHandlerPtr pluginHandler = PluginHandler::Create();
			PlatformPtr platform = Platform::Create();

			while(!done){
				std::string type, message;
				hostQueue->ReadMessage(type, message);

				if(type == "run")
					RunPlugins(message, hostQueue, pluginHandler, platform);

				else if(type == "enable"){
					std::string dir = platform->CombinePath({platform->GetWorkingDirectory(), "plugins"});
					pluginHandler->AddPlugin(message, platform->CombinePath({dir, message, Str(message << ".exe")}), dir);
				}

				else if(type == "exit")
					done = true;

				else
					FlogE("unknown command: " << type);
			}
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
