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
#include "lib.hpp"

#include "warning-disable.hpp"
#include <stdexcept>
#include "warning-enable.hpp"

nvidia::cuda::context::~context()
{
	D_LOG_LOUD("");
	if (_is_primary) {
		_cuda->cuDevicePrimaryCtxRelease(_dev);
	} else {
		_cuda->cuCtxDestroy(_ctx);
	}
}

nvidia::cuda::context::context() : _cuda(::nvidia::cuda::cuda::get()), _dev(), _ctx(), _is_primary(false)
{
	D_LOG_LOUD("");
}

nvidia::cuda::context::context(::nvidia::cuda::device_t device) : context()
{
	D_LOG_LOUD("");
	_dev        = device;
	_is_primary = true;

	if (auto res = _cuda->cuDevicePrimaryCtxRetain(&_ctx, device); res != ::nvidia::cuda::result::SUCCESS) {
		throw ::nvidia::cuda::exception(res);
	}
}

nvidia::cuda::context::context(::nvidia::cuda::context_flags flags, ::nvidia::cuda::device_t device) : context()
{
	D_LOG_LOUD("");
	_dev        = device;
	_is_primary = false;

	if (auto res = _cuda->cuCtxCreate(&_ctx, flags, device); res != ::nvidia::cuda::result::SUCCESS) {
		throw ::nvidia::cuda::exception(res);
	}

	// Restore original Context.
	pop();
}

::nvidia::cuda::context_t nvidia::cuda::context::get()
{
	D_LOG_LOUD("");
	return _ctx;
}

std::shared_ptr<::nvidia::cuda::context_stack> nvidia::cuda::context::enter()
{
	D_LOG_LOUD("");
	return std::make_shared<::nvidia::cuda::context_stack>(shared_from_this());
}

void nvidia::cuda::context::push()
{
	D_LOG_LOUD("");
	if (auto res = _cuda->cuCtxPushCurrent(_ctx); res != ::nvidia::cuda::result::SUCCESS) {
		throw ::nvidia::cuda::exception(res);
	}
}

void nvidia::cuda::context::pop()
{
	D_LOG_LOUD("");
	::nvidia::cuda::context_t ctx;
	_cuda->cuCtxPopCurrent(&ctx);
}

void nvidia::cuda::context::synchronize()
{
	D_LOG_LOUD("");
	if (auto res = _cuda->cuCtxSynchronize(); res != ::nvidia::cuda::result::SUCCESS) {
		throw ::nvidia::cuda::exception(res);
	}
}
