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

#include "win-d3d-context.hpp"
#include "lib.hpp"

#include "warning-disable.hpp"
#include <d3d11.h>
#include <dxgi.h>
#include "warning-enable.hpp"

voicefx::windows::d3d::context::~context()
{
	if (_d3d11_context) {
		reinterpret_cast<ID3D11DeviceContext*>(_d3d11_context)->Release();
		D_LOG_DEBUG("Released ID3D11DeviceContext");
	}
	if (_d3d11_device) {
		reinterpret_cast<ID3D11Device*>(_d3d11_device)->Release();
		D_LOG_DEBUG("Released ID3D11Device");
	}
	if (_dxgi_adapter) {
		reinterpret_cast<IDXGIAdapter1*>(_dxgi_adapter)->Release();
		D_LOG_DEBUG("Released ID3D11Adapter1");
	}
	if (_dxgi_factory) {
		reinterpret_cast<IDXGIFactory1*>(_dxgi_factory)->Release();
		D_LOG_DEBUG("Released IDXGIFactory1");
	}
	D_LOG_DEBUG("Destroyed");
}

voicefx::windows::d3d::context::context(::nvidia::cuda::luid_t luid) : _dxgi_library(), _dxgi_factory(nullptr), _dxgi_adapter(nullptr), _d3d11_library(), _d3d11_device(nullptr), _d3d11_context(nullptr)
{
	char message_buffer[1024];

	// Attempt to load DXGI.
	_dxgi_library                                     = ::tonplugins::platform::library::load(std::string_view("dxgi.dll"));
	decltype(CreateDXGIFactory1)* lCreateDXGIFactory1 = reinterpret_cast<decltype(CreateDXGIFactory1)*>(_dxgi_library->load_symbol("CreateDXGIFactory1"));
	if (!lCreateDXGIFactory1) {
		throw std::runtime_error("Failed to find CreateDXGIFactory1 in 'dxgi.dll'.");
	}
	D_LOG_DEBUG("Found CreateDXGIFactory1");

	// Attempt to load D3D11.
	_d3d11_library                                  = ::tonplugins::platform::library::load(std::string_view("d3d11.dll"));
	decltype(D3D11CreateDevice)* lD3D11CreateDevice = reinterpret_cast<decltype(D3D11CreateDevice)*>(_d3d11_library->load_symbol("D3D11CreateDevice"));
	if (!lD3D11CreateDevice) {
		throw std::runtime_error("Failed to find D3D11CreateDevice in 'd3d11.dll'.");
	}
	D_LOG_DEBUG("Found D3D11CreateDevice");

	// Try and initialize DXGI to 1.1.
	if (auto res = lCreateDXGIFactory1(__uuidof(IDXGIFactory1), &_dxgi_factory); res != S_OK) {
		snprintf(message_buffer, sizeof(message_buffer), "Failed to create DXGIFactory. (Code %08" PRIX32 ")\0", (int32_t)res);
		throw std::runtime_error(message_buffer);
	}
	IDXGIFactory1* dxgi_factory = reinterpret_cast<IDXGIFactory1*>(_dxgi_factory);
	D_LOG_DEBUG("Acquired IDXGIFactory1");

	// Find the matching adapter in the list of adapters.
	IDXGIAdapter1* dxgi_adapter;
	for (UINT idx = 0; dxgi_factory->EnumAdapters1(idx, &dxgi_adapter) != DXGI_ERROR_NOT_FOUND; idx++) {
		DXGI_ADAPTER_DESC1 desc;
		if (auto res = dxgi_adapter->GetDesc1(&desc); res != S_OK) {
			dxgi_adapter->Release();
			continue;
		}

		if ((desc.AdapterLuid.HighPart != luid.parts.high) || (desc.AdapterLuid.LowPart != luid.parts.low)) {
			dxgi_adapter->Release();
			continue;
		}

		_dxgi_adapter = dxgi_adapter;
		break;
	}
	if (!_dxgi_adapter) {
		throw std::runtime_error("Failed to find matching Adapter.");
	}
	D_LOG_DEBUG("Acquired IDXGIAdapter1");

	// Create Device
	std::vector<D3D_FEATURE_LEVEL> levels = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};
	D3D_FEATURE_LEVEL level;
	if (auto res = lD3D11CreateDevice(reinterpret_cast<IDXGIAdapter1*>(_dxgi_adapter), D3D_DRIVER_TYPE_UNKNOWN, NULL, D3D11_CREATE_DEVICE_BGRA_SUPPORT, levels.data(), levels.size(), D3D11_SDK_VERSION, reinterpret_cast<ID3D11Device**>(_d3d11_device), &level, reinterpret_cast<ID3D11DeviceContext**>(_d3d11_context)); (res != S_OK) && (res != S_FALSE)) {
		snprintf(message_buffer, sizeof(message_buffer), "Failed to create D3D11Device. (Code %08" PRIX32 ")\0", (int32_t)res);
		throw std::runtime_error(message_buffer);
	}
	D_LOG_DEBUG("Acquired ID3D11Device and ID3D11DeviceContext");
}
