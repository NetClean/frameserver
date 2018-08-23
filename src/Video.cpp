#include "Video.h"
#include "libvx.h"
#include "flog.h"

class VideoFrame : public Frame
{
	public:
	vx_frame* frame;

	VideoFrame(int width, int height, Video::PixelFormat fmt){
		frame = vx_frame_create(width, height, (vx_pix_fmt)fmt);
		AssertEx(frame, VideoEx, "could not allocate memory for vx_frame_info object");

		int bpp[] = {3, 1, 4};

		this->width = width;
		this->height = height;
		this->bytesPerPixel = bpp[(int)fmt];
		this->buffer = (uint8_t*)vx_frame_get_buffer(frame);
	}

	~VideoFrame(){
		FlogD("freeing frame buffer");
		vx_frame_destroy(frame);
	}
};

class CVideo : public Video {
	public:
	vx_video* video;
	OnAudioFn audioFn;

	static void AudioCb(const void* samples, int numSamples, double ts, void* userData)
	{
		CVideo* me = (CVideo*)userData;
		me->audioFn(samples, numSamples, ts);
	}

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
	
	bool AudioPresent()
	{
		return vx_get_audio_present(video) == 1;
	}

	int GetSampleRate()
	{
		return vx_get_audio_sample_rate(video);
	}

	int GetChannels()
	{
		return vx_get_audio_channels(video);
	}

	std::string GetSampleFormatStr()
	{
		return std::string(vx_get_audio_sample_format_str(video));
	}
	
	virtual void SetAudioParameters(int sampleRate, int channels, SampleFormat format, OnAudioFn fn, int maxSamplesPerFrame)
	{
		audioFn = fn;
		vx_error err = vx_set_audio_params(video, sampleRate, channels, (vx_sample_fmt)format, AudioCb, this);
		AssertEx(err == VX_ERR_SUCCESS, VideoEx, vx_get_error_str(err));
		
		err = vx_set_max_samples_per_frame(video, maxSamplesPerFrame);
		AssertEx(err == VX_ERR_SUCCESS, VideoEx, vx_get_error_str(err));
	}

	double TimeStampToSeconds(long long ts)
	{
		return vx_timestamp_to_seconds(video, ts);
	}

	bool GetFrame(int width, int height, PixelFormat fmt, FramePtr frame){
		auto vf = std::static_pointer_cast<VideoFrame>(frame);

		auto err = vx_get_frame(video, vf->frame);

		if(err == VX_ERR_EOF)
			return false;

		if(err == VX_ERR_FRAME_DEFERRED)
		{
			frame->deferred = true;
		}
		else
		{
			AssertEx(err == VX_ERR_SUCCESS, VideoEx, vx_get_error_str(err));
			
			frame->deferred = false;
			frame->flags = vx_frame_get_flags(vf->frame);
			frame->bytePos = vx_frame_get_byte_pos(vf->frame);
			frame->dts = vx_frame_get_dts(vf->frame);
			frame->pts = vx_frame_get_pts(vf->frame);
			frame->ptsSeconds = vx_timestamp_to_seconds(video, frame->pts);
			frame->dtsSeconds = vx_timestamp_to_seconds(video, frame->dts);
		}

		return true;
	}
};

void OnFrameCount(int stream, void* data)
{
	auto cb = (std::function<void(int)>*)data;
	(*cb)(stream);
}

int Video::CountFramesInFile(const std::string& filename, std::function<void(int)> onFrameCount)
{
	int count = 0;

	vx_error err = vx_count_frames_in_file_with_cb(filename.c_str(), &count, OnFrameCount, (void*)&onFrameCount);
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
