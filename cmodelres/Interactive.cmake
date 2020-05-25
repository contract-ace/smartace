# A simple target to build the interactive model.
add_executable(icmodel ${EXE_SRCS_COMMON} ${EXE_HARNESSED_CPP})
target_link_libraries(icmodel verify_interactive)
