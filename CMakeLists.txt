# Copyright 2020 Michael Fabian 'Xaymar' Dirks <info@xaymar.com>
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.

# CMake Setup
cmake_minimum_required(VERSION 3.13...4.0)
project(VoiceFX)

if(${CMAKE_VERSION} VERSION_LESS 3.19)
	set(LOGPREFIX "[VoiceFX] ")
else()
	list(APPEND CMAKE_MESSAGE_INDENT "[VoiceFX] ")
	set(LOGPREFIX "")
endif()

################################################################################
# Modules
################################################################################

# Search Paths
set(CMAKE_MODULE_PATH
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/clang"
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/codesign"
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/version"
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake"
)

# Include
include("util")							# CacheClear, CacheSet
include("version")						# version()
include("CheckIPOSupported")			# check_ipo_supported

################################################################################
# Detect if this is a standalone or grouped build
################################################################################

set(STANDALONE ON)
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_LIST_DIR}")
	set(PREFIX "")
else()
	set(STANDALONE OFF)
	set(PREFIX "VoiceFX_")
endif()

if(STANDALONE)
	message(STATUS "${LOGPREFIX}This is a standalone build, please make sure you've followed the instructions.")
else()
	message(STATUS "${LOGPREFIX}This is a combined build.")
endif()

################################################################################
# Platform Detection
################################################################################

# Operating System
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	set(D_PLATFORM_OS "windows")
	set(D_PLATFORM_WINDOWS 1)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(D_PLATFORM_OS "linux")
	set(D_PLATFORM_LINUX 1)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	set(D_PLATFORM_OS "macos")
	set(D_PLATFORM_MAC 1)
else()
	set(D_PLATFORM_OS "unknown")
	set(D_PLATFORM_UNKNOWN 1)
	message(WARNING "${LOGPREFIX}The operating system '${CMAKE_SYSTEM_NAME}' is unknown to to this script, continue at your own risk.")
endif()

# Architecture
set(ARCH_INSTR_32 "i386;i686;x86;arm;ARM")
set(ARCH_INSTR_64 "x86_64;AMD64;IA64;arm64;ARM64")
set(ARCH_INSTR_X86 "i386;i686;x86;x86_64;AMD64")
set(ARCH_INSTR_ARM "arm;ARM;arm64;ARM64")
set(ARCH_INSTR_ITANIUM "IA64")
set(ARCH_BITS 0)
set(ARCH_BITS_POINTER 0)
set(ARCH_INST "")

# - Bitness
list(FIND ARCH_INSTR_32 "${CMAKE_SYSTEM_PROCESSOR}" FOUND)
if(FOUND GREATER -1)
	set(ARCH_BITS 32)
endif()

list(FIND ARCH_INSTR_64 "${CMAKE_SYSTEM_PROCESSOR}" FOUND)
if(FOUND GREATER -1)
	set(ARCH_BITS 64)
endif()
set(D_PLATFORM_BITS ${ARCH_BITS})

# - Pointer Size (bits)
math(EXPR ARCH_BITS_POINTER "8*${CMAKE_SIZEOF_VOID_P}")
set(D_PLATFORM_BITS_PTR ${ARCH_BITS_POINTER})

# - Basic Instruction Set
list(FIND ARCH_INSTR_X86 "${CMAKE_SYSTEM_PROCESSOR}" FOUND)
if(FOUND GREATER -1)
	list(APPEND ARCH_INST "x86")
	set(D_PLATFORM_INSTR_X86 ON)
	set(D_PLATFORM_ARCH_X86 ON)
endif()

list(FIND ARCH_INSTR_ARM "${CMAKE_SYSTEM_PROCESSOR}" FOUND)
if(FOUND GREATER -1)
	list(APPEND ARCH_INST "ARM")
	set(D_PLATFORM_INSTR_ARM ON)
	set(D_PLATFORM_ARCH_ARM ON)
endif()

list(FIND ARCH_INSTR_ITANIUM "${CMAKE_SYSTEM_PROCESSOR}" FOUND)
if(FOUND GREATER -1)
	list(APPEND ARCH_INST "Itanium")
	set(D_PLATFORM_INSTR_ITANIUM ON)
	set(D_PLATFORM_ARCH_ITANIUM ON)
endif()

check_ipo_supported(RESULT D_HAS_IPO)

set(D_PLATFORM_INSTR ${ARCH_INST})
set(D_PLATFORM_ARCH ${ARCH_INST})
message(STATUS "${LOGPREFIX}Target is ${D_PLATFORM_BITS}bit ${ARCH_INST} with a pointer size of ${D_PLATFORM_BITS_PTR}bit.")

################################################################################
# Options
################################################################################
set(${PREFIX}VERSION "" CACHE STRING "Override detected or pre-configured version with this string. Accepts strings in CMake and SemVer 2.0.0 format.")

set(${PREFIX}PACKAGE_PREFIX "${CMAKE_BINARY_DIR}" CACHE PATH "Path for generated archives.")
set(${PREFIX}PACKAGE_NAME "${PROJECT_NAME}" CACHE STRING "Name for the generated archives.")
set(${PREFIX}PACKAGE_SUFFIX "" CACHE STRING "Override for the suffix.")

set(${PREFIX}ENABLE_FULL_VERSION ON CACHE BOOL "Enable the full feature set? (Do not enable for Demo/Free builds!)")
set(${PREFIX}ENABLE_STEINBERG_VST3 ON CACHE BOOL "Enable Steinberg VST3.x support?")
set(${PREFIX}ENABLE_STEINBERG_VST2 OFF CACHE BOOL "Enable Steinberg VST2.x support?")

