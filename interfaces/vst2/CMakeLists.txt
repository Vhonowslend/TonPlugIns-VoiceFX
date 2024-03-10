# Copyright 2020 Michael Fabian 'Xaymar' Dirks <info@xaymar.com>

################################################################################
# Bootstrap
################################################################################
cmake_minimum_required(VERSION 3.26)

#project(VoiceFX.vst2)
define_interface_vst2(${PROJECT_NAME})

# Search Paths
list(APPEND CMAKE_MODULE_PATH
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake"
)

################################################################################
# Dependencies
################################################################################
target_link_libraries(${INTERFACE_NAME} PUBLIC VoiceFX)

################################################################################
# Finish
################################################################################
setup_target(${INTERFACE_NAME})
