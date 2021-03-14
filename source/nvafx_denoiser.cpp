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

#include "nvafx_denoiser.hpp"
#include "lib.hpp"
#include "platform.hpp"

#include <nvAudioEffects.h>

#define SAMPLERATE 48000
#define BLOCKSIZE SAMPLERATE / 100

#define D_LOG(MESSAGE, ...) voicefx::log("<NVAFX::Denoiser> " MESSAGE, __VA_ARGS__)

nvafx::denoiser::denoiser() : _nvafx(nvafx::nvafx::instance()), _model_path(), _nvfx()
{
	// 1. Create the NvAFX effect.
	NvAFX_Handle effect;
	if (auto res = NvAFX_CreateEffect(NVAFX_EFFECT_DENOISER, &effect); res != NVAFX_STATUS_SUCCESS) {
		D_LOG("Failed to create '" NVAFX_EFFECT_DENOISER "' effect, error code %" PRIu32 ".", res);
		throw std::runtime_error("Failed to create effect.");
	}
	_nvfx = std::shared_ptr<void>(effect, [](void* p) { NvAFX_DestroyEffect(reinterpret_cast<NvAFX_Handle>(p)); });

	// 2. Tell the effect where it can find the necessary models for operation.
	_model_path = _nvafx->redistributable_path();
	_model_path.append("models/denoiser_48k.trtpkg"); // TODO: Figure out why we need to specify the exact model.
	_model_path = std::filesystem::absolute(_model_path);
	if (auto res = NvAFX_SetString(_nvfx.get(), NVAFX_PARAM_DENOISER_MODEL_PATH, _model_path.string().c_str());
		res != NVAFX_STATUS_SUCCESS) {
		D_LOG("Unable to set appropriate model path, error code %" PRIu32 ".", res);
		throw std::runtime_error("Failed to set up effect.");
	}

	// 3. Set it up for the appropriate high quality sample rate.
	if (auto res = NvAFX_SetU32(_nvfx.get(), NVAFX_PARAM_DENOISER_SAMPLE_RATE, SAMPLERATE);
		res != NVAFX_STATUS_SUCCESS) {
		D_LOG("Failed to set sample rate to 48kHz, error code %" PRIu32 ".", res);
		throw std::runtime_error("Failed to set up effect.");
	}

	// 4. Set some defaults which we don't care about failing.
	//NvAFX_SetU32(_nvfx.get(), NVAFX_PARAM_USE_DEFAULT_GPU, 1);
	NvAFX_SetFloat(_nvfx.get(), NVAFX_PARAM_DENOISER_INTENSITY_RATIO, 1.0);

	// Finally, load the effect.
	if (auto res = NvAFX_Load(_nvfx.get()); res != NVAFX_STATUS_SUCCESS) {
		D_LOG("Failed to load effect, error code %" PRIu32 ".", res);
		throw std::runtime_error("Failed to load effect.");
	}

	// Retrieve the expected block size for processing.
	if (auto res = NvAFX_GetU32(_nvfx.get(), NVAFX_PARAM_DENOISER_NUM_SAMPLES_PER_FRAME, &_block_size);
		res != NVAFX_STATUS_SUCCESS) {
		D_LOG("Failed to retrieve expected block size, error code %" PRIu32 ".", res);
		_block_size = BLOCKSIZE; // Assume 10ms.
	}
}

nvafx::denoiser::~denoiser() {}

uint32_t nvafx::denoiser::get_sample_rate()
{
	return SAMPLERATE;
}

uint32_t nvafx::denoiser::get_block_size()
{
	return _block_size;
}

void nvafx::denoiser::process(const std::vector<std::vector<float>> inputs, std::vector<std::vector<float>> outputs)
{
	// Check if the number of input and output channels match.
	if (inputs.size() != outputs.size()) {
		throw std::runtime_error("Number of input channels must match number of output channels.");
	}

	// Check if the input buffers have the correct number of samples in total (block size).
	for (size_t idx = 0; idx < inputs.size(); idx++) {
		if (inputs[idx].size() != _block_size) {
			throw std::runtime_error("Input channels must be the required block size.");
		}

		// Also forcefully resize all outputs to be of the expected size.
		// TODO: Perhaps consider this as a failure condition as well?
		outputs[idx].resize(_block_size);
	}

	// Process all channels at once.
	for (size_t idx = 0; idx < inputs.size(); idx++) {
		this->process(inputs[idx].data(), outputs[idx].data());
	}
}

void nvafx::denoiser::process(const float* input[], float* output[], size_t channels)
{
	if (auto res = NvAFX_Run(_nvfx.get(), input, output, _block_size, channels); res != NVAFX_STATUS_SUCCESS) {
		D_LOG("Running effect returned error code %" PRIu32 ".", res);
		throw std::runtime_error("Failed to process buffers.");
	}
}

void nvafx::denoiser::process(const float input[], float output[])
{
	if (auto res = NvAFX_Run(_nvfx.get(), &input, &output, _block_size, 1); res != NVAFX_STATUS_SUCCESS) {
		D_LOG("Running effect returned error code %" PRIu32 ".", res);
		throw std::runtime_error("Failed to process buffers.");
	}
}
