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

#include "vst3_effect_controller.hpp"
#include <base/source/fstreamer.h>

#define D_LOG(MESSAGE, ...) voicefx::core->log("<vst3::effect::controller> " MESSAGE, __VA_ARGS__)

vst3::effect::controller::controller()
{
	D_LOG("(0x%08" PRIxPTR ") Initializing...", this);

#ifndef TONPLUGINS_DEMO
	{
		auto p = new Steinberg::Vst::StringListParameter(STR("Mode"), PARAMETER_MODE, STR("Removal"));
		p->appendString(STR("Noise"));
		p->appendString(STR("Echo"));
		p->appendString(STR("Both"));
		parameters.addParameter(p);
	}
	{
		auto p = new Steinberg::Vst::RangeParameter(STR("Intensity"), PARAMETER_INTENSITY, STR("%"), 0.0, 100.0, 100.0, 0, Steinberg::Vst::ParameterInfo::ParameterFlags::kCanAutomate);
		//p->setPrecision(2);
		parameters.addParameter(p);
	}
#endif
}

vst3::effect::controller::~controller() {}

tresult PLUGIN_API vst3::effect::controller::initialize(FUnknown* context)
{
	if (tresult result = EditControllerEx1::initialize(context); result != kResultOk) {
		D_LOG("(0x%08" PRIxPTR ") Initialization failed with error code 0x%" PRIx32 ".", this, static_cast<int32_t>(result));
		return result;
	}

	D_LOG("(0x%08" PRIxPTR ") Initialized.", this);
	return kResultOk;
}

tresult PLUGIN_API vst3::effect::controller::setComponentState(IBStream* state)
{
	if (state == nullptr) {
		return kResultFalse;
	}

	IBStreamer streamer(state, kLittleEndian);
#ifndef TONPLUGINS_DEMO
	if (bool echo, reverb; streamer.readBool(echo) && streamer.readBool(reverb) == true) {
		if (echo && reverb) {
			setParamNormalized(PARAMETER_MODE, 1.);
		} else if (reverb) {
			setParamNormalized(PARAMETER_MODE, 0.5);
		} else {
			setParamNormalized(PARAMETER_MODE, 0.);
		}
	} else {
		return kResultFalse;
	}
	if (float value = 0; streamer.readFloat(value) == true) {
		setParamNormalized(PARAMETER_INTENSITY, value);
	} else {
		return kResultFalse;
	}
#endif

	return kResultOk;
}

tresult PLUGIN_API vst3::effect::controller::setChannelContextInfos(IAttributeList* list)
{
	return kResultOk;
}

FUnknown* vst3::effect::controller::create(void* data)
{
	return static_cast<IEditController*>(new controller());
}