set(${PREFIX}ENABLE_RESAMPLING OFF CACHE BOOL "Enable support for resampling?")

set(${PREFIX}ENABLE_LTO ON CACHE BOOL "Enable Link-Time Optimizations?")
set(${PREFIX}ENABLE_CLANG ON CACHE BOOL "Enable Clang?")
set(${PREFIX}ENABLE_CODESIGN ON CACHE BOOL "Enable code signing?")
set(${PREFIX}ENABLE_FASTMATH ON CACHE BOOL "Enable fast math optimizations? (Trades precision with performance.)")
if(D_PLATFORM_ARCH_X86)
	set(${PREFIX}TARGET_X86_64_V4 OFF CACHE BOOL "Target x86-64-v4 (x86-64-v3, AVX512F, AVX512BW, AVX512CD, AVX512DQ, AVX512VL).")
	set(${PREFIX}TARGET_X86_64_V3 OFF CACHE BOOL "Target x86-64-v3 (x86-64-v2, AVX, AVX2, BMI1, BMI2, F16C, FMA, LZCNT, MOVBE, OSXSAVE).")
	set(${PREFIX}TARGET_X86_64_V2_EX ON CACHE BOOL "Target extended x86-64-v2 (x86-64-v2, AVX, BMI1, BMI2, F16C, FMA, MOVBE).")
	set(${PREFIX}TARGET_X86_64_V2 ON CACHE BOOL "Target x86-64-v2 (x86-64, CMPXCHG16B, LAHF-SAHF, POPCNT, SSE3, SSE4.1, SSE4.2, SSSE3).")
	set(${PREFIX}TARGET_X86_64 ON CACHE BOOL "Target x86-64 (CMOV, CX8, FPU, FXSR, MMX, OSFXSR, SCE, SSE, SSE2).")
endif()
if(NOT MSVC)
	set(${PREFIX}TARGET_NATIVE OFF CACHE BOOL "Target the native CPU architecture. Enable it for development or personal builds, but disable it for distribution.")
endif()

################################################################################
# Version
################################################################################

# Version handling
set(PROJECT_VERSION_GIT_BASE "0.4.0") # For automatic versioning, which version is the "root" of the current changes?
set(PROJECT_VERSION "0.4.0") # What is our version goal?
version(PARSE PROJECT_VERSION "${PROJECT_VERSION}" REQUIRE PATCH;TWEAK)

# - If possible, automatically generate versions from git.
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/.git")
	find_program(GIT
		NAMES
			git
			git.exe
	)

	if(EXISTS "${GIT}")
		# Calculate the Tweak version component from the given base version.
		execute_process(
			COMMAND "${GIT}" describe --tags --long --match "${PROJECT_VERSION_GIT_BASE}" --abbrev=8 HEAD
			WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
			RESULT_VARIABLE GIT_RESULT
			OUTPUT_VARIABLE PROJECT_VERSION_GIT
			ERROR_VARIABLE GIT_ERROR
			OUTPUT_STRIP_TRAILING_WHITESPACE
			ERROR_STRIP_TRAILING_WHITESPACE
			ERROR_QUIET
		)
		if(GIT_RESULT EQUAL 0)
			# Some minor string editing so the output conforms to SemVer 2.0.0.
			string(REPLACE "-" ";" PROJECT_VERSION_GIT "${PROJECT_VERSION_GIT}")

			# Parse as SemVer 2.0.0
			list(GET PROJECT_VERSION_GIT 1 PROJECT_VERSION_GIT_TWEAK)
			list(GET PROJECT_VERSION_GIT 2 PROJECT_VERSION_GIT_BUILD)

			# Update our global version.
			version(MODIFY PROJECT_VERSION "${PROJECT_VERSION}" COMPRESS
				TWEAK "${PROJECT_VERSION_GIT_TWEAK}"
				BUILD "${PROJECT_VERSION_GIT_BUILD}"
				REQUIRE PATCH;TWEAK
			)
			version(PARSE PROJECT_VERSION "${PROJECT_VERSION}" REQUIRE PATCH;TWEAK)
		else()
			message(WARNING "${LOGPREFIX}Unable to detect Tweak and Build component with 'git'.")
		endif()

		# Is there a tag on the current commit?
		execute_process(
			COMMAND "${GIT}" tag "--sort=-v:refname" "--points-at" HEAD
			WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
			RESULT_VARIABLE GIT_RESULT
			OUTPUT_VARIABLE PROJECT_VERSION_TAG
			ERROR_VARIABLE GIT_ERROR
			OUTPUT_STRIP_TRAILING_WHITESPACE
			ERROR_STRIP_TRAILING_WHITESPACE
			ERROR_QUIET
		)
		if((GIT_RESULT EQUAL 0) AND (NOT "${PROJECT_VERSION_TAG}" STREQUAL ""))
			# Some minor string editing so the output conforms to SemVer 2.0.0.
			string(REGEX REPLACE "[\r\n]+.*" "" PROJECT_VERSION_TAG "${PROJECT_VERSION_TAG}")

			# Parse as SemVer 2.0.0
			version(PARSE PROJECT_VERSION_TAG "${PROJECT_VERSION_TAG}" REQUIRE PATCH;TWEAK)
			if(PROJECT_VERSION_TAG_PRERELEASE)
				string(SUBSTRING "${PROJECT_VERSION_TAG_PRERELEASE}" 1 -1 PROJECT_VERSION_TAG_TWEAK)
				string(SUBSTRING "${PROJECT_VERSION_TAG_PRERELEASE}" 0 1 PROJECT_VERSION_TAG_PRERELEASE)

				if(NOT PROJECT_VERSION_TAG_TWEAK STREQUAL PROJECT_VERSION_GIT_TWEAK)
					message(WARNING "${LOGPREFIX}'git' tag mismatches detected version: '${PROJECT_VERSION_TAG_TWEAK}' != '${PROJECT_VERSION_GIT_TWEAK}'.")
				endif()
			endif()

			# Update our global version.
			version(GENERATE PROJECT_VERSION COMPRESS
				MAJOR "${PROJECT_VERSION_TAG_MAJOR}"
				MINOR "${PROJECT_VERSION_TAG_MINOR}"
				PATCH "${PROJECT_VERSION_TAG_PATCH}"
				TWEAK "${PROJECT_VERSION_TAG_TWEAK}"
				PRERELEASE "${PROJECT_VERSION_TAG_PRERELEASE}"
				BUILD "${PROJECT_VERSION_BUILD}"
			)
			version(PARSE PROJECT_VERSION "${PROJECT_VERSION}" REQUIRE PATCH;TWEAK)
		endif()
	else()
		message(STATUS "${LOGPREFIX}'git' not found, automatic version detection disabled.")
	endif()
