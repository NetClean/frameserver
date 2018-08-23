#include "Program.h"
#include <vector>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cinttypes>
#include <algorithm>

#include "flog.h"

#include "PluginHandler.h"
#include "Platform.h"
#include "Video.h"
#include "SharedMem.h"
#include "UuidGenerator.h"
#include "RandChar.h"
#include "IpcMessageQueue.h"
#include "PriorityQueue.h"
#include "SampleBuffer.h"

#define API_VERSION_MAJOR 1
#define API_VERSION_MINOR 0
#define API_VERSION_PATCH 0

extern "C" {
#pragma pack(4)
struct __attribute__ ((packed)) ShmVidInfo {
	uint32_t reserved;
	uint32_t width;
	uint32_t height;
	uint32_t flags;
	int64_t byte_pos;
	int64_t pts;
	int64_t dts;
	
	uint32_t tot_frames;
	float fps;
	char fps_guessed;

	double pts_seconds;
	double dts_seconds;

	char has_audio;
	int32_t orig_sample_rate;
	int32_t channels;
	int32_t num_samples;
	char sample_format_str[16];
	char has_video;

	int32_t api_version_major;
	int32_t api_version_minor;
	int32_t api_version_patch;
};
#pragma pack() // restore packing
}

#define NCV_HEADER_SIZE 4096
#define NCV_PADDING_SIZE 4096

typedef PriorityQueue<SampleBufferPtr, CompareSampleBuffers> SampleBufferQueue;

