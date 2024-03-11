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
target_link_libraries(${PROJECT_NAME} PUBLIC
	NVIDIA::AudioEffects
)

# Resampler (Secret Rabbit Code)
add_subdirectory(third-party/secret-rabbit-code)
target_link_libraries(${PROJECT_NAME} PUBLIC
	secret-rabbit-code
)

################################################################################
# Finish
################################################################################
setup_target(${PROJECT_NAME})
