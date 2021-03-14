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
#include <stdexcept>
#include "lib.hpp"

#include <samplerate.h>

#define D_LOG(MESSAGE, ...) voicefx::log("<Resampler> " MESSAGE, __VA_ARGS__)

voicefx::resampler::~resampler()
{
	src_reset(reinterpret_cast<SRC_STATE*>(_instance));
	_instance = src_delete(reinterpret_cast<SRC_STATE*>(_instance));
}

voicefx::resampler::resampler() : _ratio(1.0)
{
	int error = 0;
	_instance = reinterpret_cast<void*>(src_new(SRC_SINC_BEST_QUALITY, 1, &error));
	if (error != 0) {
		throw std::runtime_error("Failed to create resampler.");
	}
}

voicefx::resampler::resampler(uint32_t input, uint32_t output) : resampler()
{
	// Calculate resampling ratio.
	_ratio = (static_cast<float>(output) / static_cast<float>(input));
}

void voicefx::resampler::process(const float input[], size_t& input_samples, float output[], size_t& output_samples)
{
	SRC_DATA data = {0};

	// Initialize our data structure.
	data.data_in           = input;
	data.data_out          = output;
	data.input_frames      = static_cast<long>(input_samples);
	data.output_frames     = static_cast<long>(output_samples);
	data.input_frames_used = 0;
	data.output_frames_gen = 0;
	data.end_of_input      = 0;
	data.src_ratio         = _ratio;

	// Process and resample the audio.
	if (int error = src_process(reinterpret_cast<SRC_STATE*>(_instance), &data); error != 0) {
		D_LOG("Failed to resample, error code %" PRId32 ".", error);
		throw std::runtime_error("Failed to resample with the given data and parameters.");
	}

	// Return some information.
	input_samples  = data.input_frames_used;
	output_samples = data.output_frames_gen;
}

void voicefx::resampler::reset(uint32_t input_samplerate, uint32_t output_samplerate)
{
	src_reset(reinterpret_cast<SRC_STATE*>(_instance));
	if ((input_samplerate != 0) && (output_samplerate != 0)) {
		_ratio = static_cast<float>(output_samplerate) / static_cast<float>(input_samplerate);
	}
}

float voicefx::resampler::ratio() const
{
	return _ratio;
}
