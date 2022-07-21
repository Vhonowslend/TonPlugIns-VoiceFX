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
cmake_minimum_required(VERSION 3.17.0)

################################################################################
# Configure Type
################################################################################

# Detect if we are building by ourselves or as part of something else.
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_LIST_DIR}")
	set(GROUPED OFF)
	set(PREFIX "")
else()
	set(GROUPED ON)
	set(PREFIX "VoiceFX_")
endif()
set(LOGPREFIX "VoiceFX:")

################################################################################
# Versioning
################################################################################

set(VERSION_MAJOR 0)
set(VERSION_MINOR 3)
set(VERSION_PATCH 2)
set(VERSION_TWEAK 0)
set(VERSION_SUFFIX "")
set(VERSION_COMMIT "00000000")

# Check if we are in a git repository.
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/.git")
	# Try and figure out where git is.
	find_program(GIT git
		PATHS
			/bin
			/sbin
			/usr/bin
			/usr/local/bin
	)

	if(GIT)
		set(GIT_RESULT)
		set(GIT_OUTPUT)
		set(GIT_ERROR)

		execute_process(
			COMMAND "${GIT}" describe --tags --long --match "[0-9]*.[0-9]*.[0-9]*" --abbrev=8 HEAD
			WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
			RESULT_VARIABLE GIT_RESULT
			OUTPUT_VARIABLE GIT_OUTPUT
			ERROR_VARIABLE GIT_ERROR
			OUTPUT_STRIP_TRAILING_WHITESPACE
			ERROR_STRIP_TRAILING_WHITESPACE
			ERROR_QUIET
		)

		if(GIT_RESULT EQUAL 0)
			string(REPLACE "-" "." GIT_OUTPUT "${GIT_OUTPUT}")
			string(REPLACE "." ";" GIT_OUTPUT "${GIT_OUTPUT}")

			# Parse Version
			list(GET GIT_OUTPUT 0 VERSION_MAJOR)
			list(GET GIT_OUTPUT 1 VERSION_MINOR)
			list(GET GIT_OUTPUT 2 VERSION_PATCH)
			list(GET GIT_OUTPUT 3 VERSION_TWEAK)
			list(GET GIT_OUTPUT 4 VERSION_COMMIT)

			# Patch needs additional parsing.
			# This may be a [0-9]*[a-z]*[0-9]+ string.
			string(REGEX MATCHALL "^([0-9]+)([a-z]+[0-9]+)?" T_MATCHES "${VERSION_PATCH}")
			set(VERSION_PATCH "${CMAKE_MATCH_1}")
			if(CMAKE_MATCH_2)
				set(VERSION_SUFFIX "${CMAKE_MATCH_2}")
			else()
				set(VERSION_SUFFIX "")
			endif()
		else()
			message(WARNING "${LOGPREFIX} Failed to detect version, using default instead.")
		endif()
	endif()
else()
	message(STATUS "${LOGPREFIX} Not a git repository, automatic version detection disabled.")
endif()

# Allow manual overrides of the detected version.
set(${PREFIX}VERSION "" CACHE STRING "Override voicefx version with this string. Format: Major.Minor.Patch[Suffix][-Tweak[-Commit8c]]")
if(NOT (${PREFIX}VERSION STREQUAL ""))
	string(REPLACE "-" "." T_VERSION "${${PREFIX}VERSION}")
	string(REPLACE "." ";" T_VERSION "${${PREFIX}VERSION}")

	list(LENGTH T_VERSION T_VERSIONLEN)
	list(GET T_VERSION 0 VERSION_MAJOR)
	list(GET T_VERSION 1 VERSION_MINOR)
	list(GET T_VERSION 2 VERSION_PATCH)
	if (T_VERSIONLEN GREATER_EQUAL 3)
		list(GET T_VERSION 3 VERSION_TWEAK)
	else()
		set(VERSION_BUILD 0)
	endif()
	if (T_VERSIONLEN GREATER_EQUAL 4)
		list(GET T_VERSION 4 VERSION_COMMIT)
	else()
		set(VERSION_COMMIT "")
	endif()

	# Patch needs additional parsing.
	# This may be a [0-9]*[a-z]*[0-9]+ string.
	string(REGEX MATCHALL "^([0-9]+)([a-z]+[0-9]+)?" T_MATCHES "${VERSION_PATCH}")
	set(VERSION_PATCH "${CMAKE_MATCH_1}")
	if(CMAKE_MATCH_2)
		set(VERSION_SUFFIX "${CMAKE_MATCH_2}")
	else()
		set(VERSION_SUFFIX "")
	endif()
