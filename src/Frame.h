#ifndef FRAME_H
#define FRAME_H

#include <cstdint>
#include <memory>

struct Frame {
	int width, height, bytesPerPixel;
	uint8_t* buffer;
};

typedef std::shared_ptr<Frame> FramePtr;

#endif
