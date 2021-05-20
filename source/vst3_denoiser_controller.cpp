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

#include "vst3_denoiser_controller.hpp"
#include <base/source/fstreamer.h>

#define D_LOG(MESSAGE, ...) voicefx::log("<VST3::Denoiser::Controller> " MESSAGE, __VA_ARGS__)

vst3::denoiser::controller::controller()
{
	D_LOG("(0x%08" PRIxPTR ") Initializing...", this);
}

vst3::denoiser::controller::~controller() {}

tresult PLUGIN_API vst3::denoiser::controller::initialize(FUnknown* context)
{
	if (tresult result = EditControllerEx1::initialize(context); result != kResultOk) {
		D_LOG("(0x%08" PRIxPTR ") Initialization failed with error code 0x%" PRIx32 ".", this,
			  static_cast<int32_t>(result));
		return result;
	}

	D_LOG("(0x%08" PRIxPTR ") Initialized.", this);
	return kResultOk;
}

tresult PLUGIN_API vst3::denoiser::controller::setComponentState(IBStream* state)
{
	if (state == nullptr) {
		return kResultFalse;
	}

	IBStreamer streamer(state, kBigEndian);

	return kResultOk;
}

tresult PLUGIN_API vst3::denoiser::controller::setChannelContextInfos(IAttributeList* list)
{
	return kResultOk;
}

FUnknown* vst3::denoiser::controller::create(void* data)
{
	return static_cast<IEditController*>(new controller());
}
