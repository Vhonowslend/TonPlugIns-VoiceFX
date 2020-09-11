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
#include <memory>
#include <vector>

#include <nvAudioEffects.h>
#include <samplerate.h>

namespace nvafx {
	class voicedenoiser {
		uint32_t _samplerate;
		uint32_t _samples_per_frame;

		std::shared_ptr<void>     _afx;
		uint32_t                  _afx_samplerate;
		uint32_t                  _afx_samples;
		uint32_t                  _afx_channels;
		std::vector<const float*> _afx_input;
		std::vector<float*>       _afx_output;

		SRC_STATE*         _src_in;
		double_t           _src_in_ratio;
		std::vector<float> _src_in_buffer;

		std::vector<float> _in_buffer;
		size_t             _in_offset;

		SRC_STATE*         _src_out;
		double_t           _src_out_ratio;
		std::vector<float> _src_out_buffer;

		std::vector<float> _out_buffer;
		size_t             _out_offset;

		bool   _delay;
		size_t _delay_samples;

		public:
		voicedenoiser();
		~voicedenoiser();

		void reset(uint32_t samplerate, uint32_t spf);

		bool process(uint32_t samples, const float* data_in, float* data_out);
	};
} // namespace nvafx
