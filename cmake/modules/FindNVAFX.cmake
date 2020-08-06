# Distributed under MIT/X11 License. See https://en.wikipedia.org/wiki/MIT_License for more info.

#[=======================================================================[.rst:
FindNVAFX
--------

Find the native NVIDIA Audio Effects headers and libraries.

IMPORTED Targets
^^^^^^^^^^^^^^^^

This module defines :prop_tgt:`IMPORTED` target ``NVIDIA::AudioEffects``, if
NVAFX has been found.

Result Variables
^^^^^^^^^^^^^^^^

This module defines the following variables:

``NVAFX_FOUND``
  "True" if ``NVAFX`` found.

``NVAFX_INCLUDE_DIRS``
  where to find ``nvAudioEffects.h``, etc.

``NVAFX_LIBRARIES``
  List of libraries when using ``NVAFX``.

``NVAFX_VERSION``
  The version of ``NVAFX`` found.

``NVAFX_BINARIES``
  Binary files for use of ``NVAFX``.

``NVAFX_EXTRA_BINARIES``
  Extra binary files that are used by ``NVAFX``.

``NVAFX_MODELS``
  Model files for use by ``NVAFX``.

#]=======================================================================]

include(FindPackageHandleStandardArgs)

# Variables
set(NVAFX_DIR "" CACHE PATH "Path to NVIDIA Audio Effects SDK")

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_NVAFX QUIET nvafx)
  if(PC_NVAFX_FOUND)
    set(NVAFX_VERSION ${PC_NVAFX_VERSION})
  endif()
endif()

find_path(NVAFX_INCLUDE_DIRS
	NAMES
		"nvAudioEffects.h"
	HINTS
		ENV NVAFX_DIR
		${NVAFX_DIR}
	PATH_SUFFIXES
		include
		nvafx/include
)
find_path(NVAFX_LIBRARY_DIR
	NAMES
		"NVAudioEffects.lib"
	HINTS
		ENV NVAFX_DIR
		${NVAFX_DIR}
	PATH_SUFFIXES
		bin
		lib
		nvafx/bin
		nvafx/lib
)

set(NVAFX_LIBRARIES
	"${NVAFX_LIBRARY_DIR}/NVAudioEffects${CMAKE_LINK_LIBRARY_SUFFIX}"
)
set(NVAFX_BINARIES
	"${NVAFX_LIBRARY_DIR}/NVAudioEffects${CMAKE_SHARED_LIBRARY_SUFFIX}"
)
set(NVAFX_EXTRA_BINARIES
	"${NVAFX_LIBRARY_DIR}/external/openssl/bin/libcrypto-1_1-x64.dll"
	"${NVAFX_LIBRARY_DIR}/external/cuda/bin/cublas64_10.dll"
	"${NVAFX_LIBRARY_DIR}/external/cuda/bin/cublasLt64_10.dll"
	"${NVAFX_LIBRARY_DIR}/external/cuda/bin/cudart64_101.dll"
	"${NVAFX_LIBRARY_DIR}/external/cuda/bin/cudnn64_7.dll"
	"${NVAFX_LIBRARY_DIR}/external/cuda/bin/cufft64_10.dll"
)
set(NVAFX_MODELS
	"${NVAFX_LIBRARY_DIR}/models/denoiser_16k.wpkg"
	"${NVAFX_LIBRARY_DIR}/models/denoiser_48k.wpkg"
)

if(NVAFX_INCLUDE_DIRS)
	if(EXISTS "${NVAFX_INCLUDE_DIRS}/nvAudioEffects.h")
		file(STRINGS "${NVAFX_INCLUDE_DIRS}/nvAudioEffects.h" _version_parse REGEX "^// AUDIOEFFECTS_VERSION_[0-9]+\.[0-9]+\.[0-9]+.*$")
		string(REGEX REPLACE "// AUDIOEFFECTS_VERSION_([0-9]+).*" "\\1" _major "${_version_parse}")
		string(REGEX REPLACE "// AUDIOEFFECTS_VERSION_[0-9]+\.([0-9]+).*" "\\1" _minor "${_version_parse}")
		string(REGEX REPLACE "// AUDIOEFFECTS_VERSION_[0-9]+\.[0-9]+\.([0-9]+).*" "\\1" _patch "${_version_parse}")

		set(NVAFX_VERSION_MAJOR "${_major}")
		set(NVAFX_VERSION_MINOR "${_minor}")
		set(NVAFX_VERSION_PATCH "${_patch}")
		set(NVAFX_VERSION "${_major}.${_minor}.${_patch}")
		message(STATUS "NVAFX: Found version ${NVAFX_VERSION}.")
	else()
		message(WARNING "NVAFX: Failed to parse version information.")
	endif()
endif()

find_package_handle_standard_args(NVAFX
	FOUND_VAR NVAFX_FOUND
	REQUIRED_VARS NVAFX_INCLUDE_DIRS NVAFX_LIBRARY_DIR NVAFX_LIBRARIES NVAFX_BINARIES NVAFX_MODELS
	VERSION_VAR NVAFX_VERSION
	HANDLE_COMPONENTS
)

if(NVAFX_FOUND)
	if(NOT TARGET NVIDIA::AudioEffects)
		add_library(NVIDIA::AudioEffects SHARED IMPORTED)
		set_target_properties(NVIDIA::AudioEffects
			PROPERTIES
				PUBLIC_HEADER "${NVAFX_INCLUDE_DIRS}/nvAudioEffects.h"	
				INTERFACE_INCLUDE_DIRECTORIES ${NVAFX_INCLUDE_DIRS}
				IMPORTED_LINK_INTERFACE_LANGUAGES "C"
				IMPORTED_LOCATION ${NVAFX_BINARIES}
				IMPORTED_IMPLIB ${NVAFX_LIBRARIES}
		)
		target_include_directories(NVIDIA::AudioEffects
			INTERFACE
				${NVAFX_INCLUDE_DIRS}
		)
		target_link_libraries(NVIDIA::AudioEffects
			INTERFACE
				${NVAFX_LIBRARIES}
		)
	endif()
endif()
