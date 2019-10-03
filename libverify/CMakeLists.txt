set(sources_common verify.h)
set(sources_interactive ${sources_common} assert_interactive.cpp nd_interactive.cpp)
set(sources_seahorn  ${sources_common} assert_seahorn.c)
set(sources_fuzz ${sources_common})

set(sources_all ${sources_interactive} ${sources_seahorn} ${sources_fuzz} CMakeLists.txt)

add_library(verify_interactive ${sources_interactive})

file(COPY ${sources_all} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/integration)
