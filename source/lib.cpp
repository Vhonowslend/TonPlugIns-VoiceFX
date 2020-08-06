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

#include "lib.hpp"
#include <public.sdk/source/main/pluginfactory.h>
#include "version.h"
#include "voice_denoiser_controller.hpp"
#include "voice_denoiser_processor.hpp"

#include <iostream>
#include <varargs.h>

#ifdef WIN32
#include <Windows.h>

HMODULE              lib;
DLL_DIRECTORY_COOKIE ck;
#endif

bool InitModule()
{
#ifdef WIN32
	SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);

	{
		ck = AddDllDirectory(gPath);
		if (!ck) {
			do_log("Failed to add library search directory, aborting.");
			return false;
		}

		lib = LoadLibraryW(L"NVAudioEffects.dll");
		if (lib == NULL) {
			do_log("Failed to load library, aborting.");
			return false;
		}
	}
#endif

	return true;
}

bool DeinitModule()
{
	FreeLibrary(lib);
	RemoveDllDirectory(ck);
	return true;
}

void do_log(const char* format, ...)
{
	static thread_local std::vector<char> buf(65535);

	va_list args;
	va_start(args, format);
	vsnprintf(buf.data(), buf.size(), format, args);
	va_end(args);

	std::cerr << "[xmr-nvidia-audiofx] " << buf.data() << std::endl;
}

BEGIN_FACTORY_DEF("Michael Fabian 'Xaymar' Dirks", "https://xaymar.com/", "mailto:info@xaymar.com")

DEF_CLASS2(INLINE_UID_FROM_FUID(xaymar::voice_denoiser::processor_uid),
		   PClassInfo::kManyInstances,               // Allow many instances
		   kVstAudioEffectClass,                     // Type
		   "NVIDIA Voice Denoiser",                  // Name
		   Vst::kDistributable,                      // Allow cross-computer usage.
		   "Fx",                                     // Categories (separate with |)
		   XMRNVAFX_VERSION_STRING,                  // Version
		   kVstVersionString,                        // VST SDK Version
		   xaymar::voice_denoiser::processor::create // Function to create the instance.
)
DEF_CLASS2(INLINE_UID_FROM_FUID(xaymar::voice_denoiser::controller_uid),
		   PClassInfo::kManyInstances,                // Allow many instances
		   kVstComponentControllerClass,              // Type
		   "NVIDIA Voice Denoiser Controller",        // Name
		   0,                                         // Unused
		   "",                                        // Unused
		   XMRNVAFX_VERSION_STRING,                   // Version
		   kVstVersionString,                         // VST SDK Version
		   xaymar::voice_denoiser::controller::create // Function to create the instance.
)

END_FACTORY
