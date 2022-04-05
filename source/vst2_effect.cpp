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

#include "vst2_effect.hpp"
#include <algorithm>
#include "lib.hpp"

#define D_LOG(MESSAGE, ...) voicefx::log("<vst2::effect> " MESSAGE, __VA_ARGS__)

voicefx::vst2::effect::effect(vst_host_callback cb)
	: _vsteffect(), _vstcb(cb), _input_arrangement(), _output_arrangement(), _dirty(true), _in_resampler(), _fx(),
	  _out_resampler(), _channels(), _delay(0), _local_delay(0)
{
	D_LOG("(0x%08" PRIxPTR ") Initializing...", &this->_vsteffect);

	{ // Set-up the initial VST2.x structure for processing.

		_vsteffect.magic_number      = 'VstP';
		_vsteffect.num_programs      = 0;
		_vsteffect.num_params        = 0;
		_vsteffect.flags             = 0b00010000;
		_vsteffect.delay             = 441;
		_vsteffect._unknown_float_00 = 1.0;
		_vsteffect.effect_internal   = this;
		_vsteffect.unique_id         = 'XVFX';
		_vsteffect.version = (VERSION_MAJOR << 24) | (VERSION_MINOR << 16) | (VERSION_PATCH << 8) | (VERSION_BUILD);

		_vsteffect.control = [](vst_effect* pthis, VST_EFFECT_OPCODE opcode, int32_t p_int1, intptr_t p_int2,
								void* p_ptr, float p_float) {
			try {
				return static_cast<voicefx::vst2::effect*>(pthis->effect_internal)
					->vst2_control(opcode, p_int1, p_int2, p_ptr, p_float);
			} catch (std::exception const& ex) {
				D_LOG("(0x%08" PRIxPTR ") Exception in control: %s", pthis, ex.what());
			} catch (...) {
				D_LOG("(0x%08" PRIxPTR ") Unknown exception in control.", pthis);
			}
			return intptr_t();
		};
		_vsteffect.set_parameter = [](vst_effect* pthis, uint32_t index, float value) {
			try {
				static_cast<voicefx::vst2::effect*>(pthis->effect_internal)->vst2_set_parameter(index, value);
			} catch (std::exception const& ex) {
				D_LOG("(0x%08" PRIxPTR ") Exception in set_parameter: %s", pthis, ex.what());
			} catch (...) {
				D_LOG("(0x%08" PRIxPTR ") Unknown exception in set_parameter.", pthis);
			}
		};
		_vsteffect.get_parameter = [](vst_effect* pthis, uint32_t index) {
			try {
				return static_cast<voicefx::vst2::effect*>(pthis->effect_internal)->vst2_get_parameter(index);
			} catch (std::exception const& ex) {
				D_LOG("(0x%08" PRIxPTR ") Exception in get_parameter: %s", pthis, ex.what());
			} catch (...) {
				D_LOG("(0x%08" PRIxPTR ") Unknown exception in get_parameter.", pthis);
			}
			return float();
		};
		_vsteffect.process = [](vst_effect* pthis, const float* const* inputs, float** outputs, int32_t samples) {
			try {
				static_cast<voicefx::vst2::effect*>(pthis->effect_internal)
					->vst2_process_float(inputs, outputs, samples);
			} catch (std::exception const& ex) {
				D_LOG("(0x%08" PRIxPTR ") Exception in process: %s", pthis, ex.what());
			} catch (...) {
				D_LOG("(0x%08" PRIxPTR ") Unknown exception in process.", pthis);
			}
		};
		_vsteffect.process_float = [](vst_effect* pthis, const float* const* inputs, float** outputs, int32_t samples) {
			try {
				static_cast<voicefx::vst2::effect*>(pthis->effect_internal)
					->vst2_process_float(inputs, outputs, samples);
			} catch (std::exception const& ex) {
				D_LOG("(0x%08" PRIxPTR ") Exception in process_float: %s", pthis, ex.what());
			} catch (...) {
				D_LOG("(0x%08" PRIxPTR ") Unknown exception in process_float.", pthis);
			}
		};
		_vsteffect.process_double = nullptr;

		{ // Set up default speaker arrangement.
			_input_arrangement      = {};
			_output_arrangement     = {};
			_input_arrangement.type = _output_arrangement.type = VST_ARRANGEMENT_TYPE_MONO;
			_input_arrangement.channels = _output_arrangement.channels = 1;
			for (size_t idx = 0; idx < VST_MAX_CHANNELS; idx++) {
				_input_arrangement.speakers[idx].type = _output_arrangement.speakers[idx].type = VST_SPEAKER_TYPE_MONO;
			}

			// Update our VST.
			_vsteffect.num_inputs = _vsteffect.num_outputs = _input_arrangement.channels;
		}
	}

	// Allocate the necessary resources.
	_in_resampler  = std::make_shared<::voicefx::resampler>();
	_out_resampler = std::make_shared<::voicefx::resampler>();
	_fx            = std::make_shared<::nvidia::afx::effect>();

	// Set the channel count to the default.
	set_channel_count(1);
}

