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

#pragma once
#include <list>
#include "nvafx_voicedenoiser.hpp"
#include "vst3.hpp"

#include <nvAudioEffects.h>
#include <public.sdk/source/vst/vstaudioeffect.h>
#include <samplerate.h>

using namespace Steinberg;
using namespace Steinberg::Vst;

// Several assumptions just failed:
// - We can't tell the application that uses us which samplerate we need.
// - We can't tell the application that uses us how many samples per call we can process.
// - We can't tell the application anything. Who designed this?

namespace vst3::voicedenoiser {
	static const FUID processor_uid(FOURCC_CREATOR_PROCESSOR, // Creator, Type
									FOURCC('N', 'v', 'D', 'e'), FOURCC('n', 'o', 'i', 's'), FOURCC('e', 'V', 'o', 'c'));

	class processor : AudioEffect {
		bool _bypass;

		std::list<nvafx::voicedenoiser> _channels;

		public:
		processor();
		virtual ~processor();

		tresult PLUGIN_API initialize(FUnknown* context) override;

		tresult PLUGIN_API canProcessSampleSize(int32 symbolicSampleSize) override;

		tresult PLUGIN_API setBusArrangements(SpeakerArrangement* inputs, int32 numIns, SpeakerArrangement* outputs,
											  int32 numOuts) override;
		uint32 PLUGIN_API  getLatencySamples() override;
		uint32 PLUGIN_API  getTailSamples() override;

		tresult PLUGIN_API process(ProcessData& data) override;

		tresult PLUGIN_API setState(IBStream* state) override;
		tresult PLUGIN_API getState(IBStream* state) override;

		void update_channel_count(size_t ch);

		public:
		static FUnknown* create(void* data);
	};
} // namespace vst3::voicedenoiser
