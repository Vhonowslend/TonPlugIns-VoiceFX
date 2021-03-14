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

################################################################################
# Setup / Bootstrap
################################################################################

# CMake Setup
cmake_minimum_required(VERSION 3.17.0)

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

# Automatic Versioning
set(VERSION_MAJOR 0)
set(VERSION_MINOR 2)
set(VERSION_PATCH 2)
set(VERSION_TWEAK 0)
set(VERSION_SUFFIX "")
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/.git")
	set(GIT_RESULT "")
	set(GIT_OUTPUT "")
	execute_process(
		COMMAND git rev-list --count --topo-order ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}${VERSION_SUFFIX}..HEAD
		WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
		RESULT_VARIABLE GIT_RESULT
		OUTPUT_VARIABLE GIT_OUTPUT
		OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_STRIP_TRAILING_WHITESPACE ERROR_QUIET
	)
	if(GIT_RESULT EQUAL 0)
		set(VERSION_TWEAK ${GIT_OUTPUT})
	endif()
	execute_process(
		COMMAND git rev-parse --short=8 HEAD
		WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
		RESULT_VARIABLE GIT_RESULT
		OUTPUT_VARIABLE GIT_OUTPUT
		OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_STRIP_TRAILING_WHITESPACE ERROR_QUIET
	)
	if(GIT_RESULT EQUAL 0)
		set(PROJECT_COMMIT ${GIT_OUTPUT})
	endif()
endif()
if(PROJECT_COMMIT)
	set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK}${VERSION_SUFFIX}-${PROJECT_COMMIT}")
else()
	set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK}${VERSION_SUFFIX}")
endif()

################################################################################
# Project
################################################################################
project(xaymar-voicefx
	LANGUAGES CXX;C
	DESCRIPTION "Xaymars VoiceFX (for VST)"
	VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK}
	HOMEPAGE_URL "https://xaymar.com/"
)
set(PROJECT_FULL_NAME "Xaymars VoiceFX (for VST)")
set(PROJECT_DESCRIPTION "")
set(PROJECT_AUTHORS "Michael Fabian 'Xaymar' Dirks <info@xaymar.com>")
set(PROJECT_COPYRIGHT_YEARS "2020")

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

################################################################################
# Project Setup
################################################################################
# Dependency: libCURL
find_package(CURL)

# Dependency: VST SDK
find_package(VSTSDK REQUIRED)

# Dependency: NVIDIA Audio Effects SDK
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
	"source/lib.cpp"
	"source/lib.hpp"
	"source/nvafx.hpp"
	"source/nvafx.cpp"
	"source/nvafx_denoiser.hpp"
	"source/nvafx_denoiser.cpp"
	"source/nvafx_voicedenoiser.hpp"
	"source/nvafx_voicedenoiser.cpp"
	"source/vst2.cpp"
	"source/vst2.hpp"
	"source/vst3.cpp"
	"source/vst3.hpp"
	"source/vst3_voicedenoiser_controller.hpp"
	"source/vst3_voicedenoiser_controller.cpp"
	"source/vst3_voicedenoiser_processor.hpp"
	"source/vst3_voicedenoiser_processor.cpp"
	"source/exports_win.def"
	"cmake/templates/version.h.in"
	"cmake/templates/version.rc.in"
	"cmake/templates/installer.iss.in"
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

