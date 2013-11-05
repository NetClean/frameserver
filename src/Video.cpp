#include "Video.h"
#include "libvx.h"
#include "flog.h"

class CVideo : public Video {
	public:
	vx_video* video;

	CVideo(const std::string& filename){
		auto err = vx_open(&video, filename.c_str());
		AssertEx(err == VX_ERR_SUCCESS, VideoEx, "could not open video: '" << filename << "', because: " << vx_get_error_str(err));
	}

	~CVideo(){
		FlogD("closing video");
		vx_close(video);
	}


	int GetWidth(){
		return vx_get_width(video);
	}

	int GetHeight(){
		return vx_get_height(video);
	}

	long long GetFilePosition(){
		return vx_get_file_position(video);
	}

	long long GetFileSize(){
		return vx_get_file_size(video);
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
		FlogD("freeing frame buffer");
		vx_free_frame_buffer(buffer);
	}
};

int Video::CountFramesInFile(const std::string& filename)
{
	int count = 0;
	vx_error err = vx_count_frames_in_file(filename.c_str(), &count);
	AssertEx(err == VX_ERR_SUCCESS, VideoEx, vx_get_error_str(err));
	return count;
}

FramePtr Video::CreateFrame(int width, int height, PixelFormat fmt)
{
	return FramePtr(new VideoFrame(width, height, fmt));
}

VideoPtr Video::Create(const std::string& filename)
{
	return VideoPtr(new CVideo(filename));
}
