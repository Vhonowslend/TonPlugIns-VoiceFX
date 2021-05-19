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

#include "vst2_denoiser.hpp"
#include <algorithm>
#include "lib.hpp"

#define D_LOG(MESSAGE, ...) voicefx::log("<VST2::Denoiser> " MESSAGE, __VA_ARGS__)

//#define DEBUG_PROCESSING

template<size_t N>
void sized_memmove(void* dst, size_t dst_off, void* src, size_t src_off, size_t cnt)
{
	memmove(reinterpret_cast<uint8_t*>(dst) + (dst_off * N), reinterpret_cast<uint8_t*>(src) + (src_off * N), cnt * N);
}

template<size_t N>
void sized_memcpy(void* dst, size_t dst_off, const void* src, size_t src_off, size_t cnt)
{
	memcpy(reinterpret_cast<uint8_t*>(dst) + (dst_off * N), reinterpret_cast<const uint8_t*>(src) + (src_off * N),
		   cnt * N);
}

template<size_t N>
void sized_memset(void* dst, size_t dst_off, int value, size_t cnt)
{
	memset(reinterpret_cast<uint8_t*>(dst) + (dst_off * N), value, cnt * N);
}

voicefx::vst2::denoiser::denoiser(vst_host_callback cb)
	: _vsteffect(), _vstcb(cb), _input_arrangement(), _output_arrangement(), _dirty(true), _samplerate(44100),
	  _blocksize(_samplerate / 100), _scratch(_samplerate, 0)
{
	D_LOG("(0x%08" PRIxPTR ") Initializing...", &this->_vsteffect);

	// Load and initialize NVIDIA Audio Effects.
	_nvafx = nvafx::nvafx::instance();
	_nvfx  = std::make_shared<::nvafx::denoiser>();

	// Initialize the VST structure.
	_vsteffect.magic_number      = 'VstP';
	_vsteffect.num_programs      = 0;
	_vsteffect.num_params        = 0;
	_vsteffect.flags             = 0b00010000;
	_vsteffect.delay             = 441;
	_vsteffect._unknown_float_00 = 1.0;
	_vsteffect.effect_internal   = this;
	_vsteffect.unique_id         = 'XVFX';
	_vsteffect.version           = (0 << 24) | (3 << 16) | (0 << 8) | (0);

	_vsteffect.control = [](vst_effect* pthis, VST_EFFECT_OPCODE opcode, int32_t p_int1, intptr_t p_int2, void* p_ptr,
							float p_float) {
		try {
			return static_cast<voicefx::vst2::denoiser*>(pthis->effect_internal)
				->vst2_control(opcode, p_int1, p_int2, p_ptr, p_float);
		} catch (std::exception const& ex) {
			D_LOG("(0x%08" PRIxPTR ") Unhandled exception in control: %s", pthis, ex.what());
		} catch (...) {
			D_LOG("(0x%08" PRIxPTR ") Unknown exception in control.", pthis);
		}
		return intptr_t();
	};
	_vsteffect.set_parameter = [](vst_effect* pthis, uint32_t index, float value) {
		try {
			static_cast<voicefx::vst2::denoiser*>(pthis->effect_internal)->vst2_set_parameter(index, value);
		} catch (std::exception const& ex) {
			D_LOG("(0x%08" PRIxPTR ") Unhandled exception in set_parameter: %s", pthis, ex.what());
		} catch (...) {
			D_LOG("(0x%08" PRIxPTR ") Unknown exception in set_parameter.", pthis);
		}
	};
	_vsteffect.get_parameter = [](vst_effect* pthis, uint32_t index) {
		try {
			return static_cast<voicefx::vst2::denoiser*>(pthis->effect_internal)->vst2_get_parameter(index);
		} catch (std::exception const& ex) {
			D_LOG("(0x%08" PRIxPTR ") Unhandled exception in get_parameter: %s", pthis, ex.what());
		} catch (...) {
			D_LOG("(0x%08" PRIxPTR ") Unknown exception in get_parameter.", pthis);
		}
		return float();
	};
	_vsteffect.process = [](vst_effect* pthis, const float* const* inputs, float** outputs, int32_t samples) {
		try {
			static_cast<voicefx::vst2::denoiser*>(pthis->effect_internal)->vst2_process_float(inputs, outputs, samples);
		} catch (std::exception const& ex) {
			D_LOG("(0x%08" PRIxPTR ") Unhandled exception in process: %s", pthis, ex.what());
		} catch (...) {
			D_LOG("(0x%08" PRIxPTR ") Unknown exception in process.", pthis);
		}
	};
	_vsteffect.process_float = [](vst_effect* pthis, const float* const* inputs, float** outputs, int32_t samples) {
		try {
			static_cast<voicefx::vst2::denoiser*>(pthis->effect_internal)->vst2_process_float(inputs, outputs, samples);
		} catch (std::exception const& ex) {
			D_LOG("(0x%08" PRIxPTR ") Unhandled exception in process_float: %s", pthis, ex.what());
		} catch (...) {
			D_LOG("(0x%08" PRIxPTR ") Unknown exception in process_float.", pthis);
		}
	};
	_vsteffect.process_double = nullptr;

	{ // Set up default speaker arrangement.
		_input_arrangement      = {};
		_output_arrangement     = {};
		_input_arrangement.type = _output_arrangement.type = VST_ARRANGEMENT_TYPE_STEREO;
		_input_arrangement.channels = _output_arrangement.channels = 2;
		_input_arrangement.speakers[0].type = _output_arrangement.speakers[0].type = VST_SPEAKER_TYPE_LEFT;
		_input_arrangement.speakers[1].type = _output_arrangement.speakers[1].type = VST_SPEAKER_TYPE_RIGHT;

		// Update our VST to have 2 inputs and outputs.
		_vsteffect.num_inputs = _vsteffect.num_outputs = _input_arrangement.channels;
	}

	// Finally reset everything to the initial state.
	reset();

	D_LOG("(0x%08" PRIxPTR ") Initialized.", &this->_vsteffect);
}