class CProgram : public Program {
	public:
	int64_t totSamples = 0;
	void RunPlugins(const std::string& videoFile, IpcMessageQueuePtr hostQueue, 
		PluginHandlerPtr pluginHandler, PlatformPtr platform, int totFrames)
	{
		VideoPtr video = Video::Create(videoFile);
		FramePtr frame = Video::CreateFrame(video->GetWidth(), video->GetHeight(), Video::PixelFormatRgb32);

		int sampleRate = 44100;
		int channels = video->GetChannels();
		int sampleSize = sizeof(float) * channels;

		PriorityQueue<SampleBufferPtr, CompareSampleBuffers> sampleBuffers;

		int frameBufferSize = frame->width * frame->height * frame->bytesPerPixel;
		int maxSamplesPerFrame = sampleRate * 8; // 8 seconds
		int maxAudioBufferSizePerFrame = maxSamplesPerFrame * sampleSize;

		std::string frameShmName = UuidGenerator::Create()->GenerateUuid(RandChar::Create(platform));

		// shared memory area layout:
		//   4096 byte reserved for header
		//   frame
		//   4096 padding as a workaround for a bug in swscale (?) where it overreads the buffer
		//   audio buffer

		SharedMemPtr frameShm = SharedMem::Create(frameShmName, NCV_HEADER_SIZE + frameBufferSize + NCV_PADDING_SIZE + maxAudioBufferSizePerFrame);

		FlogExpD(frameShmName);
		
		volatile ShmVidInfo* info = (ShmVidInfo*)frameShm->GetPtrRw();

		info->tot_frames = totFrames;

		info->api_version_major = API_VERSION_MAJOR;
		info->api_version_minor = API_VERSION_MINOR;
		info->api_version_patch = API_VERSION_PATCH;

		try {
			info->fps = video->GetFrameRate();
			info->fps_guessed = 0;
		}

		catch(VideoEx)
		{
			info->fps = 29.97;
			info->fps_guessed = 1;
		}

		int nFrames = 0;

		// audio callback lambda, may be called any number of times during video::GetFrame()
		OnAudioFn audioFn = [&](const void* samples, int nSamples, double ts)
		{
			// create a sample buffer and add it to the - by timestamp sorted - priority queue
			sampleBuffers.push(SampleBuffer::Create(ts, nSamples, channels, (float*)samples));
		};

		info->has_audio = video->AudioPresent() ? 1 : 0;

		if(info->has_audio){
			info->orig_sample_rate = video->GetSampleRate();
			info->channels = video->GetChannels();
			info->num_samples = 0;

			std::string fmt = video->GetSampleFormatStr();
			strncpy((char*)info->sample_format_str, fmt.c_str(), 16);
		
			// leave some space for maxSamplesPerFrame, since libvx might overrun the buffer a bit
			// for large audio frames
			video->SetAudioParameters(sampleRate, channels, Video::SampleFormatFlt, audioFn, maxSamplesPerFrame - 16384);
		}

		FlogD("Starting session...");
		pluginHandler->StartSession(frameShmName, platform, hostQueue);

		double reportPts = .0;
		double lastPts = .0;
		double fps = info->fps;

		try 
		{
			while(video->GetFrame(frame->width, frame->height, Video::PixelFormatRgb32, frame))
			{
				// report progress to host
				int32_t* progress = (int32_t*)hostQueue->GetWriteBuffer();
				progress[0] = nFrames;
				progress[1] = totFrames;
				hostQueue->ReturnWriteBuffer("progress", (char**)&progress, sizeof(int32_t) * 2);

				// process plugin messages
				pluginHandler->ProcessMessages(platform, hostQueue, false);
					
				float* frameAudioBuffer = (float*)((char*)frameShm->GetPtrRw() + NCV_HEADER_SIZE + frameBufferSize + NCV_PADDING_SIZE);

				if(frame->deferred)
				{
					// video deferred
					
					// prepare frame and signal plugins
					info->flags = frame->flags;
					info->byte_pos = frame->bytePos;
					info->dts = frame->dts;
					info->pts = frame->pts; 
					info->dts_seconds = video->TimeStampToSeconds(frame->dts);
					info->pts_seconds = reportPts;
					info->has_video = 0;
					
					// copy all available audio to the non-video frame
					info->num_samples = CopyAudio(sampleBuffers, frameAudioBuffer, maxSamplesPerFrame, channels, nullptr);
				}
				else
				{
					// normal video-frame

					// calculate pts
					double pts = video->TimeStampToSeconds(frame->pts);

					if(nFrames == 0){
						// first frame, make sure it will get a reported pts of 0
						lastPts = pts;
					}

					double delta = pts - lastPts;

					if(delta < .0 || delta > 2.0)
					{
						// if the next timestamp is backwards in time or there's a too 
						// large gap between the next timestamp and this, just increase reported
						// pts with frame rate
						FlogW("pts delta too large or negative, adjusting from: " << delta << " to: " << 1.0 / fps);
						delta = 1.0 / fps;
					}

					reportPts += delta;
					lastPts = pts;

					info->num_samples = CopyAudio(sampleBuffers, frameAudioBuffer, maxSamplesPerFrame, channels, &pts);

					memcpy((void*)((char*)frameShm->GetPtrRw() + NCV_HEADER_SIZE), frame->buffer, frameBufferSize);
					
					// prepare frame
					info->width = frame->width;
					info->height = frame->height;
					info->flags = frame->flags;
					info->byte_pos = frame->bytePos;
					info->dts = frame->dts;
					info->pts = frame->pts; 
					info->dts_seconds = video->TimeStampToSeconds(frame->dts);
					info->pts_seconds = reportPts;
					info->has_video = 1;
					
					nFrames++;
				}
					
				// signal plugins
				pluginHandler->Signal(PluginHandler::SignalNewFrame);
			}
		}

		catch(VideoEx ex)
		{
			FlogW(ex.GetMsg());
		}
		
		if(nFrames == 0)
			throw VideoEx("no frames in video");
		
		FlogD("end session and process last messages");
		pluginHandler->Signal(PluginHandler::SignalEndSession);
		pluginHandler->ProcessMessages(platform, hostQueue, true);

		pluginHandler->Signal(PluginHandler::SignalQuit);
		pluginHandler->EndSession();

		hostQueue->WriteMessage("done", "");

		FlogD("decoded: " << nFrames << " frames...");
	}

