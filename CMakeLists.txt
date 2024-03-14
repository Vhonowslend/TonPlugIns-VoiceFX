# Copyright 2020 Michael Fabian 'Xaymar' Dirks <info@xaymar.com>

################################################################################
# Bootstrap
################################################################################
cmake_minimum_required(VERSION 3.26)
project(VoiceFX
	DESCRIPTION "Clean up noise and echo with VoiceFX in post! (Part of TonPlugIns by Xaymar)"
	HOMEPAGE_URL "https://tonplugins.xaymar.com/"
)
list(APPEND CMAKE_MESSAGE_INDENT "[${PROJECT_NAME}] ")
if(ENABLE_FULL_VERSION)
	set(PROJECT_TITLE "VoiceFX")
else()
	set(PROJECT_TITLE "VoiceFX (Demo)")
endif()
define_plugin(
	NAME ${PROJECT_NAME}
	VST3_UUID "0B94A94D-0C53-4682-9230-587B568F25BF"
	VST2
	VST2_UUID "EC944BED-893B-4DA6-84AC-8C4A919D3791"
)

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