voicefx::vst2::denoiser::~denoiser()
{
	_channels.clear();
	_nvafx.reset();
}

void voicefx::vst2::denoiser::reset()
{
	if (!_dirty) {
		return;
	}

	float ioscale = (static_cast<float>(_samplerate) / static_cast<float>(nvafx::denoiser::get_sample_rate()));

	// (Re-)Calculate initial delay.
	_delaysamples = static_cast<int32_t>(_nvfx->get_block_size() * ioscale);

	// (Re-)Calculate absolute delay.
	_vsteffect.delay = _delaysamples + (nvafx::denoiser::get_minimum_delay() * ioscale);

	// Re-size scratch memory.
	_scratch.resize(_samplerate);

	// Re-initialize channels.
	setup_channels();
}

void voicefx::vst2::denoiser::setup_channels()
{
	// (Re-)Create all channels.
	_channels.resize(_vsteffect.num_inputs);
	for (auto& channel : _channels) {
		// (Re-)Create the re-samplers.
		channel.input_resampler.reset(_samplerate, nvafx::denoiser::get_sample_rate());
		channel.output_resampler.reset(nvafx::denoiser::get_sample_rate(), _samplerate);

		// (Re-)Create the buffers and reset offsets.
		channel.input_buffer.resize(nvafx::denoiser::get_sample_rate());
		channel.fx_buffer.resize(nvafx::denoiser::get_sample_rate());
		channel.output_buffer.resize(_samplerate);
		// TODO: Consider reducing memory impact of the above.
	}

	// Reset any remaining information.
	reset_channels();
}

void voicefx::vst2::denoiser::reset_channels()
{
	for (auto& channel : _channels) {
		// Recreate effect.
		channel.fx = std::make_shared<::nvafx::denoiser>();

		// Reset re-samplers
		channel.input_resampler.reset();
		channel.output_resampler.reset();

		// Clear Buffers
		channel.input_buffer.clear();
		channel.fx_buffer.clear();
		channel.output_buffer.clear();

		// Reset delay
		channel.delay = _delaysamples;
	}
}

