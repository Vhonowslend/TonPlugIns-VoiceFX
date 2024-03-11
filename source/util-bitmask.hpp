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
#include <cstddef>
#include <type_traits>

template<typename Enum>
struct enable_bitmask_operators {
	static const bool enable = false;
};

template<typename Enum>
typename std::enable_if<enable_bitmask_operators<Enum>::enable, Enum>::type operator|(Enum lhs, Enum rhs)
{
	using underlying = typename std::underlying_type<Enum>::type;
	return static_cast<Enum>(static_cast<underlying>(lhs) | static_cast<underlying>(rhs));
}

template<typename Enum>
typename std::enable_if<enable_bitmask_operators<Enum>::enable, Enum>::type operator&(Enum lhs, Enum rhs)
{
	using underlying = typename std::underlying_type<Enum>::type;
	return static_cast<Enum>(static_cast<underlying>(lhs) & static_cast<underlying>(rhs));
}

template<typename Enum>
typename std::enable_if<enable_bitmask_operators<Enum>::enable, bool>::type any(Enum lhs)
{
	using underlying = typename std::underlying_type<Enum>::type;
	return static_cast<underlying>(lhs) != static_cast<underlying>(0);
}

template<typename Enum>
typename std::enable_if<enable_bitmask_operators<Enum>::enable, bool>::type exact(Enum lhs, Enum rhs)
{
	using underlying = typename std::underlying_type<Enum>::type;
	return static_cast<underlying>(lhs) == static_cast<underlying>(rhs);
}

template<typename Enum>
typename std::enable_if<enable_bitmask_operators<Enum>::enable, bool>::type has(Enum lhs, Enum rhs)
{
	using underlying = typename std::underlying_type<Enum>::type;
	return (lhs & rhs) == rhs;
}

#define P_ENABLE_BITMASK_OPERATORS(x)    \
	template<>                           \
	struct enable_bitmask_operators<x> { \
		static const bool enable = true; \
	};
