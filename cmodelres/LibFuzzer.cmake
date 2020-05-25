# Links cmodel.c with the fuzzer harness.
add_executable(fuzztest ${EXE_SRCS_COMMON} ${EXE_SRCS_CPP} libverify/verify_libfuzzer.cpp)
target_link_libraries(fuzztest -fsanitize=fuzzer,address)
set_target_properties(fuzztest PROPERTIES COMPILE_FLAGS "-g -fsanitize=fuzzer,address")

# Adds a command to generate the corpus directory.
# This is where fuzzer results are cached.
set(CORPUS_DIR "corpus_dir")
set(CORPUS_DIR_FULL "${CMAKE_BINARY_DIR}/${CORPUS_DIR}")
add_custom_command(
    OUTPUT ${CORPUS_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CORPUS_DIR_FULL}
)

# User-facing command to generate fuzztest, and execute it with the default arguments.
set(CMODEL_FUZZ_ARGS "")
list(APPEND CMODEL_FUZZ_ARGS "-max_len=20000")
list(APPEND CMODEL_FUZZ_ARGS "-runs=1000000")
list(APPEND CMODEL_FUZZ_ARGS "-timeout=15")
list(APPEND CMODEL_FUZZ_ARGS "-use_value_profile=1")
list(APPEND CMODEL_FUZZ_ARGS "-print_final_stats=1")
add_custom_target(
    fuzz
    COMMAND "${CMAKE_BINARY_DIR}/fuzztest" ${CORPUS_DIR} ${CMODEL_FUZZ_ARGS}
    SOURCES ${CORPUS_DIR}
    COMMAND_EXPAND_LISTS
)
add_dependencies(fuzz fuzztest)
