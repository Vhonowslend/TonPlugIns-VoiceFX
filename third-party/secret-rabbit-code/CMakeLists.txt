# FreeBSD 2-Clause

################################################################################
# Setup / Bootstrap
################################################################################

# CMake Setup
cmake_minimum_required(VERSION 3.17.0)

# Modules
include(CheckIncludeFile)
include(CheckTypeSize)
include(CheckFunctionExists)

################################################################################
# Project
################################################################################
project(secret-rabbit-code
	LANGUAGES C
	DESCRIPTION "Secret Rabbit Code"
	VERSION 0.2.2
)

check_include_file("alsa/asoundlib.h" HAVE_ALAS_ASOUNDLIB_H)
check_include_file("dlfcn.h" HAVE_DLFCN_H)
check_include_file("inttypes.h" HAVE_INTTYPES_H)
check_include_file("memory.h" HAVE_MEMORY_H)
check_include_file("stdbool.h" HAVE_STDBOOL_H)
check_include_file("stdint.h" HAVE_STDINT_H)
check_include_file("stdlib.h" HAVE_STDLIB_H)
check_include_file("strings.h" HAVE_STRINGS_H)
check_include_file("string.h" HAVE_STRING_H)
check_include_file("sys/stat.h" HAVE_SYS_STAT_H)
check_include_file("sys/times.h" HAVE_SYS_TIMES_H)
check_include_file("sys/types.h" HAVE_SYS_TYPES_H)
check_include_file("unistd.h" HAVE_UNISTD_H)

set(CMAKE_REQUIRED_INCLUDES "")
if(SRC_HAVE_INTTYPES_H)
	list(APPEND CMAKE_REQUIRED_INCLUDES "inttypes.h")
endif()
if(SRC_HAVE_MEMORY_H)
	list(APPEND CMAKE_REQUIRED_INCLUDES "memory.h")
endif()
if(SRC_HAVE_STDBOOL_H)
	list(APPEND CMAKE_REQUIRED_INCLUDES "stdbool.h")
endif()
if(SRC_HAVE_STDINT_H)
	list(APPEND CMAKE_REQUIRED_INCLUDES "stdint.h")
endif()
if(SRC_HAVE_STDLIB_H)
	list(APPEND CMAKE_REQUIRED_INCLUDES "stdlib.h")
endif()
if(SRC_HAVE_STRINGS_H)
	list(APPEND CMAKE_REQUIRED_INCLUDES "strings.h")
endif()
if(SRC_HAVE_STRING_H)
	list(APPEND CMAKE_REQUIRED_INCLUDES "string.h")
endif()
if(SRC_HAVE_SYS_STAT_H)
	list(APPEND CMAKE_REQUIRED_INCLUDES "sys/stat.h")
endif()
if(SRC_HAVE_SYS_TIMES_H)
	list(APPEND CMAKE_REQUIRED_INCLUDES "sys/times.h")
endif()
if(SRC_HAVE_SYS_TYPES_H)
	list(APPEND CMAKE_REQUIRED_INCLUDES "sys/types.h")
endif()
if(SRC_HAVE_UNISTD_H)
	list(APPEND CMAKE_REQUIRED_INCLUDES "unistd.h")
endif()

check_type_size("double" SIZEOF_DOUBLE)
check_type_size("float" SIZEOF_FLOAT)
check_type_size("int" SIZEOF_INT)
check_type_size("long" SIZEOF_LONG)

check_function_exists("ceil" HAVE_CEIL)
check_function_exists("floor" HAVE_FLOOR)
check_function_exists("fmod" HAVE_FMOD)
check_function_exists("lrint" HAVE_LRINT)
check_function_exists("lrintf" HAVE_LRINTF)
check_function_exists("free" HAVE_FREE)
check_function_exists("malloc" HAVE_MALLOC)
check_function_exists("memcpy" HAVE_MEMCPY)
check_function_exists("memmove" HAVE_MEMMOVE)
check_function_exists("calloc" HAVE_CALLOC)
check_function_exists("signal" HAVE_SIGNAL)
check_function_exists("alarm" HAVE_ALARM)

set(ENABLE_SINC_FAST_CONVERTER ON)
set(ENABLE_SINC_MEDIUM_CONVERTER ON)
set(ENABLE_SINC_BEST_CONVERTER ON)

if(WIN32)
	set(OS_IS_WIN32 1)
else()
	set(OS_IS_WIN32 0)
endif()
if(GCC)
	set(COMPILER_IS_GCC 1)
else()
	set(COMPILER_IS_GCC 0)
endif()

configure_file(
	templates/config.h.in
	${PROJECT_BINARY_DIR}/config.h
)

add_library(${PROJECT_NAME} OBJECT
	include/samplerate.h
	src/common.h
	src/fastest_coeffs.h
	src/high_qual_coeffs.h
	src/mid_qual_coeffs.h
	src/samplerate.c
	src/src_linear.c
	src/src_sinc.c
	src/src_zoh.c
	${PROJECT_BINARY_DIR}/config.h
)
target_compile_definitions(${PROJECT_NAME}
	PRIVATE
	HAVE_CONFIG_H
)
target_include_directories(${PROJECT_NAME}
	PRIVATE
		src
		include
		${PROJECT_BINARY_DIR}
	INTERFACE
		${PROJECT_SOURCE_DIR}/src
		${PROJECT_SOURCE_DIR}/include
		${PROJECT_BINARY_DIR}
)
target_compile_options(${PROJECT_NAME} PRIVATE
	# Use static CRT
	$<$<CONFIG:>:/MT>
	$<$<CONFIG:Debug>:/MTd>
	$<$<CONFIG:Release>:/MT>
	$<$<CONFIG:RelWithDebInfo>:/MT>
	$<$<CONFIG:MinSizeRel>:/MT>
)
