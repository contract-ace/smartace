set (sources_interactive verify.h assert_interactive.c nd_interactive.c)

add_library(verify_interactive SHARED ${sources_interactive})
