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

#include "vst3_denoiser_processor.hpp"
#include <base/source/fstreamer.h>
#include <filesystem>
#include <pluginterfaces/vst/ivstparameterchanges.h>
#include "vst3_denoiser_controller.hpp"

#define D_LOG(MESSAGE, ...) voicefx::log("<VST3::Denoiser::Processor> " MESSAGE, __VA_ARGS__)

FUnknown* vst3::denoiser::processor::create(void* data)
try {
	return static_cast<IAudioProcessor*>(new processor());
} catch (std::exception const& ex) {
	D_LOG("Exception in create: %s", ex.what());
	return nullptr;
} catch (...) {
	D_LOG("Unknown exception in create.");
	return nullptr;
}

vst3::denoiser::processor::processor()
	: _dirty(true), _samplerate(), _blocksize(), _delaysamples(), _channels(), _scratch(0, nvafx::denoiser::get_sample_rate())
{
	D_LOG("(0x%08" PRIxPTR ") Initializing...", this);

	// Assign the proper controller
	setControllerClass(vst3::denoiser::controller_uid);

	// Require some things from the host.
	processContextRequirements.needContinousTimeSamples();
	processContextRequirements.needSamplesToNextClock();
	processContextRequirements.needFrameRate();

	// Load and initialize NVIDIA Audio Effects.
	_nvafx = nvafx::nvafx::instance();
}

vst3::denoiser::processor::~processor() {}

tresult PLUGIN_API vst3::denoiser::processor::initialize(FUnknown* context)
try {
	if (auto res = AudioEffect::initialize(context); res != kResultOk) {
		D_LOG("(0x%08" PRIxPTR ") Initialization failed with error code 0x%" PRIx32 ".", this,
			  static_cast<int32_t>(res));
		return res;
	}

	// Add audio input and output which default to mono.
	addAudioInput(STR16("In"), SpeakerArr::kMono);
	addAudioOutput(STR16("Out"), SpeakerArr::kMono);

	// Reset the channel layout to the defined one.
	set_channel_count(1);

	D_LOG("(0x%08" PRIxPTR ") Initialized.", this);
	return kResultOk;
} catch (std::exception const& ex) {
	D_LOG("(0x%08" PRIxPTR ") Exception in initialize: %s", this, ex.what());
	return kInternalError;
} catch (...) {
	D_LOG("(0x%08" PRIxPTR ") Unknown exception in initialize.", this);
	return kInternalError;
}

tresult PLUGIN_API vst3::denoiser::processor::canProcessSampleSize(int32 symbolicSampleSize)
try {
	return (symbolicSampleSize == kSample32) ? kResultTrue : kResultFalse;
} catch (std::exception const& ex) {
	D_LOG("(0x%08" PRIxPTR ") Exception in canProcessSampleSize: %s", this, ex.what());
	return kInternalError;
} catch (...) {
	D_LOG("(0x%08" PRIxPTR ") Unknown exception in canProcessSampleSize.", this);
	return kInternalError;
}

tresult PLUGIN_API vst3::denoiser::processor::setBusArrangements(SpeakerArrangement* inputs, int32 numIns,
																 SpeakerArrangement* outputs, int32 numOuts)
