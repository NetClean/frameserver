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

#pragma pack(4)
struct __attribute__ ((packed)) ShmVidInfo {
	uint32_t reserved;
	uint32_t width;
	uint32_t height;
	uint32_t flags;
	int64_t bytePos;
	int64_t pts;
	int64_t dts;
	
	uint32_t totFrames;
	float fps;
	bool fpsGuessed;
};
#pragma pack() // restore packing

class CProgram : public Program {
	public:
	void RunPlugins(const std::string& videoFile, IpcMessageQueuePtr hostQueue, PluginHandlerPtr pluginHandler, PlatformPtr platform, int totFrames){
		VideoPtr video = Video::Create(videoFile);
		FramePtr frame = Video::CreateFrame(video->GetWidth(), video->GetHeight(), Video::PixelFormatRgb);

		std::string frameShmName = UuidGenerator::Create()->GenerateUuid(RandChar::Create(platform));
		SharedMemPtr frameShm = SharedMem::Create(frameShmName, frame->width * frame->height * frame->bytesPerPixel + 4096);

		FlogExpD(frameShmName);
		
		volatile ShmVidInfo* info = (ShmVidInfo*)frameShm->GetPtrRw();

		info->totFrames = totFrames;

		try {
			info->fps = video->GetFrameRate();
			info->fpsGuessed = false;
		}

		catch(VideoEx)
		{
			info->fps = 29.97;
			info->fpsGuessed = true;
		}

		FlogExpD(info->fps);

		FlogD("Starting session...");
		pluginHandler->StartSession(frameShmName, platform, hostQueue);

		int nFrames = 0;

		while(video->GetFrame(frame->width, frame->height, Video::PixelFormatRgb, frame)){
			pluginHandler->ProcessMessages(platform, hostQueue, true);

			info->width = frame->width;
			info->height = frame->height;
			info->flags = frame->flags;
			info->bytePos = frame->bytePos;
			info->dts = frame->dts;
			info->pts = frame->pts; 

			memcpy((void*)((char*)frameShm->GetPtrRw() + 4096), frame->buffer, frame->width * frame->height * frame->bytesPerPixel);

			pluginHandler->Signal(PluginHandler::SignalNewFrame);

			nFrames++;
		}
		
		FlogD("process last messages");
		pluginHandler->Signal(PluginHandler::SignalQuit);
		pluginHandler->ProcessMessages(platform, hostQueue, false);

		pluginHandler->EndSession();

		hostQueue->WriteMessage("done", "");

		FlogD("decoded: " << nFrames << " frames...");
	}

	int Run(std::string shmName){
		PlatformPtr platform;

		try { 
	 		platform = Platform::Create();
			bool done = false;
			IpcMessageQueuePtr hostQueue = IpcMessageQueue::Open(shmName);
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
						FlogE("failed to run plugins: " << ex.GetMsg());
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
			#ifdef DEBUG
			platform->Sleep(5 * 1000);
			#endif
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
