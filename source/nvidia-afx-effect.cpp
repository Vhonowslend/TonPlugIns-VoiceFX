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
	: _cuda(::nvidia::cuda::cuda::get()), _context(), _stream(), _nvafx(), _lock(), _model_path(), _model_path_str(),
	  _fx_dirty(), _cfg_dirty(), _cfg_channels(), _cfg_enable_denoise()
{
	_nvafx = ::nvidia::afx::afx::instance();

	// Set up initial state.
	_fx_dirty           = true;
	_cfg_enable_denoise = true;
	_cfg_channels       = true;
	_cfg_dirty          = true;
#ifdef ENABLE_FULL_VERSION
	_cfg_enable_dereverb = true;
	_cfg_intensity       = 1.0;
#endif
}

nvidia::afx::effect::~effect()
{
	if (_fx.size() > 0) {
		for (auto& fx : _fx) {
			fx.reset();
		}
		_fx.clear();
	}
	if (_nvafx) {
		_nvafx.reset();
	}
	if (_stream) {
		_stream->synchronize();
		_stream.reset();
	}
	if (_context) {
		_context->synchronize();
		_context.reset();
	}
	if (_cuda) {
		_cuda.reset();
	}
}

uint32_t nvidia::afx::effect::samplerate()
{
	// FIXME: This is hardcoded, but may change in the future.
	return 48000; // 48kHz.
}

size_t nvidia::afx::effect::blocksize()
{
	// FIXME: At the moment, the block size is 10ms.
	return samplerate() / 100;
	//return 480; // 10ms at 48kHz.
}

size_t nvidia::afx::effect::delay()
{
	// Latency is no longer documented, but originally was documented as 72/74ms, measured as 82/84ms.
	return blocksize() * 8;
}

bool nvidia::afx::effect::denoise_enabled()
{
	return _cfg_enable_denoise;
}

void nvidia::afx::effect::enable_denoise(bool v)
{
	// Prevent outside modifications while we're working.
	auto lock = std::unique_lock<std::mutex>(_lock);

	if (v != _cfg_enable_denoise) {
		_cfg_enable_denoise = v;
		_fx_dirty           = true;
	}
}

#ifdef ENABLE_FULL_VERSION
bool nvidia::afx::effect::dereverb_enabled()
{
	return _cfg_enable_dereverb;
}

void nvidia::afx::effect::enable_dereverb(bool v)
{
	// Prevent outside modifications while we're working.
	auto lock = std::unique_lock<std::mutex>(_lock);

	if (v != _cfg_enable_dereverb) {
		_cfg_enable_dereverb = v;
		_fx_dirty            = true;
	}
}
#endif

size_t nvidia::afx::effect::channels()
{
	return _cfg_channels;
}

void nvidia::afx::effect::channels(size_t v)
{
	// Prevent outside modifications while we're working.
	auto lock = std::unique_lock<std::mutex>(_lock);

	if (v != _cfg_channels) {
		_cfg_channels = v;
		_fx_dirty     = true;
	}
}

#ifdef ENABLE_FULL_VERSION
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
#endif

