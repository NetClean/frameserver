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
	void RunPlugins(const std::string& videoFile, IpcMessageQueuePtr hostQueue, PluginHandlerPtr pluginHandler, PlatformPtr platform, int totFrames){
		VideoPtr video = Video::Create(videoFile);
		FramePtr frame = Video::CreateFrame(video->GetWidth(), video->GetHeight(), Video::PixelFormatRgb);

		std::string frameShmName = UuidGenerator::Create()->GenerateUuid(RandChar::Create());
		SharedMemPtr frameShm = SharedMem::Create(frameShmName, frame->width * frame->height * frame->bytesPerPixel + 4096);
		
		*((uint32_t*)frameShm->GetPtrRw() + 2) = totFrames;

		pluginHandler->StartSession(frameShmName, platform, hostQueue);

		int nFrames = 0;

		while(video->GetFrame(frame->width, frame->height, Video::PixelFormatRgb, frame)){
			pluginHandler->ProcessMessages(platform, hostQueue, true);

			*((uint32_t*)frameShm->GetPtrRw()) = frame->width;
			*((uint32_t*)frameShm->GetPtrRw() + 1) = frame->height;
			memcpy((void*)((char*)frameShm->GetPtrRw() + 4096), frame->buffer, frame->width * frame->height * frame->bytesPerPixel);

			pluginHandler->Signal(PluginHandler::SignalNewFrame);

			nFrames++;
		}
		
		pluginHandler->Signal(PluginHandler::SignalQuit);
		pluginHandler->ProcessMessages(platform, hostQueue, false);

		pluginHandler->EndSession();

		hostQueue->WriteMessage("done", "");

		FlogD("decoded: " << nFrames << " frames...");
	}

	int Run(int argc, char** argv){
		FlogAssert(argc == 2, "usage: " << argv[0] << " [shm name]");
		PlatformPtr platform;

		try { 
	 		platform = Platform::Create();
			bool done = false;
			IpcMessageQueuePtr hostQueue = IpcMessageQueue::Open(argv[1]);
			PluginHandlerPtr pluginHandler = PluginHandler::Create();

			while(!done){
				std::string type, message;
				FlogD("waiting for message");
				hostQueue->ReadMessage(type, message);
				FlogExpD(type);

				if(type == "run"){
					try {
						int nFrames = Video::CountFramesInFile(message);
						RunPlugins(message, hostQueue, pluginHandler, platform, nFrames);
						FlogExpD(nFrames);
					} catch (VideoEx ex) {
						hostQueue->WriteMessage("video_error", ex.GetMsg());
					}
				}

				else if(type == "enable"){
					auto vec = Tools::Split(message);
					if(vec.size() == 2){
						std::string dir = platform->CombinePath({platform->GetWorkingDirectory(), "plugins"});
						pluginHandler->AddPlugin(vec[0], platform->CombinePath({dir, vec[1], Str(vec[1] << ".exe")}), dir);
					}else{
						FlogE("enable expects 2 arguments (handle and plugin name)");
					}
				}

				else if(Tools::StartsWith(type, "argument")){
					auto vec = Tools::Split(type, '/');
					if(vec.size() == 3){
						FlogExpD(vec[0]);
						FlogExpD(vec[1]);
						FlogExpD(vec[2]);
						pluginHandler->SetArgument(vec[1], vec[2], message);
					}else{
						FlogE("argument expects argument/[plugin name]/[key] (literally slashes)");
					}
				}

				else if(type == "exit")
					done = true;

				else
					FlogE("unknown command: " << type);
			}
		}

		catch (ExBase ex) {
			FlogF("unhandled exception: " << ex.GetMsg());
			platform->Sleep(5000);
			return 1;
		}

		FlogD("bye bye");

		return 0;
	}
};

ProgramPtr Program::Create()
{
	return ProgramPtr(new CProgram);
}