voicefx::vst2::effect::~effect() {}

void voicefx::vst2::effect::reset()
{
	// Early-exit if the effect isn't "dirty".
	if (!_dirty) {
		return;
	}

	D_LOG("(0x%08" PRIxPTR ") Resetting...", &this->_vsteffect);

	// Update resamplers
	_in_resampler->ratio(_sample_rate, ::nvidia::afx::effect::samplerate());
	_in_resampler->clear();
	_in_resampler->load();
	_out_resampler->ratio(::nvidia::afx::effect::samplerate(), _sample_rate);
	_out_resampler->clear();
	_out_resampler->load();

	// Calculate absolute effect delay
	_delay = ::nvidia::afx::effect::delay();
	_delay += ::nvidia::afx::effect::blocksize();
	if (_sample_rate != ::nvidia::afx::effect::samplerate()) {
		_delay = std::llround(_delay / _in_resampler->ratio());
		_delay += ::voicefx::resampler::calculate_delay(_sample_rate, ::nvidia::afx::effect::samplerate());
		_delay += ::voicefx::resampler::calculate_delay(_sample_rate, ::nvidia::afx::effect::samplerate());
	}
	_local_delay = ::nvidia::afx::effect::blocksize();
	D_LOG("(0x%08" PRIxPTR ") Estimated latency is %" PRIu32 " samples.", &this->_vsteffect, _delay);

	// Update channel buffers.
	for (auto& channel : _channels) {
		channel.in_buffer.resize(_sample_rate);
		channel.in_fx.resize(::nvidia::afx::effect::samplerate());
		channel.out_fx.resize(::nvidia::afx::effect::samplerate());
		channel.out_buffer.resize(_sample_rate);
	}

	// Load the effect itself.
	_fx->load();

	_dirty = false;
}

void voicefx::vst2::effect::set_channel_count(size_t num)
{
	D_LOG("(0x%08" PRIxPTR ") Adjusting effect channels to %" PRIuPTR "...", &this->_vsteffect, num);

	if (num != _channels.size()) {
		_dirty = true;

		_in_resampler->channels(num);
		_out_resampler->channels(num);
		_fx->channels(num);

		_channels.resize(num);
		_channels.shrink_to_fit();
	}
}

intptr_t voicefx::vst2::effect::vst2_control(VST_EFFECT_OPCODE opcode, int32_t p1, intptr_t p2, void* p3, float p4)
try {
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
		return vst2_suspend_resume(p2 != 0);
	case VST_EFFECT_OPCODE_PROCESS_BEGIN:
		reset();
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

	D_LOG("(0x%08" PRIxPTR ") Unhandled: %04" PRIx32 " %04 " PRIx32 " %016 " PRIxPTR " %016 " PRIxPTR " %f",
		  &this->_vsteffect, opcode, p1, p2, p3, p4);
	return 0;
} catch (std::exception const& ex) {
	D_LOG("(0x%08" PRIxPTR ") Exception in control: %s", &this->_vsteffect, ex.what());
	D_LOG("(0x%08" PRIxPTR ") %04" PRIx32 " %04 " PRIx32 " %016 " PRIxPTR " %016 " PRIxPTR " %f", &this->_vsteffect,
		  opcode, p1, p2, p3, p4);
	return 1;
} catch (...) {
	D_LOG("(0x%08" PRIxPTR ") Unknown exception in control.", &this->_vsteffect);
	D_LOG("(0x%08" PRIxPTR ") %04" PRIx32 " %04 " PRIx32 " %016 " PRIxPTR " %016 " PRIxPTR " %f", &this->_vsteffect,
		  opcode, p1, p2, p3, p4);
	return 1;
}

intptr_t voicefx::vst2::effect::vst2_get_vendor_name(char* buffer, size_t buffer_len) const
{
	memset(buffer, 0, buffer_len);
	memcpy(buffer, voicefx::product_vendor.data(), std::min<size_t>(voicefx::product_vendor.size(), buffer_len));
	return 0;
}