intptr_t voicefx::vst2::denoiser::vst2_control(VST_EFFECT_OPCODE opcode, int32_t p1, intptr_t p2, void* p3, float p4)
{
	D_LOG("(0x%08" PRIxPTR ") Control Call: %08lX %08lX %016llX %016llX %f", &this->_vsteffect, opcode, p1, p2, p3, p4);

	switch (opcode) {
		// -------------------------------------------------------------------------- //
		// Metadata
	case VST_EFFECT_OPCODE_PRODUCT_NAME:
		return vst2_get_product_name(reinterpret_cast<char*>(p3), VST_PRODUCT_BUFFER_SIZE);
	case VST_EFFECT_OPCODE_VENDOR_NAME:
		return vst2_get_vendor_name(reinterpret_cast<char*>(p3), VST_VENDOR_BUFFER_SIZE);
	case VST_EFFECT_OPCODE_VENDOR_VERSION:
		return vst2_get_vendor_version();
	case VST_EFFECT_OPCODE_EFFECT_NAME:
		return vst2_get_effect_name(reinterpret_cast<char*>(p3), VST_EFFECT_BUFFER_SIZE);
	case VST_EFFECT_OPCODE_EFFECT_CATEGORY:
		return vst2_get_effect_category();
	case VST_EFFECT_OPCODE_VST_VERSION:
		return VST_VERSION_2_4_0_0;

		// -------------------------------------------------------------------------- //
		// Processing
	case VST_EFFECT_OPCODE_CREATE:
		return vst2_create();
	case VST_EFFECT_OPCODE_DESTROY:
		return vst2_destroy();

		// -------------------------------------------------------------------------- //
		// Audio Metadata
	case VST_EFFECT_OPCODE_SETSAMPLERATE:
		return vst2_set_sample_rate(p4);
	case VST_EFFECT_OPCODE_SETBLOCKSIZE:
		return vst2_set_block_size(p2);
	case VST_EFFECT_OPCODE_TAIL_SAMPLES:
		return _vsteffect.delay;

		// -------------------------------------------------------------------------- //
		// Channels
	case VST_EFFECT_OPCODE_GET_SPEAKER_ARRANGEMENT:
		return vst2_get_speaker_arrangement(reinterpret_cast<vst_speaker_arrangement const**>(p2),
											reinterpret_cast<vst_speaker_arrangement const**>(p3));
	case VST_EFFECT_OPCODE_SET_SPEAKER_ARRANGEMENT:
		return vst2_set_speaker_arrangement(reinterpret_cast<vst_speaker_arrangement*>(p2),
											reinterpret_cast<vst_speaker_arrangement*>(p3));

	// -------------------------------------------------------------------------- //
	// Processing
	case VST_EFFECT_OPCODE_SUSPEND:
		return 0;
	case VST_EFFECT_OPCODE_PROCESS_BEGIN:
		reset_channels();
		return 0;
	case VST_EFFECT_OPCODE_PROCESS_END:
		return 0;

		// -------------------------------------------------------------------------- //
		// User Interface (Not Implemented)
	case VST_EFFECT_OPCODE_WINDOW_CREATE:
	case VST_EFFECT_OPCODE_WINDOW_DESTROY:
	case VST_EFFECT_OPCODE_WINDOW_GETRECT:
		return 1;
	}

	D_LOG("(0x%08" PRIxPTR ") Unhandled Control Call: %08lX %08lX %016llX %016llX %f", &this->_vsteffect, opcode, p1,
		  p2, p3, p4);

	return intptr_t();
}

intptr_t voicefx::vst2::denoiser::vst2_get_vendor_name(char* buffer, size_t buffer_len) const
{
	memset(buffer, 0, buffer_len);
	memcpy(buffer, voicefx::vendor.data(), std::min<size_t>(voicefx::vendor.size(), buffer_len));
	return 0;
}

intptr_t voicefx::vst2::denoiser::vst2_get_vendor_version() const
{
	return 0;
}

intptr_t voicefx::vst2::denoiser::vst2_get_product_name(char* buffer, size_t buffer_len) const
{
	memset(buffer, 0, buffer_len);
	memcpy(buffer, voicefx::name.data(), std::min<size_t>(voicefx::name.size(), buffer_len));
	return 0;
}

intptr_t voicefx::vst2::denoiser::vst2_get_effect_name(char* buffer, size_t buffer_len) const
{
	memset(buffer, 0, buffer_len);
	memcpy(buffer, voicefx::name.data(), std::min<size_t>(voicefx::name.size(), buffer_len));
	return 0;
}

intptr_t voicefx::vst2::denoiser::vst2_get_effect_category() const
{
	return VST_CATEGORY_RESTORATION;
}

intptr_t voicefx::vst2::denoiser::vst2_create()
{
	reset();
	return 0;
}

intptr_t voicefx::vst2::denoiser::vst2_destroy()
{
	delete this;
	return 0;
}

intptr_t voicefx::vst2::denoiser::vst2_set_sample_rate(float_t sample_rate)
{
	uint32_t samplerate = static_cast<uint32_t>(sample_rate);

	if (_samplerate != samplerate) {
		_dirty      = true;
		_samplerate = samplerate;
	}

	reset();
	return 0;
}

intptr_t voicefx::vst2::denoiser::vst2_set_block_size(intptr_t block_size)
{
	uint32_t blocksize = static_cast<uint32_t>(block_size);

	if (_blocksize != blocksize) {
		_dirty     = true;
		_blocksize = blocksize;
	}

	reset();
	return 0;
}

