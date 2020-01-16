cmake_minimum_required(VERSION 3.1.0)

project(cmodel VERSION "1.0.0" LANGUAGES CXX C)
set(CMAKE_C_STANDARD 99)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

find_package(Boost 1.65.0 QUIET REQUIRED COMPONENTS program_options)

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

if(SEA_PATH)
    find_program(
	    SEA_EXE
	    NAMES sea
        PATHS ${SEA_PATH}
        NO_DEFAULT_PATH
    )
else()
    find_program(SEA_EXE NAMES sea)
endif()

if(SEA_EXE)
    message(STATUS "sea found: ${SEA_EXE}")
else()
    message(WARNING "Failed to find sea. See -DSEA_PATH.")
endif()

# Locates clang-format, if possible.
find_program(
    CLANG_FORMAT_EXE
    NAMES "clang-format"
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
set(EXE_SRCS_CPP cmodel.cpp)
set(EXE_HARNESSED_C ${EXE_SRCS_C} harness.c)
SET(EXE_HARNESSED_CPP ${EXE_SRCS_CPP} harness.cpp)

# Locates exactly all model files for seahorn and clang format.
# Locates seahorn deps.
set(EXE_SRCS_ABS "")
set(SEAHORN_DEPS "")
list(APPEND SEAHORN_DEPS "${CMAKE_CURRENT_SOURCE_DIR}/libverify/verify_seahorn.c")
foreach(fn ${EXE_SRCS_COMMON} ${EXE_SRCS_CPP})
    list(APPEND EXE_SRCS_ABS "${CMAKE_CURRENT_SOURCE_DIR}/${fn}")
endforeach(fn)
foreach(fn ${EXE_HARNESSED_C})
    list(APPEND SEAHORN_DEPS "${CMAKE_CURRENT_SOURCE_DIR}/${fn}")
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

add_executable(icmodel ${EXE_SRCS_COMMON} ${EXE_HARNESSED_CPP})
target_link_libraries(icmodel verify_interactive)

add_executable(fcmodel ${EXE_SRCS_COMMON} ${EXE_SRCS_CPP} libverify/verify_libfuzzer.cpp)
target_link_libraries(fcmodel -fsanitize=fuzzer,address)
set_target_properties(fcmodel PROPERTIES COMPILE_FLAGS "-g -fsanitize=fuzzer,address")

# Locates all dependencies of seahorn's libverify, along with all compiler flags.
set(CMODEL_COMPILE_DEFS "")
get_directory_property(CMODEL_COMPILE_DEFS_RAW DIRECTORY ${CMAKE_SOURCE_DIR} COMPILE_DEFINITIONS)
foreach(d ${CMODEL_COMPILE_DEFS_RAW})
    list(APPEND CMODEL_COMPILE_DEFS "-D${d}")
endforeach(d)

set(SEA_ARGS "" CACHE STRING "Additional arguments to pass to Seahorn.")
set(CMODEL_SEA_ARGS "${SEA_ARGS};--inline;--sea-dsa=cs;--dsa=sea-cs;--enable-indvar;--horn-global-constraints=true;--horn-singleton-aliases=true;--horn-use-write=true")

set(SEA_DEBUG OFF CACHE BOOL "Compiles Seahorn executables for debugging.")
if(SEA_DEBUG)
	set(CMODEL_SEA_ARGS "${CMODEL_SEA_ARGS};-g")
endif()

set(SEA_EXELOG OFF CACHE BOOL "Allows Seahorn executables to log input traces.")
if(SEA_EXELOG)
	list(APPEND CMODEL_COMPILE_DEFS "-DMC_LOG_ALL")
endif()

if(SEA_EXE)
    add_custom_target(
        verify
	COMMAND ${SEA_EXE} pf ${SEAHORN_DEPS} ${CMODEL_COMPILE_DEFS} ${CMODEL_SEA_ARGS} --show-invars
        SOURCES ${SEAHORN_DEPS}
        COMMAND_EXPAND_LISTS
    )
    add_custom_target(
        cex
	COMMAND ${SEA_EXE} pf ${SEAHORN_DEPS} ${CMODEL_COMPILE_DEFS} ${CMODEL_SEA_ARGS} --cex=cex.ll
        SOURCES ${SEAHORN_DEPS}
        COMMAND_EXPAND_LISTS
    )
    add_custom_target(
        cexcmodel
	COMMAND ${SEA_EXE} exe-cex ${SEAHORN_DEPS} ${CMODEL_COMPILE_DEFS} ${CMODEL_SEA_ARGS} -o cexcmodel
        COMMAND_EXPAND_LISTS
    )
endif()
