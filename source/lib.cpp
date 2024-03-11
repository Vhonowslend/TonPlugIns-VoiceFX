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
#include <chrono>
#include <core.hpp>
#include <cstdarg>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <mutex>
#include <vector>

#if defined(_WIN32)
#include "Windows.h"

#include "Shlobj.h"

#include "Knownfolders.h"
#endif

namespace voicefx {
	std::shared_ptr<tonplugins::core> core;
}

void voicefx::initialize()
{
	static bool _initialized = false;
	if (_initialized) {
		return;
	}

	core = tonplugins::core::instance(std::string{voicefx::product_name});

#ifdef WIN32
	// Log information about the Host process.
	{
		std::vector<wchar_t> file_name_w(256, 0);
		size_t               file_name_len = 0;
		do {
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
				file_name_w.resize(file_name_w.size() * 2);
			}
			file_name_len = static_cast<DWORD>(GetModuleFileNameW(NULL, file_name_w.data(), static_cast<DWORD>(file_name_w.size())));
		} while (GetLastError() == ERROR_INSUFFICIENT_BUFFER);

		std::string file_name = std::filesystem::path(std::wstring(file_name_w.data(), file_name_w.data() + file_name_len)).generic_string();
		core->log("Host Process: %s (0x%08" PRIx32 ")", file_name.c_str(), GetCurrentProcessId());
	}
#endif

	_initialized = true;
}
