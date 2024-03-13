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

#include "vst3.hpp"
#include "lib.hpp"
#include "vst3_effect_controller.hpp"
#include "vst3_effect_processor.hpp"

#include "warning-disable.hpp"
#include <public.sdk/source/main/pluginfactory.h>
#include "warning-enable.hpp"

BEGIN_FACTORY_DEF("Xaymar", "https://xaymar.com/", "mailto:support@xaymar.com")

#ifndef TONPLUGINS_DEMO
#define VST_NAME ""
#else
#define VST_NAME " (Demo)"
#endif

DEF_VST3_CLASS("VoiceFX" VST_NAME, Vst::PlugType::kFxRestoration, Vst::kDistributable, TONPLUGINS_VOICEFX_VERSION, INLINE_UID_FROM_FUID(vst3::effect::processor_uid), vst3::effect::processor::create, INLINE_UID_FROM_FUID(vst3::effect::controller_uid), vst3::effect::controller::create);

END_FACTORY
