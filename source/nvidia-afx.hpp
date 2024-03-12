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
#include <filesystem>
#include <memory>
#include <nvAudioEffects.h>
#include "nvidia-cuda-context.hpp"
#include "nvidia-cuda.hpp"
#include "util-library.hpp"

#define DEFAULT_CONTEXT
#define PRIMARY_CONTEXT

#ifdef WIN32
#include "win-d3d-context.hpp"
#endif

#define P_AFX_DEFINE_FUNCTION(name, ...)          \
	private:                                      \
	typedef NvAFX_Status (*t##name)(__VA_ARGS__); \
                                                  \
	public:                                       \
	t##name name = nullptr;

namespace nvidia::afx {
	class afx {
		std::filesystem::path                     _redist_path;
		std::shared_ptr<::voicefx::util::library> _library;
		std::shared_ptr<::nvidia::cuda::cuda>     _cuda;
		std::shared_ptr<::nvidia::cuda::context>  _cuda_context;

#ifdef WIN32
		std::shared_ptr<::voicefx::windows::d3d::context> _d3d;
		std::wstring                                      _dll_search_path;
		void*                                             _dll_cookie;
#endif

		private:
		afx();

		public:
		~afx();

		private:
		std::vector<int32_t> enumerate_devices();

		public:
		std::filesystem::path redistributable_path();

		std::filesystem::path model_path(NvAFX_EffectSelector effect);

		std::shared_ptr<::nvidia::cuda::context> cuda_context();

#ifdef WIN32
		void windows_fix_dll_search_paths();
#endif

		public:
		decltype(NvAFX_GetEffectList)*       GetEffectList;
		decltype(NvAFX_CreateEffect)*        CreateEffect;
		decltype(NvAFX_DestroyEffect)*       DestroyEffect;
		decltype(NvAFX_SetU32)*              SetU32;
		decltype(NvAFX_SetString)*           SetString;
		decltype(NvAFX_SetFloat)*            SetFloat;
		decltype(NvAFX_GetU32)*              GetU32;
		decltype(NvAFX_GetString)*           GetString;
		decltype(NvAFX_GetFloat)*            GetFloat;
		decltype(NvAFX_Load)*                Load;
		decltype(NvAFX_GetSupportedDevices)* GetSupportedDevices;
		decltype(NvAFX_Run)*                 Run;
		decltype(NvAFX_Reset)*               Reset;

		public /* Singleton */:
		static std::shared_ptr<::nvidia::afx::afx> instance();
	};
} // namespace nvidia::afx
