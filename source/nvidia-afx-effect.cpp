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

#include "nvidia-afx-effect.hpp"
#include "lib.hpp"
#include "util-platform.hpp"

#include <nvAudioEffects.h>

#define D_LOG(MESSAGE, ...) ::voicefx::log("<nvidia::afx::effect> " MESSAGE, __VA_ARGS__)

nvidia::afx::effect::effect()
{
	// Set up initial state.
	_fx_dirty      = true;
	_fx_denoise    = true;
	_fx_dereverb   = true;
	_fx_channels   = true;
	_cfg_dirty     = true;
	_cfg_intensity = 1.0;
}

nvidia::afx::effect::~effect()
{
	for (auto& fx : _fx) {
		fx.reset();
	}
	_fx.clear();
	_stream->synchronize();
	_stream.reset();
	_context->synchronize();
	_context.reset();
	_cuda.reset();
}

size_t nvidia::afx::effect::samplerate()
{
	// FIXME: This is hardcoded, but may change in the future.
	return 48000; // 48kHz.
}

size_t nvidia::afx::effect::blocksize()
{
	// FIXME: At the moment, the block size is 10ms.
	return samplerate() / (1000 / 10);
	//return 480; // 10ms at 48kHz.
}

bool nvidia::afx::effect::denoise_enabled()
{
	return _fx_denoise;
}

void nvidia::afx::effect::enable_denoise(bool v)
{
	// Prevent outside modifications while we're working.
	auto lock = std::unique_lock<std::mutex>(_lock);

	if (v != _fx_denoise) {
		_fx_denoise = v;
		_fx_dirty   = true;
	}
}

bool nvidia::afx::effect::dereverb_enabled()
{
	return _fx_dereverb;
}

void nvidia::afx::effect::enable_dereverb(bool v)
{
	// Prevent outside modifications while we're working.
	auto lock = std::unique_lock<std::mutex>(_lock);

	if (v != _fx_dereverb) {
		_fx_dereverb = v;
		_fx_dirty    = true;
	}
}

size_t nvidia::afx::effect::channels()
{
	return _fx_channels;
}

void nvidia::afx::effect::channels(size_t v)
{
	// Prevent outside modifications while we're working.
	auto lock = std::unique_lock<std::mutex>(_lock);

	if (v != _fx_channels) {
		_fx_channels = v;
		_fx_dirty    = true;
	}
}

float nvidia::afx::effect::intensity()
{
	return _cfg_intensity;
}

void nvidia::afx::effect::intensity(float v)
{
	// Prevent outside modifications while we're working.
	auto lock = std::unique_lock<std::mutex>(_lock);

	if (v != _cfg_intensity) {
		_cfg_intensity = v;
		_cfg_dirty     = true;
	}
}

