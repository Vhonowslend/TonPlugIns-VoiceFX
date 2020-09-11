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

#ifdef WIN32
#include <Windows.h>

HMODULE              lib;
DLL_DIRECTORY_COOKIE ck;
#endif

bool InitModule()
{
	set_vst3_path(gPath);

#ifdef WIN32

	// Figure out possible NVAFX directories.
	{
		std::vector<wchar_t> buffer(0xFFFFull);
		buffer.resize(static_cast<size_t>(GetEnvironmentVariableW(L"NVAFX_SDK_DIR", buffer.data(), 0)) + 1);
		if (buffer.size() > 0) {
			GetEnvironmentVariableW(L"NVAFX_SDK_DIR", buffer.data(), buffer.size());
			set_nvafx_path(std::filesystem::path(std::wstring(buffer.data())));
		} else {
			set_nvafx_path(vst3_path());
		}
	}

	// Add DLL search directories.
	SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
	{
		// Add VST3 directory to the known search paths.
		ck = AddDllDirectory(vst3_path().wstring().c_str());
		if (!ck) {
			do_log("Failed to add library search directory, aborting.");
			return false;
		}

		// Add SDK directory.
		ck = AddDllDirectory(nvafx_path().wstring().c_str());
		if (!ck) {
			do_log("Failed to add library search directory, aborting.");
			return false;
		}

		// Attempt to load the library.
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

BEGIN_FACTORY_DEF("Michael Fabian 'Xaymar' Dirks", "https://xaymar.com/", "mailto:info@xaymar.com")

DEF_CLASS2(INLINE_UID_FROM_FUID(vst3::voicedenoiser::processor_uid),
		   PClassInfo::kManyInstances,            // Allow many instances
		   kVstAudioEffectClass,                  // Type
		   "NVIDIA Voice Noise Removal",          // Name
		   Vst::kDistributable,                   // Allow cross-computer usage.
		   "Fx",                                  // Categories (separate with |)
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
