cmake_minimum_required(VERSION 3.0.0)

set(ETH_CMAKE_DIR   "${CMAKE_CURRENT_LIST_DIR}/cmake"   CACHE PATH "The the path to the cmake directory")
list(APPEND CMAKE_MODULE_PATH ${ETH_CMAKE_DIR})

include(EthToolchains)

# Set cmake_policies
include(EthPolicy)
eth_policy()

# project name and version should be set after cmake_policy CMP0048
set(PROJECT_VERSION "0.5.9")
project(solidity VERSION ${PROJECT_VERSION} LANGUAGES CXX C)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# option(LLL "Build LLL" OFF)
option(SOLC_LINK_STATIC "Link solc executable statically on supported platforms" OFF)
# option(LLLC_LINK_STATIC "Link lllc executable statically on supported platforms" OFF)
# option(INSTALL_LLLC "Include lllc executable in installation" ${LLL})

# Setup cccache.
include(EthCcache)

# Let's find our dependencies
include(EthDependencies)
include(jsoncpp)
include_directories(SYSTEM ${JSONCPP_INCLUDE_DIR})

find_package(Threads)

# Figure out what compiler and system are we using
include(EthCompilerSettings)

# Include utils
include(EthUtils)

# Create license.h from LICENSE.txt and template
# Converting to char array is required due to MSVC's string size limit.
file(READ ${CMAKE_SOURCE_DIR}/LICENSE.txt LICENSE_TEXT HEX)
string(REGEX MATCHALL ".." LICENSE_TEXT "${LICENSE_TEXT}")
string(REGEX REPLACE ";" ",\n\t0x" LICENSE_TEXT "${LICENSE_TEXT}")
set(LICENSE_TEXT "0x${LICENSE_TEXT}")

configure_file("${CMAKE_SOURCE_DIR}/cmake/templates/license.h.in" include/license.h)

include(EthOptions)
configure_project(TESTS)

include(McOptions)
configure_mc()

add_subdirectory(libdevcore)
add_subdirectory(liblangutil)
add_subdirectory(libevmasm)
add_subdirectory(libyul)
add_subdirectory(libsolidity)
add_subdirectory(libsolc)
add_subdirectory(libverify)

install(DIRECTORY libverify/ DESTINATION include/solc/libverify)
install(FILES cmodelres/CMakeLists.txt DESTINATION share/solc/project)
install(FILES cmodelres/Klee.cmake DESTINATION share/solc/project/cmake)
install(FILES cmodelres/Interactive.cmake DESTINATION share/solc/project/cmake)
install(FILES cmodelres/LibFuzzer.cmake DESTINATION share/solc/project/cmake)
install(FILES cmodelres/Seahorn.cmake DESTINATION share/solc/project/cmake)
install(FILES cmake/McOptions.cmake DESTINATION share/solc/project/cmake)

if (NOT EMSCRIPTEN)
	add_subdirectory(solc)
	# if (LLL)
	# 	add_subdirectory(liblll)
	# 	add_subdirectory(lllc)
	# endif()
endif()

if (TESTS AND NOT EMSCRIPTEN)
	add_subdirectory(test)
endif()
