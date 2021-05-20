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

#include <vst.h>

#include "lib.hpp"
#include "vst2.hpp"
#include "vst2_denoiser.hpp"

#define D_LOG(MESSAGE, ...) voicefx::log("<VST2> " MESSAGE, __VA_ARGS__)

// Entry Points for different platforms.
extern "C" __declspec(dllexport) VST_ENTRYPOINT
{
	try {
		// Initialize VoiceFX library.
		voicefx::initialize();

		// Create an instance of the VST2.x effect, and return its internal structure.
		return (new voicefx::vst2::denoiser(callback))->get_effect_structure();
	} catch (std::exception const& ex) {
		voicefx::log("Exception: %s", ex.what());
		return nullptr;
	} catch (...) {
		voicefx::log("Unknown Exception.");
		return nullptr;
	}
}
extern "C" __declspec(dllexport) VST_ENTRYPOINT_WINDOWS;
extern "C" __declspec(dllexport) VST_ENTRYPOINT_MACOS;
