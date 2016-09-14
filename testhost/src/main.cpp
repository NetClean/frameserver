#include "CommandLine.h"
#include "Platform.h"
#include "IpcMessageQueue.h"
#include "flog.h"
#include <fstream>
#include <iostream>

void writeBuffer(const std::string& filename, const char* buffer, int size)
{
	std::ofstream file(filename.c_str(), std::ios::binary);

	file.write(buffer, size);

	file.close();
}

int main(int argc, char** argv)
{
	CommandLine::Start();
	PlatformPtr platform = Platform::Create();
	IpcMessageQueuePtr messageQueue = IpcMessageQueue::Create("test", 2, 1024 * 1024 * 50, 2, 1024 * 16);

	bool done = false;

	try {
		while(!done){
			platform->Sleep(1);

			auto cmd = CommandLine::GetCommand();
			if(cmd.size() > 0){
				FlogExpD(cmd[0]);

				if(cmd[0] == "exit")
					done = true;

				messageQueue->WriteMessage(cmd[0], cmd.size() > 1 ? cmd[1] : "");
			}

			messageQueue->GetReadBuffer([&](const std::string& type, const char* buffer, size_t size){
				FlogExpD(type);
				auto sType = Tools::Split(type);
				if(sType.size() == 2 && sType[0] == "results"){
					FlogI("results from: " << sType[1] << " with size: " << size);
					if(sType[1] == "video-collage"){
						writeBuffer("collage.data", buffer, size);
					}

					if(sType[1] == "videntifier"){
						writeBuffer("description.desc72", buffer, size);
					}

					if(sType[1] == "interesting-frames"){
						for(int i = 0; i < (int)size / (int)sizeof(float); i++)
							FlogExpD(((float*)buffer)[i]);
					}
				}

				if(sType.size() == 3 && sType[0] == "error"){
					FlogI("error from: " << sType[2] << ", code: " << sType[1] << ", message: " << std::string(buffer, size));
				}

				if(type == "progress"){
					FlogI("progress: " << ((int32_t*)buffer)[0] << " " << ((int32_t*)buffer)[1]);
				}
			}, 1);
		}
	}

	catch (ExBase ex) {
		FlogF("unhandled exception: " << ex.GetMsg());
		return 1;
	}
	

	return 0;
}
