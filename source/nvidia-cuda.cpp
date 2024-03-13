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

#include "nvidia-cuda.hpp"
#include <platform.hpp>
#include "lib.hpp"

#include "warning-disable.hpp"
#include <mutex>
#include "warning-enable.hpp"

#if defined(_WIN32) || defined(_WIN64)
#define ST_CUDA_NAME "nvcuda.dll"
#else
#define ST_CUDA_NAME "libcuda.so.1"
#endif

#define P_CUDA_LOAD_SYMBOL(NAME)                                                             \
	{                                                                                        \
		NAME = reinterpret_cast<decltype(NAME)>(_library->load_symbol(#NAME));               \
		if (!NAME)                                                                           \
			throw std::runtime_error("Failed to load '" #NAME "' from '" ST_CUDA_NAME "'."); \
	}
#define P_CUDA_LOAD_SYMBOL_OPT(NAME)                                           \
	{                                                                          \
		NAME = reinterpret_cast<decltype(NAME)>(_library->load_symbol(#NAME)); \
		if (!NAME)                                                             \
			D_LOG("Loading of optional symbol '" #NAME "' failed.", 0);        \
	}

#define P_CUDA_LOAD_SYMBOL_EX(NAME, OVERRIDE)                                                \
	{                                                                                        \
		NAME = reinterpret_cast<decltype(NAME)>(_library->load_symbol(#OVERRIDE));           \
		if (!NAME)                                                                           \
			throw std::runtime_error("Failed to load '" #NAME "' from '" ST_CUDA_NAME "'."); \
	}
#define P_CUDA_LOAD_SYMBOL_OPT_EX(NAME, OVERRIDE)                                  \
	{                                                                              \
		NAME = reinterpret_cast<decltype(NAME)>(_library->load_symbol(#OVERRIDE)); \
		if (!NAME)                                                                 \
			D_LOG("Loading of optional symbol '" #NAME "' failed.", 0);            \
	}

#define P_CUDA_LOAD_SYMBOL_V2(NAME)                                                          \
	{                                                                                        \
		NAME = reinterpret_cast<decltype(NAME)>(_library->load_symbol(#NAME "_v2"));         \
		if (!NAME)                                                                           \
			throw std::runtime_error("Failed to load '" #NAME "' from '" ST_CUDA_NAME "'."); \
	}
#define P_CUDA_LOAD_SYMBOL_OPT_V2(NAME)                                              \
	{                                                                                \
		NAME = reinterpret_cast<decltype(NAME)>(_library->load_symbol(#NAME "_v2")); \
		if (!NAME)                                                                   \
			D_LOG("Loading of optional symbol '" #NAME "' failed.", 0);              \
	}

nvidia::cuda::cuda::~cuda()
{
	D_LOG_LOUD("");
	D_LOG("Unloading...");
}

nvidia::cuda::cuda::cuda() : _library()
{
	D_LOG_LOUD("");
	int32_t cuda_version = 0;

	D_LOG("Loading...");

	_library = ::tonplugins::platform::library::load(std::string_view(ST_CUDA_NAME));

	{ // 1. Load critical initialization functions.
		// Initialization
		P_CUDA_LOAD_SYMBOL(cuInit);

		// Version Management
		P_CUDA_LOAD_SYMBOL(cuDriverGetVersion);
	}

	{ // 2. Get the CUDA Driver version and log it.
		if (cuDriverGetVersion(&cuda_version) == result::SUCCESS) {
			int32_t major = cuda_version / 1000;
			int32_t minor = (cuda_version % 1000) / 10;
			int32_t patch = (cuda_version % 10);
			D_LOG("Version: %" PRId32 ".%" PRId32 ".%" PRId32, major, minor, patch);
		} else {
			D_LOG("Version: Unknown");
		}
	}

	{ // 3. Load remaining functions.
		// Device Management
		P_CUDA_LOAD_SYMBOL(cuDeviceGetCount);
		P_CUDA_LOAD_SYMBOL(cuDeviceGet);
		P_CUDA_LOAD_SYMBOL(cuDeviceGetName);
		P_CUDA_LOAD_SYMBOL(cuDeviceGetLuid);
		P_CUDA_LOAD_SYMBOL(cuDeviceGetUuid);
		P_CUDA_LOAD_SYMBOL(cuDeviceGetAttribute);

		// Primary Context Management
		P_CUDA_LOAD_SYMBOL(cuDevicePrimaryCtxRetain);
		P_CUDA_LOAD_SYMBOL(cuDevicePrimaryCtxRelease);

		// Context Management
		P_CUDA_LOAD_SYMBOL_V2(cuCtxCreate);
		P_CUDA_LOAD_SYMBOL_V2(cuCtxDestroy);
		P_CUDA_LOAD_SYMBOL_V2(cuCtxPushCurrent);
		P_CUDA_LOAD_SYMBOL_V2(cuCtxPopCurrent);
		P_CUDA_LOAD_SYMBOL_OPT(cuCtxGetCurrent);
		P_CUDA_LOAD_SYMBOL_OPT(cuCtxSetCurrent);
		P_CUDA_LOAD_SYMBOL_OPT(cuCtxGetStreamPriorityRange);
		P_CUDA_LOAD_SYMBOL(cuCtxSynchronize);

		// Stream Management
		P_CUDA_LOAD_SYMBOL(cuStreamCreate);
		P_CUDA_LOAD_SYMBOL_V2(cuStreamDestroy);
		P_CUDA_LOAD_SYMBOL(cuStreamSynchronize);
		P_CUDA_LOAD_SYMBOL_OPT(cuStreamCreateWithPriority);
		P_CUDA_LOAD_SYMBOL_OPT(cuStreamGetPriority);
	}

	// Initialize CUDA
	cuInit(0);
}

int32_t nvidia::cuda::cuda::version()
{
	D_LOG_LOUD("");
	int32_t v = 0;
	cuDriverGetVersion(&v);
	return v;
}

std::shared_ptr<nvidia::cuda::cuda> nvidia::cuda::cuda::get()
{
	static std::weak_ptr<nvidia::cuda::cuda> instance;
	static std::mutex                        lock;

	std::unique_lock<std::mutex> ul(lock);
	if (instance.expired()) {
		auto hard_instance = std::make_shared<nvidia::cuda::cuda>();
		instance           = hard_instance;
		return hard_instance;
	}
	return instance.lock();
}
