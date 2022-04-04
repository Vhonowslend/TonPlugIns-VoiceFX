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

#include "audiobuffer.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>

voicefx::audiobuffer::audiobuffer() : _buffer(1, 0), _avail(1), _used(0)
{
	_buffer.shrink_to_fit();
}

voicefx::audiobuffer::audiobuffer(size_t size) : _buffer(size, 0), _avail(size), _used(0) {}

voicefx::audiobuffer::~audiobuffer() {}

size_t voicefx::audiobuffer::avail() const
{
	return _avail;
}

size_t voicefx::audiobuffer::size() const
{
	return _used;
}

size_t voicefx::audiobuffer::capacity() const
{
	return _buffer.size();
}

void voicefx::audiobuffer::resize(size_t size)
{
	_buffer.resize(size);
	_buffer.shrink_to_fit();

	clear();
}

void voicefx::audiobuffer::clear()
{
	// Clear all stored data.
	memset(_buffer.data(), 0, _buffer.size());

	// Reset internal data.
	_avail = _buffer.size();
	_used  = 0;
}

const float* voicefx::audiobuffer::peek(size_t length)
{
	// If there is not enough data, throw a buffer underflow error.
	if (_used < length) {
		throw std::runtime_error("Buffer Underflow");
	}

	// Otherwise, return a pointer to the area that fits the request.
	return _buffer.data();
}

void voicefx::audiobuffer::pop(size_t length)
{
	// Calculate and verify information
	if (_used < length) {
		throw std::runtime_error("Buffer Underflow");
	}

	// If everything is fine, move the entire buffer back to the start.
	memmove(_buffer.data(), &_buffer.at(length), (_buffer.size() - length) * sizeof(float));

	// Update our internal state.
	_avail += length;
	_used -= length;
}

float* voicefx::audiobuffer::reserve(size_t length)
{
	// Early-exit if there is a problem.
	if (_avail < length) {
		throw std::runtime_error("Buffer Overflow");
	}

	// If things work out, calculate the offset.
	size_t offset = _used;

	// Update the internal state.
	_used += length;
	_avail -= length;

	return &_buffer.at(offset);
}

void voicefx::audiobuffer::push(const float* buffer, size_t length)
{
	// Reserve some space on the buffer.
	float* area = reserve(length);

	// Copy the data into the buffer.
	memcpy(area, buffer, length * sizeof(float));
}

float* voicefx::audiobuffer::front()
{
	return _buffer.data();
}

float* voicefx::audiobuffer::back()
{
	return &_buffer[_used];
}
