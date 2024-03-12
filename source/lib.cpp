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
#include "version.hpp"
#include <core.hpp>

#include "warning-disable.hpp"
#include <chrono>
#include <cstdarg>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <mutex>
#include <vector>
#include "warning-enable.hpp"

#include "warning-disable.hpp"
#if defined(_WIN32)
#include "Windows.h"

#include "Shlobj.h"

#include "Knownfolders.h"
#endif
#include "warning-enable.hpp"

#include "warning-disable.hpp"
#include <public.sdk/source/main/moduleinit.cpp>
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <public.sdk/source/main/dllmain.cpp>
#elif __APPLE__
#include <public.sdk/source/main/macmain.cpp>
#else
#include <public.sdk/source/main/linuxmain.cpp>
#endif
#include "warning-enable.hpp"

namespace voicefx {
	std::shared_ptr<tonplugins::core> core;
}

void voicefx::initialize()
{
	static bool _initialized = false;
	if (_initialized) {
		return;
	}

	core         = tonplugins::core::instance(std::string{voicefx::product_name});
	_initialized = true;
	D_LOG_LOUD("");
}

auto core = Steinberg::ModuleInitializer(
	[]() {
		try {
			// Initialize VoiceFX library.
			voicefx::initialize();

		} catch (std::exception const& ex) {
			voicefx::core->log("Exception: %s", ex.what());
			throw;
		} catch (...) {
			voicefx::core->log("Unknown Exception.");
			throw;
		}
	},
	0);