	int32_t CopyAudio(SampleBufferQueue& sampleBuffers, float* dst, int maxSamples, int channels, double* pts)
	{
		int32_t numSamples = 0;

		while(!sampleBuffers.empty() && (pts == nullptr || sampleBuffers.top()->pts <= *pts))
		{
			SampleBufferPtr buffer = sampleBuffers.top();

			int nBufferSamples = (int)buffer->samples.size() / channels;

			if(numSamples + nBufferSamples < maxSamples){
				std::copy(buffer->samples.begin(), buffer->samples.end(), dst + numSamples * channels);
				numSamples += nBufferSamples;
				totSamples += nBufferSamples;
			}
			else{
				FlogW("sample buffer overrun");
			}

			sampleBuffers.pop();
		}

		return numSamples;
	}

	int Run(std::string shmName)
	{
		PlatformPtr platform;

		try { 
	 		platform = Platform::Create();
			bool done = false;
			bool onFrameCountMessages = false;

			IpcMessageQueuePtr hostQueue = IpcMessageQueue::Open(shmName);
			PluginHandlerPtr pluginHandler = PluginHandler::Create();

			while(!done){
				std::string type, message;
				FlogD("waiting for message");
				hostQueue->ReadMessage(type, message);
				FlogExpD(type);

				if(type == "run"){
					try {
						int nFrames = Video::CountFramesInFile(message, [&](int stream){
							if(onFrameCountMessages){
								hostQueue->WriteMessage("on_frame_count", Str(stream));
							}
						});

						RunPlugins(message, hostQueue, pluginHandler, platform, nFrames);
						FlogExpD(nFrames);
					} catch (VideoEx ex) {
						FlogE("failed to run plugins: " << ex.GetMsg());
						hostQueue->WriteMessage("video_error", ex.GetMsg());
					}
				}

				else if(type == "enable"){
					auto vec = Tools::Split(message);
					if(vec.size() == 2 || vec.size() == 3){
						std::string dir = platform->CombinePath({platform->GetWorkingDirectory(), "plugins"});
						bool debug = vec.size() == 3 && vec[2] == "debug";
						pluginHandler->AddPlugin(vec[0], platform->CombinePath({dir, vec[1], Str(vec[1] << ".exe")}), dir, debug);
					}else{
						FlogE("enable expects: enable [name] [executable] (\"debug\")");
					}
				}
				
				else if(Tools::StartsWith(type, "debugger_args")){
					auto vec = Tools::Split(type, '/');
					if(vec.size() == 2){
						FlogExpD(vec[0]);
						FlogExpD(vec[1]);
						pluginHandler->SetDebuggerArgs(vec[1], message);
					}else{
						FlogE("debugger_args expects: debugger_args/[plugin name] [args] (literally slashes)");
					}
				}
			
				else if(Tools::StartsWith(type, "debugger")){
					auto vec = Tools::Split(type, '/');
					if(vec.size() == 2){
						FlogExpD(vec[0]);
						FlogExpD(vec[1]);
						pluginHandler->SetDebugger(vec[1], message);
					}else{
						FlogE("debugger expects: debugger/[plugin name] [debugger] (literally slashes)");
					}
				}
				
				else if(Tools::StartsWith(type, "start_suspended")){
					auto vec = Tools::Split(type, '/');
					if(vec.size() == 2){
						FlogExpD(vec[0]);
						FlogExpD(vec[1]);
						pluginHandler->SetStartSuspended(vec[1], message == "true");
					}else{
						FlogE("start_suspended expects: start_suspended/[plugin name] [true/false] (literally slashes)");
					}
				}
				
				else if(Tools::StartsWith(type, "show_window")){
					auto vec = Tools::Split(type, '/');
					if(vec.size() == 2){
						FlogExpD(vec[0]);
						FlogExpD(vec[1]);
						pluginHandler->SetShowWindow(vec[1], message == "true");
					}else{
						FlogE("show_window expects: show_window/[plugin name] [true/false] (literally slashes)");
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
						FlogE("argument expects: argument/[plugin name]/[key] [argument] (literally slashes)");
					}
				}

				else if(type == "on_frame_count_messages")
				{
					onFrameCountMessages = message == "true";
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
