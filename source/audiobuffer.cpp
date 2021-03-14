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

void voicefx::audiobuffer::push(const float* buffer, size_t length)
{
	// Calculate the new potential offset if everything goes well.
	int64_t avail = _avail - length;
	int64_t used  = _used + length;

	// Check if the new offset would overflow the buffer capacity.
	if (avail < 0) {
		throw std::runtime_error("Buffer Overflow");
	}

	// Move the already contained data further back.
	for (size_t idx = 0; idx < _used; idx++) {
		_buffer[avail + idx] = _buffer[_avail + idx];
	}

	// Copy the new data to the end.
	memcpy(&_buffer.at(avail + _used), buffer, length * sizeof(float));

	// Adjust internal values.
	_avail = avail;
	_used  = used;
}

const float* voicefx::audiobuffer::peek(size_t length)
{
	// Calculate the current offset.
	int64_t offset_a = _avail;

	// Calculate the required minimum offset to fulfill request.
	int64_t offset_b = _buffer.size() - length;

	// Figure out which one of these is the one that can fulfill the request.
	int64_t offset = std::min(offset_a, offset_b);

	// If the offset is outside of the buffer, error out.
	if (offset < 0) {
		throw std::runtime_error("Buffer Overflow");
	}

	// Otherwise, return a pointer to the area that fits the request.
	return &_buffer.at(offset);
}

void voicefx::audiobuffer::pop(size_t length)
{
	int64_t avail = _avail + length;
	int64_t used  = _used - length;

	// Check if the new offset would underflow the buffer.
	if (used < 0) {
		// If so, clamp to 0.
		avail = _buffer.size();
		used  = 0;
	}

	// Clear the popped memory.
	memset(_buffer.data(), 0, avail * sizeof(float));

	// Adjust internal values.
	_avail = avail;
	_used  = used;
}

void voicefx::audiobuffer::clear()
{
	// Adjust internal values.
	_avail = _buffer.size();
	_used  = 0;

	// Clear all memory.
	memset(_buffer.data(), 0, _avail * sizeof(float));
}

void voicefx::audiobuffer::resize(size_t size)
{
	_buffer.resize(size);
	_buffer.shrink_to_fit();

	// Clear the buffer.
	this->clear();
}

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
