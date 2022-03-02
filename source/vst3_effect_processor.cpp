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

#include "vst3_effect_processor.hpp"
#include <base/source/fstreamer.h>
#include <filesystem>
#include <pluginterfaces/vst/ivstparameterchanges.h>
#include "vst3_effect_controller.hpp"

#define D_LOG(MESSAGE, ...) voicefx::log("<vst3::effect::processor> " MESSAGE, __VA_ARGS__)

FUnknown* vst3::effect::processor::create(void* data)
try {
	return static_cast<IAudioProcessor*>(new processor());
} catch (std::exception const& ex) {
	D_LOG("Exception in create: %s", ex.what());
	return nullptr;
} catch (...) {
	D_LOG("Unknown exception in create.");
	return nullptr;
}

vst3::effect::processor::processor()
	: _dirty(true), _in_resampler(), _fx(), _out_resampler(), _channels(), _delay(0), _local_delay(0)
{
	D_LOG("(0x%08" PRIxPTR ") Initializing...", this);

	// Assign the proper controller
	setControllerClass(vst3::effect::controller_uid);

	// Require some things from the host.
	processContextRequirements.needContinousTimeSamples();
	processContextRequirements.needSamplesToNextClock();

	// Allocate the necessary resources.
	_in_resampler  = std::make_shared<::voicefx::resampler>();
	_out_resampler = std::make_shared<::voicefx::resampler>();
	_fx            = std::make_shared<::nvidia::afx::effect>();
}

vst3::effect::processor::~processor() {}

tresult PLUGIN_API vst3::effect::processor::initialize(FUnknown* context)
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

tresult PLUGIN_API vst3::effect::processor::canProcessSampleSize(int32 symbolicSampleSize)
try {
	return (symbolicSampleSize == kSample32) ? kResultTrue : kResultFalse;
} catch (std::exception const& ex) {
	D_LOG("(0x%08" PRIxPTR ") Exception in canProcessSampleSize: %s", this, ex.what());
	return kInternalError;
} catch (...) {
	D_LOG("(0x%08" PRIxPTR ") Unknown exception in canProcessSampleSize.", this);
	return kInternalError;
}

