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

#include "util-platform.hpp"
#include "lib.hpp"

#ifdef _WIN32
#include <Windows.h>

#include <Shlobj.h>

#include <Knownfolders.h>
#include <WinNls.h>
#elif __APPLE__
#else
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#define D_LOG(MESSAGE, ...) voicefx::core->log("<Platform> " MESSAGE, __VA_ARGS__)

#ifdef _WIN32
std::string voicefx::util::platform::native_to_utf8(std::wstring const& v)
{
	std::vector<char> buffer((v.length() + 1) * 4, 0);

	DWORD res = WideCharToMultiByte(CP_UTF8, 0, v.data(), static_cast<int>(v.length()), buffer.data(), static_cast<int>(buffer.size()), nullptr, nullptr);
	if (res == 0) {
		D_LOG("Failed to convert '%ls' to UTF-8 format.", v.data());
		throw std::runtime_error("Failed to convert Windows-native to UTF-8.");
	}

	return {buffer.data()};
}

std::filesystem::path voicefx::util::platform::native_to_utf8(std::filesystem::path const& v)
{
	auto wide   = v.wstring();
	auto narrow = native_to_utf8(wide);
	return std::filesystem::path(narrow);
}

std::wstring voicefx::util::platform::utf8_to_native(std::string const& v)
{
	std::vector<wchar_t> buffer(v.length() + 1, 0);

	DWORD res = MultiByteToWideChar(CP_UTF8, 0, v.data(), static_cast<int>(v.length()), buffer.data(), static_cast<int>(buffer.size()));
	if (res == 0) {
		D_LOG("Failed to convert '%s' to native format.", v.data());
		throw std::runtime_error("Failed to convert UTF-8 to Windows-native.");
	}

	return {buffer.data()};
}

std::filesystem::path voicefx::util::platform::utf8_to_native(std::filesystem::path const& v)
{
	auto narrow = v.string();
	auto wide   = utf8_to_native(narrow);
	return std::filesystem::path(wide);
}
#endif

std::filesystem::path voicefx::util::platform::config_path()
{
	std::filesystem::path result;

#ifdef _WIN32
	PWSTR             widebuffer;
	std::vector<char> buffer;

	const GUID paths[] = {
		FOLDERID_RoamingAppData,
		FOLDERID_UserProgramFilesCommon,
		FOLDERID_Documents,
	};

	for (const GUID path : paths) {
		if (SHGetKnownFolderPath(path, 0, NULL, &widebuffer) == S_OK) {
			size_t wsz = static_cast<size_t>(wcslen(widebuffer));
			size_t sz  = static_cast<size_t>(WideCharToMultiByte(CP_UTF8, 0, widebuffer, static_cast<int>(wsz), 0, 0, 0, nullptr));
			buffer.resize(sz + 1);
			WideCharToMultiByte(CP_UTF8, 0, widebuffer, static_cast<int>(wsz), buffer.data(), static_cast<int>(buffer.size()), 0, nullptr);
			CoTaskMemFree(widebuffer);

			result = std::filesystem::path(std::string_view(buffer.data(), buffer.size() - 1));
			break;
		}
	}
#elif __APPLE__
	result = "~" / "Library" / "Preferences";
#else
	if (char* buffer = getenv("XDG_CONFIG_HOME"); buffer != nullptr) {
		result = std::filesystem::path(buffer);
	} else {
		if (char* buffer = getenv("HOME"); buffer != nullptr) {
			result = std::filesystem::path(buffer);
		} else {
			struct passwd* pw = getpwuid(getuid());
			result            = std::filesystem::path(pw->pw_dir);
		}
		result /= ".config";
	}
#endif

	// Always have somewhere to save files, even if it's temporary.
	if (result.empty())
		result = std::filesystem::temp_directory_path();

	return result;
}

std::filesystem::path voicefx::util::platform::data_path()
{
	std::filesystem::path result;

#ifdef _WIN32
	PWSTR             widebuffer;
	std::vector<char> buffer;

	const GUID paths[] = {
		FOLDERID_LocalAppData,
		FOLDERID_RoamingAppData,
		FOLDERID_UserProgramFilesCommon,
		FOLDERID_Documents,
	};

	for (const GUID path : paths) {
		if (SHGetKnownFolderPath(path, 0, NULL, &widebuffer) == S_OK) {
			size_t wsz = static_cast<size_t>(wcslen(widebuffer));
			size_t sz  = static_cast<size_t>(WideCharToMultiByte(CP_UTF8, 0, widebuffer, static_cast<int>(wsz), 0, 0, 0, nullptr));
			buffer.resize(sz + 1);
			WideCharToMultiByte(CP_UTF8, 0, widebuffer, static_cast<int>(wsz), buffer.data(), static_cast<int>(buffer.size()), 0, nullptr);
			CoTaskMemFree(widebuffer);

			result = std::filesystem::path(std::string_view(buffer.data(), buffer.size() - 1));
			break;
		}
	}
#elif __APPLE__
	result = "~" / "Library";
#else
	if (char* buffer = getenv("XDG_DATA_HOME"); buffer != nullptr) {
		result = std::filesystem::path(buffer);
	} else {
		if (char* buffer = getenv("HOME"); buffer != nullptr) {
			result = std::filesystem::path(buffer);
		} else {
			struct passwd* pw = getpwuid(getuid());
			result            = std::filesystem::path(pw->pw_dir);
		}
		result /= ".local";
		result /= "share";
	}
#endif

	// Always have somewhere to save files, even if it's temporary.
	if (result.empty())
		result = std::filesystem::temp_directory_path();

	return result;
}

std::filesystem::path voicefx::util::platform::cache_path()
{
	std::filesystem::path result;

#ifdef _WIN32
	result = data_path();
#elif __APPLE__
	result = "~" / "Library" / "Caches";
#else
	if (char* buffer = getenv("XDG_CACHE_HOME"); buffer != nullptr) {
		result = std::filesystem::path(buffer);
	} else {
		if (char* buffer = getenv("HOME"); buffer != nullptr) {
			result = std::filesystem::path(buffer);
		} else {
			struct passwd* pw = getpwuid(getuid());
			result            = std::filesystem::path(pw->pw_dir);
		}
		result /= ".cache";
	}
#endif

	// Always have somewhere to save files, even if it's temporary.
	if (result.empty())
		result = std::filesystem::temp_directory_path();

	return result;
}
