// Copyright 2020 Michael Fabian 'Xaymar' Dirks <info@xaymar.com>
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.

#include "resampler.hpp"
#include "lib.hpp"

#include "warning-disable.hpp"
#include <cmath>
#include <samplerate.h>
#include <stdexcept>
#include "warning-enable.hpp"

voicefx::resampler::~resampler()
{
	D_LOG_LOUD("");
	_instance.clear();
}

voicefx::resampler::resampler() : _instance(), _channels(0), _ratio(0.0), _dirty(true)
{
	D_LOG_LOUD("");
}

voicefx::resampler::resampler(resampler&& r) noexcept : _instance(), _ratio(1.0), _channels(1)
{
	D_LOG_LOUD("");
	std::swap(_instance, r._instance);
	std::swap(_ratio, r._ratio);
	std::swap(_channels, r._channels);
	std::swap(_dirty, r._dirty);
}

voicefx::resampler& voicefx::resampler::operator=(voicefx::resampler&& r) noexcept
{
	D_LOG_LOUD("");
	std::swap(_instance, r._instance);
	std::swap(_ratio, r._ratio);
	std::swap(_channels, r._channels);
	std::swap(_dirty, r._dirty);
	return *this;
}

float voicefx::resampler::ratio()
{
	D_LOG_LOUD("");
	return _ratio;
}

void voicefx::resampler::ratio(uint32_t in_samplerate, uint32_t out_samplerate)
{
	D_LOG_LOUD("");
	_ratio = static_cast<float>(in_samplerate) / static_cast<float>(out_samplerate);
}

size_t voicefx::resampler::channels()
{
	D_LOG_LOUD("");
	return _channels;
}

void voicefx::resampler::channels(size_t channels)
{
	D_LOG_LOUD("");
	if (_channels > std::numeric_limits<int32_t>::max()) {
		throw_log("Channel limit exceeded.");
	}
	if (_channels != channels) {
		_channels = channels;
		_dirty    = true;
	}
}

void voicefx::resampler::load()
{
	D_LOG_LOUD("");
	if (!_dirty) {
		return;
	}

	_instance.resize(_channels);
	_instance.shrink_to_fit();

	for (auto& instance : _instance) {
		if (instance) {
			src_reset(reinterpret_cast<SRC_STATE*>(instance.get()));
		} else {
			int error = 0;
			instance  = std::shared_ptr<void>(reinterpret_cast<void*>(src_new(SRC_SINC_BEST_QUALITY, static_cast<int>(_channels), &error)), [](void* v) { src_delete(reinterpret_cast<SRC_STATE*>(v)); });
			if (error != 0) {
				throw_log("%s", src_strerror(error));
			}
		}
	}

	_dirty = false;
}

void voicefx::resampler::clear()
{
	D_LOG_LOUD("");
	for (auto& instance : _instance) {
		if (instance) {
			src_reset(reinterpret_cast<SRC_STATE*>(instance.get()));
		}
	}
}

void voicefx::resampler::process(const float* in_buffer[], size_t in_samples, size_t& in_samples_used, float* out_buffer[], size_t out_samples, size_t& out_samples_generated)
{
	D_LOG_LOUD("");
	// Ensure we have a resampler
	if (_dirty) {
		load();
	}

	// Prepare the data object.
	SRC_DATA data      = {0};
	data.input_frames  = static_cast<long>(in_samples);
	data.output_frames = static_cast<long>(out_samples);
	data.end_of_input  = in_buffer == nullptr ? true : false;
	data.src_ratio     = _ratio;

	// Process each channel.
	for (size_t idx = 0; idx < _channels; idx++) {
		auto& instance = _instance[idx];

		data.data_in           = in_buffer[idx];
		data.data_out          = out_buffer[idx];
		data.input_frames_used = 0;
		data.output_frames_gen = 0;

		if (int error = src_process(reinterpret_cast<SRC_STATE*>(instance.get()), &data); error != 0) {
			throw_log("%s", src_strerror(error));
		}

		in_samples_used       = data.input_frames_used;
		out_samples_generated = data.output_frames_gen;
	}
}

size_t voicefx::resampler::calculate_delay(uint32_t in_samplerate, uint32_t out_samplerate)
{
	D_LOG_STATIC_LOUD("");
	int  error    = 0;
	auto instance = src_new(SRC_SINC_BEST_QUALITY, 1, &error);
	if (error != 0) {
		throw_log_static("%s", src_strerror(error));
	}

	// Calculate the ratio for the conversion.
	float sr_ratio = static_cast<float>(in_samplerate) / static_cast<float>(out_samplerate);

	// Prepare some data.
	SRC_DATA data = {0};
	float    in_buffer[1024];
	float    out_buffer[1024];
	memset(in_buffer, 0, sizeof(in_buffer));
	memset(out_buffer, 0, sizeof(out_buffer));
	in_buffer[0]           = 1.;
	in_buffer[1]           = -1.;
	data.data_in           = in_buffer;
	data.data_out          = out_buffer;
	data.input_frames      = static_cast<long>(sizeof(in_buffer) / sizeof(*in_buffer));
	data.output_frames     = static_cast<long>(sizeof(out_buffer) / sizeof(*out_buffer));
	data.input_frames_used = 0;
	data.output_frames_gen = data.output_frames;
	data.end_of_input      = 0;
	data.src_ratio         = sr_ratio;

	// Calculate the delay
	for (size_t delay = 0; delay < in_samplerate; delay++) {
		data.input_frames      = static_cast<long>(sizeof(in_buffer) / sizeof(*in_buffer));
		data.output_frames     = static_cast<long>(sizeof(out_buffer) / sizeof(*out_buffer));
		data.input_frames_used = 0;
		data.output_frames_gen = data.output_frames;
		data.end_of_input      = 0;
		data.src_ratio         = sr_ratio;

		if (int error = src_process(instance, &data); error != 0) {
			throw_log_static("%s", src_strerror(error));
		}

		if (data.output_frames_gen > 0) {
			D_LOG_STATIC_LOUD("%" PRId32 " %" PRId32 " %" PRId32 " %" PRId32, data.input_frames, data.output_frames, data.input_frames_used, data.output_frames_gen);
			src_delete(instance);
			return (delay * data.input_frames) + (data.output_frames - data.output_frames_gen);
		}
	}

	src_delete(instance);
	return in_samplerate;
}
