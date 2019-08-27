cmake_minimum_required(VERSION 3.1.0)

project(cmodel VERSION "1.0.0" LANGUAGES C)
set(CMAKE_C_STANDARD 99)

set(INTERACTIVE_SO_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libverify_interactive.so")
add_library(verify_interactive SHARED IMPORTED)
set_property(TARGET verify_interactive PROPERTY IMPORTED_LOCATION "${INTERACTIVE_SO_PATH}")

include_directories(.)
set(EXE_SOURCES cmodel.h cmodel.c primitive.h)
add_executable(icmodel ${EXE_SOURCES})
target_link_libraries(icmodel verify_interactive)