try {
	if (numIns < 0 || numOuts < 0) {
		D_LOG("(0x%08" PRIxPTR ") Host called setBusArrangement with no inputs or outputs!", this);
		return kInvalidArgument;
	}

	if (numIns > static_cast<int32>(audioInputs.size()) || numOuts > static_cast<int32>(audioOutputs.size())) {
		D_LOG("(0x%08" PRIxPTR ") Host called setBusArrangement with more than the maximum allowed inputs or outputs!",
			  this);
		return kResultFalse;
	}

	for (int32 index = 0; index < static_cast<int32>(audioInputs.size()); ++index) {
		if (index >= numIns) {
			break;
		}
		FCast<Vst::AudioBus>(audioInputs[index].get())->setArrangement(inputs[index]);
		if (index < static_cast<int32>(audioOutputs.size())) {
			FCast<Vst::AudioBus>(audioOutputs[index].get())->setArrangement(inputs[index]);
		}
	}

	{ // Update our own channel information.
		// Count channels.
		SpeakerArrangement arr      = inputs[0];
		size_t             channels = 0;
		while (arr > 0) {
			if ((arr & 0x1) != 0) {
				channels++;
			}
			arr >>= 1;
		}

		set_channel_count(channels);
	}

	return kResultTrue;
} catch (std::exception const& ex) {
	D_LOG("(0x%08" PRIxPTR ") Exception in setBusArrangements: %s", this, ex.what());
	return kInternalError;
} catch (...) {
	D_LOG("(0x%08" PRIxPTR ") Unknown exception in setBusArrangements.", this);
	return kInternalError;
}

uint32 PLUGIN_API vst3::denoiser::processor::getLatencySamples()
try {
	return _channels[0].fx->get_minimum_delay();
} catch (std::exception const& ex) {
	D_LOG("(0x%08" PRIxPTR ") Exception in setBusArrangements: %s", this, ex.what());
	return kInternalError;
} catch (...) {
	D_LOG("(0x%08" PRIxPTR ") Unknown exception in setBusArrangements.", this);
	return kInternalError;
}

uint32 PLUGIN_API vst3::denoiser::processor::getTailSamples()
try {
	return getLatencySamples();
} catch (std::exception const& ex) {
	D_LOG("(0x%08" PRIxPTR ") Exception in getTailSamples: %s", this, ex.what());
	return kInternalError;
} catch (...) {
	D_LOG("(0x%08" PRIxPTR ") Unknown exception in getTailSamples.", this);
	return kInternalError;
}

