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
#include <public.sdk/source/main/pluginfactory.h>
#include "version.h"
#include "vst3_voicedenoiser_controller.hpp"
#include "vst3_voicedenoiser_processor.hpp"

#define D_LOG(MESSAGE, ...) voicefx::log("<VST2> " MESSAGE, __VA_ARGS__)

BEGIN_FACTORY_DEF("Xaymar", "https://xaymar.com/", "mailto:info@xaymar.com")

DEF_CLASS2(INLINE_UID_FROM_FUID(vst3::voicedenoiser::processor_uid),
		   PClassInfo::kManyInstances,            // Allow many instances
		   kVstAudioEffectClass,                  // Type
		   "NVIDIA Voice Noise Removal",          // Name
		   Vst::kDistributable,                   // Allow cross-computer usage.
		   Vst::PlugType::kFxRestoration,         // Categories (separate with |)
		   VERSION_STRING,                        // Version
		   kVstVersionString,                     // VST SDK Version
		   vst3::voicedenoiser::processor::create // Function to create the instance.
)
DEF_CLASS2(INLINE_UID_FROM_FUID(vst3::voicedenoiser::controller_uid),
		   PClassInfo::kManyInstances,              // Allow many instances
		   kVstComponentControllerClass,            // Type
		   "NVIDIA Voice Noise Removal Controller", // Name
		   0,                                       // Unused
		   "",                                      // Unused
		   VERSION_STRING,                          // Version
		   kVstVersionString,                       // VST SDK Version
		   vst3::voicedenoiser::controller::create  // Function to create the instance.
)

END_FACTORY

bool InitModule()
{
	try {
		// Initialize VoiceFX library.
		voicefx::initialize();
		D_LOG("Initializing...");

		// Adjust the path to the VST 3 plugin.
		set_vst3_path(gPath);

		// Log that we are now ready to perform work.
		D_LOG("Ready to work.");

		// Return true to signal the VST 3 hosts that everything is fine.
		return true;
	} catch (std::exception const& ex) {
		voicefx::log("Exception: %s", ex.what());
		return false;
	}
}

bool DeinitModule()
{
	return true;
}
