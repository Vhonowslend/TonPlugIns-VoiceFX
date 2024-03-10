# Copyright 2020 Michael Fabian 'Xaymar' Dirks <info@xaymar.com>

################################################################################
# Bootstrap
################################################################################
cmake_minimum_required(VERSION 3.26)
list(APPEND CMAKE_MESSAGE_INDENT "[VoiceFX] ")

project(VoiceFX)
define_plugin(VoiceFX)

# Search Paths
list(APPEND CMAKE_MODULE_PATH
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake"
)

################################################################################
# Dependencies
################################################################################

# Curl
find_package(CURL)

# NVIDIA Audio Effects SDK
set(NVAFX_DIR "${PROJECT_SOURCE_DIR}/third-party/nvidia-maxine-afx-sdk")
find_package(NVAFX REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE
	NVIDIA::AudioEffects
)

# Resampler (Secret Rabbit Code)
add_subdirectory(third-party/secret-rabbit-code)
target_link_libraries(${PROJECT_NAME} PRIVATE
	secret-rabbit-code
)

################################################################################
# Generate Files
################################################################################

# Generate Version information file.
configure_file(
	"templates/version.h.in"
	"${PROJECT_BINARY_DIR}/source/version.h"
	@ONLY
)

# Windows-specific
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
	# Generate Windows Resource file.
	configure_file(
		"templates/version.rc.in"
		"${PROJECT_BINARY_DIR}/source/version.rc"
		@ONLY
	)

	# Generate Manifest file so Windows fucks off.
	if(TARGET_SYSTEM_IS_X86 AND TARGET_SYSTEM_IS_ARM)
		set(MANIFEST_ARCH "arm64ec")
	elseif(TARGET_SYSTEM_IS_X86 AND TARGET_SYSTEM_IS_64BIT)
		set(MANIFEST_ARCH "amd64")
	elseif(TARGET_SYSTEM_IS_ARM AND TARGET_SYSTEM_IS_64BIT)
		set(MANIFEST_ARCH "arm64")
	elseif(TARGET_SYSTEM_IS_X86)
		set(MANIFEST_ARCH "x86")
	elseif(TARGET_SYSTEM_IS_ARM)
		set(MANIFEST_ARCH "arm")
	else()
		set(MANIFEST_ARCH "unknown")
	endif()
	configure_file(
		"templates/win.manifest.in"
		"${PROJECT_BINARY_DIR}/source/${PROJECT_NAME}.manifest"
		@ONLY
	)
endif()

################################################################################
# Finish
################################################################################
setup_target(${PROJECT_NAME})
