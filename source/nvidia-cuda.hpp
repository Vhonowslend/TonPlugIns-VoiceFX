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
#include <platform.hpp>
#include "util-bitmask.hpp"

#include "warning-disable.hpp"
#include <cstddef>
#include <tuple>
#include "warning-enable.hpp"

#define P_CUDA_DEFINE_FUNCTION(name, ...)                   \
	private:                                                \
	typedef ::nvidia::cuda::result (*t##name)(__VA_ARGS__); \
                                                            \
	public:                                                 \
	t##name name = nullptr;

namespace nvidia::cuda {
	enum class result : std::size_t {
		SUCCESS                  = 0,
		INVALID_VALUE            = 1,
		OUT_OF_MEMORY            = 2,
		NOT_INITIALIZED          = 3,
		DEINITIALIZED            = 4,
		NO_DEVICE                = 100,
		INVALID_DEVICE           = 101,
		INVALID_CONTEXT          = 201,
		MAP_FAILED               = 205,
		UNMAP_FAILED             = 206,
		ARRAY_IS_MAPPED          = 207,
		ALREADY_MAPPED           = 208,
		NOT_MAPPED               = 211,
		INVALID_GRAPHICS_CONTEXT = 219,
		// Still missing some.
	};

	enum class memory_type : uint32_t {
		HOST    = 1,
		DEVICE  = 2,
		ARRAY   = 3,
		UNIFIED = 4,
	};

	enum class array_format : uint32_t {
		UNSIGNED_INT8  = 0b00000001,
		UNSIGNED_INT16 = 0b00000010,
		UNSIGNED_INT32 = 0b00000011,
		SIGNED_INT8    = 0b00001000,
		SIGNED_INT16   = 0b00001001,
		SIGNED_INT32   = 0b00001010,
		HALF           = 0b00010000,
		FLOAT          = 0b00100000,
	};

	enum class context_flags : uint32_t {
		SCHEDULER_AUTO                 = 0x0,
		SCHEDULER_SPIN                 = 0x1,
		SCHEDULER_YIELD                = 0x2,
		SCHEDULER_BLOCKING_SYNC        = 0x4,
		MAP_HOST                       = 0x8,
		LOCAL_MEMORY_RESIZE_TO_MAXIMUM = 0x10,
	};

	enum class device_attribute : uint32_t {
		KILOHERTZ                = 13,
		MULTIPROCESSORS          = 16,
		INTEGRATED               = 18,
		ASYNC_ENGINES            = 40,
		COMPUTE_CAPABILITY_MAJOR = 75,
		COMPUTE_CAPABILITY_MINOR = 76,
	};

	enum class external_memory_handle_type : uint32_t {
		INVALID                      = 0,
		FILE_DESCRIPTOR              = 1,
		WIN32_SHARED_HANDLE          = 2,
		WIN32_GLOBAL_SHARED_HANDLE   = 3,
		D3D12_HEAP                   = 4,
		D3D12_RESOURCE               = 5,
		D3D11_SHARED_RESOURCE        = 6,
		D3D11_GLOBAL_SHARED_RESOURCE = 7,
		NVSCIBUF                     = 8,
	};

	enum class stream_flags : uint32_t {
		DEFAULT      = 0x0,
		NON_BLOCKING = 0x1,
	};

	typedef void*    array_t;
	typedef void*    context_t;
	typedef uint64_t device_ptr_t;
	typedef void*    external_memory_t;
	typedef void*    graphics_resource_t;
	typedef void*    stream_t;
	typedef int32_t  device_t;

	struct memcpy2d_v2_t {
		std::size_t src_x_in_bytes;
		std::size_t src_y;

		memory_type  src_memory_type;
		const void*  src_host;
		device_ptr_t src_device;
		array_t      src_array;
		std::size_t  src_pitch;

		std::size_t dst_x_in_bytes;
		std::size_t dst_y;

		memory_type  dst_memory_type;
		const void*  dst_host;
		device_ptr_t dst_device;
		array_t      dst_array;
		std::size_t  dst_pitch;

		std::size_t width_in_bytes;
		std::size_t height;
	};

	struct array_descriptor_v2_t {
		std::size_t  width;
		std::size_t  height;
		uint32_t     num_channels;
		array_format format;
	};

	struct external_memory_buffer_info_v1_t {
		uint64_t offset;
		uint64_t size;
		uint32_t flags;
		uint32_t reserved[16];
	};

	struct external_memory_handle_info_v1_t {
		external_memory_handle_type type;
		union {
			int32_t file;
			struct {
				void*       handle;
				const void* name;
			};
			const void* nvscibuf;
		};
		uint64_t size;
		uint32_t flags;
		uint32_t reserved[16];
	};

	struct uuid_t {
		union {
			char bytes[16];
			struct {
				uint32_t a;
				uint16_t b;
				uint16_t c;
				uint16_t d;
				uint16_t e;
				uint32_t f;
			} uuid;
		};
	};

	struct luid_t {
		union {
			char bytes[8];
			struct {
				uint32_t low;
				int32_t  high;
			} parts;
			uint64_t luid;
		};
	};

	class exception : public std::exception {
		::nvidia::cuda::result _code;

		public:
		~exception(){};
		exception(::nvidia::cuda::result code) : _code(code) {}

		::nvidia::cuda::result code()
		{
			return _code;
		}
	};

	class cuda {
		std::shared_ptr<tonplugins::platform::library> _library;

		public:
		~cuda();
		cuda();

		int32_t version();

		public:
		// Initialization
		P_CUDA_DEFINE_FUNCTION(cuInit, int32_t flags);

		// Version Management
		P_CUDA_DEFINE_FUNCTION(cuDriverGetVersion, int32_t* driverVersion);

		// Device Management
		P_CUDA_DEFINE_FUNCTION(cuDeviceGetCount, int32_t* count);
		P_CUDA_DEFINE_FUNCTION(cuDeviceGet, device_t* device, int32_t idx);
		P_CUDA_DEFINE_FUNCTION(cuDeviceGetName, char* name, int32_t length, device_t device);
		P_CUDA_DEFINE_FUNCTION(cuDeviceGetLuid, luid_t* luid, uint32_t* device_node_mask, device_t device);
		P_CUDA_DEFINE_FUNCTION(cuDeviceGetUuid, uuid_t* uuid, device_t device);
		P_CUDA_DEFINE_FUNCTION(cuDeviceGetAttribute, int32_t* value, device_attribute attribute, device_t device);

		// Primary Context Management
		P_CUDA_DEFINE_FUNCTION(cuDevicePrimaryCtxRetain, context_t* ctx, device_t dev);
		P_CUDA_DEFINE_FUNCTION(cuDevicePrimaryCtxRelease, device_t);

		// Context Management
		P_CUDA_DEFINE_FUNCTION(cuCtxCreate, context_t* ctx, context_flags flags, device_t device);
		P_CUDA_DEFINE_FUNCTION(cuCtxDestroy, context_t ctx);
		P_CUDA_DEFINE_FUNCTION(cuCtxGetCurrent, context_t* ctx);
		P_CUDA_DEFINE_FUNCTION(cuCtxGetStreamPriorityRange, int32_t* lowestPriority, int32_t* highestPriority);
		P_CUDA_DEFINE_FUNCTION(cuCtxPopCurrent, context_t* ctx);
		P_CUDA_DEFINE_FUNCTION(cuCtxPushCurrent, context_t ctx);
		P_CUDA_DEFINE_FUNCTION(cuCtxSetCurrent, context_t ctx);
		P_CUDA_DEFINE_FUNCTION(cuCtxSynchronize);

		// Stream Managment
		P_CUDA_DEFINE_FUNCTION(cuStreamCreate, stream_t* stream, stream_flags flags);
		P_CUDA_DEFINE_FUNCTION(cuStreamCreateWithPriority, stream_t* stream, stream_flags flags, int32_t priority);
		P_CUDA_DEFINE_FUNCTION(cuStreamDestroy, stream_t stream);
		P_CUDA_DEFINE_FUNCTION(cuStreamSynchronize, stream_t stream);
		P_CUDA_DEFINE_FUNCTION(cuStreamGetPriority, stream_t stream, int32_t* priority);

		public:
		static std::shared_ptr<::nvidia::cuda::cuda> get();
	};
} // namespace nvidia::cuda

P_ENABLE_BITMASK_OPERATORS(::nvidia::cuda::context_flags)
P_ENABLE_BITMASK_OPERATORS(::nvidia::cuda::stream_flags)
