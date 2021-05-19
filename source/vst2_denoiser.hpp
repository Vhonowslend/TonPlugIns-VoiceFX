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

#include <cinttypes>
#include <memory>
#include <vector>
#include "audiobuffer.hpp"
#include "nvafx.hpp"
#include "nvafx_denoiser.hpp"
#include "resampler.hpp"

#include <vst.h>

namespace voicefx {
	namespace vst2 {
		class alignas(sizeof(intptr_t)) denoiser {
			vst_effect        _vsteffect;
			vst_host_callback _vstcb;

			vst_speaker_arrangement _input_arrangement;
			vst_speaker_arrangement _output_arrangement;

			std::shared_ptr<::nvafx::nvafx>    _nvafx;
			std::shared_ptr<::nvafx::denoiser> _nvfx;

			bool     _dirty;
			uint32_t _samplerate;
			uint32_t _blocksize;
			uint32_t _delaysamples;

			struct channel_data {
				std::shared_ptr<::nvafx::denoiser> fx;

				voicefx::resampler input_resampler;
				voicefx::resampler output_resampler;

				voicefx::audiobuffer input_buffer;
				voicefx::audiobuffer fx_buffer;
				voicefx::audiobuffer output_buffer;

				int64_t delay;
			};
			std::vector<channel_data> _channels;
			std::vector<float>        _scratch;

			public:
			denoiser(vst_host_callback cb);
			~denoiser();

			vst_effect* get_effect_structure()
			{
				return &_vsteffect;
			};

			protected:
			void reset();

			void setup_channels();
			void reset_channels();

			private /* VST2 Functionality */:
			intptr_t vst2_control(VST_EFFECT_OPCODE opcode, int32_t p1, intptr_t p2, void* p3, float p4);

			intptr_t vst2_get_vendor_name(char* buffer, size_t buffer_len) const;
			intptr_t vst2_get_vendor_version() const;
			intptr_t vst2_get_product_name(char* buffer, size_t buffer_len) const;
			intptr_t vst2_get_effect_name(char* buffer, size_t buffer_len) const;
			intptr_t vst2_get_effect_category() const;

			intptr_t vst2_create();
			intptr_t vst2_destroy();

			intptr_t vst2_set_sample_rate(float_t sample_rate);
			intptr_t vst2_set_block_size(intptr_t block_size);
			intptr_t vst2_get_speaker_arrangement(vst_speaker_arrangement const** input,
												  vst_speaker_arrangement const** output) const;
			intptr_t vst2_set_speaker_arrangement(vst_speaker_arrangement* input, vst_speaker_arrangement* output);

			void  vst2_set_parameter(uint32_t index, float value);
			float vst2_get_parameter(uint32_t index) const;

			void vst2_process_float(const float* const* inputs, float** outputs, int32_t samples);
		};
	} // namespace vst2
} // namespace voicefx
