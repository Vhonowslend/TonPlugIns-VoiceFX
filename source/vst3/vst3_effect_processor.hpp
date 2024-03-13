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
#include <ringbuffer.hpp>
#include "nvidia-afx-effect.hpp"
#include "nvidia-afx.hpp"
#include "resampler.hpp"
#include "vst3.hpp"

#include "warning-disable.hpp"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <public.sdk/source/vst/vstaudioeffect.h>
#include "warning-enable.hpp"

// Several assumptions just failed:
// - We can't tell the application that uses us which samplerate we need.
// - We can't tell the application that uses us how many samples per call we can process.
// - We can't tell the application anything. Who designed this?

//#define RESAMPLE

namespace vst3::effect {
	static const Steinberg::FUID processor_uid(FOURCC_CREATOR_PROCESSOR, // Creator, Type
											   FOURCC('V', 'o', 'i', 'c'), FOURCC('e', 'F', 'X', 'N'), FOURCC('o', 'i', 's', 'e'));

	class processor : Steinberg::Vst::AudioEffect {
		bool _dirty;

		size_t  _channels;
		int64_t _samplerate;
#ifdef RESAMPLE
		bool _resample;
#endif

		int64_t _delay;
		int64_t _local_delay;

		typedef tonplugins::memory::float_ring_t raw_buffer_t;
		typedef std::shared_ptr<raw_buffer_t>    buffer_t;
		typedef std::vector<buffer_t>            buffer_container_t;

		std::mutex         _in_lock;
		buffer_container_t _in_unresampled;
#ifdef RESAMPLE
		std::shared_ptr<::voicefx::resampler> _in_resampler;
		buffer_container_t                    _in_resampled;
#endif

		std::shared_ptr<::nvidia::afx::effect> _fx;

		std::mutex _out_lock;
#ifdef RESAMPLE
		buffer_container_t                    _out_unresampled;
		std::shared_ptr<::voicefx::resampler> _out_resampler;
#endif
		buffer_container_t _out_resampled;

		std::mutex              _lock;
		std::thread             _worker;
		std::condition_variable _worker_cv;
		bool                    _worker_quit;
		bool                    _worker_signal;

		public:
		processor();
		virtual ~processor();

		Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override;

		Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32 symbolicSampleSize) override;

		Steinberg::tresult PLUGIN_API setBusArrangements(Steinberg::Vst::SpeakerArrangement* inputs, Steinberg::int32 numIns, Steinberg::Vst::SpeakerArrangement* outputs, Steinberg::int32 numOuts) override;
		Steinberg::uint32 PLUGIN_API  getLatencySamples() override;
		Steinberg::uint32 PLUGIN_API  getTailSamples() override;

		Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup& setup) override;
		Steinberg::tresult PLUGIN_API setProcessing(Steinberg::TBool state) override;
		Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData& data) override;

		Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* state) override;
		Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* state) override;

		private:
		void reset();
		void set_channel_count(size_t num);

		void step_copy_in(const float** ins, buffer_container_t& outs, size_t samples);
		void step_resample_in(buffer_container_t& ins, buffer_container_t& outs);
		void step_process(buffer_container_t& ins, buffer_container_t& outs);
		void step_resample_out(buffer_container_t& ins, buffer_container_t& outs);
		void step_copy_out(buffer_container_t& ins, float** outs, size_t samples);

		void worker();

		public:
		static FUnknown* create(void* data);
	};
} // namespace vst3::effect
