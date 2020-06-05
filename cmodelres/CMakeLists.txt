cmake_minimum_required(VERSION 3.1.0)

project(cmodel VERSION "1.0.0" LANGUAGES CXX C)
set(CMAKE_C_STANDARD 99)

find_package(Boost 1.65.0 QUIET REQUIRED COMPONENTS program_options)

# Loads in configurations.
set(
    MC_CMAKE_DIR
    "${CMAKE_CURRENT_LIST_DIR}/cmake"
    CACHE PATH
    "The the path to the cmake directory"
)
list(APPEND CMAKE_MODULE_PATH ${MC_CMAKE_DIR})

include(SmartAceOptions)
configure_smartace()

# Locates clang-format, if possible.
find_program(
    CLANG_FORMAT_EXE
    NAMES "clang-format-10" "clang-format-mp-10" "clang-format"
    DOC "Path to clang-format executable"
)

if(CLANG_FORMAT_EXE)
    message(STATUS "clang-format found: ${CLANG_FORMAT_EXE}")
else()
    message(WARNING "clang-format not found.")
endif()

# Adds dependencies...
add_subdirectory(libverify)

# Defines a list of all source files.
set(EXE_SRCS_COMMON cmodel.h primitive.h)
set(EXE_SRCS_C cmodel.c)
set(EXE_HARNESSED_C ${EXE_SRCS_C} harness.c)

# Defines how to generate the cpp files.
set(EXE_SRCS_CPP "")
foreach(fn ${EXE_SRCS_C})
    get_filename_component(raw_fn "${fn}" NAME_WE)
    set(dst_fn "${raw_fn}.cpp")
    set(dst_fn_full "${CMAKE_BINARY_DIR}/${dst_fn}")
    list(APPEND EXE_SRCS_CPP "${dst_fn_full}")
endforeach(fn)

set(EXE_HARNESSED_CPP "")
foreach(fn ${EXE_HARNESSED_C})
    get_filename_component(raw_fn "${fn}" NAME_WE)
    set(dst_fn "${raw_fn}.cpp")
    set(dst_fn_full "${CMAKE_BINARY_DIR}/${dst_fn}")
    list(APPEND EXE_HARNESSED_CPP "${dst_fn_full}")
    add_custom_command(
        OUTPUT ${dst_fn}
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${fn}" ${dst_fn_full}
        DEPENDS ${fn}
    )
endforeach(fn)

# Locates the files touched by clang-format.
set(EXE_SRCS_ABS "")
foreach(fn ${EXE_SRCS_COMMON})
    list(APPEND EXE_SRCS_ABS "${CMAKE_CURRENT_SOURCE_DIR}/${fn}")
endforeach(fn)
foreach(fn ${EXE_HARNESSED_C})
    list(APPEND EXE_SRCS_ABS "${CMAKE_CURRENT_SOURCE_DIR}/${fn}")
endforeach(fn)

# Add a clang-format target to format all source files.
if(CLANG_FORMAT_EXE)
    add_custom_target(
        run-clang-format
        COMMAND ${CLANG_FORMAT_EXE} -i ${EXE_SRCS_ABS}
    )
endif()

# Adds targets for each execution model.
include_directories(.)
include_directories(${PROJECT_BINARY_DIR})
include(Interactive)
include(Klee)
include(LibFuzzer)
include(Seahorn)