intptr_t voicefx::vst2::effect::vst2_get_vendor_version() const
{
	return 0;
}

intptr_t voicefx::vst2::effect::vst2_get_product_name(char* buffer, size_t buffer_len) const
{
	memset(buffer, 0, buffer_len);
	memcpy(buffer, voicefx::product_name.data(), std::min<size_t>(voicefx::product_name.size(), buffer_len));
	return 0;
}

intptr_t voicefx::vst2::effect::vst2_get_effect_name(char* buffer, size_t buffer_len) const
{
	memset(buffer, 0, buffer_len);
	memcpy(buffer, voicefx::product_name.data(), std::min<size_t>(voicefx::product_name.size(), buffer_len));
	return 0;
}

intptr_t voicefx::vst2::effect::vst2_get_effect_category() const
{
	return VST_CATEGORY_RESTORATION;
}

intptr_t voicefx::vst2::effect::vst2_create()
{
	// Equivalent of setupPRocessing() in VST3.x.
	reset();
	return 0;
}

intptr_t voicefx::vst2::effect::vst2_destroy()
{
	delete this;
	return 0;
}

intptr_t voicefx::vst2::effect::vst2_set_sample_rate(float_t sample_rate)
{
	uint32_t samplerate = std::lroundf(sample_rate);

	if (_sample_rate != samplerate) {
		_dirty       = true;
		_sample_rate = samplerate;
	}

	reset();
	return 0;
}

intptr_t voicefx::vst2::effect::vst2_set_block_size(intptr_t block_size)
{
	// We don't care about the incoming block size, our effect is unrelated to it.
	return 0;
}

intptr_t voicefx::vst2::effect::vst2_get_speaker_arrangement(vst_speaker_arrangement const** input,
															 vst_speaker_arrangement const** output) const
{
	*input  = &_input_arrangement;
	*output = &_output_arrangement;
	return 0;
}

intptr_t voicefx::vst2::effect::vst2_set_speaker_arrangement(vst_speaker_arrangement* input,
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

	// Update the output to match what we actually output.
	output->type     = input->type;
	output->channels = input->channels;
	for (size_t idx = 0; idx < output->channels; idx++) {
		output->speakers[idx] = input->speakers[idx];
	}

	D_LOG("(0x%08" PRIxPTR ") Speaker Arrangement adjusted to %" PRId32 " channels.", &this->_vsteffect,
		  _input_arrangement.channels);

	// Update Channels
	set_channel_count(_input_arrangement.channels);

	return 0;
}

intptr_t voicefx::vst2::effect::vst2_suspend_resume(bool should_resume)
{
	if (should_resume)
		reset();
	return 0;
}

void voicefx::vst2::effect::vst2_set_parameter(uint32_t index, float value) {}

float voicefx::vst2::effect::vst2_get_parameter(uint32_t index) const
{
	return 0.0F;
}

