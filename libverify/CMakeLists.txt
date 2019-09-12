# Builds a generator for the libverify header.
add_executable(sol_verify_h_generator sol_verify_h_generator.cpp)

# Generates the header, and ensures it is accessible to sub-projects.
include_directories(${PROJECT_BINARY_DIR})
add_custom_command(
    OUTPUT ${PROJECT_BINARY_DIR}/sol_verify.h
    COMMAND sol_verify_h_generator ${PROJECT_BINARY_DIR}/sol_verify.h
    DEPENDS sol_verify_h_generator
)

# Generates the verification library suite.
set(sources_interactive ${PROJECT_BINARY_DIR}/sol_verify.h
                        assert_interactive.c
                        nd_interactive.c)
add_library(verify_interactive SHARED ${sources_interactive})
