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

class CProgram : public Program {
	public:

	int Run(int argc, char** argv){
		FlogAssert(argc == 2, "usage: " << argv[0] << " [filename]");

		try { 
			VideoPtr video = Video::Create(argv[1]);
			FramePtr frame = Video::CreateFrame(video->GetWidth(), video->GetHeight(), Video::PixelFormatRgb);

			std::string frameShmName = UuidGenerator::Create()->GenerateUuid(RandChar::Create());
			SharedMemPtr frameShm = SharedMem::Create(frameShmName, frame->width * frame->height * frame->bytesPerPixel + 4096);

			PluginHandlerPtr pluginHandler = PluginHandler::Create();
			PlatformPtr platform = Platform::Create();

			std::string dir = platform->GetWorkingDirectory();
			pluginHandler->AddPlugin("test.exe", platform->CombinePath({dir, "VideoPlugins"}));

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

			FlogD("decoded: " << nFrames << " frames");

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
