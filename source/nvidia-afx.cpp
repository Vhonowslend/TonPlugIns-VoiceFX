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

#include "nvidia-afx.hpp"
#include <mutex>
#include "lib.hpp"
#include "util-platform.hpp"

#include <nvAudioEffects.h>

#ifdef WIN32
#include <Windows.h>
#endif

#define D_LOG(MESSAGE, ...) voicefx::log("<NVAFX> " MESSAGE, __VA_ARGS__)

static std::filesystem::path find_nvafx_redistributable()
{
	{ // 1. Check the global NVAFX_SDK_DIR environment variable.
#ifdef WIN32
		std::vector<wchar_t> buffer;

		DWORD res = GetEnvironmentVariableW(L"NVAFX_SDK_DIR", buffer.data(), 0);
		if (res != 0) {
			buffer.resize(static_cast<size_t>(res) + 1);
			GetEnvironmentVariableW(L"NVAFX_SDK_DIR", buffer.data(), buffer.size());
			return std::filesystem::u8path(voicefx::util::platform::native_to_utf8(std::wstring(buffer.data())));
		}
#else
		throw std::runtime_error("This platform is currently not supported.");
#endif
	}

	{ // 2. If that failed, assume default path for the platform of choice.
#ifdef WIN32
		// TODO: Make this use KnownFolders instead.
		return std::filesystem::u8path(voicefx::util::platform::native_to_utf8(
			std::wstring(L"C:\\Program Files\\NVIDIA Corporation\\NVIDIA Audio Effects SDK")));
#else
		throw std::runtime_error("This platform is currently not supported.");
#endif
	}
}

nvidia::afx::afx::afx() : _redist_path(find_nvafx_redistributable())
{
	{ // Log where we found the redistributable at.
		std::string _loc = _redist_path.string();
		D_LOG("Found Redistributable at: %s", _loc.c_str());
	}

	{ // Load the actual NVIDIA Audio Effects library.
#ifdef WIN32
		SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);

		{ // Add the Redistributable directory to the list of known library directories.
			_dll_cookie = nullptr;
			DLL_DIRECTORY_COOKIE res =
				AddDllDirectory(voicefx::util::platform::utf8_to_native(_redist_path).wstring().data());
			if (res == NULL) {
				D_LOG("Unable to add redistributable path to library search paths, load may fail.");
			} else {
				_dll_cookie = reinterpret_cast<void*>(res);
			}
		}

		_library =
			reinterpret_cast<void*>(LoadLibraryExW(L"NVAudioEffects.dll", nullptr, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS));
		if (_library == nullptr) {
			D_LOG("Failed to load the NVIDIA Audio Effects library, nothing will be available.");
			throw std::runtime_error("Failed to load NVIDIA Audio Effects library.");
		}
#else
		throw std::runtime_error("This platform is currently not supported.");
#endif
	}

	D_LOG("Loaded NVIDIA Audio Effects library, these effects are available:");
	{
		int                   num = 0;
		NvAFX_EffectSelector* effects;
		NvAFX_GetEffectList(&num, &effects);
		for (size_t idx = 0; idx < num; idx++) {
			D_LOG("  %s", effects[idx]);
		}
	}
}

nvidia::afx::afx::~afx()
{
#ifdef WIN32
	RemoveDllDirectory(reinterpret_cast<DLL_DIRECTORY_COOKIE>(_dll_cookie));
	FreeLibrary(reinterpret_cast<HMODULE>(_library));
#endif
}

std::filesystem::path nvidia::afx::afx::redistributable_path()
{
	return _redist_path;
}

std::shared_ptr<::nvidia::afx::afx> nvidia::afx::afx::instance()
{
	static std::mutex                        _instance_guard;
	static std::weak_ptr<::nvidia::afx::afx> _instance;

	std::lock_guard<std::mutex>         lock(_instance_guard);
	std::shared_ptr<::nvidia::afx::afx> instance;

	if (!_instance.expired()) {
		instance = _instance.lock();
	} else {
		instance  = std::shared_ptr<::nvidia::afx::afx>(new ::nvidia::afx::afx());
		_instance = instance;
	}

	return instance;
}