intptr_t voicefx::vst2::denoiser::vst2_get_speaker_arrangement(vst_speaker_arrangement const** input,
															   vst_speaker_arrangement const** output) const
{
	*input  = &_input_arrangement;
	*output = &_output_arrangement;
	return 0;
}

intptr_t voicefx::vst2::denoiser::vst2_set_speaker_arrangement(vst_speaker_arrangement* input,
															   vst_speaker_arrangement* output)
{
	// We ignore the requested output here, this VST does not perform any down/up mixing.

	// Did the Host request at least one channel?
	if (input->channels < 1) {
		// Always guarantee a single channel of audio.
		input->channels = 1;
	}

	// Update our arrangement with the requested one.
	_input_arrangement.type = _output_arrangement.type = input->type;
	_input_arrangement.channels = _output_arrangement.channels = input->channels;
	for (size_t idx = 0; idx < input->channels; idx++) {
		_input_arrangement.speakers[idx]  = input->speakers[idx];
		_output_arrangement.speakers[idx] = input->speakers[idx];
	}

	// Adjust VST information.
	_vsteffect.num_inputs  = _input_arrangement.channels;
	_vsteffect.num_outputs = _output_arrangement.channels;

	D_LOG("(0x%08" PRIxPTR ") Speaker Arrangement adjusted to %" PRId32 " channels.", &this->_vsteffect,
		  _input_arrangement.channels);

	// Update Channels
	setup_channels();

	return 0;
}

void voicefx::vst2::denoiser::vst2_set_parameter(uint32_t index, float value) {}

float voicefx::vst2::denoiser::vst2_get_parameter(uint32_t index) const
{
	return 0.0F;
}

