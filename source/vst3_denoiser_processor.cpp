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

FUnknown* vst3::denoiser::processor::create(void* data)
{
	return static_cast<IAudioProcessor*>(new processor());
}

vst3::denoiser::processor::processor() : _bypass(false)
{
	processContextRequirements.needContinousTimeSamples();
	processContextRequirements.needSamplesToNextClock();

	setControllerClass(vst3::denoiser::controller_uid);
}

vst3::denoiser::processor::~processor() {}

tresult PLUGIN_API vst3::denoiser::processor::initialize(FUnknown* context)
{
	if (auto res = AudioEffect::initialize(context); res != kResultOk) {
		return res;
	}

	addAudioInput(STR16("In"), SpeakerArr::kMono);
	addAudioOutput(STR16("Out"), SpeakerArr::kMono);
	return kResultOk;
}

tresult PLUGIN_API vst3::denoiser::processor::canProcessSampleSize(int32 symbolicSampleSize)
{
	return (symbolicSampleSize == kSample32) ? kResultTrue : kResultFalse;
}

tresult PLUGIN_API vst3::denoiser::processor::setBusArrangements(SpeakerArrangement* inputs, int32 numIns,
																 SpeakerArrangement* outputs, int32 numOuts)
{
	if (numIns < 0 || numOuts < 0) {
		return kInvalidArgument;
	}

	if (numIns > static_cast<int32>(audioInputs.size()) || numOuts > static_cast<int32>(audioOutputs.size())) {
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

		update_channel_count(channels);
	}

	return kResultTrue;
}

uint32 PLUGIN_API vst3::denoiser::processor::getLatencySamples()
{
	// In order to not glitch out, we need to delay things until we have 3x the required samples
	return processSetup.maxSamplesPerBlock << 1;
}

uint32 PLUGIN_API vst3::denoiser::processor::getTailSamples()
{
	return getLatencySamples();
}

tresult PLUGIN_API vst3::denoiser::processor::process(ProcessData& data)
{
	// Check for updates to parameters.
	if (data.inputParameterChanges != nullptr) {
		for (auto idx = 0; idx < data.inputParameterChanges->getParameterCount(); idx++) {
			if (auto* pq = data.inputParameterChanges->getParameterData(idx); pq) {
				int32      offset;
				ParamValue value;

				switch (static_cast<parameters>(pq->getParameterId())) {
				case parameters::BYPASS:
					if (pq->getPoint(pq->getPointCount() - 1, offset, value) == kResultOk) {
						_bypass = (value > 0.5);
					}
					break;
				}
			}
		}
	}

	// Check if there is any work to do.
	if ((data.numInputs == 0) || (data.numOutputs == 0)) {
		return kResultOk;
	}

	// Check if the output is silent, if so do some additional work and quit early.
	if (data.outputs[0].silenceFlags = data.inputs[0].silenceFlags; data.outputs[0].silenceFlags != 0) {
		for (size_t cdx = 0; cdx < data.outputs[0].numChannels; cdx++) {
			// Only need to clear output if it is not the same buffer.
			if (data.outputs[0].channelBuffers32[cdx] != data.inputs[0].channelBuffers32[cdx]) {
				memset(data.outputs[0].channelBuffers32[cdx], 0, sizeof(kSample32) * data.numSamples);
			}
		}

		return kResultOk;
	}

	// User wants to bypass the effect entirely.
	if (_bypass) {
		// Copy used data.
		size_t numPorts = std::min(data.numInputs, data.numOutputs);
		for (size_t idx = 0; idx < numPorts; idx++) {
			size_t numChannels = std::min(data.inputs[idx].numChannels, data.outputs[idx].numChannels);
			for (size_t cdx = 0; cdx < numChannels; cdx++) {
				if (data.outputs[idx].channelBuffers32[cdx] != data.inputs[idx].channelBuffers32[cdx]) {
					memcpy(data.outputs[idx].channelBuffers32[cdx], data.inputs[idx].channelBuffers32[cdx],
						   sizeof(kSample32) * data.numSamples);
				}
			}

			// Clear unused channels
			for (size_t cdx = numChannels; cdx < data.outputs[idx].numChannels; cdx++) {
				memset(data.outputs[idx].channelBuffers32[cdx], 0, sizeof(kSample32) * data.numSamples);
			}
		}

		// Clear unused outputs.
		for (size_t idx = numPorts; idx < data.numOutputs; idx++) {
			for (size_t cdx = 0; cdx < data.outputs[idx].numChannels; cdx++) {
				memset(data.outputs[idx].channelBuffers32[cdx], 0, sizeof(kSample32) * data.numSamples);
			}
		}

		return kResultOk;
	}

	auto itr = _channels.begin();
	for (size_t cdx = 0; cdx < data.inputs[0].numChannels; cdx++) {
		itr->process(data.numSamples, data.inputs[0].channelBuffers32[cdx], data.outputs[0].channelBuffers32[cdx]);

		itr++;
	}

	return kResultOk;
}

tresult PLUGIN_API vst3::denoiser::processor::setState(IBStream* state)
{
	if (state == nullptr) {
		return kResultFalse;
	}

	IBStreamer streamer(state, kBigEndian);
	if (!streamer.readBool(_bypass)) {
		return kResultFalse;
	}

	return kResultOk;
}

tresult PLUGIN_API vst3::denoiser::processor::getState(IBStream* state)
{
	if (state == nullptr) {
		return kResultFalse;
	}

	IBStreamer streamer(state, kBigEndian);
	if (!streamer.writeBool(_bypass)) {
		return kResultFalse;
	}

	return kResultOk;
}

void vst3::denoiser::processor::update_channel_count(size_t channels)
{
	// Channel magic
	_channels.clear();
	while (_channels.size() < channels) {
		_channels.emplace_back();
	}
	for (auto itr = _channels.begin(); itr != _channels.end(); itr++) {
		itr->reset(processSetup.sampleRate, processSetup.maxSamplesPerBlock);
	}
}