endif()

# Generate Version String
if(NOT (VERSION_COMMIT STREQUAL ""))
	set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK}${VERSION_SUFFIX}-${VERSION_COMMIT}")
else()
	set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK}${VERSION_SUFFIX}")
endif()

# Log the detected version.
message(STATUS "${LOGPREFIX} Version ${VERSION_STRING}")

################################################################################
# Setup / Bootstrap
################################################################################

# Search Paths
set(CMAKE_MODULE_PATH
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/clang"
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake"
)

# CMake Modules
include("util")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Clang/Clang.cmake")
	include("Clang")
	set(HAVE_CLANG ON)
endif()

################################################################################
# Project
################################################################################
project(VoiceFX
	LANGUAGES CXX;C
	DESCRIPTION "VoiceFX is a VST 3.x/2.x plug-in to clean up noise from any recording in post."
	VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK}
	HOMEPAGE_URL "https://xaymar.com/"
)
set(PROJECT_FULL_NAME "VoiceFX")
set(PROJECT_DESCRIPTION "")
set(PROJECT_AUTHORS "Michael Fabian 'Xaymar' Dirks <info@xaymar.com>")
set(PROJECT_COPYRIGHT_YEARS "2020 - 2021")

# Detect Architecture
set(BITS "64")
math(EXPR BITS "8*${CMAKE_SIZEOF_VOID_P}")
set(ARCH "x64")
if("${BITS}" STREQUAL "32")
	set(ARCH "x86")
endif()

