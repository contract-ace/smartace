# Attempts to find Seahorn from the user provided SEA_PATH.
# If no path is found, cmake will search for the command `sea`.
set(SEA_PATH "" CACHE STRING "A path to the Seahorn executable, sea.")

if(SEA_PATH)
    find_program(
	    SEA_EXE
	    NAMES sea
        PATHS ${SEA_PATH}
        NO_DEFAULT_PATH
    )
else()
    find_program(SEA_EXE NAMES sea)
endif()

if(SEA_EXE)
    message(STATUS "sea found: ${SEA_EXE}")
else()
    message(WARNING "Failed to find sea. See -DSEA_PATH.")
endif()

# Locates the model files requried by Seahorn.
set(SEAHORN_DEPS "")
list(APPEND SEAHORN_DEPS "${CMAKE_CURRENT_SOURCE_DIR}/libverify/verify_seahorn.c")
foreach(fn ${EXE_HARNESSED_C})
    list(APPEND SEAHORN_DEPS "${CMAKE_CURRENT_SOURCE_DIR}/${fn}")
endforeach(fn)

# Locates all libverify(Seahorn) dependancies, and forwards all compiler flags.
set(CMODEL_COMPILE_DEFS "")
get_directory_property(CMODEL_COMPILE_DEFS_RAW DIRECTORY ${CMAKE_SOURCE_DIR} COMPILE_DEFINITIONS)
foreach(d ${CMODEL_COMPILE_DEFS_RAW})
    list(APPEND CMODEL_COMPILE_DEFS "-D${d}")
endforeach(d)

set(SEA_ARGS "" CACHE STRING "Additional arguments to pass to Seahorn.")
set(CMODEL_SEA_ARGS "${SEA_ARGS}")
list(APPEND CMODEL_SEA_ARGS "--inline")
list(APPEND CMODEL_SEA_ARGS "--sea-dsa=cs")
list(APPEND CMODEL_SEA_ARGS "--dsa=sea-cs")
list(APPEND CMODEL_SEA_ARGS "--enable-indvar")
list(APPEND CMODEL_SEA_ARGS "--horn-global-constraints=true")
list(APPEND CMODEL_SEA_ARGS "--horn-singleton-aliases=true")
list(APPEND CMODEL_SEA_ARGS "--horn-use-write=true")

set(SEA_DEBUG OFF CACHE BOOL "Compiles Seahorn executables for debugging.")
if(SEA_DEBUG)
    list(APPEND CMODEL_SEA_ARGS "-g")
endif()

set(SEA_EXELOG OFF CACHE BOOL "Allows Seahorn executables to log input traces.")
if(SEA_EXELOG)
	list(APPEND CMODEL_COMPILE_DEFS "-DMC_LOG_ALL")
endif()

# If all dependancies were located, adds all Seahorn targets.
if(SEA_EXE)
    add_custom_target(
        verify
        COMMAND ${SEA_EXE} pf ${SEAHORN_DEPS} ${CMODEL_COMPILE_DEFS} ${CMODEL_SEA_ARGS} --show-invars
        SOURCES ${SEAHORN_DEPS}
        COMMAND_EXPAND_LISTS
    )
    add_custom_target(
        cex
        COMMAND ${SEA_EXE} pf ${SEAHORN_DEPS} ${CMODEL_COMPILE_DEFS} ${CMODEL_SEA_ARGS} --cex=cex.ll
        SOURCES ${SEAHORN_DEPS}
        COMMAND_EXPAND_LISTS
    )
    add_custom_target(
        witness
        COMMAND ${SEA_EXE} exe-cex ${SEAHORN_DEPS} ${CMODEL_COMPILE_DEFS} ${CMODEL_SEA_ARGS} -o witness
        COMMAND_EXPAND_LISTS
    )
endif()
