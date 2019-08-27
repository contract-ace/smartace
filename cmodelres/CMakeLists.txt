cmake_minimum_required(VERSION 3.1.0)

project(cmodel VERSION "1.0.0" LANGUAGES C)
set(CMAKE_C_STANDARD 99)

# Imports all runtime SO's.
set(INTERACTIVE_SO_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libverify_interactive.so")
add_library(verify_interactive SHARED IMPORTED)
set_property(
    TARGET verify_interactive
    PROPERTY IMPORTED_LOCATION "${INTERACTIVE_SO_PATH}"
)

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

# Defines the last of all source files.
set(EXE_SRCS cmodel.h cmodel.c primitive.h)

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
add_executable(icmodel ${EXE_SRCS})
target_link_libraries(icmodel verify_interactive)
