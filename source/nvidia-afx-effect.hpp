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
		std::shared_ptr<::nvidia::cuda::cuda>    _cuda;
		std::shared_ptr<::nvidia::cuda::context> _context;
		std::shared_ptr<::nvidia::cuda::stream>  _stream;

		std::shared_ptr<::nvidia::afx::afx> _nvafx;

		std::mutex                         _lock;
		std::filesystem::path              _model_path;
		std::string                        _model_path_str;
		std::vector<std::shared_ptr<void>> _fx;

		std::atomic_bool   _fx_dirty;
		std::atomic_bool   _fx_denoise;
		std::atomic_bool   _fx_dereverb;
		std::atomic_size_t _fx_channels;

		std::atomic_bool   _cfg_dirty;
		std::atomic<float> _cfg_intensity;

		public:
		effect();
		~effect();

		public:
		static size_t samplerate();

		static size_t blocksize();

		public:
		bool denoise_enabled();
		void enable_denoise(bool v);

		bool dereverb_enabled();
		void enable_dereverb(bool v);

		size_t channels();
		void   channels(size_t v);

		float intensity();
		void  intensity(float v);

		void load();

		void clear();

		void process(const float** input, float** output, size_t samples);
	};
} // namespace nvidia::afx
