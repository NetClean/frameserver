#include "Video.h"
#include "libvx.h"

class CVideo : public Video {
	public:
	vx_video* video;

	CVideo(const std::string& filename){
		auto err = vx_open(&video, filename.c_str());
		AssertEx(err == VX_ERR_SUCCESS, VideoEx, "could not open video: '" << filename << "', because: " << vx_get_error_str(err));
	}

	~CVideo(){
		vx_close(video);
	}


	int GetWidth(){
		return vx_get_width(video);
	}

	int GetHeight(){
		return vx_get_height(video);
	}

	float GetPixelAspectRatio(){
		float ret = 0;
		auto err = vx_get_pixel_aspect_ratio(video, &ret);

		AssertEx(err == VX_ERR_SUCCESS, VideoEx, vx_get_error_str(err));
		
		return ret;
	}

	float GetFrameRate(){
		float ret = 0;
		auto err = vx_get_frame_rate(video, &ret);

		AssertEx(err == VX_ERR_SUCCESS, VideoEx, vx_get_error_str(err));
		
		return ret;
	}

	float GetDuration(){
		float ret = 0;
		auto err = vx_get_duration(video, &ret);

		AssertEx(err == VX_ERR_SUCCESS, VideoEx, vx_get_error_str(err));
		
		return ret;
	}

	bool GetFrame(int width, int height, PixelFormat fmt, FramePtr frame){
		auto err = vx_get_frame(video, width, height, (vx_pix_fmt)fmt, frame->buffer);

		if(err == VX_ERR_EOF)
			return false;

		AssertEx(err == VX_ERR_SUCCESS, VideoEx, vx_get_error_str(err));

		return true;
	}
};

class VideoFrame : public Frame
{
	public:
	VideoFrame(int width, int height, Video::PixelFormat fmt){
		buffer = (uint8_t*)vx_alloc_frame_buffer(width, height, (vx_pix_fmt)fmt);
		this->width = width;
		this->height = height;
		this->bytesPerPixel = fmt == Video::PixelFormatGray ? 1 : 3;
	}

	~VideoFrame(){
		vx_free_frame_buffer(buffer);
	}
};

FramePtr Video::CreateFrame(int width, int height, PixelFormat fmt)
{
	return FramePtr(new VideoFrame(width, height, fmt));
}

VideoPtr Video::Create(const std::string& filename)
{
	return VideoPtr(new CVideo(filename));
}
