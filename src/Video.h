#ifndef VIDEO_H
#define VIDEO_H

#include <memory>
#include <string>

#include "Macros.h"
#include "Frame.h"

class Video;

typedef std::shared_ptr<Video> VideoPtr;
typedef std::shared_ptr<void> BufferPtr;

DefEx(VideoEx);

class Video {
	public:
	enum PixelFormat {
		PixelFormatRgb,
		PixelFormatGray
	};

	virtual int GetWidth() = 0;
	virtual int GetHeight() = 0;
	virtual float GetPixelAspectRatio() = 0;
	virtual float GetFrameRate() = 0;
	virtual float GetDuration() = 0;

	virtual bool GetFrame(int width, int height, PixelFormat fmt, FramePtr frame) = 0;
	
	static FramePtr CreateFrame(int width, int height, PixelFormat fmt);
	static VideoPtr Create(const std::string& filename);
};

#endif
