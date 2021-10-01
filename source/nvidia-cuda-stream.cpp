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

#include "nvidia-cuda-stream.hpp"
#include <stdexcept>
#include "lib.hpp"

#define D_LOG(MESSAGE, ...) voicefx::log("<CUDA::Stream> " MESSAGE, __VA_ARGS__)

nvidia::cuda::stream::~stream()
{
	_cuda->cuStreamDestroy(_stream);
}

nvidia::cuda::stream::stream(::nvidia::cuda::stream_flags flags, int32_t priority) : _cuda(::nvidia::cuda::cuda::get())
{
	nvidia::cuda::result res;
	if (priority == 0) {
		res = _cuda->cuStreamCreate(&_stream, flags);
	} else {
		res = _cuda->cuStreamCreateWithPriority(&_stream, flags, priority);
	}
	switch (res) {
	case nvidia::cuda::result::SUCCESS:
		break;
	default:
		D_LOG("Failed to create stream with error code %" PRIu32 ".", res);
		throw std::runtime_error("Failed to create CUstream object.");
	}
}

::nvidia::cuda::stream_t nvidia::cuda::stream::get()
{
	return _stream;
}

void nvidia::cuda::stream::synchronize()
{
	if (auto res = _cuda->cuStreamSynchronize(_stream); res != ::nvidia::cuda::result::SUCCESS) {
		throw ::nvidia::cuda::exception(res);
	}
}
