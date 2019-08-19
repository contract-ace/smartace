set (sources_interactive verify.h assert_interactive.c nd_interactive.c)

add_library(libverify_interactive SHARED ${sources_interactive})
