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

#include "nvidia-cuda-memory.hpp"
#include <stdexcept>
#include "lib.hpp"

#define D_LOG(MESSAGE, ...) voicefx::log("<CUDA::Memory> " MESSAGE, __VA_ARGS__)

voicefx::nvidia::cuda::memory::~memory()
{
	_cuda->cuMemFree(_pointer);
}

voicefx::nvidia::cuda::memory::memory(size_t size)
	: _cuda(::voicefx::nvidia::cuda::cuda::get()), _pointer(), _size(size)
{
	::voicefx::nvidia::cuda::result res = _cuda->cuMemAlloc(&_pointer, _size);
	switch (res) {
	case ::voicefx::nvidia::cuda::result::SUCCESS:
		break;
	default:
		D_LOG("Failed to create memory with error code %" PRIu32 ".", res);
		throw std::runtime_error("nvidia::cuda::memory: cuMemAlloc failed.");
	}
}

voicefx::nvidia::cuda::device_ptr_t voicefx::nvidia::cuda::memory::get()
{
	return _pointer;
}

std::size_t voicefx::nvidia::cuda::memory::size()
{
	return _size;
}
