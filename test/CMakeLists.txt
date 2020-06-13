file(GLOB sources "*.cpp")
file(GLOB headers "*.h")

file(GLOB contracts_sources "contracts/*.cpp")
file(GLOB contracts_headers "contracts/*.h")
file(GLOB libyul_sources "libyul/*.cpp")
file(GLOB libyul_headers "libyul/*.h")
file(GLOB libsolidity_sources "libsolidity/*.cpp")
file(GLOB libsolidity_headers "libsolidity/*.h")
file(GLOB libsolidity_util_sources "libsolidity/util/*.cpp")
file(GLOB libsolidity_util_headers "libsolidity/util/*.h")
file(GLOB libsolidity_modelcheck_sources "libsolidity/modelcheck/*.cpp")
file(GLOB libsolidity_modelcheck_analysis_sources
    "libsolidity/modelcheck/analysis/*.cpp")
file(GLOB libsolidity_modelcheck_codegen_sources
    "libsolidity/modelcheck/codegen/*.cpp")
file(GLOB libsolidity_modelcheck_model_sources
    "libsolidity/modelcheck/model/*.cpp")
file(GLOB libsolidity_modelcheck_utils_sources
    "libsolidity/modelcheck/utils/*.cpp")

add_executable(soltest ${sources} ${headers}
${contracts_sources} ${contracts_headers}
    ${libyul_sources} ${libyul_headers}
    ${libsolidity_sources} ${libsolidity_headers}
    ${libsolidity_util_sources} ${libsolidity_util_headers}
    ${libsolidity_modelcheck_sources}
    ${libsolidity_modelcheck_analysis_sources}
    ${libsolidity_modelcheck_codegen_sources}
    ${libsolidity_modelcheck_model_sources}
    ${libsolidity_modelcheck_utils_sources}
)
target_link_libraries(soltest PRIVATE libsolc yul solidity yulInterpreter evmasm
    devcore ${Boost_PROGRAM_OPTIONS_LIBRARIES}
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARIES}
)

if (NOT Boost_USE_STATIC_LIBS)
    target_compile_definitions(soltest PUBLIC -DBOOST_TEST_DYN_LINK)
endif()

add_subdirectory(tools)