else()
	message(STATUS "${LOGPREFIX}Not a git repository, automatic version detection disabled.")
endif()

# - If the user specified an override, use it instead of ours.
if(NOT ("${${PREFIX}VERSION}" STREQUAL ""))
	version(PARSE PROJECT_VERSION_CFG "${${PREFIX}VERSION}")
	if("${PROJECT_VERSION_CFG_BUILD}" STREQUAL "")
		set(PROJECT_VERSION_CFG_BUILD "${PROJECT_VERSION_BUILD}")
	endif()
	version(GENERATE PROJECT_VERSION COMPRESS
		MAJOR "${PROJECT_VERSION_CFG_MAJOR}"
		MINOR "${PROJECT_VERSION_CFG_MINOR}"
		PATCH "${PROJECT_VERSION_CFG_PATCH}"
		TWEAK "${PROJECT_VERSION_CFG_TWEAK}"
		PRERELEASE "${PROJECT_VERSION_CFG_PRERELEASE}"
		BUILD "${PROJECT_VERSION_CFG_BUILD}"
	)
endif()
message(STATUS "${LOGPREFIX}Version ${PROJECT_VERSION}")
set(PROJECT_VERSION_STRING "${PROJECT_VERSION}")

################################################################################
# Project
################################################################################

# - Generate a version number compatible with CMake
version(GENERATE PROJECT_VERSION_CMAKE
	MAJOR "${PROJECT_VERSION_MAJOR}"
	MINOR "${PROJECT_VERSION_MINOR}"
	PATCH "${PROJECT_VERSION_PATCH}"
	TWEAK "${PROJECT_VERSION_TWEAK}"
)

# Project and Metadata
project(
	VoiceFX
	LANGUAGES CXX;C
	VERSION "${PROJECT_VERSION_CMAKE}"
	DESCRIPTION "VoiceFX is a VST plug-in to clean up noise from any recording."
	HOMEPAGE_URL "https://www.xaymar.com/projects/voicefx/"
)
#set(PROJECT_NAME ...)
#set(PROJECT_VERSION ...)
#set(PROJECT_HOMEPAGE_URL ...)
#seT(PROJECT_DESCRIPTION ...)
set(PROJECT_TITLE "VoiceFX")
set(PROJECT_AUTHORS "Michael Fabian 'Xaymar' Dirks <info@xaymar.com>")
set(PROJECT_COPYRIGHT "2020 - 2022, Michael Fabian Dirks. All Rights Reserved")
set(PROJECT_TRADEMARKS "VST® is a registered trademark of Steinberg Media Technologies GmbH")
set(PROJECT_SOURCE "")
set(PROJECT_SOURCE_GENERATED "")
set(PROJECT_TEMPLATES "")
set(PROJECT_INCLUDES "")
set(PROJECT_LIBRARIES "")

# If this is not a full version, add the "(Demo)" suffix.
if(NOT ${PREFIX}ENABLE_FULL_VERSION)
	set(PROJECT_TITLE "${PROJECT_TITLE} (Demo)")
endif()

# Initialize the library stuff.
add_library(${PROJECT_NAME} MODULE)

# Set some default properties on the target.
set_target_properties(${PROJECT_NAME} PROPERTIES
	VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.${PROJECT_VERSION_TWEAK}
	SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.${PROJECT_VERSION_TWEAK}

	# No import or export prefixes please.
	PREFIX ""
	IMPORT_PREFIX ""

	# C++17 with no extensions.
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF

	# Always generate position-independent code (slower but much more secure).
	POSITION_INDEPENDENT_CODE ON
)

# Link-Time/Interprocedural Optimization
if(${PREFIX}ENABLE_LTO)
	set_target_properties(${PROJECT_NAME} PROPERTIES
		INTERPROCEDURAL_OPTIMIZATION ON
	)
endif()

