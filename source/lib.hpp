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
#include <core.hpp>

#include "warning-disable.hpp"
#include <cinttypes>
#include <filesystem>
#include "warning-enable.hpp"

#define FOURCC(a, b, c, d) ((a << 24) | (b << 16) | (c << 8) | d)

#define D_LOG(MESSAGE, ...) voicefx::core->log("<0x%zx@%s> " MESSAGE, this, __FUNCTION_SIG__, __VA_ARGS__)
#define D_LOG_STATIC(MESSAGE, ...) voicefx::core->log("<%s> " MESSAGE, __FUNCTION_SIG__, __VA_ARGS__)

//#define QUIET
#ifndef QUIET
#define D_LOG_LOUD(MESSAGE, ...) D_LOG(MESSAGE, __VA_ARGS__)
#define D_LOG_STATIC_LOUD(MESSAGE, ...) D_LOG_STATIC(MESSAGE, __VA_ARGS__)
#else
#define D_LOG_LOUD(MESSAGE, ...)
#define D_LOG_STATIC_LOUD(MESSAGE, ...)
#endif

#define throw_log(MESSAGE, ...)                                 \
	{                                                           \
		char buffer[1024];                                      \
		snprintf(buffer, sizeof(buffer), MESSAGE, __VA_ARGS__); \
		D_LOG("throw '%s'", buffer);                            \
		throw std::runtime_error(buffer);                       \
	}
#define throw_log_static(MESSAGE, ...)                          \
	{                                                           \
		char buffer[1024];                                      \
		snprintf(buffer, sizeof(buffer), MESSAGE, __VA_ARGS__); \
		D_LOG_STATIC("throw '%s'", buffer);                     \
		throw std::runtime_error(buffer);                       \
	}

namespace voicefx {
	static constexpr std::string_view product_name   = "VoiceFX";
	static constexpr std::string_view product_vendor = "Xaymar";

	extern std::shared_ptr<tonplugins::core> core;

	void initialize();
} // namespace voicefx
