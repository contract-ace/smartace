set(sources_common verify.h)
set(sources_interactive ${sources_common} verify_interactive.cpp)
set(sources_seahorn  ${sources_common} verify_seahorn.c)
set(sources_fuzz ${sources_common})

add_library(verify_interactive ${sources_interactive})
target_link_libraries(verify_interactive PRIVATE ${Boost_PROGRAM_OPTIONS_LIBRARIES})
