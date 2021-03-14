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
#include <vector>

namespace voicefx {
	class audiobuffer {
		std::vector<float> _buffer;
		size_t             _avail;
		size_t             _used;

		public:
		audiobuffer();
		audiobuffer(size_t size);
		~audiobuffer();

		/** Insert new data to the back of the buffer.
		 * 
		 * @param buffer 
		 * @param length 
		 */
		void push(const float* buffer, size_t length);

		/** Peek at data at or before the front of the buffer.
		 *
		 * @param length The length of data to look at. If the length exceeds the currently used data, empty data is shown.
		 */
		const float* peek(size_t length);

		/** Pop data from the front of the buffer.
		 *
		 * Assigned memory is automatically cleared.
		 * 
		 * @param length The length of data to pop.
		 */
		void pop(size_t length);

		/** Clear the buffer.
		 * 
		 */
		void clear();

		/** Resizes the buffer to the new size and clears it.
		 * 
		 * @param size New size for the buffer.
		 */
		void resize(size_t size);

		/** Calculate the available space in the buffer.
		 * 
		 * @return The currently available space in the buffer.
		 */
		size_t avail();

		/** Calculate the used space in the buffer.
		 * 
		 * @return The currently used space in the buffer.
		 */
		size_t size();

		/** Get the total capacity of the buffer.
		 * 
		 * @return The total capacity.
		 */
		size_t capacity();
	};

} // namespace voicefx