# C/C++ Compiler Adjustments
if(D_PLATFORM_WINDOWS AND ((CMAKE_CXX_COMPILER_ID STREQUAL "MSVC") OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")))
	# MSVC/ClangCL
	message(STATUS "${LOGPREFIX}Applying custom flags for MSVC style build.")

	# - Dynamically link Microsoft C/C++ Redistributable.
	target_compile_options(${PROJECT_NAME} PRIVATE
		$<$<CONFIG:>:/MD>
		$<$<CONFIG:Debug>:/MDd>
		$<$<CONFIG:Release>:/MD>
		$<$<CONFIG:RelWithDebInfo>:/MD>
		$<$<CONFIG:MinSizeRel>:/MD>
	)

	# - Enable most useful warnings.
	set(DISABLED_WARNINGS
		"/wd4061" "/wd4100" "/wd4180" "/wd4201" "/wd4464" "/wd4505" "/wd4514"
		"/wd4571" "/wd4623" "/wd4625" "/wd4626" "/wd4668" "/wd4710" "/wd4774"
		"/wd4820" "/wd5026" "/wd5027" "/wd5039" "/wd5045" "/wd26812"
	)
	target_compile_options(${PROJECT_NAME} PRIVATE "/W3")
	foreach(WARN ${DISABLED_WARNINGS})
		target_compile_options(${PROJECT_NAME} PRIVATE "${WARN}")
	endforeach()

	# - Require enabled instruction sets.
	if(D_PLATFORM_ARCH_X86)
		if(${PREFIX}TARGET_X86_64_V4)
			target_compile_options(${PROJECT_NAME} PRIVATE "/arch:AVX512")
			message(STATUS "${LOGPREFIX}Targeting x86-64-v4.")
		elseif(${PREFIX}TARGET_X86_64_V3)
			target_compile_options(${PROJECT_NAME} PRIVATE "/arch:AVX2")
			message(STATUS "${LOGPREFIX}Targeting x86-64-v3.")
		elseif(${PREFIX}TARGET_X86_64_V2_EX)
			target_compile_options(${PROJECT_NAME} PRIVATE "/arch:AVX")
			message(STATUS "${LOGPREFIX}Targeting extended x86-64-v2.")
		elseif(${PREFIX}TARGET_X86_64_V2)
			target_compile_options(${PROJECT_NAME} PRIVATE "/d2archSSE42")
			message(STATUS "${LOGPREFIX}Targeting x86-64-v2.")
		elseif(${PREFIX}TARGET_X86_64)
			#target_compile_options(${PROJECT_NAME} PRIVATE "/arch:SSE2")
			message(STATUS "${LOGPREFIX}Targeting x86-64.")
		endif()
	endif()

	# - Use fast unordered math if possible.
	if(${PREFIX}ENABLE_FASTMATH)
		target_compile_options(${PROJECT_NAME} PRIVATE "/fp:fast")
	else()
		target_compile_options(${PROJECT_NAME} PRIVATE "/fp:precise")
		if(MSVC_VERSION GREATER 1930)
			# Keep original behavior in VS2022 and up.
			target_compile_options(${PROJECT_NAME} PRIVATE "/fp:contract")
		endif()
	endif()

	# - Disable incremental builds
	target_compile_options(${PROJECT_NAME} PRIVATE "/INCREMENTAL:NO")

	# - Enable C++ exceptions with SEH exceptions.
	target_compile_options(${PROJECT_NAME} PRIVATE "/EHa")

	# - Enable multi-processor compiling.
	target_compile_options(${PROJECT_NAME} PRIVATE "/MP")

	# - Enable updated __cplusplus macro
	target_compile_options(${PROJECT_NAME} PRIVATE "/Zc:__cplusplus")

	# - Generic Optimizations for Release/RelWithDebInfo/MinSizeRel
	set(FLAGS
		"/OPT:REF"
		"/OPT:ICF=3"
		"/GL"
		"/Gy"
		"/GF"
		"/Ox"
		"/Ob3"
	)
	foreach(FLAG ${FLAGS})
		target_compile_options(${PROJECT_NAME} PRIVATE "$<$<CONFIG:Release,RelWithDebInfo,MinSizeRel>:${FLAG}>")
	endforeach()
elseif(D_PLATFORM_LINUX AND ((CMAKE_CXX_COMPILER_ID STREQUAL "Clang") OR (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")))
	# GCC/Clang
	message(STATUS "${LOGPREFIX}Applying custom flags for GCC/Clang style build.")

	# - Enable most useful warnings.
	target_compile_options(${PROJECT_NAME} PRIVATE "-Wall")
	target_compile_options(${PROJECT_NAME} PRIVATE "-Wextra")

	# - Require enabled instruction sets.
	if(${PREFIX}TARGET_NATIVE)
		target_compile_options(${PROJECT_NAME} PRIVATE
			"-march=native"
		)
		message(WARNING "${LOGPREFIX}Targeting native architecture. Binaries will not be distributable to other systems!")
	elseif(D_PLATFORM_ARCH_X86)
		if(${PREFIX}TARGET_X86_64_V4)
			target_compile_options(${PROJECT_NAME} PRIVATE
				"-march=x86-64-v4"
			)
			message(STATUS "${LOGPREFIX}Targeting x86-64-v4.")
		elseif(${PREFIX}TARGET_X86_64_V3)
			target_compile_options(${PROJECT_NAME} PRIVATE
				"-march=x86-64-v3"
			)
			message(STATUS "${LOGPREFIX}Targeting x86-64-v3.")
		elseif(${PREFIX}TARGET_X86_64_V2_EX)
			target_compile_options(${PROJECT_NAME} PRIVATE
				"-march=x86-64-v2"
				"-mavx"
				"-mbmi2"
				"-mbmi"
				"-mfma"
				"-mf16c"
				"-mmovbe"
				"-mpclmul"
				"-mpopcnt"
			)
			message(STATUS "${LOGPREFIX}Targeting extended x86-64-v2.")
		elseif(${PREFIX}TARGET_X86_64_V2)
			target_compile_options(${PROJECT_NAME} PRIVATE
				"-march=x86-64-v2"
			)
			message(STATUS "${LOGPREFIX}Targeting x86-64-v2.")
		elseif(${PREFIX}TARGET_X86_64)
			target_compile_options(${PROJECT_NAME} PRIVATE
				"-march=x86-64"
			)
			message(STATUS "${LOGPREFIX}Targeting x86-64.")
		endif()
		if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
			target_compile_options(${PROJECT_NAME} PRIVATE
				"-mtune=generic"
			)
		else()
			target_compile_options(${PROJECT_NAME} PRIVATE
				"-mtune=x86-64"
			)
		endif()
	endif()

	# - Use fast unordered math if possible.
	if(${PREFIX}ENABLE_FASTMATH)
		if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
			target_compile_options(${PROJECT_NAME} PRIVATE
				"-ffast-math"
			)
		else()
			target_compile_options(${PROJECT_NAME} PRIVATE
				"-ffp-model=fast"
				"-ffp-contract=on"
			)
		endif()
	else()
		target_compile_options(${PROJECT_NAME} PRIVATE
			"-ffp-model=precise"
			"-ffp-contract=on"
		)
	endif()

	# - Don't export by default, require attributes.
	# add_compile_options("-fvisibility=hidden")
elseif(D_PLATFORM_MAC AND (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang"))
	# AppleClang
	message(STATUS "${LOGPREFIX}Applying custom flags for AppleClang style build.")

	# - Enable most useful warnings.
	target_compile_options(${PROJECT_NAME} PRIVATE "-Wall")
	target_compile_options(${PROJECT_NAME} PRIVATE "-Wextra")

	# - Require enabled instruction sets.
	if(${PREFIX}TARGET_NATIVE)
		target_compile_options(${PROJECT_NAME} PRIVATE
			"-march=native"
		)
		message(WARNING "${LOGPREFIX}Targeting native architecture. Binaries will not be distributable to other systems!")
	endif()

	# - Use fast unordered math if possible.
	# FIXME: Appears to not be supported.

	# - Don't export by default, require attributes.
	# add_compile_options("-fvisibility=hidden")
endif()

# Base Project Files
list(APPEND PROJECT_SOURCE
	"source/lib.hpp"
	"source/lib.cpp"
	"source/audiobuffer.hpp"
	"source/audiobuffer.cpp"
	"source/nvidia-cuda.hpp"
	"source/nvidia-cuda.cpp"
	"source/nvidia-cuda-context.hpp"
	"source/nvidia-cuda-context.cpp"
	"source/nvidia-cuda-stream.hpp"
	"source/nvidia-cuda-stream.cpp"
	"source/nvidia-afx.hpp"
	"source/nvidia-afx.cpp"
	"source/nvidia-afx-effect.hpp"
	"source/nvidia-afx-effect.cpp"
	"source/util-bitmask.hpp"
	"source/util-library.hpp"
	"source/util-library.cpp"
	"source/util-platform.hpp"
	"source/util-platform.cpp"
)
list(APPEND PROJECT_TEMPLATES
)
list(APPEND PROJECT_DATA
	"LICENSE.md"
)
list(APPEND PROJECT_LIBRARIES
)
list(APPEND PROJECT_DEFINITIONS
)
list(APPEND PROJECT_INCLUDES
	"${PROJECT_BINARY_DIR}/generated/"
)

if(${PREFIX}ENABLE_FULL_VERSION)
	list(APPEND PROJECT_DEFINITIONS
		${PREFIX}ENABLE_FULL_VERSION
	)
endif()

if(${PREFIX}ENABLE_RESAMPLING)
	list(APPEND PROJECT_DEFINITIONS
		ENABLE_RESAMPLING
	)
	list(APPEND PROJECT_SOURCE
		"source/resampler.hpp"
		"source/resampler.cpp"
	)
endif()

if(ON) # Generate Version information file.
	configure_file(
		"templates/version.h.in"
		"${PROJECT_BINARY_DIR}/generated/version.h"
		@ONLY
	)
	list(APPEND PROJECT_TEMPLATES
		"templates/version.h.in"
	)
	list(APPEND PROJECT_SOURCE_GENERATED
		"${PROJECT_BINARY_DIR}/generated/version.h"
	)
endif()

if(D_PLATFORM_WINDOWS) # Generate Windows Resource file.
	configure_file(
		"templates/version.rc.in"
		"${PROJECT_BINARY_DIR}/generated/version.rc"
		@ONLY
	)
	list(APPEND PROJECT_TEMPLATES
		"templates/version.rc.in"
	)
	list(APPEND PROJECT_SOURCE_GENERATED
		"${PROJECT_BINARY_DIR}/generated/version.rc"
	)
endif()

if(D_PLATFORM_WINDOWS) # Generate Manifest file so Windows fucks off.
	set(MANIFEST_ARCH "amd64")
	if("${BITS}" STREQUAL "32")
		set(MANIFEST_ARCH "x86")
	endif()
	configure_file(
		"templates/win.manifest.in"
		"${PROJECT_BINARY_DIR}/generated/${PROJECT_NAME}.manifest"
		@ONLY
	)
	list(APPEND PROJECT_TEMPLATES
		"templates/win.manifest.in"
	)
	list(APPEND PROJECT_SOURCE_GENERATED
		"${PROJECT_BINARY_DIR}/generated/${PROJECT_NAME}.manifest"
	)
endif()

# Curl
find_package(CURL)

# NVIDIA Audio Effects SDK
set(NVAFX_DIR "${PROJECT_SOURCE_DIR}/third-party/nvidia-maxine-afx-sdk")
find_package(NVAFX REQUIRED)
list(APPEND PROJECT_LIBRARIES
	NVIDIA::AudioEffects
)
if(D_PLATFORM_WINDOWS)
	## Require delay loaded NVAudioEffects.dll
	set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/DELAYLOAD:NVAudioEffects.dll")
else()
	# Can this be done on Linux too?
endif()

# Resampler (Secret Rabbit Code)
if(${PREFIX}ENABLE_RESAMPLING)
	add_subdirectory(third-party/secret-rabbit-code)
	list(APPEND PROJECT_LIBRARIES
		secret-rabbit-code
	)
endif()

# API SDK: Steinberg VST3
if(${PREFIX}ENABLE_STEINBERG_VST3)
	set(VSTSDK_DIR "${PROJECT_SOURCE_DIR}/third-party/vst3sdk")
	find_package(VSTSDK REQUIRED)

	list(APPEND PROJECT_SOURCE
		"source/vst3.hpp"
		"source/vst3.cpp"
		"source/vst3_effect_controller.hpp"
		"source/vst3_effect_controller.cpp"
		"source/vst3_effect_processor.hpp"
		"source/vst3_effect_processor.cpp"
	)
	list(APPEND PROJECT_DEFINITIONS
		ENABLE_VST3
	)
	list(APPEND PROJECT_LIBRARIES
		$<TARGET_OBJECTS:VSTSDK_VST3>
	)
	list(APPEND PROJECT_INCLUDES
		${VSTSDK_INCLUDE_DIRS}
	)
endif()

# API SDK: Steinberg VST2
if(${PREFIX}ENABLE_STEINBERG_VST2)
	if(NOT EXISTS "${PROJECT_SOURCE_DIR}/third-party/vst2sdk/vst.h")
		message(FATAL_ERROR "VST2 SDK not found, make sure you cloned submodules.")
	endif()

	list(APPEND PROJECT_SOURCE
		"source/vst2.hpp"
		"source/vst2.cpp"
		"source/vst2_effect.hpp"
		"source/vst2_effect.cpp"
	)
	list(APPEND PROJECT_DEFINITIONS
		ENABLE_VST2
	)
	list(APPEND PROJECT_INCLUDES
		"${PROJECT_SOURCE_DIR}/third-party/vst2sdk"
	)
endif()

# Windows-exclusive code
if(D_PLATFORM_WINDOWS)
	list(APPEND PROJECT_SOURCE
		"source/win-d3d-context.hpp"
		"source/win-d3d-context.cpp"
	)

	target_compile_definitions(${PROJECT_NAME}
		PRIVATE
			# windows.h
			D_PLATFORM_WINDOWS_LEAN_AND_MEAN
			NOGPICAPMASKS
			NOVIRTUALKEYCODES
			#NOWINMESSAGES
			NOWINSTYLES
			NOSYSMETRICS
			NOMENUS
			NOICONS
			NOKEYSTATES
			NOSYSCOMMANDS
			NORASTEROPS
			NOSHOWWINDOW
			NOATOM
			NOCLIPBOARD
			NOCOLOR
			#NOCTLMGR
			NODRAWTEXT
			#NOGDI
			NOKERNEL
			#NOUSER
			#NONLS
			NOMB
			NOMEMMGR
			NOMETAFILE
			NOMINMAX
			#NOMSG
			NOOPENFILE
			NOSCROLL
			NOSERVICE
			NOSOUND
			#NOTEXTMETRIC
			NOWH
			NOWINOFFSETS
			NOCOMM
			NOKANJI
			#NOHELP
			NOPROFILER
			NODEFERWINDOWPOS
			NOMCX
			NOIME
			NOMDI
			NOINOUT
	)
endif()

# MSVC specific crap so we can actually use C++ and not MSVC++
if(MSVC)
	target_compile_definitions(${PROJECT_NAME}
		PRIVATE
			_CRT_SECURE_NO_WARNINGS
			_ENABLE_EXTENDED_ALIGNED_STORAGE
	)
endif()

# Finish setting up the project
target_sources(${PROJECT_NAME} PRIVATE
	${PROJECT_SOURCE}
	${PROJECT_TEMPLATES}
	${PROJECT_SOURCE_GENERATED}
	${PROJECT_DATA}
)
target_link_libraries(${PROJECT_NAME}
	${PROJECT_LIBRARIES}
)
target_include_directories(${PROJECT_NAME}
	PRIVATE
		${PROJECT_INCLUDES}
		${PROJECT_BINARY_DIR}
)
target_compile_definitions(${PROJECT_NAME}
	PRIVATE
		${PROJECT_DEFINITIONS}
)

# Visual Grouping
source_group(TREE
	"${PROJECT_SOURCE_DIR}/source"
	PREFIX "Source"
	FILES ${PROJECT_SOURCE}
)
source_group(TREE
	"${PROJECT_BINARY_DIR}/generated"
	PREFIX "Generated"
	FILES ${PROJECT_SOURCE_GENERATED}
)
source_group(TREE
	"${PROJECT_SOURCE_DIR}/templates"
	PREFIX "Templates"
	FILES ${PROJECT_TEMPLATES}
)
source_group(TREE
	"${PROJECT_SOURCE_DIR}"
	PREFIX "Data"
	FILES ${PROJECT_DATA}
)

# Don't compile some files
set_source_files_properties(${PROJECT_DATA} ${PROJECT_TEMPLATES} PROPERTIES
	HEADER_FILE_ONLY ON
)

# Clang Integration
if(${PREFIX}ENABLE_CLANG AND (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/clang/Clang.cmake"))
	include("Clang")

	generate_compile_commands_json(
		TARGETS ${PROJECT_NAME}
	)
	clang_tidy(
		TARGETS ${PROJECT_NAME}
		VERSION 9.0.0
	)
	clang_format(
		TARGETS ${PROJECT_NAME}
		DEPENDENCY
		VERSION 9.0.0
	)
endif()

# Code Signing
if(${PREFIX}ENABLE_CODESIGN AND (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/codesign/CodeSign.cmake"))
	include("CodeSign")


endif()

################################################################################
# Packaging
################################################################################

if("${${PREFIX}PACKAGE_SUFFIX}" STREQUAL "")
	set(_PACKAGE_SUFFIX "${PROJECT_VERSION_STRING}")
else()
	set(_PACKAGE_SUFFIX "${${PREFIX}PACKAGE_SUFFIX}")
endif()
if(${PREFIX}ENABLE_FULL_VERSION)
	set(_PACKAGE_NAME "${${PREFIX}PACKAGE_NAME}")
else()
	set(_PACKAGE_NAME "${${PREFIX}PACKAGE_NAME}-Demo")
endif()
set(_PACKAGE_FULL_NAME "${_PACKAGE_NAME}-${_PACKAGE_SUFFIX}")

add_custom_target(
	PACKAGE_ZIP
	${CMAKE_COMMAND} -E tar cfv "${${PREFIX}PACKAGE_PREFIX}/${_PACKAGE_FULL_NAME}.zip" --format=zip --
		"${CMAKE_INSTALL_PREFIX}"
	WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}"
)

add_custom_target(
	PACKAGE_7Z
	${CMAKE_COMMAND} -E tar cfv "${${PREFIX}PACKAGE_PREFIX}/${_PACKAGE_FULL_NAME}.7z" --format=7zip --
		"${CMAKE_INSTALL_PREFIX}"
	WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}"
)

if(${PREFIX}ENABLE_STEINBERG_VST3)
	# ${PROJECT_NAME}.vst3/
	#   Contents/
	#     Resources/
	#       <your custom content>
	#
	#- Linux
	# ${PROJECT_NAME}.vst3/
	#   Contents/
	#     armv71-linux/${PROJECT_NAME}.so
	#     i686-linux/${PROJECT_NAME}.so
	#     i386-linux/${PROJECT_NAME}.so
	#     x86_64-linux/${PROJECT_NAME}.so
	#
	#- MacOS
	# ${PROJECT_NAME}.vst3/
	#   Contents/
	#     MacOS/${PROJECT_NAME}
	#     Info.plist
	#     PkgInfo
	#
	#- Windows
	# ${PROJECT_NAME}.vst3/
	#   Contents/
	#     arm-win/${PROJECT_NAME}.dll
	#     arm_64-win/${PROJECT_NAME}.dll
	#     x86-win/${PROJECT_NAME}.dll
	#     x86_64-win/${PROJECT_NAME}.dll
	#   desktop.ini
	#   Plugin.ico

	set(VST3_INSTALL_PREFIX "VST3/${PROJECT_NAME}.vst3")
	set(VST3_CONTENT_PREFIX "${VST3_INSTALL_PREFIX}/Contents")
	set(VST3_BINARY_PREFIX "${VST3_CONTENT_PREFIX}/")
	if(D_PLATFORM_WINDOWS)
		if(D_PLATFORM_ARCH_X86)
			set(VST3_BINARY_PREFIX "${VST3_BINARY_PREFIX}/x86")
		elseif(D_PLATFORM_ARCH_ARM)
			set(VST3_BINARY_PREFIX "${VST3_BINARY_PREFIX}/arm")
		endif()

		if(D_PLATFORM_BITS EQUAL 64)
			set(VST3_BINARY_PREFIX "${VST3_BINARY_PREFIX}_64")
		endif()
		set(VST3_BINARY_PREFIX "${VST3_BINARY_PREFIX}-win")
	elseif(D_PLATFORM_LINUX)
		if(D_PLATFORM_ARCH_X86)
			set(VST3_BINARY_PREFIX "${VST3_BINARY_PREFIX}/x86")
		elseif(D_PLATFORM_ARCH_ARM)
			set(VST3_BINARY_PREFIX "${VST3_BINARY_PREFIX}/arm")
		endif()

		if(D_PLATFORM_BITS EQUAL 64)
			set(VST3_BINARY_PREFIX "${VST3_BINARY_PREFIX}_64")
		endif()
		set(VST3_BINARY_PREFIX "${VST3_BINARY_PREFIX}-linux")
	elseif(D_PLATFORM_APPLE)
		set(VST3_BINARY_PREFIX "${VST3_BINARY_PREFIX}MacOS")
	else()
		set(VST3_BINARY_PREFIX "${VST3_BINARY_PREFIX}Unknown")
	endif()

	# File Layout
	install(DIRECTORY "resources/" DESTINATION "${VST3_CONTENT_PREFIX}/Resources" USE_SOURCE_PERMISSIONS OPTIONAL)
	install(FILES "LICENSE.md" DESTINATION "${VST3_INSTALL_PREFIX}")
	install(TARGETS ${PROJECT_NAME}
		LIBRARY DESTINATION "${VST3_BINARY_PREFIX}"
		RUNTIME DESTINATION "${VST3_BINARY_PREFIX}"
	)
	if(MSVC)
		install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION "${VST3_BINARY_PREFIX}" OPTIONAL)
	endif()
	if(D_PLATFORM_WINDOWS)
		# 'desktop.ini'
		install(FILES "templates/desktop.ini" DESTINATION "${VST3_INSTALL_PREFIX}")
		install(CODE "execute_process(COMMAND attrib +S +H \"${CMAKE_INSTALL_PREFIX}/${VST3_INSTALL_PREFIX}/desktop.ini\")")

		# 'icon.ico'
		install(FILES "templates/icon.ico" DESTINATION "${VST3_INSTALL_PREFIX}")
		install(CODE "execute_process(COMMAND attrib +H \"${CMAKE_INSTALL_PREFIX}/${VST3_INSTALL_PREFIX}/icon.ico\")")

		# Rename '.dll' to '.vst3' in order to fit Steinberg's ridiculous Windows-only rules.
		get_filename_component(_TMPA "${CMAKE_INSTALL_PREFIX}/${VST3_BINARY_PREFIX}/${PROJECT_NAME}.dll" ABSOLUTE)
		get_filename_component(_TMPB "${CMAKE_INSTALL_PREFIX}/${VST3_BINARY_PREFIX}/${PROJECT_NAME}.vst3" ABSOLUTE)
		install(CODE "file(RENAME \"${_TMPA}\" \"${_TMPB}\")")
	endif()

	if(D_PLATFORM_WINDOWS) # Windows Installer (InnoSetup)
		get_filename_component(ISS_FILES_DIR "${CMAKE_INSTALL_PREFIX}/${VST3_INSTALL_PREFIX}" ABSOLUTE)
		get_filename_component(ISS_PACKAGE_DIR "${${PREFIX}PACKAGE_PREFIX}" ABSOLUTE)
		if(${PREFIX}ENABLE_FULL_VERSION)
			set(ISS_PACKAGE_NAME "${${PREFIX}PACKAGE_NAME}-VST3-${_PACKAGE_SUFFIX}")
		else()
			set(ISS_PACKAGE_NAME "${${PREFIX}PACKAGE_NAME}-VST3-Demo-${_PACKAGE_SUFFIX}")
		endif()
		configure_file(
			"templates/installer-vst3.iss.in"
			"${PROJECT_BINARY_DIR}/installer-vst3.iss"
		)
	endif()
endif()

if(${PREFIX}ENABLE_STEINBERG_VST2)
	set(VST2_INSTALL_PREFIX "VST2/${PROJECT_NAME}")

	# File Layout
	install(DIRECTORY "resources/" DESTINATION "${VST2_INSTALL_PREFIX}/Resources" USE_SOURCE_PERMISSIONS OPTIONAL)
	install(FILES "LICENSE.md" DESTINATION "${VST2_INSTALL_PREFIX}/")
	install(TARGETS ${PROJECT_NAME}
		LIBRARY DESTINATION "${VST2_INSTALL_PREFIX}/"
		RUNTIME DESTINATION "${VST2_INSTALL_PREFIX}/"
	)
	if(MSVC)
		install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION "${VST2_INSTALL_PREFIX}/" OPTIONAL)
	endif()
	if(D_PLATFORM_WINDOWS)
		# 'desktop.ini'
		install(FILES "templates/desktop.ini" DESTINATION "${VST2_INSTALL_PREFIX}")
		install(CODE "execute_process(COMMAND attrib +S +H \"${CMAKE_INSTALL_PREFIX}/${VST2_INSTALL_PREFIX}/desktop.ini\")")

		# 'icon.ico'
		install(FILES "templates/icon.ico" DESTINATION "${VST2_INSTALL_PREFIX}")
		install(CODE "execute_process(COMMAND attrib +H \"${CMAKE_INSTALL_PREFIX}/${VST2_INSTALL_PREFIX}/icon.ico\")")
	endif()

	if(D_PLATFORM_WINDOWS) # Windows Installer (InnoSetup)
		get_filename_component(ISS_FILES_DIR "${CMAKE_INSTALL_PREFIX}/${VST2_INSTALL_PREFIX}" ABSOLUTE)
		get_filename_component(ISS_PACKAGE_DIR "${${PREFIX}PACKAGE_PREFIX}" ABSOLUTE)
		if(${PREFIX}ENABLE_FULL_VERSION)
			set(ISS_PACKAGE_NAME "${${PREFIX}PACKAGE_NAME}-VST2-${_PACKAGE_SUFFIX}")
		else()
			set(ISS_PACKAGE_NAME "${${PREFIX}PACKAGE_NAME}-VST2-Demo-${_PACKAGE_SUFFIX}")
		endif()
		configure_file(
			"templates/installer-vst2.iss.in"
			"${PROJECT_BINARY_DIR}/installer-vst2.iss"
		)
	endif()
endif()
