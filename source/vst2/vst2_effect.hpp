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
#include <vst.h>
#include "audiobuffer.hpp"
#include "nvidia-afx-effect.hpp"
#include "nvidia-afx.hpp"

#ifdef ENABLE_RESAMPLER
#include "resampler.hpp"
#endif

namespace voicefx {
	namespace vst2 {
		class alignas(sizeof(intptr_t)) effect {
			vst_effect        _vsteffect;
			vst_host_callback _vstcb;

			vst_speaker_arrangement _input_arrangement;
			vst_speaker_arrangement _output_arrangement;

			uint32_t _sample_rate;
			//uint32_t _block_size;

			bool _dirty;

			struct channel_buffers {
				voicefx::audiobuffer input_resampled;
				voicefx::audiobuffer output_resampled;
#ifdef ENABLE_RESAMPLER
				voicefx::audiobuffer input_unresampled;
				voicefx::audiobuffer output_unresampled;
#endif
			};

			std::shared_ptr<::nvidia::afx::effect> _fx;
			std::vector<channel_buffers>           _channels;

			int64_t _delay;
			int64_t _local_delay;

#ifdef ENABLE_RESAMPLER
			std::shared_ptr<::voicefx::resampler> _in_resampler;
			std::shared_ptr<::voicefx::resampler> _out_resampler;
#endif
			public:
			effect(vst_host_callback cb);
			~effect();

			vst_effect* get_effect_structure()
			{
				return &_vsteffect;
			};

			protected:
			void reset();
			void set_channel_count(size_t num);

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
			intptr_t vst2_get_speaker_arrangement(vst_speaker_arrangement const** input, vst_speaker_arrangement const** output) const;
			intptr_t vst2_set_speaker_arrangement(vst_speaker_arrangement* input, vst_speaker_arrangement* output);

			intptr_t vst2_suspend_resume(bool should_resume);

#ifndef TONPLUGINS_DEMO
			void  vst2_get_parameter_properties(uint32_t index, vst_parameter_properties* parameter);
			void  vst2_get_parameter_label(uint32_t index, char text[VST_BUFFER_8]);
			void  vst2_get_parameter_name(uint32_t index, char text[VST_BUFFER_8]);
			void  vst2_get_parameter_value(uint32_t index, char text[VST_BUFFER_8]);
			bool  vst2_is_parameter_automatable(uint32_t index);
			void  vst2_set_parameter(uint32_t index, float value);
			float vst2_get_parameter(uint32_t index) const;
#endif

			void vst2_process_float(const float* const* inputs, float** outputs, int32_t samples);
		};
	} // namespace vst2
} // namespace voicefx
