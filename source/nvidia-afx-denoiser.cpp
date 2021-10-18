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

#include "nvidia-afx-denoiser.hpp"
#include "lib.hpp"
#include "util-platform.hpp"

#include <nvAudioEffects.h>

#define SAMPLERATE 48000
#define BLOCKSIZE SAMPLERATE / 100

#define D_LOG(MESSAGE, ...) ::voicefx::log("<NVAFX::Denoiser> " MESSAGE, __VA_ARGS__)

nvidia::afx::denoiser::denoiser() : _nvafx(::nvidia::afx::afx::instance()), _dirty(true), _cuda(), _context(), _stream()
{
	try {
		_cuda    = ::nvidia::cuda::cuda::get();
		_context = _nvafx->cuda_context();
	} catch (...) {
	}

	create_effect();
}

nvidia::afx::denoiser::~denoiser() {}

uint32_t nvidia::afx::denoiser::get_sample_rate()
{
	return SAMPLERATE;
}

uint32_t nvidia::afx::denoiser::get_minimum_delay()
{
	constexpr uint32_t min_delay_ms  = 74; // Documented is 74ms, but measured is ~82ms.
	constexpr uint32_t ms_to_samples = 1000;
	return (SAMPLERATE * min_delay_ms) / ms_to_samples;
}

uint32_t nvidia::afx::denoiser::get_block_size() const
{
	return _block_size;
}

void nvidia::afx::denoiser::process(const float input[], float output[])
{
	_dirty = true;
	if (auto res = NvAFX_Run(_nvfx.get(), &input, &output, _block_size, 1); res != NVAFX_STATUS_SUCCESS) {
		D_LOG("Running effect returned error code %" PRIu32 ".", res);
		throw std::runtime_error("Failed to process buffers.");
	}
}

void nvidia::afx::denoiser::reset()
{
	if (_dirty) {
		/*
		// Re-create the effect, waiting on NVIDIA to add a way to just reset things.
		_nvfx.reset();
		create_effect();
		*/

		// According to NVIDIA, we can flush the effect by feeding it roughly 100ms of
		// absolutely no sound. This should be drastically faster than unloading and
		// reloading the effect every time.
		std::vector<float> empty_buffer(static_cast<size_t>(get_block_size()), 0.);
		for (size_t idx = 0; idx < 10; idx++) {
			process(empty_buffer.data(), empty_buffer.data());
		}
		_dirty = false;
	}
}

void nvidia::afx::denoiser::create_effect()
{
	std::shared_ptr<::nvidia::cuda::context_stack> ctx_stack;
	if (_context) {
		ctx_stack = _context->enter();
	}

	// Create the NvAFX effect.
	NvAFX_Handle effect = nullptr;
	if (auto res = NvAFX_CreateEffect(NVAFX_EFFECT_DENOISER, &effect); res != NVAFX_STATUS_SUCCESS) {
		D_LOG("Failed to create '" NVAFX_EFFECT_DENOISER "' effect, error code %" PRIu32 ".", res);
		throw std::runtime_error("Failed to create effect.");
	}
	_nvfx = std::shared_ptr<void>(effect, [](void* p) { NvAFX_DestroyEffect(reinterpret_cast<NvAFX_Handle>(p)); });

	// Tell the effect where it can find the necessary models for operation.
	_model_path = _nvafx->redistributable_path();
	_model_path /= "models";
	_model_path /= "denoiser_48k.trtpkg"; // TODO: Figure out why we need to specify the exact model.
	_model_path                = std::filesystem::absolute(_model_path);
	std::string _model_path_u8 = voicefx::util::platform::native_to_utf8(_model_path).string();
	if (auto res = NvAFX_SetString(_nvfx.get(), NVAFX_PARAM_MODEL_PATH, _model_path_u8.c_str());
		res != NVAFX_STATUS_SUCCESS) {
		D_LOG("Unable to set appropriate model path, error code %" PRIu32 ".", res);
		throw std::runtime_error("Failed to set up effect.");
	}

	// Set it up for the appropriate high quality sample rate.
	if (auto res = NvAFX_SetU32(_nvfx.get(), NVAFX_PARAM_SAMPLE_RATE, SAMPLERATE); res != NVAFX_STATUS_SUCCESS) {
		D_LOG("Failed to set sample rate to 48kHz, error code %" PRIu32 ".", res);
		throw std::runtime_error("Failed to set up effect.");
	}

	// Set some defaults which we don't care about failing.
	NvAFX_SetU32(_nvfx.get(), NVAFX_PARAM_USE_DEFAULT_GPU, 0);
	NvAFX_SetFloat(_nvfx.get(), NVAFX_PARAM_INTENSITY_RATIO, 1.0);

	// Finally, load the effect.
	if (auto res = NvAFX_Load(_nvfx.get()); res != NVAFX_STATUS_SUCCESS) {
		D_LOG("Failed to load effect, error code %" PRIu32 ".", res);
		throw std::runtime_error("Failed to load effect.");
	}

	// Retrieve the expected block size for processing.
	if (auto res = NvAFX_GetU32(_nvfx.get(), NVAFX_PARAM_NUM_SAMPLES_PER_FRAME, &_block_size);
		res != NVAFX_STATUS_SUCCESS) {
		D_LOG("Failed to retrieve expected block size, error code %" PRIu32 ".", res);
		_block_size = BLOCKSIZE; // Assume 10ms.
	}

	_dirty = false;
}
