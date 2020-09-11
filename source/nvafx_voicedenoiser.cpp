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

#include "nvafx_voicedenoiser.hpp"
#include <filesystem>
#include <stdexcept>
#include "lib.hpp"

#define kSample32 float

nvafx::voicedenoiser::voicedenoiser::voicedenoiser()
	: _samplerate(0), _samples_per_frame(0), _afx(nullptr), _afx_samplerate(48000), _afx_samples(0), _afx_channels(0),
	  _src_in(nullptr), _src_in_ratio(1), _src_in_buffer(), _in_buffer(), _in_offset(0), _src_out(nullptr),
	  _src_out_ratio(1), _src_out_buffer(), _out_buffer(), _out_offset(0), _delay(true), _delay_samples(0)
{
	{
		NvAFX_Handle afx;
		if (NvAFX_CreateEffect(NVAFX_EFFECT_DENOISER, &afx) != NVAFX_STATUS_SUCCESS) {
			throw std::runtime_error("Failed to create denoiser effect.");
		}
		_afx = std::shared_ptr<void>(afx, [](void* v) { NvAFX_DestroyEffect(v); });
	}

	if (NvAFX_SetU32(_afx.get(), NVAFX_PARAM_DENOISER_SAMPLE_RATE, _afx_samplerate) != NVAFX_STATUS_SUCCESS) {
		throw std::runtime_error("Failed to set samplerate.");
	}

	{
		auto module_path = nvafx_path();
		module_path.append("models");
		module_path.append("denoiser_48k.wpkg");
		std::filesystem::path model_path = std::filesystem::absolute(module_path);

		if (NvAFX_SetString(_afx.get(), NVAFX_PARAM_DENOISER_MODEL_PATH, model_path.string().c_str())
			!= NVAFX_STATUS_SUCCESS) {
			throw std::runtime_error("Failed to set model path.");
		}
	}

	if (NvAFX_Load(_afx.get()) != NVAFX_STATUS_SUCCESS) {
		throw std::runtime_error("Failed to load effect.");
	}

	if (NvAFX_GetU32(_afx.get(), NVAFX_PARAM_DENOISER_NUM_SAMPLES_PER_FRAME, &_afx_samples) != NVAFX_STATUS_SUCCESS) {
		throw std::runtime_error("Can't figure out the number of samples per frame.");
	}

	if (NvAFX_GetU32(_afx.get(), NVAFX_PARAM_DENOISER_NUM_CHANNELS, &_afx_channels) != NVAFX_STATUS_SUCCESS) {
		throw std::runtime_error("Can't figure out the number of channels.");
	}

	_afx_input.resize(_afx_channels);
	_afx_output.resize(_afx_channels);

	// Create resamplers
	int error = 0;
	_src_in   = src_new(SRC_SINC_BEST_QUALITY, _afx_channels, &error);
	if (error != 0) {
		throw std::runtime_error("Failed to create input resampler.");
	}

	_src_out = src_new(SRC_SINC_BEST_QUALITY, _afx_channels, &error);
	if (error != 0) {
		throw std::runtime_error("Failed to create output resampler.");
	}
}

nvafx::voicedenoiser::voicedenoiser::~voicedenoiser()
{
	if (_src_in)
		_src_in = src_delete(_src_in);
	if (_src_out)
		_src_out = src_delete(_src_out);
}

void nvafx::voicedenoiser::voicedenoiser::reset(uint32_t samplerate, uint32_t spf)
{
	// Reset values.
	_samplerate        = samplerate;
	_samples_per_frame = spf;
	_in_offset         = 0;
	_out_offset        = 0;
	_delay             = true;
	_delay_samples     = static_cast<uint32_t>(
        static_cast<uint64_t>(ceil(static_cast<double_t>(_afx_samples) / static_cast<double_t>(spf))) * spf);

	// Reset resamples.
	src_reset(_src_in);
	src_reset(_src_out);

	// Calculate new resampling ratio.
	_src_in_ratio  = static_cast<double_t>(_afx_samplerate) / static_cast<double_t>(_samplerate);
	_src_out_ratio = static_cast<double_t>(_samplerate) / static_cast<double_t>(_afx_samplerate);

	// Resize resampler buffers.
	_src_in_buffer.resize(static_cast<size_t>(_samples_per_frame) << 2);
	_src_out_buffer.resize(static_cast<size_t>(_afx_samples) << 2);

	// Resize internal queues.
	_in_buffer.resize(_src_in_buffer.size() * 4);
	_out_buffer.resize(static_cast<size_t>(_samples_per_frame) * 4);
}

bool nvafx::voicedenoiser::voicedenoiser::process(uint32_t samples, const float* data_in, float* data_out)
{
	SRC_DATA srcdata;

	if (!_samplerate)
		return false;

	// Resample from Hosts sample rate to NvAFX Sample Rate
	srcdata.data_in           = data_in;
	srcdata.input_frames      = samples;
	srcdata.input_frames_used = 0;
	srcdata.data_out          = _src_in_buffer.data();
	srcdata.output_frames     = static_cast<long>(_src_in_buffer.size() - _in_offset);
	srcdata.output_frames_gen = 0;
	srcdata.end_of_input      = 0;
	srcdata.src_ratio         = _src_in_ratio;
	if (int error = src_process(_src_in, &srcdata); error != 0) {
		return false;
	}

	// Copy data from input to the internal queue.
	memcpy(_in_buffer.data() + _in_offset, _src_in_buffer.data(), sizeof(kSample32) * srcdata.output_frames_gen);
	_in_offset += srcdata.output_frames_gen;

	// Check if we have enough data for filtering.
	while (_in_offset >= _afx_samples) {
		// Filter available data.
		_afx_input[0]  = _in_buffer.data();
		_afx_output[0] = _src_out_buffer.data();
		if (NvAFX_Run(_afx.get(), _afx_input.data(), _afx_output.data(), _afx_samples, _afx_channels)
			!= NVAFX_STATUS_SUCCESS) {
			return false;
		}

		// Resample filtered data to host sample rate.
		srcdata.data_in           = _src_out_buffer.data();
		srcdata.input_frames      = _afx_samples;
		srcdata.input_frames_used = 0;
		srcdata.data_out          = _out_buffer.data() + _out_offset;
		srcdata.output_frames     = static_cast<long>(_out_buffer.size() - _out_offset);
		srcdata.output_frames_gen = 0;
		srcdata.end_of_input      = 0;
		srcdata.src_ratio         = _src_out_ratio;
		if (int error = src_process(_src_out, &srcdata); error != 0) {
			return false;
		}

		// Update offsets.
		_in_offset -= _afx_samples;
		_out_offset += srcdata.output_frames_gen;

		// Move sample buffer back.
		memcpy(_in_buffer.data(), _in_buffer.data() + _afx_samples,
			   sizeof(kSample32) * (_in_buffer.size() - _afx_samples));
	}

	// Disable forced delay once we have enough in the buffer.
	if (_out_offset >= _delay_samples)
		_delay = false;

	// Are we still delayed?
	if (_delay) {
		// Clear output buffers.
		memset(data_out, 0, sizeof(kSample32) * samples);
	} else {
		// Copy data to output.
		memcpy(data_out, _out_buffer.data(), sizeof(kSample32) * samples);

		// Fix offsets.
		_out_offset -= samples;

		// Move output buffer data back.
		memcpy(_out_buffer.data(), _out_buffer.data() + samples, sizeof(kSample32) * (_out_buffer.size() - samples));
	}

	return true;
}