void voicefx::vst2::effect::vst2_process_float(const float* const* inputs, float** outputs, int32_t samples)
try {
	// Exit-early if host application ignores our delay request.
	if ((_local_delay == _delay) && (samples < _delay)) {
		D_LOG(
			"Host only provided %zu samples of the required %zu samples to overcome latency. Further behavior is "
			"undefined.",
			samples, _delay);
	}

	{ // Processing begins from here on out.
		bool                      resample = _sample_rate != ::nvidia::afx::effect::samplerate();
		std::vector<const float*> in_buffers(_channels.size(), nullptr);
		std::vector<float*>       out_buffers(_channels.size(), nullptr);

		// Push data into the (unresampled) input buffers.
		for (size_t idx = 0; idx < _channels.size(); idx++) {
			auto& channel = _channels[idx];
			if (resample) {
				channel.in_buffer.push(inputs[idx], samples);
				in_buffers[idx] = channel.in_buffer.peek(0);
#ifdef DEBUG_BUFFERS
				D_LOG("{%02" PRIuMAX "} Pushed %" PRId32 " samples to input resample buffer.", idx, samples);
#endif
			} else {
				channel.in_fx.push(inputs[idx], samples);
				in_buffers[idx] = channel.in_fx.peek(0);
#ifdef DEBUG_BUFFERS
				D_LOG("{%02" PRIuMAX "} Pushed %" PRId32 " samples to effect input buffer.", idx, samples);
#endif
			}
#ifdef DEBUG_BUFFERS
			D_LOG("{%02" PRIuMAX "} %8" PRIuMAX " %8" PRIuMAX " %8" PRIuMAX " %8" PRIuMAX " %8" PRId64, idx,
				  channel.in_buffer.size(), channel.in_fx.size(), channel.out_fx.size(), channel.out_buffer.size(),
				  _local_delay);
#endif
		}

		// Resample the input data to match the effect sample rate.
		if (resample) {
			// Update the output buffer pointers.
			for (size_t idx = 0; idx < _channels.size(); idx++) {
				auto& channel    = _channels[idx];
				out_buffers[idx] = channel.in_fx.back();
			}

			// Resample the given audio
			size_t in_samples  = _channels[0].in_buffer.size();
			size_t out_samples = _channels[0].in_fx.avail();
			_in_resampler->process(in_buffers.data(), in_samples, in_samples, out_buffers.data(), out_samples,
								   out_samples);

			// Update channel information and pointers.
			for (size_t idx = 0; idx < _channels.size(); idx++) {
				auto& channel = _channels[idx];

				// Pop the used samples from the input buffer.
				channel.in_buffer.pop(in_samples);

				// Push the generated samples into the output buffer.
				channel.in_fx.reserve(out_samples);

				// Update input buffer for next effect.
				in_buffers[idx] = channel.in_fx.peek(0);

#ifdef DEBUG_BUFFERS
				D_LOG("{%02" PRIuMAX "} Resampled % " PRIuMAX " samples to %" PRIuMAX " samples.", idx, in_samples,
					  out_samples);
				D_LOG("{%02" PRIuMAX "} %8" PRIuMAX " %8" PRIuMAX " %8" PRIuMAX " %8" PRIuMAX " %8" PRId64, idx,
					  channel.in_buffer.size(), channel.in_fx.size(), channel.out_fx.size(), channel.out_buffer.size(),
					  _local_delay);
#endif
			}
		}

		// Process as much data as possible.
		if (_channels[0].in_fx.avail() > ::nvidia::afx::effect::blocksize()) {
			// Calculate the total number of chunks that can be processed.
			size_t chunks =
				_channels[0].in_fx.size() - (_channels[0].in_fx.size() % ::nvidia::afx::effect::blocksize());

			// Update the output buffer pointers.
			for (size_t idx = 0; idx < _channels.size(); idx++) {
				auto& channel = _channels[idx];
				if (resample) {
					out_buffers[idx] = channel.out_fx.back();
				} else {
					out_buffers[idx] = channel.out_buffer.back();
				}
			}

			// Process data.
#ifdef DEBUG_BUFFERS
			D_LOG("Processing %" PRIuMAX " samples (%" PRIuMAX " blocks).", chunks,
				  chunks / ::nvidia::afx::effect::blocksize());
#endif
			_fx->process(in_buffers.data(), out_buffers.data(), chunks);

			// Update channel information and pointers.
			for (size_t idx = 0; idx < _channels.size(); idx++) {
				auto& channel = _channels[idx];

				// Pop the used samples from the input buffer.
				channel.in_fx.pop(chunks);

				// Push the generated samples into the output buffer.
				if (resample) {
					channel.out_fx.reserve(chunks);
					in_buffers[idx] = channel.out_fx.peek(0);
				} else {
					channel.out_buffer.reserve(chunks);
					in_buffers[idx] = channel.out_buffer.peek(0);
				}

#ifdef DEBUG_BUFFERS
				D_LOG("{%02" PRIuMAX "} %8" PRIuMAX " %8" PRIuMAX " %8" PRIuMAX " %8" PRIuMAX " %8" PRId64, idx,
					  channel.in_buffer.size(), channel.in_fx.size(), channel.out_fx.size(), channel.out_buffer.size(),
					  _local_delay);
#endif
			}
		}

		// Resample the input data to match the effect sample rate.
		if (resample) {
			// Update the output buffer pointers.
			for (size_t idx = 0; idx < _channels.size(); idx++) {
				auto& channel    = _channels[idx];
				out_buffers[idx] = channel.out_buffer.back();
			}

			// Resample the given audio
			size_t in_samples  = _channels[0].out_fx.size();
			size_t out_samples = _channels[0].out_buffer.avail();
			_out_resampler->process(in_buffers.data(), in_samples, in_samples, out_buffers.data(), out_samples,
									out_samples);

			// Update channel information and pointers.
			for (size_t idx = 0; idx < _channels.size(); idx++) {
				auto& channel = _channels[idx];

				// Pop the used samples from the input buffer.
				channel.out_fx.pop(in_samples);

				// Push the generated samples into the output buffer.
				channel.out_buffer.reserve(out_samples);

				// Update input buffer for next effect.
				in_buffers[idx] = channel.out_buffer.peek(0);

#ifdef DEBUG_BUFFERS
				D_LOG("{%02" PRIuMAX "} Resampled % " PRIuMAX " samples to %" PRIuMAX " samples.", idx, in_samples,
					  out_samples);
				D_LOG("{%02" PRIuMAX "} %8" PRIuMAX " %8" PRIuMAX " %8" PRIuMAX " %8" PRIuMAX " %8" PRId64, idx,
					  channel.in_buffer.size(), channel.in_fx.size(), channel.out_fx.size(), channel.out_buffer.size(),
					  _local_delay);
#endif
			}
		}

		// Output data.
		int64_t delay_adjustment = _channels[0].out_buffer.size();
		if ((_local_delay < samples) || (_local_delay == 0)) {
			// Return full or partial data.
			size_t offset = _local_delay;
			size_t length = samples - _local_delay;

			for (size_t idx = 0; idx < _channels.size(); idx++) {
				auto& channel = _channels[idx];
				if (offset > 0) {
					memset(outputs[idx], 0, offset * sizeof(float));
				}
				memcpy(outputs[idx] + offset, channel.out_buffer.front(), length * sizeof(float));
				channel.out_buffer.pop(length);

#ifdef DEBUG_BUFFER_CONTENT
				for (size_t ndx = 0; ndx < samples; ndx += 8) {
					D_LOG(
						"{%02" PRIuMAX "} %8.4f %8.4f %8.4f %8.4f %8.4f %8.4f %8.4f %8.4f", idx,
						data.outputs[0].channelBuffers32[idx][ndx], data.outputs[0].channelBuffers32[idx][ndx + 1],
						data.outputs[0].channelBuffers32[idx][ndx + 2], data.outputs[0].channelBuffers32[idx][ndx + 3],
						data.outputs[0].channelBuffers32[idx][ndx + 4], data.outputs[0].channelBuffers32[idx][ndx + 5],
						data.outputs[0].channelBuffers32[idx][ndx + 6], data.outputs[0].channelBuffers32[idx][ndx + 7]);
				}
#endif

#ifdef DEBUG_BUFFERS
				D_LOG("{%02" PRIuMAX "} Output % " PRIuMAX " samples at %" PRIuMAX " sample offset.", idx, length,
					  offset);
				D_LOG("{%02" PRIuMAX "} %8" PRIuMAX " %8" PRIuMAX " %8" PRIuMAX " %8" PRIuMAX " %8" PRId64, idx,
					  channel.in_buffer.size(), channel.in_fx.size(), channel.out_fx.size(), channel.out_buffer.size(),
					  _local_delay);
#endif
			}
		} else {
			// In all other cases, return nothing.
			for (size_t idx = 0; idx < _channels.size(); idx++) {
				auto& channel = _channels[idx];
				memset(outputs[idx], 0, samples * sizeof(float));
#ifdef DEBUG_BUFFERS
				D_LOG("{%02" PRIuMAX "} %8" PRIuMAX " %8" PRIuMAX " %8" PRIuMAX " %8" PRIuMAX " %8" PRId64, idx,
					  channel.in_buffer.size(), channel.in_fx.size(), channel.out_fx.size(), channel.out_buffer.size(),
					  _local_delay);
#endif
			}
		}
		_local_delay = std::max<int64_t>(_local_delay - delay_adjustment, 0);
	}
} catch (std::exception const& ex) {
	D_LOG("(0x%08" PRIxPTR ") Exception in process: %s", &this->_vsteffect, ex.what());
#ifdef DEBUG_BUFFERS
	for (size_t idx = 0; idx < _channels.size(); idx++) {
		auto& channel = _channels[idx];
		D_LOG("{%02" PRIuMAX "} %8" PRIuMAX " %8" PRIuMAX " %8" PRIuMAX " %8" PRIuMAX " %8" PRId64, idx,
			  channel.in_buffer.size(), channel.in_fx.size(), channel.out_fx.size(), channel.out_buffer.size(),
			  _local_delay);
	}
#endif
} catch (...) {
	D_LOG("(0x%08" PRIxPTR ") Unknown exception in process.", &this->_vsteffect);
}
