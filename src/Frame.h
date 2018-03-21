#ifndef FRAME_H
#define FRAME_H

#include <cstdint>
#include <memory>

struct Frame {
	enum Flags {
		FKeyFrame = 1
	};

	bool deferred;
	unsigned int flags;
	int64_t bytePos, dts, pts;
	int width, height, bytesPerPixel;
	uint8_t* buffer;
	double dtsSeconds, ptsSeconds;
};

typedef std::shared_ptr<Frame> FramePtr;

#endif