# Apply compiler specific changes.
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR (WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
	message(STATUS "Applying custom flags for MSVC style build.")

	# MSVC/ClangCL
	# - Statically link Microsoft C/C++ Redistributable.
	# - Enable /W3 and disable useless warnings.
	# - Enable C++ exceptions with SEH exceptions.
	# - Enable multi-processor compiling.

	# Enable most useful warnings.
	set(DISABLED_WARNINGS
		"/wd4061" "/wd4100" "/wd4180" "/wd4201" "/wd4464" "/wd4505" "/wd4514"
		"/wd4571" "/wd4623" "/wd4625" "/wd4626" "/wd4668" "/wd4710" "/wd4774"
		"/wd4820" "/wd5026" "/wd5027" "/wd5039" "/wd5045" "/wd26812"
	)
	add_compile_options("/W3")
	foreach(WARN ${DISABLED_WARNINGS})
		add_compile_options("${WARN}")
	endforeach()

	# Build with static MSVC linkage.
    add_compile_options(
        $<$<CONFIG:>:/MT>
        $<$<CONFIG:Debug>:/MTd>
        $<$<CONFIG:Release>:/MT>
        $<$<CONFIG:RelWithDebInfo>:/MT>
        $<$<CONFIG:MinSizeRel>:/MT>
    )

	# C++ Exceptions & SEH
	add_compile_options("/EHa")

	# Multiprocessor compiling
	add_compile_options("/MP")

elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	message(STATUS "Applying custom flags for GNU style build.")

	# Clang/AppleClang/GNU
	# - Don't export by default.
	# - Enable all and extra warnings.

	add_compile_options("-Wall")
	add_compile_options("-Wextra")
	add_compile_options("-fvisibility=hidden")

endif()

# C++ Standard and Extensions
## Use C++17 and no non-standard extensions.
set(_CXX_STANDARD 17)
set(_CXX_EXTENSIONS OFF)

################################################################################
# Options
################################################################################
set(CMAKE_PACKAGE_PREFIX "${CMAKE_BINARY_DIR}" CACHE PATH "Path for generated archives.")
set(CMAKE_PACKAGE_NAME "${PROJECT_NAME}" CACHE STRING "Name for the generated archives.")
set(CMAKE_PACKAGE_SUFFIX_OVERRIDE "" CACHE STRING "Override for the suffix.")

set(ENABLE_FULL_VERSION ON CACHE BOOL "Enable the full feature set? (Do not enable for Demo/Free builds!)")
set(ENABLE_STEINBERG_VST3 ON CACHE BOOL "Enable Steinberg VST3.x support.")
set(ENABLE_STEINBERG_VST2 OFF CACHE BOOL "Enable Steinberg VST2.x support.")

################################################################################
# Project Setup
################################################################################
# Dependency: libCURL
find_package(CURL)

# Dependency: NVIDIA Audio Effects SDK
set(NVAFX_DIR "${PROJECT_SOURCE_DIR}/third-party/nvidia-maxine-afx-sdk")
find_package(NVAFX REQUIRED)

# Dependency: Secret Rabbit Code
add_subdirectory(third-party/secret-rabbit-code)

# Configure files.
configure_file(
	"${PROJECT_SOURCE_DIR}/cmake/templates/version.h.in"
	"${PROJECT_BINARY_DIR}/generated/version.h"
	@ONLY
)

# Source
set(PROJECT_SOURCE
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
	"source/resampler.hpp"
	"source/resampler.cpp"
	"source/util-bitmask.hpp"
	"source/util-library.hpp"
	"source/util-library.cpp"
	"source/util-platform.hpp"
	"source/util-platform.cpp"
	"cmake/templates/version.h.in"
	"cmake/templates/version.rc.in"
	"${PROJECT_BINARY_DIR}/generated/version.h"
)
set(PROJECT_DATA
)
set(PROJECT_LIBRARIES
	NVIDIA::AudioEffects
	secret-rabbit-code
)
set(PROJECT_DEFINITIONS
)
set(PROJECT_INCLUDES
	"${PROJECT_BINARY_DIR}/generated/"
	"${PROJECT_SOURCE_DIR}/third-party/vst2sdk"
)

if(ENABLE_FULL_VERSION)
	list(APPEND PROJECT_DEFINITIONS
		ENABLE_FULL_VERSION
	)
endif()

# VST 3.x
if(ENABLE_STEINBERG_VST3)
	set(VSTSDK_DIR "${PROJECT_SOURCE_DIR}/third-party/vst3sdk")
	find_package(VSTSDK REQUIRED)

	list(APPEND PROJECT_SOURCE
		"source/vst3.hpp"
		"source/vst3.cpp"
		"source/vst3_effect_controller.hpp"
		"source/vst3_effect_controller.cpp"
		"source/vst3_effect_processor.hpp"
		"source/vst3_effect_processor.cpp"
		"cmake/templates/installer-vst3.iss.in"
		$<TARGET_OBJECTS:VSTSDK_VST3>
	)
	list(APPEND PROJECT_DEFINITIONS
		ENABLE_VST3
	)
	list(APPEND PROJECT_INCLUDES
		${VSTSDK_INCLUDE_DIRS}
	)
endif()

# VST 2.x
if(ENABLE_STEINBERG_VST2)
	list(APPEND PROJECT_SOURCE
		"source/vst2.hpp"
		"source/vst2.cpp"
		"source/vst2_effect.hpp"
		"source/vst2_effect.cpp"
		"cmake/templates/installer-vst2.iss.in"
	)
	list(APPEND PROJECT_DEFINITIONS
		ENABLE_VST2
	)
	list(APPEND PROJECT_INCLUDES
		"${PROJECT_SOURCE_DIR}/third-party/vst2sdk"
	)
endif()

if(WIN32)
	#Packaging
	if("${CMAKE_PACKAGE_SUFFIX_OVERRIDE}" STREQUAL "")
		set(_PACKAGE_SUFFIX_OVERRIDE "${VERSION_STRING}")
	else()
		set(_PACKAGE_SUFFIX_OVERRIDE "${CMAKE_PACKAGE_SUFFIX_OVERRIDE}")
	endif()
	set(_PACKAGE_FULL_NAME "${CMAKE_PACKAGE_PREFIX}/${CMAKE_PACKAGE_NAME}-${_PACKAGE_SUFFIX_OVERRIDE}")

	# Windows Specific Resource Definition
	if(ENABLE_FULL_VERSION)
		set(PROJECT_PRODUCT_NAME "${PROJECT_FULL_NAME}")
	else()
		set(PROJECT_PRODUCT_NAME "${PROJECT_FULL_NAME} (Demo)")
	endif()
	set(PROJECT_COMPANY_NAME "${PROJECT_AUTHORS}")
	set(PROJECT_COPYRIGHT "${PROJECT_AUTHORS} © ${PROJECT_COPYRIGHT_YEARS}")
	set(PROJECT_LEGAL_TRADEMARKS_1 "")
	set(PROJECT_LEGAL_TRADEMARKS_2 "VST is a registered trademark of Steinberg Media Technologies GmbH")

	configure_file(
		"cmake/templates/version.rc.in"
		"${PROJECT_BINARY_DIR}/generated/version.rc"
		@ONLY
	)

	list(APPEND PROJECT_SOURCE
		"${PROJECT_BINARY_DIR}/generated/version.rc"
	)

	if(ON) # Windows Manifest Definition
		set(MANIFEST_ARCH "amd64")
		if("${BITS}" STREQUAL "32")
			set(MANIFEST_ARCH "x86")
		endif()
		configure_file(
			"cmake/templates/win.manifest.in"
			"${PROJECT_BINARY_DIR}/generated/${PROJECT_NAME}.manifest"
			@ONLY
		)
		list(APPEND PROJECT_SOURCE
			"${PROJECT_BINARY_DIR}/generated/${PROJECT_NAME}.manifest"
		)		
	endif()
endif()

# Create Library
add_library(${PROJECT_NAME} MODULE
	${PROJECT_SOURCE}
)
target_link_libraries(${PROJECT_NAME}
	${PROJECT_LIBRARIES}
)
target_include_directories(${PROJECT_NAME}
	PRIVATE
		${PROJECT_INCLUDES}
		${PROJECT_BINARY_DIR}
)
## Require delay loaded NVAudioEffects.dll
if(WIN32)
	set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/DELAYLOAD:NVAudioEffects.dll")
endif()

# Definitions
target_compile_definitions(${PROJECT_NAME}
	PRIVATE
		${PROJECT_DEFINITIONS}
)
if(MSVC)
	target_compile_definitions(${PROJECT_NAME}
		PRIVATE
			_CRT_SECURE_NO_WARNINGS
			_ENABLE_EXTENDED_ALIGNED_STORAGE
	)
endif()
if(WIN32)
	target_compile_definitions(${PROJECT_NAME}
		PRIVATE
			# windows.h
			WIN32_LEAN_AND_MEAN
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

# C++ Standard and Extensions
set_target_properties(
	${PROJECT_NAME}
	PROPERTIES
		CXX_STANDARD ${_CXX_STANDARD}
		CXX_STANDARD_REQUIRED ON
		CXX_EXTENSIONS ${_CXX_EXTENSIONS}
)

# File Version
if(WIN32)
	set_target_properties(
		${PROJECT_NAME}
		PROPERTIES
		VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.${PROJECT_VERSION_TWEAK}
		SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.${PROJECT_VERSION_TWEAK}
	)
else()
	set_target_properties(
		${PROJECT_NAME}
		PROPERTIES
		VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.${PROJECT_VERSION_TWEAK}
		SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.${PROJECT_VERSION_TWEAK}
	)
endif()

# Clang Tools
if(HAVE_CLANG)
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

################################################################################
# Packaging / Installation
################################################################################

if (ENABLE_STEINBERG_VST2)
	# Configure VST2 installer
	get_filename_component(ISS_FILES_DIR "${CMAKE_INSTALL_PREFIX}/VST2" ABSOLUTE)
	file(TO_NATIVE_PATH "${ISS_FILES_DIR}" ISS_FILES_DIR)
	get_filename_component(ISS_PACKAGE_DIR "${CMAKE_PACKAGE_PREFIX}" ABSOLUTE)
	file(TO_NATIVE_PATH "${ISS_PACKAGE_DIR}" ISS_PACKAGE_DIR)
	get_filename_component(ISS_SOURCE_DIR "${PROJECT_SOURCE_DIR}" ABSOLUTE)
	file(TO_NATIVE_PATH "${ISS_SOURCE_DIR}" ISS_SOURCE_DIR)
	get_filename_component(ISS_MSVCHELPER_PATH "${msvc-redist-helper_BUILD_DIR}" ABSOLUTE)
	file(TO_NATIVE_PATH "${ISS_MSVCHELPER_PATH}" ISS_MSVCHELPER_PATH)
	if(NOT ENABLE_FULL_VERSION)
		set(_PACKAGE_EXTRA_NAME " (Demo)")
		set(_PACKAGE_EXTRA_FILENAME "-Demo")
	endif()
	configure_file(
		"cmake/templates/installer-vst2.iss.in"
		"${PROJECT_BINARY_DIR}/installer-vst2.iss"
	)

	# Install files
	set(INSTALL_PREFIX "VST2/${PROJECT_NAME}")
	# - Binary files
	install(TARGETS ${PROJECT_NAME}
		LIBRARY DESTINATION "${INSTALL_PREFIX}/"
		RUNTIME DESTINATION "${INSTALL_PREFIX}/"
	)
	# - Debug files
	if(MSVC)
		install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION "${INSTALL_PREFIX}/" OPTIONAL)
	endif()
	# - License
	install(FILES "LICENSE.md" DESTINATION "${INSTALL_PREFIX}/")
	# - Windows Explorer modifications.
	if(WIN32)
		install(FILES "cmake/templates/desktop.ini" DESTINATION "${INSTALL_PREFIX}/")
		install(FILES "cmake/templates/icon.ico" DESTINATION "${INSTALL_PREFIX}/")

		# Ensure that desktop.ini is System+Hidden+Readonly
		get_filename_component(_TMP "${CMAKE_INSTALL_PREFIX}/${INSTALL_PREFIX}/desktop.ini" ABSOLUTE)
		file(TO_NATIVE_PATH "${_TMP}" _TMP)
		STRING(REGEX REPLACE "\\\\" "\\\\\\\\" _TMP "${_TMP}")
		install(CODE "execute_process(COMMAND attrib +S +R +H \"${_TMP}\")")

		# Ensure that icon.ico is Hidden+Readonly
		get_filename_component(_TMP "${CMAKE_INSTALL_PREFIX}/${INSTALL_PREFIX}/icon.ico" ABSOLUTE)
		file(TO_NATIVE_PATH "${_TMP}" _TMP)
		STRING(REGEX REPLACE "\\\\" "\\\\\\\\" _TMP "${_TMP}")
		install(CODE "execute_process(COMMAND attrib +R +H \"${_TMP}\")")
	endif()
endif()

if(ENABLE_STEINBERG_VST3)
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
#
#- Shared
# ${PROJECT_NAME}.vst3/
#   Contents/
#     Resources/
#       <your custom content>

	# Configure VST3 installer
	get_filename_component(ISS_FILES_DIR "${CMAKE_INSTALL_PREFIX}/VST3" ABSOLUTE)
	file(TO_NATIVE_PATH "${ISS_FILES_DIR}" ISS_FILES_DIR)
	get_filename_component(ISS_PACKAGE_DIR "${CMAKE_PACKAGE_PREFIX}" ABSOLUTE)
	file(TO_NATIVE_PATH "${ISS_PACKAGE_DIR}" ISS_PACKAGE_DIR)
	get_filename_component(ISS_SOURCE_DIR "${PROJECT_SOURCE_DIR}" ABSOLUTE)
	file(TO_NATIVE_PATH "${ISS_SOURCE_DIR}" ISS_SOURCE_DIR)
	get_filename_component(ISS_MSVCHELPER_PATH "${msvc-redist-helper_BUILD_DIR}" ABSOLUTE)
	file(TO_NATIVE_PATH "${ISS_MSVCHELPER_PATH}" ISS_MSVCHELPER_PATH)
	if(NOT ENABLE_FULL_VERSION)
		set(_PACKAGE_EXTRA_NAME " (Demo)")
		set(_PACKAGE_EXTRA_FILENAME "-Demo")
	endif()
	configure_file(
		"cmake/templates/installer-vst3.iss.in"
		"${PROJECT_BINARY_DIR}/installer-vst3.iss"
	)

	# Install files
	set(INSTALL_PREFIX "VST3/${PROJECT_NAME}.vst3")
	if(WIN32)
		if(${BITS} STREQUAL "32")
			set(INSTALL_PATH "x86-win")
		else()
			set(INSTALL_PATH "x86_64-win")
		endif()
	elseif(APPLE)
		set(INSTALL_PATH "MacOS")
	else()
		if(${BITS} STREQUAL "32")
			set(INSTALL_PATH "i386-linux")
		else()
			set(INSTALL_PATH "x86_64-linux")
		endif()
	endif()
	install(TARGETS ${PROJECT_NAME}
		LIBRARY DESTINATION "${INSTALL_PREFIX}/Contents/${INSTALL_PATH}"
		RUNTIME DESTINATION "${INSTALL_PREFIX}/Contents/${INSTALL_PATH}"
	)
	if(MSVC)
		install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION "${INSTALL_PREFIX}/Contents/${INSTALL_PATH}" OPTIONAL)
	endif()
	if(WIN32)
		# Steinberg wants Windows DLLs to be .vst3?
		get_filename_component(_TMPA "${CMAKE_INSTALL_PREFIX}/${INSTALL_PREFIX}/Contents/${INSTALL_PATH}/${PROJECT_NAME}.dll" ABSOLUTE)
		get_filename_component(_TMPB "${CMAKE_INSTALL_PREFIX}/${INSTALL_PREFIX}/Contents/${INSTALL_PATH}/${PROJECT_NAME}.vst3" ABSOLUTE)
		install(CODE "file(RENAME \"${_TMPA}\" \"${_TMPB}\")")
	endif()

	install(DIRECTORY "resources/" DESTINATION "${INSTALL_PREFIX}/Contents/Resources" OPTIONAL)
	if(WIN32)
		install(FILES "cmake/templates/desktop.ini" DESTINATION "${INSTALL_PREFIX}/")
		install(FILES "cmake/templates/icon.ico" DESTINATION "${INSTALL_PREFIX}/")

		# Ensure that desktop.ini is System+Hidden+Readonly
		get_filename_component(_TMP "${CMAKE_INSTALL_PREFIX}/${INSTALL_PREFIX}/desktop.ini" ABSOLUTE)
		file(TO_NATIVE_PATH "${_TMP}" _TMP)
		STRING(REGEX REPLACE "\\\\" "\\\\\\\\" _TMP "${_TMP}")
		install(CODE "execute_process(COMMAND attrib +S +R +H \"${_TMP}\")")

		# Ensure that icon.ico is Hidden+Readonly
		get_filename_component(_TMP "${CMAKE_INSTALL_PREFIX}/${INSTALL_PREFIX}/icon.ico" ABSOLUTE)
		file(TO_NATIVE_PATH "${_TMP}" _TMP)
		STRING(REGEX REPLACE "\\\\" "\\\\\\\\" _TMP "${_TMP}")
		install(CODE "execute_process(COMMAND attrib +R +H \"${_TMP}\")")
	endif()
	install(FILES "LICENSE.md" DESTINATION "${INSTALL_PREFIX}")
endif()

add_custom_target(
	PACKAGE_7Z
	${CMAKE_COMMAND} -E tar cfv "${_PACKAGE_FULL_NAME}.7z" --format=7zip --
		"${CMAKE_INSTALL_PREFIX}"
	WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}"
)
add_custom_target(
	PACKAGE_ZIP
	${CMAKE_COMMAND} -E tar cfv "${_PACKAGE_FULL_NAME}.zip" --format=zip --
		"${CMAKE_INSTALL_PREFIX}"
	WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}"
)