void nvidia::afx::effect::load()
{
	char message_buffer[1024] = {0};

	// Prevent outside modifications while we're working.
	auto lock = std::unique_lock<std::mutex>(_lock);

	if (_fx_dirty) {
		auto cstk = _nvafx->cuda_context()->enter();

#ifdef WIN32
		// Fix the search paths if some other plugin messed with them.
		_nvafx->windows_fix_dll_search_paths();
#endif

		// Decide on the effect to load.
		NvAFX_EffectSelector effect       = NVAFX_EFFECT_DENOISER;
		std::string          effect_model = "denoiser_48k.trtpkg";
#ifdef ENABLE_FULL_VERSION
		if (_cfg_enable_denoise && _cfg_enable_dereverb) {
			effect       = NVAFX_EFFECT_DEREVERB_DENOISER;
			effect_model = "dereverb_denoiser_48k.trtpkg";
		} else if (!_cfg_enable_denoise && _cfg_enable_dereverb) {
			effect       = NVAFX_EFFECT_DEREVERB;
			effect_model = "dereverb_48k.trtpkg";
		}
#endif

		// Unload all previous effects.
		_fx.resize(_cfg_channels);
		for (auto fx : _fx) {
			fx.reset();
		}

		{ // Figure out where exactly models are located.
			_model_path = _nvafx->redistributable_path();
			_model_path = std::filesystem::absolute(_model_path);
			_model_path /= "models";
			_model_path /= effect_model;
			_model_path.make_preferred();
			_model_path     = voicefx::util::platform::native_to_utf8(_model_path);
			_model_path_str = _model_path.u8string();
		}

		// Create and initialize effects.
		for (size_t idx = 0; idx < _fx.size(); idx++) {
			auto& fx = _fx.at(idx);

			// Create the chosen effect.
			NvAFX_Handle pfx = nullptr;
			if (auto error = NvAFX_CreateEffect(effect, &pfx); error != NVAFX_STATUS_SUCCESS) {
				snprintf(message_buffer, sizeof(message_buffer),
						 "{%02" PRIuMAX "} Failed to create effect. (Code %08" PRIX32 ")\0", idx, error);
				throw std::runtime_error(message_buffer);
			}
			fx = std::shared_ptr<void>(pfx, [](NvAFX_Handle v) { NvAFX_DestroyEffect(v); });

			// Sample Rate
			try {
				if (auto error =
						NvAFX_SetU32(fx.get(), NVAFX_PARAM_INPUT_SAMPLE_RATE, static_cast<unsigned int>(samplerate()));
					error != NVAFX_STATUS_SUCCESS) {
					snprintf(message_buffer, sizeof(message_buffer),
							 "{%02" PRIuMAX "} Failed to set input sample rate to %" PRIu32 ". (Code %08" PRIX32 ")\0",
							 idx, samplerate(), error);
					throw std::runtime_error(message_buffer);
				}
				D_LOG("{%02" PRIuMAX "} Input Sample Rate is now %" PRIu32 ".", idx, samplerate());
				if (auto error =
						NvAFX_SetU32(fx.get(), NVAFX_PARAM_OUTPUT_SAMPLE_RATE, static_cast<unsigned int>(samplerate()));
					error != NVAFX_STATUS_SUCCESS) {
					snprintf(message_buffer, sizeof(message_buffer),
							 "{%02" PRIuMAX "} Failed to set output sample rate to %" PRIu32 ". (Code %08" PRIX32 ")\0",
							 idx, samplerate(), error);
					throw std::runtime_error(message_buffer);
				}
				D_LOG("{%02" PRIuMAX "} Output Sample Rate is now %" PRIu32 ".", idx, samplerate());
			} catch (std::exception& ex) {
				D_LOG("{%02" PRIuMAX "} Falling back to simple sample rate due error: %s", idx, ex.what());
				if (auto error =
						NvAFX_SetU32(fx.get(), NVAFX_PARAM_SAMPLE_RATE, static_cast<unsigned int>(samplerate()));
					error != NVAFX_STATUS_SUCCESS) {
					snprintf(message_buffer, sizeof(message_buffer),
							 "{%02" PRIuMAX "} Failed to set sample rate to %" PRIu32 ". (Code %08" PRIX32 ").\0", idx,
							 samplerate(), error);
					throw std::runtime_error(message_buffer);
				}
				D_LOG("{%02" PRIuMAX "} Sample Rate is now %" PRIu32 ".", idx, samplerate());
			}

			// Automatically let the effect pick the correct GPU.
			NvAFX_SetU32(fx.get(), NVAFX_PARAM_USE_DEFAULT_GPU, 0);
			NvAFX_SetU32(fx.get(), NVAFX_PARAM_USER_CUDA_CONTEXT, 1);

			// Model Paths
			if (auto error = NvAFX_SetString(fx.get(), NVAFX_PARAM_MODEL_PATH, _model_path_str.c_str());
				error != NVAFX_STATUS_SUCCESS) {
				snprintf(message_buffer, sizeof(message_buffer),
						 "{%02" PRIuMAX "} Failed to configure effect paths. (Code %08" PRIX32 ")\0", idx, error);
				throw std::runtime_error(message_buffer);
			}
			D_LOG("{%02" PRIuMAX "} Effect Path is now: '%s'.", idx, _model_path_str.c_str());
		}

		// Load effects.
		for (size_t idx = 0; idx < _fx.size(); idx++) {
			auto& fx = _fx.at(idx);

			if (auto error = NvAFX_Load(fx.get()); error != NVAFX_STATUS_SUCCESS) {
				snprintf(message_buffer, sizeof(message_buffer),
						 "{%02" PRIuMAX "} Failed to initialize effect.(Code %08" PRIX32 ").\0", idx, error);
				throw std::runtime_error(message_buffer);
			}
		}

		_fx_dirty = false;
	}

	if (_cfg_dirty) {
		auto cstk = _nvafx->cuda_context()->enter();

		for (auto& fx : _fx) {
#ifdef ENABLE_FULL_VERSION
			if (auto error = NvAFX_SetFloat(fx.get(), NVAFX_PARAM_INTENSITY_RATIO, _cfg_intensity);
				error != NVAFX_STATUS_SUCCESS) {
				snprintf(message_buffer, sizeof(message_buffer),
						 "Failed to configure intensity. (Code %08" PRIX32 ").\0", error);
				throw std::runtime_error(message_buffer);
			}
#endif
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
	std::vector<float*> channel_data(_cfg_channels, data.data());
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

	auto cstk = _nvafx->cuda_context()->enter();

	// Process all data passed in.
	const float* input_ptr;
	float*       output_ptr;
	for (size_t idx = 0; idx < _cfg_channels; idx++) {
		auto& fx = _fx[idx];
		for (size_t offset = 0; offset < samples; offset += blocksize()) {
			input_ptr  = input[idx] + offset;
			output_ptr = output[idx] + offset;

			if (auto error = NvAFX_Run(fx.get(), &input_ptr, &output_ptr, static_cast<unsigned int>(blocksize()), 1);
				error != NVAFX_STATUS_SUCCESS) {
				char buffer[1024];
				snprintf(buffer, sizeof(buffer), "{%02" PRIuMAX "} Failed to process audio. (Code %08" PRIX32 ").\0",
						 idx, error);
				throw std::runtime_error(buffer);
			}
		}
	}
}
