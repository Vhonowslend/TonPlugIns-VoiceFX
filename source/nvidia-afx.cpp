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
			GetEnvironmentVariableW(L"NVAFX_SDK_DIR", buffer.data(), static_cast<DWORD>(buffer.size()));
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

nvidia::afx::afx::afx() : _redist_path(find_nvafx_redistributable()), _library(), _cuda(), _cuda_context()
{
#ifdef WIN32
	_d3d.reset();
	_dll_search_path.clear();
	_dll_cookie = nullptr;
#endif

	{ // Log where we found the redistributable at.
		std::string _loc = _redist_path.string();
		D_LOG("Found Redistributable at: %s", _loc.c_str());
	}

	try { // Find the CUDA device with the highest CUDA compatibility.
		_cuda = ::nvidia::cuda::cuda::get();

		::nvidia::cuda::device_t    best_device = -1;
		::nvidia::cuda::luid_t      best_luid;
		std::pair<int32_t, int32_t> best_cuda_version = {6, 0};
		{ // Enumerate and pick a default device.
			int32_t num_devices;
			if (auto v = _cuda->cuDeviceGetCount(&num_devices); v != ::nvidia::cuda::result::SUCCESS) {
				throw ::nvidia::cuda::exception(v);
			}
			D_LOG("Found %" PRId32 " CUDA capable devices:", num_devices);
			for (int32_t idx = static_cast<int32_t>(static_cast<int64_t>(num_devices) - 1); idx >= 0; idx--) {
				::nvidia::cuda::device_t    cur_device;
				std::vector<char>           cur_name(256, 0);
				uint32_t                    cur_node_mask;
				::nvidia::cuda::luid_t      cur_luid;
				std::pair<int32_t, int32_t> cur_cuda_version;

				// Get a reference to the device itself, if at all possible.
				if (auto v = _cuda->cuDeviceGet(&cur_device, idx); v != ::nvidia::cuda::result::SUCCESS) {
					continue;
				}

				// Query some device information.
				_cuda->cuDeviceGetName(cur_name.data(), static_cast<int32_t>(cur_name.size() - 1), cur_device);
				_cuda->cuDeviceGetLuid(&cur_luid, &cur_node_mask, cur_device);
				_cuda->cuDeviceGetAttribute(&cur_cuda_version.first,
											::nvidia::cuda::device_attribute::COMPUTE_CAPABILITY_MAJOR, cur_device);
				_cuda->cuDeviceGetAttribute(&cur_cuda_version.second,
											::nvidia::cuda::device_attribute::COMPUTE_CAPABILITY_MINOR, cur_device);

				// If this is the highest CUDA compatibility level so far, use this device.
				if ((cur_cuda_version.first > best_cuda_version.first)
					|| ((cur_cuda_version.first == best_cuda_version.first)
						&& (cur_cuda_version.second > best_cuda_version.second))
					|| ((cur_cuda_version.first == best_cuda_version.first)
						&& (cur_cuda_version.second >= best_cuda_version.second) && (best_device == -1))) {
					best_cuda_version = cur_cuda_version;
					best_device       = cur_device;
					best_luid         = cur_luid;
				}

				D_LOG("%4" PRId32 ": %s (%02" PRIx8 "%02" PRIx8 ":%02" PRIx8 "%02" PRIx8 ":%02" PRIx8 "%02" PRIx8
					  ":%02" PRIx8 "%02" PRIx8 ")",
					  idx, cur_name.data(), cur_luid.bytes[0], cur_luid.bytes[1], cur_luid.bytes[2], cur_luid.bytes[3],
					  cur_luid.bytes[4], cur_luid.bytes[5], cur_luid.bytes[6], cur_luid.bytes[7]);
			}
		}

#ifdef WIN32
		// Initialize a dummy D3D11 context to perhaps fool some functionality into working.
		_d3d = std::make_shared<::voicefx::windows::d3d::context>(best_luid);
#endif

		_cuda_context = std::make_shared<::nvidia::cuda::context>(best_device);
	} catch (...) {
		D_LOG("CUDA not available, some features may not work.");
	}

	// Enter the primary CUDA context, if available.
	std::shared_ptr<::nvidia::cuda::context_stack> ctx_stack;
	if (_cuda_context) {
		ctx_stack = _cuda_context->enter();
	}

	{ // Load the actual NVIDIA Audio Effects library.
#ifdef WIN32

		windows_fix_dll_search_paths();

		try {
			_library = ::voicefx::util::library::load(std::filesystem::path("NVAudioEffects.dll"));
		} catch (...) {
			try {
				_library = ::voicefx::util::library::load(std::filesystem::path(_redist_path) / "NVAudioEffects.dll");
			} catch (...) {
				D_LOG("Failed to load the NVIDIA Audio Effects library, nothing will be available.");
				throw std::runtime_error("Failed to load NVIDIA Audio Effects library.");
			}
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

std::shared_ptr<nvidia::cuda::context> nvidia::afx::afx::cuda_context()
{
	return _cuda_context;
}

void nvidia::afx::afx::windows_fix_dll_search_paths()
{
	// Set default look-up path to be System + Application + User + DLL-Load Dir
	SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR);

	// Generate the search path for later use.
	if (_dll_search_path.length() == 0) {
		auto path = voicefx::util::platform::utf8_to_native(_redist_path);
		//path += "\\";
		_dll_search_path = path.wstring();
	}

	// Specify search paths for LoadLibary.
	SetDllDirectoryW(_dll_search_path.c_str());

	// Generate a new DLL Directory Cookie for LoadLibraryEx.
	if (_dll_cookie) {
		RemoveDllDirectory(_dll_cookie);
	}
	_dll_cookie = AddDllDirectory(_dll_search_path.c_str());
	if (_dll_cookie == NULL) {
		D_LOG("Unable to add redistributable path to library search paths, load may fail.");
	}
}
