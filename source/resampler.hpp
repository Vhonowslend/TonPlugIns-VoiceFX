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
	class resampler {
		void* _instance;
		float _ratio;

		public:
		~resampler();

		/** Create a new resampler with no specified conversion ratio.
		 */
		resampler();

		/** Create a new resampler for a specific conversion ratio.
		 * 
		 * @param input_samplerate Input sample rate.
		 * @param output_samplerate Output sample rate.
		 */
		resampler(uint32_t input_samplerate, uint32_t output_samplerate);

		/** Process the given input buffer into the given output buffer.
		 * 
		 * @param input Buffer of floats containing at least #input_samples of frames.
		 * @param input_samples The number of frames in #input.
		 * @param output Buffer of floats containing at least #output_samples of frames.
		 * @param output_samples The number of frames in #output.
		*/
		void process(const float input[], size_t& input_samples, float output[], size_t& output_samples);

		/** Reset the resampler and optionally change the conversion ratio.
		 *
		 * @param input_samplerate (Optional) New input sample rate.
		 * @param output_samplerate (Optional) New output sample rate.
		 */
		void reset(uint32_t input_samplerate = 0, uint32_t output_samplerate = 0);

		float ratio() const;
	};
} // namespace voicefx
