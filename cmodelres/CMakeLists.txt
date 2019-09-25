cmake_minimum_required(VERSION 3.1.0)

project(cmodel VERSION "1.0.0" LANGUAGES CXX C)
set(CMAKE_C_STANDARD 99)

# Loads in configurations.
set(
    MC_CMAKE_DIR
    "${CMAKE_CURRENT_LIST_DIR}/cmake"
    CACHE PATH
    "The the path to the cmake directory"
)
list(APPEND CMAKE_MODULE_PATH ${MC_CMAKE_DIR})

include(McOptions)
configure_mc()

# Locates Seahorn, as required for verification.
set(
	SEA_PATH
	""
	CACHE STRING
	"A path to the Seahorn executable, sea."
)

if(SEA_PATH STREQUAL "")
        message(FATAL_ERROR "-DSEA_PATH must be set.")
endif()

find_program(
	SEA_EXE
	NAMES sea
	PATHS ${SEA_PATH}
	NO_DEFAULT_PATH
)

if(NOT SEA_EXE)
        message(FATAL_ERROR "Failed to find sea at: ${SEA_PATH}")
else()
	message(STATUS "sea found: ${SEA_EXE}")
endif()

# Locates clang-format, if possible.
find_program(
    CLANG_FORMAT_EXE
    NAMES "clang-format"
    DOC "Path to clang-format executable"
)

if(NOT CLANG_FORMAT_EXE)
    message(STATUS "clang-format not found.")
else()
    message(STATUS "clang-format found: ${CLANG_FORMAT_EXE}")
endif()

# Adds dependencies...
add_subdirectory(libverify)

# Defines a list of all source files.
set(EXE_SRCS_COMMON cmodel.h primitive.h)
set(EXE_SRCS_C cmodel.c)
set(EXE_SRCS_CPP cmodel.cpp)
set(EXE_SRCS ${EXE_SRCS_COMMON} ${EXE_SRCS_C} ${EXE_SRCS_CPP})

# Add a clang-format target to format all source files.
set(EXE_SRCS_ABS "")
FOREACH(fn ${EXE_SRCS})
	LIST(APPEND EXE_SRCS_ABS "${CMAKE_CURRENT_SOURCE_DIR}/${fn}")
ENDFOREACH(fn)
add_custom_target(
    run-clang-format
    COMMAND ${CLANG_FORMAT_EXE} -i ${EXE_SRCS_ABS}
)

# Adds targets for each execution model.
include_directories(.)
include_directories(${PROJECT_BINARY_DIR})
add_executable(icmodel ${EXE_SRCS_COMMON} ${EXE_SRCS_CPP})
target_link_libraries(icmodel verify_interactive)
