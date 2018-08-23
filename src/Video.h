#ifndef VIDEO_H
#define VIDEO_H

#include <memory>
#include <string>
#include <functional>

#include "Macros.h"
#include "Frame.h"

class Video;

typedef std::shared_ptr<Video> VideoPtr;
typedef std::shared_ptr<void> BufferPtr;
typedef std::function<void(const void* samples, int num_samples, double ts)> OnAudioFn;

DefEx(VideoEx);

class Video {
	public:
	enum PixelFormat {
		PixelFormatRgb24,
		PixelFormatGray,
		PixelFormatRgb32
	};

	enum SampleFormat {
		SampleFormatS16 = 0,
		SampleFormatFlt = 1
	};

	virtual int GetWidth() = 0;
	virtual int GetHeight() = 0;
	virtual float GetPixelAspectRatio() = 0;
	virtual float GetFrameRate() = 0;
	virtual float GetDuration() = 0;
	virtual long long GetFilePosition() = 0;
	virtual long long GetFileSize() = 0;

	virtual bool AudioPresent() = 0;
	virtual int GetSampleRate() = 0;
	virtual int GetChannels() = 0;
	virtual std::string GetSampleFormatStr() = 0;
	
	virtual double TimeStampToSeconds(long long ts) = 0;

	virtual bool GetFrame(int width, int height, PixelFormat fmt, FramePtr frame) = 0;

	virtual void SetAudioParameters(int sampleRate, int channels, SampleFormat format, OnAudioFn fn, int maxSamplesPerFrame) = 0;

	static int CountFramesInFile(const std::string& filename, std::function<void(int)> onFrameCount);
	
	static FramePtr CreateFrame(int width, int height, PixelFormat fmt);
	static VideoPtr Create(const std::string& filename);
};

#endif
