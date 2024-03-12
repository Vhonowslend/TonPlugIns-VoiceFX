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

#define D_LOG(MESSAGE, ...) voicefx::core->log("<NVAFX> " MESSAGE, __VA_ARGS__)

static std::filesystem::path find_nvafx_redistributable()
{
	{ // 1. Check the global NVAFX_SDK_DIR environment variable.
#ifdef WIN32
		std::vector<wchar_t> buffer;

		DWORD res = GetEnvironmentVariableW(L"NVAFX_SDK_DIR", buffer.data(), 0);
		if (res != 0) {
			buffer.resize(static_cast<size_t>(res) + 1);
			GetEnvironmentVariableW(L"NVAFX_SDK_DIR", buffer.data(), static_cast<DWORD>(buffer.size()));
			return std::filesystem::path(voicefx::util::platform::native_to_utf8(std::wstring(buffer.data())));
		}
#else
		throw std::runtime_error("This platform is currently not supported.");
#endif
	}

	{ // 2. If that failed, assume default path for the platform of choice.
#ifdef WIN32
		// TODO: Make this use KnownFolders instead.
		return std::filesystem::path(voicefx::util::platform::native_to_utf8(std::wstring(L"C:\\Program Files\\NVIDIA Corporation\\NVIDIA Audio Effects SDK")));
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

	{ // Load all functions.
#define P_AFX_LOAD_SYMBOL(V) V = reinterpret_cast<decltype(V)>(_library->load_symbol("NvAFX_" #V));
		P_AFX_LOAD_SYMBOL(GetEffectList);
		P_AFX_LOAD_SYMBOL(CreateEffect);
		P_AFX_LOAD_SYMBOL(DestroyEffect);
		P_AFX_LOAD_SYMBOL(SetU32);
		P_AFX_LOAD_SYMBOL(SetString);
		P_AFX_LOAD_SYMBOL(SetFloat);
		P_AFX_LOAD_SYMBOL(GetU32);
		P_AFX_LOAD_SYMBOL(GetString);
		P_AFX_LOAD_SYMBOL(GetFloat);
		P_AFX_LOAD_SYMBOL(Load);
		P_AFX_LOAD_SYMBOL(GetSupportedDevices);
		P_AFX_LOAD_SYMBOL(Run);
		P_AFX_LOAD_SYMBOL(Reset);
#undef P_AFX_LOAD_SYMBOL
	}

	{ // Log all available effects.
		D_LOG("Loaded NVIDIA Audio Effects library, these effects are available:");
		int                   num = 0;
		NvAFX_EffectSelector* effects;
		GetEffectList(&num, &effects);
		for (size_t idx = 0; idx < num; idx++) {
			D_LOG("  %s", effects[idx]);
		}
	}

#ifndef DEFAULT_CONTEXT
#ifndef PRIMARY_CONTEXT
	try { // Figure out the ideal device to run the effects on.
		auto devices = enumerate_devices();
		struct {
			::nvidia::cuda::device_t device;
			::nvidia::cuda::luid_t   luid;
			float                    score;
		} ideal = {0, 0, 0};

		_cuda = ::nvidia::cuda::cuda::get();

		// ToDo: Find the device with the highest "score".
		D_LOG("Detected %zu compatible acceleration devices:", devices.size());
		for (size_t idx = 0; idx < devices.size(); idx++) {
			const int32_t&           device_idx = devices.at(idx);
			::nvidia::cuda::device_t device;
			if (auto v = _cuda->cuDeviceGet(&device, device_idx); v != ::nvidia::cuda::result::SUCCESS) {
				continue;
			}

			::nvidia::cuda::luid_t luid;
			uint32_t               nodes;
			_cuda->cuDeviceGetLuid(&luid, &nodes, device);

			// FixMe: Temporarily picking the most preferred device for show.
			if (idx == 0) {
				ideal.device = device;
				ideal.luid   = luid;
			}

			std::vector<char> name(256, 0);
			_cuda->cuDeviceGetName(name.data(), static_cast<int32_t>(name.size() - 1), device);

			int32_t is_integrated;
			_cuda->cuDeviceGetAttribute(&is_integrated, ::nvidia::cuda::device_attribute::INTEGRATED, device);

			std::pair<int32_t, int32_t> compute_capability;
			_cuda->cuDeviceGetAttribute(&compute_capability.first, ::nvidia::cuda::device_attribute::COMPUTE_CAPABILITY_MAJOR, device);
			_cuda->cuDeviceGetAttribute(&compute_capability.second, ::nvidia::cuda::device_attribute::COMPUTE_CAPABILITY_MINOR, device);

			int32_t multiprocessors;
			_cuda->cuDeviceGetAttribute(&multiprocessors, ::nvidia::cuda::device_attribute::MULTIPROCESSORS, device);

			int32_t async_engines;
			_cuda->cuDeviceGetAttribute(&async_engines, ::nvidia::cuda::device_attribute::ASYNC_ENGINES, device);

			int32_t hertz;
			_cuda->cuDeviceGetAttribute(&hertz, ::nvidia::cuda::device_attribute::KILOHERTZ, device);

			D_LOG("\t[%4zu] %s (%s, Compute Compatibility %" PRId32 ".%" PRId32 ", %" PRId32 " Multiprocessors, %" PRId32 " Asynchronous Engines, %" PRId32 " kHz) [%02" PRIx8 "%02" PRIx8 ":%02" PRIx8 "%02" PRIx8 ":%02" PRIx8 "%02" PRIx8 ":%02" PRIx8 "%02" PRIx8 "]", idx, name.data(), is_integrated ? "Integrated" : "Dedicated", // Intentional
				  compute_capability.first, compute_capability.second, multiprocessors, async_engines, hertz, //
				  luid.bytes[0], luid.bytes[1], luid.bytes[2], luid.bytes[3], // Intentional
				  luid.bytes[4], luid.bytes[5], luid.bytes[6], luid.bytes[7]);
		}
		D_LOG("Picked acceleration device [%02" PRIx8 "%02" PRIx8 ":%02" PRIx8 "%02" PRIx8 ":%02" PRIx8 "%02" PRIx8 ":%02" PRIx8 "%02" PRIx8 "]", ideal.luid.bytes[0], ideal.luid.bytes[1], ideal.luid.bytes[2], ideal.luid.bytes[3], // Intentional
			  ideal.luid.bytes[4], ideal.luid.bytes[5], ideal.luid.bytes[6], ideal.luid.bytes[7]);

#ifdef WIN32
		// Initialize a dummy D3D11 context to perhaps fool some functionality into working.
		_d3d = std::make_shared<::voicefx::windows::d3d::context>(ideal.luid);
#endif

		_cuda_context = std::make_shared<::nvidia::cuda::context>(ideal.device);
	} catch (...) {
		D_LOG("Failed to identify ideal acceleration devices.");
	}
#else
	// Enter the primary CUDA context, if available.
	std::shared_ptr<::nvidia::cuda::context_stack> ctx_stack;
	if (_cuda_context) {
		ctx_stack = _cuda_context->enter();
	}
#endif
#endif
}

nvidia::afx::afx::~afx()
{
#ifdef WIN32
	RemoveDllDirectory(reinterpret_cast<DLL_DIRECTORY_COOKIE>(_dll_cookie));
#endif
}

std::vector<int32_t> nvidia::afx::afx::enumerate_devices()
{
	NvAFX_Handle         effect      = nullptr;
	int32_t              num_devices = 0;
	std::vector<int32_t> devices;

	try {
		auto path     = model_path(NVAFX_EFFECT_DENOISER);
		auto path_str = path.generic_string();

		if (auto ret = CreateEffect(NVAFX_EFFECT_DENOISER, &effect); ret != NVAFX_STATUS_SUCCESS) {
			throw std::runtime_error("Failed to create temporary effect.");
		}

		if (auto ret = SetString(effect, NVAFX_PARAM_MODEL_PATH, path_str.c_str()); ret != NVAFX_STATUS_SUCCESS) {
			throw std::runtime_error("Failed to set model paths");
		}

		if (auto ret = GetSupportedDevices(effect, &num_devices, nullptr); ret != NVAFX_STATUS_OUTPUT_BUFFER_TOO_SMALL) {
			throw std::runtime_error("Failed to enumerate devices.");
		}

		devices.resize(num_devices);
		if (auto ret = GetSupportedDevices(effect, &num_devices, devices.data()); ret != NVAFX_STATUS_SUCCESS) {
			throw std::runtime_error("Failed to enumerate device identifiers.");
		}

		DestroyEffect(effect);

		return devices;
	} catch (...) {
		// Clean-up as we don't have a finally.
		if (effect) {
			DestroyEffect(effect);
		}
		throw;
	}

	return devices;
}

std::filesystem::path nvidia::afx::afx::redistributable_path()
{
	return _redist_path;
}

std::filesystem::path nvidia::afx::afx::model_path(NvAFX_EffectSelector effect)
{
	std::filesystem::path path = redistributable_path();
	path /= "models";
	if (std::string_view(NVAFX_EFFECT_DENOISER) == effect) {
		path /= "denoiser_48k.trtpkg";
	} else if (std::string_view(NVAFX_EFFECT_DEREVERB) == effect) {
		path /= "dereverb_48k.trtpkg";
	} else if (std::string_view(NVAFX_EFFECT_DEREVERB_DENOISER) == effect) {
		path /= "dereverb_denoiser_48k.trtpkg";
	} else {
	}
	path = std::filesystem::absolute(path);
	path.make_preferred();
	return path;
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
