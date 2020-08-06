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
#include <pluginterfaces/vst/ivstchannelcontextinfo.h>
#include <public.sdk/source/vst/vsteditcontroller.h>
#include "lib.hpp"

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace xaymar::voice_denoiser {
	static const FUID controller_uid(FOURCC('X', 'm', 'r', 'V'), FOURCC('o', 'D', 'e', 'n'), FOURCC('o', 'i', 's', 'e'),
									 FOURCC('C', 't', 'l', 'r'));
	enum class parameters : ParamID {
		BYPASS,
	};

	class controller : public EditControllerEx1, public ChannelContext::IInfoListener {
		public:
		controller();
		virtual ~controller();

		public /* IPluginBase */:
		tresult PLUGIN_API initialize(FUnknown* context) override;

		tresult PLUGIN_API setComponentState(IBStream* state) override;

		tresult PLUGIN_API setChannelContextInfos(IAttributeList* list) override;

		public:
		OBJ_METHODS(controller, EditControllerEx1)
		DEFINE_INTERFACES
		DEF_INTERFACE(ChannelContext::IInfoListener)
		END_DEFINE_INTERFACES(EditController)
		DELEGATE_REFCOUNT(EditControllerEx1)

		public:
		static FUnknown* create(void* data);
	};

} // namespace xaymar::voice_denoiser
