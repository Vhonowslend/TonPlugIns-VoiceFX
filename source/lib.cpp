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
#include <cstdarg>
#include <iostream>
#include <vector>

#define LOG_PREFIX "[NVIDIA AudioFX VST] "

static std::filesystem::path _vst3_path;
static std::filesystem::path _sdk_path;

std::filesystem::path vst3_path()
{
	return std::filesystem::path(_vst3_path);
}

void set_vst3_path(std::filesystem::path value)
{
	_vst3_path = std::filesystem::path(value);
}

std::filesystem::path nvafx_path()
{
	return std::filesystem::path(_sdk_path);
}

void set_nvafx_path(std::filesystem::path value)
{
	_sdk_path = std::filesystem::path(value);
}

void do_log(const char* format, ...)
{
	std::vector<char> buf(65535);

	va_list args;
	va_start(args, format);
	buf.resize(static_cast<size_t>(vsnprintf(nullptr, 0, format, args)) + 1);
	vsnprintf(buf.data(), buf.size(), format, args);
	va_end(args);

	std::cerr << LOG_PREFIX << buf.data() << std::endl;
}
