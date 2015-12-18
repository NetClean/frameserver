#ifndef SAMPLEBUFFER_H
#define SAMPLEBUFFER_H

#include <vector>
#include <memory>
#include <algorithm>

typedef std::shared_ptr<class SampleBuffer> SampleBufferPtr;

class SampleBuffer
{
	public:
	double pts = .0; 
	std::vector<float> samples;

	SampleBuffer(float pts, int nSamples, int channels, float* samples) : pts(pts)
	{
		this->samples.assign(samples, samples + nSamples * channels);
	}

	static SampleBufferPtr Create(float pts, int nSamples, int channels, float* samples)
	{
		return std::make_shared<SampleBuffer>(pts, nSamples, channels, samples);
	}
};

class CompareSampleBuffers
{
	public:
		bool operator()(SampleBufferPtr a, SampleBufferPtr b) const
		{
			return a->pts > b->pts;
		}
};

#endif
