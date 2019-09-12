# Generates the header, and ensures it is accessible to sub-projects.
include_directories(${PROJECT_BINARY_DIR})
add_custom_command(
    OUTPUT ${PROJECT_BINARY_DIR}/sol_verify.h
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/verify.h
            ${PROJECT_BINARY_DIR}/sol_verify.h
    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/verify.h
)

# Generates the verification library suite.
set(sources_interactive ${PROJECT_BINARY_DIR}/sol_verify.h
                        assert_interactive.c
                        nd_interactive.c)
add_library(verify_interactive SHARED ${sources_interactive})
