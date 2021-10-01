/*
 * Modern effects for a modern Streamer
 * Copyright (C) 2020 Michael Fabian Dirks
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "nvidia-cuda-context.hpp"
#include <stdexcept>
#include "lib.hpp"

#define D_LOG(MESSAGE, ...) voicefx::log("<CUDA::Context> " MESSAGE, __VA_ARGS__)

nvidia::cuda::context::~context()
{
	_cuda->cuCtxDestroy(_ctx);
}

nvidia::cuda::context::context() : _cuda(::nvidia::cuda::cuda::get()), _ctx() {}

nvidia::cuda::context::context(context_flags flags, device_t device) : context()
{
	if (auto res = _cuda->cuCtxCreate(&_ctx, flags, device); res != ::nvidia::cuda::result::SUCCESS) {
		throw ::nvidia::cuda::exception(res);
	}
}

::nvidia::cuda::context_t nvidia::cuda::context::get()
{
	return _ctx;
}

std::shared_ptr<::nvidia::cuda::context_stack> nvidia::cuda::context::enter()
{
	return std::make_shared<::nvidia::cuda::context_stack>(shared_from_this());
}

void nvidia::cuda::context::push()
{
	if (auto res = _cuda->cuCtxPushCurrent(_ctx); res != ::nvidia::cuda::result::SUCCESS) {
		throw ::nvidia::cuda::exception(res);
	}
}

void nvidia::cuda::context::pop()
{
	::nvidia::cuda::context_t ctx;
	_cuda->cuCtxPopCurrent(&ctx);
}

void nvidia::cuda::context::synchronize()
{
	if (auto res = _cuda->cuCtxSynchronize(); res != ::nvidia::cuda::result::SUCCESS) {
		throw ::nvidia::cuda::exception(res);
	}
}
