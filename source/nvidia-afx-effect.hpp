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
#include <atomic>
#include <filesystem>
#include <memory>
#include <mutex>
#include "nvidia-afx.hpp"
#include "nvidia-cuda-context.hpp"
#include "nvidia-cuda-stream.hpp"
#include "nvidia-cuda.hpp"

namespace nvidia::afx {
	class effect {
		std::shared_ptr<::nvidia::afx::afx> _nvafx;

		std::recursive_mutex  _lock;
		std::filesystem::path _model_path;
		std::string           _model_path_str;

		std::vector<std::shared_ptr<void>> _fx;
		std::atomic_uint8_t                _fx_channels;
		std::atomic_bool                   _fx_dirty;
#ifndef TONPLUGINS_DEMO
		std::atomic_bool _fx_model;
		std::atomic_bool _fx_denoise;
		std::atomic_bool _fx_dereverb;
#endif

#ifndef TONPLUGINS_DEMO
		std::atomic_bool   _cfg_dirty;
		std::atomic<float> _cfg_intensity;
		std::atomic_bool   _cfg_vad;
#endif

		public:
		effect();
		~effect();

		protected:
		template<typename T>
		T get(NvAFX_ParameterSelector key);

		template<typename T>
		void set(NvAFX_ParameterSelector key, T value);

		public /* Effect Information */:
		size_t input_samplerate();
		size_t output_samplerate();

		size_t input_blocksize();
		size_t output_blocksize();

		size_t input_channels();
		size_t output_channels();

		public /* Wrapper Information */:
		uint8_t channels();
		void    channels(uint8_t v);

#ifndef TONPLUGINS_DEMO
		bool denoise_enabled();
		void enable_denoise(bool v);

		bool dereverb_enabled();
		void enable_dereverb(bool v);
#endif

#ifndef TONPLUGINS_DEMO
		float intensity();
		void  intensity(float v);

		bool voice_activity_detection();
		void voice_activity_detection(bool v);
#endif

		void load();

		void clear();

		void process(const float** input, float** output, size_t samples);

		void process(float const** inputs, size_t& input_samples, float** outputs, size_t& output_samples);

		public:
		static uint32_t samplerate();

		static size_t blocksize();

		static size_t delay();
	};
} // namespace nvidia::afx