tresult PLUGIN_API vst3::denoiser::processor::process(ProcessData& data)
try {
	// Are there any inputs and outputs to process?
	if ((data.numInputs == 0) || (data.numOutputs == 0)) {
		return kResultOk;
	}

	for (size_t idx = 0; idx < _channels.size(); idx++) {
		auto& channel = _channels[idx];

		if (data.numSamples < channel.delay) {
			D_LOG("(0x%08" PRIxPTR ")[%" PRIuPTR
				  "] Host application is broken and ignores delay, behavior is now undefined.",
				  this, idx);
		}

#ifdef DEBUG_PROCESSING
		D_LOG("(0x%08" PRIxPTR ")[%" PRIuPTR "] In Smp. | In Buf | FX Buf | OutBuf | Step   ", this, idx);
		D_LOG("(0x%08" PRIxPTR ")[%" PRIuPTR "] --------+--------+--------+--------+--------", this, idx);
		D_LOG("(0x%08" PRIxPTR ")[%" PRIuPTR "] %7" PRId32 " |%7" PRIuPTR " |%7" PRIuPTR " |%7" PRIuPTR " |%7" PRIuPTR,
			  this, idx, data.numSamples, channel.input_buffer.size(), channel.fx_buffer.size(),
			  channel.output_buffer.size(), 0);
#endif

		// Resample input data for current channel to effect sample rate.
		size_t out_samples = _scratch.size();
		if (_samplerate != channel.fx->get_sample_rate()) {
			size_t in_samples = data.numSamples;
			channel.input_resampler.process(data.inputs[0].channelBuffers32[idx], in_samples, _scratch.data(),
											out_samples);
#ifdef DEBUG_PROCESSING
			D_LOG("(0x%08" PRIxPTR ")[%" PRIuPTR "] Host -> FX Resample!", this, idx);
#endif
			if (in_samples != data.numSamples) {
				D_LOG("(0x%08" PRIxPTR ")[%" PRIuPTR "] Host -> FX Resample required less input samples, broken!", this,
					  idx);
			}
		} else {
			// Copy the input if sample rate matches.
			out_samples = data.numSamples;
			memcpy(_scratch.data(), data.inputs[0].channelBuffers32[idx], out_samples * sizeof(float));
		}
		channel.input_buffer.push(_scratch.data(), out_samples);
#ifdef DEBUG_PROCESSING
		D_LOG("(0x%08" PRIxPTR ")[%" PRIuPTR "] %7" PRId32 " |%7" PRIuPTR " |%7" PRIuPTR " |%7" PRIuPTR " |%7" PRIuPTR,
			  this, idx, data.numSamples, channel.input_buffer.size(), channel.fx_buffer.size(),
			  channel.output_buffer.size(), out_samples);
#endif

		// Denoise the content in segments of _nvfx->get_block_size()
		out_samples = channel.fx->get_block_size();
		while (channel.input_buffer.size() >= out_samples) {
			// 1. Push _nvfx->get_block_size() samples to the processor.
			channel.fx->process(channel.input_buffer.peek(out_samples), _scratch.data());

			// 2. Store the result in a buffer.
			channel.fx_buffer.push(_scratch.data(), out_samples);

			// 3. And remove the original sample data from the input buffer.
			channel.input_buffer.pop(out_samples);
#ifdef DEBUG_PROCESSING
			D_LOG("(0x%08" PRIxPTR ")[%" PRIuPTR "] %7" PRId32 " |%7" PRIuPTR " |%7" PRIuPTR " |%7" PRIuPTR
				  " |%7" PRIuPTR,
				  this, idx, data.numSamples, channel.input_buffer.size(), channel.fx_buffer.size(),
				  channel.output_buffer.size(), out_samples);
#endif
		}
#ifdef DEBUG_PROCESSING
		D_LOG("(0x%08" PRIxPTR ")[%" PRIuPTR "] %7" PRId32 " |%7" PRIuPTR " |%7" PRIuPTR " |%7" PRIuPTR " |%7" PRIuPTR,
			  this, idx, data.numSamples, channel.input_buffer.size(), channel.fx_buffer.size(),
			  channel.output_buffer.size(), 0);
#endif

		if (channel.fx_buffer.size() > 0) {
			// Resample the entirety of the processed data.
			out_samples = _scratch.size();
			if (_samplerate != channel.fx->get_sample_rate()) {
				size_t in_samples = channel.fx_buffer.size();
				channel.output_resampler.process(channel.fx_buffer.peek(in_samples), in_samples, _scratch.data(),
												 out_samples);
				channel.fx_buffer.pop(channel.fx_buffer.size());
#ifdef DEBUG_PROCESSING
				D_LOG("(0x%08" PRIxPTR ")[%" PRIuPTR "] FX -> Host Resample!", this, idx);
#endif
			} else {
				// Copy the input if sample rate matches.
				out_samples = channel.fx_buffer.size();
				memcpy(_scratch.data(), channel.fx_buffer.peek(out_samples), out_samples * sizeof(float));
				channel.fx_buffer.pop(out_samples);
			}
			channel.output_buffer.push(_scratch.data(), out_samples);
#ifdef DEBUG_PROCESSING
			D_LOG("(0x%08" PRIxPTR ")[%" PRIuPTR "] %7" PRId32 " |%7" PRIuPTR " |%7" PRIuPTR " |%7" PRIuPTR
				  " |%7" PRIuPTR,
				  this, idx, data.numSamples, channel.input_buffer.size(), channel.fx_buffer.size(),
				  channel.output_buffer.size(), out_samples);
#endif
		} else {
#ifdef DEBUG_PROCESSING
			D_LOG("(0x%08" PRIxPTR ")[%" PRIuPTR "] %7" PRId32 " |%7" PRIuPTR " |%7" PRIuPTR " |%7" PRIuPTR
				  " |%7" PRIuPTR,
				  this, idx, data.numSamples, channel.input_buffer.size(), channel.fx_buffer.size(),
				  channel.output_buffer.size(), 0);
#endif
		}

		// Copy the output data back to the host.
		if (channel.delay <= 0) {
			// Update the output buffer with the new content.
			memcpy(data.outputs[0].channelBuffers32[idx], channel.output_buffer.peek(data.numSamples),
				   data.numSamples * sizeof(float));
			channel.output_buffer.pop(data.numSamples);
#ifdef DEBUG_PROCESSING
			D_LOG("(0x%08" PRIxPTR ")[%" PRIuPTR "] %7" PRId32 " |%7" PRIuPTR " |%7" PRIuPTR " |%7" PRIuPTR
				  " |%7" PRIuPTR,
				  this, idx, data.numSamples, channel.input_buffer.size(), channel.fx_buffer.size(),
				  channel.output_buffer.size(), data.numSamples);
#endif
		} else {
			memset(data.outputs[0].channelBuffers32[idx], 0, data.numSamples * sizeof(float));
			channel.delay -= data.numSamples;
#ifdef DEBUG_PROCESSING
			D_LOG("(0x%08" PRIxPTR ")[%" PRIuPTR "] %7" PRId32 " |%7" PRIuPTR " |%7" PRIuPTR " |%7" PRIuPTR
				  " |%7" PRIuPTR,
				  this, idx, data.numSamples, channel.input_buffer.size(), channel.fx_buffer.size(),
				  channel.output_buffer.size(), 0);
#endif
		}
	}

	return kResultOk;
} catch (std::exception const& ex) {
	D_LOG("(0x%08" PRIxPTR ") Exception in process: %s", this, ex.what());
	return kInternalError;
} catch (...) {
	D_LOG("(0x%08" PRIxPTR ") Unknown exception in process.", this);
	return kInternalError;
}