void voicefx::vst2::denoiser::vst2_process_float(const float* const* inputs, float** outputs, int32_t samples)
{
	// Processing Structure
	// - The first call to this function fills the "buffer" for delay, but our outputs stay empty.
	// - Any subsequent calls push data to the buffer, processes the buffer as much as possible, then takes from it.
	// - This results in 10ms of delay, but we can't do anything about it due to how VST2s are structured.
	// This is the required structure, as neither VST2 or VST3 actually offer us any control about the host.

	for (size_t idx = 0; idx < _channels.size(); idx++) {
		auto& channel = _channels[idx];

		if (samples < channel.delay) {
			D_LOG("(0x%08" PRIxPTR ")[%" PRIuPTR
				  "] Host application is broken and ignores delay, behavior is now undefined.",
				  &this->_vsteffect, idx);
		}

#ifdef DEBUG_PROCESSING
		D_LOG("(0x%08" PRIxPTR ")[%" PRIuPTR "] In Smp. | In Buf | FX Buf | OutBuf | Step   ", &this->_vsteffect, idx);
		D_LOG("(0x%08" PRIxPTR ")[%" PRIuPTR "] --------+--------+--------+--------+--------", &this->_vsteffect, idx);
		D_LOG("(0x%08" PRIxPTR ")[%" PRIuPTR "] %7" PRId32 " |%7" PRIuPTR " |%7" PRIuPTR " |%7" PRIuPTR " |%7" PRIuPTR,
			  &this->_vsteffect, idx, samples, channel.input_buffer.size(), channel.fx_buffer.size(),
			  channel.output_buffer.size(), 0);
#endif

		// Resample input data for current channel to effect sample rate.
		size_t out_samples = _scratch.size();
		if (_samplerate != channel.fx->get_sample_rate()) {
			size_t in_samples = samples;
			channel.input_resampler.process(inputs[idx], in_samples, _scratch.data(), out_samples);
#ifdef DEBUG_PROCESSING
			D_LOG("(0x%08" PRIxPTR ")[%" PRIuPTR "] Host -> FX Resample!", &this->_vsteffect, idx);
#endif
			if (in_samples != samples) {
				D_LOG("(0x%08" PRIxPTR ")[%" PRIuPTR "] Host -> FX Resample required less input samples, broken!",
					  &this->_vsteffect, idx);
			}
		} else {
			// Copy the input if sample rate matches.
			out_samples = samples;
			memcpy(_scratch.data(), inputs[idx], samples * sizeof(float));
		}
		channel.input_buffer.push(_scratch.data(), out_samples);
#ifdef DEBUG_PROCESSING
		D_LOG("(0x%08" PRIxPTR ")[%" PRIuPTR "] %7" PRId32 " |%7" PRIuPTR " |%7" PRIuPTR " |%7" PRIuPTR " |%7" PRIuPTR,
			  &this->_vsteffect, idx, samples, channel.input_buffer.size(), channel.fx_buffer.size(),
			  channel.output_buffer.size(), out_samples);
#endif

		// Denoise the content in segments of _nvfx->get_block_size()
		out_samples = channel.fx->get_block_size();
		while (channel.input_buffer.size() >= out_samples) {
			// 1. Push _nvfx->get_block_size() samples to the processor.
			channel.fx->process(channel.input_buffer.peek(out_samples), _scratch.data());

			// 2. Store the result in a buffer.
			channel.fx_buffer.push(_scratch.data(), out_samples);

			// 3. And remove the original sample data from the input buffer.
			channel.input_buffer.pop(out_samples);
#ifdef DEBUG_PROCESSING
			D_LOG("(0x%08" PRIxPTR ")[%" PRIuPTR "] %7" PRId32 " |%7" PRIuPTR " |%7" PRIuPTR " |%7" PRIuPTR
				  " |%7" PRIuPTR,
				  &this->_vsteffect, idx, samples, channel.input_buffer.size(), channel.fx_buffer.size(),
				  channel.output_buffer.size(), out_samples);
#endif
		}
#ifdef DEBUG_PROCESSING
		D_LOG("(0x%08" PRIxPTR ")[%" PRIuPTR "] %7" PRId32 " |%7" PRIuPTR " |%7" PRIuPTR " |%7" PRIuPTR " |%7" PRIuPTR,
			  &this->_vsteffect, idx, samples, channel.input_buffer.size(), channel.fx_buffer.size(),
			  channel.output_buffer.size(), 0);
#endif

		if (channel.fx_buffer.size() > 0) {
			// Resample the entirety of the processed data.
			out_samples = _scratch.size();
			if (_samplerate != channel.fx->get_sample_rate()) {
				size_t in_samples = channel.fx_buffer.size();
				channel.output_resampler.process(channel.fx_buffer.peek(in_samples), in_samples, _scratch.data(),
												 out_samples);
				channel.fx_buffer.pop(channel.fx_buffer.size());
#ifdef DEBUG_PROCESSING
				D_LOG("(0x%08" PRIxPTR ")[%" PRIuPTR "] FX -> Host Resample!", &this->_vsteffect, idx);
#endif
			} else {
				// Copy the input if sample rate matches.
				out_samples = channel.fx_buffer.size();
				memcpy(_scratch.data(), channel.fx_buffer.peek(out_samples), out_samples * sizeof(float));
				channel.fx_buffer.pop(out_samples);
			}
			channel.output_buffer.push(_scratch.data(), out_samples);
#ifdef DEBUG_PROCESSING
			D_LOG("(0x%08" PRIxPTR ")[%" PRIuPTR "] %7" PRId32 " |%7" PRIuPTR " |%7" PRIuPTR " |%7" PRIuPTR
				  " |%7" PRIuPTR,
				  &this->_vsteffect, idx, samples, channel.input_buffer.size(), channel.fx_buffer.size(),
				  channel.output_buffer.size(), out_samples);
#endif
		} else {
#ifdef DEBUG_PROCESSING
			D_LOG("(0x%08" PRIxPTR ")[%" PRIuPTR "] %7" PRId32 " |%7" PRIuPTR " |%7" PRIuPTR " |%7" PRIuPTR
				  " |%7" PRIuPTR,
				  &this->_vsteffect, idx, samples, channel.input_buffer.size(), channel.fx_buffer.size(),
				  channel.output_buffer.size(), 0);
#endif
		}

		// Copy the output data back to the host.
		if (channel.delay <= 0) {
			// Update the output buffer with the new content.
			memcpy(outputs[idx], channel.output_buffer.peek(samples), samples * sizeof(float));
			channel.output_buffer.pop(samples);
#ifdef DEBUG_PROCESSING
			D_LOG("(0x%08" PRIxPTR ")[%" PRIuPTR "] %7" PRId32 " |%7" PRIuPTR " |%7" PRIuPTR " |%7" PRIuPTR
				  " |%7" PRIuPTR,
				  &this->_vsteffect, idx, samples, channel.input_buffer.size(), channel.fx_buffer.size(),
				  channel.output_buffer.size(), samples);
#endif
		} else {
			memset(outputs[idx], 0, samples * sizeof(float));
			channel.delay -= samples;
#ifdef DEBUG_PROCESSING
			D_LOG("(0x%08" PRIxPTR ")[%" PRIuPTR "] %7" PRId32 " |%7" PRIuPTR " |%7" PRIuPTR " |%7" PRIuPTR
				  " |%7" PRIuPTR,
				  &this->_vsteffect, idx, samples, channel.input_buffer.size(), channel.fx_buffer.size(),
				  channel.output_buffer.size(), 0);
#endif
		}
	}
}
