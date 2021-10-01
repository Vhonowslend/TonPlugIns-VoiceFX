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

#include "util-library.hpp"
#include <unordered_map>
#include "util-platform.hpp"

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__) // Windows
#define ST_WINDOWS
#else
#define ST_UNIX
#endif

#if defined(ST_WINDOWS)
#include <Windows.h>
#elif defined(ST_UNIX)
#include <dlfcn.h>
#endif

voicefx::util::library::library(std::filesystem::path file) : _library(nullptr)
{
#if defined(ST_WINDOWS)
	SetLastError(ERROR_SUCCESS);
	file     = ::voicefx::util::platform::utf8_to_native(file);
	_library = reinterpret_cast<void*>(LoadLibraryExW(
		file.wstring().c_str(), nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS));
	if (!_library) {
		DWORD error = GetLastError();
		if (error != ERROR_PROC_NOT_FOUND) {
			PSTR        message = NULL;
			std::string ex      = "Failed to load library.";
			FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER,
						   NULL, error, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPSTR)&message, 0, NULL);
			if (message) {
				ex = message;
				LocalFree(message);
				throw std::runtime_error(ex);
			}
		}
		throw std::runtime_error("Failed to load library.");
	}
#elif defined(ST_UNIX)
	_library = dlopen(file.u8string().c_str(), RTLD_LAZY);
	if (!_library) {
		if (char* error = dlerror(); error)
			throw std::runtime_error(error);
		else
			throw std::runtime_error("Failed to load library.");
	}
#endif
}

voicefx::util::library::~library()
{
#if defined(ST_WINDOWS)
	FreeLibrary(reinterpret_cast<HMODULE>(_library));
#elif defined(ST_UNIX)
	dlclose(_library);
#endif
}

void* voicefx::util::library::load_symbol(std::string_view name)
{
#if defined(ST_WINDOWS)
	return reinterpret_cast<void*>(GetProcAddress(reinterpret_cast<HMODULE>(_library), name.data()));
#elif defined(ST_UNIX)
	return reinterpret_cast<void*>(dlsym(_library, name.data()));
#endif
}

static std::unordered_map<std::string, std::weak_ptr<::voicefx::util::library>> libraries;

std::shared_ptr<::voicefx::util::library> voicefx::util::library::load(std::filesystem::path file)
{
	auto kv = libraries.find(file.u8string());
	if (kv != libraries.end()) {
		if (auto ptr = kv->second.lock(); ptr)
			return ptr;
		libraries.erase(kv);
	}

	auto ptr = std::make_shared<::voicefx::util::library>(file);
	libraries.emplace(file.u8string(), ptr);

	return ptr;
}

std::shared_ptr<::voicefx::util::library> voicefx::util::library::load(std::string_view name)
{
	return load(std::filesystem::u8path(name));
}