tresult PLUGIN_API vst3::denoiser::processor::setState(IBStream* state)
try {
	if (state == nullptr) {
		return kResultFalse;
	}

	return kResultOk;
} catch (std::exception const& ex) {
	D_LOG("(0x%08" PRIxPTR ") Exception in setState: %s", this, ex.what());
	return kInternalError;
} catch (...) {
	D_LOG("(0x%08" PRIxPTR ") Unknown exception in setState.", this);
	return kInternalError;
}

tresult PLUGIN_API vst3::denoiser::processor::getState(IBStream* state)
try {
	if (state == nullptr) {
		return kResultFalse;
	}

	return kResultOk;
} catch (std::exception const& ex) {
	D_LOG("(0x%08" PRIxPTR ") Exception in getState: %s", this, ex.what());
	return kInternalError;
} catch (...) {
	D_LOG("(0x%08" PRIxPTR ") Unknown exception in getState.", this);
	return kInternalError;
}

void vst3::denoiser::processor::reset()
{
	D_LOG("(0x%08" PRIxPTR ") Resetting channel states...", this);

	for (auto& channel : _channels) {
		channel.fx->reset();

		// (Re-)Create the re-samplers.
		channel.input_resampler.reset(_samplerate, nvafx::denoiser::get_sample_rate());
		channel.output_resampler.reset(nvafx::denoiser::get_sample_rate(), _samplerate);

		// (Re-)Create the buffers and reset offsets.
		channel.input_buffer.resize(nvafx::denoiser::get_sample_rate());
		channel.fx_buffer.resize(nvafx::denoiser::get_sample_rate());
		channel.output_buffer.resize(_samplerate);

		// Clear Buffers
		channel.input_buffer.clear();
		channel.fx_buffer.clear();
		channel.output_buffer.clear();

		// Reset delay
		channel.delay = _delaysamples;
	}
}

void vst3::denoiser::processor::set_channel_count(size_t num)
{
	D_LOG("(0x%08" PRIxPTR ") Adjusting effect channels to %" PRIuPTR "...", this, num);

	_channels.resize(num);
	_channels.shrink_to_fit();

	// Create any new effect instances.
	for (auto& channel : _channels) {
		if (!channel.fx) {
			channel.fx = std::make_shared<::nvafx::denoiser>();
		}
	}

	reset();
}