tresult PLUGIN_API vst3::effect::processor::setBusArrangements(SpeakerArrangement* inputs, int32 numIns,
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

uint32 PLUGIN_API vst3::effect::processor::getLatencySamples()
try {
	return _delay;
} catch (std::exception const& ex) {
	D_LOG("(0x%08" PRIxPTR ") Exception in setBusArrangements: %s", this, ex.what());
	return kInternalError;
} catch (...) {
	D_LOG("(0x%08" PRIxPTR ") Unknown exception in setBusArrangements.", this);
	return kInternalError;
}

uint32 PLUGIN_API vst3::effect::processor::getTailSamples()
try {
	return _delay;
} catch (std::exception const& ex) {
	D_LOG("(0x%08" PRIxPTR ") Exception in getTailSamples: %s", this, ex.what());
	return kInternalError;
} catch (...) {
	D_LOG("(0x%08" PRIxPTR ") Unknown exception in getTailSamples.", this);
	return kInternalError;
}

tresult PLUGIN_API vst3::effect::processor::setupProcessing(ProcessSetup& newSetup)
try {
	// Copy non-important stuff.
	processSetup.maxSamplesPerBlock = newSetup.maxSamplesPerBlock;
	processSetup.processMode        = newSetup.processMode;

	// Check that this is the appropriate sample size.
	if (canProcessSampleSize(newSetup.symbolicSampleSize) != kResultTrue)
		return kResultFalse;
	processSetup.symbolicSampleSize = newSetup.symbolicSampleSize;

	// Check if we need to reset things due to configuration changes.
	if (processSetup.sampleRate != newSetup.sampleRate) {
		processSetup.sampleRate = newSetup.sampleRate;
		_dirty                  = true;
	}

	// TODO: Are we able to modify the host here?
	return kResultOk;
} catch (std::exception const& ex) {
	D_LOG("(0x%08" PRIxPTR ") Exception in setupProcessing: %s", this, ex.what());
	return kInternalError;
} catch (...) {
	D_LOG("(0x%08" PRIxPTR ") Unknown exception in setupProcessing.", this);
	return kInternalError;
}

tresult PLUGIN_API vst3::effect::processor::setProcessing(TBool state)
try {
	if ((state == TBool(true)) && _dirty) {
		reset();
	}

	return kResultOk;
} catch (std::exception const& ex) {
	D_LOG("(0x%08" PRIxPTR ") Exception in setProcessing: %s", this, ex.what());
	return kInternalError;
} catch (...) {
	D_LOG("(0x%08" PRIxPTR ") Unknown exception in setProcessing.", this);
	return kInternalError;
}

tresult PLUGIN_API vst3::effect::processor::process(ProcessData& data)
try {
	// Exit-early if there is nothing to process.
	if ((data.numInputs == 0) || (data.numOutputs == 0)) {
		return kResultOk;
	}

	// Exit-early if there is a mismatch between input and output channels.
	if (data.inputs[0].numChannels != data.outputs[0].numChannels) {
		return kResultFalse;
	}

	// Exit-early if the inputs mismatch our configuration.
	if ((data.inputs[0].numChannels != _channels.size()) || (data.outputs[0].numChannels != _channels.size())) {
		return kNotInitialized;
	}

	// Exit-early if host application ignores our delay request.
	if ((_local_delay == _delay) && (data.numSamples < _delay)) {
		D_LOG(
			"Host only provided %zu samples of the required %zu samples to overcome latency. Further behavior is "
			"undefined.",
			data.numSamples, _delay);
	}

	{ // Processing begins from here on out.
		bool                      resample = processSetup.sampleRate != ::nvidia::afx::effect::samplerate();
		std::vector<const float*> in_buffers(_channels.size(), nullptr);
		std::vector<float*>       out_buffers(_channels.size(), nullptr);

		// Push data into the (unresampled) input buffers.
		for (size_t idx = 0; idx < _channels.size(); idx++) {
			auto& channel = _channels[idx];
			if (resample) {
				channel.in_buffer.push(data.inputs[0].channelBuffers32[idx], data.numSamples);
				in_buffers[idx] = channel.in_buffer.peek(0);
			} else {
				channel.in_fx.push(data.inputs[0].channelBuffers32[idx], data.numSamples);
				in_buffers[idx] = channel.in_fx.peek(0);
			}
		}

		// Resample the input data to match the effect sample rate.
		if (resample) {
			// Update the output buffer pointers.
			for (size_t idx = 0; idx < _channels.size(); idx++) {
				auto& channel    = _channels[idx];
				out_buffers[idx] = channel.in_fx.back();
			}

			// Resample the given audio
			size_t in_samples  = _channels[0].in_buffer.size();
			size_t out_samples = _channels[0].in_fx.avail();
			_in_resampler->process(in_buffers.data(), in_samples, in_samples, out_buffers.data(), out_samples,
								   out_samples);

			// Update channel information and pointers.
			for (size_t idx = 0; idx < _channels.size(); idx++) {
				auto& channel = _channels[idx];

				// Pop the used samples from the input buffer.
				channel.in_buffer.pop(in_samples);

				// Push the generated samples into the output buffer.
				channel.in_fx.reserve(out_samples);

				// Update input buffer for next effect.
				in_buffers[idx] = channel.in_fx.peek(0);
			}
		}

		// Process as much data as possible.
		if (_channels[0].in_fx.avail() > ::nvidia::afx::effect::blocksize()) {
			// Calculate the total number of chunks that can be processed.
			size_t chunks =
				_channels[0].in_fx.size() - (_channels[0].in_fx.size() % ::nvidia::afx::effect::blocksize());

			// Update the output buffer pointers.
			for (size_t idx = 0; idx < _channels.size(); idx++) {
				auto& channel = _channels[idx];
				if (resample) {
					out_buffers[idx] = channel.out_fx.back();
				} else {
					out_buffers[idx] = channel.out_buffer.back();
				}
			}

			// Process data.
			_fx->process(in_buffers.data(), out_buffers.data(), chunks);

			// Update channel information and pointers.
			for (size_t idx = 0; idx < _channels.size(); idx++) {
				auto& channel = _channels[idx];

				// Pop the used samples from the input buffer.
				channel.in_fx.pop(chunks);

				// Push the generated samples into the output buffer.
				if (resample) {
					channel.out_fx.reserve(chunks);
					in_buffers[idx] = channel.out_fx.peek(0);
				} else {
					channel.out_buffer.reserve(chunks);
					in_buffers[idx] = channel.out_buffer.peek(0);
				}
			}
		}

		// Resample the input data to match the effect sample rate.
		if (resample) {
			// Update the output buffer pointers.
			for (size_t idx = 0; idx < _channels.size(); idx++) {
				auto& channel    = _channels[idx];
				out_buffers[idx] = channel.out_buffer.back();
			}

			// Resample the given audio
			size_t in_samples  = _channels[0].out_fx.size();
			size_t out_samples = _channels[0].out_buffer.avail();
			_out_resampler->process(in_buffers.data(), in_samples, in_samples, out_buffers.data(), out_samples,
									out_samples);

			// Update channel information and pointers.
			for (size_t idx = 0; idx < _channels.size(); idx++) {
				auto& channel = _channels[idx];

				// Pop the used samples from the input buffer.
				channel.out_fx.pop(in_samples);

				// Push the generated samples into the output buffer.
				channel.out_buffer.reserve(out_samples);

				// Update input buffer for next effect.
				in_buffers[idx] = channel.out_buffer.peek(0);
			}
		}

		// Output data.
		if ((_local_delay < data.numSamples) || (_local_delay == 0)) {
			// Return full or partial data.
			size_t offset = _local_delay;
			size_t length = data.numSamples - _local_delay;

			for (size_t idx = 0; idx < _channels.size(); idx++) {
				auto& channel = _channels[idx];
				memcpy(data.outputs[0].channelBuffers32[idx] + offset, channel.out_buffer.front(),
					   length * sizeof(float));
				channel.out_buffer.pop(length);
			}
		} else {
			// In all other cases, return nothing.
			for (size_t idx = 0; idx < _channels.size(); idx++) {
				memset(data.outputs[0].channelBuffers32[idx], 0, data.numSamples * sizeof(float));
			}
		}
		_local_delay = std::max<int64_t>(_local_delay - data.numSamples, 0);
	}

	return kResultOk;
} catch (std::exception const& ex) {
	D_LOG("(0x%08" PRIxPTR ") Exception in process: %s", this, ex.what());
	return kInternalError;
} catch (...) {
	D_LOG("(0x%08" PRIxPTR ") Unknown exception in process.", this);
	return kInternalError;
}

tresult PLUGIN_API vst3::effect::processor::setState(IBStream* state)
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

tresult PLUGIN_API vst3::effect::processor::getState(IBStream* state)
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

void vst3::effect::processor::reset()
{
	// Early-exit if the effect isn't "dirty".
	if (!_dirty) {
		return;
	}

	D_LOG("(0x%08" PRIxPTR ") Resetting...", this);

	// Update resamplers
	_in_resampler->ratio(processSetup.sampleRate, ::nvidia::afx::effect::samplerate());
	_out_resampler->ratio(::nvidia::afx::effect::samplerate(), processSetup.sampleRate);
	_in_resampler->clear();
	_out_resampler->clear();

	// Calculate absolute effect delay
	_delay = ::nvidia::afx::effect::delay();
	_delay += ::nvidia::afx::effect::blocksize();
	if (processSetup.sampleRate != ::nvidia::afx::effect::samplerate()) {
		_delay /= _in_resampler->ratio();
		_delay += ::voicefx::resampler::calculate_delay(processSetup.sampleRate, ::nvidia::afx::effect::samplerate());
		_delay += ::voicefx::resampler::calculate_delay(processSetup.sampleRate, ::nvidia::afx::effect::samplerate());
	}
	_delay       = 0;
	_local_delay = _delay;
	D_LOG("(0x%08" PRIxPTR ") Estimated latency is %" PRIu32 " samples.", this, _delay);

	// Update channel buffers.
	for (auto& channel : _channels) {
		channel.in_buffer.resize(processSetup.sampleRate);
		channel.in_fx.resize(::nvidia::afx::effect::samplerate());
		channel.out_fx.resize(::nvidia::afx::effect::samplerate());
		channel.out_buffer.resize(processSetup.sampleRate);
	}

	_dirty = false;
}

void vst3::effect::processor::set_channel_count(size_t num)
{
	D_LOG("(0x%08" PRIxPTR ") Adjusting effect channels to %" PRIuPTR "...", this, num);

	if (num != _channels.size()) {
		_dirty = true;

		_in_resampler->channels(num);
		_out_resampler->channels(num);
		_fx->channels(num);

		_channels.resize(num);
		_channels.shrink_to_fit();
	}
}