void nvidia::afx::effect::load()
{
	// Prevent outside modifications while we're working.
	auto lock = std::unique_lock<std::mutex>(_lock);

	if (!_fx_dirty) {
		// Decide on the effect to load.
		NvAFX_EffectSelector effect = NVAFX_EFFECT_DEREVERB_DENOISER;
		if (_fx_denoise && !_fx_dereverb) {
			effect = NVAFX_EFFECT_DENOISER;
		} else if (!_fx_denoise && _fx_dereverb) {
			effect = NVAFX_EFFECT_DEREVERB;
		}

		// Unload all previous effects.
		_fx.resize(_fx_channels);
		for (auto fx : _fx) {
			fx.reset();
		}

		{ // Figure out where exactly models are located.
			_model_path     = _nvafx->redistributable_path() / "models";
			_model_path     = std::filesystem::absolute(_model_path);
			_model_path     = voicefx::util::platform::native_to_utf8(_model_path);
			_model_path_str = _model_path.u8string();
		}

		// Create and initialize effects.
		for (auto& fx : _fx) {
			// Create the chosen effect.
			NvAFX_Handle pfx = nullptr;
			if (auto error = NvAFX_CreateEffect(effect, &pfx); error != NVAFX_STATUS_SUCCESS) {
				char buffer[1024];
				snprintf(buffer, sizeof(buffer), "Failed to create effect, error code %" PRIx32 ".\0", error);
				throw std::runtime_error(buffer);
			}
			fx = std::shared_ptr<void>(pfx, [](NvAFX_Handle v) { NvAFX_DestroyEffect(v); });

			// Assign correct model path
			if (auto error = NvAFX_SetString(fx.get(), NVAFX_PARAM_MODEL_PATH, _model_path_str.c_str());
				error != NVAFX_STATUS_SUCCESS) {
				char buffer[1024];
				snprintf(buffer, sizeof(buffer), "Failed to configure effect paths, error code %" PRIx32 ".\0", error);
				throw std::runtime_error(buffer);
			}

			// Assign correct sample rate.
			if (auto error = NvAFX_SetU32(fx.get(), NVAFX_PARAM_SAMPLE_RATE, static_cast<unsigned int>(samplerate()));
				error != NVAFX_STATUS_SUCCESS) {
				char buffer[1024];
				snprintf(buffer, sizeof(buffer), "Failed to configure effect samplerate, error code %" PRIx32 ".\0",
						 error);
				throw std::runtime_error(buffer);
			}

			// Automatically let the effect pick the correct GPU.
			NvAFX_SetU32(fx.get(), NVAFX_PARAM_USE_DEFAULT_GPU, 1);
		}

		// Load effects.
		for (auto& fx : _fx) {
			if (auto error = NvAFX_Load(fx.get()); error != NVAFX_STATUS_SUCCESS) {
				char buffer[1024];
				snprintf(buffer, sizeof(buffer), "Failed to initialize effect, error code %" PRIx32 ".\0", error);
				throw std::runtime_error(buffer);
			}
		}

		_fx_dirty = false;
	}

	if (_cfg_dirty) {
		for (auto& fx : _fx) {
			if (auto error = NvAFX_SetFloat(fx.get(), NVAFX_PARAM_INTENSITY_RATIO, _cfg_intensity);
				error != NVAFX_STATUS_SUCCESS) {
				char buffer[1024];
				snprintf(buffer, sizeof(buffer), "Failed to set intensity, error code %" PRIx32 ".\0", error);
				throw std::runtime_error(buffer);
			}
		}

		_cfg_dirty = false;
	}
}

void nvidia::afx::effect::clear()
{
	// Prevent outside modifications while we're working.
	auto lock = std::unique_lock<std::mutex>(_lock);

	// Soft-clear the effect by flooding the internal buffer.
	std::vector<float>  data(blocksize() * 10, 0.f);
	std::vector<float*> channel_data(_fx_channels, data.data());
	process(const_cast<const float**>(channel_data.data()), channel_data.data(), data.size());
}

void nvidia::afx::effect::process(const float** input, float** output, size_t samples)
{
	// Safe-guard against bad usage.
	if ((samples % blocksize()) != 0) {
		throw std::runtime_error("Sample data must be provided as a multiple of 'blocksize()'.");
	}

	// Reload the effect
	if (_fx_dirty || _cfg_dirty) {
		load();
	}

	// Prevent outside modifications while we're working.
	auto lock = std::unique_lock<std::mutex>(_lock);

	// Process all data passed in.
	for (size_t channel = 0; channel < _fx_channels; channel++) {
		auto& fx = _fx[channel];
		for (size_t offset = 0; offset < samples; offset += blocksize()) {
			if (auto error =
					NvAFX_Run(fx.get(), &input[channel], &output[channel], static_cast<unsigned int>(blocksize()), 1);
				error != NVAFX_STATUS_SUCCESS) {
				char buffer[1024];
				snprintf(buffer, sizeof(buffer), "Failed to set intensity, error code %" PRIx32 ".\0", error);
				throw std::runtime_error(buffer);
			}
		}
	}
}