if(WIN32)
	#Packaging
	if("${CMAKE_PACKAGE_SUFFIX_OVERRIDE}" STREQUAL "")
		set(_PACKAGE_SUFFIX_OVERRIDE "${VERSION_STRING}")
	else()
		set(_PACKAGE_SUFFIX_OVERRIDE "${CMAKE_PACKAGE_SUFFIX_OVERRIDE}")
	endif()
	set(_PACKAGE_FULL_NAME "${CMAKE_PACKAGE_PREFIX}/${CMAKE_PACKAGE_NAME}-${_PACKAGE_SUFFIX_OVERRIDE}")

	## Installer (InnoSetup)
	get_filename_component(ISS_FILES_DIR "${CMAKE_INSTALL_PREFIX}" ABSOLUTE)
	file(TO_NATIVE_PATH "${ISS_FILES_DIR}" ISS_FILES_DIR)
	get_filename_component(ISS_PACKAGE_DIR "${CMAKE_PACKAGE_PREFIX}" ABSOLUTE)
	file(TO_NATIVE_PATH "${ISS_PACKAGE_DIR}" ISS_PACKAGE_DIR)
	get_filename_component(ISS_SOURCE_DIR "${PROJECT_SOURCE_DIR}" ABSOLUTE)
	file(TO_NATIVE_PATH "${ISS_SOURCE_DIR}" ISS_SOURCE_DIR)
	get_filename_component(ISS_MSVCHELPER_PATH "${msvc-redist-helper_BUILD_DIR}" ABSOLUTE)
	file(TO_NATIVE_PATH "${ISS_MSVCHELPER_PATH}" ISS_MSVCHELPER_PATH)
	configure_file(
		"cmake/templates/installer.iss.in"
		"${PROJECT_BINARY_DIR}/installer.iss"
	)

	# Windows Specific Resource Definition
	set(PROJECT_PRODUCT_NAME "${PROJECT_FULL_NAME}")
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
endif()

# Create Library
add_library(${PROJECT_NAME} MODULE
	${PROJECT_SOURCE}
	$<TARGET_OBJECTS:VSTSDK_VST3>
)
target_link_libraries(${PROJECT_NAME}
	${PROJECT_LIBRARIES}
)
target_include_directories(${PROJECT_NAME}
	PRIVATE
		${PROJECT_INCLUDES}
		${VSTSDK_INCLUDE_DIRS}
		${PROJECT_BINARY_DIR}
)
## VST3 requires /DELAYLOAD on Windows
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
# Installation
################################################################################

# VST3 format is this:
# ${PROJECT_NAME}.vst3/
#   Contents/
#     Resources/
#       <your custom content>
#     armv71-linux/
#       ${PROJECT_NAME}.so
#     i686-linux/
#       ${PROJECT_NAME}.so
#     i386-linux/
#       ${PROJECT_NAME}.so
#     x86_64-linux/
#       ${PROJECT_NAME}.so
#     MacOS/
#       ${PROJECT_NAME}
#     x86-win/
#       ${PROJECT_NAME}.vst3
#     x86_64-win/
#       ${PROJECT_NAME}.vst3
#     Info.plist (MacOS)
#     PkgInfo (MacOS)
#   desktop.ini (Windows)
#   Plugin.ico (Windows)
#
# Yeah. It is extremely dumb. Congrats Steinberg, you made something dumb.
# Not only is it super stupid to make up your own file ending, it's also bad behavior!

set(INSTALL_PREFIX "${PROJECT_NAME}.vst3")
if(WIN32)
	set_target_properties(${PROJECT_NAME} PROPERTIES
		SUFFIX ".vst3"
	)
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
install(DIRECTORY "resources/" DESTINATION "${INSTALL_PREFIX}/Contents/Resources")
if(WIN32)
	install(FILES "cmake/templates/desktop.ini" DESTINATION "${INSTALL_PREFIX}/")
	install(FILES "cmake/templates/icon.ico" DESTINATION "${INSTALL_PREFIX}/")
	install(CODE "execute_process(COMMAND attrib +A +S +H -R -O -I -X -V -P -U -B \"${CMAKE_INSTALL_PREFIX}/${INSTALL_PREFIX}/desktop.ini\")")
	install(CODE "execute_process(COMMAND attrib +A +S -H -R -O -I -X -V -P -U -B \"${CMAKE_INSTALL_PREFIX}/${INSTALL_PREFIX}/icon.ico\")")
endif()
install(FILES "LICENSE.md" DESTINATION "${INSTALL_PREFIX}")

add_custom_target(
	PACKAGE_7Z
	${CMAKE_COMMAND} -E tar cfv "${_PACKAGE_FULL_NAME}.7z" --format=7zip --
		"${CMAKE_INSTALL_PREFIX}/${INSTALL_PREFIX}"
	WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}"
)
add_custom_target(
	PACKAGE_ZIP
	${CMAKE_COMMAND} -E tar cfv "${_PACKAGE_FULL_NAME}.zip" --format=zip --
		"${CMAKE_INSTALL_PREFIX}/${INSTALL_PREFIX}"
	WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}"
)
